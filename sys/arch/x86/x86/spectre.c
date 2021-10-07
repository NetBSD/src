/*	$NetBSD: spectre.c,v 1.36 2021/10/07 12:52:27 msaitoh Exp $	*/

/*
 * Copyright (c) 2018-2019 NetBSD Foundation, Inc.
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
 * Mitigations for the SpectreV2, SpectreV4, MDS and TAA CPU flaws.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spectre.c,v 1.36 2021/10/07 12:52:27 msaitoh Exp $");

#include "opt_spectre.h"

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

enum v2_mitigation {
	V2_MITIGATION_NONE,
	V2_MITIGATION_AMD_DIS_IND,
	V2_MITIGATION_INTEL_IBRS,
	V2_MITIGATION_INTEL_ENHANCED_IBRS
};

enum v4_mitigation {
	V4_MITIGATION_NONE,
	V4_MITIGATION_INTEL_SSBD,
	V4_MITIGATION_INTEL_SSB_NO,
	V4_MITIGATION_AMD_SSB_NO,
	V4_MITIGATION_AMD_NONARCH_F15H,
	V4_MITIGATION_AMD_NONARCH_F16H,
	V4_MITIGATION_AMD_NONARCH_F17H
};

static enum v2_mitigation v2_mitigation_method = V2_MITIGATION_NONE;
static enum v4_mitigation v4_mitigation_method = V4_MITIGATION_NONE;

static bool v2_mitigation_enabled __read_mostly = false;
static bool v4_mitigation_enabled __read_mostly = false;

static char v2_mitigation_name[64] = "(none)";
static char v4_mitigation_name[64] = "(none)";

/* --------------------------------------------------------------------- */

static void
v2_set_name(void)
{
	char name[64] = "";
	size_t nmitig = 0;

#if defined(SPECTRE_V2_GCC_MITIGATION)
	strlcat(name, "[GCC retpoline]", sizeof(name));
	nmitig++;
#endif

	if (!v2_mitigation_enabled) {
		if (nmitig == 0)
			strlcat(name, "(none)", sizeof(name));
	} else {
		if (nmitig)
			strlcat(name, " + ", sizeof(name));
		switch (v2_mitigation_method) {
		case V2_MITIGATION_AMD_DIS_IND:
			strlcat(name, "[AMD DIS_IND]", sizeof(name));
			break;
		case V2_MITIGATION_INTEL_IBRS:
			strlcat(name, "[Intel IBRS]", sizeof(name));
			break;
		case V2_MITIGATION_INTEL_ENHANCED_IBRS:
			strlcat(name, "[Intel Enhanced IBRS]", sizeof(name));
			break;
		default:
			panic("%s: impossible", __func__);
		}
	}

	strlcpy(v2_mitigation_name, name,
	    sizeof(v2_mitigation_name));
}

static void
v2_detect_method(void)
{
	struct cpu_info *ci = curcpu();
	u_int descs[4];
	uint64_t msr;

	if (cpu_vendor == CPUVENDOR_INTEL) {
		if (cpuid_level >= 7) {
			x86_cpuid(7, descs);

			if (descs[3] & CPUID_SEF_IBRS) {
				if (descs[3] & CPUID_SEF_ARCH_CAP) {
					msr = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
					if (msr & IA32_ARCH_IBRS_ALL) {
						v2_mitigation_method =
						    V2_MITIGATION_INTEL_ENHANCED_IBRS;
						return;
					}
				}
#ifdef __x86_64__
				v2_mitigation_method = V2_MITIGATION_INTEL_IBRS;
				return;
#endif
			}
		}
		v2_mitigation_method = V2_MITIGATION_NONE;
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
			v2_mitigation_method = V2_MITIGATION_AMD_DIS_IND;
			break;
		default:
			v2_mitigation_method = V2_MITIGATION_NONE;
			break;
		}
	} else {
		v2_mitigation_method = V2_MITIGATION_NONE;
	}
}

/* -------------------------------------------------------------------------- */

static volatile unsigned long ibrs_cpu_barrier1 __cacheline_aligned;
static volatile unsigned long ibrs_cpu_barrier2 __cacheline_aligned;

