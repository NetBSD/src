/*	$NetBSD: vfs_cwd.c,v 1.4.62.1 2020/01/25 15:54:03 ad Exp $	*/

/*-
 * Copyright (c) 2008, 2020 The NetBSD Foundation, Inc.
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

/*
 * Current working directory.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_cwd.c,v 1.4.62.1 2020/01/25 15:54:03 ad Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/xcall.h>

static int	cwdi_ctor(void *, void *, int);
static void	cwdi_dtor(void *, void *);

static pool_cache_t cwdi_cache;

void
cwd_sys_init(void)
{

	cwdi_cache = pool_cache_init(sizeof(struct cwdinfo), coherency_unit,
	    0, 0, "cwdi", NULL, IPL_NONE, cwdi_ctor, cwdi_dtor, NULL);
	KASSERT(cwdi_cache != NULL);
}

/*
 * Create an initial cwdinfo structure, using the same current and root
 * directories as curproc.
 */
struct cwdinfo *
cwdinit(void)
{
	struct cwdinfo *cwdi;
	struct cwdinfo *copy;

	cwdi = pool_cache_get(cwdi_cache, PR_WAITOK);

	copy = cwdenter(RW_READER);
	cwdi->cwdi_cdir = copy->cwdi_cdir;
	if (cwdi->cwdi_cdir)
		vref(cwdi->cwdi_cdir);
	cwdi->cwdi_rdir = copy->cwdi_rdir;
	if (cwdi->cwdi_rdir)
		vref(cwdi->cwdi_rdir);
	cwdi->cwdi_edir = copy->cwdi_edir;
	if (cwdi->cwdi_edir)
		vref(cwdi->cwdi_edir);
	cwdi->cwdi_cmask = copy->cwdi_cmask;
	cwdi->cwdi_refcnt = 1;
	cwdexit(copy);

	return (cwdi);
}

static int
cwdi_ctor(void *arg, void *obj, int flags)
{
	struct cwdinfo *cwdi = obj;

	mutex_init(&cwdi->cwdi_lock, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

static void
cwdi_dtor(void *arg, void *obj)
{
	struct cwdinfo *cwdi = obj;

	mutex_destroy(&cwdi->cwdi_lock);
}

/*
 * Make p2 share p1's cwdinfo.
 */
void
cwdshare(struct proc *p2)
{
	struct cwdinfo *cwdi;

	cwdi = curproc->p_cwdi;

	atomic_inc_uint(&cwdi->cwdi_refcnt);
	p2->p_cwdi = cwdi;
}

/*
 * Make sure proc has only one reference to its cwdi, creating
 * a new one if necessary.
 */
void
cwdunshare(struct proc *p)
{
	struct cwdinfo *cwdi = p->p_cwdi;

	if (cwdi->cwdi_refcnt > 1) {
		cwdi = cwdinit();
		cwdfree(p->p_cwdi);
		p->p_cwdi = cwdi;
	}
}

/*
 * Release a cwdinfo structure.
 */
void
cwdfree(struct cwdinfo *cwdi)
{

	if (atomic_dec_uint_nv(&cwdi->cwdi_refcnt) > 0)
		return;

	vrele(cwdi->cwdi_cdir);
	if (cwdi->cwdi_rdir)
		vrele(cwdi->cwdi_rdir);
	if (cwdi->cwdi_edir)
		vrele(cwdi->cwdi_edir);
	pool_cache_put(cwdi_cache, cwdi);
}

void
cwdexec(struct proc *p)
{

	cwdunshare(p);

	if (p->p_cwdi->cwdi_edir) {
		vrele(p->p_cwdi->cwdi_edir);
	}
}

/*
 * Used when curlwp wants to use or update its cwdinfo, and needs to prevent
 * concurrent changes.
 *
 * "op" is either RW_READER or RW_WRITER indicating the kind of lock
 * required.  If a read lock on the cwdinfo is requested, then curlwp must
 * not block while holding the lock, or the cwdinfo could become stale. 
 * It's okay to block while holding a write lock.
 */
struct cwdinfo *
cwdenter(krw_t op)
{
	struct cwdinfo *cwdi = curproc->p_cwdi;

	if (__predict_true(op == RW_READER)) {
		/*
		 * Disable preemption to hold off the writer side's xcall,
		 * then observe the lock.  If it's already taken, we need to
		 * join in the melee.  Otherwise we're good to go; keeping
		 * the xcall at bay with kpreempt_disable() will prevent any
		 * changes while the caller is pondering the cwdinfo.
		 */
		kpreempt_disable();
		if (__predict_true(mutex_owner(&cwdi->cwdi_lock) == NULL))
			return cwdi;
		kpreempt_enable();
		mutex_enter(&cwdi->cwdi_lock);
	} else {
		/*
		 * About to make changes.  If there's more than one
		 * reference on the cwdinfo, or curproc has more than one
		 * LWP, then LWPs other than curlwp can also see the
		 * cwdinfo.  Run a cross call to get all LWPs out of the
		 * read section.  This also acts as a global memory barrier,
		 * meaning we don't need to do anything special with on
		 * the reader side.
		 */
		mutex_enter(&cwdi->cwdi_lock);
		if (cwdi->cwdi_refcnt + curproc->p_nlwps > 2)
			xc_barrier(0);
	}
	return cwdi;
}

/*
 * Release a lock previously taken with cwdenter().
 */
void
cwdexit(struct cwdinfo *cwdi)
{
	struct lwp *l = curlwp;

	KASSERT(cwdi == l->l_proc->p_cwdi);

	if (__predict_true(mutex_owner(&cwdi->cwdi_lock) != l))
		kpreempt_enable();
	else
		mutex_exit(&cwdi->cwdi_lock);
}

/*
 * Called when there is a need to inspect some other process' cwdinfo.  Used
 * by procfs and sysctl.  This gets you a read lock; the cwdinfo must NOT be
 * changed.
 */
const struct cwdinfo *
cwdlock(struct proc *p)
{
	struct cwdinfo *cwdi = p->p_cwdi;

	mutex_enter(&cwdi->cwdi_lock);
	return cwdi;
}

/*
 * Release a lock acquired with cwdlock().
 */
void
cwdunlock(struct proc *p)
{
	struct cwdinfo *cwdi = p->p_cwdi;

	mutex_exit(&cwdi->cwdi_lock);
}

/*
 * Get a reference to the current working directory and return it.
 */
struct vnode *
cwdcdir(void)
{
	struct cwdinfo *cwdi;
	struct vnode *vp;

	cwdi = cwdenter(RW_READER);
	if ((vp = cwdi->cwdi_cdir) != NULL)
		vref(vp);
	cwdexit(cwdi);
	return vp;
}

/*
 * Get a reference to the root directory and return it.
 */
struct vnode *
cwdrdir(void)
{
	struct cwdinfo *cwdi;
	struct vnode *vp;

	cwdi = cwdenter(RW_READER);
	if ((vp = cwdi->cwdi_rdir) != NULL)
		vref(vp);
	cwdexit(cwdi);
	return vp;
}
