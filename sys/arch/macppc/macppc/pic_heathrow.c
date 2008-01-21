/*	$NetBSD: pic_heathrow.c,v 1.2.4.3 2008/01/21 09:37:28 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_heathrow.c,v 1.2.4.3 2008/01/21 09:37:28 yamt Exp $");

#include "opt_interrupt.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <arch/powerpc/pic/picvar.h>

static void heathrow_enable_irq(struct pic_ops *, int, int);
static void heathrow_reenable_irq(struct pic_ops *, int, int);
static void heathrow_disable_irq(struct pic_ops *, int);
static int  heathrow_get_irq(struct pic_ops *, int);
static void heathrow_ack_irq(struct pic_ops *, int);
static void heathrow_establish_irq(struct pic_ops *, int, int, int);

struct heathrow_ops {
	struct pic_ops pic;
	uint32_t pending_events_h;
	uint32_t pending_events_l;
	uint32_t enable_mask_h;
	uint32_t enable_mask_l;
	uint32_t level_mask_h;
	uint32_t level_mask_l;
};

static struct heathrow_ops *setup_heathrow(uint32_t);
inline void heathrow_read_events(struct heathrow_ops *);

#define INT_STATE_REG_H		((uint32_t)pic->pic_cookie + 0x10)
#define INT_ENABLE_REG_H	((uint32_t)pic->pic_cookie + 0x14)
#define INT_CLEAR_REG_H		((uint32_t)pic->pic_cookie + 0x18)
#define INT_LEVEL_REG_H		((uint32_t)pic->pic_cookie + 0x1c)
#define INT_STATE_REG_L		((uint32_t)pic->pic_cookie + 0x20)
#define INT_ENABLE_REG_L	((uint32_t)pic->pic_cookie + 0x24)
#define INT_CLEAR_REG_L		((uint32_t)pic->pic_cookie + 0x28)
#define INT_LEVEL_REG_L		((uint32_t)pic->pic_cookie + 0x2c)
#define INT_LEVEL_MASK_HEATHROW	0x1ff00000

static const char *compat[] = {
	"heathrow",
	NULL
};

int init_heathrow(void)
{
	uint32_t reg[5];
	uint32_t obio_base;
	int      heathrow;

	heathrow = OF_finddevice("/pci/mac-io");
	if (heathrow == -1)
		heathrow = OF_finddevice("mac-io");
	if (heathrow == -1)
		return FALSE;

	if (of_compatible(heathrow, compat) == -1)
		return FALSE;

	if (OF_getprop(heathrow, "assigned-addresses", reg, sizeof(reg)) != 20) 
		return FALSE;

	obio_base = reg[2];
	aprint_normal("found heathrow PIC at %08x\n", obio_base);
	setup_heathrow(obio_base);
	/* TODO: look for 2nd Heathrow */
	return TRUE;
}