#ifdef __x86_64__
/* IBRS_ENTER. */
extern uint8_t noibrs_enter, noibrs_enter_end;
extern uint8_t ibrs_enter, ibrs_enter_end;
static const struct x86_hotpatch_source hp_noibrs_enter_source = {
	.saddr = &noibrs_enter,
	.eaddr = &noibrs_enter_end
};
static const struct x86_hotpatch_source hp_ibrs_enter_source = {
	.saddr = &ibrs_enter,
	.eaddr = &ibrs_enter_end
};
static const struct x86_hotpatch_descriptor hp_ibrs_enter_desc = {
	.name = HP_NAME_IBRS_ENTER,
	.nsrc = 2,
	.srcs = { &hp_noibrs_enter_source, &hp_ibrs_enter_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_ibrs_enter_desc);

/* IBRS_LEAVE. */
extern uint8_t noibrs_leave, noibrs_leave_end;
extern uint8_t ibrs_leave, ibrs_leave_end;
static const struct x86_hotpatch_source hp_noibrs_leave_source = {
	.saddr = &noibrs_leave,
	.eaddr = &noibrs_leave_end
};
static const struct x86_hotpatch_source hp_ibrs_leave_source = {
	.saddr = &ibrs_leave,
	.eaddr = &ibrs_leave_end
};
static const struct x86_hotpatch_descriptor hp_ibrs_leave_desc = {
	.name = HP_NAME_IBRS_LEAVE,
	.nsrc = 2,
	.srcs = { &hp_noibrs_leave_source, &hp_ibrs_leave_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_ibrs_leave_desc);

static void
ibrs_disable_hotpatch(void)
{
	x86_hotpatch(HP_NAME_IBRS_ENTER, /* noibrs */ 0);
	x86_hotpatch(HP_NAME_IBRS_LEAVE, /* noibrs */ 0);
}

static void
ibrs_enable_hotpatch(void)
{
	x86_hotpatch(HP_NAME_IBRS_ENTER, /* ibrs */ 1);
	x86_hotpatch(HP_NAME_IBRS_LEAVE, /* ibrs */ 1);
}
#else
/* IBRS not supported on i386 */
static void
ibrs_disable_hotpatch(void)
{
	panic("%s: impossible", __func__);
}
static void
ibrs_enable_hotpatch(void)
{
	panic("%s: impossible", __func__);
}
#endif

/* -------------------------------------------------------------------------- */

static void
mitigation_v2_apply_cpu(struct cpu_info *ci, bool enabled)
{
	uint64_t msr;

	switch (v2_mitigation_method) {
	case V2_MITIGATION_NONE:
		panic("impossible");
	case V2_MITIGATION_INTEL_IBRS:
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
		break;
	case V2_MITIGATION_INTEL_ENHANCED_IBRS:
		msr = rdmsr(MSR_IA32_SPEC_CTRL);
		if (enabled) {
			msr |= IA32_SPEC_CTRL_IBRS;
		} else {
			msr &= ~IA32_SPEC_CTRL_IBRS;
		}
		wrmsr(MSR_IA32_SPEC_CTRL, msr);
		break;
	case V2_MITIGATION_AMD_DIS_IND:
		msr = rdmsr(MSR_IC_CFG);
		if (enabled) {
			msr |= IC_CFG_DIS_IND;
		} else {
			msr &= ~IC_CFG_DIS_IND;
		}
		wrmsr(MSR_IC_CFG, msr);
		break;
	}
}

/*
 * Note: IBRS requires hotpatching, so we need barriers.
 */
static void
mitigation_v2_change_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	bool enabled = arg1 != NULL;
	u_long psl = 0;

	/* Rendez-vous 1 (IBRS only). */
	if (v2_mitigation_method == V2_MITIGATION_INTEL_IBRS) {
		psl = x86_read_psl();
		x86_disable_intr();

		atomic_dec_ulong(&ibrs_cpu_barrier1);
		while (atomic_cas_ulong(&ibrs_cpu_barrier1, 0, 0) != 0) {
			x86_pause();
		}
	}

	mitigation_v2_apply_cpu(ci, enabled);

	/* Rendez-vous 2 (IBRS only). */
	if (v2_mitigation_method == V2_MITIGATION_INTEL_IBRS) {
		atomic_dec_ulong(&ibrs_cpu_barrier2);
		while (atomic_cas_ulong(&ibrs_cpu_barrier2, 0, 0) != 0) {
			x86_pause();
		}

		/* Write back and invalidate cache, flush pipelines. */
		wbinvd();
		x86_flush();

		x86_write_psl(psl);
	}
}

static int
mitigation_v2_change(bool enabled)
{
	uint64_t xc;

	v2_detect_method();

	switch (v2_mitigation_method) {
	case V2_MITIGATION_NONE:
		printf("[!] No mitigation available\n");
		return EOPNOTSUPP;
	case V2_MITIGATION_AMD_DIS_IND:
	case V2_MITIGATION_INTEL_IBRS:
	case V2_MITIGATION_INTEL_ENHANCED_IBRS:
		/* Initialize the barriers */
		ibrs_cpu_barrier1 = ncpu;
		ibrs_cpu_barrier2 = ncpu;

		printf("[+] %s SpectreV2 Mitigation...",
		    enabled ? "Enabling" : "Disabling");
		xc = xc_broadcast(XC_HIGHPRI, mitigation_v2_change_cpu,
		    (void *)enabled, NULL);
		xc_wait(xc);
		printf(" done!\n");
		v2_mitigation_enabled = enabled;
		v2_set_name();
		return 0;
	default:
		panic("impossible");
	}
}

static int
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

	if (val == v2_mitigation_enabled)
		return 0;
	return mitigation_v2_change(val);
}

