/*	$NetBSD: gemini_timer.c,v 1.4 2010/01/05 13:14:56 mbalmer Exp $	*/

/* adapted from:
 *	NetBSD: omap2_geminitmr.c,v 1.1 2008/08/27 11:03:10 matt Exp
 */

/*
 * GEMINI Timers
 */

/*
 * Based on i80321_timer.c and arch/arm/sa11x0/sa11x0_ost.c
 *
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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
 *
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_timer.c,v 1.4 2010/01/05 13:14:56 mbalmer Exp $");

#include "opt_gemini.h"
#include "opt_cpuoptions.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>
#include <arm/pic/picvar.h>

#include <arm/gemini/gemini_reg.h>
#include <arm/gemini/gemini_timervar.h>
#include <arm/gemini/gemini_timervar.h>


static const uint32_t counts_per_usec = (GEMINI_TIMER_CLOCK_FREQ / 1000000);
static uint32_t counts_per_hz = ~0;

struct geminitmr_softc *clock_sc;
struct geminitmr_softc *stat_sc;
struct geminitmr_softc *ref_sc;
static uint32_t gemini_get_timecount(struct timecounter *);
static void timer_init(geminitmr_softc_t *, int, boolean_t, boolean_t);
static void timer_factors(geminitmr_softc_t *, int, boolean_t);

#ifdef GEMINI_TIMER_DEBUG
static void tfprint(uint, timer_factors_t *);
#endif

static struct timecounter gemini_timecounter = {
	.tc_get_timecount = gemini_get_timecount,
	.tc_counter_mask = 0xffffffff,
	.tc_frequency = GEMINI_TIMER_CLOCK_FREQ,
	.tc_name = "gpt",
	.tc_quality = 100,
	.tc_priv = NULL
};

static inline void
_timer_intr_dis(struct geminitmr_softc *sc)
{
	uint32_t r;

	r  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_INTRMASK);
	r |= GEMINI_TIMERn_INTRMASK(sc->sc_timerno);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_INTRMASK, r);
}

static inline void
_timer_intr_enb(struct geminitmr_softc *sc)
{
	uint32_t r;

	r  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_INTRMASK);
	r &= ~TIMER_INTRMASK_TMnMATCH1(sc->sc_timerno);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_INTRMASK, r);
}

static inline void
_timer_intr_clr(struct geminitmr_softc *sc)
{
	uint32_t r;
	int psw;

	psw = disable_interrupts(I32_bit);
	r  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_INTRSTATE);
	r &= ~GEMINI_TIMERn_INTRMASK(sc->sc_timerno);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_INTRSTATE, r);
	restore_interrupts(psw);
}

static inline uint32_t
_timer_read(struct geminitmr_softc *sc)
{
	uint32_t r;

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_TIMERn_COUNTER(sc->sc_timerno));

	return r;
}

static inline void
_timer_stop(struct geminitmr_softc *sc)
{
	uint32_t r;

	r  = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_TMCR);
	r &= ~GEMINI_TIMER_TMnCR_MASK(sc->sc_timerno);
}

/*
 * note:
 *  This function assumes the timer is enabled.
 *  If the timer is disabled, GEMINI_TIMERn_COUNTER(n) will hold the value.
 */
static inline void
_timer_reload(struct geminitmr_softc *sc, uint32_t val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_TIMERn_COUNTER(sc->sc_timerno), val);
}

static inline void
_timer_start(struct geminitmr_softc *sc)
{
	uint32_t r;
	uint n = sc->sc_timerno;
	timer_factors_t *tfp = &sc->sc_tf;

	/* set Counter, TmLoad, Match1, Match2 */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_TIMERn_COUNTER(n), tfp->tf_counter);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_TIMERn_LOAD(n), tfp->tf_reload);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_TIMERn_MATCH1(n), tfp->tf_match1);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		GEMINI_TIMERn_MATCH2(n), tfp->tf_match2);

	/* set TmCR */
	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_TMCR);
	r &= ~GEMINI_TIMER_TMnCR_MASK(n);
	r |= tfp->tf_tmcr & GEMINI_TIMER_TMnCR_MASK(n);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, GEMINI_TIMER_TMCR, r);

}

static uint32_t
gemini_get_timecount(struct timecounter *tc)
{
	uint32_t r;

	r = _timer_read(ref_sc);

	return -r;
}

int
clockintr(void *frame)
{
	struct geminitmr_softc *sc = clock_sc;

	_timer_intr_clr(sc);
	_timer_reload(sc, sc->sc_tf.tf_counter);
	hardclock(frame);
	if (clock_sc == stat_sc)
		statclock(frame);
	return 1;
}

int
statintr(void *frame)
{
	struct geminitmr_softc *sc = stat_sc;

	_timer_intr_clr(sc);
	_timer_reload(sc, sc->sc_tf.tf_counter);
	statclock(frame);
	return 1;
}

