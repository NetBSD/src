/*	$NetBSD: identcpu.c,v 1.32.2.2 2017/12/03 11:36:50 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden,  and by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: identcpu.c,v 1.32.2.2 2017/12/03 11:36:50 jdolecek Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/specialreg.h>
#include <machine/pio.h>
#include <machine/cpu.h>

#include <x86/cputypes.h>
#include <x86/cacheinfo.h>
#include <x86/cpuvar.h>
#include <x86/cpu_msr.h>
#include <x86/fpu.h>

#include <x86/x86/vmtreg.h>	/* for vmt_hvcall() */
#include <x86/x86/vmtvar.h>	/* for vmt_hvcall() */

static const struct x86_cache_info intel_cpuid_cache_info[] = INTEL_CACHE_INFO;

static const struct x86_cache_info amd_cpuid_l2cache_assoc_info[] = 
	AMD_L2CACHE_INFO;

static const struct x86_cache_info amd_cpuid_l3cache_assoc_info[] = 
	AMD_L3CACHE_INFO;

int cpu_vendor;
char cpu_brand_string[49];

int x86_fpu_save __read_mostly;
unsigned int x86_fpu_save_size __read_mostly = 512;
uint64_t x86_xsave_features __read_mostly = 0;

/*
 * Note: these are just the ones that may not have a cpuid instruction.
 * We deal with the rest in a different way.
 */
const int i386_nocpuid_cpus[] = {
	CPUVENDOR_INTEL, CPUCLASS_386,	/* CPU_386SX */
	CPUVENDOR_INTEL, CPUCLASS_386,	/* CPU_386   */
	CPUVENDOR_INTEL, CPUCLASS_486,	/* CPU_486SX */
	CPUVENDOR_INTEL, CPUCLASS_486, 	/* CPU_486   */
	CPUVENDOR_CYRIX, CPUCLASS_486,	/* CPU_486DLC */
	CPUVENDOR_CYRIX, CPUCLASS_486,	/* CPU_6x86 */
	CPUVENDOR_NEXGEN, CPUCLASS_386,	/* CPU_NX586 */
};

static const char cpu_vendor_names[][10] = {
	"Unknown", "Intel", "NS/Cyrix", "NexGen", "AMD", "IDT/VIA", "Transmeta",
	"Vortex86"
};

static const struct x86_cache_info *
cache_info_lookup(const struct x86_cache_info *cai, uint8_t desc)
{
	int i;

	for (i = 0; cai[i].cai_desc != 0; i++) {
		if (cai[i].cai_desc == desc)
			return (&cai[i]);
	}

	return (NULL);
}

static void
cpu_probe_intel_cache(struct cpu_info *ci)
{
	const struct x86_cache_info *cai;
	u_int descs[4];
	int iterations, i, j;
	uint8_t desc;

	if (cpuid_level >= 2) { 
		/* Parse the cache info from `cpuid leaf 2', if we have it. */
		x86_cpuid(2, descs);
		iterations = descs[0] & 0xff;
		while (iterations-- > 0) {
			for (i = 0; i < 4; i++) {
				if (descs[i] & 0x80000000)
					continue;
				for (j = 0; j < 4; j++) {
					if (i == 0 && j == 0)
						continue;
					desc = (descs[i] >> (j * 8)) & 0xff;
					if (desc == 0)
						continue;
					cai = cache_info_lookup(
					    intel_cpuid_cache_info, desc);
					if (cai != NULL) {
						ci->ci_cinfo[cai->cai_index] =
						    *cai;
					}
				}
			}
		}
	}

	if (cpuid_level >= 4) {
		int type, level;
		int ways, partitions, linesize, sets;
		int caitype = -1;
		int totalsize;

		/* Parse the cache info from `cpuid leaf 4', if we have it. */
		for (i = 0; ; i++) {
			x86_cpuid2(4, i, descs);
			type = __SHIFTOUT(descs[0], CPUID_DCP_CACHETYPE);
			if (type == CPUID_DCP_CACHETYPE_N)
				break;
			level = __SHIFTOUT(descs[0], CPUID_DCP_CACHELEVEL);
			switch (level) {
			case 1:
				if (type == CPUID_DCP_CACHETYPE_I)
					caitype = CAI_ICACHE;
				else if (type == CPUID_DCP_CACHETYPE_D)
					caitype = CAI_DCACHE;
				else
					caitype = -1;
				break;
			case 2:
				if (type == CPUID_DCP_CACHETYPE_U)
					caitype = CAI_L2CACHE;
				else
					caitype = -1;
				break;
			case 3:
				if (type == CPUID_DCP_CACHETYPE_U)
					caitype = CAI_L3CACHE;
				else
					caitype = -1;
				break;
			default:
				caitype = -1;
				break;
			}
			if (caitype == -1)
				continue;

			ways = __SHIFTOUT(descs[1], CPUID_DCP_WAYS) + 1;
			partitions =__SHIFTOUT(descs[1], CPUID_DCP_PARTITIONS)
			    + 1;
			linesize = __SHIFTOUT(descs[1], CPUID_DCP_LINESIZE)
			    + 1;
			sets = descs[2] + 1;
			totalsize = ways * partitions * linesize * sets;
			ci->ci_cinfo[caitype].cai_totalsize = totalsize;
			ci->ci_cinfo[caitype].cai_associativity = ways;
			ci->ci_cinfo[caitype].cai_linesize = linesize;
		}
	}
}

