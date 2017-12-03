/*	$NetBSD: netbsd32_ptrace.c,v 1.4.12.2 2017/12/03 11:36:56 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ptrace.c,v 1.4.12.2 2017/12/03 11:36:56 jdolecek Exp $");

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

static int netbsd32_copyinpiod(struct ptrace_io_desc *, const void *);
static void netbsd32_copyoutpiod(const struct ptrace_io_desc *, void *);
static int netbsd32_doregs(struct lwp *, struct lwp *, struct uio *);
static int netbsd32_dofpregs(struct lwp *, struct lwp *, struct uio *);
static int netbsd32_dodbregs(struct lwp *, struct lwp *, struct uio *);


static int
netbsd32_copyinpiod(struct ptrace_io_desc *piod, const void *addr)
{
	struct netbsd32_ptrace_io_desc piod32;

	int error = copyin(addr, &piod32, sizeof(piod32));
	if (error)
		return error;
	piod->piod_op = piod32.piod_op;
	piod->piod_offs = NETBSD32PTR64(piod32.piod_offs);
	piod->piod_addr = NETBSD32PTR64(piod32.piod_addr);
	piod->piod_len = (size_t)piod32.piod_len;

	return 0;
}

static void
netbsd32_copyoutpiod(const struct ptrace_io_desc *piod, void *addr)
{
	struct netbsd32_ptrace_io_desc piod32;

	piod32.piod_op = piod->piod_op;
	NETBSD32PTR32(piod32.piod_offs, piod->piod_offs);
	NETBSD32PTR32(piod32.piod_addr, piod->piod_addr);
	piod32.piod_len = (netbsd32_size_t)piod->piod_len;
	(void) copyout(&piod32, addr, sizeof(piod32));
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
	.ptm_copyinpiod = netbsd32_copyinpiod,
	.ptm_copyoutpiod = netbsd32_copyoutpiod,
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
