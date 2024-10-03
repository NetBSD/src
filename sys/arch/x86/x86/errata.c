/*	$NetBSD: errata.c,v 1.27.4.3 2024/10/03 12:00:57 martin Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Detect, report on, and work around known errata with x86 CPUs.
 *
 * This currently only handles AMD CPUs, and is generalised because
 * there are quite a few problems that the BIOS can patch via MSR,
 * but it is not known if the OS can patch these yet.  The list is
 * expected to grow over time.
 *
 * The data here are from: Revision Guide for AMD Athlon 64 and
 * AMD Opteron Processors, Publication #25759, Revision: 3.69,
 * Issue Date: September 2006
 *
 * https://www.amd.com/system/files/TechDocs/25759.pdf
 *
 * XXX This should perhaps be integrated with the identcpu code.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: errata.c,v 1.27.4.3 2024/10/03 12:00:57 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/xcall.h>
#include <sys/kthread.h>
#include <sys/clock.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/specialreg.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>

typedef struct errata {
	u_short		e_num;
	u_short		e_reported;
	u_int		e_data1;
	const uint8_t	*e_set;
	bool		(*e_act)(struct cpu_info *, struct errata *);
	uint64_t	e_data2;
	const char	*e_name;	/* use if e_num == 0 */
} errata_t;

/* These names match names from various AMD Errata/Revision Guides. */
typedef enum cpurev {
	/* K8 / Family 0Fh */
	BH_E4, CH_CG, CH_D0, DH_CG, DH_D0, DH_E3, DH_E6, JH_E1,
	JH_E6, SH_B0, SH_B3, SH_C0, SH_CG, SH_D0, SH_E4, SH_E5,

	/* K10 / Family 10h */
	DR_BA, DR_B2, DR_B3, RB_C2, RB_C3, BL_C2, BL_C3, DA_C2,
	DA_C3, HY_D0, HY_D1, HY_D1_G34R1,  PH_E0,

	/* Llano / Family 12h */
	LN_B0,

	/* Jaguar / Family 16h */
	KB_A1, ML_A1,

	/* Zen/Zen+/Zen2 / Family 17h */
	ZP_B1, ZP_B2, PiR_B2, Rome_B0,

	/* XXX client Zen2 names aren't known yet. */
	Z2_XB, Z2_Ren, Z2_Luc, Z2_Mat, Z2_VG, Z2_Men,

	/* Zen3/Zen4 / Family 19h */
	Milan_B1, Milan_B2, Genoa_B1,
	OINK
} cpurev_t;

/*
 * The bit-layout in the 0x80000001 CPUID result is, with bit-size
 * as the final number here:
 *
 *    resv1_4 extfam_8 extmodel_4 resv2_4 fam_4 model_4 stepping_4
 *
 * The CPUREV(family,model,stepping) macro handles the mapping for
 * family 6 and family 15 in the "fam_4" nybble, if 6 or 15, the
 * extended model is present and is bit-concatenated, and if 15,
 * the extended family is additional (ie, family 0x10 is 0xF in
 * fam_4 and 0x01 in extfam_8.)
 */
#define CPUREV(fam,mod,step)				\
	(((fam) > 0xf ?					\
	  (0xf << 8) | ((fam) - 0xf) << 20 :		\
	  (fam) << 8) |					\
	 (((mod) & 0xf) << 4) |				\
	 (((fam) == 6 || ((fam) >= 0xf)) ?		\
	  ((mod) & 0xf0) << 12 : 0) |			\
	 ((step) & 0xf))
