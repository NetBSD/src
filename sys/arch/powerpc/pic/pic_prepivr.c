/* $NetBSD: pic_prepivr.c,v 1.8.2.1 2017/12/03 11:36:37 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_prepivr.c,v 1.8.2.1 2017/12/03 11:36:37 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>

#include <powerpc/pic/picvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

static int  prepivr_get_irq(struct pic_ops *, int);
static int  motivr_get_irq(struct pic_ops *, int);
static void prepivr_establish_irq(struct pic_ops *, int, int, int);

extern vaddr_t prep_intr_reg;	/* PReP interrupt vector register */
extern uint32_t prep_intr_reg_off; /* IVR offset within the mapped page */

#define IO_ELCR1	0x4d0
#define IO_ELCR2	0x4d1

/*
 * Prior to calling init_prepivr, the port is expected to have mapped the
 * page containing the IVR with mapiodev to prep_intr_reg and set up the
 * prep_intr_reg_off.
 */

struct pic_ops *
setup_prepivr(int ivrtype)
{
	struct i8259_ops *prepivr;
	struct pic_ops *pic;
	uint32_t pivr;

	prepivr = kmem_alloc(sizeof(*prepivr), KM_SLEEP);
	pic = &prepivr->pic;

	pivr = prep_intr_reg + prep_intr_reg_off;
	pic->pic_numintrs = 16;
	pic->pic_cookie = (void *)pivr;
	pic->pic_enable_irq = i8259_enable_irq;
	pic->pic_reenable_irq = i8259_enable_irq;
	pic->pic_disable_irq = i8259_disable_irq;
	if (ivrtype == PIC_IVR_MOT)
		pic->pic_get_irq = motivr_get_irq;
	else
		pic->pic_get_irq = prepivr_get_irq;
	pic->pic_ack_irq = i8259_ack_irq;
	pic->pic_establish_irq = prepivr_establish_irq;
	pic->pic_finish_setup = NULL;
	strcpy(pic->pic_name, "prepivr");
	pic_add(pic);
	prepivr->pending_events = 0;
	prepivr->enable_mask = 0xffffffff;
	prepivr->irqs = 0;

	/* initialize the ELCR */
	isa_outb(IO_ELCR1, (0 >> 0) & 0xff);
	isa_outb(IO_ELCR2, (0 >> 8) & 0xff);
	i8259_initialize();

	return pic;
}

static void
prepivr_establish_irq(struct pic_ops *pic, int irq, int type, int maxlevel)
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

static int
motivr_get_irq(struct pic_ops *pic, int mode)
{
	int irq;

	irq = i8259_get_irq(pic, mode);
	/* read the IVR to ack it */
	(void)inb(pic->pic_cookie);

	/* i8259_get_irq will return 255 by itself, so just pass it on */
	return irq;
}

static int
prepivr_get_irq(struct pic_ops *pic, int mode)
{
	int irq;

	irq = inb(pic->pic_cookie);

	if (irq == 0 && mode == PIC_GET_IRQ)
		return 255;
	if (irq == 7 && mode == PIC_GET_RECHECK)
		return 255;

	return irq;
}
