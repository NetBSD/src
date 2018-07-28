/*	$NetBSD: tprof_x86_amd.c,v 1.2.2.2 2018/07/28 04:37:57 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tprof_x86_amd.c,v 1.2.2.2 2018/07/28 04:37:57 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
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

#define	NCTRS	4

#define	PERFEVTSEL(i)		(0xc0010000 + (i))
#define	PERFCTR(i)		(0xc0010004 + (i))

#define	PESR_EVENT_MASK_LO	__BITS(0, 7)
#define	PESR_UNIT_MASK		__BITS(8, 15)
#define	PESR_USR		__BIT(16)
#define	PESR_OS			__BIT(17)
#define	PESR_E			__BIT(18)
#define	PESR_PC			__BIT(19)
#define	PESR_INT		__BIT(20)
				/* bit 21 reserved */
#define	PESR_EN			__BIT(22)
#define	PESR_INV		__BIT(23)
#define	PESR_COUNTER_MASK	__BITS(24, 31)
#define	PESR_EVENT_MASK_HI	__BITS(32, 35)
				/* bit 36-39 reserved */
#define	PESR_GO			__BIT(40)
#define	PESR_HO			__BIT(41)
				/* bit 42-63 reserved */

/*
 * Documents:
 * http://support.amd.com/TechDocs/32559.pdf
 * http://developer.amd.com/wordpress/media/2012/10/Basic_Performance_Measurements.pdf
 */

static int ctrno = 0;
static uint64_t counter_val = 5000000;
static uint64_t counter_reset_val;
static uint32_t amd_lapic_saved[MAXCPUS];
static nmi_handler_t *amd_nmi_handle;
static tprof_param_t amd_param;

static void
tprof_amd_start_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();
	uint64_t pesr;
	uint64_t event_lo;
	uint64_t event_hi;

	event_hi = amd_param.p_event >> 8;
	event_lo = amd_param.p_event & 0xff;
	pesr =
	    ((amd_param.p_flags & TPROF_PARAM_USER) ? PESR_USR : 0) |
	    ((amd_param.p_flags & TPROF_PARAM_KERN) ? PESR_OS : 0) |
	    PESR_INT |
	    __SHIFTIN(event_lo, PESR_EVENT_MASK_LO) |
	    __SHIFTIN(event_hi, PESR_EVENT_MASK_HI) |
	    __SHIFTIN(0, PESR_COUNTER_MASK) |
	    __SHIFTIN(amd_param.p_unit, PESR_UNIT_MASK);

	wrmsr(PERFCTR(ctrno), counter_reset_val);
	wrmsr(PERFEVTSEL(ctrno), pesr);

	amd_lapic_saved[cpu_index(ci)] = lapic_readreg(LAPIC_PCINT);
	lapic_writereg(LAPIC_PCINT, LAPIC_DLMODE_NMI);

	wrmsr(PERFEVTSEL(ctrno), pesr | PESR_EN);
}

static void
tprof_amd_stop_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();

	wrmsr(PERFEVTSEL(ctrno), 0);

	lapic_writereg(LAPIC_PCINT, amd_lapic_saved[cpu_index(ci)]);
}

static int
tprof_amd_nmi(const struct trapframe *tf, void *dummy)
{
	tprof_frame_info_t tfi;
	uint64_t ctr;

	KASSERT(dummy == NULL);

	/* check if it's for us */
	ctr = rdmsr(PERFCTR(ctrno));
	if ((ctr & (UINT64_C(1) << 63)) != 0) { /* check if overflowed */
		/* not ours */
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
	wrmsr(PERFCTR(ctrno), counter_reset_val);

	return 1;
}

static uint64_t
tprof_amd_estimate_freq(void)
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
tprof_amd_ident(void)
{
	struct cpu_info *ci = curcpu();

	if (cpu_vendor != CPUVENDOR_AMD) {
		return TPROF_IDENT_NONE;
	}

	switch (CPUID_TO_FAMILY(ci->ci_signature)) {
	case 0x10:
		return TPROF_IDENT_AMD_GENERIC;
	}

	return TPROF_IDENT_NONE;
}

static int
tprof_amd_start(const tprof_param_t *param)
{
	uint64_t xc;

	if (tprof_amd_ident() == TPROF_IDENT_NONE) {
		return ENOTSUP;
	}

	KASSERT(amd_nmi_handle == NULL);
	amd_nmi_handle = nmi_establish(tprof_amd_nmi, NULL);

	counter_reset_val = - counter_val + 1;
	memcpy(&amd_param, param, sizeof(*param));

	xc = xc_broadcast(0, tprof_amd_start_cpu, NULL, NULL);
	xc_wait(xc);

	return 0;
}

static void
tprof_amd_stop(const tprof_param_t *param)
{
	uint64_t xc;

	xc = xc_broadcast(0, tprof_amd_stop_cpu, NULL, NULL);
	xc_wait(xc);

	KASSERT(amd_nmi_handle != NULL);
	nmi_disestablish(amd_nmi_handle);
	amd_nmi_handle = NULL;
}

const tprof_backend_ops_t tprof_amd_ops = {
	.tbo_estimate_freq = tprof_amd_estimate_freq,
	.tbo_ident = tprof_amd_ident,
	.tbo_start = tprof_amd_start,
	.tbo_stop = tprof_amd_stop,
};
