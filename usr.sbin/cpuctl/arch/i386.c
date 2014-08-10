/*	$NetBSD: i386.c,v 1.54.2.1 2014/08/10 06:59:25 tls Exp $	*/

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

/*-
 * Copyright (c)2008 YAMAMOTO Takashi,
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
#ifndef lint
__RCSID("$NetBSD: i386.c,v 1.54.2.1 2014/08/10 06:59:25 tls Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/cpuio.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>
#include <math.h>
#include <util.h>

#include <machine/specialreg.h>
#include <machine/cpu.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/cacheinfo.h>
#include <x86/cpu_ucode.h>

#include "../cpuctl.h"
#include "cpuctl_i386.h"

/* Size of buffer for printing humanized numbers */
#define HUMAN_BUFSIZE sizeof("999KB")

struct cpu_info {
	const char	*ci_dev;
	int32_t		ci_cpu_type;     /* for cpu's without cpuid */
	int32_t		ci_cpuid_level;	 /* highest cpuid supported */
	uint32_t	ci_cpuid_extlevel; /* highest cpuid extended func lv */
	uint32_t	ci_signature;	 /* X86 cpuid type */
	uint32_t	ci_family;	 /* from ci_signature */
	uint32_t	ci_model;	 /* from ci_signature */
	uint32_t	ci_feat_val[8];	 /* X86 CPUID feature bits
					  *	[0] basic features %edx
					  *	[1] basic features %ecx
					  *	[2] extended features %edx
					  *	[3] extended features %ecx
					  *	[4] VIA padlock features
					  *	[5] XCR0 bits (d:0 %eax)
					  *	[6] xsave flags (d:1 %eax)
					  */
	uint32_t	ci_cpu_class;	 /* CPU class */
	uint32_t	ci_brand_id;	 /* Intel brand id */
	uint32_t	ci_vendor[4];	 /* vendor string */
	uint32_t	ci_cpu_serial[3]; /* PIII serial number */
	uint64_t	ci_tsc_freq;	 /* cpu cycles/second */
	uint8_t		ci_packageid;
	uint8_t		ci_coreid;
	uint8_t		ci_smtid;
	uint32_t	ci_initapicid;

	uint32_t	ci_cur_xsave;
	uint32_t	ci_max_xsave;

	struct x86_cache_info ci_cinfo[CAI_COUNT];
	void		(*ci_info)(struct cpu_info *);
};

struct cpu_nocpuid_nameclass {
	int cpu_vendor;
	const char *cpu_vendorname;
	const char *cpu_name;
	int cpu_class;
	void (*cpu_setup)(struct cpu_info *);
	void (*cpu_cacheinfo)(struct cpu_info *);
	void (*cpu_info)(struct cpu_info *);
};

struct cpu_cpuid_nameclass {
	const char *cpu_id;
	int cpu_vendor;
	const char *cpu_vendorname;
	struct cpu_cpuid_family {
		int cpu_class;
		const char *cpu_models[256];
		const char *cpu_model_default;
		void (*cpu_setup)(struct cpu_info *);
		void (*cpu_probe)(struct cpu_info *);
		void (*cpu_info)(struct cpu_info *);
	} cpu_family[CPU_MAXFAMILY - CPU_MINFAMILY + 1];
};

static const struct x86_cache_info intel_cpuid_cache_info[] = INTEL_CACHE_INFO;

/*
 * Map Brand ID from cpuid instruction to brand name.
 * Source: Table 3-24, Mapping of Brand Indices; and Intel 64 and IA-32
 * Processor Brand Strings, Chapter 3 in "Intel (R) 64 and IA-32
 * Architectures Software Developer's Manual, Volume 2A".
 */
static const char * const i386_intel_brand[] = {
	"",		    /* Unsupported */
	"Celeron",	    /* Intel (R) Celeron (TM) processor */
	"Pentium III",      /* Intel (R) Pentium (R) III processor */
	"Pentium III Xeon", /* Intel (R) Pentium (R) III Xeon (TM) processor */
	"Pentium III",      /* Intel (R) Pentium (R) III processor */
	"",		    /* 0x05: Reserved */
	"Mobile Pentium III", /* Mobile Intel (R) Pentium (R) III processor-M */
	"Mobile Celeron",   /* Mobile Intel (R) Celeron (R) processor */    
	"Pentium 4",	    /* Intel (R) Pentium (R) 4 processor */
	"Pentium 4",	    /* Intel (R) Pentium (R) 4 processor */
	"Celeron",	    /* Intel (R) Celeron (TM) processor */
	"Xeon",		    /* Intel (R) Xeon (TM) processor */
	"Xeon MP",	    /* Intel (R) Xeon (TM) processor MP */
	"",		    /* 0x0d: Reserved */
	"Mobile Pentium 4", /* Mobile Intel (R) Pentium (R) 4 processor-M */
	"Mobile Celeron",   /* Mobile Intel (R) Celeron (R) processor */
	"",		    /* 0x10: Reserved */
	"Mobile Genuine",   /* Moblie Genuine Intel (R) processor */
	"Celeron M",        /* Intel (R) Celeron (R) M processor */
	"Mobile Celeron",   /* Mobile Intel (R) Celeron (R) processor */
	"Celeron",          /* Intel (R) Celeron (R) processor */
	"Mobile Genuine",   /* Moblie Genuine Intel (R) processor */
	"Pentium M",        /* Intel (R) Pentium (R) M processor */
	"Mobile Celeron",   /* Mobile Intel (R) Celeron (R) processor */
};

/*
 * AMD processors don't have Brand IDs, so we need these names for probe.
 */
static const char * const amd_brand[] = {
	"",
	"Duron",	/* AMD Duron(tm) */
	"MP",		/* AMD Athlon(tm) MP */
	"XP",		/* AMD Athlon(tm) XP */
	"4"		/* AMD Athlon(tm) 4 */
};

static int cpu_vendor;
static char cpu_brand_string[49];
static char amd_brand_name[48];
static int use_pae, largepagesize;

/* Setup functions */
static void	disable_tsc(struct cpu_info *);
static void	amd_family5_setup(struct cpu_info *);
static void	cyrix6x86_cpu_setup(struct cpu_info *);
static void	winchip_cpu_setup(struct cpu_info *);
/* Brand/Model name functions */
static const char *intel_family6_name(struct cpu_info *);
static const char *amd_amd64_name(struct cpu_info *);
/* Probe functions */
static void	amd_family6_probe(struct cpu_info *);
static void	powernow_probe(struct cpu_info *);
static void	intel_family_new_probe(struct cpu_info *);
static void	via_cpu_probe(struct cpu_info *);
/* (Cache) Info functions */
static void 	intel_cpu_cacheinfo(struct cpu_info *);
static void 	amd_cpu_cacheinfo(struct cpu_info *);
static void	via_cpu_cacheinfo(struct cpu_info *);
static void	tmx86_get_longrun_status(u_int *, u_int *, u_int *);
static void	transmeta_cpu_info(struct cpu_info *);
/* Common functions */
static void	cpu_probe_base_features(struct cpu_info *, const char *);
static void	cpu_probe_features(struct cpu_info *);
static void	print_bits(const char *, const char *, const char *, uint32_t);
static void	identifycpu_cpuids(struct cpu_info *);
static const struct x86_cache_info *cache_info_lookup(
    const struct x86_cache_info *, uint8_t);
static const char *print_cache_config(struct cpu_info *, int, const char *,
    const char *);
static const char *print_tlb_config(struct cpu_info *, int, const char *,
    const char *);
static void	x86_print_cache_and_tlb_info(struct cpu_info *);

/*
 * Note: these are just the ones that may not have a cpuid instruction.
 * We deal with the rest in a different way.
 */
