/* $NetBSD: ep93xx_intr.c,v 1.8.28.2 2008/01/20 16:03:58 chris Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jesse Off
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ep93xx_intr.c,v 1.8.28.2 2008/01/20 16:03:58 chris Exp $");

/*
 * Interrupt support for the Cirrus Logic EP93XX
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ep93xx/ep93xxreg.h> 
#include <arm/ep93xx/ep93xxvar.h> 

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static u_int32_t vic1_imask[NIPL];
static u_int32_t vic2_imask[NIPL];

/* Current interrupt priority level. */
volatile int current_spl_level;
volatile int hardware_spl_level;

/* Software copy of the IRQs we have enabled. */
volatile u_int32_t vic1_intr_enabled;
volatile u_int32_t vic2_intr_enabled;

/* Interrupts pending. */
static volatile int ipending;

#ifdef __HAVE_FAST_SOFTINTS
#define	SI_SOFTCLOCK	0
#define	SI_SOFTBIO	1
#define	SI_SOFTNET	2
#define	SI_SOFTSERIAL	3
/*
 * Map a software interrupt queue index (to the unused bits in the
 * VIC1 register -- XXX will need to revisit this if those bits are
 * ever used in future steppings).
 */
static const u_int32_t si_to_irqbit[] = {
	[SI_SOFTCLOCK] = EP93XX_INTR_bit30,
	[SI_SOFTBIO] = EP93XX_INTR_bit29,
	[SI_SOFTNET] = EP93XX_INTR_bit28,
	[SI_SOFTSERIAL] = EP93XX_INTR_bit27,
};

#define	INT_SWMASK							\
	((1U << EP93XX_INTR_bit30) | (1U << EP93XX_INTR_bit29) |	\
	 (1U << EP93XX_INTR_bit28) | (1U << EP93XX_INTR_bit27))

#define	SI_TO_IRQBIT(si)	(1U << si_to_irqbit[(si)])

/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[] = {
	[SI_SOFTCLOCK] = IPL_SOFTCLOCK,	
	[SI_SOFTBIO] = IPL_SOFTBIO,
	[SI_SOFTNET] = IPL_SOFTNET,
	[SI_SOFTSERIAl] = IPL_SOFTSERIAL,
};
#endif /* __HAVE_FAST_SOFTINTS */

void	ep93xx_intr_dispatch(struct irqframe *frame);

#define VIC1REG(reg)	*((volatile u_int32_t*) (EP93XX_AHB_VBASE + \
	EP93XX_AHB_VIC1 + (reg)))
#define VIC2REG(reg)	*((volatile u_int32_t*) (EP93XX_AHB_VBASE + \
	EP93XX_AHB_VIC2 + (reg)))

static void
ep93xx_set_intrmask(u_int32_t vic1_irqs, u_int32_t vic2_irqs)
{
	VIC1REG(EP93XX_VIC_IntEnClear) = vic1_irqs;
	VIC1REG(EP93XX_VIC_IntEnable) = vic1_intr_enabled & ~vic1_irqs;
	VIC2REG(EP93XX_VIC_IntEnClear) = vic2_irqs;
	VIC2REG(EP93XX_VIC_IntEnable) = vic2_intr_enabled & ~vic2_irqs;
}

static void
ep93xx_enable_irq(int irq)
{
	if (irq < VIC_NIRQ) {
		vic1_intr_enabled |= (1U << irq);
		VIC1REG(EP93XX_VIC_IntEnable) = (1U << irq);
	} else {
		vic2_intr_enabled |= (1U << (irq - VIC_NIRQ));
		VIC2REG(EP93XX_VIC_IntEnable) = (1U << (irq - VIC_NIRQ));
	}
}

