/*	$NetBSD: cacheinfo.c,v 1.12.8.1 2008/01/08 22:10:36 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: cacheinfo.c,v 1.12.8.1 2008/01/08 22:10:36 bouyer Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/null.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/specialreg.h>

#include <x86/cpufunc.h>

static const char *print_cache_config(struct cpu_info *, int, const char *,
    const char *);
static const char *print_tlb_config(struct cpu_info *, int, const char *,
    const char *);

static const char *
print_cache_config(struct cpu_info *ci, int cache_tag, const char *name,
    const char *sep)
{
	char cbuf[7];
	struct x86_cache_info *cai = &ci->ci_cinfo[cache_tag];

	if (cai->cai_totalsize == 0)
		return sep;

	if (sep == NULL)
		aprint_verbose("%s: ", ci->ci_dev->dv_xname);
	else
		aprint_verbose("%s", sep);
	if (name != NULL)
		aprint_verbose("%s ", name);

	if (cai->cai_string != NULL) {
		aprint_verbose("%s ", cai->cai_string);
	} else {
		format_bytes(cbuf, sizeof(cbuf), cai->cai_totalsize);
		aprint_verbose("%s %dB/line ", cbuf, cai->cai_linesize);
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
	char cbuf[7];
	struct x86_cache_info *cai = &ci->ci_cinfo[cache_tag];

	if (cai->cai_totalsize == 0)
		return sep;

	if (sep == NULL)
		aprint_verbose("%s: ", ci->ci_dev->dv_xname);
	else
		aprint_verbose("%s", sep);
	if (name != NULL)
		aprint_verbose("%s ", name);

	if (cai->cai_string != NULL) {
		aprint_verbose("%s", cai->cai_string);
	} else {
		format_bytes(cbuf, sizeof(cbuf), cai->cai_linesize);
		aprint_verbose("%d %s entries ", cai->cai_totalsize, cbuf);
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

const struct x86_cache_info *
cache_info_lookup(const struct x86_cache_info *cai, u_int8_t desc)
{
	int i;

	for (i = 0; cai[i].cai_desc != 0; i++) {
		if (cai[i].cai_desc == desc)
			return (&cai[i]);
	}

	return (NULL);
}


static const struct x86_cache_info amd_cpuid_l2cache_assoc_info[] = {
	{ 0, 0x01,    1, 0, 0, NULL },
	{ 0, 0x02,    2, 0, 0, NULL },
	{ 0, 0x04,    4, 0, 0, NULL },
	{ 0, 0x06,    8, 0, 0, NULL },
	{ 0, 0x08,   16, 0, 0, NULL },
	{ 0, 0x0f, 0xff, 0, 0, NULL },
	{ 0, 0x00,    0, 0, 0, NULL },
};

void
amd_cpu_cacheinfo(struct cpu_info *ci)
{
	const struct x86_cache_info *cp;
	struct x86_cache_info *cai;
	int family, model;
	u_int descs[4];
	u_int lfunc;

	family = (ci->ci_signature >> 8) & 15;
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
		family += (ci->ci_signature >> 20) & 0xff;
		model += (ci->ci_signature >> 16) & 0xf;
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
}

void
via_cpu_cacheinfo(struct cpu_info *ci)
{
	struct x86_cache_info *cai;
	int family, model, stepping;
	u_int descs[4];
	u_int lfunc;

	family = (ci->ci_signature >> 8) & 15;
	model = CPUID2MODEL(ci->ci_signature);
	stepping = CPUID2STEPPING(ci->ci_signature);

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
	if (model == 9 && stepping == 8) {
		/* Erratum: stepping 8 reports 4 when it should be 2 */
		cai->cai_associativity = 2;
	}

	cai = &ci->ci_cinfo[CAI_ICACHE];
	cai->cai_totalsize = VIA_L1_EDX_IC_SIZE(descs[3]);
	cai->cai_associativity = VIA_L1_EDX_IC_ASSOC(descs[3]);
	cai->cai_linesize = VIA_L1_EDX_IC_LS(descs[3]);
	if (model == 9 && stepping == 8) {
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
	if (model >= 9) {
		cai->cai_totalsize = VIA_L2N_ECX_C_SIZE(descs[2]);
		cai->cai_associativity = VIA_L2N_ECX_C_ASSOC(descs[2]);
		cai->cai_linesize = VIA_L2N_ECX_C_LS(descs[2]);
	} else {
		cai->cai_totalsize = VIA_L2_ECX_C_SIZE(descs[2]);
		cai->cai_associativity = VIA_L2_ECX_C_ASSOC(descs[2]);
		cai->cai_linesize = VIA_L2_ECX_C_LS(descs[2]);
	}
}

void
x86_print_cacheinfo(struct cpu_info *ci)
{
	const char *sep;

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
}
