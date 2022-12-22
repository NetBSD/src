/* $NetBSD: tprof_armv7.c,v 1.12 2022/12/22 06:59:32 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tprof_armv7.c,v 1.12 2022/12/22 06:59:32 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/percpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <arm/armreg.h>
#include <arm/locore.h>

#include <dev/tprof/tprof_armv7.h>

static uint16_t cortexa9_events[] = {
	0x40, 0x41, 0x42,
	0x50, 0x51,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x6e,
	0x70, 0x71, 0x72, 0x73, 0x74,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
	0x8a, 0x8b,
	0x90, 0x91, 0x92, 0x93,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5
};

static bool
armv7_pmu_event_implemented(uint16_t event)
{
	if (CPU_ID_CORTEX_A9_P(curcpu()->ci_midr)) {
		/* Cortex-A9 with PMUv1 lacks PMCEID0/1 */
		u_int n;

		/* Events specific to the Cortex-A9 */
		for (n = 0; n < __arraycount(cortexa9_events); n++) {
			if (cortexa9_events[n] == event) {
				return true;
			}
		}
		/* Supported architectural events */
		if (event != 0x08 && event != 0x0e && event < 0x1e) {
			return true;
		}
	} else {
		/* PMUv2 */
		uint32_t eid[2];

		if (event >= 64) {
			return false;
		}

		eid[0] = armreg_pmceid0_read();
		eid[1] = armreg_pmceid1_read();

		const u_int idx = event / 32;
		const u_int bit = event % 32;

		if (eid[idx] & __BIT(bit)) {
			return true;
		}
	}

	return false;
}

static void
armv7_pmu_set_pmevtyper(u_int counter, uint64_t val)
{
	armreg_pmselr_write(counter);
	isb();
	armreg_pmxevtyper_write(val);
}

static inline void
armv7_pmu_set_pmevcntr(u_int counter, uint32_t val)
{
	armreg_pmselr_write(counter);
	isb();
	armreg_pmxevcntr_write(val);
}

static inline uint64_t
armv7_pmu_get_pmevcntr(u_int counter)
{
	armreg_pmselr_write(counter);
	isb();
	return armreg_pmxevcntr_read();
}

/* read and write at once */
static inline uint64_t
armv7_pmu_getset_pmevcntr(u_int counter, uint64_t val)
{
	uint64_t c;

	armreg_pmselr_write(counter);
	isb();
	c = armreg_pmxevcntr_read();
	armreg_pmxevcntr_write(val);
	return c;
}

static uint32_t
armv7_pmu_ncounters(void)
{
	return __SHIFTOUT(armreg_pmcr_read(), PMCR_N);
}

static u_int
armv7_pmu_counter_bitwidth(u_int counter)
{
	return 32;
}

static uint64_t
armv7_pmu_counter_estimate_freq(u_int counter)
{
	uint64_t cpufreq = curcpu()->ci_data.cpu_cc_freq;

	if (ISSET(armreg_pmcr_read(), PMCR_D))
		cpufreq /= 64;
	return cpufreq;
}

static int
armv7_pmu_valid_event(u_int counter, const tprof_param_t *param)
{
	if (!armv7_pmu_event_implemented(param->p_event)) {
		printf("%s: event %#" PRIx64 " not implemented on this CPU\n",
		    __func__, param->p_event);
		return EINVAL;
	}
	return 0;
}

static void
armv7_pmu_configure_event(u_int counter, const tprof_param_t *param)
{
	/* Disable event counter */
	armreg_pmcntenclr_write(__BIT(counter) & PMCNTEN_P);

	/* Disable overflow interrupts */
	armreg_pmintenclr_write(__BIT(counter) & PMINTEN_P);

	/* Configure event counter */
	uint32_t pmevtyper = __SHIFTIN(param->p_event, PMEVTYPER_EVTCOUNT);
	if (!ISSET(param->p_flags, TPROF_PARAM_USER))
		pmevtyper |= PMEVTYPER_U;
	if (!ISSET(param->p_flags, TPROF_PARAM_KERN))
		pmevtyper |= PMEVTYPER_P;
	armv7_pmu_set_pmevtyper(counter, pmevtyper);

	/*
	 * Enable overflow interrupts.
	 * Whether profiled or not, the counter width of armv7 is 32 bits,
	 * so overflow handling is required anyway.
	 */
	armreg_pmintenset_write(__BIT(counter) & PMINTEN_P);

	/* Clear overflow flag */
	armreg_pmovsr_write(__BIT(counter) & PMOVS_P);

	/* reset the counter */
	armv7_pmu_set_pmevcntr(counter, param->p_value);
}

