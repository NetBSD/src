/*	$NetBSD: kern_lwp.c,v 1.49.2.1 2007/04/01 16:16:20 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: kern_lwp.c,v 1.49.2.1 2007/04/01 16:16:20 bouyer Exp $");

#include "opt_multiprocessor.h"

#define _LWP_API_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

POOL_INIT(lwp_pool, sizeof(struct lwp), 0, 0, 0, "lwppl",
    &pool_allocator_nointr);
POOL_INIT(lwp_uc_pool, sizeof(ucontext_t), 0, 0, 0, "lwpucpl",
    &pool_allocator_nointr);

static specificdata_domain_t lwp_specificdata_domain;

struct lwplist alllwp;

#define LWP_DEBUG

#ifdef LWP_DEBUG
int lwp_debug = 0;
#define DPRINTF(x) if (lwp_debug) printf x
#else
#define DPRINTF(x)
#endif

void
lwpinit(void)
{

	lwp_specificdata_domain = specificdata_domain_create();
	KASSERT(lwp_specificdata_domain != NULL);
}

/* ARGSUSED */
int
sys__lwp_create(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_create_args /* {
		syscallarg(const ucontext_t *) ucp;
		syscallarg(u_long) flags;
		syscallarg(lwpid_t *) new_lwp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct lwp *l2;
	vaddr_t uaddr;
	boolean_t inmem;
	ucontext_t *newuc;
	int s, error;

	if (p->p_flag & P_SA)
		return EINVAL;

	newuc = pool_get(&lwp_uc_pool, PR_WAITOK);

	error = copyin(SCARG(uap, ucp), newuc,
	    l->l_proc->p_emul->e_sa->sae_ucsize);
	if (error) {
		pool_put(&lwp_uc_pool, newuc);
		return (error);
	}

	/* XXX check against resource limits */

	inmem = uvm_uarea_alloc(&uaddr);
	if (__predict_false(uaddr == 0)) {
		pool_put(&lwp_uc_pool, newuc);
		return (ENOMEM);
	}

	/* XXX flags:
	 * __LWP_ASLWP is probably needed for Solaris compat.
	 */

	newlwp(l, p, uaddr, inmem,
	    SCARG(uap, flags) & LWP_DETACHED,
	    NULL, 0, startlwp, newuc, &l2);

	if ((SCARG(uap, flags) & LWP_SUSPENDED) == 0) {
		SCHED_LOCK(s);
		l2->l_stat = LSRUN;
		setrunqueue(l2);
		p->p_nrlwps++;
		SCHED_UNLOCK(s);
	} else {
		l2->l_stat = LSSUSPENDED;
	}

	error = copyout(&l2->l_lid, SCARG(uap, new_lwp),
	    sizeof(l2->l_lid));
	if (error) {
		/* XXX We should destroy the LWP. */
		return (error);
	}

	return (0);
}


int
sys__lwp_exit(struct lwp *l, void *v, register_t *retval)
{

	lwp_exit(l);
	/* NOTREACHED */
	return (0);
}


int
sys__lwp_self(struct lwp *l, void *v, register_t *retval)
{

	*retval = l->l_lid;

	return (0);
}


int
sys__lwp_getprivate(struct lwp *l, void *v, register_t *retval)
{

	*retval = (uintptr_t) l->l_private;

	return (0);
}


int
sys__lwp_setprivate(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_setprivate_args /* {
		syscallarg(void *) ptr;
	} */ *uap = v;

	l->l_private = SCARG(uap, ptr);

	return (0);
}


int
sys__lwp_suspend(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_suspend_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	int target_lid;
	struct proc *p = l->l_proc;
	struct lwp *t;
	struct lwp *t2;

	if (p->p_flag & P_SA)
		return EINVAL;

	target_lid = SCARG(uap, target);

	LIST_FOREACH(t, &p->p_lwps, l_sibling)
		if (t->l_lid == target_lid)
			break;

	if (t == NULL)
		return (ESRCH);

	if (t == l) {
		/*
		 * Check for deadlock, which is only possible
		 * when we're suspending ourself.
		 */
		LIST_FOREACH(t2, &p->p_lwps, l_sibling) {
			if ((t2 != l) && (t2->l_stat != LSSUSPENDED))
				break;
		}

		if (t2 == NULL) /* All other LWPs are suspended */
			return (EDEADLK);
	}

	return lwp_suspend(l, t);
}

