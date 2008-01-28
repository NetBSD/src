/*	$NetBSD: ifpga_intr.c,v 1.5.28.2 2008/01/28 18:29:11 matt Exp $	*/

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

#ifndef EVBARM_SPL_NOINLINE
#define	EVBARM_SPL_NOINLINE
#endif

/*
 * Interrupt support for the Integrator FPGA.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <arm/cpufunc.h>

#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgavar.h>

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
int ifpga_imask[NIPL];

/* Interrupts pending. */
volatile int ifpga_ipending;

/* Software copy of the IRQs we have enabled. */
volatile uint32_t intr_enabled;

/* Mask if interrupts steered to FIQs. */
uint32_t intr_steer;

/*
 * Interrupt bit names.
 */
const char * const ifpga_irqnames[] = {
	"soft",		/* 0 */
	"uart 0",	/* 1 */
	"uart 1",	/* 2 */
	"kbd",		/* 3 */
	"mouse",	/* 4 */
	"tmr 0",	/* 5 */
	"tmr 1 hard",	/* 6 */
	"tmr 2 stat",	/* 7 */
	"rtc",		/* 8 */
	"exp 0",	/* 9 */
	"exp 1",	/* 10 */
	"exp 2",	/* 11 */
	"exp 3",	/* 12 */
	"pci 0",	/* 13 */
	"pci 1",	/* 14 */
	"pci 2",	/* 15 */
	"pci 3",	/* 16 */
	"V3 br",	/* 17 */
	"deg",		/* 18 */
	"enum",		/* 19 */
	"pci lb",	/* 20 */
	"autoPC",	/* 21 */
	"irq 22",	/* 22 */
	"irq 23",	/* 23 */
	"irq 24",	/* 24 */
	"irq 25",	/* 25 */
	"irq 26",	/* 26 */
	"irq 27",	/* 27 */
	"irq 28",	/* 28 */
	"irq 29",	/* 29 */
	"irq 30",	/* 30 */
	"irq 31",	/* 31 */
};

void	ifpga_intr_dispatch(struct clockframe *frame);

extern struct ifpga_softc *ifpga_sc;

static inline uint32_t
ifpga_iintsrc_read(void)
{
	return bus_space_read_4(ifpga_sc->sc_iot, ifpga_sc->sc_irq_ioh,
	    IFPGA_INTR_STATUS);
}

static inline void
ifpga_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	ifpga_set_intrmask();
}

static inline void
ifpga_disable_irq(int irq)
{

	intr_enabled &= ~(1U << irq);
	ifpga_set_intrmask();
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
ifpga_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		ifpga_disable_irq(irq);
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
		ifpga_imask[ipl] = irqs;
	}

	KASSERT(ifpga_imask[IPL_NONE] == 0);

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	ifpga_imask[IPL_VM] |= 0;
	ifpga_imask[IPL_SCHED] |= ifpga_imask[IPL_VM];
	ifpga_imask[IPL_HIGH] |= ifpga_imask[IPL_SCHED];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < NIRQ; irq++) {
		int irqs = (1U << irq);
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			ifpga_enable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			irqs |= ifpga_imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

void
splx(int new)
{

	ifpga_splx(new);
}

int
_spllower(int ipl)
{

	return (ifpga_spllower(ipl));
}

int
_splraise(int ipl)
{

	return (ifpga_splraise(ipl));
}

/*
 * ifpga_intr_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
ifpga_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "ifpga", ifpga_irqnames[i]);
	}
}

void
ifpga_intr_postinit(void)
{
	ifpga_intr_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
ifpga_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	u_int oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("ifpga_intr_establish: IRQ %d out of range", irq);

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

	ifpga_intr_calculate_masks();

	restore_interrupts(oldirqstate);

	return (ih);
}

void
ifpga_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &intrq[ih->ih_irq];
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);

	ifpga_intr_calculate_masks();

	restore_interrupts(oldirqstate);
}

void
ifpga_intr_dispatch(struct clockframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, pcpl, irq, ibit, hwpend;
	struct cpu_info * const ci = curcpu();

	pcpl = ci->ci_cpl;

	hwpend = ifpga_iintsrc_read();

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	intr_enabled &= ~hwpend;
	ifpga_set_intrmask();

	/* Wait for these interrupts to be suppressed.  */
	while ((ifpga_iintsrc_read() & hwpend) != 0)
	    ;

	while (hwpend != 0) {
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (pcpl & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: the IRQ is already disabled.
			 */
			ifpga_ipending |= ibit;
			continue;
		}

		ifpga_ipending &= ~ibit;

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
		ci->ci_cpl |= iq->iq_mask;
		oldirqstate = enable_interrupts(I32_bit);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list)) {
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
		}
		restore_interrupts(oldirqstate);
		ci->ci_cpl = pcpl;

		hwpend |= (ifpga_ipending & IFPGA_INTR_HWMASK) & ~pcpl;

		/* Re-enable this interrupt now that's it's cleared. */
		intr_enabled |= ibit;
		ifpga_set_intrmask();
	}

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}
