/*	$NetBSD: i80321_icu.c,v 1.1 2002/03/27 21:45:47 thorpej Exp $	*/

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
 * Interrupt support for the Intel i80321 I/O Processor.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static int imask[NIPL];

/* Current interrupt priority level. */
__volatile int current_spl_level;  

/* Interrupts pending. */
static __volatile int ipending;

/* Software copy of the IRQs we have enabled. */
__volatile uint32_t intr_enabled;

/* Mask if interrupts steered to FIQs. */
uint32_t intr_steer;

/*
 * Map a software interrupt queue index (to the unused bits in the
 * ICU registers -- XXX will need to revisit this if those bits are
 * ever used in future steppings).
 */
static const uint32_t si_to_irqbit[SI_NQUEUES] = {
	ICU_INT_bit26,		/* SI_SOFT */
	ICU_INT_bit22,		/* SI_SOFTCLOCK */
	ICU_INT_bit5,		/* SI_SOFTNET */
	ICU_INT_bit4,		/* SI_SOFTSERIAL */
};

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

void	i80321_intr_dispatch(struct clockframe *frame);

static __inline uint32_t
i80321_iintsrc_read(void)
{
	uint32_t iintsrc;

	__asm __volatile("mrc p6, 0, %0, c8, c0, 0"
		: "=r" (iintsrc));

	/*
	 * The IINTSRC register shows bits that are active even
	 * if they are masked in INTCTL, so we have to mask them
	 * off with the interrupts we consider enabled.
	 */
	return (iintsrc & intr_enabled);
}

static __inline void
i80321_set_intrmask(void)
{

	__asm __volatile("mcr p6, 0, %0, c0, c0, 0"
		:
		: "r" (intr_enabled & ICU_INT_HWMASK));
}

static __inline void
i80321_set_intrsteer(void)
{

	__asm __volatile("mcr p6, 0, %0, c4, c0, 0"
		:
		: "r" (intr_steer & ICU_INT_HWMASK));
}

static __inline void
i80321_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	i80321_set_intrmask();
}

static __inline void
i80321_disable_irq(int irq)
{

	intr_enabled &= ~(1U << irq);
	i80321_set_intrmask();
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
i80321_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		i80321_disable_irq(irq);
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
			i80321_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			irqs |= imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

static void
i80321_do_pending(void)
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
	int oldirqstate, hwpend;

	current_spl_level = new;

	/*
	 * If there are pending HW interrupts which are being
	 * unmasked, then enable them in the INTCTL register.
	 * This will cause them to come flooding in.
	 */
	hwpend = (ipending & ICU_INT_HWMASK) & ~new;
	if (hwpend != 0) {
		oldirqstate = disable_interrupts(I32_bit);
		intr_enabled |= hwpend;
		i80321_set_intrmask();
		restore_interrupts(oldirqstate);
	}

	/* If there are software interrupts to process, do it. */
	if ((ipending & ~ICU_INT_HWMASK) & ~new)
		i80321_do_pending();
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
	if ((ipending & ~ICU_INT_HWMASK) & ~current_spl_level)
		i80321_do_pending();
}

/*
 * i80321_icu_init:
 *
 *	Initialize the i80321 ICU.  Called early in bootstrap
 *	to make sure the ICU is in a pristine state.
 */
void
i80321_icu_init(void)
{

	intr_enabled = 0;	/* All interrupts disabled */
	i80321_set_intrmask();

	intr_steer = 0;		/* All interrupts steered to IRQ */
	i80321_set_intrsteer();
}

/*
 * i80321_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
i80321_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "iop321", iq->iq_name);
	}

	i80321_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
i80321_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	u_int oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("i80321_intr_establish: IRQ %d out of range", irq);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	iq = &intrq[irq];

	/* All IOP321 interrupts are level-triggered. */
	iq->iq_ist = IST_LEVEL;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);

	i80321_intr_calculate_masks();

	restore_interrupts(oldirqstate);

	return (ih);
}

void
i80321_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &intrq[ih->ih_irq];
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);

	i80321_intr_calculate_masks();

	restore_interrupts(oldirqstate);
}

void
i80321_intr_dispatch(struct clockframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, pcpl, irq, ibit, hwpend;

	pcpl = current_spl_level;

	hwpend = i80321_iintsrc_read();

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	intr_enabled &= ~hwpend;
	i80321_set_intrmask();

	while (hwpend != 0) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (pcpl & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: the IRQ is already disabled.
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

		/* Re-enable this interrupt now that's it's cleared. */
		intr_enabled |= ibit;
		i80321_set_intrmask();
	}

	/* Check for pendings soft intrs. */
	if ((ipending & ~ICU_INT_HWMASK) & ~current_spl_level) {
		oldirqstate = enable_interrupts(I32_bit);
		i80321_do_pending();
		restore_interrupts(oldirqstate);
	}
}