/* -------------------------------------------------------------------------- */

static void
v4_set_name(void)
{
	char name[64] = "";

	if (!v4_mitigation_enabled) {
		strlcat(name, "(none)", sizeof(name));
	} else {
		switch (v4_mitigation_method) {
		case V4_MITIGATION_NONE:
			panic("%s: impossible", __func__);
		case V4_MITIGATION_INTEL_SSBD:
			strlcat(name, "[Intel SSBD]", sizeof(name));
			break;
		case V4_MITIGATION_INTEL_SSB_NO:
			strlcat(name, "[Intel SSB_NO]", sizeof(name));
			break;
		case V4_MITIGATION_AMD_SSB_NO:
			strlcat(name, "[AMD SSB_NO]", sizeof(name));
			break;
		case V4_MITIGATION_AMD_NONARCH_F15H:
		case V4_MITIGATION_AMD_NONARCH_F16H:
		case V4_MITIGATION_AMD_NONARCH_F17H:
			strlcat(name, "[AMD NONARCH]", sizeof(name));
			break;
		}
	}

	strlcpy(v4_mitigation_name, name,
	    sizeof(v4_mitigation_name));
}

static void
v4_detect_method(void)
{
	struct cpu_info *ci = curcpu();
	u_int descs[4];
	uint64_t msr;

	if (cpu_vendor == CPUVENDOR_INTEL) {
		if (cpu_info_primary.ci_feat_val[7] & CPUID_SEF_ARCH_CAP) {
			msr = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
			if (msr & IA32_ARCH_SSB_NO) {
				/* Not vulnerable to SpectreV4. */
				v4_mitigation_method = V4_MITIGATION_INTEL_SSB_NO;
				return;
			}
		}
		if (cpuid_level >= 7) {
			x86_cpuid(7, descs);
			if (descs[3] & CPUID_SEF_SSBD) {
				/* descs[3] = %edx */
				v4_mitigation_method = V4_MITIGATION_INTEL_SSBD;
				return;
			}
		}
	} else if (cpu_vendor == CPUVENDOR_AMD) {
		switch (CPUID_TO_FAMILY(ci->ci_signature)) {
		case 0x15:
			v4_mitigation_method = V4_MITIGATION_AMD_NONARCH_F15H;
			return;
		case 0x16:
			v4_mitigation_method = V4_MITIGATION_AMD_NONARCH_F16H;
			return;
		case 0x17:
			v4_mitigation_method = V4_MITIGATION_AMD_NONARCH_F17H;
			return;
		default:
			if (cpu_info_primary.ci_max_ext_cpuid < 0x80000008) {
				break;
			}
			x86_cpuid(0x80000008, descs);
			if (descs[1] & CPUID_CAPEX_SSB_NO) {
				/* Not vulnerable to SpectreV4. */
				v4_mitigation_method = V4_MITIGATION_AMD_SSB_NO;
				return;
			}

			break;
		}
	}

	v4_mitigation_method = V4_MITIGATION_NONE;
}