static inline void
ep93xx_disable_irq(int irq)
{
	if (irq < VIC_NIRQ) {
		vic1_intr_enabled &= ~(1U << irq);
		VIC1REG(EP93XX_VIC_IntEnClear) = (1U << irq);
	} else {
		vic2_intr_enabled &= ~(1U << (irq - VIC_NIRQ));
		VIC2REG(EP93XX_VIC_IntEnClear) = (1U << (irq - VIC_NIRQ));
	}
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
ep93xx_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		ep93xx_disable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int vic1_irqs = 0;
		int vic2_irqs = 0;
		for (irq = 0; irq < VIC_NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				vic1_irqs |= (1U << irq);
		}
		vic1_imask[ipl] = vic1_irqs;
		for (irq = 0; irq < VIC_NIRQ; irq++) {
			if (intrq[irq + VIC_NIRQ].iq_levels & (1U << ipl))
				vic2_irqs |= (1U << irq);
		}
		vic2_imask[ipl] = vic2_irqs;
	}

	KASSERT(vic1_imask[IPL_NONE] == 0);
	KASSERT(vic2_imask[IPL_NONE] == 0);

#ifdef __HAVE_FAST_SOFTINTS
	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	vic1_imask[IPL_SOFT] = SI_TO_IRQBIT(SI_SOFT);
	vic1_imask[IPL_SOFTCLOCK] = SI_TO_IRQBIT(SI_SOFTCLOCK);
	vic1_imask[IPL_SOFTNET] = SI_TO_IRQBIT(SI_SOFTNET);
	vic1_imask[IPL_SOFTSERIAL] = SI_TO_IRQBIT(SI_SOFTSERIAL);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	vic1_imask[IPL_SOFTCLOCK] |= vic1_imask[IPL_SOFT];
	vic2_imask[IPL_SOFTCLOCK] |= vic2_imask[IPL_SOFT];

	/*
	 * splsoftbio() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	vic1_imask[IPL_SOFTBIO] |= vic1_imask[IPL_SOFTCLOCK];
	vic2_imask[IPL_SOFTBIO] |= vic2_imask[IPL_SOFTCLOCK];

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	vic1_imask[IPL_SOFTNET] |= vic1_imask[IPL_SOFTBIO];
	vic2_imask[IPL_SOFTNET] |= vic2_imask[IPL_SOFTBIO];

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	vic1_imask[IPL_SOFTSERIAL] |= vic1_imask[IPL_SOFTNET];
	vic2_imask[IPL_SOFTSERIAL] |= vic2_imask[IPL_SOFTNET];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	vic1_imask[IPL_VM] |= vic1_imask[IPL_SOFTSERIAL];
	vic2_imask[IPL_VM] |= vic2_imask[IPL_SOFTSERIAL];
#endif /* __HAVE_FAST_SOFTINTS */

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	vic1_imask[IPL_CLOCK] |= vic1_imask[IPL_VM];
	vic2_imask[IPL_CLOCK] |= vic2_imask[IPL_VM];

	/*
	 * splhigh() must block "everything".
	 */
	vic1_imask[IPL_HIGH] |= vic1_imask[IPL_CLOCK];
	vic2_imask[IPL_HIGH] |= vic2_imask[IPL_CLOCK];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int	vic1_irqs;
		int	vic2_irqs;

		if (irq < VIC_NIRQ) {
			vic1_irqs = (1U << irq);
			vic2_irqs = 0;
		} else {
			vic1_irqs = 0;
			vic2_irqs = (1U << (irq - VIC_NIRQ));
		}
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			ep93xx_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			vic1_irqs |= vic1_imask[ih->ih_ipl];
			vic2_irqs |= vic2_imask[ih->ih_ipl];
		}
		iq->iq_vic1_mask = vic1_irqs;
		iq->iq_vic2_mask = vic2_irqs;
	}
}

#ifdef __HAVE_FAST_SOFTINTS
static void
ep93xx_do_pending(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int	new;
	u_int	oldirqstate, oldirqstate2;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	new = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si)							\
	if ((ipending & ~vic1_imask[new]) & SI_TO_IRQBIT(si)) {		\
		ipending &= ~SI_TO_IRQBIT(si);				\
		current_spl_level = si_to_ipl[(si)];			\
		oldirqstate2 = enable_interrupts(I32_bit);		\
		softintr_dispatch(si);					\
		restore_interrupts(oldirqstate2);			\
		current_spl_level = new;				\
	}

	DO_SOFTINT(SI_SOFTSERIAL);
	DO_SOFTINT(SI_SOFTNET);
	DO_SOFTINT(SI_SOFTCLOCK);
	DO_SOFTINT(SI_SOFT);

	__cpu_simple_unlock(&processing);

	restore_interrupts(oldirqstate);
}
#endif

