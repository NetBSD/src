/*	$NetBSD: timer.c,v 1.21.6.2 2006/05/28 08:09:37 kardel Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Paul Kranenburg.
 *	This product includes software developed by Harvard University.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Kernel clocks provided by "timer" device.  The hardclock is provided by
 * the timer register (aka system counter).  The statclock is provided by
 * per CPU counter register(s) (aka processor counter(s)).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: timer.c,v 1.21.6.2 2006/05/28 08:09:37 kardel Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <sparc/sparc/timerreg.h>
#include <sparc/sparc/timervar.h>

static struct intrhand level10;
static struct intrhand level14;

static u_int timer_get_timecount(struct timecounter *);

/*
 * timecounter local state
 */
static struct counter {
	volatile u_int *cntreg;	/* counter register */
	u_int limit;		/* limit we count up to */
	u_int offset;		/* accumulated offet due to wraps */
	u_int shift;		/* scaling for valid bits */
	u_int mask;		/* valid bit mask */
} cntr;

/*
 * define timecounter
 */

static struct timecounter counter_timecounter = {
	timer_get_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,                      /* frequency - set at initialisation */
	"timer-counter",	/* name */
	0,			/* quality */
	&cntr			/* private reference */
};

/*
 * timer_get_timecount provide current counter value
 */
static u_int
timer_get_timecount(struct timecounter *tc)
{
	struct counter *ctr = (struct counter *)tc->tc_priv;

	u_int c, res, r;
	int s;


	s = splhigh();

	res = c = *ctr->cntreg;

	res  &= ~TMR_LIMIT;

	if (c != res) {
		r = ctr->limit;
	} else {
		r = 0;
	}
	
	res >>= ctr->shift;
	res  &= ctr->mask;

	res += r + ctr->offset;

	splx(s);

	return res;
}

void
tickle_tc()
{
	if (timecounter->tc_get_timecount == timer_get_timecount) {
		cntr.offset += cntr.limit;
	}
}

/*
 * sun4/sun4c/sun4m common timer attach code
 */
void
timerattach(volatile int *cntreg, volatile int *limreg)
{
	u_int prec = 0, t0;

	/*
	 * Calibrate delay() by tweaking the magic constant
	 * until a delay(100) actually reads (at least) 100 us on the clock.
	 * Note: sun4m clocks tick with 500ns periods.
	 */
	for (timerblurb = 1; ; timerblurb++) {
		volatile int discard;
		u_int t1;

		/* Reset counter register by writing some large limit value */
		discard = *limreg;
		*limreg = tmr_ustolim(TMR_MASK-1);

		t0 = *cntreg;
		delay(100);
		t1 = *cntreg;
		
		prec |= (t0 ^ t1) | (*cntreg ^ *cntreg);

		if (t1 & TMR_LIMIT)
			panic("delay calibration");

		t0 = (t0 >> TMR_SHIFT) & TMR_MASK;
		t1 = (t1 >> TMR_SHIFT) & TMR_MASK;

		if (t1 >= t0 + 100)
			break;
	}

	/* find lowest active bit */
	for (t0 = 0; t0 < TMR_SHIFT; t0++)
		if ((1 << t0) & prec)
			break;

	cntr.shift = t0;
	cntr.mask = (1 << (31-t0))-1;
	counter_timecounter.tc_frequency = 1000000 * (TMR_SHIFT - t0 + 1);
	
	printf(": delay constant %d, frequency = %" PRIu64 " Hz\n", timerblurb, counter_timecounter.tc_frequency);

#if defined(SUN4) || defined(SUN4C)
	if (CPU_ISSUN4 || CPU_ISSUN4C) {
		timer_init = timer_init_4;
		level10.ih_fun = clockintr_4;
		level14.ih_fun = statintr_4;
		cntr.limit = tmr_ustolim(tick);
	}
#endif
#if defined(SUN4M)
	if (CPU_ISSUN4M) {
		timer_init = timer_init_4m;
		level10.ih_fun = clockintr_4m;
		level14.ih_fun = statintr_4m;
		cntr.limit = tmr_ustolim4m(tick);
	}
#endif
	/* link interrupt handlers */
	intr_establish(10, 0, &level10, NULL);
	intr_establish(14, 0, &level14, NULL);

	/* Establish a soft interrupt at a lower level for schedclock */
	sched_cookie = softintr_establish(IPL_SCHED, schedintr, NULL);
	if (sched_cookie == NULL)
		panic("timerattach: cannot establish schedintr");

	cntr.cntreg = cntreg;
	cntr.limit >>= cntr.shift;

	tc_init(&counter_timecounter);
}

/*
 * Both sun4 and sun4m can attach a timer on obio.
 * The sun4m OPENPROM calls the timer the "counter".
 * The sun4 timer must be probed.
 */
static int
timermatch_obio(struct device *parent, struct cfdata *cf, void *aux)
{
#if defined(SUN4) || defined(SUN4M)
	union obio_attach_args *uoba = aux;
#endif
#if defined(SUN4)
	struct obio4_attach_args *oba;
#endif

#if defined(SUN4M)
	if (uoba->uoba_isobio4 == 0)
		return (strcmp("counter", uoba->uoba_sbus.sa_name) == 0);
#endif /* SUN4M */

	if (CPU_ISSUN4 == 0) {
		printf("timermatch_obio: attach args mixed up\n");
		return (0);
	}

#if defined(SUN4)
	/* Only these sun4s have "timer" (others have "oclock") */
	if (cpuinfo.cpu_type != CPUTYP_4_300 &&
	    cpuinfo.cpu_type != CPUTYP_4_400)
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
#endif /* SUN4 */
	panic("timermatch_obio: impossible");
}

static void
timerattach_obio(struct device *parent, struct device *self, void *aux)
{
	union obio_attach_args *uoba = aux;

	if (uoba->uoba_isobio4 == 0) {
#if defined(SUN4M)
		/* sun4m timer at obio */
		timerattach_obio_4m(parent, self, aux);
#endif /* SUN4M */
		return;
	}

	if (uoba->uoba_isobio4 != 0) {
#if defined(SUN4)
		/* sun4 timer at obio */
		timerattach_obio_4(parent, self, aux);
#endif /* SUN4 */
	}
}

CFATTACH_DECL(timer_obio, sizeof(struct device),
    timermatch_obio, timerattach_obio, NULL, NULL);

/*
 * Only sun4c attaches a timer at mainbus
 */
static int
timermatch_mainbus(struct device *parent, struct cfdata *cf, void *aux)
{
#if defined(SUN4C)
	struct mainbus_attach_args *ma = aux;

	return (strcmp("counter-timer", ma->ma_name) == 0);
#else
	return (0);
#endif
}

static void
timerattach_mainbus(struct device *parent, struct device *self, void *aux)
{

#if defined(SUN4C)
	timerattach_mainbus_4c(parent, self, aux);
#endif /* SUN4C */
}

CFATTACH_DECL(timer_mainbus, sizeof(struct device),
    timermatch_mainbus, timerattach_mainbus, NULL, NULL);
