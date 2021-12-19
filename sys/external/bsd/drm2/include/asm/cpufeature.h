/*	$NetBSD: cpufeature.h,v 1.9 2021/12/19 11:52:47 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_ASM_CPUFEATURE_H_
#define	_LINUX_ASM_CPUFEATURE_H_

#include <sys/cpu.h>

#if defined(__i386__) || defined(__x86_64__)

#include <x86/specialreg.h>

#define	cpu_has_clflush	((cpu_info_primary.ci_feat_val[0] & CPUID_CLFSH) != 0)
#define	cpu_has_pat	((cpu_info_primary.ci_feat_val[0] & CPUID_PAT) != 0)

#define	X86_FEATURE_CLFLUSH	0
#define	X86_FEATURE_PAT		1

static inline bool
static_cpu_has(int feature)
{
	switch (feature) {
	case X86_FEATURE_CLFLUSH:
		return cpu_has_clflush;
	case X86_FEATURE_PAT:
		return cpu_has_pat;
	default:
		return false;
	}
}

#define	boot_cpu_has	static_cpu_has

static inline size_t
cache_line_size(void)
{
	return cpu_info_primary.ci_cflush_lsize;
}

static inline void
clflush(const void *p)
{
	asm volatile ("clflush %0" : : "m" (*(const char *)p));
}

#endif	/* x86 */

#endif	/* _LINUX_ASM_CPUFEATURE_H_ */