static void
mitigation_v4_apply_cpu(bool enabled)
{
	uint64_t msr, msrval = 0, msrbit = 0;

	switch (v4_mitigation_method) {
	case V4_MITIGATION_NONE:
	case V4_MITIGATION_INTEL_SSB_NO:
	case V4_MITIGATION_AMD_SSB_NO:
		panic("impossible");
	case V4_MITIGATION_INTEL_SSBD:
		msrval = MSR_IA32_SPEC_CTRL;
		msrbit = IA32_SPEC_CTRL_SSBD;
		break;
	case V4_MITIGATION_AMD_NONARCH_F15H:
		msrval = MSR_LS_CFG;
		msrbit = LS_CFG_DIS_SSB_F15H;
		break;
	case V4_MITIGATION_AMD_NONARCH_F16H:
		msrval = MSR_LS_CFG;
		msrbit = LS_CFG_DIS_SSB_F16H;
		break;
	case V4_MITIGATION_AMD_NONARCH_F17H:
		msrval = MSR_LS_CFG;
		msrbit = LS_CFG_DIS_SSB_F17H;
		break;
	}

	msr = rdmsr(msrval);
	if (enabled) {
		msr |= msrbit;
	} else {
		msr &= ~msrbit;
	}
	wrmsr(msrval, msr);
}

static void
mitigation_v4_change_cpu(void *arg1, void *arg2)
{
	bool enabled = arg1 != NULL;

	mitigation_v4_apply_cpu(enabled);
}

static int
mitigation_v4_change(bool enabled)
{
	uint64_t xc;

	v4_detect_method();

	switch (v4_mitigation_method) {
	case V4_MITIGATION_NONE:
		printf("[!] No mitigation available\n");
		return EOPNOTSUPP;
	case V4_MITIGATION_INTEL_SSBD:
	case V4_MITIGATION_AMD_NONARCH_F15H:
	case V4_MITIGATION_AMD_NONARCH_F16H:
	case V4_MITIGATION_AMD_NONARCH_F17H:
		printf("[+] %s SpectreV4 Mitigation...",
		    enabled ? "Enabling" : "Disabling");
		xc = xc_broadcast(0, mitigation_v4_change_cpu,
		    (void *)enabled, NULL);
		xc_wait(xc);
		printf(" done!\n");
		v4_mitigation_enabled = enabled;
		v4_set_name();
		return 0;
	case V4_MITIGATION_INTEL_SSB_NO:
	case V4_MITIGATION_AMD_SSB_NO:
		printf("[+] The CPU is not affected by SpectreV4\n");
		return 0;
	default:
		panic("impossible");
	}
}

static int
sysctl_machdep_spectreV4_mitigated(SYSCTLFN_ARGS)
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

	if (val == v4_mitigation_enabled)
		return 0;
	return mitigation_v4_change(val);
}

/* -------------------------------------------------------------------------- */

enum mds_mitigation {
	MDS_MITIGATION_NONE,
	MDS_MITIGATION_VERW,
	MDS_MITIGATION_MDS_NO
};

static char mds_mitigation_name[64] = "(none)";

static enum mds_mitigation mds_mitigation_method = MDS_MITIGATION_NONE;
static bool mds_mitigation_enabled __read_mostly = false;

static volatile unsigned long mds_cpu_barrier1 __cacheline_aligned;
static volatile unsigned long mds_cpu_barrier2 __cacheline_aligned;

#ifdef __x86_64__
/* MDS_LEAVE. */
extern uint8_t nomds_leave, nomds_leave_end;
extern uint8_t mds_leave, mds_leave_end;
static const struct x86_hotpatch_source hp_nomds_leave_source = {
	.saddr = &nomds_leave,
	.eaddr = &nomds_leave_end
};
static const struct x86_hotpatch_source hp_mds_leave_source = {
	.saddr = &mds_leave,
	.eaddr = &mds_leave_end
};
static const struct x86_hotpatch_descriptor hp_mds_leave_desc = {
	.name = HP_NAME_MDS_LEAVE,
	.nsrc = 2,
	.srcs = { &hp_nomds_leave_source, &hp_mds_leave_source }
};
__link_set_add_rodata(x86_hotpatch_descriptors, hp_mds_leave_desc);

