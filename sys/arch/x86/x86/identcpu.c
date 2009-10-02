/*	$NetBSD: identcpu.c,v 1.17 2009/10/02 18:50:03 jmcneill Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: identcpu.c,v 1.17 2009/10/02 18:50:03 jmcneill Exp $");

#include "opt_enhanced_speedstep.h"
#include "opt_intel_odcm.h"
#include "opt_intel_coretemp.h"
#include "opt_via_c7temp.h"
#include "opt_powernow_k8.h"
#include "opt_xen.h"
#ifdef i386	/* XXX */
#include "opt_powernow_k7.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/specialreg.h>
#include <machine/pio.h>
#include <machine/cpu.h>

#include <x86/cputypes.h>
#include <x86/cacheinfo.h>
#include <x86/cpuvar.h>
#include <x86/cpu_msr.h>
#include <x86/powernow.h>

static const struct x86_cache_info intel_cpuid_cache_info[] = INTEL_CACHE_INFO;

static const struct x86_cache_info amd_cpuid_l2cache_assoc_info[] = 
	AMD_L2CACHE_INFO;

static const struct x86_cache_info amd_cpuid_l3cache_assoc_info[] = 
	AMD_L3CACHE_INFO;

int cpu_vendor;
char cpu_brand_string[49];

/*
 * Info for CTL_HW
 */
char	cpu_model[120];

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
	"Unknown", "Intel", "NS/Cyrix", "NexGen", "AMD", "IDT/VIA", "Transmeta"
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
cpu_probe_amd_cache(struct cpu_info *ci)
{
	const struct x86_cache_info *cp;
	struct x86_cache_info *cai;
	int family, model;
	u_int descs[4];
	u_int lfunc;

	family = CPUID2FAMILY(ci->ci_signature);
	model = CPUID2MODEL(ci->ci_signature);

	/*
	 * K5 model 0 has none of this info.
	 */
	if (family == 5 && model == 0)
		return;

	/*
	 * Get extended values for K8 and up.
	 */
	if (family == 0xf) {
		family += CPUID2EXTFAMILY(ci->ci_signature);
		model += CPUID2EXTMODEL(ci->ci_signature);
	}

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
	cai->cai_linesize = AMD_L1_EDX_IC_LS(descs[2]);

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
	    CPUID2FAMILY(ci->ci_signature) != 5)
		return;

	if (CPUID2MODEL(ci->ci_signature) == 0) {
		/*
		 * According to the AMD Processor Recognition App Note,
		 * the AMD-K5 Model 0 uses the wrong bit to indicate
		 * support for global PTEs, instead using bit 9 (APIC)
		 * rather than bit 13 (i.e. "0x200" vs. 0x2000".  Oops!).
		 */
		flag = ci->ci_feature_flags;
		if ((flag & CPUID_APIC) != 0)
			flag = (flag & ~CPUID_APIC) | CPUID_PGE;
		ci->ci_feature_flags = flag;
	}

	cpu_probe_amd_cache(ci);
}

static void
cpu_probe_k678(struct cpu_info *ci)
{
	uint32_t descs[4];

	if (cpu_vendor != CPUVENDOR_AMD ||
	    CPUID2FAMILY(ci->ci_signature) < 6)
		return;

	/* Determine the extended feature flags. */
	x86_cpuid(0x80000000, descs);
	if (descs[0] >= 0x80000001) {
		x86_cpuid(0x80000001, descs);
		ci->ci_feature3_flags |= descs[3]; /* %edx */
		ci->ci_feature4_flags = descs[2];  /* %ecx */
	}

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
		ci->ci_feature_flags &= ~CPUID_TSC;

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
	    CPUID2FAMILY(ci->ci_signature) < 4 ||
	    CPUID2FAMILY(ci->ci_signature) > 6)
		return;

	cpu_probe_cyrix_cmn(ci);
}

static void
cpu_probe_winchip(struct cpu_info *ci)
{

	if (cpu_vendor != CPUVENDOR_IDT ||
	    CPUID2FAMILY(ci->ci_signature) != 5)
	    	return;

	if (CPUID2MODEL(ci->ci_signature) == 4) {
		/* WinChip C6 */
		ci->ci_feature_flags &= ~CPUID_TSC;
	}
}