const struct cpu_nocpuid_nameclass i386_nocpuid_cpus[] = {
	{ CPUVENDOR_INTEL, "Intel", "386SX",	CPUCLASS_386,
	  NULL, NULL, NULL },			/* CPU_386SX */
	{ CPUVENDOR_INTEL, "Intel", "386DX",	CPUCLASS_386,
	  NULL, NULL, NULL },			/* CPU_386   */
	{ CPUVENDOR_INTEL, "Intel", "486SX",	CPUCLASS_486,
	  NULL, NULL, NULL },			/* CPU_486SX */
	{ CPUVENDOR_INTEL, "Intel", "486DX",	CPUCLASS_486,
	  NULL, NULL, NULL },			/* CPU_486   */
	{ CPUVENDOR_CYRIX, "Cyrix", "486DLC",	CPUCLASS_486,
	  NULL, NULL, NULL },			/* CPU_486DLC */
	{ CPUVENDOR_CYRIX, "Cyrix", "6x86",	CPUCLASS_486,
	  NULL, NULL, NULL },		/* CPU_6x86 */
	{ CPUVENDOR_NEXGEN,"NexGen","586",      CPUCLASS_386,
	  NULL, NULL, NULL },			/* CPU_NX586 */
};

const char *classnames[] = {
	"386",
	"486",
	"586",
	"686"
};

const char *modifiers[] = {
	"",
	"OverDrive",
	"Dual",
	""
};

const struct cpu_cpuid_nameclass i386_cpuid_cpus[] = {
	{
		/*
		 * For Intel processors, check Chapter 35Model-specific
		 * registers (MSRS), in "Intel (R) 64 and IA-32 Architectures
		 * Software Developer's Manual, Volume 3C".
		 */
		"GenuineIntel",
		CPUVENDOR_INTEL,
		"Intel",
		/* Family 4 */
		{ {
			CPUCLASS_486,
			{
				"486DX", "486DX", "486SX", "486DX2", "486SL",
				"486SX2", 0, "486DX2 W/B Enhanced",
				"486DX4", 0, 0, 0, 0, 0, 0, 0,
			},
			"486",		/* Default */
			NULL,
			NULL,
			intel_cpu_cacheinfo,
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				"Pentium (P5 A-step)", "Pentium (P5)",
				"Pentium (P54C)", "Pentium (P24T)",
				"Pentium/MMX", "Pentium", 0,
				"Pentium (P54C)", "Pentium/MMX (Tillamook)",
				0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium",	/* Default */
			NULL,
			NULL,
			intel_cpu_cacheinfo,
		},
		/* Family 6 */
		{
			CPUCLASS_686,
			{
				[0x00] = "Pentium Pro (A-step)",
				[0x01] = "Pentium Pro",
				[0x03] = "Pentium II (Klamath)",
				[0x04] = "Pentium Pro",
				[0x05] = "Pentium II/Celeron (Deschutes)",
				[0x06] = "Celeron (Mendocino)",
				[0x07] = "Pentium III (Katmai)",
				[0x08] = "Pentium III (Coppermine)",
				[0x09] = "Pentium M (Banias)", 
				[0x0a] = "Pentium III Xeon (Cascades)",
				[0x0b] = "Pentium III (Tualatin)",
				[0x0d] = "Pentium M (Dothan)", 
				[0x0e] = "Pentium Core Duo, Core solo",
				[0x0f] = "Xeon 30xx, 32xx, 51xx, 53xx, 73xx, "
					 "Core 2 Quad 6xxx, "
					 "Core 2 Extreme 6xxx, "
					 "Core 2 Duo 4xxx, 5xxx, 6xxx, 7xxx "
					 "and Pentium DC",
				[0x15] = "EP80579 Integrated Processor",
				[0x16] = "Celeron (45nm)",
				[0x17] = "Xeon 31xx, 33xx, 52xx, 54xx, "
					 "Core 2 Quad 8xxx and 9xxx",
				[0x1a] = "Core i7, Xeon 34xx, 35xx and 55xx "
					 "(Nehalem)",
				[0x1c] = "Atom Family",
				[0x1d] = "XeonMP 74xx (Nehalem)",
				[0x1e] = "Core i7 and i5",
				[0x1f] = "Core i7 and i5",
				[0x25] = "Xeon 36xx & 56xx, i7, i5 and i3",
				[0x26] = "Atom Family",
				[0x27] = "Atom Family",
				[0x2a] = "Xeon E3-12xx, 2nd gen i7, i5, "
					 "i3 2xxx",
				[0x2c] = "Xeon 36xx & 56xx, i7, i5 and i3",
				[0x2d] = "Xeon E5 Sandy Bridge family, "
					 "Core i7-39xx Extreme",
				[0x2e] = "Xeon 75xx & 65xx",
				[0x2f] = "Xeon E7 family",
				[0x35] = "Atom Family",
				[0x36] = "Atom S1000",
				[0x37] = "Atom E3000, Z3000",
				[0x3a] = "Xeon E3-1200v2 and 3rd gen core, "
					 "Ivy Bridge",
				[0x3c] = "4th gen Core, Xeon E3-12xx v3 "
					 "(Haswell)",
				[0x3d] = "Core M based on Broadwell",
				[0x3e] = "Xeon E5/E7, Ivy Bridge-EP",
				[0x3f] = "Next gen Xeon based on Haswell",
				[0x45] = "4th gen Core, Xeon E3-12xx v3 "
					 "(Haswell)",
				[0x46] = "4th gen Core, Xeon E3-12xx v3 "
					 "(Haswell)",
				[0x4a] = "Future Atom E3000, Z3000",
				[0x4d] = "Atom C2000",
				[0x5a] = "Future Atom E3000, Z3000",
				[0x5d] = "Future Atom E3000, Z3000",
			},
			"Pentium Pro, II or III",	/* Default */
			NULL,
			intel_family_new_probe,
			intel_cpu_cacheinfo,
		},
		/* Family > 6 */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium 4",	/* Default */
			NULL,
			intel_family_new_probe,
			intel_cpu_cacheinfo,
		} }
	},
	{
		"AuthenticAMD",
		CPUVENDOR_AMD,
		"AMD",
		/* Family 4 */
		{ {
			CPUCLASS_486,
			{
				0, 0, 0, "Am486DX2 W/T",
				0, 0, 0, "Am486DX2 W/B",
				"Am486DX4 W/T or Am5x86 W/T 150",
				"Am486DX4 W/B or Am5x86 W/B 150", 0, 0,
				0, 0, "Am5x86 W/T 133/160",
				"Am5x86 W/B 133/160",
			},
			"Am486 or Am5x86",	/* Default */
			NULL,
			NULL,
			NULL,
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				"K5", "K5", "K5", "K5", 0, 0, "K6",
				"K6", "K6-2", "K6-III", "Geode LX", 0, 0,
				"K6-2+/III+", 0, 0,
			},
			"K5 or K6",		/* Default */
			amd_family5_setup,
			NULL,
			amd_cpu_cacheinfo,
		},
		/* Family 6 */
		{
			CPUCLASS_686,
			{
				0, "Athlon Model 1", "Athlon Model 2",
				"Duron", "Athlon Model 4 (Thunderbird)",
				0, "Athlon", "Duron", "Athlon", 0,
				"Athlon", 0, 0, 0, 0, 0,
			},
			"K7 (Athlon)",	/* Default */
			NULL,
			amd_family6_probe,
			amd_cpu_cacheinfo,
		},
		/* Family > 6 */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Unknown K8 (Athlon)",	/* Default */
			NULL,
			amd_family6_probe,
			amd_cpu_cacheinfo,
		} }
	},
	{
		"CyrixInstead",
		CPUVENDOR_CYRIX,
		"Cyrix",
		/* Family 4 */
		{ {
			CPUCLASS_486,
			{
				0, 0, 0,
				"MediaGX",
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			},
			"486",		/* Default */
			cyrix6x86_cpu_setup, /* XXX ?? */
			NULL,
			NULL,
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				0, 0, "6x86", 0,
				"MMX-enhanced MediaGX (GXm)", /* or Geode? */
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			},
			"6x86",		/* Default */
			cyrix6x86_cpu_setup,
			NULL,
			NULL,
		},
		/* Family 6 */
		{
			CPUCLASS_686,
			{
				"6x86MX", 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"6x86MX",		/* Default */
			cyrix6x86_cpu_setup,
			NULL,
			NULL,
		},
		/* Family > 6 */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Unknown 6x86MX",		/* Default */
			NULL,
			NULL,
			NULL,
		} }
	},
	{	/* MediaGX is now owned by National Semiconductor */
		"Geode by NSC",
		CPUVENDOR_CYRIX, /* XXX */
		"National Semiconductor",
		/* Family 4, NSC never had any of these */
		{ {
			CPUCLASS_486,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"486 compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		},
		/* Family 5: Geode family, formerly MediaGX */
		{
			CPUCLASS_586,
			{
				0, 0, 0, 0,
				"Geode GX1",
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Geode",		/* Default */
			cyrix6x86_cpu_setup,
			NULL,
			amd_cpu_cacheinfo,
		},
		/* Family 6, not yet available from NSC */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium Pro compatible", /* Default */
			NULL,
			NULL,
			NULL,
		},
		/* Family > 6, not yet available from NSC */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium Pro compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		} }
	},
	{
		"CentaurHauls",
		CPUVENDOR_IDT,
		"IDT",
		/* Family 4, IDT never had any of these */
		{ {
			CPUCLASS_486,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"486 compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				0, 0, 0, 0, "WinChip C6", 0, 0, 0,
				"WinChip 2", "WinChip 3", 0, 0, 0, 0, 0, 0,
			},
			"WinChip",		/* Default */
			winchip_cpu_setup,
			NULL,
			NULL,
		},
		/* Family 6, VIA acquired IDT Centaur design subsidiary */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, "C3 Samuel",
				"C3 Samuel 2/Ezra", "C3 Ezra-T",
				"C3 Nehemiah", "C7 Esther", 0, 0, "C7 Esther",
				0, "VIA Nano",
			},
			"Unknown VIA/IDT",	/* Default */
			NULL,
			via_cpu_probe,
			via_cpu_cacheinfo,
		},
		/* Family > 6, not yet available from VIA */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium Pro compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		} }
	},
	{
		"GenuineTMx86",
		CPUVENDOR_TRANSMETA,
		"Transmeta",
		/* Family 4, Transmeta never had any of these */
		{ {
			CPUCLASS_486,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"486 compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		},
		/* Family 5 */
		{
			CPUCLASS_586,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Crusoe",		/* Default */
			NULL,
			NULL,
			transmeta_cpu_info,
		},
		/* Family 6, not yet available from Transmeta */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium Pro compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		},
		/* Family > 6, not yet available from Transmeta */
		{
			CPUCLASS_686,
			{
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			},
			"Pentium Pro compatible",	/* Default */
			NULL,
			NULL,
			NULL,
		} }
	}
};

