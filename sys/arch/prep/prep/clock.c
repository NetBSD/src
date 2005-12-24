/*	$NetBSD: clock.c,v 1.13 2005/12/24 22:45:36 perry Exp $	*/
/*      $OpenBSD: clock.c,v 1.3 1997/10/13 13:42:53 pefo Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.13 2005/12/24 22:45:36 perry Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/clock_subr.h>

#include <powerpc/spr.h>

#define	MINYEAR	1990

void decr_intr __P((struct clockframe *));

u_long ticks_per_sec;
u_long ns_per_tick;
static long ticks_per_intr;
static volatile u_long lasttb;

struct device *clockdev;
const struct clockfns *clockfns;

static todr_chip_handle_t todr_handle;

void
todr_attach(handle)
	todr_chip_handle_t handle;
{

	if (todr_handle)
		panic("todr_attach: to many todclock configured");

	todr_handle = handle;
}

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks()
{

	ticks_per_intr = ticks_per_sec / hz;
	cpu_timebase = ticks_per_sec;
	if ((mfpvr() >> 16) == MPC601)
		__asm volatile ("mfspr %0,%1" : "=r"(lasttb) : "n"(SPR_RTCL_R));
	else
		__asm volatile ("mftb %0" : "=r"(lasttb));
	__asm volatile ("mtdec %0" :: "r"(ticks_per_intr));
}

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
	time_t base;
{
	int badbase, waszero;

	badbase = 0;
	waszero = (base == 0);

	if (base < (MINYEAR - 1970) * SECYR) {
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = (MINYEAR - 1970) * SECYR + 186 * SECDAY + SECDAY / 2;
		badbase = 1;
	}

	if (todr_gettime(todr_handle, &time) != 0 ||
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
		/*
		 * See if we gained/lost two or more days;
		 * if so, assume something is amiss.
		 */
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
resettodr()
{

	if (time.tv_sec == 0)
		return;

	if (todr_settime(todr_handle, &time) != 0)
		printf("resettodr: cannot set time in time-of-day clock\n");
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(arg)
	int arg;
{

	/* Nothing we can do */
}

void
decr_intr(frame)
	struct clockframe *frame;
{
	int msr;
	int pri;
	u_long tb;
	long ticks;
	int nticks;
	extern long intrcnt[];

	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

	/*
	 * Based on the actual time delay since the last decrementer reload,
	 * we arrange for earlier interrupt next time.
	 */
	__asm ("mfdec %0" : "=r"(ticks));
	for (nticks = 0; ticks < 0; nticks++)
		ticks += ticks_per_intr;
	__asm volatile ("mtdec %0" :: "r"(ticks));

	uvmexp.intrs++;
	intrcnt[CNT_CLOCK]++;

	pri = splclock();
	if (pri & SPL_CLOCK) {
		tickspending += nticks;
	} else {
		nticks += tickspending;
		tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
		if ((mfpvr() >> 16) == MPC601) {
			__asm volatile ("mfspr %0,%1" : "=r"(tb) : "n"(SPR_RTCL_R));
		} else {
			__asm volatile ("mftb %0" : "=r"(tb));
		}
		lasttb = tb + ticks - ticks_per_intr;

		/*
		 * Reenable interrupts
		 */
		__asm volatile ("mfmsr %0; ori %0, %0, %1; mtmsr %0"
			      : "=r"(msr) : "K"(PSL_EE));

		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		frame->pri = pri | SINT_CLOCK;
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);
	}
	splx(pri);
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
	
	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));
	if ((mfpvr() >> 16) == MPC601)
		__asm volatile ("mfspr %0,%1" : "=r"(tb) : "n"(SPR_RTCL_R));
	else
		__asm volatile ("mftb %0" : "=r"(tb));
	ticks = (tb - lasttb) * ns_per_tick;
	*tvp = time;
	__asm volatile ("mtmsr %0" :: "r"(msr));
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
	unsigned int n;
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;

	if ((mfpvr() >> 16) == MPC601) {
		u_int32_t rtc[2];

		mfrtc(rtc);
		while (n >= 1000000) {
			rtc[0]++;
			n -= 1000000;
		}
		rtc[1] += (n * 1000);
		if (rtc[1] >= 1000000000) {
			rtc[0]++;
			rtc[1] -= 1000000000;
		}
		__asm volatile ("1: mfspr %0,%3; cmplw %0,%1; blt 1b; bgt 2f;"
		    "mfspr %0,%4; cmplw %0,%2; blt 1b; 2:"
		    : "=&r"(scratch)
		    : "r"(rtc[0]), "r"(rtc[1]), "n"(SPR_RTCU_R), "n"(SPR_RTCL_R)
		    : "cr0");
	} else {
		tb = mftb();
		tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
		tbh = tb >> 32;
		tbl = tb;
		__asm volatile ("1: mftbu %0; cmplw %0,%1; blt 1b; bgt 2f;"
			      "mftb %0; cmplw %0,%2; blt 1b; 2:"
			      : "=&r"(scratch) : "r"(tbh), "r"(tbl)
			      : "cr0");
	}
}
