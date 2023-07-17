/*	$NetBSD: sys_lwp.c,v 1.84 2023/07/17 12:54:29 riastradh Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007, 2008, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, and Andrew Doran.
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
 * Lightweight process (LWP) system calls.  See kern_lwp.c for a description
 * of LWPs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_lwp.c,v 1.84 2023/07/17 12:54:29 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/ptrace.h>
#include <sys/sleepq.h>
#include <sys/lwpctl.h>
#include <sys/cpu.h>
#include <sys/pserialize.h>

#include <uvm/uvm_extern.h>

#define	LWP_UNPARK_MAX		1024

static const stack_t lwp_ss_init = SS_INIT;

syncobj_t lwp_park_syncobj = {
	.sobj_name	= "lwp_park",
	.sobj_flag	= SOBJ_SLEEPQ_NULL,
	.sobj_unsleep	= sleepq_unsleep,
	.sobj_changepri	= sleepq_changepri,
	.sobj_lendpri	= sleepq_lendpri,
	.sobj_owner	= syncobj_noowner,
};

static void
mi_startlwp(void *arg)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;

	(p->p_emul->e_startlwp)(arg);

	/* If the process is traced, report lwp creation to a debugger */
	if ((p->p_slflag & (PSL_TRACED|PSL_TRACELWP_CREATE)) ==
	    (PSL_TRACED|PSL_TRACELWP_CREATE)) {
		/* Paranoid check */
		mutex_enter(&proc_lock);
		if ((p->p_slflag & (PSL_TRACED|PSL_TRACELWP_CREATE)) !=
		    (PSL_TRACED|PSL_TRACELWP_CREATE)) { 
			mutex_exit(&proc_lock);
			return;
		}

		mutex_enter(p->p_lock);
		eventswitch(TRAP_LWP, PTRACE_LWP_CREATE, l->l_lid);
	}
}

int
do_lwp_create(lwp_t *l, void *arg, u_long flags, lwp_t **l2,
    const sigset_t *sigmask, const stack_t *sigstk)
{
	struct proc *p = l->l_proc;
	vaddr_t uaddr;
	int error;

	/* XXX check against resource limits */

	uaddr = uvm_uarea_alloc();
	if (__predict_false(uaddr == 0))
		return ENOMEM;

	error = lwp_create(l, p, uaddr, flags & LWP_DETACHED, NULL, 0,
	    mi_startlwp, arg, l2, l->l_class, sigmask, &lwp_ss_init);
	if (__predict_false(error)) {
		uvm_uarea_free(uaddr);
		return error;
	}

	return 0;
}

int
sys__lwp_create(struct lwp *l, const struct sys__lwp_create_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const ucontext_t *) ucp;
		syscallarg(u_long) flags;
		syscallarg(lwpid_t *) new_lwp;
	} */
	struct proc *p = l->l_proc;
	ucontext_t *newuc;
	lwp_t *l2;
	int error;

	newuc = kmem_alloc(sizeof(ucontext_t), KM_SLEEP);
	error = copyin(SCARG(uap, ucp), newuc, p->p_emul->e_ucsize);
	if (error)
		goto fail;

	/* validate the ucontext */
	if ((newuc->uc_flags & _UC_CPU) == 0) {
		error = EINVAL;
		goto fail;
	}
	error = cpu_mcontext_validate(l, &newuc->uc_mcontext);
	if (error)
		goto fail;

	const sigset_t *sigmask = newuc->uc_flags & _UC_SIGMASK ?
	    &newuc->uc_sigmask : &l->l_sigmask;
	error = do_lwp_create(l, newuc, SCARG(uap, flags), &l2, sigmask,
	    &SS_INIT);
	if (error)
		goto fail;

	error = copyout(&l2->l_lid, SCARG(uap, new_lwp), sizeof(l2->l_lid));
	if (error == 0) {
		lwp_start(l2, SCARG(uap, flags));
		return 0;
	}
	lwp_exit(l2);
 fail:
	kmem_free(newuc, sizeof(ucontext_t));
	return error;
}

