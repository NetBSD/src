/*	$NetBSD: tsc.c,v 1.25.2.2 2010/10/22 07:21:43 uebayasi Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: tsc.c,v 1.25.2.2 2010/10/22 07:21:43 uebayasi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/lwp.h>
#include <sys/atomic.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <machine/cpu_counter.h>
#include <machine/cpuvar.h>
#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#include <machine/cputypes.h>

#include "tsc.h"

u_int	tsc_get_timecount(struct timecounter *);

uint64_t	tsc_freq; /* exported for sysctl */
static int64_t	tsc_drift_max = 250;	/* max cycles */
static int64_t	tsc_drift_observed;

static volatile int64_t	tsc_sync_val;
static volatile struct cpu_info	*tsc_sync_cpu;

static struct timecounter tsc_timecounter = {
	.tc_get_timecount = tsc_get_timecount,
	.tc_counter_mask = ~0U,
	.tc_name = "TSC",
	.tc_quality = 3000,
};

void
tsc_tc_init(void)
{
	struct cpu_info *ci;
	uint32_t descs[4];
	bool safe;

	if (!cpu_hascounter()) {
		return;
	}

	ci = curcpu();
	safe = false;
	tsc_freq = ci->ci_data.cpu_cc_freq;

	if (cpu_vendor == CPUVENDOR_INTEL) {
		/*
		 * From Intel(tm) 64 and IA-32 Architectures Software
		 * Developer's Manual Volume 3A: System Programming Guide,
		 * Part 1 Order Number: 253668-026US February 2008, these
		 * are the processors where the TSC is known safe:
		 *
		 * Pentium 4, Intel Xeon (family 0f, models 03 and higher)
		 * Core Solo and Core Duo processors (family 06, model 0e)
		 * Xeon 5100 series and Core 2 Duo (family 06, model 0f)
		 *
		 * We'll also assume that it's safe on the Pentium, and
		 * that it's safe on P-II and P-III Xeons due to the
		 * typical configuration of those systems.
		 */
		switch (CPUID2FAMILY(ci->ci_signature)) {
		case 0x05:
			safe = true;
			break;
		case 0x06:
			safe = CPUID2MODEL(ci->ci_signature) == 0x0e ||
			    CPUID2MODEL(ci->ci_signature) == 0x0f ||
			    CPUID2MODEL(ci->ci_signature) == 0x0a;
			break;
		case 0x0f:
			safe = CPUID2MODEL(ci->ci_signature) >= 0x03;
			break;
		}
	} else if (cpu_vendor == CPUVENDOR_AMD) {
		/*
		 * TSC and Power Management Events on AMD Processors
		 * Nov 2, 2005 Rich Brunner, AMD Fellow
		 * http://lkml.org/lkml/2005/11/4/173
		 *
		 * There are a lot of recommendations in this note.
		 * We're only going to follow the simple, reliable
		 * ones.
		 */
		switch (CPUID2FAMILY(ci->ci_signature)) {
		case 0x06:
		case 0x07:
			/*
			 * TSC is fine, apart from P-state changes.
			 * The PowerNow code need hooks to disable
			 * the TSC if the user modifies operating
			 * parameters.  For now, punt.  XXX
			 */
			break;
		case 0x0f:
			/* Check for "invariant TSC", bit 8 of %edx. */
			x86_cpuid(0x80000007, descs);
			safe = (descs[3] & CPUID_APM_TSC) != 0;
			break;
		}
	}

	if (!safe) {
		aprint_debug("TSC not known safe on this CPU\n");
		tsc_timecounter.tc_quality = -100;
	} else if (tsc_drift_observed > tsc_drift_max) {
		aprint_error("ERROR: %lld cycle TSC drift observed\n",
		    (long long)tsc_drift_observed);
		tsc_timecounter.tc_quality = -100;
		safe = false;
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
static void
tsc_read_bp(struct cpu_info *ci, uint64_t *bptscp, uint64_t *aptscp)
{
	uint64_t bptsc;

	if (atomic_swap_ptr(&tsc_sync_cpu, ci) != NULL) {
		panic("tsc_sync_bp: 1");
	}

	/* Flag it and read our TSC. */
	atomic_or_uint(&ci->ci_flags, CPUF_SYNCTSC);
	bptsc = rdmsr(MSR_TSC) >> 1;

	/* Wait for remote to complete, and read ours again. */
	while ((ci->ci_flags & CPUF_SYNCTSC) != 0) {
		__insn_barrier();
	}
	bptsc += (rdmsr(MSR_TSC) >> 1);

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
	uint64_t bptsc, aptsc;

	tsc_read_bp(ci, &bptsc, &aptsc); /* discarded - cache effects */
	tsc_read_bp(ci, &bptsc, &aptsc);

	/* Compute final value to adjust for skew. */
	ci->ci_data.cpu_cc_skew = bptsc - aptsc;
}

/*
 * Called during startup of AP, by the AP itself.  Interrupts are
 * disabled on entry.
 */
static void
tsc_post_ap(struct cpu_info *ci)
{
	uint64_t tsc;

	/* Wait for go-ahead from primary. */
	while ((ci->ci_flags & CPUF_SYNCTSC) == 0) {
		__insn_barrier();
	}
	tsc = (rdmsr(MSR_TSC) >> 1);

	/* Instruct primary to read its counter. */
	atomic_and_uint(&ci->ci_flags, ~CPUF_SYNCTSC);
	tsc += (rdmsr(MSR_TSC) >> 1);

	/* Post result.  Ensure the whole value goes out atomically. */
	(void)atomic_swap_64(&tsc_sync_val, tsc);

	if (atomic_swap_ptr(&tsc_sync_cpu, NULL) != ci) {
		panic("tsc_sync_ap");
	}
}

void
tsc_sync_ap(struct cpu_info *ci)
{

	tsc_post_ap(ci);
	tsc_post_ap(ci);
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
