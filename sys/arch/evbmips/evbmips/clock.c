/*	$NetBSD: clock.c,v 1.6.6.1 2006/04/22 11:37:25 simonb Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.6.6.1 2006/04/22 11:37:25 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sched.h>

#include <machine/bus.h>

#include <mips/locore.h>

#include <dev/clock_subr.h>
#include <evbmips/evbmips/clockvar.h>

todr_chip_handle_t todr_handle = NULL;

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{

	next_cp0_clk_intr = mips3_cp0_count_read() + curcpu()->ci_cycles_per_hz;
	mips3_cp0_compare_write(next_cp0_clk_intr);

	tick = 1000000 / hz;	/* number of microseconds between interrupts */
	tickfix = 1000000 - (hz * tick);
#ifdef NTP
	fixtick = tickfix;
#endif
	if (tickfix) {
		int ftp;

		ftp = min(ffs(tickfix), ffs(hz));
		tickfix >>= (ftp - 1);
		tickfixinterval = hz >> (ftp - 1);
        }
}

/*
 * Attach the clock device to todr_handle.
 */
void
todr_attach(todr_chip_handle_t todr)
{

        if (todr_handle) {
                printf("todr_attach: TOD already configured\n");
		return;
	}
        todr_handle = todr;
}

/*
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(time_t base)
{
	int badbase = 0, waszero = base == 0;

	if (base < 5 * SECYR) {
		/*
		 * If base is 0, assume filesystem time is just unknown
		 * in stead of preposterous. Don't bark.
		 */
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* not going to use it anyway, if the chip is readable */
		base = 21*SECYR + 186*SECDAY + SECDAY/2;
		badbase = 1;
	}

	if ((todr_handle == NULL) ||
	    (todr_gettime(todr_handle, &time) != 0) ||
	    (time.tv_sec == 0)) {

		if (todr_handle != NULL)
			printf("WARNING: preposterous TOD clock time");
		else
			printf("WARNING: no TOD clock present");

		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the clock.
		 */
		time.tv_sec = base;
		if (!badbase)
			resettodr();
	} else {
		int deltat = time.tv_sec - base;

		if (deltat < 0)
			deltat = -deltat;
		if (waszero || deltat < 2 * SECDAY)
			return;
		printf("WARNING: clock %s %d days",
		    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
	}
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 */
void
resettodr(void)
{
	if (time.tv_sec == 0)
		return;

	if (todr_handle)
		if (todr_settime(todr_handle, &time) != 0)
			printf("Cannot set TOD clock time\n");
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
 * Return the best possible estimate of the time in the timeval to
 * which tvp points.  We guarantee that the time will be greater than
 * the value obtained by a previous call.
 */
void
microtime(struct timeval *tvp)
{
	int s;
	static struct timeval lasttime;
	uint32_t count, res;

	s = splclock();
	*tvp = time;

	/* 32bit wrap-around during subtraction ok here. */
	count = mips3_cp0_count_read() - last_cp0_count;
	MIPS_COUNT_TO_MHZ(curcpu(), count, res);
	tvp->tv_usec += res;

	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
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