static void
mds_disable_hotpatch(void)
{
	x86_hotpatch(HP_NAME_MDS_LEAVE, /* nomds */ 0);
}

static void
mds_enable_hotpatch(void)
{
	x86_hotpatch(HP_NAME_MDS_LEAVE, /* mds */ 1);
}
#else
/* MDS not supported on i386 */
static void
mds_disable_hotpatch(void)
{
	panic("%s: impossible", __func__);
}
static void
mds_enable_hotpatch(void)
{
	panic("%s: impossible", __func__);
}
#endif

static void
mitigation_mds_apply_cpu(struct cpu_info *ci, bool enabled)
{
	switch (mds_mitigation_method) {
	case MDS_MITIGATION_NONE:
	case MDS_MITIGATION_MDS_NO:
		panic("impossible");
	case MDS_MITIGATION_VERW:
		/* cpu0 is the one that does the hotpatch job */
		if (ci == &cpu_info_primary) {
			if (enabled) {
				mds_enable_hotpatch();
			} else {
				mds_disable_hotpatch();
			}
		}
		break;
	}
}

static void
mitigation_mds_change_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	bool enabled = arg1 != NULL;
	u_long psl = 0;

	/* Rendez-vous 1. */
	psl = x86_read_psl();
	x86_disable_intr();

	atomic_dec_ulong(&mds_cpu_barrier1);
	while (atomic_cas_ulong(&mds_cpu_barrier1, 0, 0) != 0) {
		x86_pause();
	}

	mitigation_mds_apply_cpu(ci, enabled);

	/* Rendez-vous 2. */
	atomic_dec_ulong(&mds_cpu_barrier2);
	while (atomic_cas_ulong(&mds_cpu_barrier2, 0, 0) != 0) {
		x86_pause();
	}

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();

	x86_write_psl(psl);
}

static void
mds_detect_method(void)
{
	u_int descs[4];
	uint64_t msr;

	if (cpu_vendor != CPUVENDOR_INTEL) {
		mds_mitigation_method = MDS_MITIGATION_MDS_NO;
		return;
	}

	if (cpuid_level < 7) {
		return;
	}

	x86_cpuid(0x7, descs);
	if (descs[3] & CPUID_SEF_ARCH_CAP) {
		msr = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
		if (msr & IA32_ARCH_MDS_NO) {
			mds_mitigation_method = MDS_MITIGATION_MDS_NO;
			return;
		}
	}

#ifdef __x86_64__
	if (descs[3] & CPUID_SEF_MD_CLEAR) {
		mds_mitigation_method = MDS_MITIGATION_VERW;
	}
#endif
}

static void
mds_set_name(void)
{
	char name[64] = "";

	if (!mds_mitigation_enabled) {
		strlcat(name, "(none)", sizeof(name));
	} else {
		switch (mds_mitigation_method) {
		case MDS_MITIGATION_NONE:
			panic("%s: impossible", __func__);
		case MDS_MITIGATION_MDS_NO:
			strlcat(name, "[MDS_NO]", sizeof(name));
			break;
		case MDS_MITIGATION_VERW:
			strlcat(name, "[VERW]", sizeof(name));
			break;
		}
	}

	strlcpy(mds_mitigation_name, name,
	    sizeof(mds_mitigation_name));
}

static int
mitigation_mds_change(bool enabled)
{
	uint64_t xc;

	mds_detect_method();

	switch (mds_mitigation_method) {
	case MDS_MITIGATION_NONE:
		printf("[!] No mitigation available\n");
		return EOPNOTSUPP;
	case MDS_MITIGATION_VERW:
		/* Initialize the barriers */
		mds_cpu_barrier1 = ncpu;
		mds_cpu_barrier2 = ncpu;

		printf("[+] %s MDS Mitigation...",
		    enabled ? "Enabling" : "Disabling");
		xc = xc_broadcast(XC_HIGHPRI, mitigation_mds_change_cpu,
		    (void *)enabled, NULL);
		xc_wait(xc);
		printf(" done!\n");
		mds_mitigation_enabled = enabled;
		mds_set_name();
		return 0;
	case MDS_MITIGATION_MDS_NO:
		printf("[+] The CPU is not affected by MDS\n");
		return 0;
	default:
		panic("impossible");
	}
}

