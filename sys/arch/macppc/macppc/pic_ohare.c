/*	$NetBSD: pic_ohare.c,v 1.1.2.1 2007/05/02 03:02:34 macallan Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_ohare.c,v 1.1.2.1 2007/05/02 03:02:34 macallan Exp $");

#include "opt_interrupt.h"

#ifdef PIC_OHARE

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <arch/powerpc/pic/picvar.h>

static int  ohare_irq_is_enabled(struct pic_ops *, int);
static void ohare_enable_irq(struct pic_ops *, int);
static void ohare_reenable_irq(struct pic_ops *, int);
static void ohare_disable_irq(struct pic_ops *, int);
static void ohare_clear_irq(struct pic_ops *, int);
static int  ohare_get_irq(struct pic_ops *, int);
static void ohare_ack_irq(struct pic_ops *, int);

struct ohare_ops {
	struct pic_ops pic;
	uint32_t pending_events;
	uint32_t enable_mask;
};

static struct ohare_ops *setup_ohare(uint32_t);
static void setup_ohare2(uint32_t, int);
inline void ohare_read_events(struct ohare_ops *);

#define INT_STATE_REG	((uint32_t)pic->pic_cookie + 0x20)
#define INT_ENABLE_REG	((uint32_t)pic->pic_cookie + 0x24)
#define INT_CLEAR_REG	((uint32_t)pic->pic_cookie + 0x28)
#define INT_LEVEL_REG	((uint32_t)pic->pic_cookie + 0x2c)
#define INT_LEVEL_MASK_OHARE	0x1ff00000	/* also for Heathrow */

int init_ohare(void)
{
	uint32_t reg[5];
	uint32_t obio_base;
	uint32_t irq;
	int      ohare, ohare2;

	ohare = OF_finddevice("/bandit/ohare");
	if (ohare == -1)
		return FALSE;

	if (OF_getprop(ohare, "assigned-addresses", reg, sizeof(reg)) != 20) 
		return FALSE;

	obio_base = reg[2];
	aprint_normal("found ohare PIC at %08x\n", obio_base);
	setup_ohare(obio_base);

	/* look for 2nd ohare */
	ohare2 = OF_finddevice("/bandit/pci106b,7");
	if (ohare2 == -1)
		goto done;

	if (OF_getprop(ohare2, "assigned-addresses", reg, sizeof(reg)) < 20)
		goto done;

	if (OF_getprop(ohare2, "AAPL,interrupts", &irq, sizeof(irq)) < 4)
		goto done;

	obio_base = reg[2];			
	aprint_normal("found ohare2 PIC at %08x\n", obio_base);
	setup_ohare2(obio_base, irq);	
done:
	return TRUE;
}

static struct ohare_ops *
setup_ohare(uint32_t addr)
{
	struct ohare_ops *ohare;
	struct pic_ops *pic;

	ohare = malloc(sizeof(struct ohare_ops), M_DEVBUF, M_NOWAIT);
	KASSERT(ohare != NULL);
	pic = &ohare->pic;

	pic->pic_numintrs = 32;
	pic->pic_cookie = (void *)addr;
	pic->pic_irq_is_enabled = ohare_irq_is_enabled;
	pic->pic_enable_irq = ohare_enable_irq;
	pic->pic_reenable_irq = ohare_reenable_irq;
	pic->pic_disable_irq = ohare_disable_irq;
	pic->pic_clear_irq = ohare_clear_irq;
	pic->pic_get_irq = ohare_get_irq;
	pic->pic_ack_irq = ohare_ack_irq;
	strcpy(pic->pic_name, "ohare");
	pic_add(pic);
	ohare->pending_events = 0;
	ohare->enable_mask = 0;
	out32rb(INT_ENABLE_REG, 0);
	out32rb(INT_CLEAR_REG, 0xffffffff);
	return ohare;
}

static void
setup_ohare2(uint32_t addr, int irq)
{
	struct ohare_ops *pic;

	pic = setup_ohare(addr);
	strcpy(pic->pic.pic_name, "ohare2");
	intr_establish(irq, IST_LEVEL, IPL_NONE, pic_handle_intr, pic);
}

static int
ohare_irq_is_enabled(struct pic_ops *pic, int irq)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t mask = 1 << irq;
	return ((ohare->enable_mask & mask) != 0); 
}

static void
ohare_enable_irq(struct pic_ops *pic, int irq)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t mask = 1 << irq;

	ohare->enable_mask |= mask;
	out32rb(INT_ENABLE_REG, ohare->enable_mask);
}

static void
ohare_reenable_irq(struct pic_ops *pic, int irq)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t levels;
	uint32_t mask = 1 << irq;

	ohare->enable_mask |= mask;
	out32rb(INT_ENABLE_REG, ohare->enable_mask);
	levels = in32rb(INT_LEVEL_REG);
	if (levels & mask) {
		pic_mark_pending(pic->pic_intrbase + irq);
		out32rb(INT_CLEAR_REG, mask);
	}
}

static void
ohare_disable_irq(struct pic_ops *pic, int irq)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t mask = 1 << irq;

	ohare->enable_mask &= ~mask;
	out32rb(INT_ENABLE_REG, ohare->enable_mask); 
}

static void
ohare_clear_irq(struct pic_ops *pic, int irq)
{
	uint32_t mask = 1 << irq;

	out32rb(INT_CLEAR_REG, mask);
}

inline void
ohare_read_events(struct ohare_ops *ohare)
{
	struct pic_ops *pic = &ohare->pic;
	uint32_t irqs, events, levels;

	irqs = in32rb(INT_STATE_REG);
	events = irqs & ~INT_LEVEL_MASK_OHARE;

	levels = in32rb(INT_LEVEL_REG) & ohare->enable_mask;
	events |= levels & INT_LEVEL_MASK_OHARE;
	out32rb(INT_CLEAR_REG, events | irqs);
	ohare->pending_events |= events;

#if 0
	if (events != 0)
		aprint_error("%s: ev %08x\n", __func__, events);
#endif
}

static int
ohare_get_irq(struct pic_ops *pic, int cpu)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	int bit, mask;

	if (ohare->pending_events == 0)
		ohare_read_events(ohare);

	if (ohare->pending_events == 0)
		return 255;

	bit = 31 - cntlzw(ohare->pending_events);
	mask = 1 << bit;
	ohare->pending_events &= ~mask;

	return bit;
}

static void
ohare_ack_irq(struct pic_ops *pic, int cpu)
{
}

#endif /* PIC_OHARE */
