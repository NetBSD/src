/*	$NetBSD: iq80310_intr.c,v 1.6.2.4 2002/03/16 15:57:27 jdolecek Exp $	*/

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

#if defined(IOP310_TEAMASA_NPWR)
/*
 * We have 5 interrupt source bits -- all in XINT3.  All interrupts
 * can be masked in the CPLD.
 */
#define	IRQ_BITS		0x1f
#define	IRQ_BITS_ALWAYS_ON	0x00
#else /* Default to stock IQ80310 */
/*
 * We have 8 interrupt source bits -- 5 in the XINT3 register, and 3
 * in the XINT0 register (the upper 3).  Note that the XINT0 IRQs
 * (SPCI INTA, INTB, and INTC) are always enabled, since they can not
 * be masked out in the CPLD (it provides only status, not masking,
 * for those interrupts).
 */
#define	IRQ_BITS		0xff
#define	IRQ_BITS_ALWAYS_ON	0xe0
#define	IRQ_READ_XINT0		1	/* XXX only if board rev >= F */
#endif /* list of IQ80310-based designs */

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static int imask[NIPL];

/* Current interrupt priority level. */
__volatile int current_spl_level;  

/* Interrupts pending. */
static __volatile int ipending;

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

void	iq80310_intr_dispatch(struct clockframe *frame);

static __inline uint32_t
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

static __inline void
iq80310_set_intrmask(void)
{
	uint32_t disabled;

	intr_enabled |= IRQ_BITS_ALWAYS_ON;

	/* The XINT_MASK register sets a bit to *disable*. */
	disabled = (~intr_enabled) & IRQ_BITS;

	CPLD_WRITE(IQ80310_XINT_MASK, disabled & 0x1f);
}

static __inline void
iq80310_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	iq80310_set_intrmask();
}

static __inline void
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
		imask[ipl] = irqs;
	}

	imask[IPL_NONE] = 0;

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

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];

	/*
	 * Enforce a heirarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	imask[IPL_AUDIO] |= imask[IPL_IMP];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * No separate statclock on the IQ80310.
	 */
	imask[IPL_STATCLOCK] |= imask[IPL_CLOCK];

	/*
	 * splhigh() must block "everything".
	 */
	imask[IPL_HIGH] |= imask[IPL_STATCLOCK];

	/*
	 * XXX We need serial drivers to run at the absolute highest priority
	 * in order to avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];

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
			irqs |= imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

static void
iq80310_do_pending(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int new, oldirqstate;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	new = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si)							\
	if ((ipending & ~new) & SI_TO_IRQBIT(si)) {			\
		ipending &= ~SI_TO_IRQBIT(si);				\
		current_spl_level |= imask[si_to_ipl[(si)]];		\
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
	int old, oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = current_spl_level;
	current_spl_level |= imask[ipl];

	restore_interrupts(oldirqstate);

	return (old);
}

__inline void
splx(int new)
{
	int old;

	old = current_spl_level;
	current_spl_level = new;

	/*
	 * If there are pending hardware interrupts (i.e. the
	 * external interrupt is disabled in the ICU), and all
	 * hardware interrupts are being unblocked, then re-enable
	 * the external hardware interrupt.
	 *
	 * XXX We have to wait for ALL hardware interrupts to
	 * XXX be unblocked, because we currently lose if we
	 * XXX get nested interrupts, and I don't know why yet.
	 */
	if ((new & IRQ_BITS) == 0 && (ipending & IRQ_BITS))
		i80200_intr_enable(INTCTL_IM);

	/* If there are software interrupts to process, do it. */
	if ((ipending & ~IRQ_BITS) & ~new)
		iq80310_do_pending();
}

int
_spllower(int ipl)
{
	int old = current_spl_level;

	splx(imask[ipl]);
	return (old);
}

void
_setsoftintr(int si)
{
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	ipending |= SI_TO_IRQBIT(si);
	restore_interrupts(oldirqstate);

	/* Process unmasked pending soft interrupts. */
	if ((ipending & ~IRQ_BITS) & ~current_spl_level)
		iq80310_do_pending();
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
	i80200_intr_enable(INTCTL_IM);

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
iq80310_intr_dispatch(struct clockframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, pcpl, irq, ibit, hwpend;

	/* First, disable external IRQs. */
	i80200_intr_disable(INTCTL_IM);

	pcpl = current_spl_level;

	for (hwpend = iq80310_intstat_read(); hwpend != 0;) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (pcpl & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: external IRQs are already
			 * disabled.
			 */
			ipending |= ibit;
			continue;
		}

		ipending &= ~ibit;

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		current_spl_level |= iq->iq_mask;
		oldirqstate = enable_interrupts(I32_bit);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
		}
		restore_interrupts(oldirqstate);

		current_spl_level = pcpl;
	}

	/* Check for pendings soft intrs. */
	if ((ipending & ~IRQ_BITS) & ~current_spl_level) {
		oldirqstate = enable_interrupts(I32_bit);
		iq80310_do_pending();
		restore_interrupts(oldirqstate);
	}

	/*
	 * If no hardware interrupts are masked, re-enable external
	 * interrupts.
	 */
	if ((ipending & IRQ_BITS) == 0)
		i80200_intr_enable(INTCTL_IM);
}
