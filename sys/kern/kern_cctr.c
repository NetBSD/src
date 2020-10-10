/*	$NetBSD: kern_cctr.c,v 1.12 2020/10/10 18:18:04 thorpej Exp $	*/

/*-
 * Copyright (c) 2020 Jason R. Thorpe
 * Copyright (c) 2018 Naruaki Etomi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Most of the following was adapted from the Linux/ia64 cycle counter
 * synchronization algorithm:
 *
 *	IA-64 Linux Kernel: Design and Implementation p356-p361
 *	(Hewlett-Packard Professional Books)
 *
 * Here's a rough description of how it works.
 *
 * The primary CPU is the reference monotonic counter.  Each secondary
 * CPU is responsible for knowing the offset of its own cycle counter
 * relative to the primary's.  When the time counter is read, the CC
 * value is adjusted by this delta.
 *
 * Calibration happens periodically, and works like this:
 *
 * Secondary CPU                               Primary CPU
 *   Send IPI to publish reference CC
 *                                   --------->
 *                                             Indicate Primary Ready
 *                <----------------------------
 *   T0 = local CC
 *   Indicate Secondary Ready
 *                           ----------------->
 *     (assume this happens at Tavg)           Publish reference CC
 *                                             Indicate completion
 *                    <------------------------
 *   Notice completion
 *   T1 = local CC
 *
 *   Tavg = (T0 + T1) / 2
 *
 *   Delta = Tavg - Published primary CC value
 *
 * "Notice completion" is performed by waiting for the primary to set
 * the calibration state to FINISHED.  This is a little unfortunate,
 * because T0->Tavg involves a single store-release on the secondary, and
 * Tavg->T1 involves a store-relaxed and a store-release.  It would be
 * better to simply wait for the reference CC to transition from 0 to
 * non-0 (i.e. just wait for a single store-release from Tavg->T1), but
 * if the cycle counter just happened to read back as 0 at that instant,
 * we would never break out of the loop.
 *
 * We trigger calibration roughly once a second; the period is actually
 * skewed based on the CPU index in order to avoid lock contention.  The
 * calibration interval does not need to be precise, and so this is fine.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_cctr.c,v 1.12 2020/10/10 18:18:04 thorpej Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/timepps.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/power.h>
#include <sys/cpu.h>
#include <machine/cpu_counter.h>

/* XXX make cc_timecounter.tc_frequency settable by sysctl() */

#if defined(MULTIPROCESSOR)
static uint32_t cc_primary __cacheline_aligned;
static uint32_t cc_calibration_state __cacheline_aligned;
static kmutex_t cc_calibration_lock __cacheline_aligned;

#define	CC_CAL_START		0	/* initial state */
#define	CC_CAL_PRIMARY_READY	1	/* primary CPU ready to respond */
#define	CC_CAL_SECONDARY_READY	2	/* secondary CPU ready to receive */
#define	CC_CAL_FINISHED		3	/* calibration attempt complete */
#endif /* MULTIPROCESSOR */

static struct timecounter cc_timecounter = {
	.tc_get_timecount	= cc_get_timecount,
	.tc_poll_pps		= NULL,
	.tc_counter_mask	= ~0u,
	.tc_frequency		= 0,
	.tc_name		= "unknown cycle counter",
	/*
	 * don't pick cycle counter automatically
	 * if frequency changes might affect cycle counter
	 */
	.tc_quality		= -100000,

	.tc_priv		= NULL,
	.tc_next		= NULL
};

/*
 * Initialize cycle counter based timecounter.  This must be done on the
 * primary CPU.
 */
struct timecounter *
cc_init(timecounter_get_t getcc, uint64_t freq, const char *name, int quality)
{
	static bool cc_init_done __diagused;
	struct cpu_info * const ci = curcpu();

	KASSERT(!cc_init_done);
	KASSERT(cold);
	KASSERT(CPU_IS_PRIMARY(ci));

#if defined(MULTIPROCESSOR)
	mutex_init(&cc_calibration_lock, MUTEX_DEFAULT, IPL_HIGH);
#endif

	cc_init_done = true;

	ci->ci_cc.cc_delta = 0;
	ci->ci_cc.cc_ticks = 0;
	ci->ci_cc.cc_cal_ticks = 0;

	if (getcc != NULL)
		cc_timecounter.tc_get_timecount = getcc;

	cc_timecounter.tc_frequency = freq;
	cc_timecounter.tc_name = name;
	cc_timecounter.tc_quality = quality;
	tc_init(&cc_timecounter);

	return &cc_timecounter;
}

