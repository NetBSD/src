/* $NetBSD: pic_cpc700.c,v 1.1.2.1 2007/05/08 17:10:48 garbled Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_cpc700.c,v 1.1.2.1 2007/05/08 17:10:48 garbled Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <arch/powerpc/pic/picvar.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700uic.h>

static void cpc700_pic_enable_irq(struct pic_ops *, int, int);
static void cpc700_pic_disable_irq(struct pic_ops *, int);
static int  cpc700_get_irq(struct pic_ops *);
static void cpc700_ack_irq(struct pic_ops *, int);

struct cpc700_ops {
	struct pic_ops pic;
};

struct pic_ops *
setup_cpc700(void)
{
	struct cpc700_ops *cpc700;
	struct pic_ops *pic;

	cpc700 = malloc(sizeof(struct cpc700_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(cpc700 != NULL);
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
cpc700_get_irq(struct pic_ops *pic)
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
