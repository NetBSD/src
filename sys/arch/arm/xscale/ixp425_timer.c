/*	$NetBSD: ixp425_timer.c,v 1.2 2003/07/26 05:51:11 thorpej Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425_timer.c,v 1.2 2003/07/26 05:51:11 thorpej Exp $");

#include "opt_perfctrs.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>

static int	ixpclk_match(struct device *, struct cfdata *, void *);
static void	ixpclk_attach(struct device *, struct device *, void *);

static uint32_t counts_per_hz;

static void *clock_ih;

/* callback functions for intr_functions */
int	ixpclk_intr(void *);

struct ixpclk_softc {
        struct device           sc_dev;
        bus_addr_t              sc_baseaddr;
        bus_space_tag_t         sc_iot;
        bus_space_handle_t      sc_ioh;
};

#define	COUNTS_PER_SEC		66000000	/* 66MHz */
#define	COUNTS_PER_USEC		(COUNTS_PER_SEC / 1000000)

static struct ixpclk_softc *ixpclk_sc = NULL;

CFATTACH_DECL(ixpclk, sizeof(struct ixpclk_softc),
		ixpclk_match, ixpclk_attach, NULL, NULL);

#define GET_TIMER_VALUE(sc)	(bus_space_read_4((sc)->sc_iot,		\
						  (sc)->sc_ioh,		\
						  IXP425_OST_TIM0))

static int
ixpclk_match(struct device *parent, struct cfdata *match, void *aux)
{
        return 2;
}

