/*	$NetBSD: tmu.c,v 1.1 2002/07/05 13:31:55 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SH-5 Timer Module
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/pbridgevar.h>
#include <sh5/dev/cprcvar.h>
#include <sh5/dev/intcreg.h>
#include <sh5/dev/tmureg.h>

#include <sh5/sh5/clockvar.h>

#include "locators.h"

struct tmu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_space_handle_t sc_bush;
	struct clock_attach_args sc_ca;
	void *sc_clkih;
	void *sc_statih;
	u_int sc_period;
};

static int tmumatch(struct device *, struct cfdata *, void *);
static void tmuattach(struct device *, struct device *, void *);

struct cfattach tmu_ca = {
	sizeof(struct tmu_softc), tmumatch, tmuattach
};
extern struct cfdriver tmu_cd;

static struct tmu_softc *tmu_sc;


static void tmu_start(void *, int, u_int);
static long tmu_microtime(void *);
static int tmu_clkint(void *);
static int tmu_statint(void *);


/*ARGSUSED*/
static int
tmumatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct pbridge_attach_args *pa = args;

	if (strcmp(pa->pa_name, tmu_cd.cd_name))
		return (0);

	if ((pa->pa_ipl = cf->cf_loc[PBRIDGECF_IPL]) == PBRIDGECF_IPL_DEFAULT)
		pa->pa_ipl = IPL_CLOCK;
	else
	if (pa->pa_ipl != IPL_CLOCK)
		panic("tmumatch: pa->pa_ipl != IPL_CLOCK (%d)", IPL_CLOCK);

	if ((pa->pa_intevt = cf->cf_loc[PBRIDGECF_INTEVT]) ==
	    PBRIDGECF_INTEVT_DEFAULT)
		pa->pa_intevt = INTC_INTEVT_TMU_TUNI0;

	return (1);
}

/*ARGSUSED*/
static void
tmuattach(struct device *parent, struct device *self, void *args)
{
	struct pbridge_attach_args *pa = args;
	struct tmu_softc *sc;
	int i;

	tmu_sc = sc = (struct tmu_softc *)self;

	sc->sc_bust = pa->pa_bust;
	bus_space_map(sc->sc_bust, pa->pa_offset, TMU_REG_SIZE, 0,&sc->sc_bush);

	/*
	 * Disable the timers
	 */
	bus_space_write_1(sc->sc_bust, sc->sc_bush, TMU_REG_TOCR, 0);
	bus_space_write_1(sc->sc_bust, sc->sc_bush, TMU_REG_TSTR, 0);

	for (i = 0; i < TMU_NTIMERS; i++)
		bus_space_write_2(sc->sc_bust, sc->sc_bush, TMU_REG_TCR(i), 0);

	/*
	 * Hook the timer interrupts.
	 * Note that passing NULL as the "arg" parameter tells the interrupt
	 * dispatcher to pass our handlers a pointer to the interrupt frame.
	 */
	sc->sc_clkih = sh5_intr_establish(pa->pa_intevt, IST_LEVEL,
	    pa->pa_ipl, tmu_clkint, NULL);
	sc->sc_statih = sh5_intr_establish(pa->pa_intevt + 0x20, IST_LEVEL,
	    pa->pa_ipl, tmu_statint, NULL);

	/*
	 * Calculate the number of nano-seconds per timer tick.
	 * This will be used in tmu_microtime() to return the
	 * number of micro-seconds since the last underflow.
	 */
	sc->sc_period = 1000000000 / (cprc_clocks.cc_peripheral / 4);

	printf(": Timer Unit\n");
	printf("%s: Tick period: %d nS\n", sc->sc_dev.dv_xname, sc->sc_period);

	/*
	 * Attach to the common clock back-end
	 */
	sc->sc_ca.ca_rate = cprc_clocks.cc_peripheral / 4;
	sc->sc_ca.ca_has_stat_clock = 1;
	sc->sc_ca.ca_arg = sc;
	sc->sc_ca.ca_start = tmu_start;
	sc->sc_ca.ca_microtime = tmu_microtime;
	clock_config(self, &sc->sc_ca, sh5_intr_evcnt(sc->sc_clkih));
}

static void
tmu_start(void *arg, int which, u_int clkint)
{
	struct tmu_softc *sc = arg;
	u_int32_t tcor;
	u_int16_t tcr;
	int timer;

	switch (which) {
	case CLK_HARDCLOCK:
		timer = 0;
		break;

	case CLK_STATCLOCK:
		timer = 1;
		break;

	default:
		return;
	}

	/*
	 * The "clkint" parameter specifies the number of uS per clock
	 * interrupt. We need to convert that to something which can be
	 * loaded into the Timer Constant register.
	 */
	tcor = sc->sc_ca.ca_rate / (1000000 / clkint);
	bus_space_write_4(sc->sc_bust, sc->sc_bush, TMU_REG_TCOR(timer), tcor);

	/*
	 * If the timer is not yet enabled, set the TCNT register to
	 * the same as TCOR, and enable underflow interrupts.
	 */
	tcr = bus_space_read_2(sc->sc_bust, sc->sc_bush, TMU_REG_TCR(timer));
	if ((tcr & TMU_TCR_UNIE) == 0) {
		bus_space_write_4(sc->sc_bust, sc->sc_bush,
		    TMU_REG_TCNT(timer), tcor);
		bus_space_write_2(sc->sc_bust, sc->sc_bush, TMU_REG_TCR(timer),
		    TMU_TCR_TPSC_PDIV4 | TMU_TCR_CKEG_RISING | TMU_TCR_UNIE);
	}
}

static long
tmu_microtime(void *arg)
{
	struct tmu_softc *sc = arg;
	u_int32_t tcnt, tcor;

	tcnt = bus_space_read_4(sc->sc_bust, sc->sc_bush, TMU_REG_TCNT(0));
	tcor = bus_space_read_4(sc->sc_bust, sc->sc_bush, TMU_REG_TCOR(0));

	return ((long)(((tcor - tcnt) * sc->sc_period) / 1000));
}

static int
tmu_clkint(void *arg)
{

	/* Clear down the underflow interrupt */
	bus_space_write_2(tmu_sc->sc_bust, tmu_sc->sc_bush, TMU_REG_TCR(0), 
	    TMU_TCR_TPSC_PDIV4 | TMU_TCR_CKEG_RISING | TMU_TCR_UNIE);

	/* The interrupt frame can be cast directly to struct clockframe */
	clock_hardint((struct clockframe *)arg);

	return (1);
}

static int
tmu_statint(void *arg)
{

	/* Clear down the underflow interrupt */
	bus_space_write_2(tmu_sc->sc_bust, tmu_sc->sc_bush, TMU_REG_TCR(1), 
	    TMU_TCR_TPSC_PDIV4 | TMU_TCR_CKEG_RISING | TMU_TCR_UNIE);

	/* The interrupt frame can be cast directly to struct clockframe */
	clock_statint((struct clockframe *)arg);

	return (1);
}
