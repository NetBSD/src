/* $NetBSD: identcpu_subr.c,v 1.7.4.3 2021/12/24 13:02:25 martin Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Masanobu SAITOH.
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
 * Subroutines for CPU.
 * This file is shared between kernel and userland.
 * See src/usr.sbin/cpuctl/{Makefile, arch/i386.c}).
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: identcpu_subr.c,v 1.7.4.3 2021/12/24 13:02:25 martin Exp $");

#ifdef _KERNEL_OPT
#include "lapic.h"
#endif

#include <sys/param.h>

#ifdef _KERNEL
#include <sys/systm.h>
#include <x86/cpuvar.h>
#include <x86/apicvar.h>
#include <x86/cacheinfo.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/specialreg.h>
#else
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86/cacheinfo.h>
#include "cpuctl.h"
#include "cpuctl_i386.h"
#endif

uint64_t
cpu_tsc_freq_cpuid(struct cpu_info *ci)
{
	uint64_t freq = 0, khz;
	uint32_t descs[4];
	uint32_t denominator, numerator;

	if (!((ci->ci_max_cpuid >= 0x15) && (cpu_vendor == CPUVENDOR_INTEL)))
		return 0;

	x86_cpuid(0x15, descs);
	denominator = descs[0];
	numerator = descs[1];
	if ((denominator != 0) && (numerator != 0)) {
		khz = 0;
		if (descs[2] != 0)
			khz = descs[2] / 1000;
		else if (CPUID_TO_FAMILY(ci->ci_signature) == 6) {
			/*
			 * Table 18-85 Nominal Core Crystal Clock Frequency,
			 * 18.7.3 Determining the Processor Base Frequency,
			 * Intel SDM.
			 */
			switch (CPUID_TO_MODEL(ci->ci_signature)) {
			case 0x55: /* Xeon Scalable */
			case 0x5f: /*
				    * Atom Goldmont (Denverton). Right?
				    * XXX Not documented!
				    */
				khz = 25000; /* 25.0 MHz */
				break;
			case 0x4e: /* 7th gen Core (Skylake) */
			case 0x5e: /* 7th gen Core (Skylake) */
			case 0x8e: /* 8th gen Core (Kaby Lake) */
			case 0x9e: /* 8th gen Core (Kaby Lake) */
				khz = 24000; /* 24.0 MHz */
				break;
			case 0x5c: /* Atom Goldmont */
				khz = 19200; /* 19.2 MHz */
				break;
			default: /* Unknown */
				break;
			}
		}
		freq = khz * 1000 * numerator / denominator;
		if (ci->ci_max_cpuid >= 0x16) {
			x86_cpuid(0x16, descs);
			if (descs[0] != 0) {
				aprint_verbose_dev(ci->ci_dev,
				    "CPU base freq %" PRIu64 " Hz\n",
				    (uint64_t)descs[0] * 1000000);

				/*
				 * If we couldn't get frequency from
				 * CPUID 0x15, use CPUID 0x16 EAX.
				 */
				if (freq == 0) {
					khz = (uint64_t)descs[0] * 1000
					    * denominator / numerator;
					freq = (uint64_t)descs[0] * 1000000;
				}
			}
			if (descs[1] != 0) {
				aprint_verbose_dev(ci->ci_dev,
				    "CPU max freq %" PRIu64 " Hz\n",
				    (uint64_t)descs[1] * 1000000);
			}
		}
#if defined(_KERNEL) &&  NLAPIC > 0
		if ((khz != 0) && (lapic_per_second == 0)) {
			lapic_per_second = khz * 1000;
			aprint_debug_dev(ci->ci_dev,
			    "lapic_per_second set to %" PRIu32 "\n",
			    lapic_per_second);
		}
#endif
	}
	if (freq != 0)
		aprint_verbose_dev(ci->ci_dev, "TSC freq CPUID %" PRIu64
		    " Hz\n", freq);

	return freq;
}

const struct x86_cache_info *
cpu_cacheinfo_lookup(const struct x86_cache_info *cai, uint8_t desc)
{
	int i;

	for (i = 0; cai[i].cai_desc != 0; i++) {
		if (cai[i].cai_desc == desc)
			return &cai[i];
	}

	return NULL;
}

/*
 * Get cache info from one of the following:
 *	Intel Deterministic Cache Parameter Leaf (0x04)
 *	AMD Cache Topology Information Leaf (0x8000001d)
 */
void
cpu_dcp_cacheinfo(struct cpu_info *ci, uint32_t leaf)
{
	u_int descs[4];
	int type, level, ways, partitions, linesize, sets, totalsize;
	int caitype = -1;
	int i;

	for (i = 0; ; i++) {
		x86_cpuid2(leaf, i, descs);
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
