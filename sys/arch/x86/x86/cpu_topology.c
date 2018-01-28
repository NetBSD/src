/*	$NetBSD: cpu_topology.c,v 1.12 2018/01/28 16:15:41 mlelstv Exp $	*/

/*-
 * Copyright (c) 2009 Mindaugas Rasiukevicius <rmind at NetBSD org>,
 * Copyright (c) 2008 YAMAMOTO Takashi,
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

/*
 * x86 CPU topology detection.
 *
 * References:
 * - 53668.pdf (7.10.2), 276613.pdf
 * - 31116.pdf, 41256.pdf, 25481.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_topology.c,v 1.12 2018/01/28 16:15:41 mlelstv Exp $");

#include <sys/param.h>
#include <sys/bitops.h>

#include <machine/specialreg.h>
#include <machine/cpu.h>

#include <x86/cpufunc.h>
#include <x86/cputypes.h>
#include <x86/cpuvar.h>

void
x86_cpu_topology(struct cpu_info *ci)
{
	u_int lp_max;		/* Logical processors per package (node) */
	u_int core_max;		/* Core per package */
	int n, cpu_family, apic_id, smt_bits, core_bits = 0;
	uint32_t descs[4];

	apic_id = ci->ci_initapicid;
	cpu_family = CPUID_TO_FAMILY(ci->ci_signature);

	/* Initial values. */
	ci->ci_package_id = apic_id;
	ci->ci_core_id = 0;
	ci->ci_smt_id = 0;

	switch (cpu_vendor) {
	case CPUVENDOR_INTEL:
		if (cpu_family < 6)
			return;
		break;
	case CPUVENDOR_AMD:
		if (cpu_family < 0xf)
			return;
		break;
	default:
		return;
	}

	/* Check for HTT support.  See notes below regarding AMD. */
	if ((ci->ci_feat_val[0] & CPUID_HTT) != 0) {
		/* Maximum number of LPs sharing a cache (ebx[23:16]). */
		x86_cpuid(1, descs);
		lp_max = __SHIFTOUT(descs[1], CPUID_HTT_CORES);
	} else {
		lp_max = 1;
	}

	switch (cpu_vendor) {
	case CPUVENDOR_INTEL:
		/* Check for leaf 4 support. */
		if (ci->ci_max_cpuid >= 4) {
			/* Maximum number of Cores per package (eax[31:26]). */
			x86_cpuid2(4, 0, descs);
			core_max = (descs[0] >> 26) + 1;
		} else {
			core_max = 1;
		}
		break;
	case CPUVENDOR_AMD:
		/* In a case of AMD, HTT flag means CMP support. */
		if ((ci->ci_feat_val[0] & CPUID_HTT) == 0) {
			core_max = 1;
			break;
		}
		/* Legacy Method, LPs represent Cores. */
		if (cpu_family < 0x10 || ci->ci_max_ext_cpuid < 0x80000008) {
			core_max = lp_max;
			break;
		}

		/* Number of Cores (NC) per package (ecx[7:0]). */
		x86_cpuid(0x80000008, descs);
		core_max = (descs[2] & 0xff) + 1;
		/* Amount of bits representing Core ID (ecx[15:12]). */
		n = (descs[2] >> 12) & 0x0f;
		if (n != 0) {
			/*
			 * Extended Method.
			 * core_max = 2 ^ n (power of two)
			 */
			core_bits = n;
		}
		break;
	default:
		core_max = 1;
	}

	KASSERT(lp_max >= core_max);
	smt_bits = ilog2((lp_max / core_max) - 1) + 1;
	if (core_bits == 0) {
		core_bits = ilog2(core_max - 1) + 1;
	}

	/*
	 * Family 0xf and 0x10 processors may have different structure of
	 * APIC ID.  Detect that via special MSR register and move the bits,
	 * if necessary (ref: InitApicIdCpuIdLo).
	 */
	if (cpu_vendor == CPUVENDOR_AMD && cpu_family < 0x11) {	/* XXX */
		const uint64_t reg = rdmsr(MSR_NB_CFG);
		if ((reg & NB_CFG_INITAPICCPUIDLO) == 0) {
			/*
			 * 0xf:  { CoreId, NodeId[2:0] }
			 * 0x10: { CoreId[1:0], 000b, NodeId[2:0] }
			 */
			const u_int node_id = apic_id & __BITS(0, 2);
			apic_id = (cpu_family == 0xf) ?
			    (apic_id >> core_bits) | (node_id << core_bits) :
			    (apic_id >> 5) | (node_id << 2);
		}
	}

	if (smt_bits + core_bits) {
		if (smt_bits + core_bits < sizeof(apic_id) * NBBY)
			ci->ci_package_id = apic_id >> (smt_bits + core_bits);
		else
			ci->ci_package_id = 0;
	}
	if (core_bits) {
		u_int core_mask = __BITS(smt_bits, smt_bits + core_bits - 1);
		ci->ci_core_id = __SHIFTOUT(apic_id, core_mask);
	}
	if (smt_bits) {
		u_int smt_mask = __BITS(0, smt_bits - 1);
		ci->ci_smt_id = __SHIFTOUT(apic_id, smt_mask);
	}
}