static const u_int cpurevs[] = {
	BH_E4,		CPUREV(0x0F, 0x2B, 0x1),
	CH_CG,		CPUREV(0x0F, 0x08, 0x2),
	CH_CG,		CPUREV(0x0F, 0x0B, 0x2),
	CH_D0,		CPUREV(0x0F, 0x18, 0x0),
	CH_D0,		CPUREV(0x0F, 0x1B, 0x0),
	DH_CG,		CPUREV(0x0F, 0x0C, 0x0),
	DH_CG,		CPUREV(0x0F, 0x0E, 0x0),
	DH_CG,		CPUREV(0x0F, 0x0F, 0x0),
	DH_D0,		CPUREV(0x0F, 0x1C, 0x0),
	DH_D0,		CPUREV(0x0F, 0x1F, 0x0),
	DH_E3,		CPUREV(0x0F, 0x2C, 0x0),
	DH_E3,		CPUREV(0x0F, 0x2F, 0x0),
	DH_E6,		CPUREV(0x0F, 0x2C, 0x2),
	DH_E6,		CPUREV(0x0F, 0x2F, 0x2),
	JH_E1,		CPUREV(0x0F, 0x21, 0x0),
	JH_E6,		CPUREV(0x0F, 0x21, 0x2),
	JH_E6,		CPUREV(0x0F, 0x23, 0x2),
	SH_B0,		CPUREV(0x0F, 0x04, 0x0),
	SH_B3,		CPUREV(0x0F, 0x05, 0x1),
	SH_C0,		CPUREV(0x0F, 0x04, 0x8),
	SH_C0,		CPUREV(0x0F, 0x05, 0x8),
	SH_CG,		CPUREV(0x0F, 0x04, 0xA),
	SH_CG,		CPUREV(0x0F, 0x05, 0xA),
	SH_CG,		CPUREV(0x0F, 0x07, 0xA),
	SH_D0,		CPUREV(0x0F, 0x14, 0x0),
	SH_D0,		CPUREV(0x0F, 0x15, 0x0),
	SH_D0,		CPUREV(0x0F, 0x17, 0x0),
	SH_E4,		CPUREV(0x0F, 0x25, 0x1),
	SH_E4,		CPUREV(0x0F, 0x27, 0x1),
	SH_E5,		CPUREV(0x0F, 0x24, 0x2),

	DR_BA,		CPUREV(0x10, 0x02, 0xA),
	DR_B2,		CPUREV(0x10, 0x02, 0x2),
	DR_B3,		CPUREV(0x10, 0x02, 0x3),
	RB_C2,		CPUREV(0x10, 0x04, 0x2),
	RB_C3,		CPUREV(0x10, 0x04, 0x3),
	BL_C2,		CPUREV(0x10, 0x05, 0x2),
	BL_C3,		CPUREV(0x10, 0x05, 0x3),
	DA_C2,		CPUREV(0x10, 0x06, 0x2),
	DA_C3,		CPUREV(0x10, 0x06, 0x3),
	HY_D0,		CPUREV(0x10, 0x08, 0x0),
	HY_D1,		CPUREV(0x10, 0x08, 0x1),
	HY_D1_G34R1,	CPUREV(0x10, 0x09, 0x1),
	PH_E0,		CPUREV(0x10, 0x0A, 0x0),

	LN_B0,		CPUREV(0x12, 0x01, 0x0),

	KB_A1,		CPUREV(0x16, 0x00, 0x1),
	ML_A1,		CPUREV(0x16, 0x30, 0x1),

	ZP_B1,		CPUREV(0x17, 0x01, 0x1),
	ZP_B2,		CPUREV(0x17, 0x01, 0x2),
	PiR_B2,		CPUREV(0x17, 0x08, 0x2),
	Rome_B0,	CPUREV(0x17, 0x31, 0x0),
	Z2_XB,		CPUREV(0x17, 0x47, 0x0),
	Z2_Ren,		CPUREV(0x17, 0x60, 0x1),
	Z2_Luc,		CPUREV(0x17, 0x68, 0x1),
	Z2_Mat,		CPUREV(0x17, 0x71, 0x0),
	Z2_VG,		CPUREV(0x17, 0x90, 0x2),
	Z2_Men,		CPUREV(0x17, 0xA0, 0x0),

	Milan_B1,	CPUREV(0x19, 0x01, 0x1),
	Milan_B2,	CPUREV(0x19, 0x01, 0x2),
	Genoa_B1,	CPUREV(0x19, 0x11, 0x1),
	OINK
};

static const uint8_t x86_errata_set1[] = {
	SH_B3, SH_C0, SH_CG, DH_CG, CH_CG, OINK
};

static const uint8_t x86_errata_set2[] = {
	SH_B3, SH_C0, SH_CG, DH_CG, CH_CG, SH_D0, DH_D0, CH_D0, OINK
};

