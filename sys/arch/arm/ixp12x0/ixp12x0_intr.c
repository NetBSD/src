/* $NetBSD: ixp12x0_intr.c,v 1.7 2003/03/25 06:12:46 igy Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: ixp12x0_intr.c,v 1.7 2003/03/25 06:12:46 igy Exp $");

/*
 * Interrupt support for the Intel ixp12x0
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/ixp12x0/ixp12x0reg.h> 
#include <arm/ixp12x0/ixp12x0var.h> 
#include <arm/ixp12x0/ixp12x0_comreg.h> 
#include <arm/ixp12x0/ixp12x0_comvar.h> 
#include <arm/ixp12x0/ixp12x0_pcireg.h> 

extern u_int32_t	ixpcom_cr;	/* current cr from *_com.c */
extern u_int32_t	ixpcom_imask;	/* tell mask to *_com.c */

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static u_int32_t imask[NIPL];
static u_int32_t pci_imask[NIPL];

/* Current interrupt priority level. */
__volatile int current_spl_level;  

/* Software copy of the IRQs we have enabled. */
__volatile u_int32_t intr_enabled;
__volatile u_int32_t pci_intr_enabled;

/* Interrupts pending. */
static __volatile int ipending;

/*
 * Map a software interrupt queue index (to the unused bits in the
 * ICU registers -- XXX will need to revisit this if those bits are
 * ever used in future steppings).
 */
static const u_int32_t si_to_irqbit[SI_NQUEUES] = {
	IXP12X0_INTR_bit30,		/* SI_SOFT */
	IXP12X0_INTR_bit29,		/* SI_SOFTCLOCK */
	IXP12X0_INTR_bit28,		/* SI_SOFTNET */
	IXP12X0_INTR_bit27,		/* SI_SOFTSERIAL */
};

#define	INT_SWMASK							\
	((1U << IXP12X0_INTR_bit30) | (1U << IXP12X0_INTR_bit29) |	\
	 (1U << IXP12X0_INTR_bit28) | (1U << IXP12X0_INTR_bit27))

#define	SI_TO_IRQBIT(si)	(1U << si_to_irqbit[(si)])

/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[SI_NQUEUES] = {
	IPL_SOFT,		/* SI_SOFT */
	IPL_SOFTCLOCK,		/* SI_SOFTCLOCK */
	IPL_SOFTNET,		/* SI_SOFTNET */
	IPL_SOFTSERIAL,		/* SI_SOFTSERIAL */
};

void	ixp12x0_intr_dispatch(struct irqframe *frame);

static __inline u_int32_t
ixp12x0_irq_read(void)
{
	return IXPREG(IXP12X0_IRQ_VBASE) & IXP12X0_INTR_MASK;
}

static __inline u_int32_t
ixp12x0_pci_irq_read(void)
{
	return IXPREG(IXPPCI_IRQ_STATUS);
}

static void
ixp12x0_enable_uart_irq(void)
{
	ixpcom_imask = 0;
	if (ixpcom_sc)
		bus_space_write_4(ixpcom_sc->sc_iot, ixpcom_sc->sc_ioh,
				  IXPCOM_CR, ixpcom_cr & ~ixpcom_imask);
}

static void
ixp12x0_disable_uart_irq(void)
{
	ixpcom_imask = CR_RIE | CR_XIE;
	if (ixpcom_sc)
		bus_space_write_4(ixpcom_sc->sc_iot, ixpcom_sc->sc_ioh,
				  IXPCOM_CR, ixpcom_cr & ~ixpcom_imask);
}

static void
ixp12x0_set_intrmask(u_int32_t irqs, u_int32_t pci_irqs)
{
	if (irqs & (1U << IXP12X0_INTR_UART)) {
		ixp12x0_disable_uart_irq();
	} else {
		ixp12x0_enable_uart_irq();
	}
	IXPREG(IXPPCI_IRQ_ENABLE_CLEAR) = pci_irqs;
	IXPREG(IXPPCI_IRQ_ENABLE_SET) = pci_intr_enabled & ~pci_irqs;
}

