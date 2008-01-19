/*	$NetBSD: pic_distopenpic.c,v 1.1.2.2 2008/01/19 12:14:43 bouyer Exp $ */

/*-
 * Copyright (c) 2008 Tim Rightnour
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
__KERNEL_RCSID(0, "$NetBSD: pic_distopenpic.c,v 1.1.2.2 2008/01/19 12:14:43 bouyer Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>

#include <arch/powerpc/pic/picvar.h>

#include "opt_interrupt.h"

/* distributed stuff */
static int opic_isu_from_irq(struct openpic_ops *, int, int *);
static u_int distopic_read(struct openpic_ops *, int, int);
static void distopic_write(struct openpic_ops *, int, int, u_int);
static void distopic_establish_irq(struct pic_ops *, int, int, int);
static void distopic_enable_irq(struct pic_ops *, int, int);
static void distopic_disable_irq(struct pic_ops *, int);

struct pic_ops *
setup_distributed_openpic(void *addr, int nrofisus, void **isu, int *maps)
{
	struct openpic_ops *opicops;
	struct pic_ops *pic;
	int irq, i;
	u_int x;

	openpic_base = (void *)addr;
	opicops = malloc(sizeof(struct openpic_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(opicops != NULL);
	pic = &opicops->pic;

	x = openpic_read(OPENPIC_FEATURE);
	if (((x & 0x07ff0000) >> 16) != 0)
		panic("Can't handle a distributed openpic with internal ISU");
	
	opicops->nrofisus = nrofisus;
	opicops->isu = malloc(sizeof(volatile unsigned char *) * nrofisus,
	    M_DEVBUF, M_NOWAIT);
	KASSERT(opicops->isu != NULL);
	opicops->irq_per = malloc(sizeof(uint8_t) * nrofisus,
	    M_DEVBUF, M_NOWAIT);
	KASSERT(opicops->irq_per != NULL);
	
	for (irq=0, i=0; i < nrofisus ; i++) {
		opicops->isu[i] = (void *)isu[i];
		opicops->irq_per[i] = maps[i]/0x20;
		irq += maps[i]/0x20;
		aprint_debug("%d: irqtotal=%d, added %d\n", i, irq,
		    maps[i]/0x20);
	}
	aprint_normal("OpenPIC Version 1.%d: "
	    "Supports %d CPUs and %d interrupt sources.\n",
	    x & 0xff, ((x & 0x1f00) >> 8) + 1, irq);
	pic->pic_numintrs = irq;
	pic->pic_cookie = addr;
	pic->pic_enable_irq = distopic_enable_irq;
	pic->pic_reenable_irq = distopic_enable_irq;
	pic->pic_disable_irq = distopic_disable_irq;
	pic->pic_get_irq = opic_get_irq;
	pic->pic_ack_irq = opic_ack_irq;
	pic->pic_establish_irq = distopic_establish_irq;
	pic->pic_finish_setup = opic_finish_setup;
	opicops->flags = OPENPIC_FLAG_DIST;
	strcpy(pic->pic_name, "openpic");
	pic_add(pic);

	openpic_set_priority(0, 15);

	for (i=0; i < nrofisus; i++) {
		for (irq = 0; irq < opicops->irq_per[i]; irq++) {
			/* make sure to keep disabled */
			distopic_write(opicops, i,
			    OPENPIC_DSRC_VECTOR_OFFSET(irq), OPENPIC_IMASK);
			/* send all interrupts to CPU 0 */
			distopic_write(opicops, i,
			    OPENPIC_DSRC_IDEST_OFFSET(irq), 1 << 0);
		}
	}

	x = openpic_read(OPENPIC_CONFIG);
	x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(OPENPIC_CONFIG, x);

	openpic_write(OPENPIC_SPURIOUS_VECTOR, 0xff);

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < pic->pic_numintrs; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

#if 0
	printf("timebase freq=%d\n", openpic_read(0x10f0));
#endif
	return pic;
	
}

/* Begin distributed openpic code */

static int
opic_isu_from_irq(struct openpic_ops *opic, int irq, int *realirq)
{
	int i;

	for (i=0; i < opic->nrofisus; i++) {
		if (irq < opic->irq_per[i]) {
			*realirq = irq;
			return i;
		} else
			irq -= opic->irq_per[i];
	}
	return -1;
}

static u_int
distopic_read(struct openpic_ops *opic, int isu, int offset)
{
	volatile unsigned char *addr = opic->isu[isu] + offset;
	
	return in32rb(addr);
}

static void
distopic_write(struct openpic_ops *opic, int isu, int offset, u_int val)
{
	volatile unsigned char *addr = opic->isu[isu] + offset;

	out32rb(addr, val);
}
	
static void
distopic_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	struct openpic_ops *opic = (struct openpic_ops *)pic;
	int isu, realirq, realpri = max(1, min(15, pri));
	uint32_t x;

	isu = opic_isu_from_irq(opic, irq, &realirq);
	KASSERT(isu != -1);

	x = irq;
	x |= OPENPIC_IMASK;
	x |= (realirq == 0 && isu == 0) ?
	    OPENPIC_POLARITY_POSITIVE :	OPENPIC_POLARITY_NEGATIVE;
	x |= (type == IST_EDGE) ? OPENPIC_SENSE_EDGE : OPENPIC_SENSE_LEVEL;
	x |= realpri << OPENPIC_PRIORITY_SHIFT;
	distopic_write(opic, isu, OPENPIC_DSRC_VECTOR_OFFSET(realirq), x);

	aprint_debug("%s: setting IRQ %d to priority %d 0x%x\n", __func__,
	    irq, realpri, x);
}

static void
distopic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct openpic_ops *opic = (struct openpic_ops *)pic;
	int isu, realirq;
	u_int x;

	isu = opic_isu_from_irq(opic, irq, &realirq);
	KASSERT(isu != -1);
	x = distopic_read(opic, isu, OPENPIC_DSRC_VECTOR_OFFSET(realirq));
	x &= ~OPENPIC_IMASK;
	distopic_write(opic, isu, OPENPIC_DSRC_VECTOR_OFFSET(realirq), x);
}

static void
distopic_disable_irq(struct pic_ops *pic, int irq)
{
	struct openpic_ops *opic = (struct openpic_ops *)pic;
	int isu, realirq;
	u_int x;

	isu = opic_isu_from_irq(opic, irq, &realirq);
	KASSERT(isu != -1);
	x = distopic_read(opic, isu, OPENPIC_DSRC_VECTOR_OFFSET(realirq));
	x |= OPENPIC_IMASK;
	distopic_write(opic, isu, OPENPIC_DSRC_VECTOR_OFFSET(realirq), x);
}
