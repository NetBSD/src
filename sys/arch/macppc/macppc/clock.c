/*	$NetBSD: clock.c,v 1.16 2001/05/27 23:38:12 wiz Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>
#include <machine/cpu.h>

#include "adb.h"

/*
 * Initially we assume a processor with a bus frequency of 50 MHz.
 */
static u_long ticks_per_sec = 50*1000*1000/4;
static u_long ns_per_tick = 80;
long ticks_per_intr;
static int clockinitted;

#ifdef MULTIPROCESSOR
#define lasttb (curcpu()->ci_lasttb)
#define tickspending (curcpu()->ci_tickspending)
#else
static volatile u_long lasttb;
volatile int tickspending;
#endif

#ifdef TIMEBASE_FREQ
u_int timebase_freq = TIMEBASE_FREQ;
#else
u_int timebase_freq = 0;
#endif

#define SECDAY 86400
#define DIFF19041970 2082844800

#if NADB > 0
extern int adb_read_date_time __P((int *));
extern int adb_set_date_time __P((int));
#endif

void
inittodr(base)
	time_t base;
{
	time_t deltat;
	u_int rtc_time;

	/*
	 * If we can't read from RTC, use the fs time.
	 */
#if NADB > 0
	if (adb_read_date_time(&rtc_time) < 0)
#endif
	{
		time.tv_sec = base;
		return;
	}
	clockinitted = 1;
	time.tv_sec = rtc_time - DIFF19041970 + rtc_offset * 60;

	deltat = time.tv_sec - base;
	if (deltat < 0)
		deltat = -deltat;
	if (deltat < 2 * SECDAY)
		return;

	printf("WARNING: clock %s %d days",
	    time.tv_sec < base ? "lost" : "gained", (int)(deltat / SECDAY));

	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Similar to the above
 */
void
resettodr()
{
#if NADB > 0
	u_int rtc_time;

	if (clockinitted) {
		rtc_time = time.tv_sec + DIFF19041970 - rtc_offset * 60;
		adb_set_date_time(rtc_time);
	}
#endif
}

void
decr_intr(frame)
	struct clockframe *frame;
{
	u_long tb;
	long tick;
	int nticks;
	int pri, msr;

	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

	/*
	 * Based on the actual time delay since the last decrementer reload,
	 * we arrange for earlier interrupt next time.
	 */
	asm ("mfdec %0" : "=r"(tick));
	for (nticks = 0; tick < 0; nticks++)
		tick += ticks_per_intr;
	asm volatile ("mtdec %0" :: "r"(tick));

	uvmexp.intrs++;
	intrcnt[CNT_CLOCK]++;

	pri = splclock();
	if (pri & (1 << SPL_CLOCK))
		tickspending += nticks;
	else {
		nticks += tickspending;
		tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
		asm ("mftb %0" : "=r"(tb));
		lasttb = tb + tick - ticks_per_intr;

		/*
		 * Reenable interrupts
		 */
		asm volatile ("mfmsr %0; ori %0, %0, %1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));
		
		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		frame->pri = pri | (1 << SIR_CLOCK);
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);
	}
	splx(pri);
}

void
cpu_initclocks()
{
}

void
calc_delayconst()
{
	int qhandle, phandle;
	char type[32];
	int msr, scratch;
	
	/*
	 * Get this info during autoconf?				XXX
	 */
	if (timebase_freq != 0) {
		ticks_per_sec = timebase_freq;
		goto found;
	}

	for (qhandle = OF_peer(0); qhandle; qhandle = phandle) {
		if (OF_getprop(qhandle, "device_type", type, sizeof type) > 0
		    && strcmp(type, "cpu") == 0
		    && OF_getprop(qhandle, "timebase-frequency",
			   &ticks_per_sec, sizeof ticks_per_sec) > 0) {
			goto found;
		}
		if ((phandle = OF_child(qhandle)))
			continue;
		while (qhandle) {
			if ((phandle = OF_peer(qhandle)))
				break;
			qhandle = OF_parent(qhandle);
		}
	}
	panic("no cpu node");

found:
	/*
	 * Should check for correct CPU here?		XXX
	 */
	asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	ns_per_tick = 1000000000 / ticks_per_sec;
	ticks_per_intr = ticks_per_sec / hz;
	asm volatile ("mftb %0" : "=r"(lasttb));
	asm volatile ("mtdec %0" :: "r"(ticks_per_intr));
	asm volatile ("mtmsr %0" :: "r"(msr));
}

static inline u_quad_t
mftb()
{
	u_long scratch;
	u_quad_t tb;
	
	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw 0,%0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return tb;
}

/*
 * Fill in *tvp with current time with microsecond resolution.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	u_long tb;
	u_long ticks;
	int msr, scratch;
	
	asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	asm ("mftb %0" : "=r"(tb));
	ticks = (tb - lasttb) * ns_per_tick;
	*tvp = time;
	asm volatile ("mtmsr %0" :: "r"(msr));
	ticks /= 1000;
	tvp->tv_usec += ticks;
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(n)
	unsigned n;
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;
	
	tb = mftb();
	tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
	tbh = tb >> 32;
	tbl = tb;
	asm volatile ("1: mftbu %0; cmplw %0,%1; blt 1b; bgt 2f;"
		      "mftb %0; cmplw %0,%2; blt 1b; 2:"
		      : "=r"(scratch) : "r"(tbh), "r"(tbl));
}

/*
 * Nothing to do.
 */
void
setstatclockrate(arg)
	int arg;
{
	/* Do nothing */
}
