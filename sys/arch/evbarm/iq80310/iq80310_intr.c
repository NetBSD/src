/*	$NetBSD: iq80310_intr.c,v 1.23 2005/12/24 20:06:59 perry Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iq80310_intr.c,v 1.23 2005/12/24 20:06:59 perry Exp $");

#ifndef EVBARM_SPL_NOINLINE
#define	EVBARM_SPL_NOINLINE
#endif

/*
 * Interrupt support for the Intel IQ80310.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/xscale/i80200reg.h>
#include <arm/xscale/i80200var.h>

#include <evbarm/iq80310/iq80310reg.h>
#include <evbarm/iq80310/iq80310var.h>
#include <evbarm/iq80310/obiovar.h>

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
int iq80310_imask[NIPL];

/* Current interrupt priority level. */
volatile int current_spl_level;  

/* Interrupts pending. */
volatile int iq80310_ipending;

/* Software copy of the IRQs we have enabled. */
uint32_t intr_enabled;

/*
 * Map a software interrupt queue index (at the top of the word, and
 * highest priority softintr is encountered first in an ffs()).
 */
#define	SI_TO_IRQBIT(si)	(1U << (31 - (si)))

/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[SI_NQUEUES] = {
	IPL_SOFT,		/* SI_SOFT */
	IPL_SOFTCLOCK,		/* SI_SOFTCLOCK */
	IPL_SOFTNET,		/* SI_SOFTNET */
	IPL_SOFTSERIAL,		/* SI_SOFTSERIAL */
};

void	iq80310_intr_dispatch(struct irqframe *frame);

static inline uint32_t
iq80310_intstat_read(void)
{
	uint32_t intstat;

	intstat = CPLD_READ(IQ80310_XINT3_STATUS) & 0x1f;
#if defined(IRQ_READ_XINT0)
	if (IRQ_READ_XINT0)
		intstat |= (CPLD_READ(IQ80310_XINT0_STATUS) & 0x7) << 5;
#endif

	/* XXX Why do we have to mask off? */
	return (intstat & intr_enabled);
}

static inline void
iq80310_set_intrmask(void)
{
	uint32_t disabled;

	intr_enabled |= IRQ_BITS_ALWAYS_ON;

	/* The XINT_MASK register sets a bit to *disable*. */
	disabled = (~intr_enabled) & IRQ_BITS;

	CPLD_WRITE(IQ80310_XINT_MASK, disabled & 0x1f);
}

static inline void
iq80310_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	iq80310_set_intrmask();
}

static inline void
iq80310_disable_irq(int irq)
{

	intr_enabled &= ~(1U << irq);
	iq80310_set_intrmask();
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
iq80310_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		iq80310_disable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int irqs = 0;
		for (irq = 0; irq < NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				irqs |= (1U << irq);
		}
		iq80310_imask[ipl] = irqs;
	}

	iq80310_imask[IPL_NONE] = 0;

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	iq80310_imask[IPL_SOFT] = SI_TO_IRQBIT(SI_SOFT);
	iq80310_imask[IPL_SOFTCLOCK] = SI_TO_IRQBIT(SI_SOFTCLOCK);
	iq80310_imask[IPL_SOFTNET] = SI_TO_IRQBIT(SI_SOFTNET);
	iq80310_imask[IPL_SOFTSERIAL] = SI_TO_IRQBIT(SI_SOFTSERIAL);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	iq80310_imask[IPL_SOFTCLOCK] |= iq80310_imask[IPL_SOFT];

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	iq80310_imask[IPL_SOFTNET] |= iq80310_imask[IPL_SOFTCLOCK];

	/*
	 * Enforce a heirarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	iq80310_imask[IPL_BIO] |= iq80310_imask[IPL_SOFTNET];
	iq80310_imask[IPL_NET] |= iq80310_imask[IPL_BIO];
	iq80310_imask[IPL_SOFTSERIAL] |= iq80310_imask[IPL_NET];
	iq80310_imask[IPL_TTY] |= iq80310_imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	iq80310_imask[IPL_VM] |= iq80310_imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	iq80310_imask[IPL_AUDIO] |= iq80310_imask[IPL_VM];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	iq80310_imask[IPL_CLOCK] |= iq80310_imask[IPL_AUDIO];

	/*
	 * No separate statclock on the IQ80310.
	 */
	iq80310_imask[IPL_STATCLOCK] |= iq80310_imask[IPL_CLOCK];

	/*
	 * splhigh() must block "everything".
	 */
	iq80310_imask[IPL_HIGH] |= iq80310_imask[IPL_STATCLOCK];

	/*
	 * XXX We need serial drivers to run at the absolute highest priority
	 * in order to avoid overruns, so serial > high.
	 */
	iq80310_imask[IPL_SERIAL] |= iq80310_imask[IPL_HIGH];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int irqs = (1U << irq);
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			iq80310_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			irqs |= iq80310_imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

