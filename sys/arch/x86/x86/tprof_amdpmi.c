/*	$NetBSD: tprof_amdpmi.c,v 1.3.14.1 2014/08/20 00:03:29 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: tprof_amdpmi.c,v 1.3.14.1 2014/08/20 00:03:29 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>

#include <sys/cpu.h>
#include <sys/xcall.h>

#include <dev/tprof/tprof.h>

#include <uvm/uvm.h>		/* VM_MIN_KERNEL_ADDRESS */

#include <x86/tprof.h>
#include <x86/nmi.h>

#include <machine/cpufunc.h>
#include <machine/cputypes.h>	/* CPUVENDER_* */
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
 * parameters
 *
 * XXX should not hardcode
 *
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/32559.pdf
 * http://developer.amd.com/Assets/Basic_Performance_Measurements.pdf
 */

static uint32_t event = 0x76;	/* CPU Clocks not Halted */
static uint32_t unit = 0;
static int ctrno = 0;

static uint64_t counter_val = 5000000;
static uint64_t counter_reset_val;
static uint32_t tprof_amdpmi_lapic_saved[MAXCPUS];

static nmi_handler_t *tprof_amdpmi_nmi_handle;
static tprof_backend_cookie_t *tprof_cookie;

static void
tprof_amdpmi_start_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();
	uint64_t pesr;
	uint64_t event_lo;
	uint64_t event_hi;

	event_hi = event >> 8;
	event_lo = event & 0xff;
	pesr = PESR_USR | PESR_OS | PESR_INT |
	    __SHIFTIN(event_lo, PESR_EVENT_MASK_LO) |
	    __SHIFTIN(event_hi, PESR_EVENT_MASK_HI) |
	    __SHIFTIN(0, PESR_COUNTER_MASK) |
	    __SHIFTIN(unit, PESR_UNIT_MASK);

	wrmsr(PERFCTR(ctrno), counter_reset_val);
	wrmsr(PERFEVTSEL(ctrno), pesr);

	tprof_amdpmi_lapic_saved[cpu_index(ci)] = i82489_readreg(LAPIC_PCINT);
	i82489_writereg(LAPIC_PCINT, LAPIC_DLMODE_NMI);

	wrmsr(PERFEVTSEL(ctrno), pesr | PESR_EN);
}

static void
tprof_amdpmi_stop_cpu(void *arg1, void *arg2)
{
	struct cpu_info * const ci = curcpu();

	wrmsr(PERFEVTSEL(ctrno), 0);

	i82489_writereg(LAPIC_PCINT, tprof_amdpmi_lapic_saved[cpu_index(ci)]);
}

static int
tprof_amdpmi_nmi(const struct trapframe *tf, void *dummy)
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
#else /* defined(__x86_64__) */
	tfi.tfi_pc = tf->tf_eip;
#endif /* defined(__x86_64__) */
	tfi.tfi_inkernel = tfi.tfi_pc >= VM_MIN_KERNEL_ADDRESS;
	tprof_sample(tprof_cookie, &tfi);

	/* reset counter */
	wrmsr(PERFCTR(ctrno), counter_reset_val);

	return 1;
}

static uint64_t
tprof_amdpmi_estimate_freq(void)
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
tprof_amdpmi_start(tprof_backend_cookie_t *cookie)
{
	struct cpu_info * const ci = curcpu();
	uint64_t xc;

	if (!(cpu_vendor == CPUVENDOR_AMD) ||
	    CPUID_TO_FAMILY(ci->ci_signature) != 0xf) { /* XXX */
		return ENOTSUP;
	}

	KASSERT(tprof_amdpmi_nmi_handle == NULL);
	tprof_amdpmi_nmi_handle = nmi_establish(tprof_amdpmi_nmi, NULL);

	counter_reset_val = - counter_val + 1;
	xc = xc_broadcast(0, tprof_amdpmi_start_cpu, NULL, NULL);
	xc_wait(xc);

	KASSERT(tprof_cookie == NULL);
	tprof_cookie = cookie;

	return 0;
}

static void
tprof_amdpmi_stop(tprof_backend_cookie_t *cookie)
{
	uint64_t xc;

	xc = xc_broadcast(0, tprof_amdpmi_stop_cpu, NULL, NULL);
	xc_wait(xc);

	KASSERT(tprof_amdpmi_nmi_handle != NULL);
	KASSERT(tprof_cookie == cookie);
	nmi_disestablish(tprof_amdpmi_nmi_handle);
	tprof_amdpmi_nmi_handle = NULL;
	tprof_cookie = NULL;
}

static const tprof_backend_ops_t tprof_amdpmi_ops = {
	.tbo_estimate_freq = tprof_amdpmi_estimate_freq,
	.tbo_start = tprof_amdpmi_start,
	.tbo_stop = tprof_amdpmi_stop,
};

MODULE(MODULE_CLASS_DRIVER, tprof_amdpmi, "tprof");

static int
tprof_amdpmi_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return tprof_backend_register("tprof_amd", &tprof_amdpmi_ops,
		    TPROF_BACKEND_VERSION);

	case MODULE_CMD_FINI:
		return tprof_backend_unregister("tprof_amd");

	default:
		return ENOTTY;
	}
}
