/* $NetBSD: clock.c,v 1.18 2011/09/13 10:42:34 reinoud Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.18 2011/09/13 10:42:34 reinoud Exp $");

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

static void	clock_signal(int sig);
static unsigned int clock_getcounter(struct timecounter *);

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

static struct clock_softc *clock_sc;

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
	static struct sigaction sa;
	clock_softc_t *sc = device_private(self);
	long tcres;

	aprint_naive("\n");
	aprint_normal("\n");

	KASSERT(clock_sc == NULL);
	clock_sc = sc;

	sc->sc_dev = self;

	sc->sc_todr.todr_gettime = clock_todr_gettime;
	todr_attach(&sc->sc_todr);

	memset(&sa, 0, sizeof(sa));
	sigfillset(&sa.sa_mask);
	sa.sa_handler = clock_signal;
	sa.sa_flags = 0;
	if (thunk_sigaction(SIGALRM, &sa, NULL) == -1)
		panic("couldn't register SIGALRM handler : %d",
		    thunk_geterrno());

	tcres = thunk_clock_getres_monotonic();
	if (tcres > 0) {
		clock_timecounter.tc_quality = 1000;
		clock_timecounter.tc_frequency = 1000000000 / tcres;
	}
	tc_init(&clock_timecounter);
}

static void
clock_signal(int sig)
{
	struct clockframe cf;

	curcpu()->ci_idepth++;

	spl_intr(IPL_SOFTCLOCK, (void (*)(void *)) hardclock, &cf);
	// hardclock(&cf);

	curcpu()->ci_idepth--;
}

static unsigned int
clock_getcounter(struct timecounter *tc)
{
	return thunk_getcounter();
}

static int
clock_todr_gettime(struct todr_chip_handle *tch, struct timeval *tv)
{
	struct thunk_timeval ttv;
	int error;

	error = thunk_gettimeofday(&ttv, NULL);
	if (error)
		return error;

	tv->tv_sec = ttv.tv_sec;
	tv->tv_usec = ttv.tv_usec;

	return 0;
}
