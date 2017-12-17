/*	$NetBSD: netbsd32_ptrace.c,v 1.5 2017/12/17 20:59:27 christos Exp $	*/

/*
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ptrace.c,v 1.5 2017/12/17 20:59:27 christos Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ptrace.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/ptrace.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

extern struct emul emul_netbsd32;


/*
 * PTRACE methods
 */

static int
netbsd32_copyin_piod(struct ptrace_io_desc *piod, const void *addr, size_t len)
{
	struct netbsd32_ptrace_io_desc piod32;

	if (len != 0 && sizeof(piod32) != len)
		return EINVAL;

	int error = copyin(addr, &piod32, sizeof(piod32));
	if (error)
		return error;
	piod->piod_op = piod32.piod_op;
	piod->piod_offs = NETBSD32PTR64(piod32.piod_offs);
	piod->piod_addr = NETBSD32PTR64(piod32.piod_addr);
	piod->piod_len = (size_t)piod32.piod_len;

	return 0;
}

static int
netbsd32_copyout_piod(const struct ptrace_io_desc *piod, void *addr, size_t len)
{
	struct netbsd32_ptrace_io_desc piod32;

	if (len != 0 && sizeof(piod32) != len)
		return EINVAL;

	piod32.piod_op = piod->piod_op;
	NETBSD32PTR32(piod32.piod_offs, piod->piod_offs);
	NETBSD32PTR32(piod32.piod_addr, piod->piod_addr);
	piod32.piod_len = (netbsd32_size_t)piod->piod_len;
	return copyout(&piod32, addr, sizeof(piod32));
}

static int
netbsd32_copyin_siginfo(struct ptrace_siginfo *psi, const void *addr, size_t len)
{
	struct netbsd32_ptrace_siginfo psi32;

	if (sizeof(psi32) != len)
		return EINVAL;

	int error = copyin(addr, &psi32, sizeof(psi32));
	if (error)
		return error;
	psi->psi_lwpid = psi32.psi_lwpid;
	netbsd32_si32_to_si(&psi->psi_siginfo, &psi32.psi_siginfo);
	return 0;
}

static int
netbsd32_copyout_siginfo(const struct ptrace_siginfo *psi, void *addr, size_t len)
{
	struct netbsd32_ptrace_siginfo psi32;

	if (sizeof(psi32) != len)
		return EINVAL;

	psi32.psi_lwpid = psi->psi_lwpid;
	netbsd32_si_to_si32(&psi32.psi_siginfo, &psi->psi_siginfo);
	return copyout(&psi32, addr, sizeof(psi32));
}

static int
netbsd32_doregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETREGS) || defined(PT_SETREGS)
	process_reg32 r32;
	int error;
	char *kv;
	int kl;

	if (uio->uio_offset < 0 || uio->uio_offset > (off_t)sizeof(r32))
		return EINVAL;

	kl = sizeof(r32);
	kv = (char *)&r32;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if ((size_t)kl > uio->uio_resid)
		kl = uio->uio_resid;
	error = process_read_regs32(l, &r32);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (l->l_stat != LSSTOP)
			error = EBUSY;
		else
			error = process_write_regs32(l, &r32);
	}

	uio->uio_offset = 0;
	return error;
#else
	return EINVAL;
#endif
}

static int
netbsd32_dofpregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETFPREGS) || defined(PT_SETFPREGS)
	process_fpreg32 r32;
	int error;
	char *kv;
	size_t kl;

	KASSERT(l->l_proc->p_flag & PK_32);
	if (uio->uio_offset < 0 || uio->uio_offset > (off_t)sizeof(r32))
		return EINVAL;
	kl = sizeof(r32);
	kv = (char *)&r32;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	error = process_read_fpregs32(l, &r32, &kl);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (l->l_stat != LSSTOP)
			error = EBUSY;
		else
			error = process_write_fpregs32(l, &r32, kl);
	}
	uio->uio_offset = 0;
	return error;
#else
	return EINVAL;
#endif
}

static int
netbsd32_dodbregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETDBREGS) || defined(PT_SETDBREGS)
	process_dbreg32 r32;
	int error;
	char *kv;
	size_t kl;

	KASSERT(l->l_proc->p_flag & PK_32);
	if (uio->uio_offset < 0 || uio->uio_offset > (off_t)sizeof(r32))
		return EINVAL;
	kl = sizeof(r32);
	kv = (char *)&r32;

	kv += uio->uio_offset;
	kl -= uio->uio_offset;
	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	error = process_read_dbregs32(l, &r32, &kl);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (l->l_stat != LSSTOP)
			error = EBUSY;
		else
			error = process_write_dbregs32(l, &r32, kl);
	}
	uio->uio_offset = 0;
	return error;
#else
	return EINVAL;
#endif
}

static struct ptrace_methods netbsd32_ptm = {
	.ptm_copyin_piod = netbsd32_copyin_piod,
	.ptm_copyout_piod = netbsd32_copyout_piod,
	.ptm_copyin_siginfo = netbsd32_copyin_siginfo,
	.ptm_copyout_siginfo = netbsd32_copyout_siginfo,
	.ptm_doregs = netbsd32_doregs,
	.ptm_dofpregs = netbsd32_dofpregs,
	.ptm_dodbregs = netbsd32_dodbregs
};


int
netbsd32_ptrace(struct lwp *l, const struct netbsd32_ptrace_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(netbsd32_voidp *) addr;
		syscallarg(int) data;
	} */

	return do_ptrace(&netbsd32_ptm, l, SCARG(uap, req), SCARG(uap, pid),
	    SCARG_P32(uap, addr), SCARG(uap, data), retval);
}

static const struct syscall_package compat_ptrace_syscalls[] = {
	{ NETBSD32_SYS_netbsd32_ptrace, 0, (sy_call_t *)netbsd32_ptrace },
	{ 0, 0, NULL },
};

#define	DEPS	"compat_netbsd32,ptrace_common"

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_ptrace, DEPS);

static int
compat_netbsd32_ptrace_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(&emul_netbsd32,
		    compat_ptrace_syscalls);
		break;
	case MODULE_CMD_FINI:
		error = syscall_disestablish(&emul_netbsd32,
		    compat_ptrace_syscalls);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
