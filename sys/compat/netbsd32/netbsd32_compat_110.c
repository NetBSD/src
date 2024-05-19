/*	$NetBSD: netbsd32_compat_110.c,v 1.1 2024/05/19 22:25:48 christos Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_110.c,v 1.1 2024/05/19 22:25:48 christos Exp $");

#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/module.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/systm.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

int
compat_110_netbsd32_dup3(struct lwp *l,
    const struct compat_110_netbsd32_dup3_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) from;
		syscallarg(int) to;
		syscallarg(int) flags;
		syscallarg(const netbsd32_kevent100p_t) changelist;
		syscallarg(netbsd32_size_t) nchanges;
		syscallarg(netbsd32_kevent100p_t) eventlist;
		syscallarg(netbsd32_size_t) nevents;
		syscallarg(netbsd32_timespecp_t) timeout;
	} */
	return dodup(l, SCARG(uap, from), SCARG(uap, to), SCARG(uap, flags),
	    retval);
}

static struct syscall_package compat_netbsd32_110_syscalls[] = {
	{ NETBSD32_SYS_compat_110_netbsd32_dup3, 0,
	    (sy_call_t *)compat_110_netbsd32_dup3 },
	{ 0, 0, NULL },
};

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_110, "compat_netbsd32,compat_110");

static int
compat_netbsd32_110_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return syscall_establish(&emul_netbsd32,
		    compat_netbsd32_110_syscalls);

	case MODULE_CMD_FINI:
		return syscall_disestablish(&emul_netbsd32,
		    compat_netbsd32_110_syscalls);

	default:
		return ENOTTY;
	}
}
