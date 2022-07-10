/*	$NetBSD: sys_process_lwpstatus.c,v 1.4 2022/07/10 17:47:58 riastradh Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: sys_process_lwpstatus.c,v 1.4 2022/07/10 17:47:58 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_ptrace.h"
#include "opt_ktrace.h"
#include "opt_pax.h"
#include "opt_compat_netbsd32.h"
#endif

#if defined(__HAVE_COMPAT_NETBSD32) && !defined(COMPAT_NETBSD32) \
    && !defined(_RUMPKERNEL)
#define COMPAT_NETBSD32
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/ptrace.h>

#ifndef PTRACE_REGS_ALIGN
#define PTRACE_REGS_ALIGN /* nothing */
#endif

void
ptrace_read_lwpstatus(struct lwp *l, struct ptrace_lwpstatus *pls)
{

	pls->pl_lwpid = l->l_lid;
	memcpy(&pls->pl_sigmask, &l->l_sigmask, sizeof(pls->pl_sigmask));
	memcpy(&pls->pl_sigpend, &l->l_sigpend.sp_set, sizeof(pls->pl_sigpend));

	if (l->l_name == NULL)
		memset(&pls->pl_name, 0, PL_LNAMELEN);
	else {
		KASSERT(strlen(l->l_name) < PL_LNAMELEN);
		strncpy(pls->pl_name, l->l_name, PL_LNAMELEN);
	}

#ifdef PTRACE_LWP_GETPRIVATE
	pls->pl_private = (void *)(intptr_t)PTRACE_LWP_GETPRIVATE(l);
#else
	pls->pl_private = l->l_private;
#endif
}

void
process_read_lwpstatus(struct lwp *l, struct ptrace_lwpstatus *pls)
{

	ptrace_read_lwpstatus(l, pls);
}

int
ptrace_update_lwp(struct proc *t, struct lwp **lt, lwpid_t lid)
{
	if (lid == 0 || lid == (*lt)->l_lid || t->p_nlwps == 1)
		return 0;

	mutex_enter(t->p_lock);
	lwp_delref2(*lt);

	*lt = lwp_find(t, lid);
	if (*lt == NULL) {
		mutex_exit(t->p_lock);
		return ESRCH;
	}

	if ((*lt)->l_flag & LW_SYSTEM) {
		mutex_exit(t->p_lock);
		*lt = NULL;
		return EINVAL;
	}

	lwp_addref(*lt);
	mutex_exit(t->p_lock);

	return 0;
}

int
process_validfpregs(struct lwp *l)
{

#if defined(PT_SETFPREGS) || defined(PT_GETFPREGS)
	return (l->l_flag & LW_SYSTEM) == 0;
#else
	return 0;
#endif
}

int
process_validregs(struct lwp *l)
{

#if defined(PT_SETREGS) || defined(PT_GETREGS)
	return (l->l_flag & LW_SYSTEM) == 0;
#else
	return 0;
#endif
}

int
process_validdbregs(struct lwp *l)
{

#if defined(PT_SETDBREGS) || defined(PT_GETDBREGS)
	return (l->l_flag & LW_SYSTEM) == 0;
#else
	return 0;
#endif
}

#ifdef PT_REGISTERS
static int
proc_regio(struct lwp *l, struct uio *uio, size_t ks, ptrace_regrfunc_t r,
    ptrace_regwfunc_t w)
{
	char buf[1024] PTRACE_REGS_ALIGN;
	int error;
	char *kv;
	size_t kl;

	if (ks > sizeof(buf))
		return E2BIG;

	if (uio->uio_offset < 0 || uio->uio_offset > (off_t)ks)
		return EINVAL;

	kv = buf + uio->uio_offset;
	kl = ks - uio->uio_offset;

	if (kl > uio->uio_resid)
		kl = uio->uio_resid;

	error = (*r)(l, buf, &ks);
	if (error == 0)
		error = uiomove(kv, kl, uio);
	if (error == 0 && uio->uio_rw == UIO_WRITE) {
		if (l->l_stat != LSSTOP)
			error = EBUSY;
		else
			error = (*w)(l, buf, ks);
	}

	uio->uio_offset = 0;
	return error;
}
#endif

int
process_doregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETREGS) || defined(PT_SETREGS)
	size_t s;
	ptrace_regrfunc_t r;
	ptrace_regwfunc_t w;

#ifdef COMPAT_NETBSD32
	const bool pk32 = (curl->l_proc->p_flag & PK_32) != 0;

	if (__predict_false(pk32)) {
		if ((l->l_proc->p_flag & PK_32) == 0) {
			// 32 bit tracer can't trace 64 bit process
			return EINVAL;
		}
		s = sizeof(process_reg32);
		r = __FPTRCAST(ptrace_regrfunc_t, process_read_regs32);
		w = __FPTRCAST(ptrace_regwfunc_t, process_write_regs32);
	} else
#endif
	{
		s = sizeof(struct reg);
		r = __FPTRCAST(ptrace_regrfunc_t, process_read_regs);
		w = __FPTRCAST(ptrace_regwfunc_t, process_write_regs);
	}
	return proc_regio(l, uio, s, r, w);
#else
	return EINVAL;
#endif
}

int
process_dofpregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETFPREGS) || defined(PT_SETFPREGS)
	size_t s;
	ptrace_regrfunc_t r;
	ptrace_regwfunc_t w;

#ifdef COMPAT_NETBSD32
	const bool pk32 = (curl->l_proc->p_flag & PK_32) != 0;

	if (__predict_false(pk32)) {
		if ((l->l_proc->p_flag & PK_32) == 0) {
			// 32 bit tracer can't trace 64 bit process
			return EINVAL;
		}
		s = sizeof(process_fpreg32);
		r = (ptrace_regrfunc_t)process_read_fpregs32;
		w = (ptrace_regwfunc_t)process_write_fpregs32;
	} else
#endif
	{
		s = sizeof(struct fpreg);
		r = (ptrace_regrfunc_t)process_read_fpregs;
		w = (ptrace_regwfunc_t)process_write_fpregs;
	}
	return proc_regio(l, uio, s, r, w);
#else
	return EINVAL;
#endif
}


int
process_dodbregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_GETDBREGS) || defined(PT_SETDBREGS)
	size_t s;
	ptrace_regrfunc_t r;
	ptrace_regwfunc_t w;

#ifdef COMPAT_NETBSD32
	const bool pk32 = (curl->l_proc->p_flag & PK_32) != 0;

	if (__predict_false(pk32)) {
		if ((l->l_proc->p_flag & PK_32) == 0) {
			// 32 bit tracer can't trace 64 bit process
			return EINVAL;
		}
		s = sizeof(process_dbreg32);
		r = (ptrace_regrfunc_t)process_read_dbregs32;
		w = (ptrace_regwfunc_t)process_write_dbregs32;
	} else
#endif
	{
		s = sizeof(struct dbreg);
		r = (ptrace_regrfunc_t)process_read_dbregs;
		w = (ptrace_regwfunc_t)process_write_dbregs;
	}
	return proc_regio(l, uio, s, r, w);
#else
	return EINVAL;
#endif
}