inline int
lwp_suspend(struct lwp *l, struct lwp *t)
{
	struct proc *p = t->l_proc;
	int s;

	if (t == l) {
		SCHED_LOCK(s);
		KASSERT(l->l_stat == LSONPROC);
		l->l_stat = LSSUSPENDED;
		p->p_nrlwps--;
		/* XXX NJWLWP check if this makes sense here: */
		p->p_stats->p_ru.ru_nvcsw++;
		mi_switch(l, NULL);
		SCHED_ASSERT_UNLOCKED();
		splx(s);
	} else {
		switch (t->l_stat) {
		case LSSUSPENDED:
			return (0); /* _lwp_suspend() is idempotent */
		case LSRUN:
			SCHED_LOCK(s);
			remrunqueue(t);
			t->l_stat = LSSUSPENDED;
			p->p_nrlwps--;
			SCHED_UNLOCK(s);
			break;
		case LSSLEEP:
			t->l_stat = LSSUSPENDED;
			break;
		case LSIDL:
		case LSZOMB:
			return (EINTR); /* It's what Solaris does..... */
		case LSSTOP:
			panic("_lwp_suspend: Stopped LWP in running process!");
			break;
		case LSONPROC:
			/* XXX multiprocessor LWPs? Implement me! */
			return (EINVAL);
		}
	}

	return (0);
}


int
sys__lwp_continue(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_continue_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	int s, target_lid;
	struct proc *p = l->l_proc;
	struct lwp *t;

	if (p->p_flag & P_SA)
		return EINVAL;

	target_lid = SCARG(uap, target);

	LIST_FOREACH(t, &p->p_lwps, l_sibling)
		if (t->l_lid == target_lid)
			break;

	if (t == NULL)
		return (ESRCH);

	SCHED_LOCK(s);
	lwp_continue(t);
	SCHED_UNLOCK(s);

	return (0);
}

void
lwp_continue(struct lwp *l)
{

	DPRINTF(("lwp_continue of %d.%d (%s), state %d, wchan %p\n",
	    l->l_proc->p_pid, l->l_lid, l->l_proc->p_comm, l->l_stat,
	    l->l_wchan));

	if (l->l_stat != LSSUSPENDED)
		return;

	if (l->l_wchan == 0) {
		/* LWP was runnable before being suspended. */
		setrunnable(l);
	} else {
		/* LWP was sleeping before being suspended. */
		l->l_stat = LSSLEEP;
	}
}

int
sys__lwp_wakeup(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_wakeup_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	lwpid_t target_lid;
	struct lwp *t;
	struct proc *p;
	int error;
	int s;

	p = l->l_proc;
	target_lid = SCARG(uap, target);

	SCHED_LOCK(s);

	LIST_FOREACH(t, &p->p_lwps, l_sibling)
		if (t->l_lid == target_lid)
			break;

	if (t == NULL) {
		error = ESRCH;
		goto exit;
	}

	if (t->l_stat != LSSLEEP) {
		error = ENODEV;
		goto exit;
	}

	if ((t->l_flag & L_SINTR) == 0) {
		error = EBUSY;
		goto exit;
	}
	/*
	 * Tell ltsleep to wakeup.
	 */
	t->l_flag |= L_CANCELLED;

	setrunnable(t);
	error = 0;
exit:
	SCHED_UNLOCK(s);

	return error;
}

int
sys__lwp_wait(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_wait_args /* {
		syscallarg(lwpid_t) wait_for;
		syscallarg(lwpid_t *) departed;
	} */ *uap = v;
	int error;
	lwpid_t dep;

	error = lwp_wait1(l, SCARG(uap, wait_for), &dep, 0);
	if (error)
		return (error);

	if (SCARG(uap, departed)) {
		error = copyout(&dep, SCARG(uap, departed),
		    sizeof(dep));
		if (error)
			return (error);
	}

	return (0);
}