int
sys__lwp_exit(struct lwp *l, const void *v, register_t *retval)
{

	lwp_exit(l);
	return 0;
}

int
sys__lwp_self(struct lwp *l, const void *v, register_t *retval)
{

	*retval = l->l_lid;
	return 0;
}

int
sys__lwp_getprivate(struct lwp *l, const void *v, register_t *retval)
{

	*retval = (uintptr_t)l->l_private;
	return 0;
}

int
sys__lwp_setprivate(struct lwp *l, const struct sys__lwp_setprivate_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(void *) ptr;
	} */

	return lwp_setprivate(l, SCARG(uap, ptr));
}

int
sys__lwp_suspend(struct lwp *l, const struct sys__lwp_suspend_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	struct proc *p = l->l_proc;
	struct lwp *t;
	int error;

	mutex_enter(p->p_lock);
	if ((t = lwp_find(p, SCARG(uap, target))) == NULL) {
		mutex_exit(p->p_lock);
		return ESRCH;
	}

	/*
	 * Check for deadlock, which is only possible when we're suspending
	 * ourself.  XXX There is a short race here, as p_nrlwps is only
	 * incremented when an LWP suspends itself on the kernel/user
	 * boundary.  It's still possible to kill -9 the process so we
	 * don't bother checking further.
	 */
	lwp_lock(t);
	if ((t == l && p->p_nrlwps == 1) ||
	    (l->l_flag & (LW_WCORE | LW_WEXIT)) != 0) {
		lwp_unlock(t);
		mutex_exit(p->p_lock);
		return EDEADLK;
	}

	/*
	 * Suspend the LWP.  XXX If it's on a different CPU, we should wait
	 * for it to be preempted, where it will put itself to sleep. 
	 *
	 * Suspension of the current LWP will happen on return to userspace.
	 */
	error = lwp_suspend(l, t);
	if (error) {
		mutex_exit(p->p_lock);
		return error;
	}

	/*
	 * Wait for:
	 *  o process exiting
	 *  o target LWP suspended
	 *  o target LWP not suspended and L_WSUSPEND clear
	 *  o target LWP exited
	 */
	for (;;) {
		error = cv_wait_sig(&p->p_lwpcv, p->p_lock);
		if (error) {
			error = ERESTART;
			break;
		}
		if (lwp_find(p, SCARG(uap, target)) == NULL) {
			error = ESRCH;
			break;
		}
		if ((l->l_flag | t->l_flag) & (LW_WCORE | LW_WEXIT)) {
			error = ERESTART;
			break;
		}
		if (t->l_stat == LSSUSPENDED ||
		    (t->l_flag & LW_WSUSPEND) == 0)
			break;
	}
	mutex_exit(p->p_lock);

	return error;
}

int
sys__lwp_continue(struct lwp *l, const struct sys__lwp_continue_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	int error;
	struct proc *p = l->l_proc;
	struct lwp *t;

	error = 0;

	mutex_enter(p->p_lock);
	if ((t = lwp_find(p, SCARG(uap, target))) == NULL) {
		mutex_exit(p->p_lock);
		return ESRCH;
	}

	lwp_lock(t);
	lwp_continue(t);
	mutex_exit(p->p_lock);

	return error;
}

int
sys__lwp_wakeup(struct lwp *l, const struct sys__lwp_wakeup_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t) target;
	} */
	struct lwp *t;
	struct proc *p;
	int error;

	p = l->l_proc;
	mutex_enter(p->p_lock);

	if ((t = lwp_find(p, SCARG(uap, target))) == NULL) {
		mutex_exit(p->p_lock);
		return ESRCH;
	}

	lwp_lock(t);
	t->l_flag |= (LW_CANCELLED | LW_UNPARKED);

	if (t->l_stat != LSSLEEP) {
		lwp_unlock(t);
		error = ENODEV;
	} else if ((t->l_flag & LW_SINTR) == 0) {
		lwp_unlock(t);
		error = EBUSY;
	} else {
		/* Wake it up.  lwp_unsleep() will release the LWP lock. */
		lwp_unsleep(t, true);
		error = 0;
	}

	mutex_exit(p->p_lock);

	return error;
}

