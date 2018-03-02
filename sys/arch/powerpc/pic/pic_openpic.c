/*	$NetBSD: pic_openpic.c,v 1.12 2018/03/02 19:36:19 macallan Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_openpic.c,v 1.12 2018/03/02 19:36:19 macallan Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>

#include <powerpc/pic/picvar.h>

#include "opt_interrupt.h"

static void opic_enable_irq(struct pic_ops *, int, int);
static void opic_disable_irq(struct pic_ops *, int);
static void opic_establish_irq(struct pic_ops*, int, int, int);

struct pic_ops *
setup_openpic(void *addr, int passthrough)
{
	struct openpic_ops *opicops;
	struct pic_ops *pic;
	int irq;
	u_int x;

	openpic_base = mapiodev((paddr_t)addr, 0x40000, false);
	opicops = kmem_alloc(sizeof(*opicops), KM_SLEEP);
	pic = &opicops->pic;

	x = openpic_read(OPENPIC_FEATURE);
	if (((x & 0x07ff0000) >> 16) == 0)
		panic("setup_openpic() called on distributed openpic");
	
	aprint_normal("OpenPIC Version 1.%d: "
	    "Supports %d CPUs and %d interrupt sources.\n",
	    x & 0xff, ((x & 0x1f00) >> 8) + 1, ((x & 0x07ff0000) >> 16) + 1);

	pic->pic_numintrs = ((x & 0x07ff0000) >> 16) + 2; /* one more slot for IPI */
	pic->pic_cookie = addr;
	pic->pic_enable_irq = opic_enable_irq;
	pic->pic_reenable_irq = opic_enable_irq;
	pic->pic_disable_irq = opic_disable_irq;
	pic->pic_get_irq = opic_get_irq;
	pic->pic_ack_irq = opic_ack_irq;
	pic->pic_establish_irq = opic_establish_irq;
	pic->pic_finish_setup = opic_finish_setup;
	opicops->isu = NULL;
	opicops->nrofisus = 0; /* internal only */
	opicops->flags = 0; /* no flags (yet) */
	opicops->irq_per = NULL; /* internal ISU only */
	strcpy(pic->pic_name, "openpic");
	pic_add(pic);

	/*
	 * the following sequence should make the same effects as openpic
	 * controller reset by writing a one at the self-clearing
	 * OPENPIC_CONFIG_RESET bit.  Please check the document of your
	 * OpenPIC compliant interrupt controller and see whether #else
	 * portion can work as described.
	 */
#if 1
	openpic_set_priority(0, 15);

	for (irq = 0; irq < pic->pic_numintrs; irq++) {
		/* make sure to keep disabled */
		openpic_write(OPENPIC_SRC_VECTOR(irq), OPENPIC_IMASK);
		/* send all interrupts to CPU 0 */
		openpic_write(OPENPIC_IDEST(irq), 1 << 0);
	}

	x = openpic_read(OPENPIC_CONFIG);
	if (passthrough)
		x &= ~OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	else 
		x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(OPENPIC_CONFIG, x);

	openpic_write(OPENPIC_SPURIOUS_VECTOR, 0xff);

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < pic->pic_numintrs; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}
#else
	irq = 0;
	openpic_write(OPENPIC_CONFIG, OPENPIC_CONFIG_RESET);
	do {
		x = openpic_read(OPENPIC_CONFIG);
	} while (x & OPENPIC_CONFIG_RESET); /* S1C bit */
	if (passthrough)
		x &= ~OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	else 
		x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(OPENPIC_CONFIG, x);
	openpic_set_priority(0, 0);
#endif

#if 0
	printf("timebase freq=%d\n", openpic_read(0x10f0));
#endif
	return pic;
}

static void
opic_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	int realpri = max(1, min(15, pri));
	uint32_t x;

	x = irq;
	x |= OPENPIC_IMASK;

	if (type == IST_EDGE_RISING || type == IST_LEVEL_HIGH)
		x |= OPENPIC_POLARITY_POSITIVE;
	else
		x |= OPENPIC_POLARITY_NEGATIVE;

	if (type == IST_EDGE_FALLING || type == IST_EDGE_RISING)
		x |= OPENPIC_SENSE_EDGE;
	else
		x |= OPENPIC_SENSE_LEVEL;

	x |= realpri << OPENPIC_PRIORITY_SHIFT;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);

	aprint_debug("%s: setting IRQ %d to priority %d\n", __func__, irq,
	    realpri);
}

static void
opic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	u_int x;

	x = openpic_read(OPENPIC_SRC_VECTOR(irq));
	x &= ~OPENPIC_IMASK;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);
}

static void
opic_disable_irq(struct pic_ops *pic, int irq)
{
	u_int x;

	x = openpic_read(OPENPIC_SRC_VECTOR(irq));
	x |= OPENPIC_IMASK;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);
}