int
lwp_wait1(struct lwp *l, lwpid_t lid, lwpid_t *departed, int flags)
{
	struct proc *p = l->l_proc;
	struct lwp *l2, *l3;
	int nfound, error, wpri;
	static const char waitstr1[] = "lwpwait";
	static const char waitstr2[] = "lwpwait2";

	DPRINTF(("lwp_wait1: %d.%d waiting for %d.\n",
	    p->p_pid, l->l_lid, lid));

	if (lid == l->l_lid)
		return (EDEADLK); /* Waiting for ourselves makes no sense. */

	wpri = PWAIT |
	    ((flags & LWPWAIT_EXITCONTROL) ? PNOEXITERR : PCATCH);
 loop:
	nfound = 0;
	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
		if ((l2 == l) || (l2->l_flag & L_DETACHED) ||
		    ((lid != 0) && (lid != l2->l_lid)))
			continue;

		nfound++;
		if (l2->l_stat == LSZOMB) {
			if (departed)
				*departed = l2->l_lid;

			simple_lock(&p->p_lock);
			LIST_REMOVE(l2, l_sibling);
			p->p_nlwps--;
			p->p_nzlwps--;
			simple_unlock(&p->p_lock);
			/* XXX decrement limits */

			pool_put(&lwp_pool, l2);

			return (0);
		} else if (l2->l_stat == LSSLEEP ||
		           l2->l_stat == LSSUSPENDED) {
			/* Deadlock checks.
			 * 1. If all other LWPs are waiting for exits
			 *    or suspended, we would deadlock.
			 */

			LIST_FOREACH(l3, &p->p_lwps, l_sibling) {
				if (l3 != l && (l3->l_stat != LSSUSPENDED) &&
				    !(l3->l_stat == LSSLEEP &&
					l3->l_wchan == (caddr_t) &p->p_nlwps))
					break;
			}
			if (l3 == NULL) /* Everyone else is waiting. */
				return (EDEADLK);

			/* XXX we'd like to check for a cycle of waiting
			 * LWPs (specific LID waits, not any-LWP waits)
			 * and detect that sort of deadlock, but we don't
			 * have a good place to store the lwp that is
			 * being waited for. wchan is already filled with
			 * &p->p_nlwps, and putting the lwp address in
			 * there for deadlock tracing would require
			 * exiting LWPs to call wakeup on both their
			 * own address and &p->p_nlwps, to get threads
			 * sleeping on any LWP exiting.
			 *
			 * Revisit later. Maybe another auxillary
			 * storage location associated with sleeping
			 * is in order.
			 */
		}
	}

	if (nfound == 0)
		return (ESRCH);

	if ((error = tsleep((caddr_t) &p->p_nlwps, wpri,
	    (lid != 0) ? waitstr1 : waitstr2, 0)) != 0)
		return (error);

	goto loop;
}


int
newlwp(struct lwp *l1, struct proc *p2, vaddr_t uaddr, boolean_t inmem,
    int flags, void *stack, size_t stacksize,
    void (*func)(void *), void *arg, struct lwp **rnewlwpp)
{
	struct lwp *l2;
	int s;

	l2 = pool_get(&lwp_pool, PR_WAITOK);

	l2->l_stat = LSIDL;
	l2->l_forw = l2->l_back = NULL;
	l2->l_proc = p2;

	lwp_initspecific(l2);

	memset(&l2->l_startzero, 0,
	       (unsigned) ((caddr_t)&l2->l_endzero -
			   (caddr_t)&l2->l_startzero));
	memcpy(&l2->l_startcopy, &l1->l_startcopy,
	       (unsigned) ((caddr_t)&l2->l_endcopy -
			   (caddr_t)&l2->l_startcopy));

#if !defined(MULTIPROCESSOR)
	/*
	 * In the single-processor case, all processes will always run
	 * on the same CPU.  So, initialize the child's CPU to the parent's
	 * now.  In the multiprocessor case, the child's CPU will be
	 * initialized in the low-level context switch code when the
	 * process runs.
	 */
	KASSERT(l1->l_cpu != NULL);
	l2->l_cpu = l1->l_cpu;
#else
	/*
	 * zero child's CPU pointer so we don't get trash.
	 */
	l2->l_cpu = NULL;
#endif /* ! MULTIPROCESSOR */

	l2->l_flag = inmem ? L_INMEM : 0;
	l2->l_flag |= (flags & LWP_DETACHED) ? L_DETACHED : 0;

	lwp_update_creds(l2);
	callout_init(&l2->l_tsleep_ch);

	if (rnewlwpp != NULL)
		*rnewlwpp = l2;

	l2->l_addr = UAREA_TO_USER(uaddr);
	uvm_lwp_fork(l1, l2, stack, stacksize, func,
	    (arg != NULL) ? arg : l2);

	simple_lock(&p2->p_lock);
	l2->l_lid = ++p2->p_nlwpid;
	LIST_INSERT_HEAD(&p2->p_lwps, l2, l_sibling);
	p2->p_nlwps++;
	simple_unlock(&p2->p_lock);

	/* XXX should be locked differently... */
	s = proclist_lock_write();
	LIST_INSERT_HEAD(&alllwp, l2, l_list);
	proclist_unlock_write(s);

	if (p2->p_emul->e_lwp_fork)
		(*p2->p_emul->e_lwp_fork)(l1, l2);

	return (0);
}


