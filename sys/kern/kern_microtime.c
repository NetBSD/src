/*	$NetBSD: kern_microtime.c,v 1.15.14.1 2006/06/19 04:07:15 chap Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 */

/******************************************************************************
 *                                                                            *
 * Copyright (c) David L. Mills 1993, 1994                                    *
 *                                                                            *
 * Permission to use, copy, modify, and distribute this software and its      *
 * documentation for any purpose and without fee is hereby granted, provided  *
 * that the above copyright notice appears in all copies and that both the    *
 * copyright notice and this permission notice appear in supporting           *
 * documentation, and that the name University of Delaware not be used in     *
 * advertising or publicity pertaining to distribution of the software        *
 * without specific, written prior permission.  The University of Delaware    *
 * makes no representations about the suitability this software for any       *
 * purpose.  It is provided "as is" without express or implied warranty.      *
 *                                                                            *
 ******************************************************************************/

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: kern_microtime.c,v 1.15.14.1 2006/06/19 04:07:15 chap Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>

#include <machine/cpu.h>
#include <machine/cpu_counter.h>

/* XXX compat definitions */
#define	ci_cc_time	ci_cc.cc_time
#define	ci_cc_cc	ci_cc.cc_cc
#define	ci_cc_ms_delta	ci_cc.cc_ms_delta
#define	ci_cc_denom	ci_cc.cc_denom

struct timeval cc_microset_time;

/*
 * Return the best possible estimate of the time in the timeval to which
 * tvp points.  The kernel logical time variable is interpolated between
 * ticks by reading the CC to determine the number of cycles since the
 * last processor update, then converting the result to microseconds.  In
 * the case of multiprocessor systems, the interpolation is specific to
 * each processor, since each processor has its own CC.
 */
void
cc_microtime(struct timeval *tvp)
{
	static struct timeval lasttime;
	static struct simplelock microtime_slock = SIMPLELOCK_INITIALIZER;
	struct timeval t;
	struct cpu_info *ci = curcpu();
	int64_t cc, sec, usec;
	int s;

#ifdef MULTIPROCESSOR
	s = splipi();		/* also blocks IPIs */
#else
	s = splclock();		/* block clock interrupts */
#endif

	if (ci->ci_cc_denom != 0) {
		/*
		 * Determine the current clock time as the time at last
		 * microset() call, plus the CC accumulation since then.
		 * This time should lie in the interval between the current
		 * master clock time and the time at the next tick, but
		 * this code does not explicitly require that in the interest
		 * of speed.  If something ugly occurs, like a settimeofday()
		 * call, the processors may disagree among themselves for not
		 * more than the interval between microset() calls.  In any
		 * case, the following sanity checks will suppress timewarps.
		 */
		simple_lock(&microtime_slock);
		t = ci->ci_cc_time;
		cc = cpu_counter32() - ci->ci_cc_cc;
		if (cc < 0)
			cc += 0x100000000LL;
		t.tv_usec += (cc * ci->ci_cc_ms_delta) / ci->ci_cc_denom;
		while (t.tv_usec >= 1000000) {
			t.tv_usec -= 1000000;
			t.tv_sec++;
		}
	} else {
		/*
		 * Can't use the CC -- just use the system time.
		 */
		/* XXXSMP: not atomic */
		simple_lock(&microtime_slock);
		getmicrotime(&t);
	}

	/*
	 * Ordinarily, the current clock time is guaranteed to be later
	 * by at least one microsecond than the last time the clock was
	 * read.  However, this rule applies only if the current time is
	 * within one second of the last time.  Otherwise, the clock will
	 * (shudder) be set backward.  The clock adjustment daemon or
	 * human equivalent is presumed to be correctly implemented and
	 * to set the clock backward only upon unavoidable crisis.
	 */
	sec = lasttime.tv_sec - t.tv_sec;
	usec = lasttime.tv_usec - t.tv_usec;
	if (usec < 0) {
		usec += 1000000;
		sec--;
	}
	if (sec == 0 && usec > 0)  {
		t.tv_usec += usec + 1;
		if (t.tv_usec >= 1000000) {
			t.tv_usec -= 1000000;
			t.tv_sec++;
		}
	}
	lasttime = t;
	simple_unlock(&microtime_slock);

	splx(s);

	*tvp = t;
}

/*
 * This routine is called about once per second directly by the master
 * processor and via an interprocessor interrupt for other processors.
 * It determines the CC frequency of each processor relative to the
 * master clock and the time this determination is made.  These values
 * are used by microtime() to interpolate the microseconds between
 * timer interrupts.  Note that we assume the kernel variables have
 * been zeroed early in life.
 */
void
cc_microset(struct cpu_info *ci)
{
	struct timeval t;
	int64_t delta, denom;

	/* Note: Clock interrupts are already blocked. */

	denom = ci->ci_cc_cc;
	t = cc_microset_time;		/* XXXSMP: not atomic */
	ci->ci_cc_cc = cpu_counter32();

	if (ci->ci_cc_denom == 0) {
		/*
		 * This is our first time here on this CPU.  Just
		 * start with reasonable initial values.
		 */
		ci->ci_cc_time = t;
		ci->ci_cc_ms_delta = 1000000;
		ci->ci_cc_denom = cpu_frequency(ci);
		return;
	}

	denom = ci->ci_cc_cc - denom;
	if (denom < 0)
		denom += 0x100000000LL;

	delta = (t.tv_sec - ci->ci_cc_time.tv_sec) * 1000000 +
	    (t.tv_usec - ci->ci_cc_time.tv_usec);

	ci->ci_cc_time = t;
	/*
	 * Make sure it's within .5 to 1.5 seconds -- otherwise,
	 * the time is probably be frobbed with by the timekeeper
	 * or the human.
	 */
	if (delta > 500000 && delta < 1500000) {
		ci->ci_cc_ms_delta = delta;
		ci->ci_cc_denom = denom;
#if 0
		printf("cc_microset[%lu]: delta %" PRId64
		    ", denom %" PRId64 "\n", ci->ci_cpuid, delta, denom);
#endif
	} else {
#if 0
		printf("cc_microset[%lu]: delta %" PRId64 ", resetting state\n",
		       (u_long)ci->ci_cpuid, delta);
#endif
		ci->ci_cc_ms_delta = 1000000;
		ci->ci_cc_denom = cpu_frequency(ci);
	}
}
