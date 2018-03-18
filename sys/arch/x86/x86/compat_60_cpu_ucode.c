/* $NetBSD: compat_60_cpu_ucode.c,v 1.1 2018/03/18 00:17:18 christos Exp $ */
/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christoph Egger.
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
__KERNEL_RCSID(0, "$NetBSD: compat_60_cpu_ucode.c,v 1.1 2018/03/18 00:17:18 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_cpu_ucode.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/cpuio.h>
#include <sys/cpu.h>

#include <dev/firmload.h>

#include <machine/cpuvar.h>
#include <machine/cputypes.h>

#include <x86/cpu_ucode.h>

#ifdef COMPAT_60
int
compat6_cpu_ucode_get_version(struct compat6_cpu_ucode *data6)
{

	if (cpu_vendor != CPUVENDOR_AMD)
		return EOPNOTSUPP;

	struct cpu_ucode_version data;

	data.loader_version = CPU_UCODE_LOADER_AMD;
	return cpu_ucode_amd_get_version(&data, &data6->version,
	    sizeof(data6->version));
}

int
compat6_cpu_ucode_apply(const struct compat6_cpu_ucode *data6)
{

	if (cpu_vendor != CPUVENDOR_AMD)
		return EOPNOTSUPP;

	struct cpu_ucode data;

	data.loader_version = CPU_UCODE_LOADER_AMD;
	data.cpu_nr = CPU_UCODE_ALL_CPUS;
	strcpy(data.fwname, data6->fwname);

	return cpu_ucode_apply(&data);
}

#endif /* COMPAT60 */
