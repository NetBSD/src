/*	$NetBSD: footbridge_irqhandler.c,v 1.23.18.1 2014/08/20 00:02:45 tls Exp $	*/

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

#ifndef ARM_SPL_NOINLINE
#define	ARM_SPL_NOINLINE
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0,"$NetBSD: footbridge_irqhandler.c,v 1.23.18.1 2014/08/20 00:02:45 tls Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/intr.h>
#include <machine/cpu.h>
#include <arm/footbridge/dc21285mem.h>
#include <arm/footbridge/dc21285reg.h>

#include <dev/pci/pcivar.h>

#include "isa.h"
#if NISA > 0
#include <dev/isa/isavar.h>
#endif

/* Interrupt handler queues. */
static struct intrq footbridge_intrq[NIRQ];

/* Interrupts to mask at each level. */
int footbridge_imask[NIPL];

/* Software copy of the IRQs we have enabled. */
volatile uint32_t intr_enabled;

/* Interrupts pending */
volatile int footbridge_ipending;

void footbridge_intr_dispatch(struct clockframe *frame);

const struct evcnt *footbridge_pci_intr_evcnt(void *, pci_intr_handle_t);

const struct evcnt *
footbridge_pci_intr_evcnt(void *pcv, pci_intr_handle_t ih)
{
	/* XXX check range is valid */
#if NISA > 0
	if (ih >= 0x80 && ih <= 0x8f) {
		return isa_intr_evcnt(NULL, (ih & 0x0f));
	}
#endif
	return &footbridge_intrq[ih].iq_ev;
}

static inline void
footbridge_enable_irq(int irq)
{
	intr_enabled |= (1U << irq);
	footbridge_set_intrmask();
}

static inline void
footbridge_disable_irq(int irq)
{
	intr_enabled &= ~(1U << irq);
	footbridge_set_intrmask();
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
footbridge_intr_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &footbridge_intrq[irq];
		footbridge_disable_irq(irq);
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			levels |= (1U << ih->ih_ipl);
		}
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int irqs = 0;
		for (irq = 0; irq < NIRQ; irq++) {
			if (footbridge_intrq[irq].iq_levels & (1U << ipl))
				irqs |= (1U << irq);
		}
		footbridge_imask[ipl] = irqs;
	}

	/* IPL_NONE must open up all interrupts */
	KASSERT(footbridge_imask[IPL_NONE] == 0);
	KASSERT(footbridge_imask[IPL_SOFTCLOCK] == 0);
	KASSERT(footbridge_imask[IPL_SOFTBIO] == 0);
	KASSERT(footbridge_imask[IPL_SOFTNET] == 0);
	KASSERT(footbridge_imask[IPL_SOFTSERIAL] == 0);

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	footbridge_imask[IPL_SCHED] |= footbridge_imask[IPL_VM];
	footbridge_imask[IPL_HIGH] |= footbridge_imask[IPL_SCHED];

	/*
	 * Calculate the ipl level to go to when handling this interrupt
	 */
	for (irq = 0, iq = footbridge_intrq; irq < NIRQ; irq++, iq++) {
		int irqs = (1U << irq);
		if (!TAILQ_EMPTY(&iq->iq_list)) {
			footbridge_enable_irq(irq);
			TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
				irqs |= footbridge_imask[ih->ih_ipl];
			}
		}
		iq->iq_mask = irqs;
	}
}

int
_splraise(int ipl)
{
    return (footbridge_splraise(ipl));
}

/* this will always take us to the ipl passed in */
void
splx(int new)
{
    footbridge_splx(new);
}

int
_spllower(int ipl)
{
    return (footbridge_spllower(ipl));
}

void
footbridge_intr_init(void)
{
	struct intrq *iq;
	int i;

	intr_enabled = 0;
	set_curcpl(0xffffffff);
	footbridge_ipending = 0;
	footbridge_set_intrmask();
	
	for (i = 0, iq = footbridge_intrq; i < NIRQ; i++, iq++) {
		TAILQ_INIT(&iq->iq_list);
	}
	
	footbridge_intr_calculate_masks();

	/* Enable IRQ's, we don't have any FIQ's*/
	enable_interrupts(I32_bit);
}

void
footbridge_intr_evcnt_attach(void)
{
	struct intrq *iq;
	int i;

	for (i = 0, iq = footbridge_intrq; i < NIRQ; i++, iq++) {

		snprintf(iq->iq_name, sizeof(iq->iq_name), "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "footbridge", iq->iq_name);
	}
}

void *
footbridge_intr_claim(int irq, int ipl, const char *name, int (*func)(void *), void *arg)
{
	struct intrq *iq;
	struct intrhand *ih;
	u_int oldirqstate;

	if (irq < 0 || irq > NIRQ)
		panic("footbridge_intr_establish: IRQ %d out of range", irq);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
	{
		printf("No memory");
		return (NULL);
	}
		
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	iq = &footbridge_intrq[irq];

	iq->iq_ist = IST_LEVEL;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);

	footbridge_intr_calculate_masks();

	/* detach the existing event counter and add the new name */
	evcnt_detach(&iq->iq_ev);
	evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
			NULL, "footbridge", name);
	
	restore_interrupts(oldirqstate);
	
	return(ih);
}

void
footbridge_intr_disestablish(void *cookie)
{
	struct intrhand *ih = cookie;
	struct intrq *iq = &footbridge_intrq[ih->ih_irq];
	int oldirqstate;

	/* XXX need to free ih ? */
	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);

	footbridge_intr_calculate_masks();

	restore_interrupts(oldirqstate);
}

static inline uint32_t footbridge_intstatus(void)
{
	return ((volatile uint32_t*)(DC21285_ARMCSR_VBASE))[IRQ_STATUS>>2];
}

/* called with external interrupts disabled */
void
footbridge_intr_dispatch(struct clockframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, irq, ibit, hwpend;
	struct cpu_info * const ci = curcpu();
	const int ppl = ci->ci_cpl;
	const int imask = footbridge_imask[ppl];

	hwpend = footbridge_intstatus();

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	intr_enabled &= ~hwpend;
	footbridge_set_intrmask();

	while (hwpend != 0) {
		int intr_rc = 0;
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (imask & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: the IRQ is already disabled.
			 */
			footbridge_ipending |= ibit;
			continue;
		}

		footbridge_ipending &= ~ibit;

		iq = &footbridge_intrq[irq];
		iq->iq_ev.ev_count++;
		ci->ci_data.cpu_nintr++;
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			ci->ci_cpl = ih->ih_ipl;
			oldirqstate = enable_interrupts(I32_bit);
			intr_rc = (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
			if (intr_rc != 1)
				break;
		}

		ci->ci_cpl = ppl;

		/* Re-enable this interrupt now that's it's cleared. */
		intr_enabled |= ibit;
		footbridge_set_intrmask();

		/* also check for any new interrupts that may have occurred,
		 * that we can handle at this spl level */
		hwpend |= (footbridge_ipending & ICU_INT_HWMASK) & ~imask;
	}

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif /* __HAVE_FAST_SOFTINTS */
}
