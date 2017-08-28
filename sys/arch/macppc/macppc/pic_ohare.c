/*	$NetBSD: pic_ohare.c,v 1.12.12.2 2017/08/28 17:51:44 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pic_ohare.c,v 1.12.12.2 2017/08/28 17:51:44 skrll Exp $");

#include "opt_interrupt.h"

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/kernel.h>

#include <machine/pio.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <arch/powerpc/pic/picvar.h>

static void ohare_enable_irq(struct pic_ops *, int, int);
static void ohare_reenable_irq(struct pic_ops *, int, int);
static void ohare_disable_irq(struct pic_ops *, int);
static int  ohare_get_irq(struct pic_ops *, int);
static void ohare_ack_irq(struct pic_ops *, int);
static void ohare_establish_irq(struct pic_ops *, int, int, int);

#define OHARE_NIRQ 32

struct ohare_ops {
	struct pic_ops pic;
	uint32_t pending_events;
	uint32_t enable_mask;
	uint32_t level_mask;
	uint32_t irqs[NIPL];			/* per priority level */
	uint32_t priority_masks[OHARE_NIRQ];	/* per IRQ */
};

static struct ohare_ops *setup_ohare(uint32_t, int);
static void setup_ohare2(uint32_t, int);
static inline void ohare_read_events(struct ohare_ops *);

#define INT_STATE_REG	((uint32_t)pic->pic_cookie + 0x20)
#define INT_ENABLE_REG	((uint32_t)pic->pic_cookie + 0x24)
#define INT_CLEAR_REG	((uint32_t)pic->pic_cookie + 0x28)
#define INT_LEVEL_REG	((uint32_t)pic->pic_cookie + 0x2c)

int init_ohare(void)
{
	uint32_t reg[5];
	uint32_t obio_base;
	uint32_t irq;
	int      ohare, ohare2, is_gc = 0;

	ohare = OF_finddevice("/bandit/ohare");
	if (ohare == -1) {
		ohare = OF_finddevice("/bandit/gc");
		is_gc = 1;
	}
		

	if (OF_getprop(ohare, "assigned-addresses", reg, sizeof(reg)) != 20) 
		return FALSE;

	obio_base = reg[2];
	aprint_normal("found %s PIC at %08x\n", 
	    is_gc ? "Grand Central" : "ohare", obio_base);
	setup_ohare(obio_base, is_gc);

	/* look for 2nd ohare */
	ohare2 = OF_finddevice("/bandit/pci106b,7");
	if (ohare2 == -1)
		goto done;

	if (OF_getprop(ohare2, "assigned-addresses", reg, sizeof(reg)) < 20)
		goto done;

	if (OF_getprop(ohare2, "AAPL,interrupts", &irq, sizeof(irq)) < 4)
		goto done;

	obio_base = reg[2];			
	aprint_normal("found ohare2 PIC at %08x, irq %d\n", obio_base, irq);
	setup_ohare2(obio_base, irq);	
done:
	return TRUE;
}

static struct ohare_ops *
setup_ohare(uint32_t addr, int is_gc)
{
	struct ohare_ops *ohare;
	struct pic_ops *pic;
	int i;

	ohare = kmem_zalloc(sizeof(struct ohare_ops), KM_SLEEP);
	pic = &ohare->pic;

	pic->pic_numintrs = OHARE_NIRQ;
	pic->pic_cookie = (void *)addr;
	pic->pic_enable_irq = ohare_enable_irq;
	pic->pic_reenable_irq = ohare_reenable_irq;
	pic->pic_disable_irq = ohare_disable_irq;
	pic->pic_get_irq = ohare_get_irq;
	pic->pic_ack_irq = ohare_ack_irq;
	pic->pic_establish_irq = ohare_establish_irq;
	pic->pic_finish_setup = NULL;

	if (is_gc) {
	
		strcpy(pic->pic_name, "gc");
	} else {

		strcpy(pic->pic_name, "ohare");
	}
	ohare->level_mask = 0;

	for (i = 0; i < OHARE_NIRQ; i++)
		ohare->priority_masks[i] = 0;
	for (i = 0; i < NIPL; i++)
		ohare->irqs[i] = 0;
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

	pic = setup_ohare(addr, 0);
	strcpy(pic->pic.pic_name, "ohare2");
	intr_establish(irq, IST_LEVEL, IPL_HIGH, pic_handle_intr, pic);
}

