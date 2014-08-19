/*	$NetBSD: mvsoc_intr.c,v 1.5.2.2 2014/08/20 00:02:47 tls Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mvsoc_intr.c,v 1.5.2.2 2014/08/20 00:02:47 tls Exp $");

#include "opt_mvsoc.h"

#define _INTR_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>

#include <machine/intr.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/pic/picvar.h>
#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>


int (*find_pending_irqs)(void);

static void mvsoc_bridge_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void mvsoc_bridge_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static int mvsoc_bridge_pic_find_pending_irqs(struct pic_softc *);
static void mvsoc_bridge_pic_establish_irq(struct pic_softc *,
					   struct intrsource *);
static void mvsoc_bridge_pic_source_name(struct pic_softc *, int, char *,
					 size_t);

static const char * const sources[] = {
    "CPUSelfInt",      "CPUTimer0IntReq", "CPUTimer1IntReq", "CPUWDTimerIntReq",
    "AccessErr",       "Bit64Err",
};

static struct pic_ops mvsoc_bridge_picops = {
	.pic_unblock_irqs = mvsoc_bridge_pic_unblock_irqs,
	.pic_block_irqs = mvsoc_bridge_pic_block_irqs,
	.pic_find_pending_irqs = mvsoc_bridge_pic_find_pending_irqs,
	.pic_establish_irq = mvsoc_bridge_pic_establish_irq,
	.pic_source_name = mvsoc_bridge_pic_source_name,
};

struct pic_softc mvsoc_bridge_pic = {
	.pic_ops = &mvsoc_bridge_picops,
	.pic_maxsources = MVSOC_MLMB_MLMBI_NIRQ,
	.pic_name = "mvsoc_bridge",
};


void
mvsoc_irq_handler(void *frame)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	const uint32_t oldipl_mask = __BIT(oldipl);
	int ipl_mask = 0;

	ci->ci_data.cpu_nintr++;

	ipl_mask = find_pending_irqs();

	/*
	 * Record the pending_ipls and deliver them if we can.
	 */
	if ((ipl_mask & ~oldipl_mask) > oldipl_mask)
		pic_do_pending_ints(I32_bit, oldipl, frame);
}

/*
 * Mbus-L to Mbus bridge
 */

void *
mvsoc_bridge_intr_establish(int ih, int ipl, int (*ih_func)(void *), void *arg)
{

	return intr_establish(mvsoc_bridge_pic.pic_irqbase + ih, ipl,
	    IST_LEVEL_HIGH, ih_func, arg);
}

/* ARGSUSED */
static void
mvsoc_bridge_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
			      uint32_t irq_mask)
{

	write_mlmbreg(MVSOC_MLMB_MLMBICR,
	    read_mlmbreg(MVSOC_MLMB_MLMBICR) & ~irq_mask);
	write_mlmbreg(MVSOC_MLMB_MLMBIMR,
	    read_mlmbreg(MVSOC_MLMB_MLMBIMR) | irq_mask);
}

/* ARGSUSED */
static void
mvsoc_bridge_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
			    uint32_t irq_mask)
{

	write_mlmbreg(MVSOC_MLMB_MLMBIMR,
	    read_mlmbreg(MVSOC_MLMB_MLMBIMR) & ~irq_mask);
}

static int
mvsoc_bridge_pic_find_pending_irqs(struct pic_softc *pic)
{
	uint32_t pending;

	pending =
	    read_mlmbreg(MVSOC_MLMB_MLMBICR) & read_mlmbreg(MVSOC_MLMB_MLMBIMR);

	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(pic, 0, pending);
}

/* ARGSUSED */
static void
mvsoc_bridge_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing */
}

static void
mvsoc_bridge_pic_source_name(struct pic_softc *pic, int irq, char *buf,
			     size_t len)
{

	strlcpy(buf, sources[irq], len);
}
