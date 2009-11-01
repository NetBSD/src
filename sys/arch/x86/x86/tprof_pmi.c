/*	$NetBSD: tprof_pmi.c,v 1.3.12.2 2009/11/01 13:58:18 jym Exp $	*/

/*-
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
__KERNEL_RCSID(0, "$NetBSD: tprof_pmi.c,v 1.3.12.2 2009/11/01 13:58:18 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <sys/cpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <x86/tprof.h>
#include <x86/nmi.h>

#include <machine/cpufunc.h>
#include <machine/cputypes.h>	/* CPUVENDER_* */
#include <machine/cpuvar.h>	/* cpu_vendor */
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#define	ESCR_T1_USR		__BIT(0)
#define	ESCR_T1_OS		__BIT(1)
#define	ESCR_T0_USR		__BIT(2)
#define	ESCR_T0_OS		__BIT(3)
#define	ESCR_TAG_ENABLE		__BIT(4)
#define	ESCR_TAG_VALUE		__BITS(5, 8)
#define	ESCR_EVENT_MASK		__BITS(9, 24)
#define	ESCR_EVENT_SELECT	__BITS(25, 30)

#define	CCCR_ENABLE		__BIT(12)
#define	CCCR_ESCR_SELECT	__BITS(13, 15)
#define	CCCR_MUST_BE_SET	__BITS(16, 17)
#define	CCCR_COMPARE		__BIT(18)
#define	CCCR_COMPLEMENT		__BIT(19)
#define	CCCR_THRESHOLD		__BITS(20, 23)
#define	CCCR_EDGE		__BIT(24)
#define	CCCR_FORCE_OVF		__BIT(25)
#define	CCCR_OVF_PMI_T0		__BIT(26)
#define	CCCR_OVF_PMI_T1		__BIT(27)
#define	CCCR_CASCADE		__BIT(30)
#define	CCCR_OVF		__BIT(31)

struct msrs {
	u_int msr_cccr;
	u_int msr_escr;
	u_int msr_counter;
};

/*
 * parameters (see 253669.pdf Table A-6)
 *
 * XXX should not hardcode
 */

static const struct msrs msrs[] = {
	{
		.msr_cccr = 0x360,	/* MSR_BPU_CCCR0 */
		.msr_escr = 0x3a2,	/* MSR_FSB_ESCR0 */
		.msr_counter = 0x300,	/* MSR_BPU_COUNTER0 */
	},
	{
		.msr_cccr = 0x362,	/* MSR_BPU_CCCR2 */
		.msr_escr = 0x3a3,	/* MSR_FSB_ESCR1 */
		.msr_counter = 0x302,	/* MSR_BPU_COUNTER2 */
	},
};
static const u_int cccr_escr_select = 0x6;	/* MSR_FSB_ESCR? */
static const u_int escr_event_select = 0x13;	/* global_power_events */
static const u_int escr_event_mask = 0x1;	/* running */

static uint64_t counter_val = 5000000;
static uint64_t counter_reset_val;
static uint32_t tprof_pmi_lapic_saved[MAXCPUS];

static nmi_handler_t *tprof_pmi_nmi_handle;
static tprof_backend_cookie_t *tprof_cookie;

static void
tprof_pmi_start_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();
	const struct msrs *msr;
	uint64_t cccr;
	uint64_t escr;

	if (ci->ci_smtid >= 2) {
		printf("%s: ignoring %s smtid=%u",
		    __func__, device_xname(ci->ci_dev), ci->ci_smtid);
		return;
	}
	msr = &msrs[ci->ci_smtid];
	escr = __SHIFTIN(escr_event_mask, ESCR_EVENT_MASK) |
	    __SHIFTIN(escr_event_select, ESCR_EVENT_SELECT);
	cccr = CCCR_ENABLE | __SHIFTIN(cccr_escr_select, CCCR_ESCR_SELECT) |
	    CCCR_MUST_BE_SET;
	if (ci->ci_smtid == 0) {
		escr |= ESCR_T0_OS;
		cccr |= CCCR_OVF_PMI_T0;
	} else {
		escr |= ESCR_T1_OS;
		cccr |= CCCR_OVF_PMI_T1;
	}

	wrmsr(msr->msr_counter, counter_reset_val);
	wrmsr(msr->msr_escr, escr);
	wrmsr(msr->msr_cccr, cccr);
	tprof_pmi_lapic_saved[cpu_index(ci)] = i82489_readreg(LAPIC_PCINT);
	i82489_writereg(LAPIC_PCINT, LAPIC_DLMODE_NMI);
}

