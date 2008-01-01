/*	$NetBSD: becc_icu.c,v 1.7.28.1 2008/01/01 15:39:43 chris Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
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
 * Interrupt support for the ADI Engineering Big Endian Companion Chip.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: becc_icu.c,v 1.7.28.1 2008/01/01 15:39:43 chris Exp $");

#ifndef EVBARM_SPL_NOINLINE
#define	EVBARM_SPL_NOINLINE
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <arm/cpufunc.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

#include <arm/xscale/i80200reg.h>
#include <arm/xscale/i80200var.h>

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
uint32_t becc_imask[NIPL];

/* Current interrupt priority level. */
volatile uint32_t current_spl_level;  

/* Interrupts pending. */
volatile uint32_t becc_ipending;
volatile uint32_t becc_sipending;

/* Software copy of the IRQs we have enabled. */
volatile uint32_t intr_enabled;

/* Mask if interrupts steered to FIQs. */
uint32_t intr_steer;

/*
 * Interrupt bit names.
 * XXX Some of these are BRH-centric.
 */
const char *becc_irqnames[] = {
	"soft",
	"timer A",
	"timer B",
	"irq 3",
	"irq 4",
	"irq 5",
	"irq 6",
	"diagerr",
	"DMA EOT",
	"DMA PERR",
	"DMA TABT",
	"DMA MABT",
	"irq 12",
	"irq 13",
	"irq 14",
	"irq 15",
	"PCI PERR",
	"irq 17",
	"irq 18",
	"PCI SERR",
	"PCI OAPE",
	"PCI OATA",
	"PCI OAMA",
	"irq 23",
	"irq 24",
	"irq 25",
	"irq 26",	/* PCI INTA */
	"irq 27",	/* PCI INTB */
	"irq 28",	/* PCI INTC */
	"irq 29",	/* PCI INTD */
	"pushbutton",
	"irq 31",
};

void	becc_intr_dispatch(struct irqframe *frame);

static inline uint32_t
becc_icsr_read(void)
{
	uint32_t icsr;

	icsr = BECC_CSR_READ(BECC_ICSR);

	/*
	 * The ICSR register shows bits that are active even if they are
	 * masked in ICMR, so we have to mask them off with the interrupts
	 * we consider enabled.
	 */
	return (icsr & intr_enabled);
}

static inline void
becc_set_intrsteer(void)
{

	BECC_CSR_WRITE(BECC_ICSTR, intr_steer & ICU_VALID_MASK);
	(void) BECC_CSR_READ(BECC_ICSTR);
}

static inline void
becc_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	becc_set_intrmask();
}

static inline void
becc_disable_irq(int irq)
{

	intr_enabled &= ~(1U << irq);
	becc_set_intrmask();
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
becc_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		becc_disable_irq(irq);
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
		becc_imask[ipl] = irqs;
	}

	becc_imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	becc_imask[IPL_SOFTCLOCK] = (1U << ICU_SOFT);
	becc_imask[IPL_SOFTNET] = (1U << ICU_SOFT);
	becc_imask[IPL_SOFTBIO] = (1U << ICU_SOFT);
	becc_imask[IPL_SOFTSERIAL] = (1U << ICU_SOFT);
	becc_imask[IPL_VM] |= becc_imask[IPL_SOFTSERIAL];
	becc_imask[IPL_SCHED] |= becc_imask[IPL_VM];
	becc_imask[IPL_HIGH] |= becc_imask[IPL_SCHED];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int irqs = (1U << irq);
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			becc_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			irqs |= becc_imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

void
splx(int new)
{

	becc_splx(new);
}

int
_spllower(int ipl)
{

	return (becc_spllower(ipl));
}

int
_splraise(int ipl)
{

	return (becc_splraise(ipl));
}

void
_setsoftintr(int si)
{

	becc_setsoftintr(si);
}

static const int si_to_ipl[SI_NQUEUES] = {
	IPL_SOFTCLOCK,		/* SI_SOFTCLOCK */
	IPL_SOFTBIO,		/* SI_SOFTBIO */
	IPL_SOFTNET,		/* SI_SOFTNET */
	IPL_SOFTSERIAL,		/* SI_SOFTSERIAL */
};               

int
becc_softint(void *arg)
{
#ifdef __HAVE_FAST_SOFTINTS
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	uint32_t	new, oldirqstate;

	/* Clear interrupt */
	BECC_CSR_WRITE(BECC_ICSR, 0);

	if (__cpu_simple_lock_try(&processing) == 0)
		return 0;

	oldirqstate = disable_interrupts(I32_bit);

	new = current_spl_level;

#define DO_SOFTINT(si)							\
	if (becc_sipending & (1 << (si))) {				\
		becc_sipending &= ~(1 << (si));				\
		current_spl_level |= becc_imask[si_to_ipl[(si)]];	\
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
#endif

	return 1;
}

/*
 * becc_icu_init:
 *
 *	Initialize the BECC ICU.  Called early in bootstrap
 *	to make sure the ICU is in a pristine state.
 */
void
becc_icu_init(void)
{

	intr_enabled = 0;	/* All interrupts disabled */
	becc_set_intrmask();

	intr_steer = 0;		/* All interrupts steered to IRQ */
	becc_set_intrsteer();

	i80200_extirq_dispatch = becc_intr_dispatch;

	i80200_intr_enable(INTCTL_IM);
}

/*
 * becc_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
becc_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "becc", becc_irqnames[i]);
	}

	becc_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
becc_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	uint32_t oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("becc_intr_establish: IRQ %d out of range", irq);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	iq = &intrq[irq];

	/* All BECC interrupts are level-triggered. */
	iq->iq_ist = IST_LEVEL;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);

	becc_intr_calculate_masks();

	restore_interrupts(oldirqstate);

	return (ih);
}

void
becc_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &intrq[ih->ih_irq];
	uint32_t oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);

	becc_intr_calculate_masks();

	restore_interrupts(oldirqstate);
}

void
becc_intr_dispatch(struct irqframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	uint32_t oldirqstate, pcpl, irq, ibit, hwpend;
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;
	pcpl = current_spl_level;
	hwpend = becc_icsr_read();

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	intr_enabled &= ~hwpend;
	becc_set_intrmask();

	while (hwpend != 0) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (pcpl & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: the IRQ is already disabled.
			 */
			becc_ipending |= ibit;
			continue;
		}

		becc_ipending &= ~ibit;

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
		becc_set_intrmask();
	}

	ci->ci_idepth--;

	if (becc_ipending & ~pcpl) {
		intr_enabled |= (becc_ipending & ~pcpl);
		becc_set_intrmask();
	}
}
