/*	$NetBSD: sunos_exec.c,v 1.55 2018/01/09 20:55:43 maya Exp $	*/

/*
 * Copyright (c) 1993 Theo de Raadt
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos_exec.c,v 1.55 2018/01/09 20:55:43 maya Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_exec.h>
#include <compat/sunos/sunos_syscall.h>

#include <machine/sunos_machdep.h>

extern int nsunos_sysent;
extern struct sysent sunos_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const sunos_syscallnames[];
#endif
extern char sunos_sigcode[], sunos_esigcode[];
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#endif

struct uvm_object *emul_sunos_object;

struct emul emul_sunos = {
	.e_name =		"sunos",
	.e_path =		"/emul/sunos",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		NULL,
	.e_nosys =		SUNOS_SYS_syscall,
	.e_nsysent =		SUNOS_SYS_NSYSENT,
#endif
	.e_sysent =		sunos_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	sunos_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		sunos_sendsig,
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
	.e_sigcode =		sunos_sigcode,
	.e_esigcode =		sunos_esigcode,
	.e_sigobject =		&emul_sunos_object,
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