static const uint8_t x86_errata_set3[] = {
	JH_E1, DH_E3, OINK
};

static const uint8_t x86_errata_set4[] = {
	SH_C0, SH_CG, DH_CG, CH_CG, SH_D0, DH_D0, CH_D0, JH_E1,
	DH_E3, SH_E4, BH_E4, SH_E5, DH_E6, JH_E6, OINK
};

static const uint8_t x86_errata_set5[] = {
	SH_B3, OINK
};

static const uint8_t x86_errata_set6[] = {
	SH_C0, SH_CG, DH_CG, CH_CG, OINK
};

static const uint8_t x86_errata_set7[] = {
	SH_C0, SH_CG, DH_CG, CH_CG, SH_D0, DH_D0, CH_D0, OINK
};

static const uint8_t x86_errata_set8[] = {
	BH_E4, CH_CG, CH_CG, CH_D0, CH_D0, DH_CG, DH_CG, DH_CG,
	DH_D0, DH_D0, DH_E3, DH_E3, DH_E6, DH_E6, JH_E1, JH_E6,
	JH_E6, SH_B0, SH_B3, SH_C0, SH_C0, SH_CG, SH_CG, SH_CG,
	SH_D0, SH_D0, SH_D0, SH_E4, SH_E4, SH_E5, OINK
};

static const uint8_t x86_errata_set9[] = {
	DR_BA, DR_B2, OINK
};

static const uint8_t x86_errata_set10[] = {
	DR_BA, DR_B2, DR_B3, OINK
};

static const uint8_t x86_errata_set11[] = {
	DR_BA, DR_B2, DR_B3, RB_C2, RB_C3, BL_C2, BL_C3, DA_C2,
	DA_C3, HY_D0, HY_D1, HY_D1_G34R1,  PH_E0, LN_B0, OINK
};

static const uint8_t x86_errata_set12[] = {
	KB_A1, OINK
};

static const uint8_t x86_errata_set13[] = {
	ZP_B1, ZP_B2, PiR_B2, OINK
};

static const uint8_t x86_errata_set14[] = {
	ZP_B1, OINK
};

static const uint8_t x86_errata_set15[] = {
	KB_A1, ML_A1, OINK
};

static const uint8_t x86_errata_zen2[] = {
	Rome_B0, Z2_XB, Z2_Ren, Z2_Luc, Z2_Mat, Z2_VG, Z2_Men, OINK
};

static bool x86_errata_setmsr(struct cpu_info *, errata_t *);
static bool x86_errata_testmsr(struct cpu_info *, errata_t *);
static bool x86_errata_amd_1474(struct cpu_info *, errata_t *);

