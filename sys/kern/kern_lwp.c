/*	$NetBSD: kern_lwp.c,v 1.1.2.4 2001/08/24 04:20:07 nathanw Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

struct lwplist alllwp;
struct lwplist deadlwp;
struct lwplist zomblwp;

#define LWP_DEBUG

#ifdef LWP_DEBUG
int lwp_debug = 0;
#define DPRINTF(x) if (lwp_debug) printf x
#else
#define DPRINTF(x)
#endif
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
	ucontext_t *newuc;
	int s, error;

	newuc = pool_get(&lwp_uc_pool, PR_WAITOK);

	error = copyin(SCARG(uap, ucp), newuc, sizeof(*newuc));
	if (error)
		return (error);

	/* XXX check against resource limits */

	uaddr = uvm_km_valloc(kernel_map, USPACE);
	if (__predict_false(uaddr == 0)) {
		return (ENOMEM);
	}

	/* XXX flags:
	 * __LWP_ASLWP is probably needed for Solaris compat.
	 */

	newlwp(l, p, uaddr, 
	    SCARG(uap, flags) & LWP_DETACHED, 
	    NULL, NULL, startlwp, newuc, &l2);

	if ((SCARG(uap, flags) & LWP_SUSPENDED) == 0) {
		SCHED_LOCK(s);
		l2->l_stat = LSRUN;
		setrunqueue(l2);
		SCHED_UNLOCK(s);
		simple_lock(&p->p_lwplock);
		p->p_nrlwps++;
		simple_unlock(&p->p_lwplock);
	} else {
		l2->l_stat = LSSUSPENDED;
	}

	error = copyout(&l2->l_lid, SCARG(uap, new_lwp), 
	    sizeof(l2->l_lid));
	if (error)
		return (error);

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
sys__lwp_suspend(struct lwp *l, void *v, register_t *retval)
{
	struct sys__lwp_suspend_args /* {
	syscallarg(lwpid_t) target;
	} */ *uap = v;
	int target_lid;
	struct proc *p = l->l_proc;
	struct lwp *t, *t2;
	int s;

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

		SCHED_LOCK(s);
		l->l_stat = LSSUSPENDED;
		/* XXX NJWLWP check if this makes sense here: */
		l->l_proc->p_stats->p_ru.ru_nvcsw++; 
		mi_switch(l, NULL);
		SCHED_ASSERT_UNLOCKED();
	} else {
		switch (t->l_stat) {
		case LSSUSPENDED:
			return (0); /* _lwp_suspend() is idempotent */
		case LSRUN:
			SCHED_LOCK(s);
			remrunqueue(t);
			t->l_stat = LSSUSPENDED;
			SCHED_UNLOCK(s);
			simple_lock(&p->p_lwplock);
			p->p_nrlwps--;
			simple_unlock(&p->p_lwplock);
			break;
		case LSSLEEP:
			t->l_stat = LSSUSPENDED;
			break;
		case LSIDL:
		case LSDEAD:
		case LSZOMB:
			return (EINTR); /* It's what Solaris does..... */
		case LSSTOP:
			panic("_lwp_suspend: Stopped LWP in running process!");
			break;
		case LSONPROC:
			panic("XXX multiprocessor LWPs? Implement me!");
			break;
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
	int target_lid;
	struct proc *p = l->l_proc;
	struct lwp *t;
	int s;

	target_lid = SCARG(uap, target);

	LIST_FOREACH(t, &p->p_lwps, l_sibling)
		if (t->l_lid == target_lid)
			break;

	if (t == NULL)
		return (ESRCH);
	
	if (t->l_stat != LSSUSPENDED)
		return (0);
	
	if (t->l_wchan == 0) { 
		/* LWP was runnable before being suspended. */
		SCHED_LOCK(s);
		setrunnable(t);
		simple_lock(&p->p_lwplock);
		p->p_nrlwps++;
		simple_unlock(&p->p_lwplock);
		SCHED_UNLOCK(s);
	} else {
		/* LWP was sleeping before being suspended */
		t->l_stat = LSSLEEP;
	}

	return (0);
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
	int nfound, error, s, wpri;
	static char waitstr1[] = "lwpwait";
	static char waitstr2[] = "lwpwait2";

	DPRINTF(("lwp_wait1: %d.%d waiting for %d.\n",
	    p->p_pid, l->l_lid, lid));

	if (lid == l->l_lid)
		return (EDEADLK); /* Waiting for ourselves makes no sense. */
	
	wpri = PWAIT | PCATCH | 
	    ((flags & LWPWAIT_EXITCONTROL) ? PNOEXITERR : 0);
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
			
			s = proclist_lock_write();
			LIST_REMOVE(l2, l_zlist); /* off zomblwp */
			proclist_unlock_write(s);

			simple_lock(&p->p_lwplock);
			LIST_REMOVE(l2, l_sibling);
			p->p_nlwps--;
			p->p_nzlwps--;
			simple_unlock(&p->p_lwplock);
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
newlwp(struct lwp *l1, struct proc *p2, vaddr_t uaddr, 
    int flags, void *stack, size_t stacksize,
    void (*func)(void *), void *arg, struct lwp **rnewlwpp)
{
	struct lwp *l2;
	int s;

	l2 = pool_get(&lwp_pool, PR_WAITOK);

	l2->l_stat = LSIDL;
	l2->l_forw = l2->l_back = NULL;
	l2->l_proc = p2;


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
	l2->l_cpu = l1->l_cpu;
#else
	/*
	 * zero child's cpu pointer so we don't get trash.
	 */
	l2->l_cpu = NULL;
#endif /* ! MULTIPROCESSOR */

	l2->l_flag = L_INMEM;
	l2->l_flag |= (flags & LWP_DETACHED) ? L_DETACHED : 0;

	callout_init(&l2->l_tsleep_ch);

	if (rnewlwpp != NULL)
		*rnewlwpp = l2;

	l2->l_addr = (struct user *)uaddr;
	uvm_lwp_fork(l1, l2, stack, stacksize, func, 
	    (arg != NULL) ? arg : l2);


	simple_lock(&p2->p_lwplock);
	l2->l_lid = ++p2->p_nlwpid;
	LIST_INSERT_HEAD(&p2->p_lwps, l2, l_sibling);
	p2->p_nlwps++;
	simple_unlock(&p2->p_lwplock);
	
	/* XXX should be locked differently... */
	s = proclist_lock_write();
	LIST_INSERT_HEAD(&alllwp, l2, l_list);
	proclist_unlock_write(s);

	return (0);
}


/*
 * Quit the process. This will call cpu_exit, which will call cpu_switch,
 * so this can only be used meaningfully if you're willing to switch away.
 * Calling with l!=curproc would be weird.
 */
void
lwp_exit(struct lwp *l)
{
	struct proc *p = l->l_proc;
	int s;

	DPRINTF(("lwp_exit: %d.%d exiting.\n", p->p_pid, l->l_lid));
	DPRINTF((" nlwps: %d nrlwps %d nzlwps: %d\n", 
	    p->p_nlwps, p->p_nrlwps, p->p_nzlwps));
	/* 
	 * If we are the last live LWP in a process, we need to exit
	 * the entire process (if that's not already going on). We do
	 * so with an exit status of zero, because it's a "controlled"
	 * exit, and because that's what Solaris does.  
	 */
	if (((p->p_nlwps - p->p_nzlwps) == 1) && ((p->p_flag & P_WEXIT) == 0)) {
		DPRINTF(("lwp_exit: %d.%d calling exit1()\n",
		    p->p_pid, l->l_lid));
		exit1(l, 0); 
	}

	s = proclist_lock_write();
	LIST_REMOVE(l, l_list);
	if ((l->l_flag & L_DETACHED) == 0) {
		DPRINTF(("lwp_exit: %d.%d going on zombie list\n", p->p_pid,
		    l->l_lid));
		LIST_INSERT_HEAD(&zomblwp, l, l_zlist);
	}
	proclist_unlock_write(s);

	simple_lock(&p->p_lwplock);
	p->p_nrlwps--;
	simple_unlock(&p->p_lwplock);

	l->l_stat = LSDEAD;
	
	/* cpu_exit() will not return */
	cpu_exit(l, 0);

}


void
lwp_exit2(struct lwp *l)
{

	simple_lock(&deadproc_slock);
	LIST_INSERT_HEAD(&deadlwp, l, l_list);
	simple_unlock(&deadproc_slock);

	wakeup(&deadproc);
}
