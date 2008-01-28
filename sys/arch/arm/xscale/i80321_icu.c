/*	$NetBSD: i80321_icu.c,v 1.14.30.3 2008/01/28 18:29:09 matt Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2006 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Steve C. Woodford for Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: i80321_icu.c,v 1.14.30.3 2008/01/28 18:29:09 matt Exp $");

#ifndef EVBARM_SPL_NOINLINE
#define	EVBARM_SPL_NOINLINE
#endif

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
int i80321_imask[NIPL];

/* Interrupts pending. */
volatile int i80321_ipending;

/* Software copy of the IRQs we have enabled. */
volatile uint32_t intr_enabled;

/* Mask if interrupts steered to FIQs. */
uint32_t intr_steer;

/*
 * Interrupt bit names.
 */
const char * const i80321_irqnames[] = {
	"DMA0 EOT",
	"DMA0 EOC",
	"DMA1 EOT",
	"DMA1 EOC",
	"irq 4",
	"irq 5",
	"AAU EOT",
	"AAU EOC",
	"core PMU",
	"TMR0 (hardclock)",
	"TMR1",
	"I2C0",
	"I2C1",
	"MU",
	"BIST",
	"periph PMU",
	"XScale PMU",
	"BIU error",
	"ATU error",
	"MCU error",
	"DMA0 error",
	"DMA1 error",
	"irq 22",
	"AAU error",
	"MU error",
	"SSP",
	"irq 26",
	"irq 27",
	"irq 28",
	"irq 29",
	"irq 30",
	"irq 31",
};

void	i80321_intr_dispatch(struct clockframe *frame);

static inline uint32_t
i80321_iintsrc_read(void)
{
	uint32_t iintsrc;

	__asm volatile("mrc p6, 0, %0, c8, c0, 0"
		: "=r" (iintsrc));

	/*
	 * The IINTSRC register shows bits that are active even
	 * if they are masked in INTCTL, so we have to mask them
	 * off with the interrupts we consider enabled.
	 */
	return (iintsrc & intr_enabled);
}

static inline void
i80321_set_intrsteer(void)
{

	__asm volatile("mcr p6, 0, %0, c4, c0, 0"
		:
		: "r" (intr_steer & ICU_INT_HWMASK));
}

static inline void
i80321_enable_irq(int irq)
{

	intr_enabled |= (1U << irq);
	i80321_set_intrmask();
}

static inline void
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
		i80321_imask[ipl] = irqs;
	}

	i80321_imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */

	KASSERT(i80321_imask[IPL_VM] != 0);
	i80321_imask[IPL_SCHED] |= i80321_imask[IPL_VM];
	i80321_imask[IPL_HIGH] |= i80321_imask[IPL_SCHED];

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
			irqs |= i80321_imask[ih->ih_ipl];
		iq->iq_mask = irqs;
	}
}

void
splx(int new)
{
	i80321_splx(new);
}

int
_spllower(int ipl)
{
	return (i80321_spllower(ipl));
}

int
_splraise(int ipl)
{
	return (i80321_splraise(ipl));
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

		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
		    NULL, "iop321", i80321_irqnames[i]);
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

/*
 * Hardware interrupt handler.
 *
 * If I80321_HPI_ENABLED is defined, this code attempts to deal with
 * HPI interrupts as best it can.
 *
 * The problem is that HPIs cannot be masked at the interrupt controller;
 * they can only be masked by disabling IRQs in the XScale core.
 *
 * So, if an HPI comes in and we determine that it should be masked at
 * the current IPL then we mark it pending in the usual way and set
 * I32_bit in the interrupt frame. This ensures that when we return from
 * i80321_intr_dispatch(), IRQs will be disabled in the XScale core. (To
 * ensure IRQs are enabled later, i80321_splx() has been modified to do
 * just that when a pending HPI interrupt is unmasked.) Additionally,
 * because HPIs are level-triggered, the registered handler for the HPI
 * interrupt will also be invoked with IRQs disabled. If a masked HPI
 * occurs at the same time as another unmasked higher priority interrupt,
 * the higher priority handler will also be invoked with IRQs disabled.
 * As a result, the system could end up executing a lot of code with IRQs
 * completely disabled if the HPI's IPL is relatively low.
 *
 * At the present time, the only known use of HPI is for the console UART
 * on a couple of boards. This is probably the least intrusive use of HPI
 * as IPL_SERIAL is the highest priority IPL in the system anyway. The
 * code has not been tested with HPI hooked up to a class of device which
 * interrupts below IPL_SERIAL. Indeed, such a configuration is likely to
 * perform very poorly if at all, even though the following code has been
 * designed (hopefully) to cope with it.
 */

