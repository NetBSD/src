/*	$NetBSD: tsc.c,v 1.57.4.1 2024/10/02 18:24:35 martin Exp $	*/

/*-
 * Copyright (c) 2008, 2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: tsc.c,v 1.57.4.1 2024/10/02 18:24:35 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/lwp.h>
#include <sys/atomic.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/xcall.h>
#include <sys/lock.h>

#include <machine/cpu_counter.h>
#include <machine/cpuvar.h>
#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#include <machine/cputypes.h>

#include "tsc.h"

#define	TSC_SYNC_ROUNDS		1000
#define	ABS(a)			((a) >= 0 ? (a) : -(a))

static u_int	tsc_get_timecount(struct timecounter *);

static void	tsc_delay(unsigned int);

static uint64_t	tsc_dummy_cacheline __cacheline_aligned;
uint64_t	tsc_freq __read_mostly;	/* exported for sysctl */
static int64_t	tsc_drift_max = 1000;	/* max cycles */
static int64_t	tsc_drift_observed;
uint64_t	(*rdtsc)(void) = rdtsc_cpuid;
uint64_t	(*cpu_counter)(void) = cpu_counter_cpuid;
uint32_t	(*cpu_counter32)(void) = cpu_counter32_cpuid;

int tsc_user_enabled = 1;

static volatile int64_t	tsc_sync_val;
static volatile struct cpu_info	*tsc_sync_cpu;

static struct timecounter tsc_timecounter = {
	.tc_get_timecount = tsc_get_timecount,
	.tc_counter_mask = ~0U,
	.tc_name = "TSC",
	.tc_quality = 3000,
};

bool
tsc_is_invariant(void)
{
	struct cpu_info *ci;
	uint32_t descs[4];
	uint32_t family;
	bool invariant;

	if (!cpu_hascounter())
		return false;

	ci = curcpu();
	invariant = false;

	if (cpu_vendor == CPUVENDOR_INTEL) {
		/*
		 * From Intel(tm) 64 and IA-32 Architectures Software
		 * Developer's Manual Volume 3A: System Programming Guide,
		 * Part 1, 17.13 TIME_STAMP COUNTER, these are the processors
		 * where the TSC is known invariant:
		 *
		 * Pentium 4, Intel Xeon (family 0f, models 03 and higher)
		 * Core Solo and Core Duo processors (family 06, model 0e)
		 * Xeon 5100 series and Core 2 Duo (family 06, model 0f)
		 * Core 2 and Xeon (family 06, model 17)
		 * Atom (family 06, model 1c)
		 *
		 * We'll also assume that it's safe on the Pentium, and
		 * that it's safe on P-II and P-III Xeons due to the
		 * typical configuration of those systems.
		 *
		 */
		switch (CPUID_TO_BASEFAMILY(ci->ci_signature)) {
		case 0x05:
			invariant = true;
			break;
		case 0x06:
			invariant = CPUID_TO_MODEL(ci->ci_signature) == 0x0e ||
			    CPUID_TO_MODEL(ci->ci_signature) == 0x0f ||
			    CPUID_TO_MODEL(ci->ci_signature) == 0x17 ||
			    CPUID_TO_MODEL(ci->ci_signature) == 0x1c;
			break;
		case 0x0f:
			invariant = CPUID_TO_MODEL(ci->ci_signature) >= 0x03;
			break;
		}
	} else if (cpu_vendor == CPUVENDOR_AMD) {
		/*
		 * TSC and Power Management Events on AMD Processors
		 * Nov 2, 2005 Rich Brunner, AMD Fellow
		 * http://lkml.org/lkml/2005/11/4/173
		 *
		 * See Appendix E.4.7 CPUID Fn8000_0007_EDX Advanced Power
		 * Management Features, AMD64 Architecture Programmer's
		 * Manual Volume 3: General-Purpose and System Instructions.
		 * The check is done below.
		 */

		 /*
		  * AMD Errata 778: Processor Core Time Stamp Counters May
		  * Experience Drift
		  *
		  * This affects all family 15h and family 16h processors.
		  */
		switch (CPUID_TO_FAMILY(ci->ci_signature)) {
		case 0x15:
		case 0x16:
			return false;
		}
	}

	/*
	 * The best way to check whether the TSC counter is invariant or not
	 * is to check CPUID 80000007.
	 */
	family = CPUID_TO_BASEFAMILY(ci->ci_signature);
	if (((cpu_vendor == CPUVENDOR_INTEL) || (cpu_vendor == CPUVENDOR_AMD))
	    && ((family == 0x06) || (family == 0x0f))) {
		x86_cpuid(0x80000000, descs);
		if (descs[0] >= 0x80000007) {
			x86_cpuid(0x80000007, descs);
			invariant = (descs[3] & CPUID_APM_ITSC) != 0;
		}
	}

	return invariant;
}

