/*	$NetBSD: pic_soft.c,v 1.1.2.2 2007/05/22 16:03:50 matt Exp $ */

/*-
 * Copyright (c) 2007 Matt Thomas
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
__KERNEL_RCSID(0, "$NetBSD: pic_soft.c,v 1.1.2.2 2007/05/22 16:03:50 matt Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <arch/powerpc/pic/picvar.h>


#define SOFTMASK ((1 << IPL_SOFTCLOCK) |\
		  (1 << IPL_SOFTNET) |\
		  (1 << IPL_SOFTI2C) |\
		  (1 << IPL_SOFTSERIAL))

static void softpic_enable_irq(struct pic_ops *, int, int);
static void softpic_disable_irq(struct pic_ops *, int);
static int  softpic_get_irq(struct pic_ops *);
static void softpic_source_name(struct pic_ops*, int, char *, size_t);

static struct softpic_ops {
	struct pic_ops pic;
	uint32_t enable_mask;
	uint32_t pending;
} softpic = {
	.pic = {
		.pic_numintrs = NIPL,
		.pic_enable_irq = softpic_enable_irq,
		.pic_reenable_irq = softpic_enable_irq,
		.pic_disable_irq = softpic_disable_irq,
		.pic_get_irq = softpic_get_irq,
		.pic_source_name = softpic_source_name,
		.pic_name = "soft",
	},
};

void
setup_softpic(void)
{
	struct pic_ops * const pic = &softpic.pic;

	pic_add(pic);

	intr_establish(pic->pic_intrbase + IPL_SOFTCLOCK, IST_SOFT,
	    IPL_SOFTCLOCK, (int (*)(void *))softintr__run,
	    (void *)IPL_SOFTCLOCK);
	intr_establish(pic->pic_intrbase + IPL_SOFTNET, IST_SOFT,
	    IPL_SOFTNET, (int (*)(void *))softintr__run,
	    (void *)IPL_SOFTNET);
	intr_establish(pic->pic_intrbase + IPL_SOFTI2C, IST_SOFT,
	    IPL_SOFTI2C, (int (*)(void *))softintr__run,
	    (void *)IPL_SOFTI2C);
	intr_establish(pic->pic_intrbase + IPL_SOFTSERIAL, IST_SOFT,
	    IPL_SOFTSERIAL, (int (*)(void *))softintr__run,
	    (void *)IPL_SOFTSERIAL);
}

static void
softpic_source_name(struct pic_ops *pic, int irq, char *name, size_t len)
{
	switch (irq) {
	case IPL_SOFTSERIAL:	strlcpy(name, "serial", len); break;
	case IPL_SOFTNET:	strlcpy(name, "net", len); break;
	case IPL_SOFTI2C:	strlcpy(name, "i2c", len); break;
	case IPL_SOFTCLOCK:	strlcpy(name, "clock", len); break;
	};
}

static void
softpic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct softpic_ops * const soft = (void *) pic;
	soft->enable_mask |= 1 << irq;
	if (soft->pending & soft->enable_mask)
		pic_mark_pending(pic->pic_intrbase + irq);
}

static void
softpic_disable_irq(struct pic_ops *pic, int irq)
{
	struct softpic_ops * const soft = (void *) pic;
	soft->enable_mask &= ~(1 << irq);
}

static int
softpic_get_irq(struct pic_ops *pic)
{
	struct softpic_ops * const soft = (void *) pic;
	uint32_t old, new;
	int ipl;

	while (((old = soft->pending) & soft->enable_mask) != 0) {
		/* use cntlzw instead of ffs since we want higher IPLs first */
		ipl = 32 - __builtin_clz(old & soft->enable_mask);
		if (ipl == 0)
			return NO_IRQ;
		new = old & ~(1 << ipl);
		if (cas32(&soft->pending, old, new))
			return ipl;
	}

	return NO_IRQ;
}

void
softintr(int ipl)
{
	KASSERT((1 << ipl) & SOFTMASK);

	for (;;) {
		uint32_t old = softpic.pending;
		uint32_t new = old | (1 << ipl);
		if (cas32(&softpic.pending, old, new)) {
			pic_handle_intr(&softpic);
			return;
		}
	}
}
