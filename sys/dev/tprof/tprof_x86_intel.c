/*	$NetBSD: tprof_x86_intel.c,v 1.2.2.2 2018/07/28 04:37:57 pgoyette Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

/*
 * Copyright (c)2008,2009 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tprof_x86_intel.c,v 1.2.2.2 2018/07/28 04:37:57 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <sys/cpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <uvm/uvm.h>		/* VM_MIN_KERNEL_ADDRESS */

#include <x86/nmi.h>

#include <machine/cpufunc.h>
#include <machine/cputypes.h>	/* CPUVENDOR_* */
#include <machine/cpuvar.h>	/* cpu_vendor */
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#define	PERFEVTSEL_EVENT_SELECT	__BITS(0, 7)
#define	PERFEVTSEL_UNIT_MASK	__BITS(8, 15)
#define	PERFEVTSEL_USR		__BIT(16)
#define	PERFEVTSEL_OS		__BIT(17)
#define	PERFEVTSEL_E		__BIT(18)
#define	PERFEVTSEL_PC		__BIT(19)
#define	PERFEVTSEL_INT		__BIT(20)
#define	PERFEVTSEL_EN		__BIT(22)
#define	PERFEVTSEL_INV		__BIT(23)
#define	PERFEVTSEL_COUNTER_MASK	__BITS(24, 31)

#define CPUID_0A_VERSION	__BITS(0, 7)
#define CPUID_0A_NCOUNTERS	__BITS(8, 15)
#define CPUID_0A_BITWIDTH	__BITS(16, 23)

static uint64_t counter_bitwidth;
static uint64_t counter_val = 5000000;
static uint64_t counter_reset_val;

static uint32_t intel_lapic_saved[MAXCPUS];
static nmi_handler_t *intel_nmi_handle;
static tprof_param_t intel_param;

static void
tprof_intel_start_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();
	uint64_t evtval;

	evtval =
	    __SHIFTIN(intel_param.p_event, PERFEVTSEL_EVENT_SELECT) |
	    __SHIFTIN(intel_param.p_unit, PERFEVTSEL_UNIT_MASK) |
	    ((intel_param.p_flags & TPROF_PARAM_USER) ? PERFEVTSEL_USR : 0) |
	    ((intel_param.p_flags & TPROF_PARAM_KERN) ? PERFEVTSEL_OS : 0) |
	    PERFEVTSEL_INT |
	    PERFEVTSEL_EN;

	wrmsr(MSR_PERFCTR0, counter_reset_val);
	wrmsr(MSR_EVNTSEL0, evtval);

	intel_lapic_saved[cpu_index(ci)] = lapic_readreg(LAPIC_PCINT);
	lapic_writereg(LAPIC_PCINT, LAPIC_DLMODE_NMI);
}

static void
tprof_intel_stop_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();

	wrmsr(MSR_EVNTSEL0, 0);
	wrmsr(MSR_PERFCTR0, 0);

	lapic_writereg(LAPIC_PCINT, intel_lapic_saved[cpu_index(ci)]);
}

static int
tprof_intel_nmi(const struct trapframe *tf, void *dummy)
{
	uint32_t pcint;
	uint64_t ctr;
	tprof_frame_info_t tfi;

	KASSERT(dummy == NULL);

	ctr = rdmsr(MSR_PERFCTR0);
	/* If the highest bit is non zero, then it's not for us. */
	if ((ctr & __BIT(counter_bitwidth-1)) != 0) {
		return 0;
	}

	/* record a sample */
#if defined(__x86_64__)
	tfi.tfi_pc = tf->tf_rip;
#else
	tfi.tfi_pc = tf->tf_eip;
#endif
	tfi.tfi_inkernel = tfi.tfi_pc >= VM_MIN_KERNEL_ADDRESS;
	tprof_sample(NULL, &tfi);

	/* reset counter */
	wrmsr(MSR_PERFCTR0, counter_reset_val);

	/* unmask PMI */
	pcint = lapic_readreg(LAPIC_PCINT);
	KASSERT((pcint & LAPIC_LVT_MASKED) != 0);
	lapic_writereg(LAPIC_PCINT, pcint & ~LAPIC_LVT_MASKED);

	return 1;
}

static uint64_t
tprof_intel_estimate_freq(void)
{
	uint64_t cpufreq = curcpu()->ci_data.cpu_cc_freq;
	uint64_t freq = 10000;

	counter_val = cpufreq / freq;
	if (counter_val == 0) {
		counter_val = UINT64_C(4000000000) / freq;
	}
	return freq;
}

static uint32_t
tprof_intel_ident(void)
{
	uint32_t descs[4];

	if (cpu_vendor != CPUVENDOR_INTEL) {
		return TPROF_IDENT_NONE;
	}

	if (cpuid_level < 0x0A) {
		return TPROF_IDENT_NONE;
	}
	x86_cpuid(0x0A, descs);
	if ((descs[0] & CPUID_0A_VERSION) == 0) {
		return TPROF_IDENT_NONE;
	}
	if ((descs[0] & CPUID_0A_NCOUNTERS) == 0) {
		return TPROF_IDENT_NONE;
	}

	counter_bitwidth = __SHIFTOUT(descs[0], CPUID_0A_BITWIDTH);

	return TPROF_IDENT_INTEL_GENERIC;
}

static int
tprof_intel_start(const tprof_param_t *param)
{
	uint64_t xc;

	if (tprof_intel_ident() == TPROF_IDENT_NONE) {
		return ENOTSUP;
	}

	KASSERT(intel_nmi_handle == NULL);
	intel_nmi_handle = nmi_establish(tprof_intel_nmi, NULL);

	counter_reset_val = - counter_val + 1;
	memcpy(&intel_param, param, sizeof(*param));

	xc = xc_broadcast(0, tprof_intel_start_cpu, NULL, NULL);
	xc_wait(xc);

	return 0;
}

static void
tprof_intel_stop(const tprof_param_t *param)
{
	uint64_t xc;

	xc = xc_broadcast(0, tprof_intel_stop_cpu, NULL, NULL);
	xc_wait(xc);

	KASSERT(intel_nmi_handle != NULL);
	nmi_disestablish(intel_nmi_handle);
	intel_nmi_handle = NULL;
}

const tprof_backend_ops_t tprof_intel_ops = {
	.tbo_estimate_freq = tprof_intel_estimate_freq,
	.tbo_ident = tprof_intel_ident,
	.tbo_start = tprof_intel_start,
	.tbo_stop = tprof_intel_stop,
};