/*
 * Quit the process. This will call cpu_exit, which will call cpu_switch,
 * so this can only be used meaningfully if you're willing to switch away.
 * Calling with l!=curlwp would be weird.
 */
void
lwp_exit(struct lwp *l)
{
	struct proc *p = l->l_proc;
	int s;

	DPRINTF(("lwp_exit: %d.%d exiting.\n", p->p_pid, l->l_lid));
	DPRINTF((" nlwps: %d nrlwps %d nzlwps: %d\n",
	    p->p_nlwps, p->p_nrlwps, p->p_nzlwps));

	if (p->p_emul->e_lwp_exit)
		(*p->p_emul->e_lwp_exit)(l);

	/*
	 * If we are the last live LWP in a process, we need to exit
	 * the entire process (if that's not already going on). We do
	 * so with an exit status of zero, because it's a "controlled"
	 * exit, and because that's what Solaris does.
	 *
	 * Note: the last LWP's specificdata will be deleted here.
	 */
	if (((p->p_nlwps - p->p_nzlwps) == 1) && ((p->p_flag & P_WEXIT) == 0)) {
		DPRINTF(("lwp_exit: %d.%d calling exit1()\n",
		    p->p_pid, l->l_lid));
		exit1(l, 0);
		/* NOTREACHED */
	}

	/* Delete the specificdata while it's still safe to sleep. */
	specificdata_fini(lwp_specificdata_domain, &l->l_specdataref);

	s = proclist_lock_write();
	LIST_REMOVE(l, l_list);
	proclist_unlock_write(s);

	/*
	 * Release our cached credentials, and collate accounting flags.
	 */
	kauth_cred_free(l->l_cred);
	simple_lock(&p->p_lock);
	p->p_acflag |= l->l_acflag;
	simple_unlock(&p->p_lock);

	/* Free MD LWP resources */
#ifndef __NO_CPU_LWP_FREE
	cpu_lwp_free(l, 0);
#endif

	pmap_deactivate(l);

	if (l->l_flag & L_DETACHED) {
		simple_lock(&p->p_lock);
		LIST_REMOVE(l, l_sibling);
		p->p_nlwps--;
		simple_unlock(&p->p_lock);

		curlwp = NULL;
		l->l_proc = NULL;
	}

	SCHED_LOCK(s);
	p->p_nrlwps--;
	l->l_stat = LSDEAD;
	SCHED_UNLOCK(s);

	/* This LWP no longer needs to hold the kernel lock. */
	KERNEL_PROC_UNLOCK(l);

	/* cpu_exit() will not return */
	cpu_exit(l);
}

/*
 * We are called from cpu_exit() once it is safe to schedule the
 * dead process's resources to be freed (i.e., once we've switched to
 * the idle PCB for the current CPU).
 *
 * NOTE: One must be careful with locking in this routine.  It's
 * called from a critical section in machine-dependent code, so
 * we should refrain from changing any interrupt state.
 */
void
lwp_exit2(struct lwp *l)
{
	struct proc *p;

	KERNEL_LOCK(LK_EXCLUSIVE);
	/*
	 * Free the VM resources we're still holding on to.
	 */
	uvm_lwp_exit(l);

	if (l->l_flag & L_DETACHED) {
		/* Nobody waits for detached LWPs. */
		pool_put(&lwp_pool, l);
		KERNEL_UNLOCK();
	} else {
		l->l_stat = LSZOMB;
		p = l->l_proc;
		p->p_nzlwps++;
		wakeup(&p->p_nlwps);
		KERNEL_UNLOCK();
	}
}

/*
 * Pick a LWP to represent the process for those operations which
 * want information about a "process" that is actually associated
 * with a LWP.
 */
