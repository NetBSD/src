/*	$NetBSD: ixp425_timer.c,v 1.18.36.1 2018/07/28 04:37:29 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: ixp425_timer.c,v 1.18.36.1 2018/07/28 04:37:29 pgoyette Exp $");

#include "opt_ixp425.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/device.h>

#include <dev/clock_subr.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#include <arm/xscale/ixp425_sipvar.h>

static int	ixpclk_match(device_t, cfdata_t, void *);
static void	ixpclk_attach(device_t, device_t, void *);
static u_int	ixpclk_get_timecount(struct timecounter *);

static uint32_t counts_per_hz;

static void *clock_ih;

/* callback functions for intr_functions */
int	ixpclk_intr(void *);

struct ixpclk_softc {
	bus_addr_t		sc_baseaddr;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t      sc_ioh;
};

#ifndef IXP425_CLOCK_FREQ
#define	COUNTS_PER_SEC		66666600	/* 66MHz */
#else
#define	COUNTS_PER_SEC		IXP425_CLOCK_FREQ
#endif
#define	COUNTS_PER_USEC		((COUNTS_PER_SEC / 1000000) + 1)

static struct ixpclk_softc *ixpclk_sc;

static struct timecounter ixpclk_timecounter = {
	ixpclk_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	0xffffffff,		/* counter_mask */
	COUNTS_PER_SEC,		/* frequency */
	"ixpclk",		/* name */
	100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

static volatile uint32_t ixpclk_base;

CFATTACH_DECL_NEW(ixpclk, sizeof(struct ixpclk_softc),
		ixpclk_match, ixpclk_attach, NULL, NULL);

#define GET_TIMER_VALUE(sc)	(bus_space_read_4((sc)->sc_iot,		\
						  (sc)->sc_ioh,		\
						  IXP425_OST_TIM0))

#define GET_TS_VALUE(sc)	(*(volatile uint32_t *) \
				  (IXP425_TIMER_VBASE + IXP425_OST_TS))

static int
ixpclk_match(device_t parent, cfdata_t match, void *aux)
{
	return 2;
}

static void
ixpclk_attach(device_t parent, device_t self, void *aux)
{
	struct ixpclk_softc		*sc = device_private(self);
	struct ixpsip_attach_args	*sa = aux;

	printf("\n");

	ixpclk_sc = sc;

	sc->sc_iot = sa->sa_iot;
	sc->sc_baseaddr = sa->sa_addr;

	if (bus_space_map(sc->sc_iot, sa->sa_addr, sa->sa_size, 0,
			  &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	aprint_normal_dev(self, "IXP425 Interval Timer\n");
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	struct ixpclk_softc *sc = ixpclk_sc;
	u_int oldirqstate;

	if (hz < 50 || COUNTS_PER_SEC % hz) {
		aprint_error("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
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

	tc_init(&ixpclk_timecounter);
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
setstatclockrate(int newhz)
{

	/*
	 * XXX Use TMR1?
	 */
}

static u_int
ixpclk_get_timecount(struct timecounter *tc)
{
	u_int	savedints, base, counter;

	savedints = disable_interrupts(I32_bit);
	base = ixpclk_base;
	counter = GET_TIMER_VALUE(ixpclk_sc);
	restore_interrupts(savedints);

	return base - counter;
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(u_int n)
{
	uint32_t first, last;
	int usecs;

	if (n == 0)
		return;

	/*
	 * Clamp the timeout at a maximum value (about 32 seconds with
	 * a 66MHz clock). *Nobody* should be delay()ing for anywhere
	 * near that length of time and if they are, they should be hung
	 * out to dry.
	 */
	if (n >= (0x80000000U / COUNTS_PER_USEC))
		usecs = (0x80000000U / COUNTS_PER_USEC) - 1;
	else
		usecs = n * COUNTS_PER_USEC;

	/* Note: Timestamp timer counts *up*, unlike the other timers */
	first = GET_TS_VALUE();

	while (usecs > 0) {
		last = GET_TS_VALUE();
		usecs -= (int)(last - first);
		first = last;
	}
}

/*
 * ixpclk_intr:
 *
 *	Handle the hardclock interrupt.
 */
int
ixpclk_intr(void *arg)
{
	struct ixpclk_softc *sc = ixpclk_sc;
	struct clockframe *frame = arg;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IXP425_OST_STATUS,
			  OST_TIM0_INT);

	atomic_add_32(&ixpclk_base, counts_per_hz);

	hardclock(frame);

	return (1);
}