static void
ixpclk_attach(struct device *parent, struct device *self, void *aux)
{
	struct ixpclk_softc		*sc = (struct ixpclk_softc*) self;
	struct ixpsip_attach_args	*sa = aux;

	printf("\n");

	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;

	/* using first timer for system ticks */
	if (ixpclk_sc == NULL)
		ixpclk_sc = sc;
	if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	 aprint_normal("%s: IXP425 Interval Timer\n", sc->sc_dev.dv_xname);
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	struct ixpclk_softc* sc = ixpclk_sc;
	u_int oldirqstate;
#if defined(PERFCTRS)
	void *pmu_ih;
#endif

	if (hz < 50 || COUNTS_PER_SEC % hz) {
		aprint_error("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
	}
	tick = 1000000 / hz;	/* number of microseconds between interrupts */
	tickfix = 1000000 - (hz * tick);
	if (tickfix) {
		int ftp;

		ftp = min(ffs(tickfix), ffs(hz));
		tickfix >>= (ftp - 1);
		tickfixinterval = hz >> (ftp - 1);
	}

	/*
	 * We only have one timer available; stathz and profhz are
	 * always left as 0 (the upper-layer clock code deals with
	 * this situation).
	 */
	if (stathz != 0)
		aprint_error("Cannot get %d Hz statclock\n", stathz);
	stathz = 0;

	if (profhz != 0)
		aprint_error("Cannot get %d Hz profclock\n", profhz);
	profhz = 0;

	/* Report the clock frequency. */
	aprint_normal("clock: hz=%d stathz=%d profhz=%d\n", hz, stathz, profhz);

	oldirqstate = disable_interrupts(I32_bit);

	/* Hook up the clock interrupt handler. */
	clock_ih = ixp425_intr_establish(IXP425_INT_TMR0, IPL_CLOCK,
					 ixpclk_intr, NULL);
	if (clock_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");

#if defined(PERFCTRS)
	pmu_ih = ixp425_intr_establish(IXP425_INT_XPMU, IPL_STATCLOCK,
					xscale_pmc_dispatch, NULL);
	if (pmu_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");
#endif

	/* Set up the new clock parameters. */

	/* clear interrupt */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXP425_OST_STATUS,
			  OST_WARM_RESET | OST_WDOG_INT | OST_TS_INT |
			  OST_TIM1_INT | OST_TIM0_INT);

	counts_per_hz = COUNTS_PER_SEC / hz;

	/* reload value & Timer enable */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXP425_OST_TIM0_RELOAD,
			  (counts_per_hz & TIMERRELOAD_MASK) | OST_TIMER_EN);

	restore_interrupts(oldirqstate);
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
void
setstatclockrate(int hz)
{

	/*
	 * XXX Use TMR1?
	 */
}

/*
 * microtime:
 *
 *	Fill in the specified timeval struct with the current time
 *	accurate to the microsecond.
 */
void
microtime(struct timeval *tvp)
{
	struct ixpclk_softc* sc = ixpclk_sc;
	static struct timeval lasttv;
	u_int oldirqstate;
	uint32_t counts;

	oldirqstate = disable_interrupts(I32_bit);

	counts = counts_per_hz - GET_TIMER_VALUE(sc);

	/* Fill in the timeval struct. */
	*tvp = time;
	tvp->tv_usec += (counts / COUNTS_PER_USEC);

	/* Make sure microseconds doesn't overflow. */
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	/* Make sure the time has advanced. */
	if (tvp->tv_sec == lasttv.tv_sec &&
	    tvp->tv_usec <= lasttv.tv_usec) {
		tvp->tv_usec = lasttv.tv_usec + 1;
		if (tvp->tv_usec >= 1000000) {
			tvp->tv_usec -= 1000000;
			tvp->tv_sec++;
		}
	}

	lasttv = *tvp;

	restore_interrupts(oldirqstate);
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(u_int n)
{
	struct ixpclk_softc* sc = ixpclk_sc;
	uint32_t cur, last, delta, usecs;

	/*
	 * This works by polling the timer and counting the
	 * number of microseconds that go by.
	 */
	last = GET_TIMER_VALUE(sc);
	delta = usecs = 0;

	while (n > usecs) {
		cur = GET_TIMER_VALUE(sc);

		/* Check to see if the timer has wrapped around. */
		if (last < cur)
			delta += (last + (counts_per_hz - cur));
		else
			delta += (last - cur);

		last = cur;

		if (delta >= COUNTS_PER_USEC) {
			usecs += delta / COUNTS_PER_USEC;
			delta %= COUNTS_PER_USEC;
		}
	}
}

todr_chip_handle_t todr_handle;

/*
 * todr_attach:
 *
 *	Set the specified time-of-day register as the system real-time clock.
 */
void
todr_attach(todr_chip_handle_t todr)
{

	if (todr_handle)
		panic("todr_attach: rtc already configured");
}

/*
 * inittodr:
 *
 *	Initialize time from the time-of-day register.
 */
#define	MINYEAR		2003	/* minimum plausible year */
void
inittodr(time_t base)
{
	time_t deltat;
	int badbase;

	if (base < (MINYEAR - 1970) * SECYR) {
		printf("WARNING: preposterous time in file system");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR;
		badbase = 1;
	} else
		badbase = 0;

	if (todr_handle == NULL ||
	    todr_gettime(todr_handle, (struct timeval *)&time) != 0 ||
	    time.tv_sec == 0) {
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		time.tv_sec = base;
		time.tv_usec = 0;
		if (todr_handle != NULL && !badbase) {
			printf("WARNING: preposterous clock chip time\n");
			resettodr();
		}
		goto bad;
	}

	if (!badbase) {
		/*
		 * See if we tained/lost two or more days; if
		 * so, assume something is amiss.
		 */
		deltat = time.tv_sec - base;
		if (deltat < 0)
			deltat = -deltat;
		if (deltat < 2 * SECDAY)
			return;		/* all is well */
		printf("WARNING: clock %s %ld days\n",
		    time.tv_sec < base ? "lost" : "gained",
		    (long)deltat / SECDAY);
	}
 bad:
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * resettodr:
 *
 *	Reset the time-of-day register with the current time.
 */
void
resettodr(void)
{

	if (time.tv_sec == 0)
		return;

	if (todr_handle != NULL &&
	    todr_settime(todr_handle, (struct timeval *)&time) != 0)
		printf("resettodr: failed to set time\n");
}

/*
 * ixpclk_intr:
 *
 *	Handle the hardclock interrupt.
 */
int
ixpclk_intr(void *arg)
{
	struct ixpclk_softc* sc = ixpclk_sc;
	struct clockframe *frame = arg;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXP425_OST_STATUS,
			  OST_TIM0_INT);

	hardclock(frame);

	return (1);
}
