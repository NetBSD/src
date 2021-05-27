/* $NetBSD: procfs_machdep.c,v 1.5 2021/05/27 06:11:20 ryo Exp $ */

/*-
 * Copyright (c) 2020 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: procfs_machdep.c,v 1.5 2021/05/27 06:11:20 ryo Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/systm.h>

#include <miscfs/procfs/procfs.h>

#include <aarch64/armreg.h>
#include <aarch64/cpufunc.h>

/* use variables named 'buf', 'left', 'total' */
#define FORWARD_BUF(_len)					\
	do {							\
		total += _len;					\
		if (_len < left) {				\
			buf += _len;				\
			left -= _len;				\
		} else {					\
			buf += left;				\
			left = 0;				\
		}						\
	} while (0 /*CONSTCOND*/)

#define OUTPUT_BUF(fmt, args...)				\
	do {							\
		size_t l = snprintf(buf, left, fmt, ## args);	\
		FORWARD_BUF(l);					\
	} while (0/*CONSTCOND*/)

static int
procfs_cpuinfo_features(struct cpu_info *ci, char *buf, int buflen)
{
	uint64_t isar0, isar1, mmfr2, pfr0, pfr1;
	size_t left, total;

	isar0 = ci->ci_id.ac_aa64isar0;
	isar1 = ci->ci_id.ac_aa64isar1;
	mmfr2 = ci->ci_id.ac_aa64mmfr2;
	pfr0 = ci->ci_id.ac_aa64pfr0;
	pfr1 = ci->ci_id.ac_aa64pfr1;

	left = buflen;
	total = 0;

	/*
	 * I don't know if we need to mimic the order of HWCAP in linux
	 */
	OUTPUT_BUF("Features\t:");
#define SO_EQ(reg, mask, val)	(__SHIFTOUT((reg), (mask)) == (val))
	if (SO_EQ(pfr0, ID_AA64PFR0_EL1_FP, ID_AA64PFR0_EL1_FP_IMPL))
		OUTPUT_BUF(" fp");
	if (SO_EQ(pfr0, ID_AA64PFR0_EL1_ADVSIMD, ID_AA64PFR0_EL1_ADV_SIMD_IMPL))
		OUTPUT_BUF(" asimd");
	/* notyet: " evtstrm" */
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_AES, ID_AA64ISAR0_EL1_AES_AES))
		OUTPUT_BUF(" aes");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_AES, ID_AA64ISAR0_EL1_AES_PMUL))
		OUTPUT_BUF(" pmull");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_SHA1,
	    ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU))
		OUTPUT_BUF(" sha1");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_SHA2,
	    ID_AA64ISAR0_EL1_SHA2_SHA256HSU))
		OUTPUT_BUF(" sha2");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_CRC32, ID_AA64ISAR0_EL1_CRC32_CRC32X))
		OUTPUT_BUF(" crc32");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_ATOMIC, ID_AA64ISAR0_EL1_ATOMIC_SWP))
		OUTPUT_BUF(" atomics");
	if (SO_EQ(pfr0, ID_AA64PFR0_EL1_FP, ID_AA64PFR0_EL1_FP_HP))
		OUTPUT_BUF(" fphp");
	if (SO_EQ(pfr0, ID_AA64PFR0_EL1_ADVSIMD, ID_AA64PFR0_EL1_ADV_SIMD_HP))
		OUTPUT_BUF(" asimdhp");
	/* notyet: " cpuid" */
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_RDM, ID_AA64ISAR0_EL1_RDM_SQRDML))
		OUTPUT_BUF(" asimdrdm");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_JSCVT,
	    ID_AA64ISAR1_EL1_JSCVT_SUPPORTED))
		OUTPUT_BUF(" jscvt");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_FCMA,
	    ID_AA64ISAR1_EL1_FCMA_SUPPORTED))
		OUTPUT_BUF(" fcma");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_LRCPC, ID_AA64ISAR1_EL1_LRCPC_PR))
		OUTPUT_BUF(" lrcpc");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_DPB, ID_AA64ISAR1_EL1_DPB_CVAP))
		OUTPUT_BUF(" dcpop");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_SHA3, ID_AA64ISAR0_EL1_SHA3_EOR3))
		OUTPUT_BUF(" sha3");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_SM3, ID_AA64ISAR0_EL1_SM3_SM3))
		OUTPUT_BUF(" sm3");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_SM4, ID_AA64ISAR0_EL1_SM4_SM4))
		OUTPUT_BUF(" sm4");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_DP,  ID_AA64ISAR0_EL1_DP_UDOT))
		OUTPUT_BUF(" asimddp");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_SHA2,
	    ID_AA64ISAR0_EL1_SHA2_SHA512HSU))
		OUTPUT_BUF(" sha512");
	if (SO_EQ(pfr0, ID_AA64PFR0_EL1_SVE, ID_AA64PFR0_EL1_SVE_IMPL))
		OUTPUT_BUF(" sve");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_FHM, ID_AA64ISAR0_EL1_FHM_FMLAL))
		OUTPUT_BUF(" asimdfhm");
	if (SO_EQ(pfr0, ID_AA64PFR0_EL1_DIT, ID_AA64PFR0_EL1_DIT_IMPL))
		OUTPUT_BUF(" dit");
	if (SO_EQ(mmfr2, ID_AA64MMFR2_EL1_AT, ID_AA64MMFR2_EL1_AT_16BIT))
		OUTPUT_BUF(" uscat");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_LRCPC, ID_AA64ISAR1_EL1_LRCPC_PR_UR))
		OUTPUT_BUF(" ilrcpc");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_TS, ID_AA64ISAR0_EL1_TS_CFINV))
		OUTPUT_BUF(" flagm");
	if (SO_EQ(pfr1, ID_AA64PFR1_EL1_SSBS, ID_AA64PFR1_EL1_SSBS_MSR_MRS))
		OUTPUT_BUF(" ssbs");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_SB, ID_AA64ISAR1_EL1_SB_SUPPORTED))
		OUTPUT_BUF(" sb");
