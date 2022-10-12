/*	$NetBSD: cpu_topology.c,v 1.21 2022/10/12 10:26:09 msaitoh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu_topology.c,v 1.21 2022/10/12 10:26:09 msaitoh Exp $");

#include "acpica.h"

#include <sys/param.h>
#include <sys/bitops.h>
#include <sys/cpu.h>

#include <machine/specialreg.h>

#include <dev/acpi/acpi_srat.h>

#include <x86/cpufunc.h>
#include <x86/cputypes.h>
#include <x86/cpuvar.h>

static uint32_t
x86_cpu_get_numa_node(uint32_t apic_id)
{
#if NACPICA > 0
	uint32_t i, j, nn, nc;
	struct acpisrat_cpu c;

	nn = acpisrat_nodes();
	for (i = 0; i < nn; i++) {
		nc = acpisrat_node_cpus(i);
		for (j = 0; j < nc; j++) {
			acpisrat_cpu(i, j, &c);
			if (c.apicid == apic_id) {
				return c.nodeid;
			}
		}
	}
#endif
	return 0;
}

void
x86_cpu_topology(struct cpu_info *ci)
{
	u_int lp_max;		/* Logical processors per package (node) */
	u_int core_max;		/* Core per package */
	int n, cpu_family, apic_id, smt_bits, core_bits = 0;
	uint32_t descs[4];
	u_int package_id, core_id, smt_id, numa_id;

	apic_id = ci->ci_initapicid;
	cpu_family = CPUID_TO_FAMILY(ci->ci_signature);

	/* Initial values. */
	package_id = apic_id;
	core_id = 0;
	smt_id = 0;
	numa_id = x86_cpu_get_numa_node(apic_id);

	switch (cpu_vendor) {
	case CPUVENDOR_INTEL:
		if (cpu_family < 6) {
			cpu_topology_set(ci, package_id, core_id, smt_id,
			    numa_id);
			return;
		}
		break;
	case CPUVENDOR_AMD:
		if (cpu_family < 0xf) {
			cpu_topology_set(ci, package_id, core_id, smt_id,
			    numa_id);
			return;
		}
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
			core_max = __SHIFTOUT(descs[0], CPUID_DCP_CORE_P_PKG)
			    + 1;
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

		/* Number of Cores (NC) per package. */
		x86_cpuid(0x80000008, descs);
		core_max = __SHIFTOUT(descs[2], CPUID_CAPEX_NC) + 1;
		/* Amount of bits representing Core ID (ecx[15:12]). */
		n = __SHIFTOUT(descs[2], CPUID_CAPEX_ApicIdSize);
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

	/* Family 0x17 and above support SMT */
	if (cpu_vendor == CPUVENDOR_AMD && cpu_family >= 0x17) { /* XXX */
		x86_cpuid(0x8000001e, descs);
		const u_int threads = __SHIFTOUT(descs[1],
		    CPUID_AMD_PROCT_THREADS_PER_CORE) + 1;

		KASSERT(smt_bits == 0);
		smt_bits = ilog2(threads);
		KASSERT(smt_bits <= core_bits);
		core_bits -= smt_bits;
	}

	if (smt_bits + core_bits) {
		if (smt_bits + core_bits < sizeof(apic_id) * NBBY)
			package_id = apic_id >> (smt_bits + core_bits);
		else
			package_id = 0;
	}
	if (core_bits) {
		u_int core_mask = __BITS(smt_bits, smt_bits + core_bits - 1);
		core_id = __SHIFTOUT(apic_id, core_mask);
	}
	if (smt_bits) {
		u_int smt_mask = __BITS(0, smt_bits - 1);
		smt_id = __SHIFTOUT(apic_id, smt_mask);
	}

	cpu_topology_set(ci, package_id, core_id, smt_id, numa_id);
}
