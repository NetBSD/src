/* $NetBSD: osf1_exec.c,v 1.45.2.1 2018/05/21 04:36:04 pgoyette Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: osf1_exec.c,v 1.45.2.1 2018/05/21 04:36:04 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscall.h>
#include <compat/osf1/osf1_cvt.h>

extern struct sysent osf1_sysent[];
extern const char * const osf1_syscallnames[];
extern char osf1_sigcode[], osf1_esigcode[];
#ifdef __HAVE_SYSCALL_INTERN
void osf1_syscall_intern(struct proc *);
#else
void syscall(void);
#endif

struct uvm_object *emul_osf1_object;

struct emul emul_osf1 = {
	.e_name =		"osf1",
	.e_path =		"/emul/osf1",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		native_to_osf1_errno,
	.e_nosys =		OSF1_SYS_syscall,
	.e_nsysent =		OSF1_SYS_NSYSENT,
#endif
	.e_sysent =		osf1_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	osf1_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		sendsig_sigcontext,
	.e_trapsignal =		trapsignal,
	.e_sigcode =		osf1_sigcode,
	.e_esigcode =		osf1_esigcode,
	.e_sigobject =		&emul_osf1_object,
	.e_setregs =		setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	osf1_syscall_intern,
#else
	.e_syscall_intern =	syscall,
#endif
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};
