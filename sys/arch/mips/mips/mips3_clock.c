/*	$NetBSD: mips3_clock.c,v 1.13.30.1 2017/08/28 17:51:45 skrll Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include "opt_multiprocessor.h"

__KERNEL_RCSID(0, "$NetBSD: mips3_clock.c,v 1.13.30.1 2017/08/28 17:51:45 skrll Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/timetc.h>

#include <mips/mips3_clock.h>

#include <mips/locore.h>

/*
 * Wait for at least "n" microseconds.
 */
void
mips3_delay(int n)
{
	u_long divisor_delay;
	uint32_t cur, last, delta, usecs;

	last = mips3_cp0_count_read();
	delta = usecs = 0;

	divisor_delay = curcpu()->ci_divisor_delay;
	if (divisor_delay == 0) {
		/*
		 * Frequency values in curcpu() are not initialized.
		 * Assume faster frequency since longer delays are harmless.
		 * Note CPU_MIPS_DOUBLE_COUNT is ignored here.
		 */
#define FAST_FREQ	(300 * 1000 * 1000)	/* fast enough? */
		divisor_delay = FAST_FREQ / (1000 * 1000);
	}

	while (n > usecs) {
		cur = mips3_cp0_count_read();

		/*
		 * The MIPS3 CP0 counter always counts upto UINT32_MAX,
		 * so no need to check wrapped around case.
		 */
		delta += (cur - last);

		last = cur;

		while (delta >= divisor_delay) {
			/*
			 * delta is not so larger than divisor_delay here,
			 * and using DIV/DIVU ops could be much slower.
			 * (though longer delay may be harmless)
			 */
			usecs++;
			delta -= divisor_delay;
		}
	}
}

/*
 * Support for using the MIPS 3 clock as a timecounter.
 */

void
mips3_init_tc(void)
{
#if !defined(MULTIPROCESSOR)
	static struct timecounter tc =  {
		(timecounter_get_t *)mips3_cp0_count_read, /* get_timecount */
		0,				/* no poll_pps */
		~0u,				/* counter_mask */
		0,				/* frequency */
		"mips3_cp0_counter",		/* name */
		100,				/* quality */
	};

	tc.tc_frequency = curcpu()->ci_cpu_freq;
	if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT) {
		tc.tc_frequency /= 2;
	}
	curcpu()->ci_cctr_freq = tc.tc_frequency;

	tc_init(&tc);
#endif
}

__weak_alias(delay, mips3_delay);