int
sys__lwp_wait(struct lwp *l, const struct sys__lwp_wait_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t) wait_for;
		syscallarg(lwpid_t *) departed;
	} */
	struct proc *p = l->l_proc;
	int error;
	lwpid_t dep;

	mutex_enter(p->p_lock);
	error = lwp_wait(l, SCARG(uap, wait_for), &dep, false);
	mutex_exit(p->p_lock);

	if (!error && SCARG(uap, departed)) {
		error = copyout(&dep, SCARG(uap, departed), sizeof(dep));
	}

	return error;
}

int
sys__lwp_kill(struct lwp *l, const struct sys__lwp_kill_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t)	target;
		syscallarg(int)		signo;
	} */
	struct proc *p = l->l_proc;
	struct lwp *t;
	ksiginfo_t ksi;
	int signo = SCARG(uap, signo);
	int error = 0;

	if ((u_int)signo >= NSIG)
		return EINVAL;

	KSI_INIT(&ksi);
	ksi.ksi_signo = signo;
	ksi.ksi_code = SI_LWP;
	ksi.ksi_pid = p->p_pid;
	ksi.ksi_uid = kauth_cred_geteuid(l->l_cred);
	ksi.ksi_lid = SCARG(uap, target);

	mutex_enter(&proc_lock);
	mutex_enter(p->p_lock);
	if ((t = lwp_find(p, ksi.ksi_lid)) == NULL)
		error = ESRCH;
	else if (signo != 0)
		kpsignal2(p, &ksi);
	mutex_exit(p->p_lock);
	mutex_exit(&proc_lock);

	return error;
}

int
sys__lwp_detach(struct lwp *l, const struct sys__lwp_detach_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t)	target;
	} */
	struct proc *p;
	struct lwp *t;
	lwpid_t target;
	int error;

	target = SCARG(uap, target);
	p = l->l_proc;

	mutex_enter(p->p_lock);

	if (l->l_lid == target)
		t = l;
	else {
		/*
		 * We can't use lwp_find() here because the target might
		 * be a zombie.
		 */
		t = proc_find_lwp(p, target);
		KASSERT(t == NULL || t->l_lid == target);
	}

	/*
	 * If the LWP is already detached, there's nothing to do.
	 * If it's a zombie, we need to clean up after it.  LSZOMB
	 * is visible with the proc mutex held.
	 *
	 * After we have detached or released the LWP, kick any
	 * other LWPs that may be sitting in _lwp_wait(), waiting
	 * for the target LWP to exit.
	 */
	if (t != NULL && t->l_stat != LSIDL) {
		if ((t->l_prflag & LPR_DETACHED) == 0) {
			p->p_ndlwps++;
			t->l_prflag |= LPR_DETACHED;
			if (t->l_stat == LSZOMB) {
				/* Releases proc mutex. */
				lwp_free(t, false, false);
				return 0;
			}
			error = 0;

			/*
			 * Have any LWPs sleeping in lwp_wait() recheck
			 * for deadlock.
			 */
			cv_broadcast(&p->p_lwpcv);
		} else
			error = EINVAL;
	} else
		error = ESRCH;

	mutex_exit(p->p_lock);

	return error;
}