/*
 * disable the TSC such that we don't use the TSC in microtime(9)
 * because some CPUs got the implementation wrong.
 */
static void
disable_tsc(struct cpu_info *ci)
{
	if (ci->ci_feat_val[0] & CPUID_TSC) {
		ci->ci_feat_val[0] &= ~CPUID_TSC;
		aprint_error("WARNING: broken TSC disabled\n");
	}
}

static void
amd_family5_setup(struct cpu_info *ci)
{

	switch (ci->ci_model) {
	case 0:		/* AMD-K5 Model 0 */
		/*
		 * According to the AMD Processor Recognition App Note,
		 * the AMD-K5 Model 0 uses the wrong bit to indicate
		 * support for global PTEs, instead using bit 9 (APIC)
		 * rather than bit 13 (i.e. "0x200" vs. 0x2000".  Oops!).
		 */
		if (ci->ci_feat_val[0] & CPUID_APIC)
			ci->ci_feat_val[0] =
			    (ci->ci_feat_val[0] & ~CPUID_APIC) | CPUID_PGE;
		/*
		 * XXX But pmap_pg_g is already initialized -- need to kick
		 * XXX the pmap somehow.  How does the MP branch do this?
		 */
		break;
	}
}

static void
cyrix6x86_cpu_setup(struct cpu_info *ci)
{

	/* 
	 * Do not disable the TSC on the Geode GX, it's reported to
	 * work fine.
	 */
	if (ci->ci_signature != 0x552)
		disable_tsc(ci);
}

static void
winchip_cpu_setup(struct cpu_info *ci)
{
	switch (ci->ci_model) {
	case 4:	/* WinChip C6 */
		disable_tsc(ci);
	}
}


static const char *
intel_family6_name(struct cpu_info *ci)
{
	const char *ret = NULL;
	u_int l2cache = ci->ci_cinfo[CAI_L2CACHE].cai_totalsize;

	if (ci->ci_model == 5) {
		switch (l2cache) {
		case 0:
		case 128 * 1024:
			ret = "Celeron (Covington)";
			break;
		case 256 * 1024:
			ret = "Mobile Pentium II (Dixon)";
			break;
		case 512 * 1024:
			ret = "Pentium II";
			break;
		case 1 * 1024 * 1024:
		case 2 * 1024 * 1024:
			ret = "Pentium II Xeon";
			break;
		}
	} else if (ci->ci_model == 6) {
		switch (l2cache) {
		case 256 * 1024:
		case 512 * 1024:
			ret = "Mobile Pentium II";
			break;
		}
	} else if (ci->ci_model == 7) {
		switch (l2cache) {
		case 512 * 1024:
			ret = "Pentium III";
			break;
		case 1 * 1024 * 1024:
		case 2 * 1024 * 1024:
			ret = "Pentium III Xeon";
			break;
		}
	} else if (ci->ci_model >= 8) {
		if (ci->ci_brand_id && ci->ci_brand_id < 0x10) {
			switch (ci->ci_brand_id) {
			case 0x3:
				if (ci->ci_signature == 0x6B1)
					ret = "Celeron";
				break;
			case 0x8:
				if (ci->ci_signature >= 0xF13)
					ret = "genuine processor";
				break;
			case 0xB:
				if (ci->ci_signature >= 0xF13)
					ret = "Xeon MP";
				break;
			case 0xE:
				if (ci->ci_signature < 0xF13)
					ret = "Xeon";
				break;
			}
			if (ret == NULL)
				ret = i386_intel_brand[ci->ci_brand_id];
		}
	}

	return ret;
}

/*
 * Identify AMD64 CPU names from cpuid.
 *
 * Based on:
 * "Revision Guide for AMD Athlon 64 and AMD Opteron Processors"
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/25759.pdf
 * "Revision Guide for AMD NPT Family 0Fh Processors"
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/33610.pdf
 * and other miscellaneous reports.
 *
 * This is all rather pointless, these are cross 'brand' since the raw
 * silicon is shared.
 */
static const char *
amd_amd64_name(struct cpu_info *ci)
{
	static char family_str[32];

	/* Only called if family >= 15 */

	switch (ci->ci_family) {
	case 15:
		switch (ci->ci_model) {
		case 0x21:	/* rev JH-E1/E6 */
		case 0x41:	/* rev JH-F2 */
			return "Dual-Core Opteron";
		case 0x23:	/* rev JH-E6 (Toledo) */
			return "Dual-Core Opteron or Athlon 64 X2";
		case 0x43:	/* rev JH-F2 (Windsor) */
			return "Athlon 64 FX or Athlon 64 X2";
		case 0x24:	/* rev SH-E5 (Lancaster?) */
			return "Mobile Athlon 64 or Turion 64";
		case 0x05:	/* rev SH-B0/B3/C0/CG (SledgeHammer?) */
			return "Opteron or Athlon 64 FX";
		case 0x15:	/* rev SH-D0 */
		case 0x25:	/* rev SH-E4 */
			return "Opteron";
		case 0x27:	/* rev DH-E4, SH-E4 */
			return "Athlon 64 or Athlon 64 FX or Opteron";
		case 0x48:	/* rev BH-F2 */
			return "Turion 64 X2";
		case 0x04:	/* rev SH-B0/C0/CG (ClawHammer) */
		case 0x07:	/* rev SH-CG (ClawHammer) */
		case 0x0b:	/* rev CH-CG */
		case 0x14:	/* rev SH-D0 */
		case 0x17:	/* rev SH-D0 */
		case 0x1b:	/* rev CH-D0 */
			return "Athlon 64";
		case 0x2b:	/* rev BH-E4 (Manchester) */
		case 0x4b:	/* rev BH-F2 (Windsor) */
			return "Athlon 64 X2";
		case 0x6b:	/* rev BH-G1 (Brisbane) */
			return "Athlon X2 or Athlon 64 X2";
		case 0x08:	/* rev CH-CG */
		case 0x0c:	/* rev DH-CG (Newcastle) */
		case 0x0e:	/* rev DH-CG (Newcastle?) */
		case 0x0f:	/* rev DH-CG (Newcastle/Paris) */
		case 0x18:	/* rev CH-D0 */
		case 0x1c:	/* rev DH-D0 (Winchester) */
		case 0x1f:	/* rev DH-D0 (Winchester/Victoria) */
		case 0x2c:	/* rev DH-E3/E6 */
		case 0x2f:	/* rev DH-E3/E6 (Venice/Palermo) */
		case 0x4f:	/* rev DH-F2 (Orleans/Manila) */
		case 0x5f:	/* rev DH-F2 (Orleans/Manila) */
		case 0x6f:	/* rev DH-G1 */
			return "Athlon 64 or Sempron";
		default:
			break;
		}
		return "Unknown AMD64 CPU";

#if 0
	case 16:
		return "Family 10h";
	case 17:
		return "Family 11h";
	case 18:
		return "Family 12h";
	case 19:
		return "Family 14h";
	case 20:
		return "Family 15h";
#endif

	default:
		break;
	}

	snprintf(family_str, sizeof family_str, "Family %xh", ci->ci_family);
	return family_str;
}

