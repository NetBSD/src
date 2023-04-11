/*	$NetBSD: tprof_x86_intel.c,v 1.8 2023/04/11 10:07:12 msaitoh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tprof_x86_intel.c,v 1.8 2023/04/11 10:07:12 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <sys/cpu.h>
#include <sys/percpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <uvm/uvm.h>		/* VM_MIN_KERNEL_ADDRESS */

#include <x86/nmi.h>

#include <machine/cpufunc.h>
#include <machine/cputypes.h>	/* CPUVENDOR_* */
#include <machine/cpuvar.h>	/* cpu_vendor */
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

static u_int counter_bitwidth;

#define	PERFEVTSEL(i)		(MSR_EVNTSEL0 + (i))
#define	PERFCTR(i)		(MSR_PERFCTR0 + (i))

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

static uint32_t intel_lapic_saved[MAXCPUS];
static nmi_handler_t *intel_nmi_handle;

static uint32_t
tprof_intel_ncounters(void)
{
	uint32_t descs[4];

	if (cpuid_level < 0x0a)
		return 0;

	x86_cpuid(0x0a, descs);

	return __SHIFTOUT(descs[0], CPUID_PERF_NGPPC);
}

static u_int
tprof_intel_counter_bitwidth(u_int counter)
{

	return counter_bitwidth;
}

static inline void
tprof_intel_counter_write(u_int counter, uint64_t val)
{

	wrmsr(PERFCTR(counter), val);
}

static inline uint64_t
tprof_intel_counter_read(u_int counter)
{

	return rdmsr(PERFCTR(counter));
}

static void
tprof_intel_configure_event(u_int counter, const tprof_param_t *param)
{
	uint64_t evtval;

	evtval =
	    __SHIFTIN(param->p_event, PERFEVTSEL_EVENT_SELECT) |
	    __SHIFTIN(param->p_unit, PERFEVTSEL_UNIT_MASK) |
	    ((param->p_flags & TPROF_PARAM_USER) ? PERFEVTSEL_USR : 0) |
	    ((param->p_flags & TPROF_PARAM_KERN) ? PERFEVTSEL_OS : 0) |
	    PERFEVTSEL_INT;
	wrmsr(PERFEVTSEL(counter), evtval);

	/* Reset the counter */
	tprof_intel_counter_write(counter, param->p_value);
}

static void
tprof_intel_start(tprof_countermask_t runmask)
{
	int bit;

	while ((bit = ffs(runmask)) != 0) {
		bit--;
		CLR(runmask, __BIT(bit));
		wrmsr(PERFEVTSEL(bit), rdmsr(PERFEVTSEL(bit)) | PERFEVTSEL_EN);
	}
}

static void
tprof_intel_stop(tprof_countermask_t stopmask)
{
	int bit;

	while ((bit = ffs(stopmask)) != 0) {
		bit--;
		CLR(stopmask, __BIT(bit));
		wrmsr(PERFEVTSEL(bit), rdmsr(PERFEVTSEL(bit)) &
		    ~PERFEVTSEL_EN);
	}
}