static void
armv7_pmu_start(tprof_countermask_t runmask)
{
	/* Enable event counters */
	armreg_pmcntenset_write(runmask & PMCNTEN_P);

	/*
	 * PMCR.E is shared with PMCCNTR and event counters.
	 * It is set here in case PMCCNTR is not used in the system.
	 */
	armreg_pmcr_write(armreg_pmcr_read() | PMCR_E);
}

static void
armv7_pmu_stop(tprof_countermask_t stopmask)
{
	/* Disable event counter */
	armreg_pmcntenclr_write(stopmask & PMCNTEN_P);
}

/* XXX: argument of armv8_pmu_intr() */
extern struct tprof_backend *tprof_backend;
static void *pmu_intr_arg;

int
armv7_pmu_intr(void *priv)
{
	const struct trapframe * const tf = priv;
	tprof_backend_softc_t *sc = pmu_intr_arg;
	tprof_frame_info_t tfi;
	int bit;
	const uint32_t pmovs = armreg_pmovsr_read();

	uint64_t *counters_offset =
	    percpu_getptr_remote(sc->sc_ctr_offset_percpu, curcpu());
	uint32_t mask = pmovs;
	while ((bit = ffs(mask)) != 0) {
		bit--;
		CLR(mask, __BIT(bit));

		if (ISSET(sc->sc_ctr_prof_mask, __BIT(bit))) {
			/* account for the counter, and reset */
			uint64_t ctr = armv7_pmu_getset_pmevcntr(bit,
			    sc->sc_count[bit].ctr_counter_reset_val);
			counters_offset[bit] +=
			    sc->sc_count[bit].ctr_counter_val + ctr;

			/* record a sample */
			tfi.tfi_pc = tf->tf_pc;
			tfi.tfi_counter = bit;
			tfi.tfi_inkernel =
			    tfi.tfi_pc >= VM_MIN_KERNEL_ADDRESS &&
			    tfi.tfi_pc < VM_MAX_KERNEL_ADDRESS;
			tprof_sample(NULL, &tfi);
		} else if (ISSET(sc->sc_ctr_ovf_mask, __BIT(bit))) {
			/* counter has overflowed */
			counters_offset[bit] += __BIT(32);
		}
	}
	armreg_pmovsr_write(pmovs);

	return 1;
}

static uint32_t
armv7_pmu_ident(void)
{
	return TPROF_IDENT_ARMV7_GENERIC;
}

static const tprof_backend_ops_t tprof_armv7_pmu_ops = {
	.tbo_ident = armv7_pmu_ident,
	.tbo_ncounters = armv7_pmu_ncounters,
	.tbo_counter_bitwidth = armv7_pmu_counter_bitwidth,
	.tbo_counter_read = armv7_pmu_get_pmevcntr,
	.tbo_counter_estimate_freq = armv7_pmu_counter_estimate_freq,
	.tbo_valid_event = armv7_pmu_valid_event,
	.tbo_configure_event = armv7_pmu_configure_event,
	.tbo_start = armv7_pmu_start,
	.tbo_stop = armv7_pmu_stop,
	.tbo_establish = NULL,
	.tbo_disestablish = NULL,
};

static void
armv7_pmu_init_cpu(void *arg1, void *arg2)
{
	/* Disable user mode access to performance monitors */
	armreg_pmuserenr_write(0);

	/* Disable interrupts */
	armreg_pmintenclr_write(PMINTEN_P);

	/* Disable counters */
	armreg_pmcntenclr_write(PMCNTEN_P);
}

int
armv7_pmu_init(void)
{
	int error, ncounters;

	ncounters = armv7_pmu_ncounters();
	if (ncounters == 0)
		return ENOTSUP;

	uint64_t xc = xc_broadcast(0, armv7_pmu_init_cpu, NULL, NULL);
	xc_wait(xc);

	error = tprof_backend_register("tprof_armv7", &tprof_armv7_pmu_ops,
	    TPROF_BACKEND_VERSION);
	if (error == 0) {
		/* XXX: for argument of armv7_pmu_intr() */
		pmu_intr_arg = tprof_backend;
	}

	return error;
}
