/* $NetBSD: tprof_armv8.c,v 1.14 2022/05/16 09:42:32 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tprof_armv8.c,v 1.14 2022/05/16 09:42:32 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <dev/tprof/tprof_armv8.h>

static tprof_param_t armv8_pmu_param;
static const u_int armv8_pmu_counter = 0;
static uint32_t counter_val;
static uint32_t counter_reset_val;

static bool
armv8_pmu_event_implemented(uint16_t event)
{
	uint64_t eid[2];

	if (event >= 64)
		return false;

	eid[0] = reg_pmceid0_el0_read();
	eid[1] = reg_pmceid1_el0_read();

	/* The low 32bits of PMCEID[01]_EL0 contain the common events 0 to n */
	const u_int idx = event / 32;
	const u_int bit = event % 32;

	if (eid[idx] & __BIT(bit))
		return true;

	return false;
}

static void
armv8_pmu_set_pmevtyper(u_int counter, uint64_t val)
{
	reg_pmselr_el0_write(counter);
	isb();
	reg_pmxevtyper_el0_write(val);
}

static void
armv8_pmu_set_pmevcntr(u_int counter, uint32_t val)
{
	reg_pmselr_el0_write(counter);
	isb();
	reg_pmxevcntr_el0_write(val);
}

static void
armv8_pmu_start_cpu(void *arg1, void *arg2)
{
	const uint32_t counter_mask = __BIT(armv8_pmu_counter);
	uint64_t pmevtyper;

	/* Disable event counter */
	reg_pmcntenclr_el0_write(counter_mask);

	/* Configure event counter */
	pmevtyper = __SHIFTIN(armv8_pmu_param.p_event, PMEVTYPER_EVTCOUNT);
	if (!ISSET(armv8_pmu_param.p_flags, TPROF_PARAM_USER))
		pmevtyper |= PMEVTYPER_U;
	if (!ISSET(armv8_pmu_param.p_flags, TPROF_PARAM_KERN))
		pmevtyper |= PMEVTYPER_P;

	armv8_pmu_set_pmevtyper(armv8_pmu_counter, pmevtyper);

	/* Enable overflow interrupts */
	reg_pmintenset_el1_write(counter_mask);

	/* Clear overflow flag */
	reg_pmovsclr_el0_write(counter_mask);

	/* Initialize event counter value */
	armv8_pmu_set_pmevcntr(armv8_pmu_counter, counter_reset_val);

	/* Enable event counter */
	reg_pmcntenset_el0_write(counter_mask);
	reg_pmcr_el0_write(PMCR_E);
}

static void
armv8_pmu_stop_cpu(void *arg1, void *arg2)
{
	const uint32_t counter_mask = __BIT(armv8_pmu_counter);

	/* Disable overflow interrupts */
	reg_pmintenclr_el1_write(counter_mask);

	/* Disable event counter */
	reg_pmcntenclr_el0_write(counter_mask);
	reg_pmcr_el0_write(0);
}

static uint64_t
armv8_pmu_estimate_freq(void)
{
	uint64_t cpufreq = curcpu()->ci_data.cpu_cc_freq;
	uint64_t freq = 10000;

	counter_val = cpufreq / freq;
	if (counter_val == 0)
		counter_val = 4000000000ULL / freq;

	return freq;
}

static uint32_t
armv8_pmu_ident(void)
{
	return TPROF_IDENT_ARMV8_GENERIC;
}

static int
armv8_pmu_start(const tprof_param_t *param)
{
	/* PMCR.N of 0 means that no event counters are available */
	if (__SHIFTOUT(reg_pmcr_el0_read(), PMCR_N) == 0) {
		return EINVAL;
	}

	if (!armv8_pmu_event_implemented(param->p_event)) {
		printf("%s: event %#" PRIx64 " not implemented on this CPU\n",
		    __func__, param->p_event);
		return EINVAL;
	}

	counter_reset_val = -counter_val + 1;

	armv8_pmu_param = *param;
	uint64_t xc = xc_broadcast(0, armv8_pmu_start_cpu, NULL, NULL);
	xc_wait(xc);

	return 0;
}

static void
armv8_pmu_stop(const tprof_param_t *param)
{
	uint64_t xc;

	xc = xc_broadcast(0, armv8_pmu_stop_cpu, NULL, NULL);
	xc_wait(xc);
}

static const tprof_backend_ops_t tprof_armv8_pmu_ops = {
	.tbo_estimate_freq = armv8_pmu_estimate_freq,
	.tbo_ident = armv8_pmu_ident,
	.tbo_start = armv8_pmu_start,
	.tbo_stop = armv8_pmu_stop,
};

int
armv8_pmu_intr(void *priv)
{
	const struct trapframe * const tf = priv;
	const uint32_t counter_mask = __BIT(armv8_pmu_counter);
	tprof_frame_info_t tfi;

	const uint32_t pmovs = reg_pmovsset_el0_read();
	if ((pmovs & counter_mask) != 0) {
		tfi.tfi_pc = tf->tf_pc;
		tfi.tfi_inkernel = tfi.tfi_pc >= VM_MIN_KERNEL_ADDRESS &&
		    tfi.tfi_pc < VM_MAX_KERNEL_ADDRESS;
		tprof_sample(NULL, &tfi);

		armv8_pmu_set_pmevcntr(armv8_pmu_counter, counter_reset_val);
	}
	reg_pmovsclr_el0_write(pmovs);

	return 1;
}

static void
armv8_pmu_init_cpu(void *arg1, void *arg2)
{
	/* Disable EL0 access to performance monitors */
	reg_pmuserenr_el0_write(0);

	/* Disable interrupts */
	reg_pmintenclr_el1_write(~0U);

	/* Disable event counters */
	reg_pmcntenclr_el0_write(PMCNTEN_P);
}

bool
armv8_pmu_detect(void)
{
	const uint64_t dfr0 = reg_id_aa64dfr0_el1_read();
	const u_int pmuver = __SHIFTOUT(dfr0, ID_AA64DFR0_EL1_PMUVER);

	return pmuver != ID_AA64DFR0_EL1_PMUVER_NONE &&
	       pmuver != ID_AA64DFR0_EL1_PMUVER_IMPL;
}

int
armv8_pmu_init(void)
{
	KASSERT(armv8_pmu_detect());

	uint64_t xc = xc_broadcast(0, armv8_pmu_init_cpu, NULL, NULL);
	xc_wait(xc);

	return tprof_backend_register("tprof_armv8", &tprof_armv8_pmu_ops,
	    TPROF_BACKEND_VERSION);
}