static void
cpu_probe_intel(struct cpu_info *ci)
{

	if (cpu_vendor != CPUVENDOR_INTEL)
		return;

	cpu_probe_intel_cache(ci);
}

static void
cpu_probe_amd_cache(struct cpu_info *ci)
{
	const struct x86_cache_info *cp;
	struct x86_cache_info *cai;
	int family, model;
	u_int descs[4];
	u_int lfunc;

	family = CPUID_TO_FAMILY(ci->ci_signature);
	model = CPUID_TO_MODEL(ci->ci_signature);

	/*
	 * K5 model 0 has none of this info.
	 */
	if (family == 5 && model == 0)
		return;

	/*
	 * Determine the largest extended function value.
	 */
	x86_cpuid(0x80000000, descs);
	lfunc = descs[0];

	/*
	 * Determine L1 cache/TLB info.
	 */
	if (lfunc < 0x80000005) {
		/* No L1 cache info available. */
		return;
	}

	x86_cpuid(0x80000005, descs);

	/*
	 * K6-III and higher have large page TLBs.
	 */
	if ((family == 5 && model >= 9) || family >= 6) {
		cai = &ci->ci_cinfo[CAI_ITLB2];
		cai->cai_totalsize = AMD_L1_EAX_ITLB_ENTRIES(descs[0]);
		cai->cai_associativity = AMD_L1_EAX_ITLB_ASSOC(descs[0]);
		cai->cai_linesize = (4 * 1024 * 1024);

		cai = &ci->ci_cinfo[CAI_DTLB2];
		cai->cai_totalsize = AMD_L1_EAX_DTLB_ENTRIES(descs[0]);
		cai->cai_associativity = AMD_L1_EAX_DTLB_ASSOC(descs[0]);
		cai->cai_linesize = (4 * 1024 * 1024);
	}

	cai = &ci->ci_cinfo[CAI_ITLB];
	cai->cai_totalsize = AMD_L1_EBX_ITLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L1_EBX_ITLB_ASSOC(descs[1]);
	cai->cai_linesize = (4 * 1024);

	cai = &ci->ci_cinfo[CAI_DTLB];
	cai->cai_totalsize = AMD_L1_EBX_DTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L1_EBX_DTLB_ASSOC(descs[1]);
	cai->cai_linesize = (4 * 1024);

	cai = &ci->ci_cinfo[CAI_DCACHE];
	cai->cai_totalsize = AMD_L1_ECX_DC_SIZE(descs[2]);
	cai->cai_associativity = AMD_L1_ECX_DC_ASSOC(descs[2]);
	cai->cai_linesize = AMD_L1_ECX_DC_LS(descs[2]);

	cai = &ci->ci_cinfo[CAI_ICACHE];
	cai->cai_totalsize = AMD_L1_EDX_IC_SIZE(descs[3]);
	cai->cai_associativity = AMD_L1_EDX_IC_ASSOC(descs[3]);
	cai->cai_linesize = AMD_L1_EDX_IC_LS(descs[3]);

	/*
	 * Determine L2 cache/TLB info.
	 */
	if (lfunc < 0x80000006) {
		/* No L2 cache info available. */
		return;
	}

	x86_cpuid(0x80000006, descs);

	cai = &ci->ci_cinfo[CAI_L2CACHE];
	cai->cai_totalsize = AMD_L2_ECX_C_SIZE(descs[2]);
	cai->cai_associativity = AMD_L2_ECX_C_ASSOC(descs[2]);
	cai->cai_linesize = AMD_L2_ECX_C_LS(descs[2]);

	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	if (family < 0xf) {
		/* No L3 cache info available. */
		return;
	}

	cai = &ci->ci_cinfo[CAI_L3CACHE];
	cai->cai_totalsize = AMD_L3_EDX_C_SIZE(descs[3]);
	cai->cai_associativity = AMD_L3_EDX_C_ASSOC(descs[3]);
	cai->cai_linesize = AMD_L3_EDX_C_LS(descs[3]);

	cp = cache_info_lookup(amd_cpuid_l3cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown reserved */

	if (lfunc < 0x80000019) {
		/* No 1GB Page TLB */
		return;
	}

	x86_cpuid(0x80000019, descs);

	cai = &ci->ci_cinfo[CAI_L1_1GBDTLB];
	cai->cai_totalsize = AMD_L1_1GB_EAX_DTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L1_1GB_EAX_DTLB_ASSOC(descs[1]);
	cai->cai_linesize = (1 * 1024);

	cai = &ci->ci_cinfo[CAI_L1_1GBITLB];
	cai->cai_totalsize = AMD_L1_1GB_EAX_IUTLB_ENTRIES(descs[0]);
	cai->cai_associativity = AMD_L1_1GB_EAX_IUTLB_ASSOC(descs[0]);
	cai->cai_linesize = (1 * 1024);

	cai = &ci->ci_cinfo[CAI_L2_1GBDTLB];
	cai->cai_totalsize = AMD_L2_1GB_EBX_DUTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L2_1GB_EBX_DUTLB_ASSOC(descs[1]);
	cai->cai_linesize = (1 * 1024);

	cai = &ci->ci_cinfo[CAI_L2_1GBITLB];
	cai->cai_totalsize = AMD_L2_1GB_EBX_IUTLB_ENTRIES(descs[0]);
	cai->cai_associativity = AMD_L2_1GB_EBX_IUTLB_ASSOC(descs[0]);
	cai->cai_linesize = (1 * 1024);
}

static void
cpu_probe_k5(struct cpu_info *ci)
{
	int flag;

	if (cpu_vendor != CPUVENDOR_AMD ||
	    CPUID_TO_FAMILY(ci->ci_signature) != 5)
		return;

	if (CPUID_TO_MODEL(ci->ci_signature) == 0) {
		/*
		 * According to the AMD Processor Recognition App Note,
		 * the AMD-K5 Model 0 uses the wrong bit to indicate
		 * support for global PTEs, instead using bit 9 (APIC)
		 * rather than bit 13 (i.e. "0x200" vs. 0x2000".  Oops!).
		 */
		flag = ci->ci_feat_val[0];
		if ((flag & CPUID_APIC) != 0)
			flag = (flag & ~CPUID_APIC) | CPUID_PGE;
		ci->ci_feat_val[0] = flag;
	}

	cpu_probe_amd_cache(ci);
}

static void
cpu_probe_k678(struct cpu_info *ci)
{

	if (cpu_vendor != CPUVENDOR_AMD ||
	    CPUID_TO_FAMILY(ci->ci_signature) < 6)
		return;

	cpu_probe_amd_cache(ci);
}

static inline uint8_t
cyrix_read_reg(uint8_t reg)
{

	outb(0x22, reg);
	return inb(0x23);
}

static inline void
cyrix_write_reg(uint8_t reg, uint8_t data)
{

	outb(0x22, reg);
	outb(0x23, data);
}

static void
cpu_probe_cyrix_cmn(struct cpu_info *ci)
{
	/*
	 * i8254 latch check routine:
	 *     National Geode (formerly Cyrix MediaGX) has a serious bug in
	 *     its built-in i8254-compatible clock module (cs5510 cs5520).
	 *     Set the variable 'clock_broken_latch' to indicate it.
	 *
	 * This bug is not present in the cs5530, and the flag
	 * is disabled again in sys/arch/i386/pci/pcib.c if this later
	 * model device is detected. Ideally, this work-around should not
	 * even be in here, it should be in there. XXX
	 */
	uint8_t c3;
#ifndef XEN
	extern int clock_broken_latch;

	switch (ci->ci_signature) {
	case 0x440:     /* Cyrix MediaGX */
	case 0x540:     /* GXm */
		clock_broken_latch = 1;
		break;
	}
#endif

	/* set up various cyrix registers */
	/*
	 * Enable suspend on halt (powersave mode).
	 * When powersave mode is enabled, the TSC stops counting
	 * while the CPU is halted in idle() waiting for an interrupt.
	 * This means we can't use the TSC for interval time in
	 * microtime(9), and thus it is disabled here.
	 *
	 * It still makes a perfectly good cycle counter
	 * for program profiling, so long as you remember you're
	 * counting cycles, and not time. Further, if you don't
	 * mind not using powersave mode, the TSC works just fine,
	 * so this should really be optional. XXX
	 */
	cyrix_write_reg(0xc2, cyrix_read_reg(0xc2) | 0x08);

	/* 
	 * Do not disable the TSC on the Geode GX, it's reported to
	 * work fine.
	 */
	if (ci->ci_signature != 0x552)
		ci->ci_feat_val[0] &= ~CPUID_TSC;

	/* enable access to ccr4/ccr5 */
	c3 = cyrix_read_reg(0xC3);
	cyrix_write_reg(0xC3, c3 | 0x10);
	/* cyrix's workaround  for the "coma bug" */
	cyrix_write_reg(0x31, cyrix_read_reg(0x31) | 0xf8);
	cyrix_write_reg(0x32, cyrix_read_reg(0x32) | 0x7f);
	cyrix_write_reg(0x33, cyrix_read_reg(0x33) & ~0xff);
	cyrix_write_reg(0x3c, cyrix_read_reg(0x3c) | 0x87);
	/* disable access to ccr4/ccr5 */
	cyrix_write_reg(0xC3, c3);
}

static void
cpu_probe_cyrix(struct cpu_info *ci)
{

	if (cpu_vendor != CPUVENDOR_CYRIX ||
	    CPUID_TO_FAMILY(ci->ci_signature) < 4 ||
	    CPUID_TO_FAMILY(ci->ci_signature) > 6)
		return;

	cpu_probe_cyrix_cmn(ci);
}

static void
cpu_probe_winchip(struct cpu_info *ci)
{

	if (cpu_vendor != CPUVENDOR_IDT)
	    	return;

	switch (CPUID_TO_FAMILY(ci->ci_signature)) {
	case 5:
		/* WinChip C6 */
		if (CPUID_TO_MODEL(ci->ci_signature) == 4)
			ci->ci_feat_val[0] &= ~CPUID_TSC;
		break;
	case 6:
		/*
		 * VIA Eden ESP 
		 *
		 * Quoting from page 3-4 of: "VIA Eden ESP Processor Datasheet"
		 * http://www.via.com.tw/download/mainboards/6/14/Eden20v115.pdf
		 * 
		 * 1. The CMPXCHG8B instruction is provided and always enabled,
		 *    however, it appears disabled in the corresponding CPUID
		 *    function bit 0 to avoid a bug in an early version of
		 *    Windows NT. However, this default can be changed via a
		 *    bit in the FCR MSR.
		 */
		ci->ci_feat_val[0] |= CPUID_CX8;
		wrmsr(MSR_VIA_FCR, rdmsr(MSR_VIA_FCR) | 0x00000001);
		break;
	}
}

static void
cpu_probe_c3(struct cpu_info *ci)
{
	u_int family, model, stepping, descs[4], lfunc, msr;
	struct x86_cache_info *cai;

	if (cpu_vendor != CPUVENDOR_IDT ||
	    CPUID_TO_FAMILY(ci->ci_signature) < 6)
	    	return;

	family = CPUID_TO_FAMILY(ci->ci_signature);
	model = CPUID_TO_MODEL(ci->ci_signature);
	stepping = CPUID_TO_STEPPING(ci->ci_signature);

	/* Determine the largest extended function value. */
	x86_cpuid(0x80000000, descs);
	lfunc = descs[0];

	if (family > 6 || model > 0x9 || (model == 0x9 && stepping >= 3)) {
		/* Nehemiah or Esther */
		x86_cpuid(0xc0000000, descs);
		lfunc = descs[0];
		if (lfunc >= 0xc0000001) {	/* has ACE, RNG */
		    int rng_enable = 0, ace_enable = 0;
		    x86_cpuid(0xc0000001, descs);
		    lfunc = descs[3];
		    ci->ci_feat_val[4] = lfunc;
		    /* Check for and enable RNG */
		    if (lfunc & CPUID_VIA_HAS_RNG) {
		    	if (!(lfunc & CPUID_VIA_DO_RNG)) {
			    rng_enable++;
			    ci->ci_feat_val[4] |= CPUID_VIA_DO_RNG;
			}
		    }
		    /* Check for and enable ACE (AES-CBC) */
		    if (lfunc & CPUID_VIA_HAS_ACE) {
			if (!(lfunc & CPUID_VIA_DO_ACE)) {
			    ace_enable++;
			    ci->ci_feat_val[4] |= CPUID_VIA_DO_ACE;
			}
		    }
		    /* Check for and enable SHA */
		    if (lfunc & CPUID_VIA_HAS_PHE) {
			if (!(lfunc & CPUID_VIA_DO_PHE)) {
			    ace_enable++;
			    ci->ci_feat_val[4] |= CPUID_VIA_DO_PHE;
			}
		    }
		    /* Check for and enable ACE2 (AES-CTR) */
		    if (lfunc & CPUID_VIA_HAS_ACE2) {
			if (!(lfunc & CPUID_VIA_DO_ACE2)) {
			    ace_enable++;
			    ci->ci_feat_val[4] |= CPUID_VIA_DO_ACE2;
			}
		    }
		    /* Check for and enable PMM (modmult engine) */
		    if (lfunc & CPUID_VIA_HAS_PMM) {
			if (!(lfunc & CPUID_VIA_DO_PMM)) {
			    ace_enable++;
			    ci->ci_feat_val[4] |= CPUID_VIA_DO_PMM;
			}
		    }

		    /*
		     * Actually do the enables.  It's a little gross,
		     * but per the PadLock programming guide, "Enabling
		     * PadLock", condition 3, we must enable SSE too or
		     * else the first use of RNG or ACE instructions
		     * will generate a trap.
		     *
		     * We must do this early because of kernel RNG
		     * initialization but it is safe without the full
		     * FPU-detect as all these CPUs have SSE.
		     */
		    lcr4(rcr4() | CR4_OSFXSR);

		    if (rng_enable) {
			msr = rdmsr(MSR_VIA_RNG);
			msr |= MSR_VIA_RNG_ENABLE;
			/* C7 stepping 8 and subsequent CPUs have dual RNG */
			if (model > 0xA || (model == 0xA && stepping > 0x7)) {
				msr |= MSR_VIA_RNG_2NOISE;
			}
			wrmsr(MSR_VIA_RNG, msr);
		    }

		    if (ace_enable) {
			msr = rdmsr(MSR_VIA_ACE);
			wrmsr(MSR_VIA_ACE, msr | MSR_VIA_ACE_ENABLE);
		    }

		}
	}

	/*
	 * Determine L1 cache/TLB info.
	 */
	if (lfunc < 0x80000005) {
		/* No L1 cache info available. */
		return;
	}

	x86_cpuid(0x80000005, descs);

	cai = &ci->ci_cinfo[CAI_ITLB];
	cai->cai_totalsize = VIA_L1_EBX_ITLB_ENTRIES(descs[1]);
	cai->cai_associativity = VIA_L1_EBX_ITLB_ASSOC(descs[1]);
	cai->cai_linesize = (4 * 1024);

	cai = &ci->ci_cinfo[CAI_DTLB];
	cai->cai_totalsize = VIA_L1_EBX_DTLB_ENTRIES(descs[1]);
	cai->cai_associativity = VIA_L1_EBX_DTLB_ASSOC(descs[1]);
	cai->cai_linesize = (4 * 1024);

	cai = &ci->ci_cinfo[CAI_DCACHE];
	cai->cai_totalsize = VIA_L1_ECX_DC_SIZE(descs[2]);
	cai->cai_associativity = VIA_L1_ECX_DC_ASSOC(descs[2]);
	cai->cai_linesize = VIA_L1_EDX_IC_LS(descs[2]);
	if (family == 6 && model == 9 && stepping == 8) {
		/* Erratum: stepping 8 reports 4 when it should be 2 */
		cai->cai_associativity = 2;
	}

	cai = &ci->ci_cinfo[CAI_ICACHE];
	cai->cai_totalsize = VIA_L1_EDX_IC_SIZE(descs[3]);
	cai->cai_associativity = VIA_L1_EDX_IC_ASSOC(descs[3]);
	cai->cai_linesize = VIA_L1_EDX_IC_LS(descs[3]);
	if (family == 6 && model == 9 && stepping == 8) {
		/* Erratum: stepping 8 reports 4 when it should be 2 */
		cai->cai_associativity = 2;
	}

	/*
	 * Determine L2 cache/TLB info.
	 */
	if (lfunc < 0x80000006) {
		/* No L2 cache info available. */
		return;
	}

	x86_cpuid(0x80000006, descs);

	cai = &ci->ci_cinfo[CAI_L2CACHE];
	if (family > 6 || model >= 9) {
		cai->cai_totalsize = VIA_L2N_ECX_C_SIZE(descs[2]);
		cai->cai_associativity = VIA_L2N_ECX_C_ASSOC(descs[2]);
		cai->cai_linesize = VIA_L2N_ECX_C_LS(descs[2]);
	} else {
		cai->cai_totalsize = VIA_L2_ECX_C_SIZE(descs[2]);
		cai->cai_associativity = VIA_L2_ECX_C_ASSOC(descs[2]);
		cai->cai_linesize = VIA_L2_ECX_C_LS(descs[2]);
	}
}

static void
cpu_probe_geode(struct cpu_info *ci)
{

	if (memcmp("Geode by NSC", ci->ci_vendor, 12) != 0 ||
	    CPUID_TO_FAMILY(ci->ci_signature) != 5)
	    	return;

	cpu_probe_cyrix_cmn(ci);
	cpu_probe_amd_cache(ci);
}

static void
cpu_probe_vortex86(struct cpu_info *ci)
{
#define PCI_MODE1_ADDRESS_REG	0x0cf8
#define PCI_MODE1_DATA_REG	0x0cfc
#define PCI_MODE1_ENABLE	0x80000000UL

	uint32_t reg;

	if (cpu_vendor != CPUVENDOR_VORTEX86)
		return;
	/*
	 * CPU model available from "Customer ID register" in
	 * North Bridge Function 0 PCI space
	 * we can't use pci_conf_read() because the PCI subsystem is not
	 * not initialised early enough
	 */

	outl(PCI_MODE1_ADDRESS_REG, PCI_MODE1_ENABLE | 0x90);
	reg = inl(PCI_MODE1_DATA_REG);

	switch(reg) {
	case 0x31504d44:
		strcpy(cpu_brand_string, "Vortex86SX");
		break;
	case 0x32504d44:
		strcpy(cpu_brand_string, "Vortex86DX");
		break;
	case 0x33504d44:
		strcpy(cpu_brand_string, "Vortex86MX");
		break;
	case 0x37504d44:
		strcpy(cpu_brand_string, "Vortex86EX");
		break;
	default:
		strcpy(cpu_brand_string, "Unknown Vortex86");
		break;
	}

#undef PCI_MODE1_ENABLE
#undef PCI_MODE1_ADDRESS_REG
#undef PCI_MODE1_DATA_REG
}

static void
cpu_probe_old_fpu(struct cpu_info *ci)
{
#if defined(__i386__) && !defined(XEN)

	clts();
	fninit();

	/* Check for 'FDIV' bug on the original Pentium */
	if (npx586bug1(4195835, 3145727) != 0)
		/* NB 120+MHz cpus are not affected */
		i386_fpu_fdivbug = 1;

	stts();
#endif
}

static void
cpu_probe_fpu(struct cpu_info *ci)
{
	u_int descs[4];

	x86_fpu_save = FPU_SAVE_FSAVE;

#ifdef i386 /* amd64 always has fxsave, sse and sse2 */
	/* If we have FXSAVE/FXRESTOR, use them. */
	if ((ci->ci_feat_val[0] & CPUID_FXSR) == 0) {
		i386_use_fxsave = 0;
		/* Allow for no fpu even if cpuid is supported */
		cpu_probe_old_fpu(ci);
		return;
	}

	i386_use_fxsave = 1;
	/*
	 * If we have SSE/SSE2, enable XMM exceptions, and
	 * notify userland.
	 */
	if (ci->ci_feat_val[0] & CPUID_SSE)
		i386_has_sse = 1;
	if (ci->ci_feat_val[0] & CPUID_SSE2)
		i386_has_sse2 = 1;
#else
	/*
	 * For amd64 i386_use_fxsave, i386_has_sse and i386_has_sse2 are
	 * #defined to 1.
	 */
#endif	/* i386 */

	x86_fpu_save = FPU_SAVE_FXSAVE;

	/* See if xsave (for AVX) is supported */
	if ((ci->ci_feat_val[1] & CPUID2_XSAVE) == 0)
		return;

	x86_fpu_save = FPU_SAVE_XSAVE;

	/* xsaveopt ought to be faster than xsave */
	x86_cpuid2(0xd, 1, descs);
	if (descs[0] & CPUID_PES1_XSAVEOPT)
		x86_fpu_save = FPU_SAVE_XSAVEOPT;

	/* Get features and maximum size of the save area */
	x86_cpuid(0xd, descs);
	if (descs[2] > 512)
		x86_fpu_save_size = descs[2];

#ifdef XEN
	/* Don't use xsave, force fxsave with x86_xsave_features = 0. */
	x86_fpu_save = FPU_SAVE_FXSAVE;
#else
	x86_xsave_features = (uint64_t)descs[3] << 32 | descs[0];
#endif
}

void
cpu_probe(struct cpu_info *ci)
{
	u_int descs[4];
	int i;
	uint32_t miscbytes;
	uint32_t brand[12];

	cpu_vendor = i386_nocpuid_cpus[cputype << 1];
	cpu_class = i386_nocpuid_cpus[(cputype << 1) + 1];

	if (cpuid_level < 0) {
		/* cpuid instruction not supported */
		cpu_probe_old_fpu(ci);
		return;
	}

	for (i = 0; i < __arraycount(ci->ci_feat_val); i++) {
		ci->ci_feat_val[i] = 0;
	}

	x86_cpuid(0, descs);
	cpuid_level = descs[0];
	ci->ci_max_cpuid = descs[0];

	ci->ci_vendor[0] = descs[1];
	ci->ci_vendor[2] = descs[2];
	ci->ci_vendor[1] = descs[3];
	ci->ci_vendor[3] = 0;

	if (memcmp(ci->ci_vendor, "GenuineIntel", 12) == 0)
		cpu_vendor = CPUVENDOR_INTEL;
	else if (memcmp(ci->ci_vendor,  "AuthenticAMD", 12) == 0)
		cpu_vendor = CPUVENDOR_AMD;
	else if (memcmp(ci->ci_vendor,  "CyrixInstead", 12) == 0)
		cpu_vendor = CPUVENDOR_CYRIX;
	else if (memcmp(ci->ci_vendor,  "Geode by NSC", 12) == 0)
		cpu_vendor = CPUVENDOR_CYRIX;
	else if (memcmp(ci->ci_vendor, "CentaurHauls", 12) == 0)
		cpu_vendor = CPUVENDOR_IDT;
	else if (memcmp(ci->ci_vendor, "GenuineTMx86", 12) == 0)
		cpu_vendor = CPUVENDOR_TRANSMETA;
	else if (memcmp(ci->ci_vendor, "Vortex86 SoC", 12) == 0)
		cpu_vendor = CPUVENDOR_VORTEX86;
	else
		cpu_vendor = CPUVENDOR_UNKNOWN;

	if (cpuid_level >= 1) {
		x86_cpuid(1, descs);
		ci->ci_signature = descs[0];
		miscbytes = descs[1];
		ci->ci_feat_val[1] = descs[2];
		ci->ci_feat_val[0] = descs[3];

		/* Determine family + class. */
		cpu_class = CPUID_TO_FAMILY(ci->ci_signature)
		    + (CPUCLASS_386 - 3);
		if (cpu_class > CPUCLASS_686)
			cpu_class = CPUCLASS_686;

		/* CLFLUSH line size is next 8 bits */
		if (ci->ci_feat_val[0] & CPUID_CFLUSH)
			ci->ci_cflush_lsize
			    = __SHIFTOUT(miscbytes, CPUID_CLFUSH_SIZE) << 3;
		ci->ci_initapicid = __SHIFTOUT(miscbytes, CPUID_LOCAL_APIC_ID);
	}

	/*
	 * Get the basic information from the extended cpuid leafs.
	 * These were first implemented by amd, but most of the values
	 * match with those generated by modern intel cpus.
	 */
	x86_cpuid(0x80000000, descs);
	if (descs[0] >= 0x80000000)
		ci->ci_max_ext_cpuid = descs[0];
	else
		ci->ci_max_ext_cpuid = 0;

	if (ci->ci_max_ext_cpuid >= 0x80000001) {
		/* Determine the extended feature flags. */
		x86_cpuid(0x80000001, descs);
		ci->ci_feat_val[3] = descs[2]; /* %ecx */
		ci->ci_feat_val[2] = descs[3]; /* %edx */
	}

	if (ci->ci_max_ext_cpuid >= 0x80000004) {
		x86_cpuid(0x80000002, brand);
		x86_cpuid(0x80000003, brand + 4);
		x86_cpuid(0x80000004, brand + 8);
		/* Skip leading spaces on brand */
		for (i = 0; i < 48; i++) {
			if (((char *) brand)[i] != ' ')
				break;
		}
		memcpy(cpu_brand_string, ((char *) brand) + i, 48 - i);
	}

	/*
	 * Get the structured extended features.
	 */
	if (cpuid_level >= 7) {
		x86_cpuid(7, descs);
		ci->ci_feat_val[5] = descs[1]; /* %ebx */
		ci->ci_feat_val[6] = descs[2]; /* %ecx */
	}

	cpu_probe_intel(ci);
	cpu_probe_k5(ci);
	cpu_probe_k678(ci);
	cpu_probe_cyrix(ci);
	cpu_probe_winchip(ci);
	cpu_probe_c3(ci);
	cpu_probe_geode(ci);
	cpu_probe_vortex86(ci);

	cpu_probe_fpu(ci);

	x86_cpu_topology(ci);

	if (cpu_vendor != CPUVENDOR_AMD && (ci->ci_feat_val[0] & CPUID_TM) &&
	    (rdmsr(MSR_MISC_ENABLE) & (1 << 3)) == 0) {
		/* Enable thermal monitor 1. */
		wrmsr(MSR_MISC_ENABLE, rdmsr(MSR_MISC_ENABLE) | (1<<3));
	}

	ci->ci_feat_val[0] &= ~CPUID_FEAT_BLACKLIST;
	if (ci == &cpu_info_primary) {
		/* If first. Boot Processor is the cpu_feature reference. */
		for (i = 0; i < __arraycount(cpu_feature); i++) {
			cpu_feature[i] = ci->ci_feat_val[i];
		}
		identify_hypervisor();
#ifndef XEN
		/* Early patch of text segment. */
		x86_patch(true);
#endif
	} else {
		/*
		 * If not first. Warn about cpu_feature mismatch for
		 * secondary CPUs.
		 */
		for (i = 0; i < __arraycount(cpu_feature); i++) {
			if (cpu_feature[i] != ci->ci_feat_val[i])
				aprint_error_dev(ci->ci_dev,
				    "feature mismatch: cpu_feature[%d] is "
				    "%#x, but CPU reported %#x\n",
				    i, cpu_feature[i], ci->ci_feat_val[i]);
		}
	}
}

/* Write what we know about the cpu to the console... */
void
cpu_identify(struct cpu_info *ci)
{

	cpu_setmodel("%s %d86-class",
	    cpu_vendor_names[cpu_vendor], cpu_class + 3);
	if (cpu_brand_string[0] != '\0') {
		aprint_normal_dev(ci->ci_dev, "%s", cpu_brand_string);
	} else {
		aprint_normal_dev(ci->ci_dev, "%s", cpu_getmodel());
		if (ci->ci_data.cpu_cc_freq != 0)
			aprint_normal(", %dMHz",
			    (int)(ci->ci_data.cpu_cc_freq / 1000000));
	}
	if (ci->ci_signature != 0)
		aprint_normal(", id 0x%x", ci->ci_signature);
	aprint_normal("\n");
	aprint_normal_dev(ci->ci_dev, "package %lu, core %lu, smt %lu\n",
	    ci->ci_package_id, ci->ci_core_id, ci->ci_smt_id);
	if (cpu_brand_string[0] == '\0') {
		strlcpy(cpu_brand_string, cpu_getmodel(),
		    sizeof(cpu_brand_string));
	}
	if (cpu_class == CPUCLASS_386) {
		panic("NetBSD requires an 80486DX or later processor");
	}
	if (cputype == CPU_486DLC) {
		aprint_error("WARNING: BUGGY CYRIX CACHE\n");
	}

#if !defined(XEN) || defined(DOM0OPS)       /* on Xen rdmsr is for Dom0 only */
	if (cpu_vendor == CPUVENDOR_AMD     /* check enablement of an */
	    && device_unit(ci->ci_dev) == 0 /* AMD feature only once */
	    && ((cpu_feature[3] & CPUID_SVM) == CPUID_SVM)) {
		uint64_t val;

		val = rdmsr(MSR_VMCR);
		if (((val & VMCR_SVMED) == VMCR_SVMED)
		    && ((val & VMCR_LOCK) == VMCR_LOCK)) {
			aprint_normal_dev(ci->ci_dev,
				"SVM disabled by the BIOS\n");
		}
	}
#endif

#ifdef i386
	if (i386_fpu_fdivbug == 1)
		aprint_normal_dev(ci->ci_dev,
		    "WARNING: Pentium FDIV bug detected!\n");

	if (cpu_vendor == CPUVENDOR_TRANSMETA) {
		u_int descs[4];
		x86_cpuid(0x80860000, descs);
		if (descs[0] >= 0x80860007)
			/* Create longrun sysctls */
			tmx86_init_longrun();
	}
#endif	/* i386 */

}

/*
 * Hypervisor
 */
vm_guest_t vm_guest = VM_GUEST_NO;

static const char * const vm_bios_vendors[] = {
	"QEMU",				/* QEMU */
	"Plex86",			/* Plex86 */
	"Bochs",			/* Bochs */
	"Xen",				/* Xen */
	"BHYVE",			/* bhyve */
	"Seabios",			/* KVM */
};

static const char * const vm_system_products[] = {
	"VMware Virtual Platform",	/* VMWare VM */
	"Virtual Machine",		/* Microsoft VirtualPC */
	"VirtualBox",			/* Sun xVM VirtualBox */
	"Parallels Virtual Platform",	/* Parallels VM */
	"KVM",				/* KVM */
};

void
identify_hypervisor(void)
{
	u_int regs[6];
	char hv_vendor[12];
	const char *p;
	int i;

	if (vm_guest != VM_GUEST_NO)
		return;

	/*
	 * [RFC] CPUID usage for interaction between Hypervisors and Linux.
	 * http://lkml.org/lkml/2008/10/1/246
	 *
	 * KB1009458: Mechanisms to determine if software is running in
	 * a VMware virtual machine
	 * http://kb.vmware.com/kb/1009458
	 */
	if (ISSET(cpu_feature[1], CPUID2_RAZ)) {
		vm_guest = VM_GUEST_VM;
		x86_cpuid(0x40000000, regs);
		if (regs[0] >= 0x40000000) {
			memcpy(&hv_vendor[0], &regs[1], sizeof(*regs));
			memcpy(&hv_vendor[4], &regs[2], sizeof(*regs));
			memcpy(&hv_vendor[8], &regs[3], sizeof(*regs));
			if (memcmp(hv_vendor, "VMwareVMware", 12) == 0)
				vm_guest = VM_GUEST_VMWARE;
			else if (memcmp(hv_vendor, "Microsoft Hv", 12) == 0)
				vm_guest = VM_GUEST_HV;
			else if (memcmp(hv_vendor, "KVMKVMKVM\0\0\0", 12) == 0)
				vm_guest = VM_GUEST_KVM;
			/* FreeBSD bhyve: "bhyve bhyve " */
			/* OpenBSD vmm:   "OpenBSDVMM58" */
		}
		return;
	}

	/*
	 * Examine SMBIOS strings for older hypervisors.
	 */
	p = pmf_get_platform("system-serial");
	if (p != NULL) {
		if (strncmp(p, "VMware-", 7) == 0 || strncmp(p, "VMW", 3) == 0) {
			vmt_hvcall(VM_CMD_GET_VERSION, regs);
			if (regs[1] == VM_MAGIC) {
				vm_guest = VM_GUEST_VMWARE;
				return;
			}
		}
	}
	p = pmf_get_platform("bios-vendor");
	if (p != NULL) {
		for (i = 0; i < __arraycount(vm_bios_vendors); i++) {
			if (strcmp(p, vm_bios_vendors[i]) == 0) {
				vm_guest = VM_GUEST_VM;
				return;
			}
		}
	}
	p = pmf_get_platform("system-product");
	if (p != NULL) {
		for (i = 0; i < __arraycount(vm_system_products); i++) {
			if (strcmp(p, vm_system_products[i]) == 0) {
				vm_guest = VM_GUEST_VM;
				return;
			}
		}
	}
}
