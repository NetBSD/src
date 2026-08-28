/*	$NetBSD: sys_process_lwpstatus.c,v 1.9 2026/08/28 16:15:15 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sys_process_lwpstatus.c,v 1.9 2026/08/28 16:15:15 riastradh Exp $");

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

#if defined(PT_FPREGS)
	return (l->l_flag & LW_SYSTEM) == 0;
#else
	return 0;
#endif
}

int
process_validregs(struct lwp *l)
{

#if defined(PT_REGS)
	return (l->l_flag & LW_SYSTEM) == 0;
#else
	return 0;
#endif
}

int
process_validdbregs(struct lwp *l)
{

#if defined(PT_DBREGS)
	return (l->l_flag & LW_SYSTEM) == 0;
#else
	return 0;
#endif
}

#ifdef PT_REGISTERS
/*
 * Sync with proc_regio in netbsd32_ptrace.c.
 */
static int
proc_regio(struct lwp *l, struct uio *uio, void *buf, size_t buflen,
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
#endif

#if defined(PT_REGS)
#ifdef COMPAT_NETBSD32
static int
process_read_regs32_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	process_reg32 *r32 = buf;

	KASSERT(*sizep == sizeof(*r32));
	return process_read_regs32(l, buf);
}

static int
process_write_regs32_wrapper(struct lwp *l, void *buf, size_t size)
{
	process_reg32 *r32 = buf;

	KASSERT(size == sizeof(*r32));
	return process_write_regs32(l, buf);
}
#endif	/* COMPAT_NETBSD32 */

static int
process_read_regs_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	struct reg *r = buf;

	KASSERT(*sizep == sizeof(*r));
	return process_read_regs(l, buf);
}

static int
process_write_regs_wrapper(struct lwp *l, void *buf, size_t size)
{
	struct reg *r = buf;

	KASSERT(size == sizeof(*r));
	return process_write_regs(l, buf);
}
#endif	/* PT_REGS */

int
process_doregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_REGS)
#ifdef COMPAT_NETBSD32
	const bool pk32 = (curl->l_proc->p_flag & PK_32) != 0;

	if (__predict_false(pk32)) {
		struct reg32 reg32 PTRACE_REGS_ALIGN;

		if ((l->l_proc->p_flag & PK_32) == 0) {
			// 32 bit tracer can't trace 64 bit process
			return EINVAL;
		}
		return proc_regio(l, uio, &reg32, sizeof(reg32),
		    process_read_regs32_wrapper,
		    process_write_regs32_wrapper);
	} else
#endif
	{
		struct reg reg PTRACE_REGS_ALIGN;

		return proc_regio(l, uio, &reg, sizeof(reg),
		    process_read_regs_wrapper,
		    process_write_regs_wrapper);
	}
#else
	return EINVAL;
#endif
}

#if defined(PT_FPREGS)
#ifdef COMPAT_NETBSD32
static int
process_read_fpregs32_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	process_fpreg32 *r32 = buf;

	KASSERT(*sizep == sizeof(*r32));
	return process_read_fpregs32(l, buf, sizep);
}

static int
process_write_fpregs32_wrapper(struct lwp *l, void *buf, size_t size)
{
	process_fpreg32 *r32 = buf;

	KASSERT(size == sizeof(*r32));
	return process_write_fpregs32(l, buf, size);
}
#endif	/* COMPAT_NETBSD32 */

static int
process_read_fpregs_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	struct fpreg *r = buf;

	KASSERT(*sizep == sizeof(*r));
	return process_read_fpregs(l, buf, sizep);
}

static int
process_write_fpregs_wrapper(struct lwp *l, void *buf, size_t size)
{
	struct fpreg *r = buf;

	KASSERT(size == sizeof(*r));
	return process_write_fpregs(l, buf, size);
}
#endif	/* PT_FPREGS */

int
process_dofpregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_FPREGS)
#ifdef COMPAT_NETBSD32
	const bool pk32 = (curl->l_proc->p_flag & PK_32) != 0;

	if (__predict_false(pk32)) {
		struct fpreg32 fpreg32 PTRACE_REGS_ALIGN;

		if ((l->l_proc->p_flag & PK_32) == 0) {
			// 32 bit tracer can't trace 64 bit process
			return EINVAL;
		}
		return proc_regio(l, uio, &fpreg32, sizeof(fpreg32),
		    process_read_fpregs32_wrapper,
		    process_write_fpregs32_wrapper);
	} else
#endif
	{
		struct fpreg fpreg PTRACE_REGS_ALIGN;

		return proc_regio(l, uio, &fpreg, sizeof(fpreg),
		    process_read_fpregs_wrapper,
		    process_write_fpregs_wrapper);
	}
#else
	return EINVAL;
#endif
}

#if defined(PT_DBREGS)
#ifdef COMPAT_NETBSD32
static int
process_read_dbregs32_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	process_dbreg32 *r32 = buf;

	KASSERT(*sizep == sizeof(*r32));
	return process_read_dbregs32(l, buf, sizep);
}

static int
process_write_dbregs32_wrapper(struct lwp *l, void *buf, size_t size)
{
	process_dbreg32 *r32 = buf;

	KASSERT(size == sizeof(*r32));
	return process_write_dbregs32(l, buf, size);
}
#endif	/* COMPAT_NETBSD32 */

static int
process_read_dbregs_wrapper(struct lwp *l, void *buf, size_t *sizep)
{
	struct dbreg *r = buf;

	KASSERT(*sizep == sizeof(*r));
	return process_read_dbregs(l, buf, sizep);
}

static int
process_write_dbregs_wrapper(struct lwp *l, void *buf, size_t size)
{
	struct dbreg *r = buf;

	KASSERT(size == sizeof(*r));
	return process_write_dbregs(l, buf, size);
}
#endif	/* PT_DBREGS */

int
process_dodbregs(struct lwp *curl /*tracer*/,
    struct lwp *l /*traced*/,
    struct uio *uio)
{
#if defined(PT_DBREGS)
	KASSERT(rw_lock_held(&l->l_proc->p_reflock));
	process_alloc_dbregs(l);

#ifdef COMPAT_NETBSD32
	const bool pk32 = (curl->l_proc->p_flag & PK_32) != 0;

	if (__predict_false(pk32)) {
		struct dbreg32 dbreg32 PTRACE_REGS_ALIGN;

		if ((l->l_proc->p_flag & PK_32) == 0) {
			// 32 bit tracer can't trace 64 bit process
			return EINVAL;
		}
		return proc_regio(l, uio, &dbreg32, sizeof(dbreg32),
		    process_read_dbregs32_wrapper,
		    process_write_dbregs32_wrapper);
	} else
#endif
	{
		struct dbreg dbreg PTRACE_REGS_ALIGN;

		return proc_regio(l, uio, &dbreg, sizeof(dbreg),
		    process_read_dbregs_wrapper,
		    process_write_dbregs_wrapper);
	}
#else
	return EINVAL;
#endif
}
