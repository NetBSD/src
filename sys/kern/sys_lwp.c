/*	$NetBSD: sys_lwp.c,v 1.1.2.3 2006/11/17 16:34:37 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006 The NetBSD Foundation, Inc.
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

/*
 * Lightweight process (LWP) system calls.  See kern_lwp.c for a description
 * of LWPs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sys_lwp.c,v 1.1.2.3 2006/11/17 16:34:37 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/types.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>

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
	int error, lid;

	mutex_enter(&p->p_smutex);
	if ((p->p_sflag & (PS_SA | PS_WEXIT)) != 0 || p->p_sa != NULL) {
		mutex_exit(&p->p_smutex);
		return EINVAL;
	}
	p->p_sflag |= PS_NOSA;
	mutex_exit(&p->p_smutex);

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

	mutex_enter(&p->p_smutex);
	lwp_lock(l2);
	lid = l2->l_lid;
	if ((SCARG(uap, flags) & LWP_SUSPENDED) == 0 &&
	    (l->l_flag & L_WREBOOT) == 0) {
		p->p_nrlwps++;
		lwp_relock(l2, &sched_mutex);
		l2->l_stat = LSRUN;
		setrunqueue(l2);
	} else
		l2->l_stat = LSSUSPENDED;
	lwp_unlock(l2);
	mutex_exit(&p->p_smutex);

	error = copyout(&lid, SCARG(uap, new_lwp), sizeof(lid));
	if (error) {
		/* XXX We should destroy the LWP. */
		return (error);
	}

	return (0);
}

int
sys__lwp_exit(struct lwp *l, void *v, register_t *retval)
{

	return lwp_exit(l, 1);
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

	mb_read();
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
	mb_write();

	return (0);
}

int
sys__lwp_suspend(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_suspend_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct lwp *t;
	int error;

	mutex_enter(&p->p_smutex);

	if ((p->p_flag & P_SA) != 0 || p->p_sa != NULL) {
		mutex_exit(&p->p_smutex);
		return EINVAL;
	}

	if ((t = lwp_byid(p, SCARG(uap, target))) == NULL) {
		mutex_exit(&p->p_smutex);
		return (ESRCH);
	}

	/*
	 * Check for deadlock, which is only possible when we're suspending
	 * ourself.
	 */
	if (t == l && l->l_stat == LSONPROC && p->p_nrlwps == 1) {
		lwp_unlock(l);
		mutex_exit(&p->p_smutex);
		return (EDEADLK);
	}

	/*
	 * Suspend the LWP.  If it's on a different CPU, we need to wait for
	 * it to be preempted, where it will put itself to sleep.  If not 
	 * suspending ourselves, the LWP will be returned unlocked.
	 */
	error = lwp_halt(l, t, LSSUSPENDED);
	mutex_exit(&p->p_smutex);

	/*
	 * If we suspended ourself, we need to sleep now.
	 */
	if (t == l && !error) {
		lwp_lock(l);
		l->l_nvcsw++;
		mi_switch(t, NULL);
	}

	return (error);
}

int
sys__lwp_continue(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_continue_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	int error;
	struct proc *p = l->l_proc;
	struct lwp *t;

	error = 0;

	mutex_enter(&p->p_smutex);

	if ((p->p_flag & P_SA) != 0 || p->p_sa != NULL)
		error = EINVAL;
	else if ((t = lwp_byid(p, SCARG(uap, target))) == NULL)
		error = ESRCH;
	else if (t == l || t->l_stat != LSSUSPENDED)
		lwp_unlock(t);
	else
		lwp_continue(t);

	mutex_exit(&p->p_smutex);
	
	return (error);
}

int
sys__lwp_wakeup(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_wakeup_args /* {
		syscallarg(lwpid_t) target;
	} */ *uap = v;
	struct lwp *t;
	struct proc *p;
	int error;

	p = l->l_proc;
	mutex_enter(&p->p_smutex);

	if ((t = lwp_byid(p, SCARG(uap, target))) == NULL) {
		mutex_exit(&p->p_smutex);
		return ESRCH;
	}

	if (t->l_stat != LSSLEEP) {
		error = ENODEV;
		goto bad;
	}

	if ((t->l_flag & L_SINTR) == 0) {
		error = EBUSY;
		goto bad;
	}

	/* wake it up  setrunnable() will release the LWP lock. */
	t->l_flag |= L_CANCELLED;
	setrunnable(t);
	mutex_exit(&p->p_smutex);
	return 0;

 bad:
 	lwp_unlock(l);
	mutex_exit(&p->p_smutex);
	return error;
}

int
sys__lwp_wait(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_wait_args /* {
		syscallarg(lwpid_t) wait_for;
		syscallarg(lwpid_t *) departed;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	lwpid_t dep;

	mutex_enter(&p->p_smutex);
	error = lwp_wait1(l, SCARG(uap, wait_for), &dep, 0);
	mutex_exit(&p->p_smutex);
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

/* ARGSUSED */
int
sys__lwp_kill(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_kill_args /* {
		syscallarg(lwpid_t)	target;
		syscallarg(int)		signo;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct lwp *t;
	ksiginfo_t ksi;
	int signo = SCARG(uap, signo);
	int error;

	if ((u_int)signo >= NSIG)
		return (EINVAL);

	KSI_INIT(&ksi);
	ksi.ksi_signo = signo;
	ksi.ksi_code = SI_USER;
	ksi.ksi_pid = p->p_pid;
	ksi.ksi_uid = kauth_cred_geteuid(l->l_cred);
	ksi.ksi_lid = SCARG(uap, target);

	rw_enter(&proclist_lock, RW_READER);
	mutex_enter(&p->p_smutex);
	if ((t = lwp_byid(p, ksi.ksi_lid)) == NULL)
		error = ESRCH;
	else {
		lwp_unlock(t);
		kpsignal2(p, &ksi);
		error = 0;
	}
	mutex_exit(&p->p_smutex);
	rw_exit(&proclist_lock);

	return (error);
}
