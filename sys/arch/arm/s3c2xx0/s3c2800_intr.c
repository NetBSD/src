/* $NetBSD: s3c2800_intr.c,v 1.5 2003/07/15 00:24:48 lukem Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * IRQ handler for Samsung S3C2800 processor.
 * It has integrated interrupt controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2800_intr.c,v 1.5 2003/07/15 00:24:48 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/s3c2xx0/s3c2800reg.h>
#include <arm/s3c2xx0/s3c2800var.h>

/*
 * interrupt dispatch table.
 */

struct s3c2xx0_intr_dispatch handler[ICU_LEN];

__volatile int softint_pending;

__volatile int current_spl_level;
__volatile int intr_mask;

/* interrupt masks for each level */
int s3c2xx0_imask[NIPL];
int s3c2xx0_ilevel[ICU_LEN];

vaddr_t intctl_base;		/* interrupt controller registers */
#define icreg(offset) \
	(*(volatile uint32_t *)(intctl_base+(offset)))

/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[SI_NQUEUES] = {
	IPL_SOFT,		/* SI_SOFT */
	IPL_SOFTCLOCK,		/* SI_SOFTCLOCK */
	IPL_SOFTNET,		/* SI_SOFTNET */
	IPL_SOFTSERIAL,		/* SI_SOFTSERIAL */
};

/*
 *   Clearing interrupt pending bits affects some built-in
 * peripherals.  For example, IIC starts transmitting next data when
 * its interrupt pending bit is cleared.
 *   We need to leave those bits to peripheral handlers.
 */
#define PENDING_CLEAR_MASK	(~((1<<S3C2800_INT_IIC0)|(1<<S3C2800_INT_IIC1)))

/*
 * called from irq_entry.
 */
void s3c2800_irq_handler(struct clockframe *);
void
s3c2800_irq_handler(struct clockframe *frame)
{
	uint32_t irqbits;
	int irqno;
	int saved_spl_level;

	saved_spl_level = current_spl_level;

	/* get pending IRQs */
	irqbits = icreg(INTCTL_IRQPND) & ICU_INT_HWMASK;

	for (irqno = 0; irqbits; ++irqno) {
		if ((irqbits & (1 << irqno)) == 0)
			continue;
		/* raise spl to stop interrupts of lower priorities */
		if (saved_spl_level < handler[irqno].level)
			s3c2xx0_setipl(handler[irqno].level);

		/* clear pending bit */
		icreg(INTCTL_SRCPND) = PENDING_CLEAR_MASK & (1 << irqno);
#ifdef notyet
		/* Enable interrupt */
#endif
		(*handler[irqno].func) (
		    handler[irqno].cookie == 0
		    ? frame : handler[irqno].cookie);
#ifdef notyet
		/* Disable interrupt */
#endif

		irqbits &= ~(1 << irqno);
	}

	/* restore spl to that was when this interrupt happen */
	s3c2xx0_setipl(saved_spl_level);

	if (softint_pending & intr_mask)
		s3c2xx0_do_pending();
}

static const u_char s3c2800_ist[] = {
	EXTINTR_LOW,		/* NONE */
	EXTINTR_FALLING,	/* PULSE */
	EXTINTR_FALLING,	/* EDGE */
	EXTINTR_LOW,		/* LEVEL */
	EXTINTR_HIGH,
	EXTINTR_RISING,
	EXTINTR_BOTH,
};

void *
s3c2800_intr_establish(int irqno, int level, int type,
    int (* func) (void *), void *cookie)
{
	int save;

	if (irqno < 0 || irqno >= ICU_LEN ||
	    type < IST_NONE || IST_EDGE_BOTH < type)
		panic("intr_establish: bogus irq or type");

	save = disable_interrupts(I32_bit);

	handler[irqno].cookie = cookie;
	handler[irqno].func = func;
	handler[irqno].level = level;

	s3c2xx0_update_intr_masks(irqno, level);

	if (irqno <= S3C2800_INT_EXT(7)) {
		/*
		 * Update external interrupt control
		 */
		uint32_t reg;
		u_int 	trig;

		trig = s3c2800_ist[type];

		reg = bus_space_read_4(s3c2xx0_softc->sc_iot,
				       s3c2xx0_softc->sc_gpio_ioh,
				       GPIO_EXTINTR);

		reg = reg & ~(0x0f << (4*irqno));
		reg |= trig << (4*irqno);

		bus_space_write_4(s3c2xx0_softc->sc_iot, s3c2xx0_softc->sc_gpio_ioh,
				  GPIO_EXTINTR, reg);
	}

	intr_mask = s3c2xx0_imask[current_spl_level];
	*s3c2xx0_intr_mask_reg = intr_mask;

	restore_interrupts(save);

	return (&handler[irqno]);
}


void
s3c2800_intr_init(struct s3c2800_softc *sc)
{
	intctl_base = (vaddr_t) bus_space_vaddr(sc->sc_sx.sc_iot,
	    sc->sc_sx.sc_intctl_ioh);

	s3c2xx0_intr_mask_reg = (uint32_t *)(intctl_base + INTCTL_INTMSK);

	/* clear all pending interrupt */
	icreg(INTCTL_SRCPND) = 0xffffffff;

	s3c2xx0_intr_init(handler, ICU_LEN);

}
