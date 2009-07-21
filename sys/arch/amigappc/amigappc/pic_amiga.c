/* $NetBSD: pic_amiga.c,v 1.1 2009/07/21 09:49:15 phx Exp $ */

/*-
 * Copyright (c) 2008,2009 Frank Wille.
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
__KERNEL_RCSID(0, "$NetBSD: pic_amiga.c,v 1.1 2009/07/21 09:49:15 phx Exp $");

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

#define NIRQ 15

struct amiga_ops {
	struct pic_ops pic;
	int prev_level[NIRQ];
};

struct pic_ops *
setup_amiga_intr(void)
{
	struct amiga_ops *amipic;
	struct pic_ops *pic;

	amipic = malloc(sizeof(struct amiga_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(amipic != NULL);
	pic = &amipic->pic;

	pic->pic_numintrs = NIRQ;
	pic->pic_cookie = (void *)NULL;
	pic->pic_enable_irq = amiga_enable_irq;
	pic->pic_reenable_irq = amiga_enable_irq;
	pic->pic_disable_irq = amiga_disable_irq;
	pic->pic_get_irq = amiga_get_irq;
	pic->pic_ack_irq = amiga_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	strcpy(pic->pic_name, "amiga");
	memset(amipic->prev_level, 0, NIRQ * sizeof(int));
	pic_add(pic);

	return pic;
}

static void
amiga_enable_irq(struct pic_ops *pic, int irq, int type)
{

	custom.intena = INTF_SETCLR | (1 << irq);
}

static void
amiga_disable_irq(struct pic_ops *pic, int irq)
{

	custom.intena = 1 << irq;
}

static int
amiga_get_irq(struct pic_ops *pic, int mode)
{
	struct amiga_ops *amipic = (struct amiga_ops *)pic;
	int ipl, levels;
	uint32_t mask;

	/* Compute the interrupt's 68k IPL - the bits are active low */
	P5read(P5_IPL_EMU, levels);
	ipl = ~(levels >> 3) & 7;

	/* Store previous PPC IPL to restore on acknowledge */
	if (amipic->prev_level[ipl] != 0)
		return 255;
	amipic->prev_level[ipl] = ~levels & 7;

	mask = (uint32_t)custom.intreqr & 0x7fff;
	if (mask == 0)
		return 255;  /* no interrupt pending - spurious interrupt? */

	/* Raise the emulated PPC IPL to the interrupt's IPL (active low) */
	P5write(P5_IPL_EMU, P5_SET_CLEAR | P5_DISABLE_INT | (ipl ^ 7));
	P5write(P5_IPL_EMU, P5_DISABLE_INT | ipl);

	return 31 - __builtin_clz(mask);
}

static void
amiga_ack_irq(struct pic_ops *pic, int irq)
{
	struct amiga_ops *amipic = (struct amiga_ops *)pic;
	int ipl;

	ipl = amipic->prev_level[irq];
	amipic->prev_level[irq] = 0;

	/* Acknowledge the interrupt request */
	custom.intreq = (unsigned short)(1 << irq);

	/* Lower the emulated PPC IPL to the state before handling this irq */
	P5write(P5_IPL_EMU, P5_SET_CLEAR | P5_DISABLE_INT | (ipl ^ 7));
	P5write(P5_IPL_EMU, P5_DISABLE_INT | ipl);
}
