/* $NetBSD: pic_pi.c,v 1.1.2.2 2024/02/03 11:47:07 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Processor interface interrupt controller. Top level controller for all
 * EXT interrupts.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pic_pi.c,v 1.1.2.2 2024/02/03 11:47:07 martin Exp $");

#include <sys/param.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/bitops.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <arch/powerpc/pic/picvar.h>
#include <machine/wii.h>

static uint32_t pic_irqmask;
static uint32_t pic_actmask;

void pi_init_intr(void);

#define WR4(reg, val)	out32(reg, val)
#define RD4(reg)	in32(reg)

static void
pi_enable_irq(struct pic_ops *pic, int irq, int type)
{
	pic_irqmask |= __BIT(irq);
	WR4(PI_INTMR, pic_irqmask & ~pic_actmask);
}

static void
pi_disable_irq(struct pic_ops *pic, int irq)
{
	pic_irqmask &= ~__BIT(irq);
	WR4(PI_INTMR, pic_irqmask & ~pic_actmask);
}

static int
pi_get_irq(struct pic_ops *pic, int mode)
{
	uint32_t raw, pend;
	int irq;

	raw = RD4(PI_INTSR);
	pend = raw & pic_irqmask;
	if (pend == 0) {
		return 255;
	}
	irq = ffs32(pend) - 1;

	pic_actmask |= __BIT(irq);
	WR4(PI_INTMR, pic_irqmask & ~pic_actmask);

	return irq;
}

static void
pi_ack_irq(struct pic_ops *pic, int irq)
{
	pic_actmask &= ~__BIT(irq);
	WR4(PI_INTMR, pic_irqmask & ~pic_actmask);
	WR4(PI_INTSR, __BIT(irq));
}

static struct pic_ops pic = {
	.pic_name = "pi",
	.pic_numintrs = 32,
	.pic_cookie = NULL,
	.pic_enable_irq = pi_enable_irq,
	.pic_reenable_irq = pi_enable_irq,
	.pic_disable_irq = pi_disable_irq,
	.pic_get_irq = pi_get_irq,
	.pic_ack_irq = pi_ack_irq,
	.pic_establish_irq = dummy_pic_establish_intr,
};

void
pi_init_intr(void)
{
	pic_irqmask = 0;
	pic_actmask = 0;

	/* Mask and clear all interrupts. */
	WR4(PI_INTMR, 0);
	WR4(PI_INTSR, ~0U);

	pic_add(&pic);
}