#ifdef ARMV83_PAC
	if (aarch64_pac_enabled)
		OUTPUT_BUF(" paca pacg");
#endif
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_DPB, ID_AA64ISAR1_EL1_DPB_CVAP_CVADP))
		OUTPUT_BUF(" dcpodp");
	/* notyet: " sve2" */
	/* notyet: " sveaes" */
	/* notyet: " svepmull" */
	/* notyet: " svebitperm" */
	/* notyet: " svesha3" */
	/* notyet: " svesm4" */
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_TS, ID_AA64ISAR0_EL1_TS_AXFLAG))
		OUTPUT_BUF(" flagm2");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_FRINTTS,
	    ID_AA64ISAR1_EL1_FRINTTS_SUPPORTED))
		OUTPUT_BUF(" frint");
	/* notyet: " svei8mm" */
	/* notyet: " svef32mm" */
	/* notyet: " svef64mm" */
	/* notyet: " svebf16" */
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_I8MM,
	    ID_AA64ISAR1_EL1_I8MM_SUPPORTED))
		OUTPUT_BUF(" i8mm");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_BF16, ID_AA64ISAR1_EL1_BF16_BFDOT))
		OUTPUT_BUF(" bf16");
	if (SO_EQ(isar1, ID_AA64ISAR1_EL1_DGH, ID_AA64ISAR1_EL1_DGH_SUPPORTED))
		OUTPUT_BUF(" dgh");
	if (SO_EQ(isar0, ID_AA64ISAR0_EL1_RNDR, ID_AA64ISAR0_EL1_RNDR_RNDRRS))
		OUTPUT_BUF(" rng");
#ifdef ARMV85_BTI
	if (aarch64_bti_enabled)
		OUTPUT_BUF(" bti");
#endif
	OUTPUT_BUF("\n");
#undef SO_EQ

	return total;
}

int
procfs_getcpuinfstr(char *buf, size_t *lenp)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	size_t left, len, total;
	int ret = 0;

	left = *lenp;
	total = 0;

	for (CPU_INFO_FOREACH(cii, ci)) {
		OUTPUT_BUF("processor\t: %d\n", cii);

		len = procfs_cpuinfo_features(ci, buf, left);
		FORWARD_BUF(len);

		OUTPUT_BUF("CPU implementer\t: 0x%02lx\n",
		    __SHIFTOUT(ci->ci_id.ac_midr, CPU_ID_IMPLEMENTOR_MASK));
		OUTPUT_BUF("CPU architecture: 8\n");	/* ARMv8 */
		OUTPUT_BUF("CPU variant\t: 0x%lx\n",
		    __SHIFTOUT(ci->ci_id.ac_midr, CPU_ID_VARIANT_MASK));
		OUTPUT_BUF("CPU part\t: 0x%03lx\n",
		    __SHIFTOUT(ci->ci_id.ac_midr, CPU_ID_PARTNO_MASK));
		OUTPUT_BUF("CPU revision\t: %lu\n",
		    __SHIFTOUT(ci->ci_id.ac_midr, CPU_ID_REVISION_MASK));
		OUTPUT_BUF("\n");
	}

	/* not enough buffer? */
	if (total >= *lenp)
		ret = -1;

	*lenp = total + 1;	/* total output + '\0' */
	return ret;
}
