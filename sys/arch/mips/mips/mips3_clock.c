/*	$NetBSD: mips3_clock.c,v 1.2 2006/09/07 03:14:22 simonb Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: mips3_clock.c,v 1.2 2006/09/07 03:14:22 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <mips/mips3_clock.h>
#include <machine/intr.h>
#include <machine/locore.h>

#ifdef	__HAVE_TIMECOUNTER
static void init_mips3_tc(void);
#endif

struct evcnt mips_int5_evcnt =
    EVCNT_INITIALIZER(EVCNT_TYPE_INTR, NULL, "mips", "int 5 (clock)");

uint32_t next_cp0_clk_intr;	/* used to schedule hard clock interrupts */

/*
 * Handling to be done upon receipt of a MIPS 3 clock interrupt.  This
 * routine is to be called from the master interrupt routine
 * (e.g. cpu_intr), if MIPS INT5 is pending.  The caller is
 * responsible for blocking and renabling the interrupt in the
 * cpu_intr() routine.
 */
void
mips3_clockintr(uint32_t status, uint32_t pc)
{
	uint32_t		new_cnt;
	struct clockframe	cf;

	next_cp0_clk_intr += curcpu()->ci_cycles_per_hz;
	mips3_cp0_compare_write(next_cp0_clk_intr);

	/* Check for lost clock interrupts */
	new_cnt = mips3_cp0_count_read();

	/* 
	 * Missed one or more clock interrupts, so let's start 
	 * counting again from the current value.
	 */
	if ((next_cp0_clk_intr - new_cnt) & 0x80000000) {
#if 0	/* XXX - should add an event counter for this */
		missed_clk_intrs++;
#endif

		next_cp0_clk_intr = new_cnt + curcpu()->ci_cycles_per_hz;
		mips3_cp0_compare_write(next_cp0_clk_intr);
	}

	cf.pc = pc;
	cf.sr = status;
	hardclock(&cf);

	mips_int5_evcnt.ev_count++;

	/* caller should renable clock interrupts */
}

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{
	evcnt_attach_static(&mips_int5_evcnt);

	next_cp0_clk_intr = mips3_cp0_count_read() + curcpu()->ci_cycles_per_hz;
	mips3_cp0_compare_write(next_cp0_clk_intr);

#ifdef	__HAVE_TIMECOUNTER
	init_mips3_tc();
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

	/* nothing we can do */
}

/*
 * Wait for at least "n" microseconds.
 */
void
delay(int n)
{
	uint32_t cur, last, delta, usecs;

	last = mips3_cp0_count_read();
	delta = usecs = 0;

	while (n > usecs) {
		cur = mips3_cp0_count_read();

		/* Check to see if the timer has wrapped around. */
		if (cur < last)
			delta += ((curcpu()->ci_cycles_per_hz - last) + cur);
		else
			delta += (cur - last);

		last = cur;

		if (delta >= curcpu()->ci_divisor_delay) {
			usecs += delta / curcpu()->ci_divisor_delay;
			delta %= curcpu()->ci_divisor_delay;
		}
	}
}

/*
 * Support for using the MIPS 3 clock as a timecounter.
 */

#ifdef	__HAVE_TIMECOUNTER
void
init_mips3_tc(void)
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
	if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT) {
		tc.tc_frequency /= 2;
	}

	tc_init(&tc);
#endif
}
#endif	/* __HAVE_TIMECOUNTER */

