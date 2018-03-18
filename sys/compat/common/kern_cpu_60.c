/*	$NetBSD: kern_cpu_60.c,v 1.1.2.3 2018/03/18 01:17:29 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: kern_cpu_60.c,v 1.1.2.3 2018/03/18 01:17:29 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_cpu_ucode.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/cpuio.h>

#include <compat/common/compat_mod.h>

static int
compat6_cpuctl_ioctl(struct lwp *l, u_long cmd, void *data)
{
#if defined(CPU_UCODE) && defined(COMPAT_60)
	int error;
#endif

	switch (cmd) {
#if defined(CPU_UCODE) && defined(COMPAT_60)
	case OIOC_CPU_UCODE_GET_VERSION:
		return compat6_cpu_ucode_get_version(data);

	case OIOC_CPU_UCODE_APPLY:
		error = kauth_authorize_machdep(l->l_cred,
		    KAUTH_MACHDEP_CPU_UCODE_APPLY, NULL, NULL, NULL, NULL);
		if (error)
			return error;
		return compat6_cpu_ucode_apply(data);
#endif
 	default:
		return ENOTTY;
 	}
}

int
kern_cpu_60_init(void)
{

	compat_cpuctl_ioctl = compat6_cpuctl_ioctl;
	return 0;
}

int
kern_cpu_60_fini(void)
{

	compat_cpuctl_ioctl = (void *)enosys;
	return 0;
}