static void
ixp12x0_enable_irq(int irq)
{
	if (irq < SYS_NIRQ) {
		intr_enabled |= (1U << irq);
		switch (irq) {
		case IXP12X0_INTR_UART:
			ixp12x0_enable_uart_irq();
			break;

		case IXP12X0_INTR_PCI:
			/* nothing to do */
			break;
		default:
			panic("enable_irq:bad IRQ %d", irq);
		}
	} else {
		pci_intr_enabled |= (1U << (irq - SYS_NIRQ));
		IXPREG(IXPPCI_IRQ_ENABLE_SET) = (1U << (irq - SYS_NIRQ));
	}
}

static __inline void
ixp12x0_disable_irq(int irq)
{
	if (irq < SYS_NIRQ) {
		intr_enabled ^= ~(1U << irq);
		switch (irq) {
		case IXP12X0_INTR_UART:
			ixp12x0_disable_uart_irq();
			break;

		case IXP12X0_INTR_PCI:
			/* nothing to do */
			break;
		default:
			/* nothing to do */
		}
	} else {
		pci_intr_enabled &= ~(1U << (irq - SYS_NIRQ));
		IXPREG(IXPPCI_IRQ_ENABLE_CLEAR) = (1U << (irq - SYS_NIRQ));
	}
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
ixp12x0_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		ixp12x0_disable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int irqs = 0;
		int pci_irqs = 0;
		for (irq = 0; irq < SYS_NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				irqs |= (1U << irq);
		}
		imask[ipl] = irqs;
		for (irq = 0; irq < SYS_NIRQ; irq++) {
			if (intrq[irq + SYS_NIRQ].iq_levels & (1U << ipl))
				pci_irqs |= (1U << irq);
		}
		pci_imask[ipl] = pci_irqs;
	}

	imask[IPL_NONE] = 0;
	pci_imask[IPL_NONE] = 0;

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFT] = SI_TO_IRQBIT(SI_SOFT);
	imask[IPL_SOFTCLOCK] = SI_TO_IRQBIT(SI_SOFTCLOCK);
	imask[IPL_SOFTNET] = SI_TO_IRQBIT(SI_SOFTNET);
	imask[IPL_SOFTSERIAL] = SI_TO_IRQBIT(SI_SOFTSERIAL);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_SOFT];
	pci_imask[IPL_SOFTCLOCK] |= pci_imask[IPL_SOFT];

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	pci_imask[IPL_SOFTNET] |= pci_imask[IPL_SOFTCLOCK];

	/*
	 * Enforce a heirarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	pci_imask[IPL_BIO] |= pci_imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	pci_imask[IPL_NET] |= pci_imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	pci_imask[IPL_SOFTSERIAL] |= pci_imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];
	pci_imask[IPL_TTY] |= pci_imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];
	pci_imask[IPL_IMP] |= pci_imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	imask[IPL_AUDIO] |= imask[IPL_IMP];
	pci_imask[IPL_AUDIO] |= pci_imask[IPL_IMP];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];
	pci_imask[IPL_CLOCK] |= pci_imask[IPL_AUDIO];

	/*
	 * No separate statclock on the IXP12x0.
	 */
	imask[IPL_STATCLOCK] |= imask[IPL_CLOCK];
	pci_imask[IPL_STATCLOCK] |= pci_imask[IPL_CLOCK];

	/*
	 * splhigh() must block "everything".
	 */
	imask[IPL_HIGH] |= imask[IPL_STATCLOCK];
	pci_imask[IPL_HIGH] |= pci_imask[IPL_STATCLOCK];

	/*
	 * XXX We need serial drivers to run at the absolute highest priority
	 * in order to avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];
	pci_imask[IPL_SERIAL] |= pci_imask[IPL_HIGH];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int	irqs;
		int	pci_irqs;

		if (irq < SYS_NIRQ) {
			irqs = (1U << irq);
			pci_irqs = 0;
		} else {
			irqs = 0;
			pci_irqs = (1U << (irq - SYS_NIRQ));
		}
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			ixp12x0_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			irqs |= imask[ih->ih_ipl];
			pci_irqs |= pci_imask[ih->ih_ipl];
		}
		iq->iq_mask = irqs;
		iq->iq_pci_mask = pci_irqs;
	}
}

static void
ixp12x0_do_pending(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int	new;
	u_int	oldirqstate;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	new = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si)							\
	if ((ipending & ~imask[new]) & SI_TO_IRQBIT(si)) {		\
		ipending &= ~SI_TO_IRQBIT(si);				\
		current_spl_level = si_to_ipl[(si)];			\
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

__inline void
splx(int new)
{
	int	old;
	u_int	oldirqstate;

	if (current_spl_level == new)
		return;
	oldirqstate = disable_interrupts(I32_bit);
	old = current_spl_level;
	current_spl_level = new;
	ixp12x0_set_intrmask(imask[new], pci_imask[new]);
	restore_interrupts(oldirqstate);

	/* If there are software interrupts to process, do it. */
	if ((ipending & INT_SWMASK) & ~imask[new])
		ixp12x0_do_pending();
}