static errata_t errata[] = {
	/*
	 * 81: Cache Coherency Problem with Hardware Prefetching
	 * and Streaming Stores
	 */
	{
		81, FALSE, MSR_DC_CFG, x86_errata_set5,
		x86_errata_testmsr, DC_CFG_DIS_SMC_CHK_BUF, NULL
	},
	/*
	 * 86: DRAM Data Masking Feature Can Cause ECC Failures
	 */
	{
		86, FALSE, MSR_NB_CFG, x86_errata_set1,
		x86_errata_testmsr, NB_CFG_DISDATMSK, NULL
	},
	/*
	 * 89: Potential Deadlock With Locked Transactions
	 */
	{
		89, FALSE, MSR_NB_CFG, x86_errata_set8,
		x86_errata_testmsr, NB_CFG_DISIOREQLOCK, NULL
	},
	/*
	 * 94: Sequential Prefetch Feature May Cause Incorrect
	 * Processor Operation
	 */
	{
		94, FALSE, MSR_IC_CFG, x86_errata_set1,
		x86_errata_testmsr, IC_CFG_DIS_SEQ_PREFETCH, NULL
	},
	/*
	 * 97: 128-Bit Streaming Stores May Cause Coherency
	 * Failure
	 *
	 * XXX "This workaround must not be applied to processors
	 * prior to revision C0."  We don't apply it, but if it
	 * can't be applied, it shouldn't be reported.
	 */
	{
		97, FALSE, MSR_DC_CFG, x86_errata_set6,
		x86_errata_testmsr, DC_CFG_DIS_CNV_WC_SSO, NULL
	},
	/*
	 * 104: DRAM Data Masking Feature Causes ChipKill ECC
	 * Failures When Enabled With x8/x16 DRAM Devices
	 */
	{
		104, FALSE, MSR_NB_CFG, x86_errata_set7,
		x86_errata_testmsr, NB_CFG_DISDATMSK, NULL
	},
	/*
	 * 113: Enhanced Write-Combining Feature Causes System Hang
	 */
	{
		113, FALSE, MSR_BU_CFG, x86_errata_set3,
		x86_errata_setmsr, BU_CFG_WBENHWSBDIS, NULL
	},
	/*
	 * 69: Multiprocessor Coherency Problem with Hardware
	 * Prefetch Mechanism
	 */
	{
		69, FALSE, MSR_BU_CFG, x86_errata_set5,
		x86_errata_setmsr, BU_CFG_WBPFSMCCHKDIS, NULL
	},
	/*
	 * 101: DRAM Scrubber May Cause Data Corruption When Using
	 * Node-Interleaved Memory
	 */
	{
		101, FALSE, 0, x86_errata_set2,
		NULL, 0, NULL
	},
	/*
	 * 106: Potential Deadlock with Tightly Coupled Semaphores
	 * in an MP System
	 */
	{
		106, FALSE, MSR_LS_CFG, x86_errata_set2,
		x86_errata_testmsr, LS_CFG_DIS_LS2_SQUISH, NULL
	},
	/*
	 * 107: Possible Multiprocessor Coherency Problem with
	 * Setting Page Table A/D Bits
	 */
	{
		107, FALSE, MSR_BU_CFG, x86_errata_set2,
		x86_errata_testmsr, BU_CFG_THRL2IDXCMPDIS, NULL
	},
	/*
	 * 122: TLB Flush Filter May Cause Coherency Problem in
	 * Multiprocessor Systems
	 */
	{
		122, FALSE, MSR_HWCR, x86_errata_set4,
		x86_errata_setmsr, HWCR_FFDIS, NULL
	},
	/*
	 * 254: Internal Resource Livelock Involving Cached TLB Reload
	 */
	{
		254, FALSE, MSR_BU_CFG, x86_errata_set9,
		x86_errata_testmsr, BU_CFG_ERRATA_254, NULL
	},
	/*
	 * 261: Processor May Stall Entering Stop-Grant Due to Pending Data
	 * Cache Scrub
	 */
	{
		261, FALSE, MSR_DC_CFG, x86_errata_set10,
		x86_errata_testmsr, DC_CFG_ERRATA_261, NULL
	},
	/*
	 * 298: L2 Eviction May Occur During Processor Operation To Set
	 * Accessed or Dirty Bit
	 */
	{
		298, FALSE, MSR_HWCR, x86_errata_set9,
		x86_errata_testmsr, HWCR_TLBCACHEDIS, NULL
	},
	{
		298, FALSE, MSR_BU_CFG, x86_errata_set9,
		x86_errata_testmsr, BU_CFG_ERRATA_298, NULL
	},
	/*
	 * 309: Processor Core May Execute Incorrect Instructions on
	 * Concurrent L2 and Northbridge Response
	 */
	{
		309, FALSE, MSR_BU_CFG, x86_errata_set9,
		x86_errata_testmsr, BU_CFG_ERRATA_309, NULL
	},
	/*
	 * 721: Processor May Incorrectly Update Stack Pointer
	 */
	{
		721, FALSE, MSR_DE_CFG, x86_errata_set11,
		x86_errata_setmsr, DE_CFG_ERRATA_721, NULL
	},
	/*
	 * 776: Incorrect Processor Branch Prediction for Two Consecutive
	 * Linear Pages
	 */
	{
		776, FALSE, MSR_IC_CFG, x86_errata_set12,
		x86_errata_setmsr, IC_CFG_ERRATA_776, NULL
	},
	/*
	 * 793: Specific Combination of Writes to Write Combined Memory
	 * Types and Locked Instructions May Cause Core Hang
	 */
	{
		793, FALSE, MSR_LS_CFG, x86_errata_set15,
		x86_errata_setmsr, LS_CFG_ERRATA_793, NULL
	},
	/*
	 * 1021: Load Operation May Receive Stale Data From Older Store
	 * Operation
	 */
	{
		1021, FALSE, MSR_DE_CFG, x86_errata_set13,
		x86_errata_setmsr, DE_CFG_ERRATA_1021, NULL
	},
	/*
	 * 1033: A Lock Operation May Cause the System to Hang
	 */
	{
		1033, FALSE, MSR_LS_CFG, x86_errata_set14,
		x86_errata_setmsr, LS_CFG_ERRATA_1033, NULL
	},
	/*
	 * 1049: FCMOV Instruction May Not Execute Correctly
	 */
	{
		1049, FALSE, MSR_FP_CFG, x86_errata_set13,
		x86_errata_setmsr, FP_CFG_ERRATA_1049, NULL
	},
#if 0	/* Should we apply this errata? The other OSes don't. */
	/*
	 * 1091: Address Boundary Crossing Load Operation May Receive
	 * Stale Data
	 */
	{
		1091, FALSE, MSR_LS_CFG2, x86_errata_set13,
		x86_errata_setmsr, LS_CFG2_ERRATA_1091, NULL
	},
#endif
	/*
	 * 1095: Potential Violation of Read Ordering In Lock Operation
	 * In SMT (Simultaneous Multithreading) Mode
	 */
	{
		1095, FALSE, MSR_LS_CFG, x86_errata_set13,
		x86_errata_setmsr, LS_CFG_ERRATA_1095, NULL
	},
	/*
	 * 1474: A CPU core may hang after about 1044 days
	 */
	{
		1474, FALSE, MSR_CC6_CFG, x86_errata_zen2,
		x86_errata_amd_1474, CC6_CFG_DISABLE_BITS, NULL
	},
	/*
	 * Zenbleed:
	 * https://www.amd.com/en/resources/product-security/bulletin/amd-sb-7008.html
	 * https://github.com/google/security-research/security/advisories/GHSA-v6wh-rxpg-cmm8
	 * https://lock.cmpxchg8b.com/zenbleed.html
	 */
	{
		0, FALSE, MSR_DE_CFG, x86_errata_zen2,
		x86_errata_setmsr, DE_CFG_ERRATA_ZENBLEED,
		"ZenBleed"
	},
};

