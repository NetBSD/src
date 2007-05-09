/* $NetBSD: pic_i8259.c,v 1.1.2.3 2007/05/09 20:22:38 garbled Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_i8259.c,v 1.1.2.3 2007/05/09 20:22:38 garbled Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <machine/intr.h>

#include <arch/powerpc/pic/picvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

void i8259_initialize(void);
static int  i8259_get_irq(struct pic_ops *);

struct pic_ops *
setup_i8259(void)
{
	struct i8259_ops *i8259;
	struct pic_ops *pic;

	i8259 = malloc(sizeof(struct i8259_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(i8259 != NULL);
	pic = &i8259->pic;

	pic->pic_numintrs = 16;
	pic->pic_cookie = (void *)NULL;
	pic->pic_enable_irq = i8259_enable_irq;
	pic->pic_reenable_irq = i8259_enable_irq;
	pic->pic_disable_irq = i8259_disable_irq;
	pic->pic_get_irq = i8259_get_irq;
	pic->pic_ack_irq = i8259_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	strcpy(pic->pic_name, "i8259");
	pic_add(pic);
	i8259->pending_events = 0;
	i8259->enable_mask = 0xffffffff;
	i8259->irqs = 0;

	i8259_initialize();

	return pic;
}

static int
i8259_get_irq(struct pic_ops *pic)
{
	int irq;

	isa_outb(IO_ICU1, 0x0c);
	irq = isa_inb(IO_ICU1) & 0x07;
	if (irq == IRQ_SLAVE) {
		isa_outb(IO_ICU2, 0x0c);
		irq = (isa_inb(IO_ICU2) & 0x07) + 8;
	}

	if (irq == 0)
		return 255;

	return irq;
}