static void
intel_family_new_probe(struct cpu_info *ci)
{
	uint32_t descs[4];

	x86_cpuid(0x80000000, descs);

	/*
	 * Determine extended feature flags.
	 */
	if (descs[0] >= 0x80000001) {
		x86_cpuid(0x80000001, descs);
		ci->ci_feat_val[2] |= descs[3];
		ci->ci_feat_val[3] |= descs[2];
	}
}

static void
via_cpu_probe(struct cpu_info *ci)
{
	u_int stepping = CPUID_TO_STEPPING(ci->ci_signature);
	u_int descs[4];
	u_int lfunc;

	/*
	 * Determine the largest extended function value.
	 */
	x86_cpuid(0x80000000, descs);
	lfunc = descs[0];

	/*
	 * Determine the extended feature flags.
	 */
	if (lfunc >= 0x80000001) {
		x86_cpuid(0x80000001, descs);
		ci->ci_feat_val[2] |= descs[3];
	}

	if (ci->ci_model < 0x9 || (ci->ci_model == 0x9 && stepping < 3))
		return;

	/* Nehemiah or Esther */
	x86_cpuid(0xc0000000, descs);
	lfunc = descs[0];
	if (lfunc < 0xc0000001)	/* no ACE, no RNG */
		return;

	x86_cpuid(0xc0000001, descs);
	lfunc = descs[3];
	ci->ci_feat_val[4] = lfunc;
}

static void
amd_family6_probe(struct cpu_info *ci)
{
	uint32_t descs[4];
	char *p;
	size_t i;

	x86_cpuid(0x80000000, descs);

	/*
	 * Determine the extended feature flags.
	 */
	if (descs[0] >= 0x80000001) {
		x86_cpuid(0x80000001, descs);
		ci->ci_feat_val[2] |= descs[3]; /* %edx */
		ci->ci_feat_val[3] = descs[2]; /* %ecx */
	}

	if (*cpu_brand_string == '\0')
		return;
	
	for (i = 1; i < __arraycount(amd_brand); i++)
		if ((p = strstr(cpu_brand_string, amd_brand[i])) != NULL) {
			ci->ci_brand_id = i;
			strlcpy(amd_brand_name, p, sizeof(amd_brand_name));
			break;
		}
}

static void
intel_cpu_cacheinfo(struct cpu_info *ci)
{
	const struct x86_cache_info *cai;
	u_int descs[4];
	int iterations, i, j;
	int type, level;
	int ways, partitions, linesize, sets;
	int caitype = -1;
	int totalsize;
	uint8_t desc;

	/* Return if the cpu is old pre-cpuid instruction cpu */
	if (ci->ci_cpu_type >= 0)
		return;

	if (ci->ci_cpuid_level < 2)
		return;

	/*
	 * Parse the cache info from `cpuid leaf 2', if we have it.
	 * XXX This is kinda ugly, but hey, so is the architecture...
	 */
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
				cai = cache_info_lookup(intel_cpuid_cache_info,
				    desc);
				if (cai != NULL)
					ci->ci_cinfo[cai->cai_index] = *cai;
				else if ((verbose != 0) && (desc != 0xff))
					printf("Unknown cacheinfo desc %02x\n",
					    desc);
			}
		}
		x86_cpuid(2, descs);
	}

	if (ci->ci_cpuid_level < 4)
		return;

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
		if (caitype == -1) {
			printf("unknown cache level&type (%d & %d)\n",
			    level, type);
			continue;
		}
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

static const struct x86_cache_info amd_cpuid_l2cache_assoc_info[] = 
    AMD_L2CACHE_INFO;

static const struct x86_cache_info amd_cpuid_l3cache_assoc_info[] = 
    AMD_L3CACHE_INFO;

