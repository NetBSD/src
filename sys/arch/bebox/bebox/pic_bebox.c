/* $NetBSD: pic_bebox.c,v 1.5.2.1 2008/05/18 12:31:43 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_bebox.c,v 1.5.2.1 2008/05/18 12:31:43 yamt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/atomic.h>
#include <machine/pio.h>

#include <arch/powerpc/pic/picvar.h>

extern paddr_t bebox_mb_reg;

#define BEBOX_INTR_MASK		0x0ffffffc
#define BEBOX_SET_MASK		0x80000000
#define BEBOX_INTR(x)		(0x80000000 >> x)
#define CPU0_INT_MASK		0x0f0
#define CPU1_INT_MASK		0x1f0
#define INT_STATE_REG		0x2f0

static void bebox_enable_irq(struct pic_ops *, int, int);
static void bebox_disable_irq(struct pic_ops *, int);
static int bebox_get_irq(struct pic_ops *, int);
static void bebox_ack_irq(struct pic_ops *, int);
struct pic_ops * setup_bebox_intr(void); 

struct pic_ops *
setup_bebox_intr(void)
{
	struct pic_ops *pic;

	pic = malloc(sizeof(struct pic_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(pic != NULL);

	pic->pic_numintrs = 32;
	pic->pic_cookie = (void *)bebox_mb_reg;
	pic->pic_enable_irq = bebox_enable_irq;
	pic->pic_reenable_irq = bebox_enable_irq;
	pic->pic_disable_irq = bebox_disable_irq;
	pic->pic_get_irq = bebox_get_irq;
	pic->pic_ack_irq = bebox_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	strcpy(pic->pic_name, "bebox");
	pic_add(pic);
	return(pic);
}

static void
bebox_enable_irq(struct pic_ops *pic, int irq, int type)
{

	*(volatile unsigned int *)(bebox_mb_reg + CPU0_INT_MASK) =
	    BEBOX_SET_MASK | (1 << (31 - irq));
}

static void
bebox_disable_irq(struct pic_ops *pic, int irq)
{

	*(volatile unsigned int *)(bebox_mb_reg + CPU0_INT_MASK) =
	    (1 << (31 - irq));
}

static int
bebox_get_irq(struct pic_ops *pic, int mode)
{
	unsigned int state;

	state = *(volatile unsigned int *)(bebox_mb_reg + INT_STATE_REG);
	state &= BEBOX_INTR_MASK;
	state &= *(volatile unsigned int *)(bebox_mb_reg + CPU0_INT_MASK);
	if (state == 0)
		return 255;
	return __builtin_clz(state);
}

static void
bebox_ack_irq(struct pic_ops *pic, int irq)
{
	/* do nothing */
}
