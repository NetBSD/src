/*	$NetBSD: becc_timer.c,v 1.11.36.1 2007/12/13 05:05:13 yamt Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Timer/clock support for the ADI Engineering Big Endian Companion Chip.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: becc_timer.c,v 1.11.36.1 2007/12/13 05:05:13 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>
#include <arm/cpufunc.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

void	(*becc_hardclock_hook)(void);

/*
 * Note, since COUNTS_PER_USEC doesn't divide evenly, we round up.
 */
#define	COUNTS_PER_SEC		BECC_PERIPH_CLOCK
#define	COUNTS_PER_USEC		((COUNTS_PER_SEC / 1000000) + 1)

static void *clock_ih;

/*
 * Since the timer interrupts when the counter underflows, we need to
 * subtract 1 from counts_per_hz when loading the preload register.
 */
static uint32_t counts_per_hz;

int	clockhandler(void *);

/*
 * becc_calibrate_delay:
 *
 *	Calibrate the delay loop.
 */
void
becc_calibrate_delay(void)
{

	/*
	 * Just use hz=100 for now -- we'll adjust it, if necessary,
	 * in cpu_initclocks().
	 */
	counts_per_hz = COUNTS_PER_SEC / 100;

	/* Stop both timers, clear interrupts. */
	BECC_CSR_WRITE(BECC_TSCRA, TSCRx_TIF);
	BECC_CSR_WRITE(BECC_TSCRB, TSCRx_TIF);

	/* Set the timer preload value. */
	BECC_CSR_WRITE(BECC_TPRA, counts_per_hz - 1);

	/* Start the timer. */
	BECC_CSR_WRITE(BECC_TSCRA, TSCRx_TE | TSCRx_CM);
}

/*
 * cpu_initclocks:
 *
 *	Initialize the clock and get them going.
 */
void
cpu_initclocks(void)
{
	u_int oldirqstate;

#if 0
	if (hz < 50 || COUNTS_PER_SEC % hz) {
		printf("Cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
	}
#endif
	tick = 1000000 / hz;	/* number of microseconds between interrupts */
	tickfix = 1000000 - (hz * tick);
	if (tickfix) {
		int ftp;

		ftp = min(ffs(tickfix), ffs(hz));
		tickfix >>= (ftp - 1);
		tickfixinterval = hz >> (ftp - 1);
	}

	/*
	 * We only have one timer available; stathz and profhz are
	 * always left as 0 (the upper-layer clock code deals with
	 * this situation).
	 */
	if (stathz != 0)
		printf("Cannot get %d Hz statclock\n", stathz);
	stathz = 0;

	if (profhz != 0)
		printf("Cannot get %d Hz profclock\n", profhz);
	profhz = 0;

	/* Report the clock frequency. */
	aprint_normal("clock: hz=%d stathz=%d profhz=%d\n", hz, stathz, profhz);

	oldirqstate = disable_interrupts(I32_bit);

	/* Hook up the clock interrupt handler. */
	clock_ih = becc_intr_establish(ICU_TIMERA, IPL_CLOCK,
	    clockhandler, NULL);
	if (clock_ih == NULL)
		panic("cpu_initclocks: unable to register timer interrupt");

	/* Set up the new clock parameters. */

	/* Stop timer, clear interrupt */
	BECC_CSR_WRITE(BECC_TSCRA, TSCRx_TIF);

	counts_per_hz = COUNTS_PER_SEC / hz;

	/* Set the timer preload value. */
	BECC_CSR_WRITE(BECC_TPRA, counts_per_hz - 1);

	/* ...and start it in motion. */
	BECC_CSR_WRITE(BECC_TSCRA, TSCRx_TE | TSCRx_CM);

	/* register soft interrupt handler as well */
	becc_intr_establish(ICU_SOFT, IPL_SOFTCLOCK, becc_softint, NULL);

	restore_interrupts(oldirqstate);
}

/*
 * setstatclockrate:
 *
 *	Set the rate of the statistics clock.
 *
 *	We assume that hz is either stathz or profhz, and that neither
 *	will change after being set by cpu_initclocks().  We could
 *	recalculate the intervals here, but that would be a pain.
 */
void
setstatclockrate(int new_hz)
{

	/*
	 * XXX Use TMR1?
	 */
}

/*
 * microtime:
 *
 *	Fill in the specified timeval struct with the current time
 *	accurate to the microsecond.
 */
void
microtime(struct timeval *tvp)
{
	static struct timeval lasttv;
	u_int oldirqstate;
	uint32_t counts;

	oldirqstate = disable_interrupts(I32_bit);

	/*
	 * XXX How do we compensate for the -1 behavior of the preload value?
	 */
	counts = counts_per_hz - BECC_CSR_READ(BECC_TCVRA);

	/* Fill in the timeval struct. */
	*tvp = time;
	tvp->tv_usec += (counts / COUNTS_PER_USEC);

	/* Make sure microseconds doesn't overflow. */
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	/* Make sure the time has advanced. */
	if (tvp->tv_sec == lasttv.tv_sec &&
	    tvp->tv_usec <= lasttv.tv_usec) {
		tvp->tv_usec = lasttv.tv_usec + 1;
		if (tvp->tv_usec >= 1000000) {
			tvp->tv_usec -= 1000000;
			tvp->tv_sec++;
		}
	}

	lasttv = *tvp;

	restore_interrupts(oldirqstate);
}

/*
 * delay:
 *
 *	Delay for at least N microseconds.
 */
void
delay(u_int n)
{
	uint32_t cur, last, delta, usecs;

	/*
	 * This works by polling the timer and counting the
	 * number of microseconds that go by.
	 */
	last = BECC_CSR_READ(BECC_TCVRA);
	delta = usecs = 0;

	while (n > usecs) {
		cur = BECC_CSR_READ(BECC_TCVRA);

		/* Check to see if the timer has wrapped around. */
		if (last < cur)
			delta += (last + (counts_per_hz - cur));
		else
			delta += (last - cur);

		last = cur;

		if (delta >= COUNTS_PER_USEC) {
			usecs += delta / COUNTS_PER_USEC;
			delta %= COUNTS_PER_USEC;
		}
	}
}

/*
 * clockhandler:
 *
 *	Handle the hardclock interrupt.
 */
int
clockhandler(void *arg)
{
	struct clockframe *frame = arg;

	/* ACK the interrupt. */
	BECC_CSR_WRITE(BECC_TSCRA, TSCRx_TE | TSCRx_CM | TSCRx_TIF);

	hardclock(frame);

	if (becc_hardclock_hook != NULL)
		(*becc_hardclock_hook)();

	return (1);
}