static void
amd_cpu_cacheinfo(struct cpu_info *ci)
{
	const struct x86_cache_info *cp;
	struct x86_cache_info *cai;
	u_int descs[4];
	u_int lfunc;

	/*
	 * K5 model 0 has none of this info.
	 */
	if (ci->ci_family == 5 && ci->ci_model == 0)
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
	if ((ci->ci_family == 5 && ci->ci_model >= 9) || ci->ci_family >= 6) {
		cai = &ci->ci_cinfo[CAI_ITLB2];
		cai->cai_totalsize = AMD_L1_EAX_ITLB_ENTRIES(descs[0]);
		cai->cai_associativity = AMD_L1_EAX_ITLB_ASSOC(descs[0]);
		cai->cai_linesize = largepagesize;

		cai = &ci->ci_cinfo[CAI_DTLB2];
		cai->cai_totalsize = AMD_L1_EAX_DTLB_ENTRIES(descs[0]);
		cai->cai_associativity = AMD_L1_EAX_DTLB_ASSOC(descs[0]);
		cai->cai_linesize = largepagesize;
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

	cai = &ci->ci_cinfo[CAI_L2_ITLB];
	cai->cai_totalsize = AMD_L2_EBX_IUTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L2_EBX_IUTLB_ASSOC(descs[1]);
	cai->cai_linesize = (4 * 1024);
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	cai = &ci->ci_cinfo[CAI_L2_ITLB2];
	cai->cai_totalsize = AMD_L2_EAX_IUTLB_ENTRIES(descs[0]);
	cai->cai_associativity = AMD_L2_EAX_IUTLB_ASSOC(descs[0]);
	cai->cai_linesize = largepagesize;
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	cai = &ci->ci_cinfo[CAI_L2_DTLB];
	cai->cai_totalsize = AMD_L2_EBX_DTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L2_EBX_DTLB_ASSOC(descs[1]);
	cai->cai_linesize = (4 * 1024);
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	cai = &ci->ci_cinfo[CAI_L2_DTLB2];
	cai->cai_totalsize = AMD_L2_EAX_DTLB_ENTRIES(descs[0]);
	cai->cai_associativity = AMD_L2_EAX_DTLB_ASSOC(descs[0]);
	cai->cai_linesize = largepagesize;
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

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

	/*
	 * Determine L3 cache info on AMD Family 10h and newer processors
	 */
	if (ci->ci_family >= 0x10) {
		cai = &ci->ci_cinfo[CAI_L3CACHE];
		cai->cai_totalsize = AMD_L3_EDX_C_SIZE(descs[3]);
		cai->cai_associativity = AMD_L3_EDX_C_ASSOC(descs[3]);
		cai->cai_linesize = AMD_L3_EDX_C_LS(descs[3]);

		cp = cache_info_lookup(amd_cpuid_l3cache_assoc_info,
		    cai->cai_associativity);
		if (cp != NULL)
			cai->cai_associativity = cp->cai_associativity;
		else
			cai->cai_associativity = 0;	/* XXX Unkn/Rsvd */
	}

	/*
	 * Determine 1GB TLB info.
	 */
	if (lfunc < 0x80000019) {
		/* No 1GB TLB info available. */
		return;
	}

	x86_cpuid(0x80000019, descs);

	cai = &ci->ci_cinfo[CAI_L1_1GBITLB];
	cai->cai_totalsize = AMD_L1_1GB_EAX_IUTLB_ENTRIES(descs[0]);
	cai->cai_associativity = AMD_L1_1GB_EAX_IUTLB_ASSOC(descs[0]);
	cai->cai_linesize = (1024 * 1024 * 1024);
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	cai = &ci->ci_cinfo[CAI_L1_1GBDTLB];
	cai->cai_totalsize = AMD_L1_1GB_EAX_DTLB_ENTRIES(descs[0]);
	cai->cai_associativity = AMD_L1_1GB_EAX_DTLB_ASSOC(descs[0]);
	cai->cai_linesize = (1024 * 1024 * 1024);
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	cai = &ci->ci_cinfo[CAI_L2_1GBITLB];
	cai->cai_totalsize = AMD_L2_1GB_EBX_IUTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L2_1GB_EBX_IUTLB_ASSOC(descs[1]);
	cai->cai_linesize = (1024 * 1024 * 1024);
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */

	cai = &ci->ci_cinfo[CAI_L2_1GBDTLB];
	cai->cai_totalsize = AMD_L2_1GB_EBX_DUTLB_ENTRIES(descs[1]);
	cai->cai_associativity = AMD_L2_1GB_EBX_DUTLB_ASSOC(descs[1]);
	cai->cai_linesize = (1024 * 1024 * 1024);
	cp = cache_info_lookup(amd_cpuid_l2cache_assoc_info,
	    cai->cai_associativity);
	if (cp != NULL)
		cai->cai_associativity = cp->cai_associativity;
	else
		cai->cai_associativity = 0;	/* XXX Unknown/reserved */
}

static void
via_cpu_cacheinfo(struct cpu_info *ci)
{
	struct x86_cache_info *cai;
	int stepping;
	u_int descs[4];
	u_int lfunc;

	stepping = CPUID_TO_STEPPING(ci->ci_signature);

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
	if (ci->ci_model == 9 && stepping == 8) {
		/* Erratum: stepping 8 reports 4 when it should be 2 */
		cai->cai_associativity = 2;
	}

	cai = &ci->ci_cinfo[CAI_ICACHE];
	cai->cai_totalsize = VIA_L1_EDX_IC_SIZE(descs[3]);
	cai->cai_associativity = VIA_L1_EDX_IC_ASSOC(descs[3]);
	cai->cai_linesize = VIA_L1_EDX_IC_LS(descs[3]);
	if (ci->ci_model == 9 && stepping == 8) {
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
	if (ci->ci_model >= 9) {
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
tmx86_get_longrun_status(u_int *frequency, u_int *voltage, u_int *percentage)
{
	u_int descs[4];

	x86_cpuid(0x80860007, descs);
	*frequency = descs[0];
	*voltage = descs[1];
	*percentage = descs[2];
}

static void
transmeta_cpu_info(struct cpu_info *ci)
{
	u_int descs[4], nreg;
	u_int frequency, voltage, percentage;

	x86_cpuid(0x80860000, descs);
	nreg = descs[0];
	if (nreg >= 0x80860001) {
		x86_cpuid(0x80860001, descs);
		aprint_verbose_dev(ci->ci_dev, "Processor revision %u.%u.%u.%u\n",
		    (descs[1] >> 24) & 0xff,
		    (descs[1] >> 16) & 0xff,
		    (descs[1] >> 8) & 0xff,
		    descs[1] & 0xff);
	}
	if (nreg >= 0x80860002) {
		x86_cpuid(0x80860002, descs);
		aprint_verbose_dev(ci->ci_dev, "Code Morphing Software Rev: %u.%u.%u-%u-%u\n",
		    (descs[1] >> 24) & 0xff,
		    (descs[1] >> 16) & 0xff,
		    (descs[1] >> 8) & 0xff,
		    descs[1] & 0xff,
		    descs[2]);
	}
	if (nreg >= 0x80860006) {
		union {
			char text[65];
			u_int descs[4][4];
		} info;
		int i;

		for (i=0; i<4; i++) {
			x86_cpuid(0x80860003 + i, info.descs[i]);
		}
		info.text[64] = '\0';
		aprint_verbose_dev(ci->ci_dev, "%s\n", info.text);
	}

	if (nreg >= 0x80860007) {
		tmx86_get_longrun_status(&frequency,
		    &voltage, &percentage);
		aprint_verbose_dev(ci->ci_dev, "LongRun <%dMHz %dmV %d%%>\n",
		    frequency, voltage, percentage);
	}
}

static void
cpu_probe_base_features(struct cpu_info *ci, const char *cpuname)
{
	u_int descs[4];
	int i;
	uint32_t brand[12];

	memset(ci, 0, sizeof(*ci));
	ci->ci_dev = cpuname;

	ci->ci_cpu_type = x86_identify();
	if (ci->ci_cpu_type >= 0) {
		/* Old pre-cpuid instruction cpu */
		ci->ci_cpuid_level = -1;
		return;
	}

	/*
	 * This CPU supports cpuid instruction, so we can call x86_cpuid()
	 * function.
	 */

	/*
	 * Fn0000_0000:
	 * - Save cpuid max level.
	 * - Save vendor string.
	 */
	x86_cpuid(0, descs);
	ci->ci_cpuid_level = descs[0];
	/* Save vendor string */
	ci->ci_vendor[0] = descs[1];
	ci->ci_vendor[2] = descs[2];
	ci->ci_vendor[1] = descs[3];
	ci->ci_vendor[3] = 0;

	aprint_verbose("%s: highest basic info %08x\n", cpuname,
	    ci->ci_cpuid_level);
	if (verbose) {
		int bf;
		
		for (bf = 0; bf <= ci->ci_cpuid_level; bf++) {
			x86_cpuid(bf, descs);
			printf("%s: %08x: %08x %08x %08x %08x\n", cpuname,
			    bf, descs[0], descs[1], descs[2], descs[3]);
		}
	}

	/*
	 * Fn8000_0000:
	 * - Get cpuid extended function's max level.
	 */
	x86_cpuid(0x80000000, descs);
	if (descs[0] >=  0x80000000) {
		ci->ci_cpuid_extlevel = descs[0];
		aprint_verbose("%s: highest extended info %08x\n", cpuname,
		    ci->ci_cpuid_extlevel);
	} else {
		/* Set lower value than 0x80000000 */
		ci->ci_cpuid_extlevel = 0;
	}
	if (verbose) {
		unsigned int ef;

		for (ef = 0x80000000; ef <= ci->ci_cpuid_extlevel; ef++) {
			x86_cpuid(ef, descs);
			printf("%s: %08x: %08x %08x %08x %08x\n", cpuname,
			    ef, descs[0], descs[1], descs[2], descs[3]);
		}
	}

	/*
	 * Fn8000_000[2-4]:
	 * - Save brand string.
	 */
	if (ci->ci_cpuid_extlevel >= 0x80000004) {
		x86_cpuid(0x80000002, brand);
		x86_cpuid(0x80000003, brand + 4);
		x86_cpuid(0x80000004, brand + 8);
		for (i = 0; i < 48; i++)
			if (((char *) brand)[i] != ' ')
				break;
		memcpy(cpu_brand_string, ((char *) brand) + i, 48 - i);
	}

	if (ci->ci_cpuid_level < 1)
		return;

	/*
	 * Fn0000_0001:
	 * - Get CPU family, model and stepping (from eax).
	 * - Initial local APIC ID and brand ID (from ebx)
	 * - CPUID2 (from ecx)
	 * - CPUID (from edx)
	 */
	x86_cpuid(1, descs);
	ci->ci_signature = descs[0];

	/* Extract full family/model values */
	ci->ci_family = CPUID_TO_FAMILY(ci->ci_signature);
	ci->ci_model = CPUID_TO_MODEL(ci->ci_signature);

	/* Brand is low order 8 bits of ebx */
	ci->ci_brand_id = descs[1] & 0xff;
	/* Initial local APIC ID */
	ci->ci_initapicid = (descs[1] >> 24) & 0xff;

	ci->ci_feat_val[1] = descs[2];
	ci->ci_feat_val[0] = descs[3];

	if (ci->ci_cpuid_level < 3)
		return;

	/*
	 * If the processor serial number misfeature is present and supported,
	 * extract it here.
	 */
	if ((ci->ci_feat_val[0] & CPUID_PN) != 0) {
		ci->ci_cpu_serial[0] = ci->ci_signature;
		x86_cpuid(3, descs);
		ci->ci_cpu_serial[2] = descs[2];
		ci->ci_cpu_serial[1] = descs[3];
	}

	if (ci->ci_cpuid_level < 0xd)
		return;

	/* Get support XCR0 bits */
	x86_cpuid2(0xd, 0, descs);
	ci->ci_feat_val[5] = descs[0];	/* Actually 64 bits */
	ci->ci_cur_xsave = descs[1];
	ci->ci_max_xsave = descs[2];

	/* Additional flags (eg xsaveopt support) */
	x86_cpuid2(0xd, 1, descs);
	ci->ci_feat_val[6] = descs[0];   /* Actually 64 bits */
}

static void
cpu_probe_features(struct cpu_info *ci)
{
	const struct cpu_cpuid_nameclass *cpup = NULL;
	unsigned int i;

	if (ci->ci_cpuid_level < 1)
		return;

	for (i = 0; i < __arraycount(i386_cpuid_cpus); i++) {
		if (!strncmp((char *)ci->ci_vendor,
		    i386_cpuid_cpus[i].cpu_id, 12)) {
			cpup = &i386_cpuid_cpus[i];
			break;
		}
	}

	if (cpup == NULL)
		return;

	i = ci->ci_family - CPU_MINFAMILY;

	if (i >= __arraycount(cpup->cpu_family))
		i = __arraycount(cpup->cpu_family) - 1;

	if (cpup->cpu_family[i].cpu_probe == NULL)
		return;

	(*cpup->cpu_family[i].cpu_probe)(ci);
}

static void
print_bits(const char *cpuname, const char *hdr, const char *fmt, uint32_t val)
{
	char buf[32 * 16];
	char *bp;

#define	MAX_LINE_LEN	79	/* get from command arg or 'stty cols' ? */

	if (val == 0 || fmt == NULL)
		return;

	snprintb_m(buf, sizeof(buf), fmt, val,
	    MAX_LINE_LEN - strlen(cpuname) - 2 - strlen(hdr) - 1);
	bp = buf;
	while (*bp != '\0') {
		aprint_verbose("%s: %s %s\n", cpuname, hdr, bp);
		bp += strlen(bp) + 1;
	}
}

static void
identifycpu_cpuids(struct cpu_info *ci)
{
	const char *cpuname = ci->ci_dev;
	u_int lp_max = 1;	/* logical processors per package */
	u_int smt_max;		/* smt per core */
	u_int core_max = 1;	/* core per package */
	u_int smt_bits, core_bits;
	uint32_t descs[4];

	aprint_verbose("%s: Initial APIC ID %u\n", cpuname, ci->ci_initapicid);
	ci->ci_packageid = ci->ci_initapicid;
	ci->ci_coreid = 0;
	ci->ci_smtid = 0;
	if (cpu_vendor != CPUVENDOR_INTEL) {
		return;
	}

	/*
	 * 253668.pdf 7.10.2
	 */

	if ((ci->ci_feat_val[0] & CPUID_HTT) != 0) {
		x86_cpuid(1, descs);
		lp_max = (descs[1] >> 16) & 0xff;
	}
	if (ci->ci_cpuid_level >= 4) {
		x86_cpuid2(4, 0, descs);
		core_max = (descs[0] >> 26) + 1;
	}
	assert(lp_max >= core_max);
	smt_max = lp_max / core_max;
	smt_bits = ilog2(smt_max - 1) + 1;
	core_bits = ilog2(core_max - 1) + 1;
	if (smt_bits + core_bits) {
		ci->ci_packageid = ci->ci_initapicid >> (smt_bits + core_bits);
	}
	aprint_verbose("%s: Cluster/Package ID %u\n", cpuname,
	    ci->ci_packageid);
	if (core_bits) {
		u_int core_mask = __BITS(smt_bits, smt_bits + core_bits - 1);

		ci->ci_coreid =
		    __SHIFTOUT(ci->ci_initapicid, core_mask);
		aprint_verbose("%s: Core ID %u\n", cpuname, ci->ci_coreid);
	}
	if (smt_bits) {
		u_int smt_mask = __BITS((int)0, (int)(smt_bits - 1));

		ci->ci_smtid = __SHIFTOUT(ci->ci_initapicid, smt_mask);
		aprint_verbose("%s: SMT ID %u\n", cpuname, ci->ci_smtid);
	}
}

void
identifycpu(int fd, const char *cpuname)
{
	const char *name = "", *modifier, *vendorname, *brand = "";
	int class = CPUCLASS_386;
	unsigned int i;
	int modif, family;
	const struct cpu_cpuid_nameclass *cpup = NULL;
	const struct cpu_cpuid_family *cpufam;
	struct cpu_info *ci, cistore;
	size_t sz;
	struct cpu_ucode_version ucode;
	union {
		struct cpu_ucode_version_amd amd;
		struct cpu_ucode_version_intel1 intel1;
	} ucvers;

	ci = &cistore;
	cpu_probe_base_features(ci, cpuname);
	cpu_probe_features(ci);

	if (ci->ci_cpu_type >= 0) {
		/* Old pre-cpuid instruction cpu */
		if (ci->ci_cpu_type >= (int)__arraycount(i386_nocpuid_cpus))
			errx(1, "unknown cpu type %d", ci->ci_cpu_type);
		name = i386_nocpuid_cpus[ci->ci_cpu_type].cpu_name;
		cpu_vendor = i386_nocpuid_cpus[ci->ci_cpu_type].cpu_vendor;
		vendorname = i386_nocpuid_cpus[ci->ci_cpu_type].cpu_vendorname;
		class = i386_nocpuid_cpus[ci->ci_cpu_type].cpu_class;
		ci->ci_info = i386_nocpuid_cpus[ci->ci_cpu_type].cpu_info;
		modifier = "";
	} else {
		/* CPU which support cpuid instruction */
		modif = (ci->ci_signature >> 12) & 0x3;
		family = ci->ci_family;
		if (family < CPU_MINFAMILY)
			errx(1, "identifycpu: strange family value");
		if (family > CPU_MAXFAMILY)
			family = CPU_MAXFAMILY;

		for (i = 0; i < __arraycount(i386_cpuid_cpus); i++) {
			if (!strncmp((char *)ci->ci_vendor,
			    i386_cpuid_cpus[i].cpu_id, 12)) {
				cpup = &i386_cpuid_cpus[i];
				break;
			}
		}

		if (cpup == NULL) {
			cpu_vendor = CPUVENDOR_UNKNOWN;
			if (ci->ci_vendor[0] != '\0')
				vendorname = (char *)&ci->ci_vendor[0];
			else
				vendorname = "Unknown";
			class = family - 3;
			modifier = "";
			name = "";
			ci->ci_info = NULL;
		} else {
			cpu_vendor = cpup->cpu_vendor;
			vendorname = cpup->cpu_vendorname;
			modifier = modifiers[modif];
			cpufam = &cpup->cpu_family[family - CPU_MINFAMILY];
			name = cpufam->cpu_models[ci->ci_model];
			if (name == NULL || *name == '\0')
			    name = cpufam->cpu_model_default;
			class = cpufam->cpu_class;
			ci->ci_info = cpufam->cpu_info;

			if (cpu_vendor == CPUVENDOR_INTEL) {
				if (ci->ci_family == 6 && ci->ci_model >= 5) {
					const char *tmp;
					tmp = intel_family6_name(ci);
					if (tmp != NULL)
						name = tmp;
				}
				if (ci->ci_family == 15 &&
				    ci->ci_brand_id <
				    __arraycount(i386_intel_brand) &&
				    i386_intel_brand[ci->ci_brand_id])
					name =
					     i386_intel_brand[ci->ci_brand_id];
			}

			if (cpu_vendor == CPUVENDOR_AMD) {
				if (ci->ci_family == 6 && ci->ci_model >= 6) {
					if (ci->ci_brand_id == 1)
						/* 
						 * It's Duron. We override the 
						 * name, since it might have
						 * been misidentified as Athlon.
						 */
						name =
						    amd_brand[ci->ci_brand_id];
					else
						brand = amd_brand_name;
				}
				if (CPUID_TO_BASEFAMILY(ci->ci_signature)
				    == 0xf) {
					/* Identify AMD64 CPU names.  */
					const char *tmp;
					tmp = amd_amd64_name(ci);
					if (tmp != NULL)
						name = tmp;
				}
			}
			
			if (cpu_vendor == CPUVENDOR_IDT && ci->ci_family >= 6)
				vendorname = "VIA";
		}
	}

	ci->ci_cpu_class = class;

	sz = sizeof(ci->ci_tsc_freq);
	(void)sysctlbyname("machdep.tsc_freq", &ci->ci_tsc_freq, &sz, NULL, 0);
	sz = sizeof(use_pae);
	(void)sysctlbyname("machdep.pae", &use_pae, &sz, NULL, 0);
	largepagesize = (use_pae ? 2 * 1024 * 1024 : 4 * 1024 * 1024);

	/*
	 * The 'cpu_brand_string' is much more useful than the 'cpu_model'
	 * we try to determine from the family/model values.
	 */
	if (*cpu_brand_string != '\0')
		aprint_normal("%s: \"%s\"\n", cpuname, cpu_brand_string);

	aprint_normal("%s: %s", cpuname, vendorname);
	if (*modifier)
		aprint_normal(" %s", modifier);
	if (*name)
		aprint_normal(" %s", name);
	if (*brand)
		aprint_normal(" %s", brand);
	aprint_normal(" (%s-class)", classnames[class]);

	if (ci->ci_tsc_freq != 0)
		aprint_normal(", %ju.%02ju MHz\n",
		    ((uintmax_t)ci->ci_tsc_freq + 4999) / 1000000,
		    (((uintmax_t)ci->ci_tsc_freq + 4999) / 10000) % 100);

	aprint_normal_dev(ci->ci_dev, "family %#x model %#x stepping %#x",
	    ci->ci_family, ci->ci_model, CPUID_TO_STEPPING(ci->ci_signature));
	if (ci->ci_signature != 0)
		aprint_normal(" (id %#x)", ci->ci_signature);
	aprint_normal("\n");

	if (ci->ci_info)
		(*ci->ci_info)(ci);

	/*
	 * display CPU feature flags
	 */

	print_bits(cpuname, "features", CPUID_FLAGS1, ci->ci_feat_val[0]);
	print_bits(cpuname, "features1", CPUID2_FLAGS1, ci->ci_feat_val[1]);

	/* These next two are actually common definitions! */
	print_bits(cpuname, "features2",
	    cpu_vendor == CPUVENDOR_INTEL ? CPUID_INTEL_EXT_FLAGS
		: CPUID_EXT_FLAGS, ci->ci_feat_val[2]);
	print_bits(cpuname, "features3",
	    cpu_vendor == CPUVENDOR_INTEL ? CPUID_INTEL_FLAGS4
		: CPUID_AMD_FLAGS4, ci->ci_feat_val[3]);

	print_bits(cpuname, "padloack features", CPUID_FLAGS_PADLOCK,
	    ci->ci_feat_val[4]);

	print_bits(cpuname, "xsave features", XCR0_FLAGS1, ci->ci_feat_val[5]);
	print_bits(cpuname, "xsave instructions", CPUID_PES1_FLAGS,
	    ci->ci_feat_val[6]);

	if (ci->ci_max_xsave != 0) {
		aprint_normal("%s: xsave area size: current %d, maximum %d",
			cpuname, ci->ci_cur_xsave, ci->ci_max_xsave);
		aprint_normal(", xgetbv %sabled\n",
		    ci->ci_feat_val[1] & CPUID2_OSXSAVE ? "en" : "dis");
		if (ci->ci_feat_val[1] & CPUID2_OSXSAVE)
			print_bits(cpuname, "enabled xsave", XCR0_FLAGS1,
			    x86_xgetbv());
	}

	x86_print_cache_and_tlb_info(ci);

	if (ci->ci_cpuid_level >= 3 && (ci->ci_feat_val[0] & CPUID_PN)) {
		aprint_verbose("%s: serial number %04X-%04X-%04X-%04X-%04X-%04X\n",
		    cpuname,
		    ci->ci_cpu_serial[0] / 65536, ci->ci_cpu_serial[0] % 65536,
		    ci->ci_cpu_serial[1] / 65536, ci->ci_cpu_serial[1] % 65536,
		    ci->ci_cpu_serial[2] / 65536, ci->ci_cpu_serial[2] % 65536);
	}

	if (ci->ci_cpu_class == CPUCLASS_386) {
		errx(1, "NetBSD requires an 80486 or later processor");
	}

	if (ci->ci_cpu_type == CPU_486DLC) {
#ifndef CYRIX_CACHE_WORKS
		aprint_error("WARNING: CYRIX 486DLC CACHE UNCHANGED.\n");
#else
#ifndef CYRIX_CACHE_REALLY_WORKS
		aprint_error("WARNING: CYRIX 486DLC CACHE ENABLED IN HOLD-FLUSH MODE.\n");
#else
		aprint_error("WARNING: CYRIX 486DLC CACHE ENABLED.\n");
#endif
#endif
	}

	/*
	 * Everything past this point requires a Pentium or later.
	 */
	if (ci->ci_cpuid_level < 0)
		return;

	identifycpu_cpuids(ci);

#ifdef INTEL_CORETEMP
	if (cpu_vendor == CPUVENDOR_INTEL && ci->ci_cpuid_level >= 0x06)
		coretemp_register(ci);
#endif

	if (cpu_vendor == CPUVENDOR_AMD) {
		uint32_t data[4];

		x86_cpuid(0x80000000, data);
		if (data[0] >= 0x80000007)
			powernow_probe(ci);

		if ((data[0] >= 0x8000000a)
		   && (ci->ci_feat_val[3] & CPUID_SVM) != 0) {
			x86_cpuid(0x8000000a, data);
			aprint_verbose("%s: SVM Rev. %d\n", cpuname,
			    data[0] & 0xf);
			aprint_verbose("%s: SVM NASID %d\n", cpuname, data[1]);
			print_bits(cpuname, "SVM features", CPUID_AMD_SVM_FLAGS,
				   data[3]);
		}
	} else if (cpu_vendor == CPUVENDOR_INTEL) {
		uint32_t data[4];
		int32_t bi_index;

		for (bi_index = 1; bi_index <= ci->ci_cpuid_level; bi_index++) {
			x86_cpuid(bi_index, data);
			switch (bi_index) {
			case 6:
				print_bits(cpuname, "DSPM-eax",
				    CPUID_DSPM_FLAGS, data[0]);
				print_bits(cpuname, "DSPM-ecx",
				    CPUID_DSPM_FLAGS1, data[2]);
				break;
			case 7:
				aprint_verbose("%s: SEF highest subleaf %08x\n",
				    cpuname, data[0]);
				print_bits(cpuname, "SEF-main", CPUID_SEF_FLAGS,
				    data[1]);
				break;
#if 0
			default:
				aprint_verbose("%s: basic %08x-eax %08x\n",
				    cpuname, bi_index, data[0]);
				aprint_verbose("%s: basic %08x-ebx %08x\n",
				    cpuname, bi_index, data[1]);
				aprint_verbose("%s: basic %08x-ecx %08x\n",
				    cpuname, bi_index, data[2]);
				aprint_verbose("%s: basic %08x-edx %08x\n",
				    cpuname, bi_index, data[3]);
				break;
#endif
			}
		}
	}

#ifdef INTEL_ONDEMAND_CLOCKMOD
	clockmod_init();
#endif

	if (cpu_vendor == CPUVENDOR_AMD)
		ucode.loader_version = CPU_UCODE_LOADER_AMD;
	else if (cpu_vendor == CPUVENDOR_INTEL)
		ucode.loader_version = CPU_UCODE_LOADER_INTEL1;
	else
		return;

	ucode.data = &ucvers;
	if (ioctl(fd, IOC_CPU_UCODE_GET_VERSION, &ucode) < 0) {
#ifdef __i386__
		struct cpu_ucode_version_64 ucode_64;
		if (errno != ENOTTY)
			return;
		/* Try the 64 bit ioctl */
		memset(&ucode_64, 0, sizeof ucode_64);
		ucode_64.data = &ucvers;
		ucode_64.loader_version = ucode.loader_version;
		if (ioctl(fd, IOC_CPU_UCODE_GET_VERSION_64, &ucode_64) < 0)
			return;
#endif
	}

	if (cpu_vendor == CPUVENDOR_AMD)
		printf("%s: UCode version: 0x%"PRIx64"\n", cpuname, ucvers.amd.version);
	else if (cpu_vendor == CPUVENDOR_INTEL)
		printf("%s: microcode version 0x%x, platform ID %d\n", cpuname,
		       ucvers.intel1.ucodeversion, ucvers.intel1.platformid);
}

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

static const char *
print_cache_config(struct cpu_info *ci, int cache_tag, const char *name,
    const char *sep)
{
	struct x86_cache_info *cai = &ci->ci_cinfo[cache_tag];
	char human_num[HUMAN_BUFSIZE];

	if (cai->cai_totalsize == 0)
		return sep;

	if (sep == NULL)
		aprint_verbose_dev(ci->ci_dev, "");
	else
		aprint_verbose("%s", sep);
	if (name != NULL)
		aprint_verbose("%s ", name);

	if (cai->cai_string != NULL) {
		aprint_verbose("%s ", cai->cai_string);
	} else {
		(void)humanize_number(human_num, sizeof(human_num),
			cai->cai_totalsize, "B", HN_AUTOSCALE, HN_NOSPACE);
		aprint_verbose("%s %dB/line ", human_num, cai->cai_linesize);
	}
	switch (cai->cai_associativity) {
	case    0:
		aprint_verbose("disabled");
		break;
	case    1:
		aprint_verbose("direct-mapped");
		break;
	case 0xff:
		aprint_verbose("fully associative");
		break;
	default:
		aprint_verbose("%d-way", cai->cai_associativity);
		break;
	}
	return ", ";
}

static const char *
print_tlb_config(struct cpu_info *ci, int cache_tag, const char *name,
    const char *sep)
{
	struct x86_cache_info *cai = &ci->ci_cinfo[cache_tag];
	char human_num[HUMAN_BUFSIZE];

	if (cai->cai_totalsize == 0)
		return sep;

	if (sep == NULL)
		aprint_verbose_dev(ci->ci_dev, "");
	else
		aprint_verbose("%s", sep);
	if (name != NULL)
		aprint_verbose("%s ", name);

	if (cai->cai_string != NULL) {
		aprint_verbose("%s", cai->cai_string);
	} else {
		(void)humanize_number(human_num, sizeof(human_num),
			cai->cai_linesize, "B", HN_AUTOSCALE, HN_NOSPACE);
		aprint_verbose("%d %s entries ", cai->cai_totalsize,
		    human_num);
		switch (cai->cai_associativity) {
		case 0:
			aprint_verbose("disabled");
			break;
		case 1:
			aprint_verbose("direct-mapped");
			break;
		case 0xff:
			aprint_verbose("fully associative");
			break;
		default:
			aprint_verbose("%d-way", cai->cai_associativity);
			break;
		}
	}
	return ", ";
}

static void
x86_print_cache_and_tlb_info(struct cpu_info *ci)
{
	const char *sep = NULL;

	if (ci->ci_cinfo[CAI_ICACHE].cai_totalsize != 0 ||
	    ci->ci_cinfo[CAI_DCACHE].cai_totalsize != 0) {
		sep = print_cache_config(ci, CAI_ICACHE, "I-cache", NULL);
		sep = print_cache_config(ci, CAI_DCACHE, "D-cache", sep);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L2CACHE].cai_totalsize != 0) {
		sep = print_cache_config(ci, CAI_L2CACHE, "L2 cache", NULL);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L3CACHE].cai_totalsize != 0) {
		sep = print_cache_config(ci, CAI_L3CACHE, "L3 cache", NULL);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_PREFETCH].cai_linesize != 0) {
		aprint_verbose_dev(ci->ci_dev, "%dB prefetching",
			ci->ci_cinfo[CAI_PREFETCH].cai_linesize);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_ITLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_ITLB, "ITLB", NULL);
		sep = print_tlb_config(ci, CAI_ITLB2, NULL, sep);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_DTLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_DTLB, "DTLB", NULL);
		sep = print_tlb_config(ci, CAI_DTLB2, NULL, sep);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L2_ITLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L2_ITLB, "L2 ITLB", NULL);
		sep = print_tlb_config(ci, CAI_L2_ITLB2, NULL, sep);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L2_DTLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L2_DTLB, "L2 DTLB", NULL);
		sep = print_tlb_config(ci, CAI_L2_DTLB2, NULL, sep);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L2_STLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L2_STLB, "L2 STLB", NULL);
		sep = print_tlb_config(ci, CAI_L2_STLB2, NULL, sep);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L1_1GBITLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L1_1GBITLB, "L1 1GB page ITLB",
		    NULL);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L1_1GBDTLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L1_1GBDTLB, "L1 1GB page DTLB",
		    NULL);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L2_1GBITLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L2_1GBITLB, "L2 1GB page ITLB",
		    NULL);
		if (sep != NULL)
			aprint_verbose("\n");
	}
	if (ci->ci_cinfo[CAI_L2_1GBDTLB].cai_totalsize != 0) {
		sep = print_tlb_config(ci, CAI_L2_1GBDTLB, "L2 1GB page DTLB",
		    NULL);
		if (sep != NULL)
			aprint_verbose("\n");
	}
}