void
iq80310_do_soft(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int new, oldirqstate;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	new = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si)							\
	if ((iq80310_ipending & ~new) & SI_TO_IRQBIT(si)) {		\
		iq80310_ipending &= ~SI_TO_IRQBIT(si);			\
		current_spl_level |= iq80310_imask[si_to_ipl[(si)]];	\
		restore_interrupts(oldirqstate);			\
		softintr_dispatch(si);					\
		oldirqstate = disable_interrupts(I32_bit);		\
		current_spl_level = new;				\
	}

	DO_SOFTINT(SI_SOFTSERIAL);
	DO_SOFTINT(SI_SOFTNET);
	DO_SOFTINT(SI_SOFTCLOCK);
	DO_SOFTINT(SI_SOFT);

	__cpu_simple_unlock(&processing);

	restore_interrupts(oldirqstate);
}

int
_splraise(int ipl)
{

	return (iq80310_splraise(ipl));
}

inline void
splx(int new)
{

	return (iq80310_splx(new));
}

int
_spllower(int ipl)
{

	return (iq80310_spllower(ipl));
}

void
_setsoftintr(int si)
{
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	iq80310_ipending |= SI_TO_IRQBIT(si);
	restore_interrupts(oldirqstate);

	/* Process unmasked pending soft interrupts. */
	if ((iq80310_ipending & ~IRQ_BITS) & ~current_spl_level)
		iq80310_do_soft();
}

void
iq80310_intr_init(void)
{
	struct intrq *iq;
	int i;

	/*
	 * The Secondary PCI interrupts INTA, INTB, and INTC
	 * area always enabled, since they cannot be masked
	 * in the CPLD.
	 */
	intr_enabled |= IRQ_BITS_ALWAYS_ON;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "iq80310", iq->iq_name);
	}

	iq80310_intr_calculate_masks();

	/* Enable external interrupts on the i80200. */
	i80200_extirq_dispatch = iq80310_intr_dispatch;
	i80200_intr_enable(INTCTL_IM | INTCTL_PM);

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
iq80310_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	u_int oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("iq80310_intr_establish: IRQ %d out of range", irq);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	iq = &intrq[irq];

	/* All IQ80310 interrupts are level-triggered. */
	iq->iq_ist = IST_LEVEL;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);

	iq80310_intr_calculate_masks();

	restore_interrupts(oldirqstate);

	return (ih);
}

void
iq80310_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &intrq[ih->ih_irq];
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);

	iq80310_intr_calculate_masks();

	restore_interrupts(oldirqstate);
}

void
iq80310_intr_dispatch(struct irqframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, pcpl, irq, ibit, hwpend, rv, stray;

	stray = 1;

	/* First, disable external IRQs. */
	i80200_intr_disable(INTCTL_IM | INTCTL_PM);

	pcpl = current_spl_level;

	for (hwpend = iq80310_intstat_read(); hwpend != 0;) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		stray = 0;

		hwpend &= ~ibit;

		if (pcpl & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: external IRQs are already
			 * disabled.
			 */
			iq80310_ipending |= ibit;
			continue;
		}

		iq80310_ipending &= ~ibit;
		rv = 0;

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		current_spl_level |= iq->iq_mask;
		oldirqstate = enable_interrupts(I32_bit);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			rv |= (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
		}
		restore_interrupts(oldirqstate);

		current_spl_level = pcpl;

#if 0 /* XXX */
		if (rv == 0)
			printf("Stray interrupt: IRQ %d\n", irq);
#endif
	}

#if 0 /* XXX */
	if (stray)
		printf("Stray external interrupt\n");
#endif

	/* Check for pendings soft intrs. */
	if ((iq80310_ipending & ~IRQ_BITS) & ~current_spl_level) {
		oldirqstate = enable_interrupts(I32_bit);
		iq80310_do_soft();
		restore_interrupts(oldirqstate);
	}

	/*
	 * If no hardware interrupts are masked, re-enable external
	 * interrupts.
	 */
	if ((iq80310_ipending & IRQ_BITS) == 0)
		i80200_intr_enable(INTCTL_IM | INTCTL_PM);
}
