/*	$NetBSD: ofwgencfg_clock.c,v 1.6.14.2 2000/12/13 14:50:14 bouyer Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/* Include header files */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>

#include <machine/irqhandler.h>
#include <machine/cpufunc.h>
#include <machine/cpu.h>
#include <machine/ofw.h>

static void *clockirq;


/*
 * int clockhandler(struct clockframe *frame)
 *
 * Function called by timer 0 interrupts. This just calls
 * hardclock(). Eventually the irqhandler can call hardclock() directly
 * but for now we use this function so that we can debug IRQ's
 */
 
int
clockhandler(frame)
	struct clockframe *frame;
{

	hardclock(frame);
	return(0);	/* Pass the interrupt on down the chain */
}


/*
 * int statclockhandler(struct clockframe *frame)
 *
 * Function called by timer 1 interrupts. This just calls
 * statclock(). Eventually the irqhandler can call statclock() directly
 * but for now we use this function so that we can debug IRQ's
 */
 
int
statclockhandler(frame)
	struct clockframe *frame;
{

	statclock(frame);
	return(0);	/* Pass the interrupt on down the chain */
}


/*
 * void setstatclockrate(int hz)
 *
 * Set the stat clock rate. The stat clock uses timer1
 */

void
setstatclockrate(hz)
	int hz;
{

#ifdef	OFWGENCFG
	printf("Not setting statclock: OFW generic has only one clock.\n");
#endif
}


/*
 * void cpu_initclocks(void)
 *
 * Initialise the clocks.
 * This sets up the two timers in the IOMD and installs the IRQ handlers
 *
 * NOTE: Currently only timer 0 is setup and the IRQ handler is not installed
 */
 
void
cpu_initclocks()
{
	/*
	 * Load timer 0 with count down value
	 * This timer generates 100Hz interrupts for the system clock
	 */

	printf("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

        clockirq = intr_claim(IRQ_TIMER0, IPL_CLOCK, "tmr0 hard clk",
            clockhandler, 0);
        if (clockirq == NULL)
                panic("Cannot installer timer 0 IRQ handler\n");

	/* Notify callback handler that it can start processing ticks. */
	ofw_handleticks = 1;

	if (stathz) {
	    printf("Not installing statclock: OFW generic has only one clock.\n");
	}
}


/*
 * void microtime(struct timeval *tvp)
 *
 * Fill in the specified timeval struct with the current time
 * accurate to the microsecond.
 */

void
microtime(tvp)
	struct timeval *tvp;
{
	int s;
	static struct timeval oldtv;

	s = splhigh();

	/* Fill in the timeval struct */

	*tvp = time;    

	/* Make sure the micro seconds don't overflow. */

	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}

	/* Make sure the time has advanced. */

	if (tvp->tv_sec == oldtv.tv_sec &&
	    tvp->tv_usec <= oldtv.tv_usec) {
		tvp->tv_usec = oldtv.tv_usec + 1;
		if (tvp->tv_usec >= 1000000) {
			tvp->tv_usec -= 1000000;
			++tvp->tv_sec;
		}
	}
	    

	oldtv = *tvp;
	(void)splx(s);		
}

/*
 * Estimated loop for n microseconds
 */

/* Need to re-write this to use the timers */

/* One day soon I will actually do this */

int delaycount = 50;

void
delay(n)
	u_int n;
{
	u_int i;

	if (n == 0) return;
	while (--n > 0) {
		if (cputype == ID_SA110)	/* XXX - Seriously gross hack */
			for (i = delaycount; --i;);
		else
			for (i = 8; --i;);
	}
}
