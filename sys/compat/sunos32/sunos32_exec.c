/*	$NetBSD: sunos32_exec.c,v 1.34 2018/01/09 20:55:43 maya Exp $	 */

/*
 * Copyright (c) 2001 Matthew R. Green
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos32_exec.c,v 1.34 2018/01/09 20:55:43 maya Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_syscall.h>
#include <compat/sunos32/sunos32_exec.h>

#include <machine/sunos_machdep.h>

extern int nsunos32_sysent;
extern struct sysent sunos32_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const sunos32_syscallnames[];
#endif
extern char sunos_sigcode[], sunos_esigcode[];

#ifndef __HAVE_SYSCALL_INTERN
void	syscall(void);
#endif

struct uvm_object *emul_sunos32_object;

struct emul emul_sunos = {
	.e_name =		"sunos32",
	.e_path =		"/emul/sunos32",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		NULL,
	.e_nosys =		SUNOS32_SYS_syscall,
	.e_nsysent =		SUNOS32_SYS_NSYSENT,
#endif
	.e_sysent =		sunos32_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	sunos32_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		sunos32_sendsig,
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
	.e_sigcode =		sunos_sigcode,
	.e_esigcode =		sunos_esigcode,
	.e_sigobject =		&emul_sunos32_object,
	.e_setregs =		setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	sunos_syscall_intern,
#else
	.e_syscall_intern =	syscall,
#endif
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};
