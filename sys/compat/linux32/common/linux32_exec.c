/*	$NetBSD: linux32_exec.c,v 1.22.2.1 2018/05/21 04:36:03 pgoyette Exp $ */

/*-
 * Copyright (c) 1994-2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden, Eric Haszlakiewicz,
 * Thor Lancelot Simon, and Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: linux32_exec.c,v 1.22.2.1 2018/05/21 04:36:03 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_exec.h>

#include <compat/linux32/common/linux32_exec.h>
#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>

#include <compat/linux32/linux32_syscallargs.h>
#include <compat/linux32/linux32_syscall.h>

extern struct sysent linux32_sysent[];
extern const char * const linux32_syscallnames[];
extern char linux32_sigcode[], linux32_esigcode[];

/*
 * Emulation switch.
 */

struct uvm_object *emul_linux32_object;

struct emul emul_linux32 = {
	.e_name =		"linux32",
	.e_path =		"/emul/linux32",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		NULL,
	.e_nosys =		LINUX32_SYS_syscall,
	.e_nsysent =		LINUX32_SYS_NSYSENT,
#endif
	.e_sysent =		linux32_sysent,
	.e_syscallnames =	linux32_syscallnames,
	.e_sendsig =		linux32_sendsig,
	.e_trapsignal =		linux_trapsignal,
	.e_sigcode =		linux32_sigcode,
	.e_esigcode =		linux32_esigcode,
	.e_sigobject =		&emul_linux32_object,
	.e_setregs =		linux32_setregs,
	.e_proc_exec =		linux_e_proc_exec,
	.e_proc_fork =		linux_e_proc_fork,
	.e_proc_exit =		linux_e_proc_exit,
	.e_lwp_fork =		linux_e_lwp_fork,
	.e_lwp_exit =		linux_e_lwp_exit,
	.e_syscall_intern =	linux32_syscall_intern,
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	netbsd32_vm_default_addr,
	.e_usertrap =		NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};
