/*	$NetBSD: omap2_mputmr.c,v 1.4.2.1 2013/01/23 00:05:42 yamt Exp $	*/

/*
 * OMAP 2430 GP timers
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
__KERNEL_RCSID(0, "$NetBSD: omap2_mputmr.c,v 1.4.2.1 2013/01/23 00:05:42 yamt Exp $");

#include "opt_omap.h"
#include "opt_cpuoptions.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/omap/omap_gptmrreg.h>
#include <arm/omap/omap2_mputmrvar.h>

#ifndef ARM11_PMC
uint32_t counts_per_usec, counts_per_hz;
#endif
struct mputmr_softc *clock_sc;
struct mputmr_softc *stat_sc;
struct mputmr_softc *ref_sc;
static uint32_t mpu_get_timecount(struct timecounter *);

static struct timecounter mpu_timecounter = {
	.tc_get_timecount = mpu_get_timecount,
	.tc_counter_mask = 0xffffffff,
	.tc_frequency = OMAP_MPU_TIMER_CLOCK_FREQ,
	.tc_name = "gpt",
	.tc_quality = 100,
	.tc_priv = NULL
};

static inline void
_timer_intr_dis(struct mputmr_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIER, 0);
}

static inline void
_timer_intr_enb(struct mputmr_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TIER, TIER_OVF_IT_ENA);
}

static inline uint32_t
_timer_intr_sts(struct mputmr_softc *sc)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, TISR);
}

static inline void
_timer_intr_ack(struct mputmr_softc *sc)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TISR, TIER_OVF_IT_ENA);
}

static inline uint32_t
_timer_read(struct mputmr_softc *sc)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, TCRR);
}

static inline void
_timer_stop(struct mputmr_softc *sc)
{
	uint32_t r;

	r = bus_space_read_4(sc->sc_iot, sc->sc_ioh, TCLR);
	r &= ~TCLR_ST;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TCLR, r);
}

static inline void
_timer_reload(struct mputmr_softc *sc, uint32_t val)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TLDR, val);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TCRR, val);
}

static inline void
_timer_start(struct mputmr_softc *sc, timer_factors *tfp)
{
	uint32_t r=0;

	if (tfp->ptv != 0) {
		r |= TCLR_PRE(1);
		r |= (TCLR_PTV(tfp->ptv - 1) & TCLR_PTV_MASK);
	}
	r |= (TCLR_CE | TCLR_AR | TCLR_ST);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TCLR, r);
}

static uint32_t
mpu_get_timecount(struct timecounter *tc)
{
	return _timer_read(ref_sc);
}

int
clockintr(void *frame)
{
	_timer_intr_ack(clock_sc);
	hardclock(frame);
	return 1;
}

int
statintr(void *frame)
{
	_timer_intr_ack(stat_sc);
	statclock(frame);
	return 1;
}

static void
setclockrate(struct mputmr_softc *sc, int schz)
{
	timer_factors tf;

	_timer_stop(sc);
	calc_timer_factors(schz, &tf);
	_timer_reload(sc, tf.reload);
	_timer_start(sc, &tf);
}

void
setstatclockrate(int schz)
{
	setclockrate(stat_sc, schz);
}

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

	_timer_intr_dis(clock_sc);
	_timer_intr_dis(stat_sc);
	_timer_intr_dis(ref_sc);

	setclockrate(clock_sc, hz);
	setclockrate(stat_sc, stathz);
	setclockrate(ref_sc, 0);


	/*
	 * The "cookie" parameter must be zero to pass the interrupt frame
	 * through to hardclock() and statclock().
	 */

	intr_establish(clock_sc->sc_intr, IPL_CLOCK, IST_LEVEL, clockintr, 0);
	intr_establish(stat_sc->sc_intr, IPL_HIGH, IST_LEVEL, statintr, 0);

	_timer_intr_enb(clock_sc);
	_timer_intr_enb(stat_sc);

	tc_init(&mpu_timecounter);
}

#if !(defined(ARM11_PMC) || defined(CORTEX_PMC))
void
delay(u_int n)
{
	uint32_t cur, last, delta, usecs;

	if (clock_sc == NULL)
		panic("The timer must be initialized sooner.");

	/*
	 * This works by polling the timer and counting the
	 * number of microseconds that go by.
	 */
	last = _timer_read(clock_sc);

	delta = usecs = 0;

	while (n > usecs) {
		cur = _timer_read(clock_sc);

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
#endif /* ARM11_PMC || CORTEX_PMC */

/*
 * OVF_Rate =
 *	(0xFFFFFFFF - GPTn.TLDR + 1) * (timer functional clock period)  * PS
 */
void
calc_timer_factors(int ints_per_sec, timer_factors *tf)
{
	uint32_t ptv_power;	/* PS */
	uint32_t count_freq;
	const uint32_t us_per_sec = 1000000;
	
	if (ints_per_sec == 0) {
		/*
		 * When ints_per_sec equal to zero there is mean full range
		 * timer usage. Nevertheless autoreload mode is still enabled.
		 */
		tf->ptv = 0;
		tf->reload = 0;
		tf->counts_per_usec = OMAP_MPU_TIMER_CLOCK_FREQ / us_per_sec;
		return;
	}


	tf->ptv = 8;
	for (;;) {
		ptv_power = 1 << tf->ptv;
		count_freq = OMAP_MPU_TIMER_CLOCK_FREQ;
		count_freq /= ints_per_sec;
		count_freq /= ptv_power;
		tf->reload = -count_freq;
		tf->counts_per_usec = count_freq / us_per_sec;
		if ((tf->reload * ptv_power * ints_per_sec
		     == OMAP_MPU_TIMER_CLOCK_FREQ)
		    && (tf->counts_per_usec * ptv_power * us_per_sec
			== OMAP_MPU_TIMER_CLOCK_FREQ))
		{	/* Exact match.  Life is good. */
			/* Currently reload is MPU_LOAD_TIMER+1.  Fix it. */
			tf->reload--;
			return;
		}
		if (tf->ptv == 0) {
			tf->counts_per_usec++;
			return;
		}
		tf->ptv--;
	}
}
