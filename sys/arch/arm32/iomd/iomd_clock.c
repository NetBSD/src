/* $NetBSD: iomd_clock.c,v 1.13 1997/07/31 01:08:01 mark Exp $ */

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * clock.c
 *
 * Timer related machine specific code
 *
 * Created      : 29/09/94
 */

/* Include header files */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <dev/clock_subr.h>

#include <machine/katelib.h>
#include <machine/iomd.h>
#include <machine/irqhandler.h>
#include <machine/cpu.h>
#include <machine/rtc.h>

#include "rtc.h"

#if NRTC == 0
#error "Need at least one RTC device for timeofday management"
#endif

#define TIMER0_COUNT 20000		/* 100Hz */
#define TIMER_FREQUENCY 2000000		/* 2MHz clock */
#define TICKS_PER_MICROSECOND (TIMER_FREQUENCY / 1000000)

static irqhandler_t clockirq;
static irqhandler_t statclockirq;


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
#ifdef RC7500
	extern void setleds();
	static int leds = 0;

	setleds(1 << leds);
	leds++;
	if (leds >> 3)
		leds = 0;
#endif	/* RC7500 */

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
	int count;
    
	count = TIMER_FREQUENCY / hz;
    
	printf("Setting statclock to %dHz (%d ticks)\n", hz, count);
    
	WriteByte(IOMD_T1LOW,  (count >> 0) & 0xff);
	WriteByte(IOMD_T1HIGH, (count >> 8) & 0xff);

	/* reload the counter */

	WriteByte(IOMD_T1GO, 0);
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
	int count;

	/*
	 * Load timer 0 with count down value
	 * This timer generates 100Hz interrupts for the system clock
	 */

	printf("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

	count = TIMER_FREQUENCY / hz;
	WriteByte(IOMD_T0LOW,  (count >> 0) & 0xff);
	WriteByte(IOMD_T0HIGH, (count >> 8) & 0xff);

	/* reload the counter */

	WriteByte(IOMD_T0GO, 0);

	clockirq.ih_func = clockhandler;
	clockirq.ih_arg = 0;
	clockirq.ih_level = IPL_CLOCK;
	clockirq.ih_name = "TMR0 hard clk";
	if (irq_claim(IRQ_TIMER0, &clockirq) == -1)
		panic("Cannot installer timer 0 IRQ handler\n");

	if (stathz) {
		setstatclockrate(stathz);
        
		statclockirq.ih_func = statclockhandler;
		statclockirq.ih_arg = 0;
		statclockirq.ih_level = IPL_CLOCK;
		if (irq_claim(IRQ_TIMER1, &clockirq) == -1)
			panic("Cannot installer timer 1 IRQ handler\n");
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
	int tm;
	int deltatm;
	static int oldtm;
	static struct timeval oldtv;

	s = splhigh();

	/*
	 * Latch the current value of the timer and then read it.
	 * This garentees an atmoic reading of the time.
	 */
 
	WriteByte(IOMD_T0LATCH, 0);
	tm = ReadByte(IOMD_T0LOW) + (ReadByte(IOMD_T0HIGH) << 8);
	deltatm = tm - oldtm;
	if (deltatm < 0) deltatm += TIMER0_COUNT;
	if (deltatm < 0) {
		printf("opps deltatm < 0 tm=%d oldtm=%d deltatm=%d\n",
		    tm, oldtm, deltatm);
	}
	oldtm = tm;

	/* Fill in the timeval struct */

	*tvp = time;    
#ifdef HIGHLY_DUBIOUS
	tvp->tv_usec += (deltatm / TICKS_PER_MICROSECOND);
#else
	tvp->tv_usec += (tm / TICKS_PER_MICROSECOND);
#endif

	/* Make sure the micro seconds don't overflow. */

	while (tvp->tv_usec > 1000000) {
		tvp->tv_usec -= 1000000;
		++tvp->tv_sec;
	}

	/* Make sure the time has advanced. */

	if (tvp->tv_sec == oldtv.tv_sec &&
	    tvp->tv_usec <= oldtv.tv_usec) {
		tvp->tv_usec = oldtv.tv_usec + 1;
		if (tvp->tv_usec > 1000000) {
			tvp->tv_usec -= 1000000;
			++tvp->tv_sec;
		}
	}
	    

	oldtv = *tvp;
	(void)splx(s);		
}


void
need_proftick(p)
	struct proc *p;
{
}

/*
 * Machine-dependent clock routines.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

static int timeset = 0;

/*
 * Write back the time of day to the rtc
 */

void
resettodr()
{
	struct clock_ymdhms dt;
	int s;
	rtc_t rtc;

	if (!timeset)
		return;

	/* Convert from secs to ymdhms fields */
	clock_secs_to_ymdhms(time.tv_sec - (rtc_offset * 60), &dt);

	/* Fill out an RTC structure */
	rtc.rtc_cen = dt.dt_year / 100;
	rtc.rtc_year = dt.dt_year % 100;
	rtc.rtc_mon = dt.dt_mon;
	rtc.rtc_day = dt.dt_day;
	rtc.rtc_hour = dt.dt_hour;
	rtc.rtc_min = dt.dt_min;
	rtc.rtc_sec = dt.dt_sec;
	rtc.rtc_centi = 0;
	rtc.rtc_micro = 0;

/*	printf("resettod: %d/%d/%d%d %d:%d:%d\n", rtc.rtc_day,
	    rtc.rtc_mon, rtc.rtc_cen, rtc.rtc_year, rtc.rtc_hour,
	    rtc.rtc_min, rtc.rtc_sec);*/

	/* Pass the time to the todclock device */
	s = splclock();
	rtc_write(&rtc);
	(void)splx(s);
}

/*
 * Initialise the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */

void
inittodr(base)
	time_t base;
{
	struct clock_ymdhms dt;
	time_t diff;
	int s;
	int days;
	rtc_t rtc;

	/*
	 * Get the time from the todclock device
	 */

	s = splclock();
	if (rtc_read(&rtc) == 0) {
		(void)splx(s);
		return;
	}

	(void)splx(s);

	/* Convert to clock_ymdhms structure */
	dt.dt_sec = rtc.rtc_sec;
	dt.dt_min = rtc.rtc_min;
	dt.dt_hour = rtc.rtc_hour;
	dt.dt_day = rtc.rtc_day;
	dt.dt_mon = rtc.rtc_mon;
	dt.dt_year = rtc.rtc_year + (rtc.rtc_cen * 100);

	/* Convert to seconds */
	time.tv_sec = clock_ymdhms_to_secs(&dt) + (rtc_offset * 60);
	time.tv_usec = 0;

	/* timeset is used to ensure the time is valid before a resettodr() */

	timeset = 1;

	/* If the base was 0 then no time so keep quiet */

	if (base) {
		printf("inittodr: %02d:%02d:%02d %02d/%02d/%04d\n",
		    dt.dt_hour, dt.dt_min, dt.dt_sec, dt.dt_day,
		    dt.dt_mon, dt.dt_year);

		diff = time.tv_sec - base;
		if (diff < 0)
			diff = - diff;

		if (diff > 60) {
			days = diff / 86400;
			printf("Clock has %s %d day%c %ld hours %ld minutes %ld secs\n",
			    ((time.tv_sec - base) > 0) ? "gained" : "lost", 
			    days, ((days == 1) ? 0 : 's'), (diff  / 3600) % 24,
			    (diff / 60) % 60, diff % 60);
		}
	}
}  

/* End of clock.c */
