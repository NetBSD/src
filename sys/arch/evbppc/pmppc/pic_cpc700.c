/* $NetBSD: pic_cpc700.c,v 1.7.8.1 2020/12/14 14:37:54 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_cpc700.c,v 1.7.8.1 2020/12/14 14:37:54 thorpej Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/kernel.h>

#include <machine/pio.h>
#include <machine/intr.h>
#include <sys/bus.h>

#include <arch/powerpc/pic/picvar.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700uic.h>

static void cpc700_pic_enable_irq(struct pic_ops *, int, int);
static void cpc700_pic_disable_irq(struct pic_ops *, int);
static int  cpc700_get_irq(struct pic_ops *, int);
static void cpc700_ack_irq(struct pic_ops *, int);

struct cpc700_ops {
	struct pic_ops pic;
};

struct pic_ops *
setup_cpc700(void)
{
	struct cpc700_ops *cpc700;
	struct pic_ops *pic;

	cpc700 = kmem_alloc(sizeof(struct cpc700_ops), KM_SLEEP);
	pic = &cpc700->pic;

	pic->pic_numintrs = 32;
	pic->pic_cookie = (void *)NULL;
	pic->pic_enable_irq = cpc700_pic_enable_irq;
	pic->pic_reenable_irq = cpc700_pic_enable_irq;
	pic->pic_disable_irq = cpc700_pic_disable_irq;
	pic->pic_get_irq = cpc700_get_irq;
	pic->pic_ack_irq = cpc700_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	strcpy(pic->pic_name, "cpc700");
	pic_add(pic);

	return pic;
}

static void
cpc700_pic_enable_irq(struct pic_ops *pic, int irq, int type)
{
	cpc700_enable_irq(irq);
}

static void
cpc700_pic_disable_irq(struct pic_ops *pic, int irq)
{
	cpc700_disable_irq(irq);
}

static int
cpc700_get_irq(struct pic_ops *pic, int dummy)
{
	int irq;

	irq = cpc700_read_irq();
	if (irq < 0)
		return 255;
	return irq;
}

static void
cpc700_ack_irq(struct pic_ops *pic, int irq)
{
	cpc700_eoi(irq);
}
