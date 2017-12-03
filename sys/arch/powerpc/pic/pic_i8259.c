/* $NetBSD: pic_i8259.c,v 1.6.6.1 2017/12/03 11:36:37 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_i8259.c,v 1.6.6.1 2017/12/03 11:36:37 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>

#include <powerpc/pic/picvar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

void i8259_initialize(void);

struct pic_ops *
setup_i8259(void)
{
	struct i8259_ops *i8259;
	struct pic_ops *pic;

	i8259 = kmem_alloc(sizeof(*i8259), KM_SLEEP);
	pic = &i8259->pic;

	pic->pic_numintrs = 16;
	pic->pic_cookie = (void *)NULL;
	pic->pic_enable_irq = i8259_enable_irq;
	pic->pic_reenable_irq = i8259_enable_irq;
	pic->pic_disable_irq = i8259_disable_irq;
	pic->pic_get_irq = i8259_get_irq;
	pic->pic_ack_irq = i8259_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	pic->pic_finish_setup = NULL;
	strcpy(pic->pic_name, "i8259");
	pic_add(pic);
	i8259->pending_events = 0;
	i8259->enable_mask = 0xffffffff;
	i8259->irqs = 0;

	i8259_initialize();

	return pic;
}
