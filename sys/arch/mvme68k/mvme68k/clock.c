/*      $NetBSD: clock.c,v 1.18 2003/07/15 02:43:49 lukem Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratory.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      @(#)clock.c     8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.18 2003/07/15 02:43:49 lukem Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/bus.h>

#include <dev/mvme/clockvar.h>

static	struct clock_attach_args *clock_args;
static	todr_chip_handle_t todr_handle;

struct	evcnt clock_profcnt;
struct	evcnt clock_statcnt;

/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would 
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
/* XXX fix comment to match value */
int	clock_statvar = 8192;
int	clock_statmin;		/* statclock interval - (1/2 * variance) */


/*
 * Common parts of clock autoconfiguration.
 */
void
clock_config(dev, ca, ev)
	struct device *dev;
	struct clock_attach_args *ca;
	struct evcnt *ev;
{
	extern int delay_divisor;	/* from machdep.c */

	/* Hook up that which we need. */
	clock_args = ca;

	evcnt_attach_dynamic(&clock_profcnt, EVCNT_TYPE_INTR, ev,
	    dev->dv_xname, "profint");
	evcnt_attach_dynamic(&clock_statcnt, EVCNT_TYPE_INTR, ev,
	    dev->dv_xname, "statint");

	/* Print info about the clock. */
	printf(": delay_divisor %d\n", delay_divisor);
}

void
clock_rtc_config(todr)
	todr_chip_handle_t todr;
{

	if (todr_handle)
		panic("clock_config: clock already configured");

	todr_handle = todr;
}

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only
 * if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks()
{
	int statint, minint;

	if (clock_args == NULL)
		panic("clock not configured");

	if (1000000 % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}
	if (stathz == 0)
		stathz = hz;
	if (1000000 % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}
	profhz = stathz;	/* always */ 

	statint = 1000000 / stathz;
	minint = statint / 2 + 100;
	while (clock_statvar > minint)
		clock_statvar >>= 1;

	clock_statmin = statint - (clock_statvar >> 1);

	/* Call the machine-specific initclocks hook. */
	(*clock_args->ca_initfunc)(clock_args->ca_arg, tick, statint);
}

void
setstatclockrate(newhz)
	int newhz;
{

	/* XXX should we do something here? XXX */
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time, in uSec, since the last clock interrupt
 * (clock_args->ca_microtime()) was handled.
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */

void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += (*clock_args->ca_microtime)(clock_args->ca_arg);
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
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
 * Set up the system's time, given a `reasonable' time value.
 */
void
inittodr(base)
        time_t base;
{
        int badbase = 0, waszero = (base == 0);

	if (todr_handle == NULL)
		panic("todr not configured");

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

        if (todr_gettime(todr_handle, (struct timeval *)&time) != 0 ||
            time.tv_sec == 0) {
                printf("WARNING: bad date in battery clock");
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
 * Reset the clock based on the current time.
 * Used when the current clock is preposterous, when the time is changed,
 * and when rebooting.  Do nothing if the time is not yet known, e.g.,
 * when crashing during autoconfig.
 */
void
resettodr()
{

        if (!time.tv_sec)
                return;

        if (todr_settime(todr_handle, (struct timeval *)&time) != 0)
                printf("resettodr: failed to set time\n");
}