int
lwp_unpark(const lwpid_t *tp, const u_int ntargets)
{
	u_int target;
	int error, s;
	proc_t *p;
	lwp_t *t;

	p = curproc;
	error = 0;

	s = pserialize_read_enter();
	for (target = 0; target < ntargets; target++) {
		t = proc_find_lwp_unlocked(p, tp[target]);
		if (__predict_false(t == NULL)) {
			error = ESRCH;
			continue;
		}

		KASSERT(lwp_locked(t, NULL));

		if (__predict_true(t->l_syncobj == &lwp_park_syncobj)) {
			/*
			 * As expected it's parked, so wake it up. 
			 * lwp_unsleep() will release the LWP lock.
			 */
			lwp_unsleep(t, true);
		} else if (__predict_false(t->l_stat == LSZOMB)) {
			lwp_unlock(t);
			error = ESRCH;
		} else {
			/*
			 * It hasn't parked yet because the wakeup side won
			 * the race, or something else has happened to make
			 * the thread not park.  Why doesn't really matter. 
			 * Set the operation pending, so that the next call
			 * to _lwp_park() in the LWP returns early.  If it
			 * turns out to be a spurious wakeup, no harm done.
			 */
			t->l_flag |= LW_UNPARKED;
			lwp_unlock(t);
		}
	}
	pserialize_read_exit(s);

	return error;
}

int
lwp_park(clockid_t clock_id, int flags, struct timespec *ts)
{
	int timo, error;
	struct timespec start;
	lwp_t *l;
	bool timeremain = !(flags & TIMER_ABSTIME) && ts;

	if (ts != NULL) {
		if ((error = ts2timo(clock_id, flags, ts, &timo, 
		    timeremain ? &start : NULL)) != 0)
			return error;
		KASSERT(timo != 0);
	} else {
		timo = 0;
	}

	/*
	 * Before going the full route and blocking, check to see if an
	 * unpark op is pending.
	 */
	l = curlwp;
	lwp_lock(l);
	if ((l->l_flag & (LW_CANCELLED | LW_UNPARKED)) != 0) {
		l->l_flag &= ~(LW_CANCELLED | LW_UNPARKED);
		lwp_unlock(l);
		return EALREADY;
	}
	l->l_biglocks = 0;
	sleepq_enqueue(NULL, l, "parked", &lwp_park_syncobj, true);
	error = sleepq_block(timo, true, &lwp_park_syncobj);
	switch (error) {
	case EWOULDBLOCK:
		error = ETIMEDOUT;
		if (timeremain)
			memset(ts, 0, sizeof(*ts));
		break;
	case ERESTART:
		error = EINTR;
		/*FALLTHROUGH*/
	default:
		if (timeremain)
			clock_timeleft(clock_id, ts, &start);
		break;
	}
	return error;
}

/*
 * 'park' an LWP waiting on a user-level synchronisation object.  The LWP
 * will remain parked until another LWP in the same process calls in and
 * requests that it be unparked.
 */
int
sys____lwp_park60(struct lwp *l, const struct sys____lwp_park60_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(clockid_t)			clock_id;
		syscallarg(int)				flags;
		syscallarg(struct timespec *)		ts;
		syscallarg(lwpid_t)			unpark;
		syscallarg(const void *)		hint;
		syscallarg(const void *)		unparkhint;
	} */
	struct timespec ts, *tsp;
	int error;

	if (SCARG(uap, ts) == NULL)
		tsp = NULL;
	else {
		error = copyin(SCARG(uap, ts), &ts, sizeof(ts));
		if (error != 0)
			return error;
		tsp = &ts;
	}

	if (SCARG(uap, unpark) != 0) {
		error = lwp_unpark(&SCARG(uap, unpark), 1);
		if (error != 0)
			return error;
	}

	error = lwp_park(SCARG(uap, clock_id), SCARG(uap, flags), tsp);
	if (SCARG(uap, ts) != NULL && (SCARG(uap, flags) & TIMER_ABSTIME) == 0)
		(void)copyout(tsp, SCARG(uap, ts), sizeof(*tsp));
	return error;
}

int
sys__lwp_unpark(struct lwp *l, const struct sys__lwp_unpark_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t)		target;
		syscallarg(const void *)	hint;
	} */

	return lwp_unpark(&SCARG(uap, target), 1);
}

