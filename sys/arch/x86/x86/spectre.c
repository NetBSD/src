/*	$NetBSD: spectre.c,v 1.5.2.2 2018/03/30 06:20:13 pgoyette Exp $	*/

/*
 * Copyright (c) 2018 NetBSD Foundation, Inc.
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
 * Mitigations for the Spectre V2 CPU flaw.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spectre.c,v 1.5.2.2 2018/03/30 06:20:13 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/specialreg.h>
#include <machine/frameasm.h>

#include <x86/cputypes.h>

enum spec_mitigation {
	MITIGATION_NONE,
	MITIGATION_AMD_DIS_IND,
	MITIGATION_INTEL_IBRS
};

bool spec_mitigation_enabled __read_mostly = false;
static enum spec_mitigation mitigation_method = MITIGATION_NONE;

void speculation_barrier(struct lwp *, struct lwp *);

void
speculation_barrier(struct lwp *oldlwp, struct lwp *newlwp)
{
	if (!spec_mitigation_enabled)
		return;

	/*
	 * From kernel thread to kernel thread, no need for a barrier.
	 */
	if ((oldlwp != NULL && (oldlwp->l_flag & LW_SYSTEM)) &&
	    (newlwp->l_flag & LW_SYSTEM))
		return;

	switch (mitigation_method) {
	case MITIGATION_INTEL_IBRS:
		wrmsr(MSR_IA32_PRED_CMD, IA32_PRED_CMD_IBPB);
		break;
	default:
		/* nothing */
		break;
	}
}

static void
speculation_detect_method(void)
{
	struct cpu_info *ci = curcpu();
	u_int descs[4];

	if (cpu_vendor == CPUVENDOR_INTEL) {
		if (cpuid_level >= 7) {
			x86_cpuid(7, descs);
			if (descs[3] & CPUID_SEF_IBRS) {
				/* descs[3] = %edx */
				mitigation_method = MITIGATION_INTEL_IBRS;
				return;
			}
		}
		mitigation_method = MITIGATION_NONE;
	} else if (cpu_vendor == CPUVENDOR_AMD) {
		/*
		 * The AMD Family 10h manual documents the IC_CFG.DIS_IND bit.
		 * This bit disables the Indirect Branch Predictor.
		 *
		 * Families 12h and 16h are believed to have this bit too, but
		 * their manuals don't document it.
		 */
		switch (CPUID_TO_FAMILY(ci->ci_signature)) {
		case 0x10:
		case 0x12:
		case 0x16:
			mitigation_method = MITIGATION_AMD_DIS_IND;
			break;
		default:
			mitigation_method = MITIGATION_NONE;
			break;
		}
	} else {
		mitigation_method = MITIGATION_NONE;
	}
}

/* -------------------------------------------------------------------------- */

#ifdef __x86_64__
static volatile unsigned long ibrs_cpu_barrier1 __cacheline_aligned;
static volatile unsigned long ibrs_cpu_barrier2 __cacheline_aligned;

static void
ibrs_disable_hotpatch(void)
{
	extern uint8_t noibrs_enter, noibrs_enter_end;
	extern uint8_t noibrs_leave, noibrs_leave_end;
	u_long psl, cr0;
	uint8_t *bytes;
	size_t size;

	x86_patch_window_open(&psl, &cr0);

	bytes = &noibrs_enter;
	size = (size_t)&noibrs_enter_end - (size_t)&noibrs_enter;
	x86_hotpatch(HP_NAME_IBRS_ENTER, bytes, size);

	bytes = &noibrs_leave;
	size = (size_t)&noibrs_leave_end - (size_t)&noibrs_leave;
	x86_hotpatch(HP_NAME_IBRS_LEAVE, bytes, size);

	x86_patch_window_close(psl, cr0);
}

static void
ibrs_enable_hotpatch(void)
{
	extern uint8_t ibrs_enter, ibrs_enter_end;
	extern uint8_t ibrs_leave, ibrs_leave_end;
	u_long psl, cr0;
	uint8_t *bytes;
	size_t size;

	x86_patch_window_open(&psl, &cr0);

	bytes = &ibrs_enter;
	size = (size_t)&ibrs_enter_end - (size_t)&ibrs_enter;
	x86_hotpatch(HP_NAME_IBRS_ENTER, bytes, size);

	bytes = &ibrs_leave;
	size = (size_t)&ibrs_leave_end - (size_t)&ibrs_leave;
	x86_hotpatch(HP_NAME_IBRS_LEAVE, bytes, size);

	x86_patch_window_close(psl, cr0);
}