int
_splraise(int ipl)
{
	int	old = current_spl_level;

	if (old >= ipl)
		return (old);
	splx(ipl);
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

void
_setsoftintr(int si)
{
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	ipending |= SI_TO_IRQBIT(si);
	restore_interrupts(oldirqstate);

	/* Process unmasked pending soft interrupts. */
	if ((ipending & INT_SWMASK) & ~imask[current_spl_level])
		ixp12x0_do_pending();
}

/*
 * ixp12x0_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
ixp12x0_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;
	pci_intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "ipl %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				     NULL, "ixpintr", iq->iq_name);
	}
	current_intr_depth = 0;
	current_spl_level = 0;

	ixp12x0_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
ixp12x0_intr_establish(int irq, int ipl, int (*ih_func)(void *), void *arg)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;
#ifdef DEBUG
	printf("ixp12x0_intr_establish(irq=%d, ipl=%d, ih_func=%08x, arg=%08x)\n",
	       irq, ipl, (u_int32_t) ih_func, (u_int32_t) arg);
#endif
	if (irq < 0 || irq > NIRQ)
		panic("ixp12x0_intr_establish: IRQ %d out of range", ipl);
	if (ipl < 0 || ipl > NIPL)
		panic("ixp12x0_intr_establish: IPL %d out of range", ipl);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = ih_func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = ipl;

	iq = &intrq[irq];
	iq->iq_ist = IST_LEVEL;

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);
	ixp12x0_intr_calculate_masks();
	restore_interrupts(oldirqstate);

	return (ih);
}

void
ixp12x0_intr_disestablish(void *cookie)
{
	struct intrhand*	ih = cookie;
	struct intrq*		iq = &intrq[ih->ih_ipl];
	u_int			oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);
	ixp12x0_intr_calculate_masks();
	restore_interrupts(oldirqstate);
}

void
ixp12x0_intr_dispatch(struct clockframe *frame)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;
	int			pcpl;
	u_int32_t		hwpend;
	u_int32_t		pci_hwpend;
	int			irq;
	u_int32_t		ibit;

	pcpl = current_spl_level;

	hwpend = ixp12x0_irq_read();
	pci_hwpend = ixp12x0_pci_irq_read();

	while (hwpend) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			int ipl;
			current_spl_level = ipl = ih->ih_ipl;
			ixp12x0_set_intrmask(imask[ipl] | hwpend,
					     pci_imask[ipl] | pci_hwpend);
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
			hwpend &= ~ibit;
		}
	}
	while (pci_hwpend) {
		irq = ffs(pci_hwpend) - 1;
		ibit = (1U << irq);

		iq = &intrq[irq + SYS_NIRQ];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			int	ipl;

			current_spl_level = ipl = ih->ih_ipl;
			ixp12x0_set_intrmask(imask[ipl] | hwpend,
					     pci_imask[ipl] | pci_hwpend);
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
			pci_hwpend &= ~ibit;
		}
	}

	splx(pcpl);

	/* Check for pendings soft intrs. */
	if ((ipending & INT_SWMASK) & ~imask[pcpl]) {
		oldirqstate = enable_interrupts(I32_bit);
		ixp12x0_do_pending();
		restore_interrupts(oldirqstate);
	}
}