/* Setup function pointers for rdtsc() and timecounter(9). */
void
tsc_setfunc(struct cpu_info *ci)
{
	bool use_lfence, use_mfence;

	use_lfence = use_mfence = false;

	/*
	 * XXX On AMD, we might be able to use lfence for some cases:
	 *   a) if MSR_DE_CFG exist and the bit 1 is set.
	 *   b) family == 0x0f or 0x11. Those have no MSR_DE_CFG and
	 *      lfence is always serializing.
	 *
	 * We don't use it because the test result showed mfence was better
	 * than lfence with MSR_DE_CFG.
	 */
	if (cpu_vendor == CPUVENDOR_AMD)
		use_mfence = true;
	else if (cpu_vendor == CPUVENDOR_INTEL)
		use_lfence = true;

	/* LFENCE and MFENCE are applicable if SSE2 is set. */
	if ((ci->ci_feat_val[0] & CPUID_SSE2) == 0)
		use_lfence = use_mfence = false;

#define TSC_SETFUNC(fence)						      \
	do {								      \
		rdtsc = rdtsc_##fence;					      \
		cpu_counter = cpu_counter_##fence;			      \
		cpu_counter32 = cpu_counter32_##fence;			      \
	} while (/* CONSTCOND */ 0)

	if (use_lfence)
		TSC_SETFUNC(lfence);
	else if (use_mfence)
		TSC_SETFUNC(mfence);
	else
		TSC_SETFUNC(cpuid);

	aprint_verbose_dev(ci->ci_dev, "Use %s to serialize rdtsc\n",
	    use_lfence ? "lfence" : (use_mfence ? "mfence" : "cpuid"));
}

/*
 * Initialize timecounter(9) and DELAY() function of TSC.
 *
 * This function is called after all secondary processors were brought up
 * and drift has been measured, and after any other potential delay funcs
 * have been installed (e.g. lapic_delay()).
 */
void
tsc_tc_init(void)
{
	struct cpu_info *ci;
	bool invariant;

	if (!cpu_hascounter())
		return;

	ci = curcpu();
	tsc_freq = ci->ci_data.cpu_cc_freq;
	invariant = tsc_is_invariant();
	if (!invariant) {
		aprint_debug("TSC not known invariant on this CPU\n");
		tsc_timecounter.tc_quality = -100;
	} else if (tsc_drift_observed > tsc_drift_max) {
		aprint_error("ERROR: %lld cycle TSC drift observed\n",
		    (long long)tsc_drift_observed);
		tsc_timecounter.tc_quality = -100;
		invariant = false;
	} else if (vm_guest == VM_GUEST_NO) {
		delay_func = tsc_delay;
	} else if (vm_guest == VM_GUEST_VIRTUALBOX) {
		tsc_timecounter.tc_quality = -100;
	}

	if (tsc_freq != 0) {
		tsc_timecounter.tc_frequency = tsc_freq;
		tc_init(&tsc_timecounter);
	}
}

/*
 * Record drift (in clock cycles).  Called during AP startup.
 */
void
tsc_sync_drift(int64_t drift)
{

	if (drift < 0)
		drift = -drift;
	if (drift > tsc_drift_observed)
		tsc_drift_observed = drift;
}

/*
 * Called during startup of APs, by the boot processor.  Interrupts
 * are disabled on entry.
 */
static void __noinline
tsc_read_bp(struct cpu_info *ci, uint64_t *bptscp, uint64_t *aptscp)
{
	uint64_t bptsc;

	if (atomic_swap_ptr(&tsc_sync_cpu, ci) != NULL) {
		panic("tsc_sync_bp: 1");
	}

	/* Prepare a cache miss for the other side. */
	(void)atomic_swap_uint((void *)&tsc_dummy_cacheline, 0);

	/* Flag our readiness. */
	atomic_or_uint(&ci->ci_flags, CPUF_SYNCTSC);

	/* Wait for other side then read our TSC. */
	while ((ci->ci_flags & CPUF_SYNCTSC) != 0) {
		__insn_barrier();
	}
	bptsc = rdtsc();

	/* Wait for the results to come in. */
	while (tsc_sync_cpu == ci) {
		x86_pause();
	}
	if (tsc_sync_cpu != NULL) {
		panic("tsc_sync_bp: 2");
	}

	*bptscp = bptsc;
	*aptscp = tsc_sync_val;
}

