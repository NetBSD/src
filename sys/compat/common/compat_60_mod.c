/*	$NetBSD: compat_60_mod.c,v 1.1.2.2 2018/03/16 01:16:29 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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
 * Linkage for the compat module: spaghetti.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_60_mod.c,v 1.1.2.2 2018/03/16 01:16:29 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>

static const struct syscall_package compat_60_syscalls[] = {
	{ SYS_compat_60_lwp_park, 0, (sy_call_t *)compat_60_sys__lwp_park },
	{ NULL, 0, NULL }
};

#define REQUIRED_60 "compat_70"		/* XXX No compat_80 yet */
MODULE(MODULE_CLASS_EXEC, compat_60, REQUIRED_60);

#ifdef CPU_UCODE
int (*orig_compat_6_cpu_ucode)(struct compat6_cpu_ucode *);
int (*orig_compat6_cpu_ucode_apply)(const struct compat6_cpu_ucode *);
#endif

static int
compat_60_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(NULL, compat_60_syscalls);
		if (error != 0)
			return error;
#ifdef CPU_UCODE
		orig_get_version = vec_compat6_cpu_ucode_get_version;
		*vec_compat6_cpu_ucode_get_version =
		    compat6_cpu_ucode_get_version;
		orig_apply = vec_compat6_cpu_ucode_apply;
		*vec_compat6_cpu_ucode_apply = compat6_cpu_ucode_apply;
#endif
		return 0;

	case MODULE_CMD_FINI:
		*vec_compat6_cpu_ucode_get_version = orig_get_version;
		*vec_compat6_cpu_ucode_apply = orig_apply;
		error = syscall_disestablish(NULL, compat_60_syscalls);
		if (error != 0)
			return error;
		return 0;

	default:
		return ENOTTY;
	}
}


