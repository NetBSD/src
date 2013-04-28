/*	$NetBSD: clpssoc.c,v 1.1 2013/04/28 11:57:13 kiyohara Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: clpssoc.c,v 1.1 2013/04/28 11:57:13 kiyohara Exp $");

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

#include <arm/clps711x/clps711xreg.h>
#include <arm/clps711x/clpssocvar.h>

#include "locators.h"


static int clpssoc_match(device_t, cfdata_t, void *);
static void clpssoc_attach(device_t parent, device_t self, void *aux);

static int clpssoc_print(void *, const char *);
static int clpssoc_submatch(device_t, cfdata_t, const int *, void *);

static int clpssoc_find_pending_irqs(void);

static void clpssoc_pic_unblock_irqs(struct pic_softc *, size_t, uint32_t);
static void clpssoc_pic_block_irqs(struct pic_softc *, size_t, uint32_t);
static void clpssoc_pic_establish_irq(struct pic_softc *, struct intrsource *);

#define INTSR	(*((volatile uint16_t *)(ARM7XX_INTRREG_VBASE + PS711X_INTSR)))
#define INTMR	(*((volatile uint16_t *)(ARM7XX_INTRREG_VBASE + PS711X_INTMR)))
#define SYSCON	(*((volatile uint32_t *)(ARM7XX_INTRREG_VBASE + PS711X_SYSCON)))
#define TC1D	(*((volatile uint16_t *)(ARM7XX_INTRREG_VBASE + PS711X_TC1D)))
#define TC2D	(*((volatile uint16_t *)(ARM7XX_INTRREG_VBASE + PS711X_TC2D)))
static void clpssoc_initclocks(void);
static int clpssoc_clockintr(void *);
static void clpssoc_tc_init(void);
static u_int clpssoc_get_timecount(struct timecounter *);

static void clpssoc_delay(unsigned int);

CFATTACH_DECL_NEW(clpssoc, 0, clpssoc_match, clpssoc_attach, NULL, NULL);

static const struct {
	const char *name;
	int irq[3];
} clpssoc_periphs[] = {
	{ "clpsaudio",	{CLPSSOCCF_IRQ_DEFAULT} },
	{ "clpscom",	{12, 13, 14} },
	{ "clpskbd",	{CLPSSOCCF_IRQ_DEFAULT} },
	{ "clpslcd",	{CLPSSOCCF_IRQ_DEFAULT} },
	{ "clpspm",	{CLPSSOCCF_IRQ_DEFAULT} },
	{ "clpsrtc",	{10, CLPSSOCCF_IRQ_DEFAULT} },
};

extern struct bus_space clps711x_bs_tag;

static struct pic_ops clpssoc_picops = {
	.pic_unblock_irqs = clpssoc_pic_unblock_irqs,
	.pic_block_irqs = clpssoc_pic_block_irqs,
	.pic_establish_irq = clpssoc_pic_establish_irq,
};
static struct pic_softc clpssoc_pic = {
	.pic_ops = &clpssoc_picops,
	.pic_maxsources = 16,
	.pic_name = "clpssoc",
};

/* ARGSUSED */
static int
clpssoc_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/* ARGSUSED */
static void
clpssoc_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *maa = aux;
	struct clpssoc_attach_args aa;
	bus_space_handle_t ioh;
	uint32_t sysflg;
	int i, j;

	if (bus_space_map(&clps711x_bs_tag, maa->mb_iobase, ARM7XX_INTRREG_SIZE,
								0, &ioh) != 0) {
		aprint_error_dev(self, "can't map registers\n");
		return;
	}
	sysflg = bus_space_read_4(&clps711x_bs_tag, ioh, PS711X_SYSFLG);
	aprint_normal(": CL PS-711x rev %d\n", SYSFLG_VERID(sysflg));
	aprint_naive("\n");

	INTMR = 0;

	pic_add(&clpssoc_pic, 0);
	soc_find_pending_irqs = clpssoc_find_pending_irqs;
	soc_initclocks = clpssoc_initclocks;
	soc_delay = clpssoc_delay;

	for (i = 0; i < __arraycount(clpssoc_periphs); i++) {
		aa.aa_name = clpssoc_periphs[i].name;
		aa.aa_iot = &clps711x_bs_tag;
		aa.aa_ioh = &ioh;
		for (j = 0; j < __arraycount(clpssoc_periphs[i].irq); j++)
			aa.aa_irq[j] = clpssoc_periphs[i].irq[j];

		config_found_sm_loc(self, "clpssoc", NULL, &aa,
		    clpssoc_print, clpssoc_submatch);
	}
}

