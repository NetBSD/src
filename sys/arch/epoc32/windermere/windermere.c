/*	$NetBSD: windermere.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: windermere.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $");

#include "opt_com.h"

#define _INTR_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/types.h>

#include <arm/mainbus/mainbus.h>
#include <arm/pic/picvar.h>

#include <machine/epoc32.h>
#include <machine/limits.h>

#include <epoc32/windermere/windermerereg.h>
#include <epoc32/windermere/windermerevar.h>

#include "locators.h"


static int windermere_match(device_t, cfdata_t, void *);
static void windermere_attach(device_t parent, device_t self, void *aux);

static int windermere_print(void *, const char *);
static int windermere_submatch(device_t, cfdata_t, const int *, void *);

static int windermere_find_pending_irqs(void);

static void windermere_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void windermere_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void windermere_pic_establish_irq(struct pic_softc *,
					 struct intrsource *);

#define TC_READ(base, off)		(*((base) + (off)))
#define TC_WRITE(base, off, val)	(*((base) + (off)) = (val))
static void windermere_initclocks(void);
static int windermere_clockintr(void *);
static void windermere_tc_init(void);
static u_int windermere_get_timecount(struct timecounter *);

static void windermere_delay(unsigned int);

CFATTACH_DECL_NEW(windermere, 0,
    windermere_match, windermere_attach, NULL, NULL);

static const struct {
	const char *name;
	bus_addr_t offset;
	int irq;
} windermere_periphs[] = {
	{ "wmlcd",	0x200, WINDERMERECF_IRQ_DEFAULT },
	{ "wmcom",	0x600, 12 },
	{ "wmcom",	0x700, 13 },
	{ "wmrtc",	0xd00, 10 },
	{ "wmpm",	WINDERMERECF_OFFSET_DEFAULT, WINDERMERECF_IRQ_DEFAULT },
	{ "wmaudio",	WINDERMERECF_OFFSET_DEFAULT, WINDERMERECF_IRQ_DEFAULT },
	{ "wmkbd",	0xe28, WINDERMERECF_IRQ_DEFAULT },
};

static struct pic_ops windermere_picops = {
	.pic_unblock_irqs = windermere_pic_unblock_irqs,
	.pic_block_irqs = windermere_pic_block_irqs,
	.pic_establish_irq = windermere_pic_establish_irq,
};
static struct pic_softc windermere_pic = {
	.pic_ops = &windermere_picops,
	.pic_maxsources = 16,
	.pic_name = "windermere",
};
static uint16_t pic_mask = 0x0000;

/* ARGSUSED */
static int
windermere_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/* ARGSUSED */
static void
windermere_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *maa = aux;
	struct windermere_attach_args aa;
	bus_space_handle_t ioh;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");

	if (bus_space_map(maa->mb_iot, maa->mb_iobase, ARM7XX_INTRREG_SIZE, 0,
								&ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}

	pic_add(&windermere_pic, 0);
	soc_find_pending_irqs = windermere_find_pending_irqs;
	soc_initclocks = windermere_initclocks;
	soc_delay = windermere_delay;

	for (i = 0; i < __arraycount(windermere_periphs); i++) {
		aa.aa_name = windermere_periphs[i].name;
		aa.aa_iot = maa->mb_iot;
		aa.aa_ioh = &ioh;
		aa.aa_offset = windermere_periphs[i].offset;
		aa.aa_size = 0;
		aa.aa_irq = windermere_periphs[i].irq;

		config_found_sm_loc(self, "windermere", NULL, &aa,
		    windermere_print, windermere_submatch);
	}
}

static int
windermere_print(void *aux, const char *pnp)
{
	struct windermere_attach_args *aa = aux;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);
	else {
		if (aa->aa_offset != WINDERMERECF_OFFSET_DEFAULT) {
			aprint_normal(" offset 0x%04lx", aa->aa_offset);
			if (aa->aa_size > 0)
				aprint_normal("-0x%04lx",
				    aa->aa_offset + aa->aa_size - 1);
		}
		if (aa->aa_irq != WINDERMERECF_IRQ_DEFAULT)
			aprint_normal(" irq %d", aa->aa_irq);
	}

	return UNCONF;
}