/*
 * 1474: A CPU core may hang after about 1044 days
 *
 * This requires disabling CC6 power level, which can be a performance
 * issue since it stops full turbo in some implementations (eg, half the
 * cores must be in CC6 to achieve the highest boost level.)  Set a timer
 * to fire in 1000 days -- except NetBSD timers end up having a signed
 * 32-bit hz-based value, which rolls over in under 25 days with HZ=1000,
 * and doing xcall(9) or kthread(9) from a callout is not allowed anyway,
 * so just have a kthread wait 1 day for 1000 times.
 */

#define AMD_ERRATA_1474_WARN_DAYS	 950
#define AMD_ERRATA_1474_BAD_DAYS	1000

static void
amd_errata_1474_disable_cc6(void *a1, void *a2)
{
	errata_t *e = a1;
	uint64_t val;

	val = rdmsr_locked(e->e_data1);
	if ((val & e->e_data2) == 0)
		return;
	wrmsr_locked(e->e_data1, val & ~e->e_data2);
	aprint_debug_dev(curcpu()->ci_dev, "erratum %u patched\n",
	    e->e_num);
}

static void
amd_errata_1474_thread(void *arg)
{
	int loops = 0;
	int ticks;

	ticks = hz * SECS_PER_DAY;
#ifdef X86_ERRATA_TEST_AMD_1474
	/*
	 * Make this trigger warning after 50 seconds, and workaround
	 * at 100 seconds, for easy testing.
	 */
	ticks = hz;
	loops = 900;
#endif

	while (loops++ < AMD_ERRATA_1474_BAD_DAYS) {
		if (loops == AMD_ERRATA_1474_WARN_DAYS) {
			printf("warning: AMD Errata 1474 workaround scheduled "
			       "for %u days.\n", AMD_ERRATA_1474_BAD_DAYS -
						 AMD_ERRATA_1474_WARN_DAYS);
			printf("warning: reboot required to avoid.\n");
		}
		kpause("amd1474", false, ticks, NULL);
	}

	/* Been 1000 days, disable CC6 and warn about it. */
	uint64_t xc = xc_broadcast(0, amd_errata_1474_disable_cc6, arg, NULL);
	xc_wait(xc);

	printf("warning: AMD CC6 disabled due to errata 1474.\n");
	printf("warning: reboot required to restore full turbo speeds.\n");

	kthread_exit(0);
}

