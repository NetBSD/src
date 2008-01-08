/*	$NetBSD: clock.c,v 1.18.42.1 2008/01/08 22:10:18 bouyer Exp $	*/
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.18.42.1 2008/01/08 22:10:18 bouyer Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <prop/proplib.h>

#include <machine/cpu.h>

#include <powerpc/spr.h>

/*
 * Initially we assume a processor with a bus frequency of 12.5 MHz.
 */
static u_long ticks_per_sec;
static u_long ns_per_tick;
static long ticks_per_intr;
static volatile u_long lasttb, lasttb2;
static u_long ticksmissed;
static volatile int tickspending;

static void init_ppc4xx_tc(void);
static u_int get_ppc4xx_timecount(struct timecounter *);

static struct timecounter ppc4xx_timecounter = {
	get_ppc4xx_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,			/* frequency */
	"ppc_timebase",		/* name */
	100,			/* quality */
	NULL,			/* tc_priv */
	NULL			/* tc_next */
};

void decr_intr(struct clockframe *);	/* called from trap_subr.S */
void stat_intr(struct clockframe *);	/* called from trap_subr.S */

#ifdef FAST_STAT_CLOCK
/* Stat clock runs at ~ 1.5 kHz */
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
	mtspr(SPR_TSR, TSR_FIS);	/* Clear TSR[FIS] */
	curcpu()->ci_ev_statclock.ev_count++;

	/* Nobody can interrupt us, but see if we're allowed to run. */
	if (! (curcpu()->ci_cpl & mask_statclock))
  		statclock(frame);
}

void
decr_intr(struct clockframe *frame)
{
	int pri;
	long tbtick, xticks;
	int nticks;

	/*
	 * Check whether we are initialized.
	 */
	if (!ticks_per_intr)
		return;

	tbtick = mftbl();
	mtspr(SPR_TSR, TSR_PIS);	/* Clear TSR[PIS] */

	xticks = tbtick - lasttb2;	/* Number of TLB cycles since last exception */
	for (nticks = 0; xticks > ticks_per_intr; nticks++)
		xticks -= ticks_per_intr;
	lasttb2 = tbtick - xticks;

	curcpu()->ci_ev_clock.ev_count++;
	pri = splclock();
	if (pri & mask_clock) {
		tickspending += nticks;
		ticksmissed += nticks;
	} else {
		nticks += tickspending;
		tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
		lasttb = lasttb2;

		/*
		 * Reenable interrupts
		 */
		__asm volatile ("wrteei 1");

		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		frame->pri = pri | mask_clock;
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
	/* Initialized in powerpc/ibm4xx/cpu.c */
	evcnt_attach_static(&curcpu()->ci_ev_clock);
	evcnt_attach_static(&curcpu()->ci_ev_statclock);

	ticks_per_intr = ticks_per_sec / hz;
	stathz = profhz = ticks_per_sec / (1 << PERIOD_POWER);

	printf("Setting PIT to %ld/%d = %ld\n", ticks_per_sec, hz,
	    ticks_per_intr);

	lasttb2 = lasttb = mftbl();
	mtspr(SPR_PIT, ticks_per_intr);

	/* Enable PIT & FIT(2^17c = 0.655ms) interrupts and auto-reload */
	mtspr(SPR_TCR, TCR_PIE | TCR_ARE | TCR_FIE | TCR_PERIOD);

	init_ppc4xx_tc();
}

void
calc_delayconst(void)
{
	prop_number_t freq;

	freq = prop_dictionary_get(board_properties, "processor-frequency");
	KASSERT(freq != NULL);

	ticks_per_sec = (u_long) prop_number_integer_value(freq);
	ns_per_tick = 1000000000 / ticks_per_sec;
}

static u_int
get_ppc4xx_timecount(struct timecounter *tc)
{
	u_long tb;
	int msr;

	__asm volatile ("mfmsr %0; wrteei 0" : "=r"(msr) :);
	tb = mftbl();
	__asm volatile ("mtmsr %0" :: "r"(msr));

	return tb;
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
	__asm volatile (
#ifdef PPC_IBM403
	    "1:	mftbhi %0	\n"
#else
	    "1:	mftbu %0	\n"
#endif
	    "	cmplw %0,%1	\n"
	    "	blt 1b		\n"
	    "	bgt 2f		\n"
#ifdef PPC_IBM403
	    "	mftblo %0	\n"
#else
	    "	mftb %0		\n"
#endif
	    "	cmplw %0,%2	\n"
	    "	blt 1b		\n"
	    "2: 		\n"
	    : "=&r"(scratch) : "r"(tbh), "r"(tbl) : "cr0");
}

/*
 * Nothing to do.
 */
void
setstatclockrate(int arg)
{

	/* Do nothing */
}

static void
init_ppc4xx_tc(void)
{
	/* from machdep initialization */
	ppc4xx_timecounter.tc_frequency = ticks_per_sec;
	tc_init(&ppc4xx_timecounter);
}
