/* $NetBSD: i8259_common.c,v 1.6.2.1 2012/04/17 00:06:48 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: i8259_common.c,v 1.6.2.1 2012/04/17 00:06:48 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>

#include <powerpc/pic/picvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

void
i8259_initialize(void)
{
	isa_outb(IO_ICU1, 0x11);		/* program device, four bytes */
	isa_outb(IO_ICU1+1, 0);			/* starting at this vector */
	isa_outb(IO_ICU1+1, 1 << IRQ_SLAVE);	/* slave on line 2 */
	isa_outb(IO_ICU1+1, 1);			/* 8086 mode */
	isa_outb(IO_ICU1+1, 0xff);		/* leave interrupts masked */

	isa_outb(IO_ICU2, 0x11);		/* program device, four bytes */
	isa_outb(IO_ICU2+1, 8);			/* starting at this vector */
	isa_outb(IO_ICU2+1, IRQ_SLAVE);
	isa_outb(IO_ICU2+1, 1);			/* 8086 mode */
	isa_outb(IO_ICU2+1, 0xff);		/* leave interrupts masked */
}

void
i8259_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct i8259_ops *i8259 = (struct i8259_ops *)pic;

	i8259->irqs |= 1 << irq;
	if (i8259->irqs >= 0x100) /* IRQS >= 8 in use? */
		i8259->irqs |= 1 << IRQ_SLAVE;

	i8259->enable_mask = ~i8259->irqs;
	isa_outb(IO_ICU1+1, i8259->enable_mask);
	isa_outb(IO_ICU2+1, i8259->enable_mask >> 8);
}

void
i8259_disable_irq(struct pic_ops *pic, int irq)
{
	struct i8259_ops *i8259 = (struct i8259_ops *)pic;
	uint32_t mask = 1 << irq;

	i8259->enable_mask |= mask;
	isa_outb(IO_ICU1+1, i8259->enable_mask);
	isa_outb(IO_ICU2+1, i8259->enable_mask >> 8);
}

void
i8259_ack_irq(struct pic_ops *pic, int irq)
{
	if (irq < 8) {
		isa_outb(IO_ICU1, 0xe0 | irq);
	} else {
		isa_outb(IO_ICU2, 0xe0 | (irq & 7));
		isa_outb(IO_ICU1, 0xe0 | IRQ_SLAVE);
	}
}

int
i8259_get_irq(struct pic_ops *pic, int mode)
{
	int irq;

	isa_outb(IO_ICU1, 0x0c);
	irq = isa_inb(IO_ICU1) & 0x07;
	if (irq == IRQ_SLAVE) {
		isa_outb(IO_ICU2, 0x0c);
		irq = (isa_inb(IO_ICU2) & 0x07) + 8;
	}

	if (irq == 0 && mode == PIC_GET_IRQ)
		return 255;
	if (irq == 7 && mode == PIC_GET_RECHECK)
		return 255;

	return irq;
}