struct lwp *
proc_representative_lwp(struct proc *p)
{
	struct lwp *l, *onproc, *running, *sleeping, *stopped, *suspended;
	struct lwp *signalled;

	/* Trivial case: only one LWP */
	if (p->p_nlwps == 1)
		return (LIST_FIRST(&p->p_lwps));

	switch (p->p_stat) {
	case SSTOP:
	case SACTIVE:
		/* Pick the most live LWP */
		onproc = running = sleeping = stopped = suspended = NULL;
		signalled = NULL;
		LIST_FOREACH(l, &p->p_lwps, l_sibling) {
			if (l->l_lid == p->p_sigctx.ps_lwp)
				signalled = l;
			switch (l->l_stat) {
			case LSONPROC:
				onproc = l;
				break;
			case LSRUN:
				running = l;
				break;
			case LSSLEEP:
				sleeping = l;
				break;
			case LSSTOP:
				stopped = l;
				break;
			case LSSUSPENDED:
				suspended = l;
				break;
			}
		}
		if (signalled)
			return signalled;
		if (onproc)
			return onproc;
		if (running)
			return running;
		if (sleeping)
			return sleeping;
		if (stopped)
			return stopped;
		if (suspended)
			return suspended;
		break;
	case SZOMB:
		/* Doesn't really matter... */
		return (LIST_FIRST(&p->p_lwps));
#ifdef DIAGNOSTIC
	case SDYING:
	case SIDL:
		/* We have more than one LWP and we're in SIDL?
		 * How'd that happen?
		 */
		panic("Too many LWPs (%d) in SIDL process %d (%s)",
		    p->p_nrlwps, p->p_pid, p->p_comm);
	default:
		panic("Process %d (%s) in unknown state %d",
		    p->p_pid, p->p_comm, p->p_stat);
#endif
	}

	panic("proc_representative_lwp: couldn't find a lwp for process"
		" %d (%s)", p->p_pid, p->p_comm);
	/* NOTREACHED */
	return NULL;
}

/*
 * Update an LWP's cached credentials to mirror the process' master copy.
 *
 * This happens early in the syscall path, on user trap, and on LWP
 * creation.  A long-running LWP can also voluntarily choose to update
 * it's credentials by calling this routine.  This may be called from
 * LWP_CACHE_CREDS(), which checks l->l_cred != p->p_cred beforehand.
 */
void
lwp_update_creds(struct lwp *l)
{
	kauth_cred_t oc;
	struct proc *p;

	p = l->l_proc;
	oc = l->l_cred;

	simple_lock(&p->p_lock);
	kauth_cred_hold(p->p_cred);
	l->l_cred = p->p_cred;
	simple_unlock(&p->p_lock);
	if (oc != NULL)
		kauth_cred_free(oc);
}

/*
 * lwp_specific_key_create --
 *	Create a key for subsystem lwp-specific data.
 */
int
lwp_specific_key_create(specificdata_key_t *keyp, specificdata_dtor_t dtor)
{

	return (specificdata_key_create(lwp_specificdata_domain, keyp, dtor));
}

/*
 * lwp_specific_key_delete --
 *	Delete a key for subsystem lwp-specific data.
 */
void
lwp_specific_key_delete(specificdata_key_t key)
{

	specificdata_key_delete(lwp_specificdata_domain, key);
}

/*
 * lwp_initspecific --
 *	Initialize an LWP's specificdata container.
 */
void
lwp_initspecific(struct lwp *l)
{
	int error;

	error = specificdata_init(lwp_specificdata_domain, &l->l_specdataref);
	KASSERT(error == 0);
}

/*
 * lwp_finispecific --
 *	Finalize an LWP's specificdata container.
 */
void
lwp_finispecific(struct lwp *l)
{

	specificdata_fini(lwp_specificdata_domain, &l->l_specdataref);
}

/*
 * lwp_getspecific --
 *	Return lwp-specific data corresponding to the specified key.
 *
 *	Note: LWP specific data is NOT INTERLOCKED.  An LWP should access
 *	only its OWN SPECIFIC DATA.  If it is necessary to access another
 *	LWP's specifc data, care must be taken to ensure that doing so
 *	would not cause internal data structure inconsistency (i.e. caller
 *	can guarantee that the target LWP is not inside an lwp_getspecific()
 *	or lwp_setspecific() call).
 */
void *
lwp_getspecific(specificdata_key_t key)
{

	return (specificdata_getspecific_unlocked(lwp_specificdata_domain,
						  &curlwp->l_specdataref, key));
}

void *
_lwp_getspecific_by_lwp(struct lwp *l, specificdata_key_t key)
{

	return (specificdata_getspecific_unlocked(lwp_specificdata_domain,
						  &l->l_specdataref, key));
}

/*
 * lwp_setspecific --
 *	Set lwp-specific data corresponding to the specified key.
 */
void
lwp_setspecific(specificdata_key_t key, void *data)
{

	specificdata_setspecific(lwp_specificdata_domain,
				 &curlwp->l_specdataref, key, data);
}
