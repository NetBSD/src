/*      $NetBSD: lwproc.c,v 1.20 2013/03/07 18:49:13 pooka Exp $	*/

/*
 * Copyright (c) 2010, 2011 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lwproc.c,v 1.20 2013/03/07 18:49:13 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/resourcevar.h>
#include <sys/uidinfo.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

struct emul *emul_default = &emul_netbsd;

static void
lwproc_proc_free(struct proc *p)
{
	kauth_cred_t cred;

	mutex_enter(proc_lock);

	KASSERT(p->p_nlwps == 0);
	KASSERT(LIST_EMPTY(&p->p_lwps));
	KASSERT(p->p_stat == SACTIVE || p->p_stat == SDYING ||
	    p->p_stat == SDEAD);

	LIST_REMOVE(p, p_list);
	LIST_REMOVE(p, p_sibling);
	proc_free_pid(p->p_pid); /* decrements nprocs */
	proc_leavepgrp(p); /* releases proc_lock */

	cred = p->p_cred;
	chgproccnt(kauth_cred_getuid(cred), -1);
	if (rump_proc_vfs_release)
		rump_proc_vfs_release(p);

	lim_free(p->p_limit);
	pstatsfree(p->p_stats);
	kauth_cred_free(p->p_cred);
	proc_finispecific(p);

	mutex_obj_free(p->p_lock);
	mutex_destroy(&p->p_stmutex);
	mutex_destroy(&p->p_auxlock);
	rw_destroy(&p->p_reflock);
	cv_destroy(&p->p_waitcv);
	cv_destroy(&p->p_lwpcv);

	/* non-kernel vmspaces are not shared */
	if (!RUMP_LOCALPROC_P(p)) {
		KASSERT(p->p_vmspace->vm_refcnt == 1);
		kmem_free(p->p_vmspace, sizeof(*p->p_vmspace));
	}

	proc_free_mem(p);
}

/*
 * Allocate a new process.  Mostly mimic fork by
 * copying the properties of the parent.  However, there are some
 * differences.
 *
 * Switch to the new lwp and return a pointer to it.
 */
static struct proc *
lwproc_newproc(struct proc *parent, int flags)
{
	uid_t uid = kauth_cred_getuid(parent->p_cred);
	struct proc *p;

	/* maxproc not enforced */
	atomic_inc_uint(&nprocs);

	/* allocate process */
	p = proc_alloc();
	memset(&p->p_startzero, 0,
	    offsetof(struct proc, p_endzero)
	      - offsetof(struct proc, p_startzero));
	memcpy(&p->p_startcopy, &parent->p_startcopy,
	    offsetof(struct proc, p_endcopy)
	      - offsetof(struct proc, p_startcopy));

	/* some other garbage we need to zero */
	p->p_sigacts = NULL;
	p->p_aio = NULL;
	p->p_dtrace = NULL;
	p->p_mqueue_cnt = p->p_exitsig = 0;
	p->p_flag = p->p_sflag = p->p_slflag = p->p_lflag = p->p_stflag = 0;
	p->p_trace_enabled = 0;
	p->p_xstat = p->p_acflag = 0;
	p->p_stackbase = 0;

	p->p_stats = pstatscopy(parent->p_stats);

	p->p_vmspace = vmspace_kernel();
	p->p_emul = emul_default;
	if (*parent->p_comm)
		strcpy(p->p_comm, parent->p_comm);
	else
		strcpy(p->p_comm, "rumproc");

	if ((flags & RUMP_RFCFDG) == 0)
		KASSERT(parent == curproc);
	if (flags & RUMP_RFFDG)
		p->p_fd = fd_copy();
	else if (flags & RUMP_RFCFDG)
		p->p_fd = fd_init(NULL);
	else
		fd_share(p);

	lim_addref(parent->p_limit);
	p->p_limit = parent->p_limit;

	LIST_INIT(&p->p_lwps);
	LIST_INIT(&p->p_children);

	p->p_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&p->p_stmutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&p->p_auxlock, MUTEX_DEFAULT, IPL_NONE);
	rw_init(&p->p_reflock);
	cv_init(&p->p_waitcv, "pwait");
	cv_init(&p->p_lwpcv, "plwp");

	p->p_pptr = parent;
	p->p_ppid = parent->p_pid;
	p->p_stat = SACTIVE;

	kauth_proc_fork(parent, p);

	/* initialize cwd in rump kernels with vfs */
	if (rump_proc_vfs_init)
		rump_proc_vfs_init(p);

	chgproccnt(uid, 1); /* not enforced */

	/* publish proc various proc lists */
	mutex_enter(proc_lock);
	LIST_INSERT_HEAD(&allproc, p, p_list);
	LIST_INSERT_HEAD(&parent->p_children, p, p_sibling);
	LIST_INSERT_AFTER(parent, p, p_pglist);
	mutex_exit(proc_lock);

	return p;
}