static int
tprof_intel_nmi(const struct trapframe *tf, void *arg)
{
	tprof_backend_softc_t *sc = arg;
	tprof_frame_info_t tfi;
	uint32_t pcint;
	int bit;

	uint64_t *counters_offset =
	    percpu_getptr_remote(sc->sc_ctr_offset_percpu, curcpu());
	tprof_countermask_t mask = sc->sc_ctr_ovf_mask;
	while ((bit = ffs(mask)) != 0) {
		bit--;
		CLR(mask, __BIT(bit));

		/* If the highest bit is non zero, then it's not for us. */
		uint64_t ctr = tprof_intel_counter_read(bit);
		if ((ctr & __BIT(counter_bitwidth - 1)) != 0)
			continue;	/* not overflowed */

		if (ISSET(sc->sc_ctr_prof_mask, __BIT(bit))) {
			/* Account for the counter, and reset */
			tprof_intel_counter_write(bit,
			    sc->sc_count[bit].ctr_counter_reset_val);
			counters_offset[bit] +=
			    sc->sc_count[bit].ctr_counter_val + ctr;

			/* Record a sample */
#if defined(__x86_64__)
			tfi.tfi_pc = tf->tf_rip;
#else
			tfi.tfi_pc = tf->tf_eip;
#endif
			tfi.tfi_counter = bit;
			tfi.tfi_inkernel = tfi.tfi_pc >= VM_MIN_KERNEL_ADDRESS;
			tprof_sample(NULL, &tfi);
		} else {
			/* Not profiled, but require to consider overflow */
			counters_offset[bit] += __BIT(counter_bitwidth);
		}
	}

	/* Unmask PMI */
	pcint = lapic_readreg(LAPIC_LVT_PCINT);
	KASSERT((pcint & LAPIC_LVT_MASKED) != 0);
	lapic_writereg(LAPIC_LVT_PCINT, pcint & ~LAPIC_LVT_MASKED);

	return 1;
}

static uint64_t
tprof_intel_counter_estimate_freq(u_int counter)
{

	return curcpu()->ci_data.cpu_cc_freq;
}

static uint32_t
tprof_intel_ident(void)
{
	uint32_t descs[4];

	if (cpu_vendor != CPUVENDOR_INTEL)
		return TPROF_IDENT_NONE;

	if (cpuid_level < 0x0a)
		return TPROF_IDENT_NONE;

	x86_cpuid(0x0a, descs);
	if ((descs[0] & CPUID_PERF_VERSION) == 0)
		return TPROF_IDENT_NONE;

	if ((descs[0] & CPUID_PERF_NGPPC) == 0)
		return TPROF_IDENT_NONE;

	counter_bitwidth = __SHIFTOUT(descs[0], CPUID_PERF_NBWGPPC);

	return TPROF_IDENT_INTEL_GENERIC;
}

static void
tprof_intel_establish_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();

	intel_lapic_saved[cpu_index(ci)] = lapic_readreg(LAPIC_LVT_PCINT);
	lapic_writereg(LAPIC_LVT_PCINT, LAPIC_DLMODE_NMI);
}

static void
tprof_intel_disestablish_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();

	lapic_writereg(LAPIC_LVT_PCINT, intel_lapic_saved[cpu_index(ci)]);
}

static int
tprof_intel_establish(tprof_backend_softc_t *sc)
{
	uint64_t xc;

	if (tprof_intel_ident() == TPROF_IDENT_NONE)
		return ENOTSUP;

	KASSERT(intel_nmi_handle == NULL);
	intel_nmi_handle = nmi_establish(tprof_intel_nmi, sc);

	xc = xc_broadcast(0, tprof_intel_establish_cpu, sc, NULL);
	xc_wait(xc);

	return 0;
}

static void
tprof_intel_disestablish(tprof_backend_softc_t *sc)
{
	uint64_t xc;

	xc = xc_broadcast(0, tprof_intel_disestablish_cpu, sc, NULL);
	xc_wait(xc);

	KASSERT(intel_nmi_handle != NULL);
	nmi_disestablish(intel_nmi_handle);
	intel_nmi_handle = NULL;
}

const tprof_backend_ops_t tprof_intel_ops = {
	.tbo_ident = tprof_intel_ident,
	.tbo_ncounters = tprof_intel_ncounters,
	.tbo_counter_bitwidth = tprof_intel_counter_bitwidth,
	.tbo_counter_read = tprof_intel_counter_read,
	.tbo_counter_estimate_freq = tprof_intel_counter_estimate_freq,
	.tbo_valid_event = NULL,
	.tbo_configure_event = tprof_intel_configure_event,
	.tbo_start = tprof_intel_start,
	.tbo_stop = tprof_intel_stop,
	.tbo_establish = tprof_intel_establish,
	.tbo_disestablish = tprof_intel_disestablish,
};
