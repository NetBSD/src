/* $NetBSD: ep93xx_intr.c,v 1.17.2.2 2014/08/20 00:02:45 tls Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ep93xx_intr.c,v 1.17.2.2 2014/08/20 00:02:45 tls Exp $");

/*
 * Interrupt support for the Cirrus Logic EP93XX
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/termios.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <arm/locore.h>

#include <arm/ep93xx/ep93xxreg.h> 
#include <arm/ep93xx/ep93xxvar.h> 

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static uint32_t vic1_imask[NIPL];
static uint32_t vic2_imask[NIPL];

/* Current interrupt priority level. */
volatile int hardware_spl_level;

/* Software copy of the IRQs we have enabled. */
volatile uint32_t vic1_intr_enabled;
volatile uint32_t vic2_intr_enabled;

void	ep93xx_intr_dispatch(struct trapframe *);

#define VIC1REG(reg)	*((volatile uint32_t*) (EP93XX_AHB_VBASE + \
	EP93XX_AHB_VIC1 + (reg)))
#define VIC2REG(reg)	*((volatile uint32_t*) (EP93XX_AHB_VBASE + \
	EP93XX_AHB_VIC2 + (reg)))

static void
ep93xx_set_intrmask(uint32_t vic1_irqs, uint32_t vic2_irqs)
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
	KASSERT(vic1_imask[IPL_SOFTCLOCK] == 0);
	KASSERT(vic2_imask[IPL_SOFTCLOCK] == 0);
	KASSERT(vic1_imask[IPL_SOFTBIO] == 0);
	KASSERT(vic2_imask[IPL_SOFTBIO] == 0);
	KASSERT(vic1_imask[IPL_SOFTNET] == 0);
	KASSERT(vic2_imask[IPL_SOFTNET] == 0);
	KASSERT(vic1_imask[IPL_SOFTSERIAL] == 0);
	KASSERT(vic2_imask[IPL_SOFTSERIAL] == 0);

	/*
	 * splsched() must block anything that uses the scheduler.
	 */
	vic1_imask[IPL_SCHED] |= vic1_imask[IPL_VM];
	vic2_imask[IPL_SCHED] |= vic2_imask[IPL_VM];

	/*
	 * splhigh() must block "everything".
	 */
	vic1_imask[IPL_HIGH] |= vic1_imask[IPL_SCHED];
	vic2_imask[IPL_HIGH] |= vic2_imask[IPL_SCHED];

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

inline void
splx(int new)
{
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	set_curcpl(new);
	if (new != hardware_spl_level) {
		hardware_spl_level = new;
		ep93xx_set_intrmask(vic1_imask[new], vic2_imask[new]);
	}
	restore_interrupts(oldirqstate);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

int
_splraise(int ipl)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = curcpl();
	set_curcpl(ipl);
	restore_interrupts(oldirqstate);
	return (old);
}

int
_spllower(int ipl)
{
	int	old = curcpl();

	if (old <= ipl)
		return (old);
	splx(ipl);
	return (old);
}

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

		snprintf(iq->iq_name, sizeof(iq->iq_name), "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				     NULL, (i < VIC_NIRQ ? "vic1" : "vic2"),
		                     iq->iq_name);
	}
	curcpu()->ci_intr_depth = 0;
	set_curcpl(0);
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
ep93xx_intr_dispatch(struct trapframe *frame)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;
	int			pcpl;
	uint32_t		vic1_hwpend;
	uint32_t		vic2_hwpend;
	int			irq;

	pcpl = curcpl();

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
		curcpu()->ci_data.cpu_nintr++;
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			set_curcpl(ih->ih_ipl);
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
		}
	} else if (vic2_hwpend) {
		irq = ffs(vic2_hwpend) - 1;

		iq = &intrq[irq + VIC_NIRQ];
		iq->iq_ev.ev_count++;
		curcpu()->ci_data.cpu_nintr++;
		TAILQ_FOREACH(ih, &iq->iq_list, ih_list) {
			set_curcpl(ih->ih_ipl);
			oldirqstate = enable_interrupts(I32_bit);
			(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
			restore_interrupts(oldirqstate);
		}
	}

	set_curcpl(pcpl);
	hardware_spl_level = pcpl;
	ep93xx_set_intrmask(vic1_imask[pcpl], vic2_imask[pcpl]);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}
