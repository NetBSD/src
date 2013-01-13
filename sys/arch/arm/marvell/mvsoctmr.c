/*	$NetBSD: mvsoctmr.c,v 1.3.2.1 2013/01/13 19:03:05 bouyer Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: mvsoctmr.c,v 1.3.2.1 2013/01/13 19:03:05 bouyer Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/systm.h>
#include <sys/wdog.h>

#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/marvell/mvsocreg.h>
#include <arm/marvell/mvsocvar.h>
#include <arm/marvell/mvsoctmrreg.h>

#include <dev/marvell/marvellvar.h>

#include <dev/sysmon/sysmonvar.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif


struct mvsoctmr_softc {
	device_t sc_dev;

	struct sysmon_wdog sc_wdog;
	uint32_t sc_wdog_period;
	uint32_t sc_wdog_armed;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};


static int mvsoctmr_match(device_t, struct cfdata *, void *);
static void mvsoctmr_attach(device_t, device_t, void *);

static int clockhandler(void *);

static u_int mvsoctmr_get_timecount(struct timecounter *);

static void mvsoctmr_cntl(struct mvsoctmr_softc *, int, u_int, int, int);

static int mvsoctmr_wdog_tickle(struct sysmon_wdog *);
static int mvsoctmr_wdog_setmode(struct sysmon_wdog *);

#ifdef DDB
static void mvsoctmr_wdog_ddb_trap(int);
#endif

#define MVSOC_WDOG_MAX_PERIOD	(0xffffffff / mvTclk)

static struct mvsoctmr_softc *mvsoctmr_sc;
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
	uint32_t rstoutn;

	aprint_naive("\n");
	aprint_normal(": Marvell SoC Timer\n");

	if (mvsoctmr_sc == NULL)
		mvsoctmr_sc = sc;

	sc->sc_dev = self;
	sc->sc_iot = mva->mva_iot;
	if (bus_space_subregion(mva->mva_iot, mva->mva_ioh,
	    mva->mva_offset, mva->mva_size, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	mvsoctmr_timecounter.tc_name = device_xname(self);
	mvsoctmr_cntl(sc, MVSOCTMR_TIMER1, 0xffffffff, 1, 1);

	/*
	 * stop watchdog timer, enable watchdog timer resets
	 */
	mvsoctmr_cntl(sc, MVSOCTMR_WATCHDOG, 0xffffffff, 0, 0);
	rstoutn = read_mlmbreg(MVSOC_MLMB_RSTOUTNMASKR);
	write_mlmbreg(MVSOC_MLMB_RSTOUTNMASKR,
		      rstoutn | MVSOC_MLMB_RSTOUTNMASKR_WDRSTOUTEN);

#ifdef DDB
	db_trap_callback = mvsoctmr_wdog_ddb_trap;
#endif

	sc->sc_wdog.smw_name = device_xname(self);
	sc->sc_wdog.smw_cookie = sc;
	sc->sc_wdog.smw_setmode = mvsoctmr_wdog_setmode;
	sc->sc_wdog.smw_tickle = mvsoctmr_wdog_tickle;
	sc->sc_wdog.smw_period = MVSOC_WDOG_MAX_PERIOD;

	if (sysmon_wdog_register(&sc->sc_wdog) != 0)
		aprint_error_dev(self,
				 "unable to register watchdog with sysmon\n");
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

	hardclock(frame);

	return 1;
}

/*
 * setstatclockrate:
 *
 *	Set the rate of the statistics clock.
 */
/* ARGSUSED */
void
setstatclockrate(int newhz)
{
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	struct mvsoctmr_softc *sc;
	void *clock_ih;
	const int en = 1, autoen = 1;
	uint32_t timer0_tval;

	sc = mvsoctmr_sc;
	if (sc == NULL)
		panic("cpu_initclocks: mvsoctmr not found");

	mvsoctmr_timecounter.tc_priv = sc;
	mvsoctmr_timecounter.tc_frequency = mvTclk;

	timer0_tval = (mvTclk * 2) / (u_long) hz;
	timer0_tval = (timer0_tval / 2) + (timer0_tval & 1);

	mvsoctmr_cntl(sc, MVSOCTMR_TIMER0, timer0_tval, en, autoen);
	mvsoctmr_cntl(sc, MVSOCTMR_TIMER1, 0xffffffff, en, autoen);

	clock_ih = mvsoc_bridge_intr_establish(MVSOC_MLMB_MLMBI_CPUTIMER0INTREQ,
	    IPL_CLOCK, clockhandler, NULL);
	if (clock_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");

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
	    MVSOCTMR_TIMER(MVSOCTMR_TIMER1));

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
		    MVSOCTMR_TIMER(MVSOCTMR_TIMER1));
		if (cur_tick > initial_tick)
			remaining -= 0xffffffff - cur_tick + initial_tick;
		else
			remaining -= (initial_tick - cur_tick);
		initial_tick = cur_tick;
	}
}

static u_int
mvsoctmr_get_timecount(struct timecounter *tc)
{
	struct mvsoctmr_softc *sc = tc->tc_priv;

	return 0xffffffff - bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    MVSOCTMR_TIMER(MVSOCTMR_TIMER1));
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

static int
mvsoctmr_wdog_setmode(struct sysmon_wdog *smw)
{
	struct mvsoctmr_softc *sc = smw->smw_cookie;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		sc->sc_wdog_armed = 0;
		mvsoctmr_cntl(sc, MVSOCTMR_WATCHDOG, 0xffffffff, 0, 0);
	} else {
		sc->sc_wdog_armed = 1;
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = MVSOC_WDOG_MAX_PERIOD;
		else if (smw->smw_period > MVSOC_WDOG_MAX_PERIOD ||
			 smw->smw_period <= 0)
			return (EOPNOTSUPP);
		sc->sc_wdog_period = smw->smw_period * mvTclk;
		mvsoctmr_cntl(sc, MVSOCTMR_WATCHDOG, sc->sc_wdog_period, 1, 0);
	}

	return (0);
}

static int
mvsoctmr_wdog_tickle(struct sysmon_wdog *smw)
{
	struct mvsoctmr_softc *sc = smw->smw_cookie;

	mvsoctmr_cntl(sc, MVSOCTMR_WATCHDOG, sc->sc_wdog_period, 1, 0);

	return (0);
}

#ifdef DDB
static void
mvsoctmr_wdog_ddb_trap(int enter)
{
	struct mvsoctmr_softc *sc = mvsoctmr_sc;

	if (sc == NULL)
		return;

	if (sc->sc_wdog_armed) {
		if (enter)
			mvsoctmr_cntl(sc, MVSOCTMR_WATCHDOG, 0xffffffff, 0, 0);
		else
			mvsoctmr_cntl(sc, MVSOCTMR_WATCHDOG,
				      sc->sc_wdog_period, 1, 0);
	}
}
#endif
