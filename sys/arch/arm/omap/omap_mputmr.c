/*	$NetBSD: omap_mputmr.c,v 1.1.44.2 2008/01/20 17:51:05 bouyer Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: omap_mputmr.c,v 1.1.44.2 2008/01/20 17:51:05 bouyer Exp $");

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

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>

#include "opt_omap.h"

static int	omapmputmr_match(struct device *, struct cfdata *, void *);
static void	omapmputmr_attach(struct device *, struct device *, void *);

static int	clockintr(void *);
static int	statintr(void *);
void		rtcinit(void);

typedef struct timer_factors {
	uint32_t ptv;
	uint32_t reload;
	uint32_t counts_per_usec;
} timer_factors;
static void	calc_timer_factors(int, timer_factors*);

struct omapmputmr_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	int			sc_intr;
};

static uint32_t counts_per_usec, counts_per_hz;
static uint32_t hardref;
static struct omapmputmr_softc *clock_sc = NULL;
static struct omapmputmr_softc *stat_sc = NULL;
static struct omapmputmr_softc *ref_sc = NULL;

#ifndef STATHZ
#define STATHZ	64
#endif

#ifndef OMAP_MPU_TIMER_CLOCK_FREQ
#error Specify the timer frequency in Hz with the OMAP_MPU_TIMER_CLOCK_FREQ option.
#endif

/* Encapsulate the device knowledge within this source. */
/* Register offsets and values */
#define MPU_CNTL_TIMER	0x00
#define  MPU_FREE		(1<<6)
#define  MPU_CLOCK_ENABLE	(1<<5)
#define  MPU_PTV_SHIFT		2
#define  MPU_AR			(1<<1)
#define  MPU_ST			(1<<0)
#define MPU_LOAD_TIMER	0x04
#define MPU_READ_TIMER	0x08

CFATTACH_DECL(omapmputmr, sizeof(struct omapmputmr_softc),
    omapmputmr_match, omapmputmr_attach, NULL, NULL);

static int
omapmputmr_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1 || tipb->tipb_intr == -1)
	    panic("omapmputmr must have addr and intr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 256;	/* Per the OMAP TRM. */

	/* We implicitly trust the config file. */
	return (1);
}

void
omapmputmr_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapmputmr_softc *sc = (struct omapmputmr_softc*)self;
	struct tipb_attach_args *tipb = aux;
	int ints_per_sec;

	sc->sc_iot = tipb->tipb_iot;
	sc->sc_intr = tipb->tipb_intr;

	if (bus_space_map(tipb->tipb_iot, tipb->tipb_addr, tipb->tipb_size, 0,
			 &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	switch (self->dv_unit) {
	case 0:
		clock_sc = sc;
		ints_per_sec = hz;
		break;
	case 1:
		stat_sc = sc;
		ints_per_sec = profhz = stathz = STATHZ;
		break;
	case 2:
		ref_sc = sc;
		ints_per_sec = hz;	/* Same rate as clock */
		break;
	default:
		ints_per_sec = hz;	/* Better value? */
		break;
	}

	aprint_normal(": OMAP MPU Timer\n");
	aprint_naive("\n");

	/* Stop the timer from counting, but keep the timer module working. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MPU_CNTL_TIMER,
			  MPU_CLOCK_ENABLE);

	timer_factors tf;
	calc_timer_factors(ints_per_sec, &tf);

	switch (self->dv_unit) {
	case 0:
		counts_per_hz = tf.reload + 1;
		counts_per_usec = tf.counts_per_usec;
		break;
	case 2:

		/*
		 * The microtime reference clock for all practical purposes
		 * just wraps around as an unsigned int.
		 */

		tf.reload = 0xffffffff;
		break;

	default:
		break;
	}

	/* Set the reload value. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MPU_LOAD_TIMER, tf.reload);
	/* Set the PTV and the other required bits and pieces. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MPU_CNTL_TIMER,
			  ( MPU_CLOCK_ENABLE
			    | (tf.ptv << MPU_PTV_SHIFT)
			    | MPU_AR
			    | MPU_ST));
	/* The clock is now running, but is not generating interrupts. */
}

static int
clockintr(void *arg)
{
	struct clockframe *frame = arg;
	unsigned int newref;
	int ticks, i, oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	newref = bus_space_read_4(ref_sc->sc_iot, ref_sc->sc_ioh,
				  MPU_READ_TIMER);
	ticks = hardref ? (hardref - newref) / counts_per_hz : 1;
	hardref = newref;
	restore_interrupts(oldirqstate);

	if (ticks == 0)
		ticks = 1;

#ifdef DEBUG
	if (ticks > 1)
		printf("Missed %d ticks.\n", ticks-1);
#endif


	for (i = 0; i < ticks; i++)
		hardclock(frame);

	if (ticks > 1) {
		newref = bus_space_read_4(ref_sc->sc_iot, ref_sc->sc_ioh,
					  MPU_READ_TIMER);

		if ((hardref - newref) / counts_per_hz)
			hardclock(frame);
	}

	return(1);
}