int
sys__lwp_unpark_all(struct lwp *l, const struct sys__lwp_unpark_all_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const lwpid_t *)	targets;
		syscallarg(size_t)		ntargets;
		syscallarg(const void *)	hint;
	} */
	lwpid_t targets[32], *tp;
	int error;
	u_int ntargets;
	size_t sz;

	ntargets = SCARG(uap, ntargets);
	if (SCARG(uap, targets) == NULL) {
		/*
		 * Let the caller know how much we are willing to do, and
		 * let it unpark the LWPs in blocks.
		 */
		*retval = LWP_UNPARK_MAX;
		return 0;
	}
	if (ntargets > LWP_UNPARK_MAX || ntargets == 0)
		return EINVAL;

	/*
	 * Copy in the target array.  If it's a small number of LWPs, then
	 * place the numbers on the stack.
	 */
	sz = sizeof(lwpid_t) * ntargets;
	if (sz <= sizeof(targets))
		tp = targets;
	else
		tp = kmem_alloc(sz, KM_SLEEP);
	error = copyin(SCARG(uap, targets), tp, sz);
	if (error != 0) {
		if (tp != targets) {
			kmem_free(tp, sz);
		}
		return error;
	}
	error = lwp_unpark(tp, ntargets);
	if (tp != targets)
		kmem_free(tp, sz);
	return error;
}

int
sys__lwp_setname(struct lwp *l, const struct sys__lwp_setname_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t)		target;
		syscallarg(const char *)	name;
	} */
	char *name, *oname;
	lwpid_t target;
	proc_t *p;
	lwp_t *t;
	int error;

	if ((target = SCARG(uap, target)) == 0)
		target = l->l_lid;

	name = kmem_alloc(MAXCOMLEN, KM_SLEEP);
	error = copyinstr(SCARG(uap, name), name, MAXCOMLEN, NULL);
	switch (error) {
	case ENAMETOOLONG:
	case 0:
		name[MAXCOMLEN - 1] = '\0';
		break;
	default:
		kmem_free(name, MAXCOMLEN);
		return error;
	}

	p = curproc;
	mutex_enter(p->p_lock);
	if ((t = lwp_find(p, target)) == NULL) {
		mutex_exit(p->p_lock);
		kmem_free(name, MAXCOMLEN);
		return ESRCH;
	}
	lwp_lock(t);
	oname = t->l_name;
	t->l_name = name;
	lwp_unlock(t);
	mutex_exit(p->p_lock);

	if (oname != NULL)
		kmem_free(oname, MAXCOMLEN);

	return 0;
}

int
sys__lwp_getname(struct lwp *l, const struct sys__lwp_getname_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(lwpid_t)		target;
		syscallarg(char *)		name;
		syscallarg(size_t)		len;
	} */
	char name[MAXCOMLEN];
	lwpid_t target;
	size_t len;
	proc_t *p;
	lwp_t *t;

	if ((target = SCARG(uap, target)) == 0)
		target = l->l_lid;

	p = curproc;
	mutex_enter(p->p_lock);
	if ((t = lwp_find(p, target)) == NULL) {
		mutex_exit(p->p_lock);
		return ESRCH;
	}
	lwp_lock(t);
	if (t->l_name == NULL)
		name[0] = '\0';
	else
		strlcpy(name, t->l_name, sizeof(name));
	lwp_unlock(t);
	mutex_exit(p->p_lock);

	len = uimin(SCARG(uap, len), sizeof(name));

	return copyoutstr(name, SCARG(uap, name), len, NULL);
}

int
sys__lwp_ctl(struct lwp *l, const struct sys__lwp_ctl_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int)			features;
		syscallarg(struct lwpctl **)	address;
	} */
	int error, features;
	vaddr_t vaddr;

	features = SCARG(uap, features);
	features &= ~(LWPCTL_FEATURE_CURCPU | LWPCTL_FEATURE_PCTR);
	if (features != 0)
		return ENODEV;
	if ((error = lwp_ctl_alloc(&vaddr)) != 0)
		return error;
	return copyout(&vaddr, SCARG(uap, address), sizeof(void *));
}
