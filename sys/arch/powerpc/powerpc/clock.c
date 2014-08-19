/*	$NetBSD: clock.c,v 1.12.12.2 2014/08/20 00:03:20 tls Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.12.12.2 2014/08/20 00:03:20 tls Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <uvm/uvm_extern.h>

#include <powerpc/psl.h>
#include <powerpc/spr.h>
#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/spr.h>
#elif defined (PPC_BOOKE)
#include <powerpc/booke/spr.h>
#elif defined (PPC_IBM4XX)
#include <powerpc/ibm4xx/spr.h>
#else
#error unknown powerpc variant
#endif

void decr_intr(struct clockframe *);
void init_powerpc_tc(void);
static u_int get_powerpc_timecount(struct timecounter *);
#ifdef PPC_OEA601
static u_int get_601_timecount(struct timecounter *);
#endif

uint32_t ticks_per_sec;
uint32_t ns_per_tick;
uint32_t ticks_per_intr = 0;

#ifdef PPC_OEA601
static struct timecounter powerpc_601_timecounter = {
	.tc_get_timecount = get_601_timecount,
	.tc_poll_pps = 0,
	.tc_counter_mask = 0x7fffffff,
	.tc_frequency = 0,
	.tc_name = "rtc",
	.tc_quality = 100,
	.tc_priv = NULL,
	.tc_next = NULL
};
#endif

static struct timecounter powerpc_timecounter = {
	.tc_get_timecount = get_powerpc_timecount,
	.tc_poll_pps = 0,
	.tc_counter_mask = 0x7fffffff,
	.tc_frequency = 0,
	.tc_name = "mftb",
	.tc_quality = 100,
	.tc_priv = NULL,
	.tc_next = NULL
};

/*
 * Start the real-time and statistics clocks. Leave stathz 0 since there
 * are no other timers available.
 */
void
cpu_initclocks(void)
{
	struct cpu_info * const ci = curcpu();
	uint32_t msr;

	ticks_per_intr = ticks_per_sec / hz;
	cpu_timebase = ticks_per_sec;
#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601)
		ci->ci_lasttb = rtc_nanosecs();
	else
#endif
		__asm volatile ("mftb %0" : "=r"(ci->ci_lasttb));
	__asm volatile ("mtdec %0" :: "r"(ticks_per_intr));
	init_powerpc_tc();

	/*
	 * Now allow all hardware interrupts including hardclock(9).
	 */
	__asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
	    : "=r"(msr) : "K"(PSL_EE|PSL_RI));
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int arg)
{

	/* Nothing we can do */
}

void
decr_intr(struct clockframe *cfp)
{
	struct cpu_info * const ci = curcpu();
	const register_t msr = mfmsr();
	int pri;
	u_long tb;
	long ticks;
	int nticks;

	/* Check whether we are initialized */
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

	ci->ci_data.cpu_nintr++;
	ci->ci_ev_clock.ev_count++;

	pri = splclock();
	if (pri >= IPL_CLOCK) {
		ci->ci_tickspending += nticks;
	} else {
		nticks += ci->ci_tickspending;
		ci->ci_tickspending = 0;

		/*
		 * lasttb is used during microtime. Set it to the virtual
		 * start of this tick interval.
		 */
#ifdef PPC_OEA601
		if ((mfpvr() >> 16) == MPC601)
			tb = rtc_nanosecs();
		else
#endif
			__asm volatile ("mftb %0" : "=r"(tb));

		ci->ci_lasttb = tb + ticks - ticks_per_intr;
		ci->ci_idepth++;
		mtmsr(msr | PSL_EE);
		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		while (--nticks > 0)
			hardclock(cfp);
		hardclock(cfp);
		mtmsr(msr);
		ci->ci_idepth--;
	}
	mtmsr(msr | PSL_EE);
	splx(pri);
	mtmsr(msr);
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(unsigned int n)
{
#ifdef _ARCH_PPC64
	uint64_t tb, scratch;
#else
	uint64_t tb;
	uint32_t tbh, tbl, scratch;

#ifdef PPC_OEA601
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
	} else
#endif /* PPC_OEA601 */
#endif /* !_ARCH_PPC64 */
	{
		tb = mftb();
		tb += (n * 1000 + ns_per_tick - 1) / ns_per_tick;
#ifdef _ARCH_PPC64
		__asm volatile ("1: mftb %0; cmpld %0,%1; blt 1b;"
			      : "=&r"(scratch) : "r"(tb)
			      : "cr0");
#else
		tbh = tb >> 32;
		tbl = tb;
		__asm volatile ("1: mftbu %0; cmplw %0,%1; blt 1b; bgt 2f;"
			      "mftb %0; cmplw %0,%2; blt 1b; 2:"
			      : "=&r"(scratch) : "r"(tbh), "r"(tbl)
			      : "cr0");
#endif
	}
}

static u_int
get_powerpc_timecount(struct timecounter *tc)
{
	u_long tb;
	int msr, scratch;
	
	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));

	tb = (u_int)(mftb() & 0x7fffffff);
	mtmsr(msr);

	return tb;
}

#ifdef PPC_OEA601
static u_int
get_601_timecount(struct timecounter *tc)
{
	u_long tb;
	int msr, scratch;
	
	__asm volatile ("mfmsr %0; andi. %1,%0,%2; mtmsr %1"
		      : "=r"(msr), "=r"(scratch) : "K"((u_short)~PSL_EE));

	tb = rtc_nanosecs();
	mtmsr(msr);

	return tb;
}
#endif

void
init_powerpc_tc(void)
{
	struct timecounter *tc;

#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601) {
		tc = &powerpc_601_timecounter;
	} else
#endif
		tc = &powerpc_timecounter;

	tc->tc_frequency = ticks_per_sec;
	tc_init(tc);
}