static struct heathrow_ops *
setup_heathrow(uint32_t addr)
{
	struct heathrow_ops *heathrow;
	struct pic_ops *pic;

	heathrow = malloc(sizeof(struct heathrow_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(heathrow != NULL);
	pic = &heathrow->pic;

	pic->pic_numintrs = 64;
	pic->pic_cookie = (void *)addr;
	pic->pic_enable_irq = heathrow_enable_irq;
	pic->pic_reenable_irq = heathrow_reenable_irq;
	pic->pic_disable_irq = heathrow_disable_irq;
	pic->pic_get_irq = heathrow_get_irq;
	pic->pic_ack_irq = heathrow_ack_irq;
	pic->pic_establish_irq = heathrow_establish_irq;
	pic->pic_finish_setup = NULL;

	strcpy(pic->pic_name, "heathrow");
	pic_add(pic);
	heathrow->pending_events_l = 0;
	heathrow->enable_mask_l = 0;
	heathrow->level_mask_l = 0;
	heathrow->pending_events_h = 0;
	heathrow->enable_mask_h = 0;
	heathrow->level_mask_h = 0;
	out32rb(INT_ENABLE_REG_L, 0);
	out32rb(INT_CLEAR_REG_L, 0xffffffff);
	out32rb(INT_ENABLE_REG_H, 0);
	out32rb(INT_CLEAR_REG_H, 0xffffffff);
	return heathrow;
}

static void
heathrow_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct heathrow_ops *heathrow = (struct heathrow_ops *)pic;
	uint32_t mask = 1 << (irq & 0x1f);

	if (irq & 0x20) {
		heathrow->enable_mask_h |= mask;
		out32rb(INT_ENABLE_REG_H, heathrow->enable_mask_h);
	} else {
		heathrow->enable_mask_l |= mask;
		out32rb(INT_ENABLE_REG_L, heathrow->enable_mask_l);
	}
}

static void
heathrow_reenable_irq(struct pic_ops *pic, int irq, int type)
{
	struct heathrow_ops *heathrow = (struct heathrow_ops *)pic;
	uint32_t levels;
	uint32_t mask = 1 << (irq  & 0x1f);

	if (irq & 0x20) {
		heathrow->enable_mask_h |= mask;
		out32rb(INT_ENABLE_REG_H, heathrow->enable_mask_h);
		levels = in32rb(INT_LEVEL_REG_H);
		if (levels & mask) {
			pic_mark_pending(pic->pic_intrbase + irq);
			out32rb(INT_CLEAR_REG_H, mask);
		}
	} else {
		heathrow->enable_mask_l |= mask;
		out32rb(INT_ENABLE_REG_L, heathrow->enable_mask_l);
		levels = in32rb(INT_LEVEL_REG_L);
		if (levels & mask) {
			pic_mark_pending(pic->pic_intrbase + irq);
			out32rb(INT_CLEAR_REG_L, mask);
		}
	}
}

static void
heathrow_disable_irq(struct pic_ops *pic, int irq)
{
	struct heathrow_ops *heathrow = (struct heathrow_ops *)pic;
	uint32_t mask = 1 << (irq & 0x1f);

	if (irq & 0x20) {
		heathrow->enable_mask_h &= ~mask;
		out32rb(INT_ENABLE_REG_H, heathrow->enable_mask_h); 
	} else {
		heathrow->enable_mask_l &= ~mask;
		out32rb(INT_ENABLE_REG_L, heathrow->enable_mask_l); 
	}
}

inline void
heathrow_read_events(struct heathrow_ops *heathrow)
{
	struct pic_ops *pic = &heathrow->pic;
	uint32_t irqs, events, levels;

	/* first the low 32 IRQs */
	irqs = in32rb(INT_STATE_REG_L);
	events = irqs & ~heathrow->level_mask_l/*INT_LEVEL_MASK_HEATHROW*/;

	levels = in32rb(INT_LEVEL_REG_L) & heathrow->enable_mask_l;
	events |= levels & heathrow->level_mask_l/*INT_LEVEL_MASK_HEATHROW*/;
	out32rb(INT_CLEAR_REG_L, events | irqs);
	heathrow->pending_events_l |= events;

	/* then the upper 32 */
	irqs = in32rb(INT_STATE_REG_H);
	events = irqs & ~heathrow->level_mask_h;
	levels = in32rb(INT_LEVEL_REG_L) & heathrow->enable_mask_h;
	events |= levels & heathrow->level_mask_h;
	out32rb(INT_CLEAR_REG_H, events);
	heathrow->pending_events_h |= events;
}

static int
heathrow_get_irq(struct pic_ops *pic, int mode)
{
	struct heathrow_ops *heathrow = (struct heathrow_ops *)pic;
	int bit, mask;

	if ((heathrow->pending_events_h == 0) && 
	    (heathrow->pending_events_l == 0))
		heathrow_read_events(heathrow);

	if ((heathrow->pending_events_h == 0) && 
	    (heathrow->pending_events_l == 0))
		return 255;

	if (heathrow->pending_events_l != 0) {
		bit = 31 - cntlzw(heathrow->pending_events_l);
		mask = 1 << bit;
		heathrow->pending_events_l &= ~mask;
		return bit;
	}

	if (heathrow->pending_events_h != 0) {
		bit = 31 - cntlzw(heathrow->pending_events_h);
		mask = 1 << bit;
		heathrow->pending_events_h &= ~mask;
		return bit + 32;
	}
	/* we should never get here */
	return 255;
}

static void
heathrow_ack_irq(struct pic_ops *pic, int irq)
{
}

static void
heathrow_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	struct heathrow_ops *heathrow = (struct heathrow_ops *)pic;
	uint32_t mask;

	KASSERT((irq >= 0) && (irq < 64));

	mask = 1 << (irq & 0x1f);
	if (irq & 0x20) {
		if (type == IST_LEVEL) {

			heathrow->level_mask_h |= mask;
		} else {

			heathrow->level_mask_h &= ~mask;
		}
	} else {
		if (type == IST_LEVEL) {

			heathrow->level_mask_l |= mask;
		} else {

			heathrow->level_mask_l &= ~mask;
		}
	}
	aprint_debug("mask: %08x %08x\n", heathrow->level_mask_h,
	    heathrow->level_mask_l);
}
