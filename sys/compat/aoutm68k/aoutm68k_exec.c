/*	$NetBSD: aoutm68k_exec.c,v 1.26 2010/10/15 16:51:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
__KERNEL_RCSID(0, "$NetBSD: aoutm68k_exec.c,v 1.26 2010/10/15 16:51:09 tsutsui Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/signalvar.h>
#include <sys/syscallvar.h>

#include <uvm/uvm_extern.h>

#include <compat/aoutm68k/aoutm68k_syscall.h>

extern struct sysent aoutm68k_sysent[];
extern char sigcode[], esigcode[];
void aoutm68k_syscall_intern(struct proc *);

struct uvm_object *emul_netbsd_aoutm68k_object;

struct emul emul_netbsd_aoutm68k = {
	.e_name =		"aoutm68k",
	.e_path =		NULL,
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		EMUL_HAS_SYS___syscall,
	.e_errno =		NULL,
	.e_nosys =		AOUTM68K_SYS_syscall,
	.e_nsysent =		AOUTM68K_SYS_NSYSENT,
#endif
	.e_sysent =		aoutm68k_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	syscallnames,
#endif
	.e_sendsig =		sendsig,
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
	.e_sigcode =		sigcode,
	.e_esigcode =		esigcode,
	.e_sigobject =		&emul_netbsd_aoutm68k_object,
	.e_setregs =		setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
	.e_syscall_intern =	aoutm68k_syscall_intern,
	.e_sysctlovly =		NULL,
	.e_fault =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		NULL,
	.e_sa =			NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};
