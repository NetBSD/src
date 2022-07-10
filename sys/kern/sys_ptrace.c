/*	$NetBSD: sys_ptrace.c,v 1.12 2022/07/10 14:07:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)sys_process.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_ptrace.c,v 1.12 2022/07/10 14:07:55 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_ptrace.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/pax.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/ras.h>
#include <sys/kmem.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/syscall.h>
#include <sys/module.h>

#include <uvm/uvm_extern.h>

#include <machine/reg.h>

/*
 * PTRACE methods
 */

static int
ptrace_copyin_piod(struct ptrace_io_desc *piod, const void *addr, size_t len)
{
	if (len != 0 && sizeof(*piod) != len)
		return EINVAL;

	return copyin(addr, piod, sizeof(*piod));
}

static int
ptrace_copyout_piod(const struct ptrace_io_desc *piod, void *addr, size_t len)
{
	if (len != 0 && sizeof(*piod) != len)
		return EINVAL;

	return copyout(piod, addr, sizeof(*piod));
}

static int
ptrace_copyin_siginfo(struct ptrace_siginfo *psi, const void *addr, size_t len)
{
	if (sizeof(*psi) != len)
		return EINVAL;

	return copyin(addr, psi, sizeof(*psi));
}

static int
ptrace_copyout_siginfo(const struct ptrace_siginfo *psi, void *addr, size_t len)
{
	if (sizeof(*psi) != len)
		return EINVAL;

	return copyout(psi, addr, sizeof(*psi));
}

static int
ptrace_copyout_lwpstatus(const struct ptrace_lwpstatus *pls, void *addr,
    size_t len)
{

	return copyout(pls, addr, len);
}

static struct ptrace_methods native_ptm = {
	.ptm_copyin_piod = ptrace_copyin_piod,
	.ptm_copyout_piod = ptrace_copyout_piod,
	.ptm_copyin_siginfo = ptrace_copyin_siginfo,
	.ptm_copyout_siginfo = ptrace_copyout_siginfo,
	.ptm_copyout_lwpstatus = ptrace_copyout_lwpstatus,
	.ptm_doregs = process_doregs,
	.ptm_dofpregs = process_dofpregs,
	.ptm_dodbregs = process_dodbregs,
};

static const struct syscall_package ptrace_syscalls[] = {
	{ SYS_ptrace, 0, (sy_call_t *)sys_ptrace },
	{ 0, 0, NULL },
};

/*
 * Process debugging system call.
 */
int
sys_ptrace(struct lwp *l, const struct sys_ptrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(void *) addr;
		syscallarg(int) data;
	} */

        return do_ptrace(&native_ptm, l, SCARG(uap, req), SCARG(uap, pid),
            SCARG(uap, addr), SCARG(uap, data), retval);
}

#define	DEPS	"ptrace_common"

MODULE(MODULE_CLASS_EXEC, ptrace, DEPS);

static int
ptrace_init(void)
{
	int error;

	error = syscall_establish(&emul_netbsd, ptrace_syscalls);
	return error;
}

static int
ptrace_fini(void)
{
	int error;

	error = syscall_disestablish(&emul_netbsd, ptrace_syscalls);
	return error;
}


static int
ptrace_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = ptrace_init();
		break;
	case MODULE_CMD_FINI:
		error = ptrace_fini();
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
