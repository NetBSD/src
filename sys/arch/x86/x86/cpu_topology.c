/*	$NetBSD: cpu_topology.c,v 1.2.2.2 2009/06/16 02:19:44 snj Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu_topology.c,v 1.2.2.2 2009/06/16 02:19:44 snj Exp $");

#include <sys/param.h>
#include <sys/bitops.h>

#include <machine/specialreg.h>
#include <machine/cpu.h>

#include <x86/cpufunc.h>
#include <x86/cputypes.h>
#include <x86/cpuvar.h>

void
x86_cpu_toplogy(struct cpu_info *ci)
{
	u_int lp_max = 1;	/* logical processors per package */
	u_int smt_max;		/* smt per core */
	u_int core_max = 1;	/* core per package */
	int smt_bits, core_bits;
	uint32_t descs[4];

	if (cpu_vendor != CPUVENDOR_INTEL ||
	    CPUID2FAMILY(ci->ci_signature) < 6)
		return;

	/* Determine the extended feature flags. */
	x86_cpuid(0x80000000, descs);
	if (descs[0] >= 0x80000001) {
		x86_cpuid(0x80000001, descs);
		ci->ci_feature3_flags |= descs[3]; /* %edx */
	}

	/* Determine topology. 253668.pdf 7.10.2. */
	ci->ci_packageid = ci->ci_initapicid;
	ci->ci_coreid = 0;
	ci->ci_smtid = 0;
	if ((ci->ci_feature_flags & CPUID_HTT) != 0) {
		x86_cpuid(1, descs);
		lp_max = (descs[1] >> 16) & 0xff;
	}
	x86_cpuid(0, descs);
	if (descs[0] >= 4) {
		x86_cpuid2(4, 0, descs);
		core_max = (descs[0] >> 26) + 1;
	}
	KASSERT(lp_max >= core_max);
	smt_max = lp_max / core_max;
	smt_bits = ilog2(smt_max - 1) + 1;
	core_bits = ilog2(core_max - 1) + 1;
	if (smt_bits + core_bits) {
		ci->ci_packageid = ci->ci_initapicid >> (smt_bits + core_bits);
	}
	if (core_bits) {
		u_int core_mask = __BITS(smt_bits, smt_bits + core_bits - 1);
		ci->ci_coreid = __SHIFTOUT(ci->ci_initapicid, core_mask);
	}
	if (smt_bits) {
		u_int smt_mask = __BITS(0, smt_bits - 1);
		ci->ci_smtid = __SHIFTOUT(ci->ci_initapicid, smt_mask);
	}
}
