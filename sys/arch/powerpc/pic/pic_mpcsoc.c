/*	$NetBSD: pic_mpcsoc.c,v 1.1.2.2 2009/08/19 18:46:41 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_mpcsoc.c,v 1.1.2.2 2009/08/19 18:46:41 yamt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>

#include <arch/powerpc/pic/picvar.h>

#include "opt_interrupt.h"

static void mpcpic_enable_irq(struct pic_ops *, int, int);
static void mpcpic_disable_irq(struct pic_ops *, int);
static void mpcpic_establish_irq(struct pic_ops *, int, int, int);
static void mpcpic_finish_setup(struct pic_ops *);

static u_int steer8245[] = {
	0x10200,	/* external irq 0 direct/serial */
	0x10220,	/* external irq 1 direct/serial */
	0x10240,	/* external irq 2 direct/serial */
	0x10260,	/* external irq 3 direct/serial */
	0x10280,	/* external irq 4 direct/serial */
	0x102a0,	/* external irq 5 serial mode */
	0x102c0,	/* external irq 6 serial mode */
	0x102e0,	/* external irq 7 serial mode */
	0x10300,	/* external irq 8 serial mode */
	0x10320,	/* external irq 9 serial mode */
	0x10340,	/* external irq 10 serial mode */
	0x10360,	/* external irq 11 serial mode */
	0x10380,	/* external irq 12 serial mode */
	0x103a0,	/* external irq 13 serial mode */
	0x103c0,	/* external irq 14 serial mode */
	0x103e0,	/* external irq 15 serial mode */
	0x11020,	/* I2C */
	0x11040,	/* DMA 0 */
	0x11060,	/* DMA 1 */
	0x110c0,	/* MU/I2O */
	0x01120,	/* Timer 0 */
	0x01160,	/* Timer 1 */
	0x011a0,	/* Timer 2 */
	0x011e0,	/* Timer 3 */
	0x11120,	/* DUART 0, MPC8245 */
	0x11140,	/* DUART 1, MPC8245 */
};
#define MPCPIC_IVEC(n)	(steer8245[(n)])
#define MPCPIC_IDST(n)	(steer8245[(n)] + 0x10)

static int i8259iswired = 0;

struct pic_ops *
setup_mpcpic(void *addr)
{
	struct openpic_ops *ops;
	struct pic_ops *self;
	int irq;
	u_int x;

	openpic_base = addr;
	ops = malloc(sizeof(struct openpic_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(ops != NULL);
	self = &ops->pic;

	x = openpic_read(OPENPIC_FEATURE);
	if (((x & 0x07ff0000) >> 16) == 0)
		panic("setup_mpcpic() called on distributed openpic");
	
	aprint_normal("OpenPIC Version 1.%d: "
	    "Supports %d CPUs and %d interrupt sources.\n",
	    x & 0xff, ((x & 0x1f00) >> 8) + 1, ((x & 0x07ff0000) >> 16) + 1);

#ifdef PIC_I8259
	i8259iswired = 1;
#endif
	self->pic_numintrs = ((x & 0x07ff0000) >> 16) + 1;
	self->pic_cookie = addr;
	self->pic_enable_irq = mpcpic_enable_irq;
	self->pic_reenable_irq = mpcpic_enable_irq;
	self->pic_disable_irq = mpcpic_disable_irq;
	self->pic_get_irq = opic_get_irq;
	self->pic_ack_irq = opic_ack_irq;
	self->pic_establish_irq = mpcpic_establish_irq;
	self->pic_finish_setup = mpcpic_finish_setup;
	ops->isu = NULL;
	ops->nrofisus = 0; /* internal only */
	ops->flags = 0; /* no flags (yet) */
	ops->irq_per = NULL; /* internal ISU only */
	strcpy(self->pic_name, "mpcpic");
	pic_add(self);

	openpic_set_priority(0, 15);
	for (irq = 0; irq < self->pic_numintrs; irq++) {
		/* make sure to keep disabled */
		openpic_write(MPCPIC_IVEC(irq), OPENPIC_IMASK);
		/* send all interrupts to CPU 0 */
		openpic_write(MPCPIC_IDST(irq), 1 << 0);
	}
	openpic_write(OPENPIC_SPURIOUS_VECTOR, 0xff);
	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < self->pic_numintrs; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

#if 0
	printf("timebase freq=%d\n", openpic_read(0x10f0));
#endif
	return self;
}

void
mpcpic_reserv16()
{
	extern int max_base; /* intr.c */

	/*
	 * reserve 16 irq slot for the case when no i8259 exists to use.
	 */
	max_base += 16;
}

static void
mpcpic_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	int realpri = max(1, min(15, pri));
	u_int x;

	x = irq;
	x |= OPENPIC_IMASK;
	x |= (i8259iswired && irq == 0) ?
	    OPENPIC_POLARITY_POSITIVE :	OPENPIC_POLARITY_NEGATIVE;
	x |= (type == IST_EDGE) ? OPENPIC_SENSE_EDGE : OPENPIC_SENSE_LEVEL;
	x |= realpri << OPENPIC_PRIORITY_SHIFT;
	openpic_write(MPCPIC_IVEC(irq), x);

	aprint_debug("%s: setting IRQ %d to priority %d\n", __func__, irq,
	    realpri);
}

static void
mpcpic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	u_int x;

	x = openpic_read(MPCPIC_IVEC(irq));
	x &= ~OPENPIC_IMASK;
	openpic_write(MPCPIC_IVEC(irq), x);
}

static void
mpcpic_disable_irq(struct pic_ops *pic, int irq)
{
	u_int x;

	x = openpic_read(MPCPIC_IVEC(irq));
	x |= OPENPIC_IMASK;
	openpic_write(MPCPIC_IVEC(irq), x);
}

static void
mpcpic_finish_setup(struct pic_ops *pic)
{
	uint32_t cpumask = 1;
	int i;

	for (i = 0; i < pic->pic_numintrs; i++) {
		/* send all interrupts to all active CPUs */
		openpic_write(MPCPIC_IDST(i), cpumask);
	}
}
