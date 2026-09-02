/*	$NetBSD: netbsd32_ptrace.c,v 1.11 2026/09/02 16:39:41 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ptrace.c,v 1.11 2026/09/02 16:39:41 riastradh Exp $");

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

#ifndef PTRACE_TRANSLATE_REQUEST32
#define PTRACE_TRANSLATE_REQUEST32(x) x
#endif

static void
netbsd32_lwpstatus_to_lwpstatus32(struct netbsd32_ptrace_lwpstatus *pls32,
    const struct ptrace_lwpstatus *pls)
{
	memset(pls32, 0, sizeof(*pls32));
	pls32->pl_lwpid = pls->pl_lwpid;
	pls32->pl_sigpend = pls->pl_sigpend;
	pls32->pl_sigmask = pls->pl_sigmask;
	memcpy(&pls32->pl_name, &pls->pl_name, PL_LNAMELEN);
	NETBSD32PTR32(pls32->pl_private, pls->pl_private);
}

void
netbsd32_read_lwpstatus(struct lwp *l, struct netbsd32_ptrace_lwpstatus *pls32)
{
	struct ptrace_lwpstatus pls;

	process_read_lwpstatus(l, &pls);

	netbsd32_lwpstatus_to_lwpstatus32(pls32, &pls);
}

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

	memset(&piod32, 0, sizeof(piod32));
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

	memset(&psi32, 0, sizeof(psi32));
	psi32.psi_lwpid = psi->psi_lwpid;
	netbsd32_si_to_si32(&psi32.psi_siginfo, &psi->psi_siginfo);
	return copyout(&psi32, addr, sizeof(psi32));
}

static int
netbsd32_copyout_lwpstatus(const struct ptrace_lwpstatus *pls, void *addr, size_t len)
{
	struct netbsd32_ptrace_lwpstatus pls32;

	if (len > sizeof(pls32))
		return EINVAL;

	netbsd32_lwpstatus_to_lwpstatus32(&pls32, pls);

	return copyout(&pls32, addr, MIN(len, sizeof(pls32)));
}

/*
 * Sync with proc_regio in sys_process_lwpstatus.c.
 */
static int
netbsd32_proc_regio(struct lwp *l, struct uio *uio, void *buf, size_t buflen,
    ptrace_regrfunc_t r, ptrace_regwfunc_t w)
{
	int error;
	char *kv;
	size_t kl;
	size_t ks = buflen;

	if (uio->uio_offset < 0 || uio->uio_offset > (off_t)ks)
		return EINVAL;

	kv = (char *)buf + uio->uio_offset;
	kl = ks - uio->uio_offset;

	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	/*
	 * Read all the registers first, even if the caller is writing
	 * to them, because the caller might be writing to only a part
	 * of them.  But if the process is not stopped, refuse.  Must
	 * hold the lwp lock to verify l->l_stat remains LSSTOP as we
	 * read.
	 */
	lwp_lock(l);
	if (l->l_stat != LSSTOP)
		error = EBUSY;
	else
		error = (*r)(l, buf, &ks);
	lwp_unlock(l);
	if (error)
		goto out;

	/*
	 * Copy (part of) the register content to or from the user's
	 * I/O space.
	 */
	error = uiomove(kv, kl, uio);
	if (error)
		goto out;

	/*
	 * If we're writing registers (or part of the registers), write
	 * them back.  Again, if the process is not stopped, refuse.
	 * Must hold the lwp lock to verify l->l_stat remains LSSTOP as
	 * we write.
	 *
	 * Note that there is a potential race condition or ABA problem
	 * here: in the time between the lwp_unlock above and the
	 * lwp_lock below, l->l_stat could transition from LSSTOP to
	 * something else back to LSSTOP again, and the register
	 * content could be scrambled.  We should really fail with
	 * EBUSY if that happens at all, but there's no easy way to
	 * detect that case, and we certainly can't hold the lwp locked
	 * across uiomove(9) which might wait indefinitely for swap
	 * I/O.
	 */
	if (uio->uio_rw == UIO_WRITE) {
		lwp_lock(l);
		if (l->l_stat != LSSTOP)
			error = EBUSY;
		else
			error = (*w)(l, buf, ks);
		lwp_unlock(l);
		if (error)
			goto out;
	}

out:	uio->uio_offset = 0;
	return error;
}

