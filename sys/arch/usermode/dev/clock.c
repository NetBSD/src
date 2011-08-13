/* $NetBSD: clock.c,v 1.8 2011/08/13 12:06:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.8 2011/08/13 12:06:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/time.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

#include <dev/clock_subr.h>

static int	clock_match(device_t, cfdata_t, void *);
static void	clock_attach(device_t, device_t, void *);

static void 	clock_intr(int);
static u_int	clock_getcounter(struct timecounter *);

static int	clock_todr_gettime(struct todr_chip_handle *, struct timeval *);

typedef struct clock_softc {
	device_t		sc_dev;
	struct todr_chip_handle	sc_todr;
} clock_softc_t;

static struct timecounter clock_timecounter = {
	clock_getcounter,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,			/* frequency */
	"CLOCK_MONOTONIC",	/* name */
	-100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

CFATTACH_DECL_NEW(clock, sizeof(clock_softc_t),
    clock_match, clock_attach, NULL, NULL);

static int
clock_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_CLOCK)
		return 0;

	return 1;
}

static void
clock_attach(device_t parent, device_t self, void *opaque)
{
	clock_softc_t *sc = device_private(self);
	struct itimerval itimer;
	struct timespec res;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	sc->sc_todr.todr_gettime = clock_todr_gettime;
	todr_attach(&sc->sc_todr);

	(void)signal(SIGALRM, clock_intr);

	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 10000;
	itimer.it_value = itimer.it_interval;
	thunk_setitimer(ITIMER_REAL, &itimer, NULL);

	if (thunk_clock_getres(CLOCK_MONOTONIC, &res) == 0 && res.tv_nsec > 0) {
		clock_timecounter.tc_quality = 1000;
		clock_timecounter.tc_frequency = 1000000000 / res.tv_nsec;
	}
	tc_init(&clock_timecounter);
}

static void
clock_intr(int notused)
{
	extern int usermode_x;
	struct clockframe cf;

	curcpu()->ci_idepth++;

	hardclock(&cf);

	curcpu()->ci_idepth--;
}

static u_int
clock_getcounter(struct timecounter *tc)
{
	struct timespec ts;

	thunk_clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_nsec;
}

static int
clock_todr_gettime(struct todr_chip_handle *tch, struct timeval *tv)
{
	return thunk_gettimeofday(tv,  NULL);
}
