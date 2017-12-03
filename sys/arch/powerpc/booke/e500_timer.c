/*	$NetBSD: e500_timer.c,v 1.4.12.2 2017/12/03 11:36:36 jdolecek Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: e500_timer.c,v 1.4.12.2 2017/12/03 11:36:36 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/intr.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/openpicreg.h>

uint32_t ticks_per_sec;
static u_long ns_per_tick;

static void init_ppcbooke_tc(void);
static u_int get_ppcbooke_timecount(struct timecounter *);

static struct timecounter ppcbooke_timecounter = {
	get_ppcbooke_timecount,	/* get_timecount */
	0,			/* no poll_pps */
	~0u,			/* counter_mask */
	0,			/* frequency */
	"ppc_timebase",		/* name */
	100,			/* quality */
	NULL,			/* tc_priv */
	NULL			/* tc_next */
};

static inline void 
openpic_write(struct cpu_softc *cpu, bus_size_t offset, uint32_t val)
{

	return bus_space_write_4(cpu->cpu_bst, cpu->cpu_bsh,
	    OPENPIC_BASE + offset, val);
}

int
e500_clock_intr(void *v)
{
	struct trapframe * const tf = v;
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	u_int nticks;

	/*
	 * Check whether we are initialized.
	 */
	if (!cpu->cpu_ticks_per_clock_intr)
		return 0;

	/*
	 * Now let's how delayed the clock interrupt was.  Obviously it must
	 * at least one clock tick since the clock interrupt.  But it might
	 * be more if interrupts were blocked for a long time.  We keep 
	 * suubtracting an interrupts We should be
	 * [well] within a single tick.
	 * We add back one tick (which should put us back above 0).  If we
	 * are still below 0, keep adding ticks until we are above 0.
	 */
	const uint64_t now = mftb();
	uint64_t latency = now - (ci->ci_lastintr + cpu->cpu_ticks_per_clock_intr);
#if 0
	uint64_t orig_latency = latency;
#endif
	if (now < ci->ci_lastintr + cpu->cpu_ticks_per_clock_intr)
		latency = 0;

	nticks = 1 + latency / cpu->cpu_ticks_per_clock_intr;
	latency %= cpu->cpu_ticks_per_clock_intr;
#if 0
	for (nticks = 1; latency >= cpu->cpu_ticks_per_clock_intr; nticks++)  {
		latency -= cpu->cpu_ticks_per_clock_intr;
	}
#endif

	ci->ci_ev_clock.ev_count++;
	cpu->cpu_ev_late_clock.ev_count += nticks - 1;

	/*
	 * lasttb is used during microtime. Set it to the virtual
	 * start of this tick interval.
	 */
#if 0
	if (nticks > 10 || now - ci->ci_lastintr < 7 * cpu->cpu_ticks_per_clock_intr / 8)
	printf("%s: nticks=%u lastintr=%#"PRIx64"(%#"PRIx64") now=%#"PRIx64" latency=%#"PRIx64" orig=%#"PRIx64"\n", __func__,
	    nticks, ci->ci_lastintr, now - latency, now, latency, orig_latency);
#endif
	ci->ci_lastintr = now - latency;
	ci->ci_lasttb = now;

	wrtee(PSL_EE);	/* Reenable interrupts */

	/*
	 * Do standard timer interrupt stuff.
	 */
	while (nticks-- > 0) {
		hardclock(&tf->tf_cf);
	}

	wrtee(0);	/* turn off interrupts */

	tf->tf_srr1 &= ~PSL_POW;	/* make cpu_idle exit */

	return 1;
}

void
cpu_initclocks(void)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;

	cpu->cpu_ticks_per_clock_intr = (ci->ci_data.cpu_cc_freq + hz/2 - 1) / hz;

	/* interrupt established in e500_intr_cpu_init */

	ci->ci_lastintr = ci->ci_lasttb = mftb();
	openpic_write(cpu, cpu->cpu_clock_gtbcr,
	     GTBCR_CI | cpu->cpu_ticks_per_clock_intr);
	openpic_write(cpu, cpu->cpu_clock_gtbcr,
	     cpu->cpu_ticks_per_clock_intr);

	if (CPU_IS_PRIMARY(ci))
		init_ppcbooke_tc();
}

void
calc_delayconst(void)
{
	struct cpu_info * const ci = curcpu();

	ci->ci_data.cpu_cc_freq = board_info_get_number("timebase-frequency");
	ticks_per_sec = (uint32_t)ci->ci_data.cpu_cc_freq;
	ns_per_tick = 1000000000 / (u_int)ci->ci_data.cpu_cc_freq;
}

static u_int
get_ppcbooke_timecount(struct timecounter *tc)
{
	return mftbl();
}

/*
 * Wait for about n microseconds (at least!).
 */
void
delay(unsigned int n)
{
	uint64_t tb;
	u_long tbh, tbl, scratch;

	tb = mftb();
	/* use 1000ULL to force 64 bit math to avoid 32 bit overflows */
	tb += (n * 1000ULL + ns_per_tick - 1) / ns_per_tick;
	tbh = tb >> 32;
	tbl = tb;
	__asm volatile (
		"1:	mfspr	%0,%4"	"\n"
		"	cmplw	%0,%1"	"\n"
		"	blt	1b"	"\n"
		"	bgt	2f"	"\n"
		"	mfspr	%0,%3"	"\n"
		"	cmplw	%0,%2"	"\n"
		"	blt	1b"	"\n"
		"2:" 			"\n"
	    : "=&r"(scratch)
	    : "r"(tbh), "r"(tbl), "n"(SPR_TBL), "n"(SPR_TBU)
	     : "cr0");
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
init_ppcbooke_tc(void)
{
	/* from machdep initialization */
	ppcbooke_timecounter.tc_frequency = curcpu()->ci_data.cpu_cc_freq;
	tc_init(&ppcbooke_timecounter);
}