static void
powernow_probe(struct cpu_info *ci)
{
	uint32_t regs[4];
	char buf[256];

	x86_cpuid(0x80000007, regs);

	snprintb(buf, sizeof(buf), CPUID_APM_FLAGS, regs[3]);
	aprint_normal_dev(ci->ci_dev, "AMD Power Management features: %s\n",
	    buf);
}

int
ucodeupdate_check(int fd, struct cpu_ucode *uc)
{
	struct cpu_info ci;
	int loader_version, res;
	struct cpu_ucode_version versreq;

	cpu_probe_base_features(&ci, "unknown");

	if (!strcmp((char *)ci.ci_vendor, "AuthenticAMD"))
		loader_version = CPU_UCODE_LOADER_AMD;
	else if (!strcmp((char *)ci.ci_vendor, "GenuineIntel"))
		loader_version = CPU_UCODE_LOADER_INTEL1;
	else
		return -1;

	/* check whether the kernel understands this loader version */
	versreq.loader_version = loader_version;
	versreq.data = 0;
	res = ioctl(fd, IOC_CPU_UCODE_GET_VERSION, &versreq);
	if (res)
		return -1;

	switch (loader_version) {
	case CPU_UCODE_LOADER_AMD:
		if (uc->cpu_nr != -1) {
			/* printf? */
			return -1;
		}
		uc->cpu_nr = CPU_UCODE_ALL_CPUS;
		break;
	case CPU_UCODE_LOADER_INTEL1:
		if (uc->cpu_nr == -1)
			uc->cpu_nr = CPU_UCODE_ALL_CPUS; /* for Xen */
		else
			uc->cpu_nr = CPU_UCODE_CURRENT_CPU;
		break;
	default: /* can't happen */
		return -1;
	}
	uc->loader_version = loader_version;
	return 0;
}
