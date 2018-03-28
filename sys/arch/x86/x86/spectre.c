/*	$NetBSD: spectre.c,v 1.1 2018/03/28 14:56:59 maxv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: spectre.c,v 1.1 2018/03/28 14:56:59 maxv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/specialreg.h>

#include <x86/cputypes.h>

enum spec_mitigation {
	MITIGATION_NONE,
	MITIGATION_AMD_DIS_IND,
	MITIGATION_INTEL_IBRS
};

bool spec_mitigation_enabled __read_mostly = false;
static enum spec_mitigation mitigation_method = MITIGATION_NONE;

static void
speculation_detect_method(void)
{
	struct cpu_info *ci = curcpu();

	if (cpu_vendor == CPUVENDOR_INTEL) {
		/* TODO: detect MITIGATION_INTEL_IBRS */
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

static void
mitigation_disable_cpu(void *arg1, void *arg2)
{
	uint64_t msr;

	switch (mitigation_method) {
	case MITIGATION_NONE:
		panic("impossible");
		break;
	case MITIGATION_AMD_DIS_IND:
		msr = rdmsr(MSR_IC_CFG);
		msr &= ~IC_CFG_DIS_IND;
		wrmsr(MSR_IC_CFG, msr);
		break;
	case MITIGATION_INTEL_IBRS:
		/* ibrs_disable() TODO */
		break;
	}
}

static void
mitigation_enable_cpu(void *arg1, void *arg2)
{
	uint64_t msr;

	switch (mitigation_method) {
	case MITIGATION_NONE:
		panic("impossible");
		break;
	case MITIGATION_AMD_DIS_IND:
		msr = rdmsr(MSR_IC_CFG);
		msr |= IC_CFG_DIS_IND;
		wrmsr(MSR_IC_CFG, msr);
		break;
	case MITIGATION_INTEL_IBRS:
		/* ibrs_enable() TODO */
		break;
	}
}

static int
mitigation_disable(void)
{
	uint64_t xc;

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
		/* TODO */
		return 0;
	default:
		panic("impossible");
	}
}

static int
mitigation_enable(void)
{
	uint64_t xc;

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
		/* TODO */
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
	int error, val;

	val = *(int *)rnode->sysctl_data;

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