static void
ibrs_change_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	bool enabled = (bool)arg1;
	u_long psl;

	psl = x86_read_psl();
	x86_disable_intr();

	atomic_dec_ulong(&ibrs_cpu_barrier1);
	while (atomic_cas_ulong(&ibrs_cpu_barrier1, 0, 0) != 0) {
		x86_pause();
	}

	/* cpu0 is the one that does the hotpatch job */
	if (ci == &cpu_info_primary) {
		if (enabled) {
			ibrs_enable_hotpatch();
		} else {
			ibrs_disable_hotpatch();
		}
	}

	if (!enabled) {
		wrmsr(MSR_IA32_SPEC_CTRL, 0);
	}

	atomic_dec_ulong(&ibrs_cpu_barrier2);
	while (atomic_cas_ulong(&ibrs_cpu_barrier2, 0, 0) != 0) {
		x86_pause();
	}

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();

	x86_write_psl(psl);
}

static int
ibrs_change(bool enabled)
{
	struct cpu_info *ci = NULL;
	CPU_INFO_ITERATOR cii;
	uint64_t xc;

	mutex_enter(&cpu_lock);

	/*
	 * We expect all the CPUs to be online.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		struct schedstate_percpu *spc = &ci->ci_schedstate;
		if (spc->spc_flags & SPCF_OFFLINE) {
			printf("[!] cpu%d offline, IBRS not changed\n",
			    cpu_index(ci));
			mutex_exit(&cpu_lock);
			return EOPNOTSUPP;
		}
	}

	ibrs_cpu_barrier1 = ncpu;
	ibrs_cpu_barrier2 = ncpu;

	printf("[+] %s SpectreV2 Mitigation (IBRS)...",
	    (enabled == true) ? "Enabling" : "Disabling");
	xc = xc_broadcast(0, ibrs_change_cpu, (void *)enabled, NULL);
	xc_wait(xc);
	printf(" done!\n");

	mutex_exit(&cpu_lock);

	return 0;
}
#else
/*
 * TODO: i386
 */
static int
ibrs_change(bool enabled)
{
	panic("not supported");
}
#endif

/* -------------------------------------------------------------------------- */

static void
mitigation_disable_cpu(void *arg1, void *arg2)
{
	uint64_t msr;

	switch (mitigation_method) {
	case MITIGATION_NONE:
	case MITIGATION_INTEL_IBRS:
		panic("impossible");
		break;
	case MITIGATION_AMD_DIS_IND:
		msr = rdmsr(MSR_IC_CFG);
		msr &= ~IC_CFG_DIS_IND;
		wrmsr(MSR_IC_CFG, msr);
		break;
	}
}

static void
mitigation_enable_cpu(void *arg1, void *arg2)
{
	uint64_t msr;

	switch (mitigation_method) {
	case MITIGATION_NONE:
	case MITIGATION_INTEL_IBRS:
		panic("impossible");
		break;
	case MITIGATION_AMD_DIS_IND:
		msr = rdmsr(MSR_IC_CFG);
		msr |= IC_CFG_DIS_IND;
		wrmsr(MSR_IC_CFG, msr);
		break;
	}
}

static int
mitigation_disable(void)
{
	uint64_t xc;
	int error;

	speculation_detect_method();

	switch (mitigation_method) {
	case MITIGATION_NONE:
		printf("[!] No mitigation available\n");
		return EOPNOTSUPP;
	case MITIGATION_AMD_DIS_IND:
		printf("[+] Disabling SpectreV2 Mitigation...");
		xc = xc_broadcast(0, mitigation_disable_cpu,
		    NULL, NULL);
		xc_wait(xc);
		printf(" done!\n");
		spec_mitigation_enabled = false;
		return 0;
	case MITIGATION_INTEL_IBRS:
		error = ibrs_change(false);
		if (error)
			return error;
		spec_mitigation_enabled = false;
		return 0;
	default:
		panic("impossible");
	}
}

static int
mitigation_enable(void)
{
	uint64_t xc;
	int error;

	speculation_detect_method();

	switch (mitigation_method) {
	case MITIGATION_NONE:
		printf("[!] No mitigation available\n");
		return EOPNOTSUPP;
	case MITIGATION_AMD_DIS_IND:
		printf("[+] Enabling SpectreV2 Mitigation...");
		xc = xc_broadcast(0, mitigation_enable_cpu,
		    NULL, NULL);
		xc_wait(xc);
		printf(" done!\n");
		spec_mitigation_enabled = true;
		return 0;
	case MITIGATION_INTEL_IBRS:
		error = ibrs_change(true);
		if (error)
			return error;
		spec_mitigation_enabled = true;
		return 0;
	default:
		panic("impossible");
	}
}

int sysctl_machdep_spectreV2_mitigated(SYSCTLFN_ARGS);

int
sysctl_machdep_spectreV2_mitigated(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error;
	bool val;

	val = *(bool *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	if (val == 0) {
		if (!spec_mitigation_enabled)
			error = 0;
		else
			error = mitigation_disable();
	} else {
		if (spec_mitigation_enabled)
			error = 0;
		else
			error = mitigation_enable();
	}

	return error;
}