static void
ohare_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t mask = 1 << irq;

	ohare->enable_mask |= mask;
	out32rb(INT_ENABLE_REG, ohare->enable_mask);
}

static void
ohare_reenable_irq(struct pic_ops *pic, int irq, int type)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t levels;
	uint32_t mask = 1 << irq;

	ohare->enable_mask |= mask;
	out32rb(INT_ENABLE_REG, ohare->enable_mask);
	levels = in32rb(INT_STATE_REG);
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

static inline void
ohare_read_events(struct ohare_ops *ohare)
{
	struct pic_ops *pic = &ohare->pic;
	uint32_t irqs, events, levels;

	irqs = in32rb(INT_STATE_REG);
	events = irqs & ~ohare->level_mask;

	levels = in32rb(INT_LEVEL_REG) & ohare->enable_mask;
	events |= levels & ohare->level_mask;
	out32rb(INT_CLEAR_REG, events | irqs);
	ohare->pending_events |= events;

#if 0
	if (events != 0)
		aprint_error("%s: ev %08x\n", __func__, events);
#endif
}

static int
ohare_get_irq(struct pic_ops *pic, int mode)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t evt;
	uint16_t prio;
	int bit, mask, lvl;
#ifdef OHARE_DEBUG
	int bail = 0;
#endif

	if (ohare->pending_events == 0)
		ohare_read_events(ohare);

	if (ohare->pending_events == 0)
		return 255;

	bit = 31 - __builtin_clz(ohare->pending_events);
	mask = 1 << bit;

	if ((ohare->pending_events & ~mask) == 0) {

		ohare->pending_events = 0;
		return bit;
	}

	/*
	 * if we get here we have more than one irq pending so return them
	 * according to priority
	 */

	evt = ohare->pending_events & ~mask;
	prio = ohare->priority_masks[bit];
	while (evt != 0) {
		bit = 31 - __builtin_clz(evt);
		prio |= ohare->priority_masks[bit];
		evt &= ~(1 << bit);
#ifdef OHARE_DEBUG
		bail++;
		if (bail > 31)
			panic("hanging in ohare_get_irq");
#endif
	}
	lvl = 31 - __builtin_clz(prio);
	evt = ohare->pending_events & ohare->irqs[lvl];

	if (evt == 0) {
#ifdef OHARE_DEBUG
		aprint_verbose("%s: spurious interrupt\n", 
		    ohare->pic.pic_name);
		printf("levels: %08x\n", in32rb(INT_LEVEL_REG));
		printf("states: %08x\n", in32rb(INT_STATE_REG));
		printf("enable: %08x\n", in32rb(INT_ENABLE_REG));
		printf("events: %08x\n", ohare->pending_events);
#endif
		evt = ohare->pending_events;
	}

	bit = 31 - __builtin_clz(evt);
	mask = 1 << bit;
	ohare->pending_events &= ~mask;
	return bit;	
}

static void
ohare_ack_irq(struct pic_ops *pic, int irq)
{
}

static void
ohare_establish_irq(struct pic_ops *pic, int irq, int type, int pri)
{
	struct ohare_ops *ohare = (struct ohare_ops *)pic;
	uint32_t mask = (1 << irq);
	int realpri = min(NIPL, max(0, pri)), i;
	uint32_t level = 1 << realpri;

	KASSERT((irq >= 0) && (irq < OHARE_NIRQ));

	if (type == IST_LEVEL) {

		ohare->level_mask |= mask;
	} else {

		ohare->level_mask &= ~mask;
	}
	aprint_debug("mask: %08x\n", ohare->level_mask);
	ohare->priority_masks[irq] = level;
	for (i = 0; i < NIPL; i++)
		ohare->irqs[i] = 0;
		
	for (i = 0; i < OHARE_NIRQ; i++) {
		if (ohare->priority_masks[i] == 0)
			continue;
		level = 31 - __builtin_clz(ohare->priority_masks[i]);
		ohare->irqs[level] |= (1 << i);
	}
}
