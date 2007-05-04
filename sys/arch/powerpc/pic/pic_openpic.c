/*	$NetBSD: pic_openpic.c,v 1.1.2.7 2007/05/04 02:54:33 macallan Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_openpic.c,v 1.1.2.7 2007/05/04 02:54:33 macallan Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <powerpc/openpic.h>

#include <arch/powerpc/pic/picvar.h>

static void openpic_set_priority(int cpu, int pri);
static void opic_enable_irq(struct pic_ops *, int, int);
static void opic_disable_irq(struct pic_ops *, int);
static int  opic_get_irq(struct pic_ops *);
static void opic_ack_irq(struct pic_ops *, int);
static void opic_establish_irq(struct pic_ops*, int, int, int);

struct openpic_ops {
	struct pic_ops pic;
	uint32_t enable_mask;
};

volatile unsigned char *openpic_base;

struct pic_ops *
setup_openpic(void *addr, int passthrough)
{
	struct openpic_ops *openpic;
	struct pic_ops *pic;
	int irq;
	u_int x;

	openpic_base = (void *)addr;
	openpic = malloc(sizeof(struct openpic_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(openpic != NULL);
	pic = &openpic->pic;

	x = openpic_read(OPENPIC_FEATURE);
	aprint_normal("OpenPIC Version 1.%d: "
	    "Supports %d CPUs and %d interrupt sources.\n",
	    x & 0xff, ((x & 0x1f00) >> 8) + 1, ((x & 0x07ff0000) >> 16) + 1);

	pic->pic_numintrs = ((x & 0x07ff0000) >> 16) + 1;
	pic->pic_cookie = addr;
	pic->pic_enable_irq = opic_enable_irq;
	pic->pic_reenable_irq = opic_enable_irq;
	pic->pic_disable_irq = opic_disable_irq;
	pic->pic_get_irq = opic_get_irq;
	pic->pic_ack_irq = opic_ack_irq;
	pic->pic_establish_irq = opic_establish_irq;
	strcpy(pic->pic_name, "openpic");
	pic_add(pic);

	/* disable all interrupts */
	for (irq = 0; irq < pic->pic_numintrs; irq++)
		openpic_write(OPENPIC_SRC_VECTOR(irq), OPENPIC_IMASK);

	openpic_set_priority(0, 15);

	x = openpic_read(OPENPIC_CONFIG);
	if (passthrough) {
		x &= ~OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	} else {
		x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	}
	openpic_write(OPENPIC_CONFIG, x);

	for (irq = 0; irq < pic->pic_numintrs; irq++) {
		x = irq;
		x |= OPENPIC_IMASK;
		x |= (irq == 0) ?
		    OPENPIC_POLARITY_POSITIVE :	OPENPIC_POLARITY_NEGATIVE;
		x |= OPENPIC_SENSE_LEVEL;
		x |= 8 << OPENPIC_PRIORITY_SHIFT;
		openpic_write(OPENPIC_SRC_VECTOR(irq), x);
		/* send all interrupts to CPU 0 */
		openpic_write(OPENPIC_IDEST(irq), 1 << 0);
	}

	openpic_write(OPENPIC_SPURIOUS_VECTOR, 0xff);

	openpic_set_priority(0, 0);

	/* clear all pending interrunts */
	for (irq = 0; irq < pic->pic_numintrs; irq++) {
		openpic_read_irq(0);
		openpic_eoi(0);
	}

	for (irq = 0; irq < pic->pic_numintrs; irq++)
		opic_disable_irq(pic, irq);

#ifdef MULTIPROCESSOR
	x = openpic_read(OPENPIC_IPI_VECTOR(1));
	x &= ~(OPENPIC_IMASK | OPENPIC_PRIORITY_MASK | OPENPIC_VECTOR_MASK);
	x |= (15 << OPENPIC_PRIORITY_SHIFT) | IPI_VECTOR;
	openpic_write(OPENPIC_IPI_VECTOR(1), x);
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
	x |= (irq == 0) ?
	    OPENPIC_POLARITY_POSITIVE :	OPENPIC_POLARITY_NEGATIVE;
	x |= (type == IST_EDGE) ? OPENPIC_SENSE_EDGE : OPENPIC_SENSE_LEVEL;
	x |= realpri << OPENPIC_PRIORITY_SHIFT;
	openpic_write(OPENPIC_SRC_VECTOR(irq), x);
	/* send all interrupts to CPU 0 */
	openpic_write(OPENPIC_IDEST(irq), 1 << 0);
	aprint_debug("%s: setting IRQ %d to priority %d\n", __func__, irq, realpri);
}

static void
openpic_set_priority(int cpu, int pri)
{
	u_int x;

	x = openpic_read(OPENPIC_CPU_PRIORITY(cpu));
	x &= ~OPENPIC_CPU_PRIORITY_MASK;
	x |= pri;
	openpic_write(OPENPIC_CPU_PRIORITY(cpu), x);
}

static void
opic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	u_int x;

	x = openpic_read(OPENPIC_SRC_VECTOR(irq));
	x &= ~(OPENPIC_IMASK | OPENPIC_SENSE_LEVEL | OPENPIC_SENSE_EDGE);
	if (type == IST_LEVEL)
		x |= OPENPIC_SENSE_LEVEL;
	else
		x |= OPENPIC_SENSE_EDGE;
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

static int
opic_get_irq(struct pic_ops *pic)
{

	return openpic_read_irq(cpu_number());
}

static void
opic_ack_irq(struct pic_ops *pic, int irq)
{

	openpic_eoi(cpu_number());
}
