/*	$NetBSD: iq80310_intr.c,v 1.4.4.2 2002/01/08 00:24:27 nathanw Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
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

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/cpu.h>
#include <arm/cpufunc.h>

#include <arm/xscale/i80200reg.h>

#include <evbarm/iq80310/iq80310reg.h>
#include <evbarm/iq80310/iq80310var.h>
#include <evbarm/iq80310/obiovar.h>

irqhandler_t *irqhandlers[NIRQS];

int current_intr_depth;		/* Depth of interrupt nesting */
u_int intr_claimed_mask;	/* Interrupts that are claimed */
u_int intr_disabled_mask;	/* Interrupts that are temporarily disabled */
u_int intr_current_mask;	/* Interrupts currently allowable */
u_int spl_mask;
u_int irqmasks[IPL_LEVELS];
u_int irqblock[NIRQS];

u_int iq80310_intrmask;		/* actual interrupts currently enabled */

extern u_int soft_interrupts;   /* Only so we can initialise it */

extern char *_intrnames;
extern void set_spl_masks(void);

/* Called only from assembler code. */
uint32_t iq80310_intstat_read(void);
void	 stray_irqhandler(int);

/*
 * We have 8 interrupt source bits -- 5 in the XINT3 register, and 3
 * in the XINT0 register (the upper 3).
 */
#define	IRQ_BITS	0xff

void
irq_init(void)
{
	int loop;

	/* Clear all the IRQ handlers and the IRQ block masks. */
	for (loop = 0; loop < NIRQS; ++loop) {
		irqhandlers[loop] = NULL;
		irqblock[loop] = 0;
	}

	/*
	 * Set up the irqmasks for the different interrupt priority
	 * levels.  We will start with no bits set and these will be
	 * updated as handlers are installed at different IPLs.
	 */
	for (loop = 0; loop < IPL_LEVELS; ++loop)
		irqmasks[loop] = 0;

	current_intr_depth = 0;
	intr_claimed_mask = 0x00000000;
	intr_disabled_mask = 0x00000000;
	intr_current_mask = 0x00000000;
	spl_mask = 0x00000000;  
	soft_interrupts = 0x00000000;

	set_spl_masks();
	irq_setmasks();

	/* Steer PMU and BCU interrupts to IRQ. */
	__asm __volatile("mcr p13, 0, %0, c2, c0, 0"
		:
		: "r" (0));

	/*
	 * Enable external IRQs, disable external FIQs and
	 * the PMU and BCU interrupts.
	 */
	__asm __volatile("mcr p13, 0, %0, c0, c0, 0"
		:
		: "r" (INTCTL_IM));

	/* Enable IRQs. */
	enable_interrupts(I32_bit);
}

uint32_t
iq80310_intstat_read(void)
{
	uint32_t intstat;

	intstat = CPLD_READ(IQ80310_XINT3_STATUS) & 0x1f;
	if (1/*rev F or later board*/)
		intstat |= (CPLD_READ(IQ80310_XINT0_STATUS) & 0x7) << 5;

	/*
	 * Yuck.  Even if the interrupt is disabled, the bit will
	 * still light up in the interrupt status register (it
	 * just won't assert IRQ#).
	 */
	return (intstat & iq80310_intrmask);
}

__inline void
irq_setmasks_nointr(void)
{
	u_int disabled;

	/* The actual mask of IRQs actually right *right now*. */
	iq80310_intrmask = (intr_current_mask & spl_mask) & IRQ_BITS;

	/*
	 * The XINT_MASK register sets a bit to *disable*.
	 */
	disabled = ~iq80310_intrmask;

	/*
	 * The PCI interrupts are all masked by a single
	 * bit in XINT3.
	 */
	if (disabled >> 5)
		disabled |= XINT3_SINTD;

	CPLD_WRITE(IQ80310_XINT_MASK, disabled & 0x1f);
}

void
irq_setmasks(void)
{
	u_int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	irq_setmasks_nointr();
	restore_interrupts(oldirqstate);
}

void
enable_irq(int irq)
{

	intr_claimed_mask |= (1U << irq);
	intr_current_mask = intr_claimed_mask & ~intr_disabled_mask;
	irq_setmasks_nointr();
}

void
disable_irq(int irq)
{

	intr_claimed_mask &= ~(1U << irq);
	intr_current_mask = intr_claimed_mask & ~intr_disabled_mask;
	irq_setmasks_nointr();
}

void
stray_irqhandler(int irq)
{

	panic("no handlers for IRQ %d (xint_mask = 0x%02x)\n", irq,
	    CPLD_READ(IQ80310_XINT_MASK));
}

void *
iq80310_intr_establish(int irq, int ipl, int (*func)(void *), void *arg)
{
	irqhandler_t *ih, *ptr;
	u_int oldirqstate;
	int loop;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_level = ipl;
	ih->ih_name = NULL;
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_flags = 0;
	ih->ih_num = irq;

	oldirqstate = disable_interrupts(I32_bit);

	/* Attach handler at top of chain */
	ih->ih_next = irqhandlers[irq];
	irqhandlers[irq] = ih;

	/* Update the IRQ masks. */
	ptr = irqhandlers[irq];
	if (ptr) {
		ipl = ptr->ih_level - 1;
		while (ptr) {
			if (ptr->ih_level - 1 < ipl)
				ipl = ptr->ih_level - 1;
			ptr = ptr->ih_next;
		}
		for (loop = 0; loop < IPL_LEVELS; ++loop) {
			if (ipl >= loop)
				irqmasks[loop] |= (1U << irq);
			else
				irqmasks[loop] &= ~(1U << irq);
		}
	}

	/* splimp > spltty */
	irqmasks[IPL_NET] &= irqmasks[IPL_TTY];

	/*
	 * We now need to update the irqblock array.  This array indicates
	 * what other interrupts should be blocked when a given interrupt
	 * is asserted.  This basically emulates hardware interrupt
	 * priorities e.g. by blocking all other IPL_BIO interrupts when
	 * an IPL_BIO interrupt is asserted.  For each interrupt, we find
	 * the highest IPL and set the block mask to the interrupt mask
	 * for that level.
	 */
	for (loop = 0; loop < NIRQS; ++loop) {
		ptr = irqhandlers[loop];
		if (ptr) {
			/* There is at least 1 handler so scan the chain */
			ipl = ptr->ih_level;
			while (ptr) {
				if (ptr->ih_level > ipl)
					ipl = ptr->ih_level;
				ptr = ptr->ih_next;
			}
			irqblock[loop] = ~irqmasks[ipl];
		} else {
			/* No handlers, so nothing else needs to be blocked. */
			irqblock[loop] = 0;
		}
	}

	enable_irq(irq);
	set_spl_masks();

	restore_interrupts(oldirqstate);

	return (ih);
}

void
iq80310_intr_disestablish(void *cookie)
{

	panic("iq80310_intr_disestablish");
}
