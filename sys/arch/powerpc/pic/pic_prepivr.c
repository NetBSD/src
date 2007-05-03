/* $NetBSD: pic_prepivr.c,v 1.1.2.2 2007/05/03 16:00:15 nisimura Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: pic_prepivr.c,v 1.1.2.2 2007/05/03 16:00:15 nisimura Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <machine/intr.h>

#include <arch/powerpc/pic/picvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

static int  prepivr_irq_is_enabled(struct pic_ops *, int);
static void prepivr_reenable_irq(struct pic_ops *, int, int);
static void prepivr_enable_irq(struct pic_ops *, int, int);
static void prepivr_disable_irq(struct pic_ops *, int);
static void prepivr_clear_irq(struct pic_ops *, int);
static int  prepivr_get_irq(struct pic_ops *);
static void prepivr_ack_irq(struct pic_ops *, int);
static void prepivr_establish_irq(struct pic_ops *pic, int irq, int type);

struct prepivr_ops {
	struct pic_ops pic;
	uint32_t pending_events;
	uint32_t enable_mask;
	uint32_t irqs;
};

vaddr_t prep_intr_reg;		/* PReP interrupt vector register */
uint32_t prep_intr_reg_off;	/* IVR offset within the mapped page */

void init_prepivr(void);

#define IO_ELCR1	0x4d0
#define IO_ELCR2	0x4d1

/*
 * Prior to calling init_prepivr, the port is expected to have mapped the
 * page containing the IVR with mapiodev to prep_intr_reg and set up the
 * prep_intr_reg_off.
 */

void
init_prepivr(void)
{
	struct prepivr_ops *prepivr;
	struct pic_ops *pic;
	uint32_t pivr;

	prepivr = malloc(sizeof(struct prepivr_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(prepivr != NULL);
	pic = &prepivr->pic;

	pivr = prep_intr_reg + prep_intr_reg_off;
	pic->pic_numintrs = 16;
	pic->pic_cookie = (void *)pivr;
	pic->pic_irq_is_enabled = prepivr_irq_is_enabled;
	pic->pic_enable_irq = prepivr_enable_irq;
	pic->pic_reenable_irq = prepivr_reenable_irq;
	pic->pic_disable_irq = prepivr_disable_irq;
	pic->pic_clear_irq = prepivr_clear_irq;
	pic->pic_get_irq = prepivr_get_irq;
	pic->pic_ack_irq = prepivr_ack_irq;
	pic->pic_establish_irq = prepivr_establish_irq;
	strcpy(pic->pic_name, "prepivr");
	pic_add(pic);
	prepivr->pending_events = 0;
	prepivr->enable_mask = 0xffffffff;
	prepivr->irqs = 0;
}

static int
prepivr_irq_is_enabled(struct pic_ops *pic, int irq)
{
	return 1;
}

static void
prepivr_establish_irq(struct pic_ops *pic, int irq, int type)
{
	u_int8_t elcr[2];
	int icu, bit;

	icu = irq / 8;
	bit = irq % 8;
	elcr[0] = isa_inb(IO_ELCR1);
	elcr[1] = isa_inb(IO_ELCR2);

	if (type == IST_LEVEL)
		elcr[icu] |= 1 << bit;
	else
		elcr[icu] &= ~(1 << bit);

	isa_outb(IO_ELCR1, elcr[0]);
	isa_outb(IO_ELCR2, elcr[1]);
}

static void
prepivr_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct prepivr_ops *prepivr = (struct prepivr_ops *)pic;

	prepivr->irqs |= 1 << irq;
	if (prepivr->irqs >= 0x100) /* IRQS >= 8 in use? */
		prepivr->irqs |= 1 << IRQ_SLAVE;

	prepivr->enable_mask = ~prepivr->irqs;
	isa_outb(IO_ICU1+1, prepivr->enable_mask);
	isa_outb(IO_ICU2+1, prepivr->enable_mask >> 8);
}

static void
prepivr_reenable_irq(struct pic_ops *pic, int irq, int type)
{
	prepivr_enable_irq(pic, irq, type);
}

static void
prepivr_disable_irq(struct pic_ops *pic, int irq)
{
	struct prepivr_ops *prepivr = (struct prepivr_ops *)pic;
	uint32_t mask = 1 << irq;

	prepivr->enable_mask |= mask;
	isa_outb(IO_ICU1+1, prepivr->enable_mask);
	isa_outb(IO_ICU2+1, prepivr->enable_mask >> 8);
}

static void
prepivr_clear_irq(struct pic_ops *pic, int irq)
{
	/* do nothing */
}

static int
prepivr_get_irq(struct pic_ops *pic)
{
	static int lirq;
	int irq;

	irq = inb(pic->pic_cookie);
	if (lirq == 7 && irq == lirq)
		return 255;

	lirq = irq;
	if (irq == 0)
		return 255;

	return irq;
}

static void
prepivr_ack_irq(struct pic_ops *pic, int irq)
{
	if (irq < 8) {
		isa_outb(IO_ICU1, 0xe0 | irq);
	} else {
		isa_outb(IO_ICU2, 0xe0 | (irq & 7));
		isa_outb(IO_ICU1, 0xe0 | IRQ_SLAVE);
	}
}
