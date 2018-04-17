/*	$NetBSD: compat_sysv_10_mod.c,v 1.1.2.1 2018/04/17 23:06:11 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Paul Goyette
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
__KERNEL_RCSID(0, "$NetBSD: compat_sysv_10_mod.c,v 1.1.2.1 2018/04/17 23:06:11 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_sysv.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/sysctl.h>

MODULE(MODULE_CLASS_EXEC, compat_sysv_10, "sysv_ipc,sysv_50,sysv_14");

/* Build the syscall package based on options specified */

static const struct syscall_package compat_sysv_10_syscalls[] = {
#if !defined(_LP64)
#ifdef	SYSVSHM
	{ SYS_compat_10_oshmsys, 0, (sy_call_t *)compat_10_sys_shmsys },
#endif
#ifdef	SYSVSEM
	{ SYS_compat_10_osemsys, 0, (sy_call_t *)compat_10_sys_semsys },
#endif
#ifdef	SYSVMSG
	{ SYS_compat_10_omsgsys, 0, (sy_call_t *)compat_10_sys_msgsys },
#endif
#endif /* !defined(_LP64) */
	{ 0, 0, NULL }
};

static int
compat_sysv_10_modcmd(modcmd_t cmd, void *arg)
{

	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(NULL, compat_sysv_10_syscalls);
		if (error != 0) {
			break;
		}
		break;

	case MODULE_CMD_FINI:
		error = syscall_disestablish(NULL, compat_sysv_10_syscalls);
		if (error != 0) {
			break;
		}
		break;

	default:
		error = ENOTTY;
		break;
	}
	return error;
}
