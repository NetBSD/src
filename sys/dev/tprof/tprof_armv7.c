/* $NetBSD: tprof_armv7.c,v 1.2.2.2 2018/07/28 04:37:57 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tprof_armv7.c,v 1.2.2.2 2018/07/28 04:37:57 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <arm/armreg.h>
#include <arm/locore.h>

#include <dev/tprof/tprof_armv7.h>

#define	PMCR_D			__BIT(3)
#define	PMCR_E			__BIT(0)

#define	PMEVTYPER_P		__BIT(31)
#define	PMEVTYPER_U		__BIT(30)
#define	PMEVTYPER_EVTCOUNT	__BITS(7,0)

static tprof_param_t armv7_pmu_param;
static const u_int armv7_pmu_counter = 1;
static uint32_t counter_val;
static uint32_t counter_reset_val;

static bool
armv7_pmu_event_implemented(uint16_t event)
{
	uint32_t eid[2];

	if (event >= 64)
		return false;

	eid[0] = armreg_pmceid0_read();
	eid[1] = armreg_pmceid1_read();

	const u_int idx = event / 32;
	const u_int bit = event % 32;

	if (eid[idx] & __BIT(bit))
		return true;

	return false;
}

static void
armv7_pmu_set_pmevtyper(u_int counter, uint64_t val)
{
	armreg_pmselr_write(counter);
	arm_isb();
	armreg_pmxevtyper_write(val);
}

static void
armv7_pmu_set_pmevcntr(u_int counter, uint32_t val)
{
	armreg_pmselr_write(counter);
	arm_isb();
	armreg_pmxevcntr_write(val);
}

static void
armv7_pmu_start_cpu(void *arg1, void *arg2)
{
	const uint32_t counter_mask = __BIT(armv7_pmu_counter);
	uint64_t pmcr, pmevtyper;

	/* Enable performance monitor */
	pmcr = armreg_pmcr_read();
	pmcr |= PMCR_E;
	armreg_pmcr_write(pmcr);

	/* Disable event counter */
	armreg_pmcntenclr_write(counter_mask);

	/* Configure event counter */
	pmevtyper = __SHIFTIN(armv7_pmu_param.p_event, PMEVTYPER_EVTCOUNT);
	if (!ISSET(armv7_pmu_param.p_flags, TPROF_PARAM_USER))
		pmevtyper |= PMEVTYPER_U;
	if (!ISSET(armv7_pmu_param.p_flags, TPROF_PARAM_KERN))
		pmevtyper |= PMEVTYPER_P;

	armv7_pmu_set_pmevtyper(armv7_pmu_counter, pmevtyper);

	/* Enable overflow interrupts */
	armreg_pmintenset_write(counter_mask);

	/* Clear overflow flag */
	armreg_pmovsr_write(counter_mask);

	/* Initialize event counter value */
	armv7_pmu_set_pmevcntr(armv7_pmu_counter, counter_reset_val);

	/* Enable event counter */
	armreg_pmcntenset_write(counter_mask);
}

static void
armv7_pmu_stop_cpu(void *arg1, void *arg2)
{
	const uint32_t counter_mask = __BIT(armv7_pmu_counter);
	uint32_t pmcr;

	/* Disable overflow interrupts */
	armreg_pmintenclr_write(counter_mask);

	/* Disable event counter */
	armreg_pmcntenclr_write(counter_mask);

	/* Disable performance monitor */
	pmcr = armreg_pmcr_read();
	pmcr &= ~PMCR_E;
	armreg_pmcr_write(pmcr);
}

static uint64_t
armv7_pmu_estimate_freq(void)
{
	uint64_t cpufreq = curcpu()->ci_data.cpu_cc_freq;
	uint64_t freq = 10000;
	uint32_t pmcr;

	counter_val = cpufreq / freq;
	if (counter_val == 0)
		counter_val = 4000000000ULL / freq;

	pmcr = armreg_pmcr_read();
	if (pmcr & PMCR_D)
		counter_val /= 64;

	return freq;
}

static uint32_t
armv7_pmu_ident(void)
{
	return TPROF_IDENT_ARMV7_GENERIC;
}

static int
armv7_pmu_start(const tprof_param_t *param)
{
	uint64_t xc;

	if (!armv7_pmu_event_implemented(param->p_event)) {
		printf("%s: event 0x%#llx not implemented on this CPU\n",
		    __func__, param->p_event);
		return EINVAL;
	}

	counter_reset_val = -counter_val + 1;

	armv7_pmu_param = *param;
	xc = xc_broadcast(0, armv7_pmu_start_cpu, NULL, NULL);
	xc_wait(xc);

	return 0;
}

static void
armv7_pmu_stop(const tprof_param_t *param)
{
	uint64_t xc;

	xc = xc_broadcast(0, armv7_pmu_stop_cpu, NULL, NULL);
	xc_wait(xc);
}

static const tprof_backend_ops_t tprof_armv7_pmu_ops = {
	.tbo_estimate_freq = armv7_pmu_estimate_freq,
	.tbo_ident = armv7_pmu_ident,
	.tbo_start = armv7_pmu_start,
	.tbo_stop = armv7_pmu_stop,
};

int
armv7_pmu_intr(void *priv)
{
	const struct trapframe * const tf = priv;
	const uint32_t counter_mask = __BIT(armv7_pmu_counter);
	tprof_frame_info_t tfi;

	const uint32_t pmovsr = armreg_pmovsr_read();
	if ((pmovsr & counter_mask) != 0) {
		tfi.tfi_pc = tf->tf_pc;
		tfi.tfi_inkernel = tfi.tfi_pc >= VM_MIN_KERNEL_ADDRESS &&
		    tfi.tfi_pc < VM_MAX_KERNEL_ADDRESS;
		tprof_sample(NULL, &tfi);

		armv7_pmu_set_pmevcntr(armv7_pmu_counter, counter_reset_val);
	}
	armreg_pmovsr_write(pmovsr);

	return 1;
}

int
armv7_pmu_init(void)
{
	/* Disable user mode access to performance monitors */
	armreg_pmuserenr_write(0);

	/* Disable interrupts */
	armreg_pmintenclr_write(~0U);

	/* Disable counters */
	armreg_pmcntenclr_write(~0U);

	/* Disable performance monitor */
	armreg_pmcr_write(0);

	return tprof_backend_register("tprof_armv7", &tprof_armv7_pmu_ops,
	    TPROF_BACKEND_VERSION);
}