/* ARGSUSED */
static int
windermere_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct windermere_attach_args *aa = aux;

	/*
	 * special case. match *kbd@windermere.
	 * Windermere helps implement to machine-dependent keyboard.
	 */
	if (strcmp(aa->aa_name, "wmkbd") == 0 &&
	    strcmp(cf->cf_name + strlen(cf->cf_name) - 3, "kbd") == 0)
		return config_match(parent, cf, aux);

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	return config_match(parent, cf, aux);
}


static int
windermere_find_pending_irqs(void)
{
	volatile uint32_t *intr =
	    (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_INTR_OFFSET);
	uint16_t pending;

	pending = *(intr + INTSR);
	if (pending & ~pic_mask) {
		*(intr + INTENC) = pending & ~pic_mask;
		printf("stray interrupt pending: 0x%04x\n",
		    pending & ~pic_mask);
		pending &= pic_mask;
	}
	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&windermere_pic, 0, pending);
}

/* ARGSUSED */
static void
windermere_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
			    uint32_t irq_mask)
{
	volatile uint32_t *intr =
	    (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_INTR_OFFSET);

	*(intr + INTENS) = irq_mask;
	pic_mask |= irq_mask;
}

/* ARGSUSED */
static void
windermere_pic_block_irqs(struct pic_softc *pic, size_t irqbase,
			  uint32_t irq_mask)
{
	volatile uint32_t *intr =
	    (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_INTR_OFFSET);

	*(intr + INTENC) = irq_mask;
	pic_mask &= ~irq_mask;
}

static void
windermere_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing */
}

static void
windermere_initclocks(void)
{
	volatile uint32_t *tc1, *tc2;
	uint32_t ctrl;

	tc1 =
	 (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_TC_OFFSET + TC1_OFFSET);
	tc2 =
	 (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_TC_OFFSET + TC2_OFFSET);

	/* set clock to TC1 */ 
	ctrl = TC_READ(tc1, TC_CTRL);
	ctrl |= (CTRL_CLKSEL | CTRL_MODE | CTRL_ENABLE); /* select 512kHz */
	TC_WRITE(tc1, TC_CTRL, ctrl);
	TC_WRITE(tc1, TC_LOAD, 512 * 1000 / hz - 1);	/* 512kHz / hz - 1 */
	intr_establish(8, IPL_CLOCK, 0, windermere_clockintr, NULL);

	/* set delay counter */ 
	ctrl = TC_READ(tc2, TC_CTRL);
	ctrl |= (CTRL_CLKSEL | CTRL_MODE | CTRL_ENABLE); /* select 512kHz */
	TC_WRITE(tc2, TC_CTRL, ctrl);
	TC_WRITE(tc2, TC_LOAD, TC_MAX);

	windermere_tc_init();
}

static int
windermere_clockintr(void *arg)
{
	volatile uint32_t *tc1;

	tc1 =
	 (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_TC_OFFSET + TC1_OFFSET);
	TC_WRITE(tc1, TC_EOI, EOI_EOI);

	hardclock(arg);

	return 1;
}

static void
windermere_tc_init(void)
{
	static struct timecounter windermere_tc = {
		.tc_get_timecount = windermere_get_timecount,
		.tc_counter_mask = TC_MASK,
		.tc_frequency = 512000,
		.tc_name = "windermere",
		.tc_quality = 100,
	};

	tc_init(&windermere_tc);
}

static u_int
windermere_get_timecount(struct timecounter *tc)
{
	volatile uint32_t *tc2;

	tc2 =
	 (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_TC_OFFSET + TC2_OFFSET);

	return TC_READ(tc2, TC_VAL) ^ TC_MASK;	/* It is decremental counter */
}

static void
windermere_delay(unsigned int us)
{
	volatile uint32_t *tc2;
	int prev, now, remaining;

	tc2 =
	 (uint32_t *)(ARM7XX_INTRREG_VBASE + WINDERMERE_TC_OFFSET + TC2_OFFSET);

	prev = TC_READ(tc2, TC_VAL) & TC_MASK;
	remaining = us * 512 / 1000 + 1;

	while (remaining > 0) {
		now = TC_READ(tc2, TC_VAL) & TC_MASK;
		if (now >= prev)
			remaining -= (now - prev);
		else
			remaining -= (USHRT_MAX - now + prev + 1);
		prev = now;
	}
}