static int
sysctl_machdep_mds_mitigated(SYSCTLFN_ARGS)
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

	if (val == mds_mitigation_enabled)
		return 0;
	return mitigation_mds_change(val);
}

/* -------------------------------------------------------------------------- */

enum taa_mitigation {
	TAA_MITIGATION_NONE,
	TAA_MITIGATION_TAA_NO,
	TAA_MITIGATION_MDS,
	TAA_MITIGATION_RTM_DISABLE
};

static char taa_mitigation_name[64] = "(none)";

static enum taa_mitigation taa_mitigation_method = TAA_MITIGATION_NONE;
static bool taa_mitigation_enabled __read_mostly = false;
static bool *taa_mitigation_enabled_ptr = &taa_mitigation_enabled;

static void
mitigation_taa_apply_cpu(struct cpu_info *ci, bool enabled)
{
	uint64_t msr;

	switch (taa_mitigation_method) {
	case TAA_MITIGATION_NONE:
	case TAA_MITIGATION_TAA_NO:
	case TAA_MITIGATION_MDS:
		panic("impossible");
	case TAA_MITIGATION_RTM_DISABLE:
		msr = rdmsr(MSR_IA32_TSX_CTRL);
		if (enabled) {
			msr |= IA32_TSX_CTRL_RTM_DISABLE;
		} else {
			msr &= ~IA32_TSX_CTRL_RTM_DISABLE;
		}
		wrmsr(MSR_IA32_TSX_CTRL, msr);
		break;
	}
}

static void
mitigation_taa_change_cpu(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	bool enabled = arg1 != NULL;

	mitigation_taa_apply_cpu(ci, enabled);
}

static void
taa_detect_method(void)
{
	u_int descs[4];
	uint64_t msr;

	taa_mitigation_enabled_ptr = &taa_mitigation_enabled;

	if (cpu_vendor != CPUVENDOR_INTEL) {
		taa_mitigation_method = TAA_MITIGATION_TAA_NO;
		return;
	}
	if (!(cpu_feature[5] & CPUID_SEF_RTM)) {
		taa_mitigation_method = TAA_MITIGATION_TAA_NO;
		return;
	}

	/*
	 * If the CPU doesn't have MDS_NO set, then the TAA mitigation is based
	 * on the MDS mitigation.
	 */
	if (cpuid_level < 7) {
		taa_mitigation_method = TAA_MITIGATION_MDS;
		taa_mitigation_enabled_ptr = &mds_mitigation_enabled;
		return;
	}
	x86_cpuid(0x7, descs);
	if (!(descs[3] & CPUID_SEF_ARCH_CAP)) {
		taa_mitigation_method = TAA_MITIGATION_MDS;
		taa_mitigation_enabled_ptr = &mds_mitigation_enabled;
		return;
	}
	msr = rdmsr(MSR_IA32_ARCH_CAPABILITIES);
	if (!(msr & IA32_ARCH_MDS_NO)) {
		taa_mitigation_method = TAA_MITIGATION_MDS;
		taa_mitigation_enabled_ptr = &mds_mitigation_enabled;
		return;
	}

	/*
	 * Otherwise, we need the TAA-specific mitigation.
	 */
	if (msr & IA32_ARCH_TAA_NO) {
		taa_mitigation_method = TAA_MITIGATION_TAA_NO;
		return;
	}
	if (msr & IA32_ARCH_TSX_CTRL) {
		taa_mitigation_method = TAA_MITIGATION_RTM_DISABLE;
		return;
	}
}

