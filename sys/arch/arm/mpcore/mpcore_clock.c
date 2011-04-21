/*	$NetBSD: mpcore_clock.c,v 1.1.2.2 2011/04/21 01:40:52 rmind Exp $ */
/*
 * Copyright (c) 2009, 2010, 2011  Genetec corp.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec corp.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mpcore_clock.c,v 1.1.2.2 2011/04/21 01:40:52 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <sys/types.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <arm/mpcore/mpcorereg.h>
#include <arm/mpcore/mpcorevar.h>

#ifndef	MPCORE_CPU_FREQ
#error	define MPCORE_CPU_FREQ
#endif

static volatile uint32_t mpcoreclk_base;

static u_int mpcore_get_timecount(struct timecounter *);
static int mpcoreclk_intr(void *);

static struct timecounter mpcore_timecounter = {
	mpcore_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	0,			/* frequency */
	"cpuclock",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

struct mpcoreclk_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_intr;

	int sc_reload_value;

	void *sc_ih;			/* interrupt handler */
};

static struct mpcoreclk_softc *mpcoreclk;

static int mpcoreclk_match(device_t, struct cfdata *, void *);
static void mpcoreclk_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mpcoreclk, sizeof(struct mpcoreclk_softc),
    mpcoreclk_match, mpcoreclk_attach, NULL, NULL);

static int
mpcoreclk_match(device_t parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "mpcoreclk") == 0)
		return 1;

	return 0;
}

static void
mpcoreclk_attach(device_t parent, device_t self, void *aux)
{
	struct mpcoreclk_softc *sc = device_private(self);
	struct pmr_attach_args * const pa = aux;

	aprint_normal(": internal timer\n");
	aprint_naive("\n");

	sc->sc_dev = self;
	sc->sc_iot = pa->pa_iot;
	sc->sc_intr = pa->pa_irq;

	mpcoreclk = sc;

	if (bus_space_subregion(sc->sc_iot, pa->pa_ioh,
		MPCORE_PMR_TIMER, MPCORE_PMR_TIMER_SIZE,
		&sc->sc_ioh)){
		aprint_error_dev(self, "can't subregion\n");
		return;
	}

}

void
cpu_initclocks(void)
{
	int freq;

	if (!mpcoreclk) {
		panic("%s: driver has not been initialized!", __func__);
	}

	freq = MPCORE_CPU_FREQ / 2;
	mpcore_timecounter.tc_frequency = freq;
	mpcoreclk->sc_reload_value = freq / hz;

	/* stop all timers */
	bus_space_write_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh,
	    PMR_CLK_CONTROL, 0);

	aprint_normal("clock: hz=%d stathz = %d\n", hz, stathz);

	
	bus_space_write_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh, PMR_CLK_LOAD,
	    mpcoreclk->sc_reload_value);
	bus_space_write_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh, PMR_CLK_COUNTER,
	    mpcoreclk->sc_reload_value);
	bus_space_write_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh, PMR_CLK_CONTROL,
	    (0 << CLK_CONTROL_PRESCALER_SHIFT) |
	    CLK_CONTROL_ENABLE | CLK_CONTROL_AUTOLOAD |
	    CLK_CONTROL_ITENABLE);

	mpcoreclk->sc_ih = intr_establish(mpcoreclk->sc_intr, IPL_CLOCK,
	    IST_LEVEL, mpcoreclk_intr, NULL);

	tc_init(&mpcore_timecounter);

}

#if 0
void
microtime(struct timeval *tvp)
{
}
#endif

void
setstatclockrate(int schz)
{
}

static int
mpcoreclk_intr(void *arg)
{
	uint32_t reg;

	reg = bus_space_read_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh,
	    PMR_CLK_INTR);
	if (reg) {
		/* clear status */
		bus_space_write_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh,
		    PMR_CLK_INTR, reg);

		atomic_add_32(&mpcoreclk_base, mpcoreclk->sc_reload_value);

	}

	hardclock((struct clockframe *)arg);

	return 1;
}

u_int
mpcore_get_timecount(struct timecounter *tc)
{
	uint32_t counter;
	uint32_t base;
	u_int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	counter = bus_space_read_4(mpcoreclk->sc_iot, mpcoreclk->sc_ioh,
	    PMR_CLK_COUNTER);
	base = mpcoreclk_base;
	restore_interrupts(oldirqstate);

	return base - counter;
}