static void
tprof_pmi_stop_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();
	const struct msrs *msr;

	if (ci->ci_smtid >= 2) {
		printf("%s: ignoring %s smtid=%u",
		    __func__, device_xname(ci->ci_dev), ci->ci_smtid);
		return;
	}
	msr = &msrs[ci->ci_smtid];

	wrmsr(msr->msr_escr, 0);
	wrmsr(msr->msr_cccr, 0);
	i82489_writereg(LAPIC_PCINT, tprof_pmi_lapic_saved[cpu_index(ci)]);
}

static int
tprof_pmi_nmi(const struct trapframe *tf, void *dummy)
{
	struct cpu_info * const ci = curcpu();
	const struct msrs *msr;
	uint32_t pcint;
	uint64_t cccr;
	tprof_frame_info_t tfi;

	KASSERT(dummy == NULL);

	if (ci->ci_smtid >= 2) {
		/* not ours */
		return 0;
	}
	msr = &msrs[ci->ci_smtid];

	/* check if it's for us */
	cccr = rdmsr(msr->msr_cccr);
	if ((cccr & CCCR_OVF) == 0) {
		/* not ours */
		return 0;
	}

	/* record a sample */
#if defined(__x86_64__)
	tfi.tfi_pc = tf->tf_rip;
#else /* defined(__x86_64__) */
	tfi.tfi_pc = tf->tf_eip;
#endif /* defined(__x86_64__) */
	tprof_sample(tprof_cookie, &tfi);

	/* reset counter */
	wrmsr(msr->msr_counter, counter_reset_val);
	wrmsr(msr->msr_cccr, cccr & ~CCCR_OVF);

	/* unmask PMI */
	pcint = i82489_readreg(LAPIC_PCINT);
	KASSERT((pcint & LAPIC_LVT_MASKED) != 0);
	i82489_writereg(LAPIC_PCINT, pcint & ~LAPIC_LVT_MASKED);

	return 1;
}

static uint64_t
tprof_pmi_estimate_freq(void)
{
	uint64_t cpufreq = curcpu()->ci_data.cpu_cc_freq;
	uint64_t freq = 10000;

	counter_val = cpufreq / freq;
	if (counter_val == 0) {
		counter_val = UINT64_C(4000000000) / freq;
		return freq;
	}
	return freq;
}

static int
tprof_pmi_start(tprof_backend_cookie_t *cookie)
{
	struct cpu_info * const ci = curcpu();
	uint64_t xc;

	if (!(cpu_vendor == CPUVENDOR_INTEL &&
	    CPUID2FAMILY(ci->ci_signature) == 15)) {
		return ENOTSUP;
	}

	KASSERT(tprof_pmi_nmi_handle == NULL);
	tprof_pmi_nmi_handle = nmi_establish(tprof_pmi_nmi, NULL);

	counter_reset_val = - counter_val + 1;
	xc = xc_broadcast(0, tprof_pmi_start_cpu, NULL, NULL);
	xc_wait(xc);

	KASSERT(tprof_cookie == NULL);
	tprof_cookie = cookie;

	return 0;
}

static void
tprof_pmi_stop(tprof_backend_cookie_t *cookie)
{
	uint64_t xc;

	xc = xc_broadcast(0, tprof_pmi_stop_cpu, NULL, NULL);
	xc_wait(xc);

	KASSERT(tprof_pmi_nmi_handle != NULL);
	KASSERT(tprof_cookie == cookie);
	nmi_disestablish(tprof_pmi_nmi_handle);
	tprof_pmi_nmi_handle = NULL;
	tprof_cookie = NULL;
}

static const tprof_backend_ops_t tprof_pmi_ops = {
	.tbo_estimate_freq = tprof_pmi_estimate_freq,
	.tbo_start = tprof_pmi_start,
	.tbo_stop = tprof_pmi_stop,
};

MODULE(MODULE_CLASS_DRIVER, tprof_pmi, "tprof");

static int
tprof_pmi_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return tprof_backend_register("tprof_pmi", &tprof_pmi_ops,
		    TPROF_BACKEND_VERSION);

	case MODULE_CMD_FINI:
		return tprof_backend_unregister("tprof_pmi");

	default:
		return ENOTTY;
	}
}