static void
taa_set_name(void)
{
	char name[64] = "";

	switch (taa_mitigation_method) {
	case TAA_MITIGATION_NONE:
		strlcpy(name, "(none)", sizeof(name));
		break;
	case TAA_MITIGATION_TAA_NO:
		strlcpy(name, "[TAA_NO]", sizeof(name));
		break;
	case TAA_MITIGATION_MDS:
		strlcpy(name, "[MDS]", sizeof(name));
		break;
	case TAA_MITIGATION_RTM_DISABLE:
		if (!taa_mitigation_enabled) {
			strlcpy(name, "(none)", sizeof(name));
		} else {
			strlcpy(name, "[RTM_DISABLE]", sizeof(name));
		}
		break;
	}

	strlcpy(taa_mitigation_name, name, sizeof(taa_mitigation_name));
}

static int
mitigation_taa_change(bool enabled)
{
	uint64_t xc;

	taa_detect_method();

	switch (taa_mitigation_method) {
	case TAA_MITIGATION_NONE:
		printf("[!] No mitigation available\n");
		return EOPNOTSUPP;
	case TAA_MITIGATION_TAA_NO:
		printf("[+] The CPU is not affected by TAA\n");
		return 0;
	case TAA_MITIGATION_MDS:
		printf("[!] Mitigation based on MDS, use machdep.mds\n");
		taa_set_name();
		return EINVAL;
	case TAA_MITIGATION_RTM_DISABLE:
		printf("[+] %s TAA Mitigation...",
		    enabled ? "Enabling" : "Disabling");
		xc = xc_broadcast(XC_HIGHPRI, mitigation_taa_change_cpu,
		    (void *)enabled, NULL);
		xc_wait(xc);
		printf(" done!\n");
		taa_mitigation_enabled = enabled;
		taa_set_name();
		return 0;
	default:
		panic("impossible");
	}
}

static int
sysctl_machdep_taa_mitigated(SYSCTLFN_ARGS)
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

	if (val == *taa_mitigation_enabled_ptr)
		return 0;
	return mitigation_taa_change(val);
}

/* -------------------------------------------------------------------------- */

void speculation_barrier(struct lwp *, struct lwp *);

void
speculation_barrier(struct lwp *oldlwp, struct lwp *newlwp)
{
	/*
	 * Speculation barriers are applicable only to Spectre V2.
	 */
	if (!v2_mitigation_enabled)
		return;

	/*
	 * From kernel thread to kernel thread, no need for a barrier.
	 */
	if ((oldlwp->l_flag & LW_SYSTEM) && (newlwp->l_flag & LW_SYSTEM))
		return;

	switch (v2_mitigation_method) {
	case V2_MITIGATION_INTEL_IBRS:
		wrmsr(MSR_IA32_PRED_CMD, IA32_PRED_CMD_IBPB);
		break;
	default:
		/* nothing */
		break;
	}
}

/*
 * cpu0 is the one that detects the method and sets the global 'enabled'
 * variable for each mitigation.
 */
void
cpu_speculation_init(struct cpu_info *ci)
{
	/*
	 * Spectre V2.
	 */
	if (ci == &cpu_info_primary) {
		v2_detect_method();
		v2_mitigation_enabled =
		    (v2_mitigation_method != V2_MITIGATION_NONE);
		v2_set_name();
	}
	if (v2_mitigation_method != V2_MITIGATION_NONE) {
		mitigation_v2_apply_cpu(ci, true);
	}

	/*
	 * Spectre V4.
	 *
	 * Disabled by default, as recommended by AMD, but can be enabled
	 * dynamically. We only detect if the CPU is not vulnerable, to
	 * mark it as 'mitigated' in the sysctl.
	 */
#if 0
	if (ci == &cpu_info_primary) {
		v4_detect_method();
		v4_mitigation_enabled =
		    (v4_mitigation_method != V4_MITIGATION_NONE);
		v4_set_name();
	}
	if (v4_mitigation_method != V4_MITIGATION_NONE &&
	    v4_mitigation_method != V4_MITIGATION_INTEL_SSB_NO &&
	    v4_mitigation_method != V4_MITIGATION_AMD_SSB_NO) {
		mitigation_v4_apply_cpu(ci, true);
	}
#else
	if (ci == &cpu_info_primary) {
		v4_detect_method();
		if (v4_mitigation_method == V4_MITIGATION_INTEL_SSB_NO ||
		    v4_mitigation_method == V4_MITIGATION_AMD_SSB_NO) {
			v4_mitigation_enabled = true;
			v4_set_name();
		}
	}
#endif

	/*
	 * Microarchitectural Data Sampling.
	 */
	if (ci == &cpu_info_primary) {
		mds_detect_method();
		mds_mitigation_enabled =
		    (mds_mitigation_method != MDS_MITIGATION_NONE);
		mds_set_name();
	}
	if (mds_mitigation_method != MDS_MITIGATION_NONE &&
	    mds_mitigation_method != MDS_MITIGATION_MDS_NO) {
		mitigation_mds_apply_cpu(ci, true);
	}

	/*
	 * TSX Asynchronous Abort.
	 */
	if (ci == &cpu_info_primary) {
		taa_detect_method();
		taa_mitigation_enabled =
		    (taa_mitigation_method == TAA_MITIGATION_RTM_DISABLE) ||
		    (taa_mitigation_method == TAA_MITIGATION_TAA_NO);
		taa_set_name();
	}
	if (taa_mitigation_method == TAA_MITIGATION_RTM_DISABLE) {
		mitigation_taa_apply_cpu(ci, true);
	}
}