static int
statintr(void *arg)
{
	struct clockframe *frame = arg;

	statclock(frame);

	return(1);
}


void
setstatclockrate(int schz)
{
	/* Stop the timer from counting, but keep the timer module working. */
	bus_space_write_4(stat_sc->sc_iot, stat_sc->sc_ioh, MPU_CNTL_TIMER,
			  MPU_CLOCK_ENABLE);

	timer_factors tf;
	calc_timer_factors(schz, &tf);

	/* Set the reload value. */
	bus_space_write_4(stat_sc->sc_iot, stat_sc->sc_ioh, MPU_LOAD_TIMER,
			  tf.reload);
	/* Set the PTV and the other required bits and pieces. */
	bus_space_write_4(stat_sc->sc_iot, stat_sc->sc_ioh, MPU_CNTL_TIMER,
			  ( MPU_CLOCK_ENABLE
			    | (tf.ptv << MPU_PTV_SHIFT)
			    | MPU_AR
			    | MPU_ST));
}

static u_int
mpu_get_timecount(struct timecounter *tc)
{
	uint32_t counter;
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	counter = bus_space_read_4(ref_sc->sc_iot, ref_sc->sc_ioh,
			       MPU_READ_TIMER);
	restore_interrupts(oldirqstate);

	return counter;
}

static struct timecounter mpu_timecounter = {
	mpu_get_timecount,
	NULL,
	0xffffffff,
	0,
	"mpu",
	100,
	NULL,
	NULL,
};

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

	omap_intr_establish(clock_sc->sc_intr, IPL_CLOCK,
			    clock_sc->sc_dev.dv_xname, clockintr, 0);
	omap_intr_establish(stat_sc->sc_intr, IPL_HIGH,
			    stat_sc->sc_dev.dv_xname, statintr, 0);

	tc_init(&mpu_timecounter);
}

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
	last = bus_space_read_4(clock_sc->sc_iot, clock_sc->sc_ioh,
				MPU_READ_TIMER);
	delta = usecs = 0;

	while (n > usecs) {
		cur = bus_space_read_4(clock_sc->sc_iot, clock_sc->sc_ioh,
					MPU_READ_TIMER);

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
calc_timer_factors(int ints_per_sec, timer_factors *tf)
{
	/*
	 * From the OMAP Technical Reference Manual:
	 *  T(MPU_Interrupt) = T(MPU_ref_clk) * (MPU_LOAD_TIMER+1) * 2**(PTV+1)
	 *
	 * T(MPU_ref_clk) is 1/OMAP_MPU_TIMER_CLOCK_FREQ and we want
	 * T(MPU_Interrupt) to be 1/ints_per_sec.  Rewriting the equation:
	 *
	 *                    1         (MPU_LOAD_TIMER+1) * 2**(PTV+1)
	 *               ------------ = -------------------------------
	 *               ints_per_sec      OMAP_MPU_TIMER_CLOCK_FREQ
	 *
	 * or
	 *
	 *    OMAP_MPU_TIMER_CLOCK_FREQ
	 *    ------------------------- = (MPU_LOAD_TIMER+1) * 2**(PTV+1)
	 *           ints_per_sec
	 *
	 * or
	 *
	 *    OMAP_MPU_TIMER_CLOCK_FREQ
	 *    ------------------------- = (MPU_LOAD_TIMER+1)
	 *    ints_per_sec * 2**(PTV+1)
	 *
	 *
	 * To save that last smidgen of power, find the largest prescaler that
	 * will give us a reload value that doesn't have any error.  However,
	 * to keep delay() accurate, it is desireable to have the number of
	 * counts per us be non-fractional.
	 *
	 * us_incs = OMAP_MPU_TIMER_CLOCK_FREQ / 2**(PTV+1) / 1,000,000
	 */

	/* The PTV can range from 7 to 0. */
	tf->ptv = 7;
	for (;;) {
		static const uint32_t us_per_sec = 1000000;
		uint32_t ptv_power = 1 << (tf->ptv + 1);
		uint32_t count_freq = OMAP_MPU_TIMER_CLOCK_FREQ / ptv_power;

		tf->reload = count_freq / ints_per_sec;
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
			/*
			 * Not exact, but we're out of options.  Leave the
			 * reload at being one too large and bump the counts
			 * per microsecond up one to make sure that we run a
			 * bit slow rather than too fast.
			 */
			tf->counts_per_usec++;
			return;
		}
		tf->ptv--;
	}
}
