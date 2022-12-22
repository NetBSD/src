/* $NetBSD: tprof_armv8.c,v 1.19 2022/12/22 06:59:32 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tprof_armv8.c,v 1.19 2022/12/22 06:59:32 ryo Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/percpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <dev/tprof/tprof_armv8.h>

static u_int counter_bitwidth;

/*
 * armv8 can handle up to 31 event counters,
 * PMCR_EL0.N counters are actually available.
 */

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

static inline void
armv8_pmu_set_pmevcntr(u_int counter, uint64_t val)
{
	reg_pmselr_el0_write(counter);
	isb();
	reg_pmxevcntr_el0_write(val);
}

static inline uint64_t
armv8_pmu_get_pmevcntr(u_int counter)
{
	reg_pmselr_el0_write(counter);
	isb();
	return reg_pmxevcntr_el0_read();
}

/* read and write at once */
static inline uint64_t
armv8_pmu_getset_pmevcntr(u_int counter, uint64_t val)
{
	uint64_t c;

	reg_pmselr_el0_write(counter);
	isb();
	c = reg_pmxevcntr_el0_read();
	reg_pmxevcntr_el0_write(val);
	return c;
}

static uint32_t
armv8_pmu_ncounters(void)
{
	return __SHIFTOUT(reg_pmcr_el0_read(), PMCR_N);
}

static u_int
armv8_pmu_counter_bitwidth(u_int counter)
{
	return counter_bitwidth;
}

static uint64_t
armv8_pmu_counter_estimate_freq(u_int counter)
{
	return curcpu()->ci_data.cpu_cc_freq;
}

static int
armv8_pmu_valid_event(u_int counter, const tprof_param_t *param)
{
	if (!armv8_pmu_event_implemented(param->p_event)) {
		printf("%s: event %#" PRIx64 " not implemented on this CPU\n",
		    __func__, param->p_event);
		return EINVAL;
	}
	return 0;
}

static void
armv8_pmu_configure_event(u_int counter, const tprof_param_t *param)
{
	/* Disable event counter */
	reg_pmcntenclr_el0_write(__BIT(counter) & PMCNTEN_P);

	/* Disable overflow interrupts */
	reg_pmintenclr_el1_write(__BIT(counter) & PMINTEN_P);

	/* Configure event counter */
	uint64_t pmevtyper = __SHIFTIN(param->p_event, PMEVTYPER_EVTCOUNT);
	if (!ISSET(param->p_flags, TPROF_PARAM_USER))
		pmevtyper |= PMEVTYPER_U;
	if (!ISSET(param->p_flags, TPROF_PARAM_KERN))
		pmevtyper |= PMEVTYPER_P;
	armv8_pmu_set_pmevtyper(counter, pmevtyper);

	if (ISSET(param->p_flags, TPROF_PARAM_PROFILE) ||
	    counter_bitwidth != 64) {
		/* Enable overflow interrupts */
		reg_pmintenset_el1_write(__BIT(counter) & PMINTEN_P);
	}

	/* Clear overflow flag */
	reg_pmovsclr_el0_write(__BIT(counter) & PMOVS_P);

	/* reset the counter */
	armv8_pmu_set_pmevcntr(counter, param->p_value);
}

static void
armv8_pmu_start(tprof_countermask_t runmask)
{
	/* Enable event counters */
	reg_pmcntenset_el0_write(runmask & PMCNTEN_P);

	/*
	 * PMCR.E is shared with PMCCNTR_EL0 and event counters.
	 * It is set here in case PMCCNTR_EL0 is not used in the system.
	 */
	reg_pmcr_el0_write(reg_pmcr_el0_read() | PMCR_E);
}

static void
armv8_pmu_stop(tprof_countermask_t stopmask)
{
	/* Disable event counter */
	reg_pmcntenclr_el0_write(stopmask & PMCNTEN_P);
}

/* XXX: argument of armv8_pmu_intr() */
extern struct tprof_backend *tprof_backend;
static void *pmu_intr_arg;

int
armv8_pmu_intr(void *priv)
{
	const struct trapframe * const tf = priv;
	tprof_backend_softc_t *sc = pmu_intr_arg;
	tprof_frame_info_t tfi;
	int bit;
	const uint32_t pmovs = reg_pmovsset_el0_read();

	uint64_t *counters_offset =
	    percpu_getptr_remote(sc->sc_ctr_offset_percpu, curcpu());
	uint32_t mask = pmovs;
	while ((bit = ffs(mask)) != 0) {
		bit--;
		CLR(mask, __BIT(bit));

		if (ISSET(sc->sc_ctr_prof_mask, __BIT(bit))) {
			/* account for the counter, and reset */
			uint64_t ctr = armv8_pmu_getset_pmevcntr(bit,
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
	reg_pmovsclr_el0_write(pmovs);

	return 1;
}

static uint32_t
armv8_pmu_ident(void)
{
	return TPROF_IDENT_ARMV8_GENERIC;
}

static const tprof_backend_ops_t tprof_armv8_pmu_ops = {
	.tbo_ident = armv8_pmu_ident,
	.tbo_ncounters = armv8_pmu_ncounters,
	.tbo_counter_bitwidth = armv8_pmu_counter_bitwidth,
	.tbo_counter_read = armv8_pmu_get_pmevcntr,
	.tbo_counter_estimate_freq = armv8_pmu_counter_estimate_freq,
	.tbo_valid_event = armv8_pmu_valid_event,
	.tbo_configure_event = armv8_pmu_configure_event,
	.tbo_start = armv8_pmu_start,
	.tbo_stop = armv8_pmu_stop,
	.tbo_establish = NULL,
	.tbo_disestablish = NULL,
};

static void
armv8_pmu_init_cpu(void *arg1, void *arg2)
{
	/* Disable EL0 access to performance monitors */
	reg_pmuserenr_el0_write(0);

	/* Disable interrupts */
	reg_pmintenclr_el1_write(PMINTEN_P);

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
	int error, ncounters;

	KASSERT(armv8_pmu_detect());

	ncounters = armv8_pmu_ncounters();
	if (ncounters == 0)
		return ENOTSUP;

	/* Is 64bit event counter available? */
	const uint64_t dfr0 = reg_id_aa64dfr0_el1_read();
	const u_int pmuver = __SHIFTOUT(dfr0, ID_AA64DFR0_EL1_PMUVER);
	if (pmuver >= ID_AA64DFR0_EL1_PMUVER_V3P5 &&
	    ISSET(reg_pmcr_el0_read(), PMCR_LP))
		counter_bitwidth = 64;
	else
		counter_bitwidth = 32;

	uint64_t xc = xc_broadcast(0, armv8_pmu_init_cpu, NULL, NULL);
	xc_wait(xc);

	error = tprof_backend_register("tprof_armv8", &tprof_armv8_pmu_ops,
	    TPROF_BACKEND_VERSION);
	if (error == 0) {
		/* XXX: for argument of armv8_pmu_intr() */
		pmu_intr_arg = tprof_backend;
	}

	return error;
}
