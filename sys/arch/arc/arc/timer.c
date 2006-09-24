/* $NetBSD: timer.c,v 1.9 2006/09/24 02:20:48 tsutsui Exp $ */
/* NetBSD: clock.c,v 1.31 2001/05/27 13:53:24 sommerfeld Exp  */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: timer.c,v 1.9 2006/09/24 02:20:48 tsutsui Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <mips/locore.h>
#include <mips/mips3_clock.h>

#include <arc/arc/timervar.h>

struct device *timerdev;
const struct timerfns *timerfns;
int timerinitted;
uint32_t last_cp0_count;

#ifdef ENABLE_INT5_STATCLOCK
/*
 * Statistics clock variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
static const uint32_t statvar = 1024;
static uint32_t statint;	/* number of clock ticks for stathz */
static uint32_t statmin;	/* minimum stat clock count in ticks */
static uint32_t statprev;/* last value of we set statclock to */
static u_int statcountperusec;	/* number of ticks per usec at current stathz */
#endif

void
timerattach(struct device *dev, const struct timerfns *fns)
{

	/*
	 * Just bookkeeping.
	 */

	if (timerfns != NULL)
		panic("timerattach: multiple timers");
	timerdev = dev;
	timerfns = fns;
}

/*
 * Machine-dependent clock routines.
 */

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{

#ifdef ENABLE_INT5_STATCLOCK
	if (stathz == 0)
		stathz = hz;

	if (profhz == 0)
		profhz = hz * 5;

	setstatclockrate(stathz);
#endif

	if (timerfns == NULL)
		panic("cpu_initclocks: no timer attached");

	/*
	 * Get the clock started.
	 */
	(*timerfns->tf_init)(timerdev);

	/* init timecounter */
	mips3_init_tc();

#ifdef ENABLE_INT5_STATCLOCK
	/* enable interrupts including CPU INT 5 */
	_splnone();
#endif
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int newhz)
{
#ifdef ENABLE_INT5_STATCLOCK
	uint32_t countpersecond, statvarticks;

	statprev = mips3_cp0_count_read();

	statint = ((curcpu()->ci_cpu_freq + newhz / 2) / newhz) / 2;

	/* Get the total ticks a second */
	countpersecond = statint * newhz;

	/* now work out how many ticks per usec */
	statcountperusec = countpersecond / 1000000;

	/* calculate a variance range of statvar */
	statvarticks = statcountperusec * statvar;

	/* minimum is statint - 50% of variant */
	statmin = statint - (statvarticks / 2);

	mips3_cp0_compare_write(statprev + statint);
#endif
}

#ifdef ENABLE_INT5_STATCLOCK
void
statclockintr(struct clockframe *cfp)
{
	uint32_t curcount, statnext, delta, r;
	int lost;

	lost = 0;

	do {
		r = (uint32_t)random() & (statvar - 1);
	} while (r == 0);
	statnext = statprev + statmin + (r * statcountperusec);

	mips3_cp0_compare_write(statnext);
	curcount = mips3_cp0_count_read();
	delta = statnext - curcount;

	while (__predict_false((int32_t)delta < 0)) {
		lost++;
		delta += statint;
	}
	if (__predict_false(lost > 0)) {
		statnext = curcount + delta;
		mips3_cp0_compare_write(statnext);
		for (; lost > 0; lost--)
			statclock(cfp);
	}
	statclock(cfp);

	statprev = statnext;
}
#endif
