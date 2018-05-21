/*	$NetBSD: svr4_32_exec.c,v 1.28.2.1 2018/05/21 04:36:04 pgoyette Exp $	 */

/*-
 * Copyright (c) 1994, 2000 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_exec.c,v 1.28.2.1 2018/05/21 04:36:04 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#define	ELFSIZE		32				/* XXX should die */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <machine/svr4_machdep.h>
#include <machine/svr4_32_machdep.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_syscall.h>
#include <compat/svr4/svr4_errno.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <compat/svr4_32/svr4_32_exec.h>

extern char svr4_32_sigcode[], svr4_32_esigcode[];
extern struct sysent svr4_32_sysent[];
extern const char * const svr4_32_syscallnames[];
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#endif

struct uvm_object *emul_svr4_32_object;

struct emul emul_svr4_32 = {
	.e_name =		"svr4_32",
	.e_path =		"/emul/svr4_32",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		native_to_svr4_errno,
	.e_nosys =		SVR4_32_SYS_syscall,
	.e_nsysent =		SVR4_32_SYS_NSYSENT,
#endif
	.e_sysent =		svr4_32_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	svr4_32_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		svr4_32_sendsig,
	.e_trapsignal =		trapsignal,
	.e_sigcode =		svr4_32_sigcode,
	.e_esigcode =		svr4_32_esigcode,
	.e_sigobject =		&emul_svr4_32_object,
	.e_setregs =		svr4_32_setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	svr4_32_syscall_intern,
#else
	.e_syscall_intern =	syscall,
#endif
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	svr4_32_vm_default_addr,
	.e_usertrap =		NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};
