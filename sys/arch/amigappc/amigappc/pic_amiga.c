/* $NetBSD: pic_amiga.c,v 1.2 2010/02/02 19:15:33 phx Exp $ */

/*-
 * Copyright (c) 2008,2009,2010 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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
__KERNEL_RCSID(0, "$NetBSD: pic_amiga.c,v 1.2 2010/02/02 19:15:33 phx Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <arch/powerpc/pic/picvar.h>
#include <amiga/amiga/custom.h>
#include <amigappc/amigappc/p5reg.h>

static void amiga_enable_irq(struct pic_ops *, int, int);
static void amiga_disable_irq(struct pic_ops *, int);
static int amiga_get_irq(struct pic_ops *, int);
static void amiga_ack_irq(struct pic_ops *, int);
struct pic_ops *setup_amiga_intr(void); 

/*
 * Number of amigappc hardware interrupts, based on 68000 IPL mask.
 * In reality 6, because level 0 means no interrupt and level 7 (NMI)
 * should not happen.
 */
#define MAXIPL 7

struct amiga_ops {
	struct pic_ops pic;
	int disablemask;
};

struct pic_ops *
setup_amiga_intr(void)
{
	struct amiga_ops *amipic;
	struct pic_ops *pic;

	amipic = malloc(sizeof(struct amiga_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(amipic != NULL);
	pic = &amipic->pic;

	pic->pic_numintrs = MAXIPL;
	pic->pic_cookie = (void *)NULL;
	pic->pic_enable_irq = amiga_enable_irq;
	pic->pic_reenable_irq = amiga_enable_irq;
	pic->pic_disable_irq = amiga_disable_irq;
	pic->pic_get_irq = amiga_get_irq;
	pic->pic_ack_irq = amiga_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	strcpy(pic->pic_name, "amiga");

	/* Set PPC IPL to 7, disabling all interrupts */
	amipic->disablemask = (1 << MAXIPL) - 1;
	P5write(P5_IPL_EMU, P5_DISABLE_INT | 7);

	pic_add(pic);

	return pic;
}

static void
amiga_enable_irq(struct pic_ops *pic, int ipl, int type)
{
	struct amiga_ops *amipic = (struct amiga_ops *)pic;
	int iplmask, dmask, newipl;

	iplmask = 1 << ipl;
	dmask = amipic->disablemask;

	if ((dmask & iplmask)) {

		dmask &= ~iplmask;
		amipic->disablemask = dmask;
		if (!(dmask & ~(iplmask - 1))) {

			/* Lower the emulated PPC IPL to the next highest */
			newipl = 31 - cntlzw(dmask);
			P5write(P5_IPL_EMU, P5_SET_CLEAR | P5_DISABLE_INT |
			    (newipl ^ P5_IPL_MASK));
			P5write(P5_IPL_EMU, P5_DISABLE_INT | newipl);
		}
	}
}

static void
amiga_disable_irq(struct pic_ops *pic, int ipl)
{
	struct amiga_ops *amipic = (struct amiga_ops *)pic;
	int iplmask, dmask;

	iplmask = 1 << ipl;
	dmask = amipic->disablemask;

	if (!(dmask & iplmask)) {

		if (!(dmask & ~(iplmask - 1))) {

			/* Raise the emulated PPC IPL to the new ipl */
			P5write(P5_IPL_EMU, P5_SET_CLEAR | P5_DISABLE_INT |
			    (ipl ^ P5_IPL_MASK));
			P5write(P5_IPL_EMU, P5_DISABLE_INT | ipl);
		}
		amipic->disablemask |= iplmask;
	}
}

static int
amiga_get_irq(struct pic_ops *pic, int mode)
{
	unsigned char ipl;

	if (mode == PIC_GET_RECHECK)
		return 255;

	/* Get the interrupt's 68k IPL - the bits are active low */
	P5read(P5_IPL_EMU, ipl);
	ipl = ~(ipl >> 3) & P5_IPL_MASK;

	return ipl == 0 ? 255 : ipl;
}

static void
amiga_ack_irq(struct pic_ops *pic, int ipl)
{
}
