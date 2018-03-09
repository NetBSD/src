/*	$NetBSD: sysv_mod.c,v 1.1.2.3 2018/03/09 03:58:32 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sysv_mod.c,v 1.1.2.3 2018/03/09 03:58:32 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_50.h"
#include "opt_compat_14.h"
#include "opt_compat_10.h"
#include "opt_sysv.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/sysctl.h>

#include <compat/common/sysv_ipc.h>

#ifdef COMPAT_50
int sysctl_kern_sysvipc50(SYSCTLFN_PROTO);
#endif

MODULE(MODULE_CLASS_EXEC, compat_sysv_ipc, "sysv_ipc");

/* Build the syscall package based on options specified */

static const struct syscall_package compat_syscalls[] = {
#if defined(COMPAT_10) && !defined(_LP64)
#ifdef	SYSVSHM
	{ SYS_compat_10_oshmsys, 0, (sy_call_t *)compat_10_sys_shmsys },
#endif
#ifdef	SYSVSEM
	{ SYS_compat_10_osemsys, 0, (sy_call_t *)compat_10_sys_semsys },
#endif
#ifdef	SYSVMSG
	{ SYS_compat_10_omsgsys, 0, (sy_call_t *)compat_10_sys_msgsys },
#endif
#endif /* defined(COMPAT_10) && !defined(_LP64) */

#if defined(COMPAT_14)
#ifdef SYSVSHM
	{ SYS_compat_14_shmctl, 0, (sy_call_t *)compat_14_sys_shmctl },
#endif
#ifdef	SYSVSEM
	{ SYS_compat_14___semctl, 0, (sy_call_t *)compat_14_sys___semctl },
#endif
#ifdef	SYSVMSG
	{ SYS_compat_14_msgctl, 0, (sy_call_t *)compat_14_sys_msgctl },
#endif
#endif	/* defined(COMPAT_14) */

#if defined(COMPAT_50)
#ifdef SYSVSHM
	{ SYS_compat_50___shmctl13, 0, (sy_call_t *)compat_50_sys___shmctl13 },
#endif
#ifdef	SYSVSEM
	{ SYS_compat_50_____semctl13, 0, (sy_call_t *)compat_50_sys_____semctl13 },
#endif
#ifdef	SYSVMSG
	{ SYS_compat_50___msgctl13, 0, (sy_call_t *)compat_50_sys___msgctl13 },
#endif
#endif	/* defined(COMPAT_50) */

	{ 0, 0, NULL }
};

static int
compat_sysv_ipc_modcmd(modcmd_t cmd, void *arg)
{
	static (*orig_sysvipc50_sysctl)(SYSCTLFN_PROTO);

	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(NULL, compat_syscalls);
		if (error != 0) {
			break;
		}
#if defined(COMPAT_50)
		orig_sysvipc50_sysctl = vec_sysvipc50_sysctl;
		vec_sysvipc50_sysctl = sysctl_kern_sysvipc50;
#endif
		break;

	case MODULE_CMD_FINI:
#if defined(COMPAT_50)
		vec_sysvipc50_sysctl = orig_sysvipc50_sysctl;
#endif
		error = syscall_disestablish(NULL, compat_syscalls);
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
