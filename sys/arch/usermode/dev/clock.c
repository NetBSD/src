/* $NetBSD: clock.c,v 1.20.6.1 2012/02/18 07:33:23 mrg Exp $ */

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

#include "opt_hz.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.20.6.1 2012/02/18 07:33:23 mrg Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/malloc.h>
#include <sys/timetc.h>
#include <sys/time.h>

#include <machine/pcb.h>
#include <machine/mainbus.h>
#include <machine/thunk.h>

#include <dev/clock_subr.h>

static int	clock_match(device_t, cfdata_t, void *);
static void	clock_attach(device_t, device_t, void *);

static unsigned int clock_getcounter(struct timecounter *);

static int	clock_todr_gettime(struct todr_chip_handle *, struct timeval *);

extern void setup_clock_intr(void);
void clock_intr(void *priv);


struct clock_softc {
	device_t		sc_dev;
	struct todr_chip_handle	sc_todr;
};

static struct timecounter clock_timecounter = {
	clock_getcounter,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	1000000000ULL,		/* frequency */
	"CLOCK_MONOTONIC",	/* name */
	-100,			/* quality */
	NULL,			/* prev */
	NULL,			/* next */
};

timer_t clock_timerid;
int clock_running = 0;

CFATTACH_DECL_NEW(clock, sizeof(struct clock_softc),
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
	struct clock_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	sc->sc_todr.todr_gettime = clock_todr_gettime;
	todr_attach(&sc->sc_todr);

	clock_timerid = thunk_timer_attach();
	clock_timecounter.tc_quality = 1000;
	tc_init(&clock_timecounter);

	setup_clock_intr();
	clock_running = 1;
}

void
clock_intr(void *priv)
{
	struct clockframe cf;
	int nticks = thunk_timer_getoverrun(clock_timerid) + 1;

	while (nticks-- > 0) {
		hardclock(&cf);
	}
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