void sysctl_speculation_init(struct sysctllog **);

void
sysctl_speculation_init(struct sysctllog **clog)
{
	const struct sysctlnode *spec_rnode;

	/* SpectreV1 */
	spec_rnode = NULL;
	sysctl_createv(clog, 0, NULL, &spec_rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "spectre_v1", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE);
	sysctl_createv(clog, 0, &spec_rnode, &spec_rnode,
		       CTLFLAG_PERMANENT | CTLFLAG_IMMEDIATE,
		       CTLTYPE_BOOL, "mitigated",
		       SYSCTL_DESCR("Whether Spectre Variant 1 is mitigated"),
		       NULL, 0 /* mitigated=0 */, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	/* SpectreV2 */
	spec_rnode = NULL;
	sysctl_createv(clog, 0, NULL, &spec_rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "spectre_v2", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "hwmitigated",
		       SYSCTL_DESCR("Whether Spectre Variant 2 is HW-mitigated"),
		       sysctl_machdep_spectreV2_mitigated, 0,
		       &v2_mitigation_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_PERMANENT | CTLFLAG_IMMEDIATE,
		       CTLTYPE_BOOL, "swmitigated",
		       SYSCTL_DESCR("Whether Spectre Variant 2 is SW-mitigated"),
#if defined(SPECTRE_V2_GCC_MITIGATION)
		       NULL, 1,
#else
		       NULL, 0,
#endif
		       NULL, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "method",
		       SYSCTL_DESCR("Mitigation method in use"),
		       NULL, 0,
		       v2_mitigation_name, 0,
		       CTL_CREATE, CTL_EOL);

	/* SpectreV4 */
	spec_rnode = NULL;
	sysctl_createv(clog, 0, NULL, &spec_rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "spectre_v4", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "mitigated",
		       SYSCTL_DESCR("Whether Spectre Variant 4 is mitigated"),
		       sysctl_machdep_spectreV4_mitigated, 0,
		       &v4_mitigation_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "method",
		       SYSCTL_DESCR("Mitigation method in use"),
		       NULL, 0,
		       v4_mitigation_name, 0,
		       CTL_CREATE, CTL_EOL);

	/* Microarchitectural Data Sampling */
	spec_rnode = NULL;
	sysctl_createv(clog, 0, NULL, &spec_rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "mds", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "mitigated",
		       SYSCTL_DESCR("Whether MDS is mitigated"),
		       sysctl_machdep_mds_mitigated, 0,
		       &mds_mitigation_enabled, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "method",
		       SYSCTL_DESCR("Mitigation method in use"),
		       NULL, 0,
		       mds_mitigation_name, 0,
		       CTL_CREATE, CTL_EOL);

	/* TSX Asynchronous Abort */
	spec_rnode = NULL;
	sysctl_createv(clog, 0, NULL, &spec_rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "taa", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "mitigated",
		       SYSCTL_DESCR("Whether TAA is mitigated"),
		       sysctl_machdep_taa_mitigated, 0,
		       taa_mitigation_enabled_ptr, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &spec_rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "method",
		       SYSCTL_DESCR("Mitigation method in use"),
		       NULL, 0,
		       taa_mitigation_name, 0,
		       CTL_CREATE, CTL_EOL);
}