static bool
x86_errata_amd_1474(struct cpu_info *ci, errata_t *e)
{
	int error;

	/* Don't do anything on non-primary CPUs. */
	if (!CPU_IS_PRIMARY(ci))
		return FALSE;

	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    amd_errata_1474_thread, e, NULL, "amd1474");
	if (error) {
		printf("WARNING: Unable to disable AMD errata 1474!\n");
		printf("WARNING: reboot system after %u days to avoid CPU "
		    "hangs.\n", AMD_ERRATA_1474_BAD_DAYS);
	} else {
		aprint_debug_dev(ci->ci_dev, "workaround for erratum %u "
		    "scheduled for %u days\n", e->e_num,
		    AMD_ERRATA_1474_BAD_DAYS);
	}

	/* Do own warning here, it's not like most others. */
	return FALSE;
}

static void
x86_errata_log(device_t dev, errata_t *e, const char *msg)
{

	if (e->e_num == 0)
		aprint_debug_dev(dev, "erratum '%s' %s\n", e->e_name, msg);
	else
		aprint_debug_dev(dev, "erratum %u %s\n", e->e_num, msg);
}

static bool
x86_errata_testmsr(struct cpu_info *ci, errata_t *e)
{
	uint64_t val;

	(void)ci;

	val = rdmsr_locked(e->e_data1);
	if ((val & e->e_data2) != 0)
		return FALSE;

	e->e_reported = TRUE;
	return TRUE;
}

static bool
x86_errata_setmsr(struct cpu_info *ci, errata_t *e)
{
	uint64_t val;

	(void)ci;

	val = rdmsr_locked(e->e_data1);
	if ((val & e->e_data2) != 0)
		return FALSE;
	wrmsr_locked(e->e_data1, val | e->e_data2);
	x86_errata_log(ci->ci_dev, e, "patched");

	return FALSE;
}

void
x86_errata(void)
{
	struct cpu_info *ci;
	uint32_t descs[4];
	errata_t *e, *ex;
	cpurev_t rev;
	int i, j, upgrade;
	static int again;

	/* don't run if we are under a hypervisor */
	if (cpu_feature[1] & CPUID2_RAZ)
		return;

	/* only for AMD */
	if (cpu_vendor != CPUVENDOR_AMD)
		return;

	ci = curcpu();

	x86_cpuid(0x80000001, descs);
	if (CPU_IS_PRIMARY(ci)) {
		aprint_verbose_dev(ci->ci_dev,
		    "searching errata for cpu revision 0x%08"PRIx32"\n",
		    descs[0]);
	}

	for (i = 0;; i += 2) {
		if ((rev = cpurevs[i]) == OINK)
			return;
		if (cpurevs[i + 1] == descs[0])
			break;
	}

	ex = errata + __arraycount(errata);
	for (upgrade = 0, e = errata; e < ex; e++) {
		if (e->e_reported)
			continue;
		if (e->e_set != NULL) {
			for (j = 0; e->e_set[j] != OINK; j++)
				if (e->e_set[j] == rev)
					break;
			if (e->e_set[j] == OINK)
				continue;
		}

		x86_errata_log(ci->ci_dev, e, "testing");

		if (e->e_act == NULL)
			e->e_reported = TRUE;
		else if ((*e->e_act)(ci, e) == FALSE)
			continue;

		x86_errata_log(ci->ci_dev, e, "present");
		upgrade = 1;
	}

	if (upgrade && !again) {
		again = 1;
		aprint_normal_dev(ci->ci_dev, "WARNING: errata present,"
		    " BIOS upgrade may be\n");
		aprint_normal_dev(ci->ci_dev, "WARNING: necessary to ensure"
		    " reliable operation\n");
	}
}