static void
lwproc_freelwp(struct lwp *l)
{
	struct proc *p;

	p = l->l_proc;
	mutex_enter(p->p_lock);

	/* XXX: l_refcnt */
	KASSERT(l->l_flag & LW_WEXIT);
	KASSERT(l->l_refcnt == 0);

	/* ok, zero references, continue with nuke */
	LIST_REMOVE(l, l_sibling);
	KASSERT(p->p_nlwps >= 1);
	if (--p->p_nlwps == 0) {
		KASSERT(p != &proc0);
		p->p_stat = SDEAD;
	}
	cv_broadcast(&p->p_lwpcv); /* nobody sleeps on this in rump? */
	kauth_cred_free(l->l_cred);
	mutex_exit(p->p_lock);

	mutex_enter(proc_lock);
	LIST_REMOVE(l, l_list);
	mutex_exit(proc_lock);

	if (l->l_name)
		kmem_free(l->l_name, MAXCOMLEN);
	lwp_finispecific(l);

	kmem_free(l, sizeof(*l));

	if (p->p_stat == SDEAD)
		lwproc_proc_free(p);	
}

extern kmutex_t unruntime_lock;

/*
 * called with p_lock held, releases lock before return
 */
static void
lwproc_makelwp(struct proc *p, struct lwp *l, bool doswitch, bool procmake)
{

	p->p_nlwps++;
	l->l_refcnt = 1;
	l->l_proc = p;

	l->l_lid = p->p_nlwpid++;
	LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);

	l->l_fd = p->p_fd;
	l->l_cpu = rump_cpu;
	l->l_target_cpu = rump_cpu; /* Initial target CPU always the same */
	l->l_stat = LSRUN;
	l->l_mutex = &unruntime_lock;
	TAILQ_INIT(&l->l_ld_locks);
	mutex_exit(p->p_lock);

	lwp_update_creds(l);
	lwp_initspecific(l);

	if (doswitch) {
		rump_lwproc_switch(l);
	}

	/* filedesc already has refcount 1 when process is created */
	if (!procmake) {
		fd_hold(l);
	}

	mutex_enter(proc_lock);
	LIST_INSERT_HEAD(&alllwp, l, l_list);
	mutex_exit(proc_lock);
}

struct lwp *
rump__lwproc_alloclwp(struct proc *p)
{
	struct lwp *l;
	bool newproc = false;

	if (p == NULL) {
		p = lwproc_newproc(&proc0, 0);
		newproc = true;
	}

	l = kmem_zalloc(sizeof(*l), KM_SLEEP);

	mutex_enter(p->p_lock);
	KASSERT((p->p_sflag & PS_RUMP_LWPEXIT) == 0);
	lwproc_makelwp(p, l, false, newproc);

	return l;
}