void
tsc_sync_bp(struct cpu_info *ci)
{
	int64_t bptsc, aptsc, val, diff;

	if (!cpu_hascounter())
		return;

	val = INT64_MAX;
	for (int i = 0; i < TSC_SYNC_ROUNDS; i++) {
		tsc_read_bp(ci, &bptsc, &aptsc);
		diff = bptsc - aptsc;
		if (ABS(diff) < ABS(val)) {
			val = diff;
		}
	}

	ci->ci_data.cpu_cc_skew = val;
}

/*
 * Called during startup of AP, by the AP itself.  Interrupts are
 * disabled on entry.
 */
static void __noinline
tsc_post_ap(struct cpu_info *ci)
{
	uint64_t tsc;

	/* Wait for go-ahead from primary. */
	while ((ci->ci_flags & CPUF_SYNCTSC) == 0) {
		__insn_barrier();
	}

	/* Instruct primary to read its counter. */
	atomic_and_uint(&ci->ci_flags, ~CPUF_SYNCTSC);

	/* Suffer a cache miss, then read TSC. */
	__insn_barrier();
	tsc = tsc_dummy_cacheline;
	__insn_barrier();
	tsc += rdtsc();

	/* Post result.  Ensure the whole value goes out atomically. */
	(void)atomic_swap_64(&tsc_sync_val, tsc);

	if (atomic_swap_ptr(&tsc_sync_cpu, NULL) != ci) {
		panic("tsc_sync_ap");
	}
}

void
tsc_sync_ap(struct cpu_info *ci)
{

	if (!cpu_hascounter())
		return;

	for (int i = 0; i < TSC_SYNC_ROUNDS; i++) {
		tsc_post_ap(ci);
	}
}

static void
tsc_apply_cpu(void *arg1, void *arg2)
{
	bool enable = arg1 != NULL;
	if (enable) {
		lcr4(rcr4() & ~CR4_TSD);
	} else {
		lcr4(rcr4() | CR4_TSD);
	}
}

void
tsc_user_enable(void)
{
	uint64_t xc;

	xc = xc_broadcast(0, tsc_apply_cpu, (void *)true, NULL);
	xc_wait(xc);
}

void
tsc_user_disable(void)
{
	uint64_t xc;

	xc = xc_broadcast(0, tsc_apply_cpu, (void *)false, NULL);
	xc_wait(xc);
}

uint64_t
cpu_frequency(struct cpu_info *ci)
{

	return ci->ci_data.cpu_cc_freq;
}

int
cpu_hascounter(void)
{

	return cpu_feature[0] & CPUID_TSC;
}

static void
tsc_delay(unsigned int us)
{
	uint64_t start, delta;

	start = cpu_counter();
	delta = (uint64_t)us * tsc_freq / 1000000;

	while ((cpu_counter() - start) < delta) {
		x86_pause();
	}
}

static u_int
tsc_get_timecount(struct timecounter *tc)
{
#ifdef _LP64 /* requires atomic 64-bit store */
	static __cpu_simple_lock_t lock = __SIMPLELOCK_UNLOCKED;
	static int lastwarn;
	uint64_t cur, prev;
	lwp_t *l = curlwp;
	int ticks;

	/*
	 * Previous value must be read before the counter and stored to
	 * after, because this routine can be called from interrupt context
	 * and may run over the top of an existing invocation.  Ordering is
	 * guaranteed by "volatile" on md_tsc.
	 */
	prev = l->l_md.md_tsc;
	cur = cpu_counter();
	if (__predict_false(cur < prev)) {
		if ((cur >> 63) == (prev >> 63) &&
		    __cpu_simple_lock_try(&lock)) {
			ticks = getticks();
			if (ticks - lastwarn >= hz) {
				printf(
				    "WARNING: TSC time went backwards by %u - "
				    "change sysctl(7) kern.timecounter?\n",
				    (unsigned)(prev - cur));
				lastwarn = ticks;
			}
			__cpu_simple_unlock(&lock);
		}
	}
	l->l_md.md_tsc = cur;
	return (uint32_t)cur;
#else
	return cpu_counter32();
#endif
}

/*
 * tsc has been reset; zero the cached tsc of every lwp in the system
 * so we don't spuriously report that the tsc has gone backward.
 * Caller must ensure all LWPs are quiescent (except the current one,
 * obviously) and interrupts are blocked while we update this.
 */
void
tsc_tc_reset(void)
{
	struct lwp *l;

	LIST_FOREACH(l, &alllwp, l_list)
		l->l_md.md_tsc = 0;
}