#if defined(PT_GETREGS) || defined(PT_SETREGS)
static int
process_read_regs32_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	process_reg32 *reg32 = buf;

	KASSERT(*sizep == sizeof(*reg32));
	return process_read_regs32(l, reg32);
}

static int
process_write_regs32_wrapper(struct lwp *l, void *buf, size_t size)
{
	process_reg32 *reg32 = buf;

	KASSERT(size == sizeof(*reg32));
	return process_write_regs32(l, reg32);
}
#endif	/* defined(PT_GETREGS) || defined(PT_SETREGS) */

static int
netbsd32_doregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETREGS) || defined(PT_SETREGS)
	process_reg32 r32;

	return netbsd32_proc_regio(l, uio, &r32, sizeof(r32),
	    process_read_regs32_wrapper,
	    process_write_regs32_wrapper);
#else
	return EINVAL;
#endif
}

#if defined(PT_GETFPREGS) || defined(PT_SETFPREGS)
static int
process_read_fpregs32_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	process_fpreg32 *fpreg32 = buf;

	KASSERT(*sizep == sizeof(*fpreg32));
	return process_read_fpregs32(l, fpreg32, sizep);
}

static int
process_write_fpregs32_wrapper(struct lwp *l, void *buf, size_t size)
{
	process_fpreg32 *fpreg32 = buf;

	KASSERT(size == sizeof(*fpreg32));
	return process_write_fpregs32(l, fpreg32, size);
}
#endif	/* defined(PT_GETFPREGS) || defined(PT_SETFPREGS) */

static int
netbsd32_dofpregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETFPREGS) || defined(PT_SETFPREGS)
	process_fpreg32 r32;

	return netbsd32_proc_regio(l, uio, &r32, sizeof(r32),
	    process_read_fpregs32_wrapper,
	    process_write_fpregs32_wrapper);
#else
	return EINVAL;
#endif
}

#if defined(PT_GETDBREGS) || defined(PT_SETDBREGS)
static int
process_read_dbregs32_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	process_dbreg32 *dbreg32 = buf;

	KASSERT(*sizep == sizeof(*dbreg32));
	return process_read_dbregs32(l, dbreg32, sizep);
}

static int
process_write_dbregs32_wrapper(struct lwp *l, void *buf, size_t size)
{
	process_dbreg32 *dbreg32 = buf;

	KASSERT(size == sizeof(*dbreg32));
	return process_write_dbregs32(l, dbreg32, size);
}
#endif	/* defined(PT_GETDBREGS) || defined(PT_SETDBREGS) */

static int
netbsd32_dodbregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETDBREGS) || defined(PT_SETDBREGS)
	process_dbreg32 r32;

	KASSERT(rw_lock_held(&l->l_proc->p_reflock));
	process_alloc_dbregs(l);

	return netbsd32_proc_regio(l, uio, &r32, sizeof(r32),
	    process_read_dbregs32_wrapper,
	    process_write_dbregs32_wrapper);
#else
	return EINVAL;
#endif
}

static struct ptrace_methods netbsd32_ptm = {
	.ptm_copyin_piod = netbsd32_copyin_piod,
	.ptm_copyout_piod = netbsd32_copyout_piod,
	.ptm_copyin_siginfo = netbsd32_copyin_siginfo,
	.ptm_copyout_siginfo = netbsd32_copyout_siginfo,
	.ptm_copyout_lwpstatus = netbsd32_copyout_lwpstatus,
	.ptm_doregs = netbsd32_doregs,
	.ptm_dofpregs = netbsd32_dofpregs,
	.ptm_dodbregs = netbsd32_dodbregs
};


int
netbsd32_ptrace(struct lwp *l, const struct netbsd32_ptrace_args *uap,
    register_t *retval)
{
	int req;

	/* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(netbsd32_voidp *) addr;
		syscallarg(int) data;
	} */

	req = PTRACE_TRANSLATE_REQUEST32(SCARG(uap, req));
	if (req == -1)
		return EOPNOTSUPP;

	return do_ptrace(&netbsd32_ptm, l, req, SCARG(uap, pid),
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
