/*	$NetBSD: clock.c,v 1.1 2002/03/13 23:12:11 eeh Exp $	*/
/*      $OpenBSD: clock.c,v 1.3 1997/10/13 13:42:53 pefo Exp $  */

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

#include <machine/walnut.h>
#include <machine/dcr.h>

#include <powerpc/spr.h>

/*
 * Initially we assume a processor with a bus frequency of 12.5 MHz.
 */
static u_long ticks_per_sec;
static u_long ns_per_tick;
static long ticks_per_intr;
static volatile u_long lasttb;
static u_long ticksmissed;
static volatile int tickspending;

void decr_intr(struct clockframe *);	/* called from trap_subr.S */
void stat_intr(struct clockframe *);	/* called from trap_subr.S */
static inline u_quad_t mftb(void);

#ifdef FAST_STAT_CLOCK
/* Stat clock runs at ~ 1.5KHz */
#define PERIOD_POWER	17
#define TCR_PERIOD	TCR_FP_2_17
#else
/* Stat clock runs at ~ 95Hz */
#define PERIOD_POWER	21
#define TCR_PERIOD	TCR_FP_2_21
#endif  


void
stat_intr(struct clockframe *frame)
{
	extern u_long intrcnt[];
	
	mtspr(SPR_TSR, TSR_FIS);	/* Clear TSR[FIS] */
	intrcnt[CNT_STATCLOCK]++;
  	statclock(frame);
}

void
decr_intr(struct clockframe *frame)
{
	int pri;
	long tick, xticks;
	int nticks;
	extern u_long intrcnt[];
	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

	asm volatile("mftb %0":"=r"(tick):);
	mtspr(SPR_TSR, TSR_PIS);	/* Clear TSR[PIS] */
	/*
	 * lasttb is used during microtime. Set it to the virtual
	 * start of this tick interval.
	 */
	xticks = tick - lasttb;	/* Number of TLB cycles since last exception */
	for (nticks = 0; xticks > ticks_per_intr; nticks++)
		xticks -= ticks_per_intr;
	lasttb = tick - xticks;

	intrcnt[CNT_CLOCK]++;
	pri = splclock();
	if (pri & SPL_CLOCK) {
		tickspending += nticks;
		ticksmissed+= nticks;
	} else {
		nticks += tickspending;
		tickspending = 0;

		/*
		 * Reenable interrupts
		 */
		asm volatile ("wrteei 1");

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

void
cpu_initclocks(void)
{
	ticks_per_intr = ticks_per_sec / hz;
	stathz = profhz = ticks_per_sec / (1<<PERIOD_POWER); 
	printf("Setting PIT to %ld/%d = %ld\n", ticks_per_sec, hz, ticks_per_intr);
	asm volatile ("mftb %0" : "=r"(lasttb));
	mtspr(SPR_PIT, ticks_per_intr);
	/* Enable PIT & FIT(2^17c = 0.655ms) interrupts and auto-reload */
	mtspr(SPR_TCR, TCR_PIE | TCR_ARE | TCR_FIE | TCR_PERIOD);	
}

void
calc_delayconst(void)
{

	ticks_per_sec = board_data.processor_speed;
	ns_per_tick = 1000000000 / ticks_per_sec;

	/* Make sure that timers run at CPU frequency */
	mtdcr(DCR_CPC0_CR1, mfdcr(DCR_CPC0_CR1) & ~CPC0_CR1_CETE);
}

static inline u_quad_t
mftb(void)
{
	u_long scratch;
	u_quad_t tb;

	asm ("1: mftbu %0; mftb %0+1; mftbu %1; cmpw %0,%1; bne 1b"
	    : "=r"(tb), "=r"(scratch));
	return tb;
}

/*
 * Fill in *tvp with current time with microsecond resolution.
 */
void
microtime(struct timeval *tvp)
{
	u_long tb;
	u_long ticks;
	int msr;

	asm volatile ("mfmsr %0; wrteei 0" : "=r"(msr) :);
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
delay(unsigned int n)
{
	u_quad_t tb;
	u_long tbh, tbl, scratch;

	tb = mftb();
	/* use 1000ULL to force 64 bit math to avoid 32 bit overflows */
	tb += (n * 1000ULL + ns_per_tick - 1) / ns_per_tick;
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
setstatclockrate(int arg)
{

	/* Do nothing */
}
