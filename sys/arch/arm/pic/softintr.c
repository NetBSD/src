/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.1.2.1 2007/08/29 05:17:40 matt Exp $");

#define _INTR_PRIVATE
 
#include <sys/param.h>
#include <sys/evcnt.h>
 
#include <uvm/uvm_extern.h>
  
#include <machine/intr.h>

static void softintr_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void softintr_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void softintr_pic_establish_irq(struct pic_softc *, int, int, int);
 
const struct pic_ops softintr_pic_ops = {
	.pic_block_irqs = softintr_pic_block_irqs,
	.pic_unblock_irqs = softintr_pic_unblock_irqs,
	.pic_establish_irq = softintr_pic_establish_irq,
};

struct pic_softc pic_softgeneric = {
	.pic_ops = &softintr_pic_ops,
	.pic_maxsources = 32,
	.pic_name = "softgeneric",
};

struct pic_softc pic_softclock = {
	.pic_ops = &softintr_pic_ops,
	.pic_maxsources = 32,
	.pic_name = "softclock",
};

struct pic_softc pic_softnet = {
	.pic_ops = &softintr_pic_ops,
	.pic_maxsources = 32,
	.pic_name = "softnet",
};

struct pic_softc pic_softserial = {
	.pic_ops = &softintr_pic_ops,
	.pic_maxsources = 32,
	.pic_name = "softserial",
};

struct pic_softc * const soft_pics[] = {
	[IPL_SOFT] = &pic_softgeneric,
	[IPL_SOFTCLOCK] = &pic_softclock,
	[IPL_SOFTNET] = &pic_softnet,
	[IPL_SOFTSERIAL] = &pic_softserial,
};

void
softintr_pic_block_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
}

void
softintr_pic_unblock_irqs(struct pic_softc *pic, size_t irq_base, uint32_t irq_mask)
{
}

void
softintr_pic_establish_irq(struct pic_softc *pic, int irq, int ipl, int type)
{
	KASSERT(type == IST_SOFT);
	KASSERT(pic == soft_pics[ipl]);
}

void
softintr_schedule(void *arg)
{
	struct intrsource * const is = arg;
	pic_mark_pending(is->is_pic, is->is_irq);
}

void
softintr_disestablish(void *arg)
{
	struct intrsource * const is = arg;
	pic_disestablish_source(is);
}

void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct pic_softc * const pic = soft_pics[ipl];
	int irq;

	KASSERT(ipl >= 0);
	KASSERT(ipl < __arraycount(soft_pics));
	KASSERT(pic != NULL);

	irq = pic_alloc_irq(pic);
	if (irq >= 0)
		return pic_establish_intr(pic, irq, ipl, IST_SOFT,
		    (int (*)(void *))func, arg);

	return NULL;
}

#include <net/netisr.h>

void
softintr_init(void)
{
	pic_add(&pic_softserial, -1);
	pic_add(&pic_softnet, -1);
	pic_add(&pic_softclock, -1);
	pic_add(&pic_softgeneric, -1);


#define DONETISR(bit, fn) \
	pic_establish_intr(&pic_softnet, bit, IPL_SOFTNET, IST_SOFT, \
	    (int (*)(void *))(fn), NULL)

#include <net/netisr_dispatch.h>
}
