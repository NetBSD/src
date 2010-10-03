/*	$NetBSD: mvsoctmr.c,v 1.1 2010/10/03 05:49:24 kiyohara Exp $	*/
/*
 * Copyright (c) 2007, 2008 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: mvsoctmr.c,v 1.1 2010/10/03 05:49:24 kiyohara Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/systm.h>

#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mvsoctmrreg.h>

#include <dev/marvell/marvellvar.h>


struct mvsoctmr_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};


static int mvsoctmr_match(device_t, struct cfdata *, void *);
static void mvsoctmr_attach(device_t, device_t, void *);

static int clockhandler(void *);
static int statclockhandler(void *);

static u_int mvsoctmr_get_timecount(struct timecounter *);

static void mvsoctmr_cntl(struct mvsoctmr_softc *, int, u_int, int, int);

#ifndef STATHZ
#define STATHZ	64
#endif

static struct mvsoctmr_softc *mvsoctmr_sc;
static uint32_t clock_ticks, statclock_ticks;
static struct timecounter mvsoctmr_timecounter = {
	mvsoctmr_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,			/* frequency  (set by cpu_initclocks()) */
	"mvsoctmr",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};
static volatile uint32_t mvsoctmr_base;

CFATTACH_DECL_NEW(mvsoctmr, sizeof(struct mvsoctmr_softc),
    mvsoctmr_match, mvsoctmr_attach, NULL, NULL);


/* ARGSUSED */
static int
mvsoctmr_match(device_t parent, struct cfdata *match, void *aux)
{
	struct marvell_attach_args *mva = aux;

	if (strcmp(mva->mva_name, match->cf_name) != 0)
		return 0;
	if (mva->mva_offset == MVA_OFFSET_DEFAULT)
		return 0;

	mva->mva_size = MVSOCTMR_SIZE;
	return 1;
}

/* ARGSUSED */
static void
mvsoctmr_attach(device_t parent, device_t self, void *aux)
{
        struct mvsoctmr_softc *sc = device_private(self);
	struct marvell_attach_args *mva = aux;

	aprint_naive("\n");
	aprint_normal(": Marvell SoC Timer\n");

	if (mvsoctmr_sc == NULL)
		mvsoctmr_sc = sc;

	sc->sc_dev = self;
	sc->sc_iot = mva->mva_iot;
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
	    mva->mva_offset, mva->mva_size, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));
}

/*
 * clockhandler:
 *
 *	Handle the hardclock interrupt.
 */
static int
clockhandler(void *arg)
{
	struct clockframe *frame = arg;

	atomic_add_32(&mvsoctmr_base, clock_ticks);

	hardclock(frame);

	return 1;
}

/*
 * statclockhandler:
 *
 *	Handle the statclock interrupt.
 */
static int
statclockhandler(void *arg)
{
	struct clockframe *frame = arg;

	statclock(frame);

	return 1;
}


/*
 * setstatclockrate:
 *
 *	Set the rate of the statistics clock.
 *
 *	We assume that hz is either stathz or profhz, and that neither
 *	will change after being set by cpu_initclocks().  We could
 *	recalculate the intervals here, but that would be a pain.
 */
/* ARGSUSED */
void
setstatclockrate(int newhz)
{
	struct mvsoctmr_softc *sc = mvsoctmr_sc;
	const int en = 1, autoen = 1;

	statclock_ticks = mvTclk / newhz;

	mvsoctmr_cntl(sc, MVSOCTMR_TIMER1, statclock_ticks, en, autoen);
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks()
{
	struct mvsoctmr_softc *sc;
	void *clock_ih;
	const int en = 1, autoen = 1;

	sc = mvsoctmr_sc;
	if (sc == NULL)
		panic("cpu_initclocks: mvsoctmr not found");

	stathz = profhz = STATHZ;
	mvsoctmr_timecounter.tc_frequency = mvTclk;
	clock_ticks = mvTclk / hz;

	mvsoctmr_cntl(sc, MVSOCTMR_TIMER0, clock_ticks, en, autoen);

	clock_ih = mvsoc_bridge_intr_establish(MVSOC_MLMB_MLMBI_CPUTIMER0INTREQ,
	    IPL_CLOCK, clockhandler, NULL);
	if (clock_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");

	if (stathz) {
		setstatclockrate(stathz);
		clock_ih = mvsoc_bridge_intr_establish(
		    MVSOC_MLMB_MLMBI_CPUTIMER1INTREQ, IPL_HIGH,
		    statclockhandler, NULL);
		if (clock_ih == NULL)
			panic("cpu_initclocks:"
			    " unable to register statclock timer interrupt");
	}

	tc_init(&mvsoctmr_timecounter);
}

void
delay(unsigned int n)
{
	struct mvsoctmr_softc *sc;
	unsigned int cur_tick, initial_tick;
	int remaining;

	sc = mvsoctmr_sc;
#ifdef DEBUG
	if (sc == NULL) {
		printf("%s: called before start mvsoctmr\n", __func__);
		return;
	}
#endif

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	initial_tick = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    MVSOCTMR_TIMER(MVSOCTMR_TIMER0));

	if (n <= UINT_MAX / mvTclk) {
		/*
		 * For unsigned arithmetic, division can be replaced with
		 * multiplication with the inverse and a shift.
		 */
		remaining = n * mvTclk / 1000000;
	} else {
		/*
		 * This is a very long delay.
		 * Being slow here doesn't matter.
		 */
		remaining = (unsigned long long) n * mvTclk / 1000000;
	}

	while (remaining > 0) {
		cur_tick = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    MVSOCTMR_TIMER(MVSOCTMR_TIMER0));
		if (cur_tick > initial_tick)
			remaining -= clock_ticks - cur_tick + initial_tick;
		else
			remaining -= (initial_tick - cur_tick);
		initial_tick = cur_tick;
	}
}

static u_int
mvsoctmr_get_timecount(struct timecounter *tc)
{
	struct mvsoctmr_softc *sc = mvsoctmr_sc;
	uint32_t counter, base;
	u_int intrstat;

	intrstat = disable_interrupts(I32_bit);
	base = mvsoctmr_base;
	counter = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    MVSOCTMR_TIMER(MVSOCTMR_TIMER0));
	restore_interrupts(intrstat);

	return base - counter;
}


static void
mvsoctmr_cntl(struct mvsoctmr_softc *sc, int num, u_int ticks, int en,
	      int autoen)
{
	uint32_t ctrl;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSOCTMR_RELOAD(num),
	    ticks);

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSOCTMR_TIMER(num), ticks);

	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSOCTMR_CTCR);
	if (en)
		ctrl |= MVSOCTMR_CTCR_CPUTIMEREN(num);
	else
		ctrl &= ~MVSOCTMR_CTCR_CPUTIMEREN(num);
	if (autoen)
		ctrl |= MVSOCTMR_CTCR_CPUTIMERAUTO(num);
	else
		ctrl &= ~MVSOCTMR_CTCR_CPUTIMERAUTO(num);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSOCTMR_CTCR, ctrl);
}