int
rump_lwproc_newlwp(pid_t pid)
{
	struct proc *p;
	struct lwp *l;

	l = kmem_zalloc(sizeof(*l), KM_SLEEP);
	mutex_enter(proc_lock);
	p = proc_find_raw(pid);
	if (p == NULL) {
		mutex_exit(proc_lock);
		kmem_free(l, sizeof(*l));
		return ESRCH;
	}
	mutex_enter(p->p_lock);
	if (p->p_sflag & PS_RUMP_LWPEXIT) {
		mutex_exit(proc_lock);
		mutex_exit(p->p_lock);
		kmem_free(l, sizeof(*l));
		return EBUSY;
	}
	mutex_exit(proc_lock);
	lwproc_makelwp(p, l, true, false);

	return 0;
}

int
rump_lwproc_rfork(int flags)
{
	struct proc *p;
	struct lwp *l;

	if (flags & ~(RUMP_RFFDG|RUMP_RFCFDG) ||
	    (~flags & (RUMP_RFFDG|RUMP_RFCFDG)) == 0)
		return EINVAL;

	p = lwproc_newproc(curproc, flags);
	l = kmem_zalloc(sizeof(*l), KM_SLEEP);
	mutex_enter(p->p_lock);
	KASSERT((p->p_sflag & PS_RUMP_LWPEXIT) == 0);
	lwproc_makelwp(p, l, true, true);

	return 0;
}

/*
 * Switch to a new process/thread.  Release previous one if
 * deemed to be exiting.  This is considered a slow path for
 * rump kernel entry.
 */
void
rump_lwproc_switch(struct lwp *newlwp)
{
	struct lwp *l = curlwp;

	KASSERT(!(l->l_flag & LW_WEXIT) || newlwp);

	if (__predict_false(newlwp && (newlwp->l_pflag & LP_RUNNING)))
		panic("lwp %p (%d:%d) already running",
		    newlwp, newlwp->l_proc->p_pid, newlwp->l_lid);

	if (newlwp == NULL) {
		l->l_pflag &= ~LP_RUNNING;
		l->l_flag |= LW_RUMP_CLEAR;
		return;
	}

	/* fd_free() must be called from curlwp context.  talk about ugh */
	if (l->l_flag & LW_WEXIT) {
		fd_free();
	}

	rumpuser_set_curlwp(NULL);

	newlwp->l_cpu = newlwp->l_target_cpu = l->l_cpu;
	newlwp->l_mutex = l->l_mutex;
	newlwp->l_pflag |= LP_RUNNING;

	rumpuser_set_curlwp(newlwp);

	/*
	 * Check if the thread should get a signal.  This is
	 * mostly to satisfy the "record" rump sigmodel.
	 */
	mutex_enter(newlwp->l_proc->p_lock);
	if (sigispending(newlwp, 0)) {
		newlwp->l_flag |= LW_PENDSIG;
	}
	mutex_exit(newlwp->l_proc->p_lock);

	l->l_mutex = &unruntime_lock;
	l->l_pflag &= ~LP_RUNNING;
	l->l_flag &= ~LW_PENDSIG;
	l->l_stat = LSRUN;

	if (l->l_flag & LW_WEXIT) {
		lwproc_freelwp(l);
	}
}

void
rump_lwproc_releaselwp(void)
{
	struct proc *p;
	struct lwp *l = curlwp;

	if (l->l_refcnt == 0 && l->l_flag & LW_WEXIT)
		panic("releasing non-pertinent lwp");

	p = l->l_proc;
	mutex_enter(p->p_lock);
	KASSERT(l->l_refcnt != 0);
	l->l_refcnt--;
	mutex_exit(p->p_lock);
	l->l_flag |= LW_WEXIT; /* will be released when unscheduled */
}

struct lwp *
rump_lwproc_curlwp(void)
{
	struct lwp *l = curlwp;

	if (l->l_flag & LW_WEXIT)
		return NULL;
	return l;
}

/* this interface is under construction (like the proverbial 90's web page) */
int rump_i_know_what_i_am_doing_with_sysents = 0;
void
rump_lwproc_sysent_usenative()
{

	if (!rump_i_know_what_i_am_doing_with_sysents)
		panic("don't use rump_lwproc_sysent_usenative()");
	curproc->p_emul = &emul_netbsd;
}