inline void
splx(int new)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = current_spl_level;
	current_spl_level = new;
	if (new != hardware_spl_level) {
		hardware_spl_level = new;
		ep93xx_set_intrmask(vic1_imask[new], vic2_imask[new]);
	}
	restore_interrupts(oldirqstate);

#ifdef __HAVE_FAST_SOFTINTS
	/* If there are software interrupts to process, do it. */
	if ((ipending & INT_SWMASK) & ~vic1_imask[new])
		ep93xx_do_pending();
#endif
}

int
_splraise(int ipl)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = current_spl_level;
	current_spl_level = ipl;
	restore_interrupts(oldirqstate);
	return (old);
}

int
_spllower(int ipl)
{
	int	old = current_spl_level;

	if (old <= ipl)
		return (old);
	splx(ipl);
	return (old);
}

#ifdef __HAVE_FAST_SOFTINTS
void
_setsoftintr(int si)
{
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	ipending |= SI_TO_IRQBIT(si);
	restore_interrupts(oldirqstate);

	/* Process unmasked pending soft interrupts. */
	if ((ipending & INT_SWMASK) & ~vic1_imask[current_spl_level])
		ep93xx_do_pending();
}
#endif

/*
 * ep93xx_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
ep93xx_intr_init(void)
{
	struct intrq *iq;
	int i;

	vic1_intr_enabled = 0;
	vic2_intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				     NULL, (i < VIC_NIRQ ? "vic1" : "vic2"),
		                     iq->iq_name);
	}
	current_spl_level = 0;
	hardware_spl_level = 0;

	/* All interrupts should use IRQ not FIQ */
	VIC1REG(EP93XX_VIC_IntSelect) = 0;
	VIC2REG(EP93XX_VIC_IntSelect) = 0;

	ep93xx_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
ep93xx_intr_establish(int irq, int ipl, int (*ih_func)(void *), void *arg)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("ep93xx_intr_establish: IRQ %d out of range", irq);
	if (ipl < 0 || ipl > NIPL)
		panic("ep93xx_intr_establish: IPL %d out of range", ipl);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = ih_func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = ipl;

	iq = &intrq[irq];

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);
	ep93xx_intr_calculate_masks();
	restore_interrupts(oldirqstate);

	return (ih);
}

void
ep93xx_intr_disestablish(void *cookie)
{
	struct intrhand*	ih = cookie;
	struct intrq*		iq = &intrq[ih->ih_irq];
	u_int			oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);
	ep93xx_intr_calculate_masks();
	restore_interrupts(oldirqstate);
}

void
ep93xx_intr_dispatch(struct irqframe *frame)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;
	int			pcpl;
	u_int32_t		vic1_hwpend;
	u_int32_t		vic2_hwpend;
	int			irq;

	pcpl = current_spl_level;

	vic1_hwpend = VIC1REG(EP93XX_VIC_IRQStatus);
	vic2_hwpend = VIC2REG(EP93XX_VIC_IRQStatus);

	hardware_spl_level = pcpl;
	ep93xx_set_intrmask(vic1_imask[pcpl] | vic1_hwpend,
			     vic2_imask[pcpl] | vic2_hwpend);

	vic1_hwpend &= ~vic1_imask[pcpl];
	vic2_hwpend &= ~vic2_imask[pcpl];

	if (vic1_hwpend) {
		irq = ffs(vic1_hwpend) - 1;

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			current_spl_level = ih->ih_ipl;
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
		}
	} else if (vic2_hwpend) {
		irq = ffs(vic2_hwpend) - 1;

		iq = &intrq[irq + VIC_NIRQ];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			current_spl_level = ih->ih_ipl;
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
		}
	}

	current_spl_level = pcpl;
	hardware_spl_level = pcpl;
	ep93xx_set_intrmask(vic1_imask[pcpl], vic2_imask[pcpl]);

#ifdef __HAVE_FAST_SOFTINTS
	/* Check for pendings soft intrs. */
	if ((ipending & INT_SWMASK) & ~vic1_imask[pcpl]) {
		ep93xx_do_pending();
	}
#endif
}