/*
 * Initialize cycle counter timecounter calibration data on a secondary
 * CPU.  Must be called on that secondary CPU.
 */
void
cc_init_secondary(struct cpu_info * const ci)
{
	KASSERT(!CPU_IS_PRIMARY(curcpu()));
	KASSERT(ci == curcpu());

	ci->ci_cc.cc_ticks = 0;

	/*
	 * It's not critical that calibration be performed in
	 * precise intervals, so skew when calibration is done
	 * on each secondary CPU based on it's CPU index to
	 * avoid contending on the calibration lock.
	 */
	ci->ci_cc.cc_cal_ticks = hz - cpu_index(ci);
	KASSERT(ci->ci_cc.cc_cal_ticks);

	cc_calibrate_cpu(ci);
}

/*
 * pick up tick count scaled to reference tick count
 */
u_int
cc_get_timecount(struct timecounter *tc)
{
#if defined(MULTIPROCESSOR)
	int64_t rcc, ncsw;

 retry:
 	ncsw = curlwp->l_ncsw;

 	__insn_barrier();
	/* N.B. the delta is always 0 on the primary. */
	rcc = cpu_counter32() - curcpu()->ci_cc.cc_delta;
 	__insn_barrier();

 	if (ncsw != curlwp->l_ncsw) {
 		/* Was preempted */ 
 		goto retry;
	}

	return rcc;
#else
	return cpu_counter32();
#endif /* MULTIPROCESSOR */
}

#if defined(MULTIPROCESSOR)
static inline bool
cc_get_delta(struct cpu_info * const ci)
{
	int64_t t0, t1, tcenter = 0;

	t0 = cpu_counter32();

	atomic_store_release(&cc_calibration_state, CC_CAL_SECONDARY_READY);

	for (;;) {
		if (atomic_load_acquire(&cc_calibration_state) ==
		    CC_CAL_FINISHED) {
			break;
		}
	}

	t1 = cpu_counter32();

	if (t1 < t0) {
		/* Overflow! */
		return false;
	}

	/* average t0 and t1 without overflow: */
	tcenter = (t0 >> 1) + (t1 >> 1);
	if ((t0 & 1) + (t1 & 1) == 2)
		tcenter++;

	ci->ci_cc.cc_delta = tcenter - cc_primary;

	return true;
}
#endif /* MULTIPROCESSOR */

/*
 * Called on secondary CPUs to calibrate their cycle counter offset
 * relative to the primary CPU.
 */
void
cc_calibrate_cpu(struct cpu_info * const ci)
{
#if defined(MULTIPROCESSOR)
	KASSERT(!CPU_IS_PRIMARY(ci));

	mutex_spin_enter(&cc_calibration_lock);

 retry:
	atomic_store_release(&cc_calibration_state, CC_CAL_START);

	/* Trigger primary CPU. */
	cc_get_primary_cc();

	for (;;) {
		if (atomic_load_acquire(&cc_calibration_state) ==
		    CC_CAL_PRIMARY_READY) {
			break;
		}
	}

	if (! cc_get_delta(ci)) {
		goto retry;
	}

	mutex_exit(&cc_calibration_lock);
#endif /* MULTIPROCESSOR */
}

void
cc_primary_cc(void)
{
#if defined(MULTIPROCESSOR)
	/* N.B. We expect all interrupts to be blocked. */

	atomic_store_release(&cc_calibration_state, CC_CAL_PRIMARY_READY);

	for (;;) {
		if (atomic_load_acquire(&cc_calibration_state) ==
		    CC_CAL_SECONDARY_READY) {
			break;
		}
	}

	cc_primary = cpu_counter32();
	atomic_store_release(&cc_calibration_state, CC_CAL_FINISHED);
#endif /* MULTIPROCESSOR */
}
