/* $NetBSD: microtime.c,v 1.3 2001/05/27 13:53:24 sommerfeld Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: microtime.c,v 1.3 2001/05/27 13:53:24 sommerfeld Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <machine/cpu.h>
#include <machine/rpb.h>

struct timeval microset_time;

/*
 * Return the best possible estimate of the time in the timeval to which
 * tvp points.  The kernel logical time variable is interpolated between
 * ticks by reading the PCC to determine the number of cycles since the
 * last processor update, then converting the result to microseconds.  In
 * the case of multiprocessor systems, the interpolation is specific to
 * each processor, since each processor has its own PCC.
 */
void
microtime(struct timeval *tvp)
{
	static struct timeval lasttime;
	static struct simplelock microtime_slock = SIMPLELOCK_INITIALIZER;
	struct timeval t, st;
	struct cpu_info *ci = curcpu();
	long sec, usec;
	int s;

	s = splclock();		/* also blocks IPIs */

	/* XXXSMP: not atomic */
	st = time;		/* read system time */

	if (ci->ci_pcc_denom != 0) {
		/*
		 * Determine the current clock time as the time at last
		 * microset() call, plus the PCC accumulation since then.
		 * This time should lie in the interval between the current
		 * master clock time and the time at the nexdt tick, but
		 * this code does not explicitly require that in the interest
		 * of speed.  If something ugly occurs, like a settimeofday()
		 * call, the processors may disagree among themselves for not
		 * more than the interval between microset() calls.  In any
		 * case, the following sanity checks will suppress timewarps.
		 */
		t = ci->ci_pcc_time;
		usec = (alpha_rpcc() & 0xffffffffUL) - ci->ci_pcc_pcc;
		if (usec < 0)
			usec += 0x100000000L;
		t.tv_usec += (usec * ci->ci_pcc_ms_delta) / ci->ci_pcc_denom;
		while (t.tv_usec >= 1000000) {
			t.tv_usec -= 1000000;
			t.tv_sec++;
		}
	} else {
		/*
		 * Can't use the PCC -- just use the system time.
		 */
		t = st;
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
	simple_lock(&microtime_slock);
	sec = lasttime.tv_sec - t.tv_sec;
	usec = lasttime.tv_usec - t.tv_usec;
	if (usec < 0) {
		usec += 1000000;
		sec--;
	}
	if (sec == 0) {
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
 * It determines the PCC frequency of each processor relative to the
 * master clock and the time this determination is made.  These values
 * are used by microtime() to interpolate the microseconds between
 * timer interrupts.  Note that we assume the kernel variables have
 * been zeroed early in life.
 */
void
microset(struct cpu_info *ci, struct trapframe *framep)
{
	struct timeval t;
	long delta, denom;

	/* Note: Clock interrupts are already blocked. */

	/* XXX BLOCK MACHINE CHECKS? */

	denom = ci->ci_pcc_pcc;
	t = microset_time;
	ci->ci_pcc_pcc = alpha_rpcc() & 0xffffffffUL;

	/* XXX UNBLOCK MACHINE CHECKS? */

	if (ci->ci_pcc_denom == 0) {
		/*
		 * This is our first time here on this CPU.  Just
		 * start with reasonable initial values.
		 */
		ci->ci_pcc_time = t;
		ci->ci_pcc_ms_delta = 1000000;
		ci->ci_pcc_denom = hwrpb->rpb_cc_freq;
		return;
	}

	denom = ci->ci_pcc_pcc - denom;
	if (denom < 0)
		denom += 0x100000000L;

	delta = (t.tv_sec - ci->ci_pcc_time.tv_sec) * 1000000 +
	    (t.tv_usec - ci->ci_pcc_time.tv_usec);

	ci->ci_pcc_time = t;
	/*
	 * Make sure it's within .5 to 1.5 seconds -- otherwise,
	 * the time is probably be frobbed with by the timekeeper
	 * or the human.
	 */
	if (delta > 500000 && delta < 1500000) {
		ci->ci_pcc_ms_delta = delta;
		ci->ci_pcc_denom = denom;
	} else {
#if 0
		printf("microset: delta %ld, resetting state\n", delta);
#endif
		ci->ci_pcc_ms_delta = 1000000; 
		ci->ci_pcc_denom = hwrpb->rpb_cc_freq; 
	}
}