static int
clpssoc_print(void *aux, const char *pnp)
{
	struct clpssoc_attach_args *aa = aux;
	int i;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);
	else
		if (aa->aa_irq[0] != CLPSSOCCF_IRQ_DEFAULT) {
			aprint_normal(" irq %d", aa->aa_irq[0]);
			for (i = 1; i < __arraycount(aa->aa_irq); i++) {
				if (aa->aa_irq[i] == CLPSSOCCF_IRQ_DEFAULT)
					break;
				aprint_normal(",%d", aa->aa_irq[i]);
			}
		}

	return UNCONF;
}

/* ARGSUSED */
static int
clpssoc_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct clpssoc_attach_args *aa = aux;

	if (strcmp(aa->aa_name, "clpskbd") == 0 &&
	    strcmp(cf->cf_name + strlen(cf->cf_name) - 3, "kbd") == 0)
		return config_match(parent, cf, aux);

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		return 0;
	return config_match(parent, cf, aux);
}


static int
clpssoc_find_pending_irqs(void)
{
	uint16_t pending;

	pending = INTSR;
	pending &= INTMR;
	if (pending == 0)
		return 0;

	return pic_mark_pending_sources(&clpssoc_pic, 0, pending);
}

/* ARGSUSED */
static void
clpssoc_pic_unblock_irqs(struct pic_softc *pic, size_t irqbase,
			 uint32_t irq_mask)
{

	INTMR |= irq_mask;
}

/* ARGSUSED */
static void
clpssoc_pic_block_irqs(struct pic_softc *pic, size_t irqbase, uint32_t irq_mask)
{

	INTMR &= ~irq_mask;
}

static void
clpssoc_pic_establish_irq(struct pic_softc *pic, struct intrsource *is)
{
	/* Nothing */
}

static void
clpssoc_initclocks(void)
{

	/* set prescale mode to TC1 and free-running mode TC2 */
	SYSCON |= (SYSCON_TC1M | SYSCON_TC1S | SYSCON_TC2S);
	TC1D = 512 * 1000 / hz - 1;	/* 512kHz / hz - 1 */
	intr_establish(IRQ_TC1DI, IPL_CLOCK, 0, clpssoc_clockintr, NULL);

	clpssoc_tc_init();
}

static int
clpssoc_clockintr(void *arg)
{

	*(volatile uint32_t *)(ARM7XX_INTRREG_VBASE + PS711X_TC1EOI) = 1;

	hardclock(arg);

	return 1;
}

static void
clpssoc_tc_init(void)
{
	static struct timecounter clpssoc_tc = {
		.tc_get_timecount = clpssoc_get_timecount,
		.tc_counter_mask = UINT16_MAX,
		.tc_frequency = 512000,
		.tc_name = "clpssoc",
		.tc_quality = 100,
	};

	tc_init(&clpssoc_tc);
}

static u_int
clpssoc_get_timecount(struct timecounter *tc)
{

	return TC2D ^ UINT16_MAX;	/* It is decremental counter */
}

static void
clpssoc_delay(unsigned int us)
{
	int prev, now, remaining;

	prev = TC2D & UINT16_MAX;
	remaining = us * 512 / 1000 + 1;

	while (remaining > 0) {
		now = TC2D & UINT16_MAX;
		if (now >= prev)
			remaining -= (now - prev);
		else
			remaining -= (UINT16_MAX - now + prev + 1);
		prev = now;
	}
}
