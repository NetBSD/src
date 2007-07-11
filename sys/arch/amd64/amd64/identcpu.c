/*	$NetBSD: identcpu.c,v 1.11.4.1 2007/07/11 19:57:31 mjf Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: identcpu.c,v 1.11.4.1 2007/07/11 19:57:31 mjf Exp $");

#include "opt_enhanced_speedstep.h"
#include "opt_intel_odcm.h"
#include "opt_powernow_k8.h"

#include <sys/types.h>
#include <sys/systm.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/powernow.h>

/* sysctl wants this. */
char cpu_model[48];

void
identifycpu(struct cpu_info *ci)
{
	u_int64_t last_tsc;
	u_int32_t dummy, val;
	char buf[512];
	u_int32_t brand[12];
	int vendor;
	const char *feature_str[3];

	CPUID(1, ci->ci_signature, val,
	    ci->ci_feature2_flags, ci->ci_feature_flags);
	CPUID(0x80000001, dummy, dummy, dummy, val);
	ci->ci_feature_flags |= val;

	CPUID(0x80000002, brand[0], brand[1], brand[2], brand[3]);
	CPUID(0x80000003, brand[4], brand[5], brand[6], brand[7]);
	CPUID(0x80000004, brand[8], brand[9], brand[10], brand[11]);

	strcpy(cpu_model, (char *)brand);

	vendor = CPUVENDOR_AMD;
	if (cpu_model[0] == 0)
		strcpy(cpu_model, "Opteron or Athlon 64");
	else if (strstr(cpu_model, "AMD") == NULL)
		vendor = CPUVENDOR_INTEL;

	last_tsc = rdtsc();
	delay(100000);
	ci->ci_tsc_freq = (rdtsc() - last_tsc) * 10;

	amd_cpu_cacheinfo(ci);

	aprint_normal("%s: %s", ci->ci_dev->dv_xname, cpu_model);

	if (ci->ci_tsc_freq != 0)
		aprint_normal(", %lu.%02lu MHz",
		    (ci->ci_tsc_freq + 4999) / 1000000,
		    ((ci->ci_tsc_freq + 4999) / 10000) % 100);
	aprint_normal("\n");

	if (vendor == CPUVENDOR_INTEL) {
		feature_str[0] = CPUID_FLAGS1;
		feature_str[1] = CPUID_FLAGS2;
		feature_str[2] = CPUID_FLAGS3;
	} else {
		feature_str[0] = CPUID_FLAGS1;
		feature_str[1] = CPUID_EXT_FLAGS2;
		feature_str[2] = CPUID_EXT_FLAGS3;
	}

	if ((ci->ci_feature_flags & CPUID_MASK1) != 0) {
		bitmask_snprintf(ci->ci_feature_flags,
		    feature_str[0], buf, sizeof(buf));
		aprint_verbose("%s: features: %s\n",
		   ci->ci_dev->dv_xname, buf);
	}
	if ((ci->ci_feature_flags & CPUID_MASK2) != 0) {
		bitmask_snprintf(ci->ci_feature_flags,
		    feature_str[1], buf, sizeof(buf));
		aprint_verbose("%s: features: %s\n",
		    ci->ci_dev->dv_xname, buf);
	}
	if ((ci->ci_feature_flags & CPUID_MASK3) != 0) {
		bitmask_snprintf(ci->ci_feature_flags,
		    feature_str[2], buf, sizeof(buf));
		aprint_verbose("%s: features: %s\n",
		    ci->ci_dev->dv_xname, buf);
	}

	if (ci->ci_feature2_flags) {
		bitmask_snprintf(ci->ci_feature2_flags,
		    CPUID2_FLAGS, buf, sizeof(buf));
		aprint_verbose("%s: features2: %s\n",
		    ci->ci_dev->dv_xname, buf);
	}

	if (vendor == CPUVENDOR_INTEL &&
	    (ci->ci_feature_flags & CPUID_MASK4) != 0) {
		bitmask_snprintf(ci->ci_feature_flags,
		    CPUID_FLAGS4, buf, sizeof(buf));
		aprint_verbose("%s: features3: %s\n",
		    ci->ci_dev->dv_xname, buf);
	}

	x86_print_cacheinfo(ci);

#ifdef POWERNOW_K8
	if (CPUID2FAMILY(ci->ci_signature) == 15 &&
	    (cpu_model[0] == 'A' || cpu_model[0] == 'O') &&
	    powernow_probe(ci))
		k8_powernow_init();
#endif

	x86_errata(ci, vendor);
	x86_patch();

#ifdef INTEL_ONDEMAND_CLOCKMOD
	clockmod_init();
#endif

#ifdef ENHANCED_SPEEDSTEP
	if ((vendor == CPUVENDOR_INTEL) &&
	    (ci->ci_feature2_flags & CPUID2_EST)) {
		if (rdmsr(MSR_MISC_ENABLE) & (1 << 16))
			est_init(CPUVENDOR_INTEL);
		else
			aprint_normal("%s: Enhanced SpeedStep disabled by "
			    "BIOS\n", device_xname(ci->ci_dev));
	}
#endif

}

void
cpu_probe_features(struct cpu_info *ci)
{
	ci->ci_feature_flags = cpu_feature;
	ci->ci_signature = 0;
}