void
i80321_intr_dispatch(struct clockframe *frame)
{
	struct intrq *iq;
	struct intrhand *ih;
	int oldirqstate, irq, ibit, hwpend;
#ifdef I80321_HPI_ENABLED
	int oldpending;
#endif
	struct cpu_info * const ci = curcpu();
	const int ppl = ci->ci_cpl;
	const uint32_t imask = i80321_imask[ppl];

	hwpend = i80321_iintsrc_read();

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	intr_enabled &= ~hwpend;
	i80321_set_intrmask();

#ifdef I80321_HPI_ENABLED
	oldirqstate = 0;	/* XXX: quell gcc warning */
#endif

	while (hwpend != 0) {
#ifdef I80321_HPI_ENABLED
		/* Deal with HPI interrupt first */
		if (__predict_false(hwpend & INT_HPIMASK))
			irq = ICU_INT_HPI;
		else
#endif
		irq = ffs(hwpend) - 1;
		ibit = (1U << irq);

		hwpend &= ~ibit;

		if (imask & ibit) {
			/*
			 * IRQ is masked; mark it as pending and check
			 * the next one.  Note: the IRQ is already disabled.
			 */
#ifdef I80321_HPI_ENABLED
			if (__predict_false(irq == ICU_INT_HPI)) {
				/*
				 * This is an HPI. We *must* disable
				 * IRQs in the interrupt frame until
				 * INT_HPIMASK is cleared by a later
				 * call to splx(). Otherwise the level-
				 * triggered interrupt will just keep
				 * coming back.
				 */
				frame->cf_if.if_spsr |= I32_bit;
			}
#endif
			i80321_ipending |= ibit;
			continue;
		}

#ifdef I80321_HPI_ENABLED
		oldpending = i80321_ipending | ibit;
#endif
		i80321_ipending &= ~ibit;

		iq = &intrq[irq];
		iq->iq_ev.ev_count++;
		uvmexp.intrs++;
#ifdef I80321_HPI_ENABLED
		/*
		 * Re-enable interrupts iff an HPI is not pending
		 */
		if (__predict_true((oldpending & INT_HPIMASK) == 0)) {
#endif
			TAILQ_FOREACH (ih, &iq->iq_list, ih_list) {
				ci->ci_cpl = ih->ih_ipl;
				oldirqstate = enable_interrupts(I32_bit);
				(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
				restore_interrupts(oldirqstate);
			}
#ifdef I80321_HPI_ENABLED
		} else if (irq == ICU_INT_HPI) {
			/*
			 * We've just handled the HPI. Make sure IRQs
			 * are enabled in the interrupt frame.
			 * Here's hoping the handler really did clear
			 * down the source...
			 */
			frame->cf_if.if_spsr &= ~I32_bit;
		}
#endif
		ci->ci_cpl = ppl;

		/* Re-enable this interrupt now that's it's cleared. */
		intr_enabled |= ibit;
		i80321_set_intrmask();

		/*
		 * Don't forget to include interrupts which may have
		 * arrived in the meantime.
		 */
		hwpend |= ((i80321_ipending & ICU_INT_HWMASK) & ~imask);
	}

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}
