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
__KERNEL_RCSID(0, "$NetBSD: identcpu_subr.c,v 1.2.4.2 2020/04/25 11:23:57 bouyer Exp $");

#ifdef _KERNEL_OPT
#include "lapic.h"
#endif

#include <sys/param.h>

#ifdef _KERNEL
#include <sys/systm.h>
#include <x86/cpuvar.h>
#include <x86/apicvar.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/specialreg.h>
#else
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	if ((denominator == 0) || numerator == 0) {
		aprint_debug_dev(ci->ci_dev,
		    "TSC/core crystal clock ratio is not enumerated\n");
	} else {
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
		aprint_verbose_dev(ci->ci_dev, "TSC freq %" PRIu64 " Hz\n",
		    freq);

	return freq;
}
