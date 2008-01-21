/*	$NetBSD: pic_iocc.c,v 1.1.8.2 2008/01/21 09:38:59 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pic_iocc.c,v 1.1.8.2 2008/01/21 09:38:59 yamt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <machine/intr.h>
#include <machine/iocc.h>
#include <machine/bus.h>

#include <arch/powerpc/pic/picvar.h>

struct pic_ops *setup_iocc(void);

static void iocc_set_priority(int, int, int);
static int iocc_get_irq(struct pic_ops *, int);
static void iocc_enable_irq(struct pic_ops *, int, int);
static void iocc_ack_irq(struct pic_ops *, int);
static void iocc_disable_irq(struct pic_ops *, int);

struct pic_ops *
setup_iocc(void)
{
	struct pic_ops *pic;
	int i;

	pic = malloc(sizeof(struct pic_ops), M_DEVBUF, M_NOWAIT);
        KASSERT(pic != NULL);

	pic->pic_numintrs = 16;
	pic->pic_cookie = (void *)NULL;
	pic->pic_enable_irq = iocc_enable_irq;
	pic->pic_reenable_irq = iocc_enable_irq;
	pic->pic_disable_irq = iocc_disable_irq;
	pic->pic_get_irq = iocc_get_irq;
	pic->pic_ack_irq = iocc_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	pic->pic_finish_setup = NULL;
	strcpy(pic->pic_name, "iocc");
	pic_add(pic);

	/* set all priorities as high */
	for (i=0; i < pic->pic_numintrs; i++)
		iocc_set_priority(0, 0, i);
	return(pic);
}

/*
 * the IOCC IER is a bitmask of pending IRQs, where only 0-15 are valid
 */
static int
iocc_get_irq(struct pic_ops *pic, int mode)
{
        int irq;
        uint32_t rv = 0;

        rv = in32rb(RS6000_BUS_SPACE_IO + IOCC_IRR);
        if (rv == 0)
                return 255;

        irq = 31 - cntlzw(rv);
        if (irq >= 0 && irq < 16)
                return irq;
        return 255;
}

/* enable an IRQ on the IOCC */
static void
iocc_enable_irq(struct pic_ops *pic, int irq, int type)
{
        uint32_t mask;

        mask = in32rb(RS6000_BUS_SPACE_IO + IOCC_IEE);
        mask |= 1 << irq;
        out32rb(RS6000_BUS_SPACE_IO + IOCC_IEE, mask);
}

/* disable an IRQ on the IOCC */
static void
iocc_disable_irq(struct pic_ops *pic, int irq)
{
        uint32_t mask;

        mask = in32rb(RS6000_BUS_SPACE_IO + IOCC_IEE);
        mask &= ~(1 << irq);
        out32rb(RS6000_BUS_SPACE_IO + IOCC_IEE, mask);
}

/* Issue an EOI to the IOCC */
static void
iocc_ack_irq(struct pic_ops *pic, int irq)
{
        out32rb(RS6000_BUS_SPACE_IO + IOCC_EOI(irq), 0x1); /* val is ignored */
}

/* set interrupt priority and destination */
static void
iocc_set_priority(int cpu, int pri, int irq)
{
        uint32_t x;

        x = pri;
        x |= cpu<<8;
        out32rb(RS6000_BUS_SPACE_IO + IOCC_XIVR(irq), x);
}
