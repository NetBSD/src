/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__RCSID("$NetBSD: arm_initfini.c,v 1.10 2025/03/03 17:00:22 christos Exp $");

#include "namespace.h"

/*
 * To properly implement setjmp/longjmp for the ARM AAPCS ABI, it has to be
 * aware of whether there is a FPU is present or not.  Regardless of whether
 * the hard-float ABI is being used, setjmp needs to save D8-D15.  But it can
 * only do this if those instructions won't cause an exception.
 */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

__dso_hidden int _libc_arm_fpu_present;
#ifndef __ARM_ARCH_EXT_IDIV__
__dso_hidden int _libc_arm_hwdiv_present;
# define NENTRIES 2
#else
# define NENTRIES 1
#endif

static bool _libc_aapcs_initialized;

void	_libc_aapcs_init(void) __attribute__((__constructor__, __used__));

void __section(".text.startup")
_libc_aapcs_init(void)
{
	if (_libc_aapcs_initialized)
		return;

	struct sysctlnode query, md[64];
	int mib[2];
	size_t len, mlen = sizeof(_libc_arm_fpu_present), nentries = 0;

	_libc_aapcs_initialized = true;
	mib[0] = CTL_MACHDEP;
	mib[1] = CTL_QUERY;
	memset(&query, 0, sizeof(query));
	query.sysctl_flags = SYSCTL_VERSION;
	len = sizeof(md);
	if (sysctl(mib, 2, md, &len, &query, sizeof(query)) == -1)
		return;

	for (size_t i = 0; i < len / sizeof(md[0]); i++) {
		if (strcmp(md[i].sysctl_name, "fpu_present") == 0) {
			mib[1] = md[i].sysctl_num;
			(void)sysctl(mib, 2, &_libc_arm_fpu_present, &mlen,
			    NULL, 0);
			nentries++;

		}
#ifndef __ARM_ARCH_EXT_IDIV__
		else if (strcmp(md[i].sysctl_name, "hwdiv_present") == 0) {
			mib[1] = md[i].sysctl_num;
			(void)sysctl(mib, 2, &_libc_arm_hwdiv_present, &mlen,
			    NULL, 0);
			nentries++;
		}
#endif
		if (nentries >= NENTRIES)
			return;
	}
}
