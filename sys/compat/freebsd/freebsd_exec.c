/*	$NetBSD: freebsd_exec.c,v 1.42 2018/08/10 21:44:58 pgoyette Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
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
__KERNEL_RCSID(0, "$NetBSD: freebsd_exec.c,v 1.42 2018/08/10 21:44:58 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <compat/freebsd/freebsd_syscall.h>
#include <compat/freebsd/freebsd_exec.h>
#include <compat/freebsd/freebsd_signal.h>
#include <compat/common/compat_util.h>

#include <compat/freebsd/freebsd_machdep.h>

extern struct sysent freebsd_sysent[];
extern const uint32_t freebsd_sysent_nomodbits[];
extern const char * const freebsd_syscallnames[];

struct uvm_object *emul_freebsd_object;

#ifndef __HAVE_SYSCALL_INTERN
void	syscall(void);
#endif

struct emul emul_freebsd = {
	.e_name =		"freebsd",
	.e_path =		"/emul/freebsd",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		EMUL_HAS_SYS___syscall,
	.e_errno =		NULL,
	.e_nosys =		FREEBSD_SYS_syscall,
	.e_nsysent =		FREEBSD_SYS_NSYSENT,
#endif
	.e_sysent =		freebsd_sysent,
	.e_nomodbits =		freebsd_sysent_nomodbits,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	freebsd_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		freebsd_sendsig,
	.e_trapsignal =		trapsignal,
	.e_sigcode =		freebsd_sigcode,
	.e_esigcode =		freebsd_esigcode,
	.e_sigobject =		&emul_freebsd_object,
	.e_setregs =		freebsd_setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	freebsd_syscall_intern,
#else
	.e_syscall_intern =	syscall,
#endif
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};