static void
cpu_probe_c3(struct cpu_info *ci)
{
	u_int family, model, stepping, descs[4], lfunc, msr;
	struct x86_cache_info *cai;

	if (cpu_vendor != CPUVENDOR_IDT ||
	    CPUID2FAMILY(ci->ci_signature) < 6)
	    	return;

	family = CPUID2FAMILY(ci->ci_signature);
	model = CPUID2MODEL(ci->ci_signature);
	stepping = CPUID2STEPPING(ci->ci_signature);

	/* Determine the largest extended function value. */
	x86_cpuid(0x80000000, descs);
	lfunc = descs[0];

	/* Determine the extended feature flags. */
	if (lfunc >= 0x80000001) {
		x86_cpuid(0x80000001, descs);
		ci->ci_feature_flags |= descs[3];
	}

	if (family > 6 || model > 0x9 || (model == 0x9 && stepping >= 3)) {
		/* Nehemiah or Esther */
		x86_cpuid(0xc0000000, descs);
		lfunc = descs[0];
		if (lfunc >= 0xc0000001) {	/* has ACE, RNG */
		    int rng_enable = 0, ace_enable = 0;
		    x86_cpuid(0xc0000001, descs);
		    lfunc = descs[3];
		    ci->ci_padlock_flags = lfunc;
		    /* Check for and enable RNG */
		    if (lfunc & CPUID_VIA_HAS_RNG) {
		    	if (!(lfunc & CPUID_VIA_DO_RNG)) {
			    rng_enable++;
			    ci->ci_padlock_flags |= CPUID_VIA_HAS_RNG;
			}
		    }
		    /* Check for and enable ACE (AES-CBC) */
		    if (lfunc & CPUID_VIA_HAS_ACE) {
			if (!(lfunc & CPUID_VIA_DO_ACE)) {
			    ace_enable++;
			    ci->ci_padlock_flags |= CPUID_VIA_DO_ACE;
			}
		    }
		    /* Check for and enable SHA */
		    if (lfunc & CPUID_VIA_HAS_PHE) {
			if (!(lfunc & CPUID_VIA_DO_PHE)) {
			    ace_enable++;
			    ci->ci_padlock_flags |= CPUID_VIA_DO_PHE;
			}
		    }
		    /* Check for and enable ACE2 (AES-CTR) */
		    if (lfunc & CPUID_VIA_HAS_ACE2) {
			if (!(lfunc & CPUID_VIA_DO_ACE2)) {
			    ace_enable++;
			    ci->ci_padlock_flags |= CPUID_VIA_DO_ACE2;
			}
		    }
		    /* Check for and enable PMM (modmult engine) */
		    if (lfunc & CPUID_VIA_HAS_PMM) {
			if (!(lfunc & CPUID_VIA_DO_PMM)) {
			    ace_enable++;
			    ci->ci_padlock_flags |= CPUID_VIA_DO_PMM;
			}
		    }

		    /* Actually do the enables. */
		    if (rng_enable) {
			msr = rdmsr(MSR_VIA_RNG);
			wrmsr(MSR_VIA_RNG, msr | MSR_VIA_RNG_ENABLE);
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
	    CPUID2FAMILY(ci->ci_signature) != 5)
	    	return;

	cpu_probe_cyrix_cmn(ci);
	cpu_probe_amd_cache(ci);
}

void
cpu_probe(struct cpu_info *ci)
{
	const struct x86_cache_info *cai;
	u_int descs[4];
	int iterations, i, j;
	uint8_t desc;
	uint32_t miscbytes;
	uint32_t brand[12];

	cpu_vendor = i386_nocpuid_cpus[cpu << 1];
	cpu_class = i386_nocpuid_cpus[(cpu << 1) + 1];

	if (cpuid_level < 0)
		return;

	x86_cpuid(0, descs);
	cpuid_level = descs[0];
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
	else
		cpu_vendor = CPUVENDOR_UNKNOWN;

	x86_cpuid(0x80000000, brand);
	if (brand[0] >= 0x80000004) {
		x86_cpuid(0x80000002, brand);
		x86_cpuid(0x80000003, brand + 4);
		x86_cpuid(0x80000004, brand + 8);
		for (i = 0; i < 48; i++) {
			if (((char *) brand)[i] != ' ')
				break;
		}
		memcpy(cpu_brand_string, ((char *) brand) + i, 48 - i);
	}

	if (cpuid_level >= 1) {
		x86_cpuid(1, descs);
		ci->ci_signature = descs[0];
		miscbytes = descs[1];
		ci->ci_feature2_flags = descs[2];
		ci->ci_feature_flags = descs[3];

		/* Determine family + class. */
		cpu_class = CPUID2FAMILY(ci->ci_signature) + (CPUCLASS_386 - 3);
		if (cpu_class > CPUCLASS_686)
			cpu_class = CPUCLASS_686;

		/* CLFLUSH line size is next 8 bits */
		if (ci->ci_feature_flags & CPUID_CFLUSH)
			ci->ci_cflush_lsize = ((miscbytes >> 8) & 0xff) << 3;
		ci->ci_initapicid = (miscbytes >> 24) & 0xff;
	}

	if (cpuid_level >= 2) { 
		/* Parse the cache info from `cpuid', if we have it. */
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

	cpu_probe_k5(ci);
	cpu_probe_k678(ci);
	cpu_probe_cyrix(ci);
	cpu_probe_winchip(ci);
	cpu_probe_c3(ci);
	cpu_probe_geode(ci);

	x86_cpu_toplogy(ci);

	if (cpu_vendor != CPUVENDOR_AMD && (ci->ci_feature_flags & CPUID_TM) &&
	    (rdmsr(MSR_MISC_ENABLE) & (1 << 3)) == 0) {
		/* Enable thermal monitor 1. */
		wrmsr(MSR_MISC_ENABLE, rdmsr(MSR_MISC_ENABLE) | (1<<3));
	}

	if ((cpu_feature | cpu_feature2) == 0) {
		/* If first. */
		cpu_feature = ci->ci_feature_flags;
		cpu_feature2 = ci->ci_feature2_flags;
		/* Early patch of text segment. */
#ifndef XEN
		x86_patch(true);
#endif
	} else {
		/* If not first. */
		cpu_feature &= ci->ci_feature_flags;
		cpu_feature2 &= ci->ci_feature2_flags;
	}
}

void
cpu_identify(struct cpu_info *ci)
{

	snprintf(cpu_model, sizeof(cpu_model), "%s %d86-class",
	    cpu_vendor_names[cpu_vendor], cpu_class + 3);
	aprint_normal(": %s", cpu_model);
	if (ci->ci_data.cpu_cc_freq != 0)
		aprint_normal(", %dMHz", (int)(ci->ci_data.cpu_cc_freq / 1000000));
	if (ci->ci_signature != 0)
		aprint_normal(", id 0x%x", ci->ci_signature);
	aprint_normal("\n");

	if (cpu_brand_string[0] == '\0') {
		strlcpy(cpu_brand_string, cpu_model, sizeof(cpu_brand_string));
	}
	if (cpu_class == CPUCLASS_386) {
		panic("NetBSD requires an 80486DX or later processor");
	}
	if (cpu == CPU_486DLC) {
		aprint_error("WARNING: BUGGY CYRIX CACHE\n");
	}

	if ((cpu_vendor == CPUVENDOR_AMD) /* check enablement of an */
	  && (device_unit(ci->ci_dev) == 0) /* AMD feature only once */
	  && ((ci->ci_feature4_flags & CPUID_SVM) == CPUID_SVM)
#if defined(XEN) && !defined(DOM0OPS)
	  && (false)  /* on Xen rdmsr is for Dom0 only */
#endif
	  )
	{
		uint64_t val;

		val = rdmsr(MSR_VMCR);
		if (((val & VMCR_SVMED) == VMCR_SVMED)
		  && ((val & VMCR_LOCK) == VMCR_LOCK))
		{
			aprint_normal_dev(ci->ci_dev,
				"SVM disabled by the BIOS\n");
		}
	}

#ifdef i386 /* XXX for now */
	if (cpu_vendor == CPUVENDOR_TRANSMETA) {
		u_int descs[4];
		x86_cpuid(0x80860000, descs);
		if (descs[0] >= 0x80860007)
			tmx86_init_longrun();
	}

	/* If we have FXSAVE/FXRESTOR, use them. */
	if (cpu_feature & CPUID_FXSR) {
		i386_use_fxsave = 1;
		/*
		 * If we have SSE/SSE2, enable XMM exceptions, and
		 * notify userland.
		 */
		if (cpu_feature & CPUID_SSE)
			i386_has_sse = 1;
		if (cpu_feature & CPUID_SSE2)
			i386_has_sse2 = 1;
	} else
		i386_use_fxsave = 0;
#endif	/* i386 */

#ifdef ENHANCED_SPEEDSTEP
	if (cpu_feature2 & CPUID2_EST) {
		if (rdmsr(MSR_MISC_ENABLE) & (1 << 16))
			est_init(cpu_vendor);
	}
#endif /* ENHANCED_SPEEDSTEP */

#ifdef INTEL_CORETEMP
	if (cpu_vendor == CPUVENDOR_INTEL && cpuid_level >= 0x06)
		coretemp_register(ci);
#endif

#ifdef VIA_C7TEMP
	if (cpu_vendor == CPUVENDOR_IDT &&
	    CPUID2FAMILY(ci->ci_signature) == 6 &&
	    CPUID2MODEL(ci->ci_signature) >= 0x9) {
		uint32_t descs[4];

		x86_cpuid(0xc0000000, descs);
		if (descs[0] >= 0xc0000002)	/* has temp sensor */
			viac7temp_register(ci);
	}
#endif

#if defined(POWERNOW_K7) || defined(POWERNOW_K8)
	if (cpu_vendor == CPUVENDOR_AMD && powernow_probe(ci)) {
		switch (CPUID2FAMILY(ci->ci_signature)) {
#ifdef POWERNOW_K7
		case 6:
			k7_powernow_init();
			break;
#endif
#ifdef POWERNOW_K8
		case 15:
			k8_powernow_init();
			break;
#endif
		default:
			break;
		}
	}
#endif /* POWERNOW_K7 || POWERNOW_K8 */

#ifdef INTEL_ONDEMAND_CLOCKMOD
	if (cpuid_level >= 1) {
		clockmod_init();
	}
#endif
}