static void
timer_init(geminitmr_softc_t *sc, int schz, boolean_t autoload, boolean_t intr)
{
	int psw;

	psw = disable_interrupts(I32_bit);
	timer_factors(sc, schz, autoload);
	_timer_stop(sc);
	_timer_intr_dis(sc);
	_timer_intr_clr(sc);
	if (intr)
		_timer_intr_enb(sc);
	_timer_start(sc);
	psw = disable_interrupts(I32_bit);
}

void
gemini_microtime_init(void)
{
	if (ref_sc == NULL)
		panic("microtime reference timer was not configured.");
	timer_init(ref_sc, 0, TRUE, FALSE);
}

void
setstatclockrate(int schz)
{
	if (stat_sc == NULL)
		panic("Statistics timer was not configured.");
	if (stat_sc != clock_sc)
		timer_init(stat_sc, schz, FALSE, TRUE);
}

/*
 * clock_sc and stat_sc starts here
 * ref_sc is initialized already by obiotimer_attach
 */
void
cpu_initclocks(void)
{
	if (clock_sc == NULL)
		panic("Clock timer was not configured.");
	if (stat_sc == NULL)
		panic("Statistics timer was not configured.");
	if (ref_sc == NULL)
		panic("Microtime reference timer was not configured.");

	/*
	 * We already have the timers running, but not generating interrupts.
	 * In addition, we've set stathz and profhz.
	 */
	printf("clock: hz=%d stathz=%d\n", hz, stathz);

	/*
	 * The "cookie" parameter must be zero to pass the interrupt frame
	 * through to hardclock() and statclock().
	 */
	intr_establish(clock_sc->sc_intr, IPL_CLOCK, IST_LEVEL_HIGH,
		clockintr, 0);

	if (clock_sc != stat_sc)
		intr_establish(stat_sc->sc_intr, IPL_HIGH, IST_LEVEL_HIGH,
			statintr, 0);

	timer_init(clock_sc, hz, FALSE, TRUE);
	if (clock_sc != stat_sc)
		timer_init(stat_sc, stathz, FALSE, TRUE);

	tc_init(&gemini_timecounter);
}

void
delay(u_int n)
{
	struct geminitmr_softc *sc = ref_sc;
	uint32_t cur, last, delta, usecs;

	if (sc == NULL)
		panic("The timer must be initialized sooner.");

	/*
	 * This works by polling the timer and counting the
	 * number of microseconds that go by.
	 */
	last = _timer_read(sc);

	delta = usecs = 0;

	while (n > usecs) {
		cur = _timer_read(sc);

		/* Check to see if the timer has wrapped around. */
		if (last < cur)
			delta += (last + (counts_per_hz - cur));
		else
			delta += (last - cur);

		last = cur;

		if (delta >= counts_per_usec) {
			usecs += delta / counts_per_usec;
			delta %= counts_per_usec;
		}
	}
}

static void
timer_factors(
	geminitmr_softc_t *sc,
	int ints_per_sec,
	boolean_t autoload)
{
	timer_factors_t *tfp = &sc->sc_tf;
	uint n = sc->sc_timerno;
	const uint32_t us_per_sec = 1000000;

	/*
	 * UPDOWN=0		(Down)
	 * OFENABLE=0		(no Irpt on overflow)
	 * CLOCK=0		(PCLK)
	 * ENABLE=1
	 */
	tfp->tf_tmcr = TIMER_TMCR_TMnENABLE(n);

	if (ints_per_sec == 0) {
		tfp->tf_counter = ~0U;
	} else {
		uint32_t count_freq;

		count_freq = GEMINI_TIMER_CLOCK_FREQ;
		count_freq /= ints_per_sec;
		tfp->tf_counter = count_freq;
	}
	tfp->tf_counts_per_usec = GEMINI_TIMER_CLOCK_FREQ / us_per_sec;

	if (autoload)
		tfp->tf_reload = tfp->tf_counter;	/* auto-reload */
	else
		tfp->tf_reload = 0;			/* no-auto_reload */

	tfp->tf_match1 = 0;
	tfp->tf_match2 = 0;

#ifdef GEMINI_TIMER_DEBUG
	tfprint(sc->sc_timerno, tfp);
	Debugger();
#endif
}

#ifdef GEMINI_TIMER_DEBUG
void
tfprint(uint n, timer_factors_t *tfp)
{
	printf("%s: timer# %d\n", __FUNCTION__, n);
	printf("\ttf_counts_per_usec: %#x\n", tfp->tf_counts_per_usec);
	printf("\ttf_tmcr: %#x\n", tfp->tf_tmcr);
	printf("\ttf_counter: %#x\n", tfp->tf_counter);
	printf("\ttf_reload: %#x\n", tfp->tf_reload);
	printf("\ttf_match1: %#x\n", tfp->tf_match1);
	printf("\ttf_match2: %#x\n", tfp->tf_match2);
}
#endif
