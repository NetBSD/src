/*	$NetBSD: kern_sa.c,v 1.20 2003/08/11 21:18:18 fvdl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_sa.c,v 1.20 2003/08/11 21:18:18 fvdl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <sys/kernel.h>  /* For lbolt hack */

static void sa_vp_donate(struct lwp *);
static int sa_newcachelwp(struct lwp *);
static struct lwp *sa_vp_repossess(struct lwp *l);

void sa_upcall_getstate(struct sadata_upcall *, struct lwp *, struct lwp *);

MALLOC_DEFINE(M_SA, "sa", "Scheduler activations");

#define SA_DEBUG

#ifdef SA_DEBUG
#define DPRINTF(x)	do { if (sadebug) printf x; } while (0)
#define DPRINTFN(n,x)	do { if (sadebug & (1<<(n-1))) printf x; } while (0)
int	sadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif



#define SA_LWP_STATE_LOCK(l,s)      \
    do {                            \
           s =  (l)->l_flag ;       \
           (l)->l_flag &= ~L_SA;    \
    } while (0)                     
           


#define SA_LWP_STATE_UNLOCK(l,s)      \
    do {                               \
	(l)->l_flag |=  ( s & L_SA);   \
    } while (0)                     
   



/*
 * sadata_upcall_alloc:
 *
 *	Allocate an sadata_upcall structure.
 */
struct sadata_upcall *
sadata_upcall_alloc(int waitok)
{

	/* XXX zero the memory? */
	return (pool_get(&saupcall_pool, waitok ? PR_WAITOK : PR_NOWAIT));
}

/*
 * sadata_upcall_free:
 *
 *	Free an sadata_upcall structure, and any associated
 *	argument data.
 */
void
sadata_upcall_free(struct sadata_upcall *sau)
{
	extern struct pool siginfo_pool;	/* XXX Ew. */

	/*
	 * XXX We have to know what the origin of sau_arg is
	 * XXX in order to do the right thing, here.  Sucks
	 * XXX to be a non-garbage-collecting kernel.
	 */
	if (sau->sau_arg) {
		switch (sau->sau_type) {
		case SA_UPCALL_SIGNAL:
			pool_put(&siginfo_pool, sau->sau_arg);
			break;
		case SA_UPCALL_SIGEV:
			/* don't need to deallocate it at all */
			break;
		default:
			panic("sadata_free: unknown type of sau_arg: %d",
			    sau->sau_type);
		}
	}

	pool_put(&saupcall_pool, sau);
}

int
sys_sa_register(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_register_args /* {
		syscallarg(sa_upcall_t) new;
		syscallarg(sa_upcall_t *) old;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sadata *sa;
	sa_upcall_t prev;
	int error;

	if (p->p_sa == NULL) {
		/* Allocate scheduler activations data structure */
		sa = pool_get(&sadata_pool, PR_WAITOK);
		/* Initialize. */
		memset(sa, 0, sizeof(*sa));
		simple_lock_init(&sa->sa_lock);
		sa->sa_flag = SCARG(uap, flags) & SA_FLAG_ALL;
		sa->sa_vp = NULL;
		sa->sa_old_lwp = NULL;
		sa->sa_vp_wait_count = 0;
		sa->sa_idle = NULL;
		sa->sa_woken = NULL;
		sa->sa_concurrency = 1;
		sa->sa_stacks = malloc(sizeof(stack_t) * SA_NUMSTACKS,
		    M_SA, M_WAITOK);
		sa->sa_nstacks = 0;
		LIST_INIT(&sa->sa_lwpcache);
		SIMPLEQ_INIT(&sa->sa_upcalls);
		p->p_sa = sa;
		sa_newcachelwp(l);
	}

	prev = p->p_sa->sa_upcall;
	p->p_sa->sa_upcall = SCARG(uap, new);
	if (SCARG(uap, old)) {
		error = copyout(&prev, SCARG(uap, old),
		    sizeof(prev));
		if (error)
			return (error);
	}

	return (0);
}


int
sys_sa_stacks(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_stacks_args /* {
		syscallarg(int) num;
		syscallarg(stack_t *) stacks;
	} */ *uap = v;
	struct sadata *sa = l->l_proc->p_sa;
	int error, count;

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	count = SCARG(uap, num);
	if (count < 0)
		return (EINVAL);
	count = min(count, SA_NUMSTACKS - sa->sa_nstacks);

	error = copyin(SCARG(uap, stacks), sa->sa_stacks + sa->sa_nstacks,
	    sizeof(stack_t) * count);
	if (error)
		return (error);

	if ((sa->sa_nstacks == 0) && (sa->sa_vp_wait_count != 0))
	{
		l->l_flag |= L_SA_UPCALL; 
	}

	sa->sa_nstacks += count;
	DPRINTFN(9, ("sa_stacks(%d.%d) nstacks + %d = %2d\n",
	    l->l_proc->p_pid, l->l_lid, count, sa->sa_nstacks));

	*retval = count;
	return (0);
}


int
sys_sa_enable(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	int error;

	DPRINTF(("sys_sa_enable(%d.%d)\n", l->l_proc->p_pid,
	    l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	if (p->p_flag & P_SA) /* Already running! */
		return (EBUSY);

	error = sa_upcall(l, SA_UPCALL_NEWPROC, l, NULL, 0, NULL);
	if (error)
		return (error);

	p->p_flag |= P_SA;
	l->l_flag |= L_SA; /* We are now an activation LWP */

	/* Assign this LWP to the virtual processor */
	sa->sa_vp = l;

	/* This will not return to the place in user space it came from. */
	return (0);
}


int
sys_sa_setconcurrency(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_setconcurrency_args /* {
		syscallarg(int) concurrency;
	} */ *uap = v;
	struct sadata *sa = l->l_proc->p_sa;

	DPRINTF(("sys_sa_concurrency(%d.%d)\n", l->l_proc->p_pid,
	    l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	if (SCARG(uap, concurrency) < 1)
		return (EINVAL);

	*retval = sa->sa_concurrency;
	/*
	 * Concurrency greater than the number of physical CPUs does
	 * not make sense.
	 * XXX Should we ever support hot-plug CPUs, this will need
	 * adjustment.
	 */
	sa->sa_concurrency = min(SCARG(uap, concurrency), 1 /* XXX ncpus */);

	return (0);
}

int
sys_sa_yield(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	if (p->p_sa == NULL || !(p->p_flag & P_SA))
		return (EINVAL);

	sa_yield(l);

	return (0);
}

void
sa_yield(struct lwp *l)
{
#if 0
	struct lwp *l2;
#endif
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	int s, ret;
	
	/*
	 * If we're the last running LWP, stick around to recieve
	 * signals.
	 */
#if 0
	if (p->p_nrlwps == 1) {
#endif
		DPRINTFN(1,("sa_yield(%d.%d) going dormant\n",
		    p->p_pid, l->l_lid));
		/*
		 * A signal will probably wake us up. Worst case, the upcall
		 * happens and just causes the process to yield again.
		 */	
	SCHED_ASSERT_UNLOCKED();

		sa_vp_donate(l);

	SCHED_ASSERT_UNLOCKED();

		s = splsched();	/* Protect from timer expirations */
		KDASSERT(sa->sa_vp == l);
		/*
		 * If we were told to make an upcall or exit before
		 * the splsched(), make sure we process it instead of
		 * going to sleep. It might make more sense for this to
		 * be handled inside of tsleep....
		 */
		ret = 0;	

		while  ((ret == 0) && (p->p_userret == NULL)) {
			sa->sa_idle = l;
			l->l_flag &= ~L_SA;
			SCHED_ASSERT_UNLOCKED();
		

			ret = tsleep((caddr_t) l, PUSER | PCATCH, "sawait", 0);

			SCHED_ASSERT_UNLOCKED();

			l->l_flag |= L_SA;
			sa->sa_idle = NULL;
			splx(s);
			sa_vp_donate(l);
			KDASSERT(sa->sa_vp == l);

		
			s = splsched();	/* Protect from timer expirations */

		}


		l->l_flag |= L_SA_UPCALL; 
		splx(s);
		DPRINTFN(1,("sa_yield(%d.%d) returned\n",
		    p->p_pid, l->l_lid));
#if 0
	} else {
		DPRINTFN(1,("sa_yield(%d.%d) stepping aside\n", p->p_pid, l->l_lid));
	
		SCHED_LOCK(s);
		l2 = sa->sa_woken;
		sa->sa_woken = NULL;
		sa->sa_vp = NULL;
		p->p_nrlwps--;
		sa_putcachelwp(p, l);
		KDASSERT((l2 == NULL) || (l2->l_proc == l->l_proc));
		KDASSERT((l2 == NULL) || (l2->l_stat == LSRUN));
		mi_switch(l, l2);
		/*
		 * This isn't quite a NOTREACHED; we may get here if
		 * the process exits before this LWP is reused. In
		 * that case, we want to call lwp_exit(), which will
		 * be done by the userret() hooks.
		 */
		SCHED_ASSERT_UNLOCKED();
		splx(s);
		KDASSERT(p->p_flag & P_WEXIT);
		/* mostly NOTREACHED */
	}
#endif
}


int
sys_sa_preempt(struct lwp *l, void *v, register_t *retval)
{

	/* XXX Implement me. */
	return (ENOSYS);
}


/* XXX Hm, naming collision. */
void
sa_preempt(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;

	if (sa->sa_flag & SA_FLAG_PREEMPT)
		sa_upcall(l, SA_UPCALL_PREEMPTED, l, NULL, 0, NULL);
}


/*
 * Set up the user-level stack and trapframe to do an upcall.
 *
 * NOTE: This routine WILL FREE "arg" in the case of failure!  Callers
 * should not touch the "arg" pointer once calling sa_upcall().
 */
int
sa_upcall(struct lwp *l, int type, struct lwp *event, struct lwp *interrupted,
	size_t argsize, void *arg)
{
	struct sadata_upcall *sau;
	struct sadata *sa = l->l_proc->p_sa;
	stack_t st;
	int s;

	/* XXX prevent recursive upcalls if we sleep formemory */
	SA_LWP_STATE_LOCK(l,s);

	sau = sadata_upcall_alloc(1);

	SA_LWP_STATE_UNLOCK(l,s);


	if (sa->sa_nstacks == 0) {
		/* assign to assure that it gets freed */
		sau->sau_type = type & ~SA_UPCALL_DEFER;
		sau->sau_arg = arg;
		sadata_upcall_free(sau);
		return (ENOMEM);
	}
	st = sa->sa_stacks[--sa->sa_nstacks];
	DPRINTFN(9,("sa_upcall(%d.%d) nstacks--   = %2d\n", 
	    l->l_proc->p_pid, l->l_lid, sa->sa_nstacks));

	return sa_upcall0(l, type, event, interrupted, argsize, arg, sau, &st);
}

int
sa_upcall0(struct lwp *l, int type, struct lwp *event, struct lwp *interrupted,
    size_t argsize, void *arg, struct sadata_upcall *sau, stack_t *st)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;

	KDASSERT((event == NULL) || (event != interrupted));

	sau->sau_flags = 0;
	sau->sau_type = type & ~SA_UPCALL_DEFER;
	sau->sau_argsize = argsize;
	sau->sau_arg = arg;
	sau->sau_stack = *st;

	if (type & SA_UPCALL_DEFER) {
		sau->sau_state.deferred.e_lwp = event;
		sau->sau_state.deferred.i_lwp = interrupted;
		sau->sau_flags = SAU_FLAG_DEFERRED;
	} else
		sa_upcall_getstate(sau, event, interrupted);

	SIMPLEQ_INSERT_TAIL(&sa->sa_upcalls, sau, sau_next);
	l->l_flag |= L_SA_UPCALL;

	return (0);
}


void
sa_upcall_getstate(struct sadata_upcall *sau, struct lwp *event,
    struct lwp *interrupted)
{

	if (event) {
		getucontext(event, &sau->sau_state.captured.e_ctx);
		sau->sau_state.captured.e_sa.sa_context = (ucontext_t *)
		    (intptr_t)((_UC_MACHINE_SP(&sau->sau_state.captured.e_ctx) -
			sizeof(ucontext_t))
#ifdef _UC_UCONTEXT_ALIGN
			& _UC_UCONTEXT_ALIGN
#endif
			    );
		sau->sau_state.captured.e_sa.sa_id = event->l_lid;
		sau->sau_state.captured.e_sa.sa_cpu = 0; /* XXX extract from l_cpu */
	} else
		sau->sau_state.captured.e_sa.sa_context = NULL;

	if (interrupted) {
		getucontext(interrupted, &sau->sau_state.captured.i_ctx);
		sau->sau_state.captured.i_sa.sa_context = (ucontext_t *)
		    (intptr_t)((_UC_MACHINE_SP(&sau->sau_state.captured.i_ctx) -
			sizeof(ucontext_t))
#ifdef _UC_UCONTEXT_ALIGN
			& _UC_UCONTEXT_ALIGN
#endif
			    );
		sau->sau_state.captured.i_sa.sa_id = interrupted->l_lid;
		sau->sau_state.captured.i_sa.sa_cpu = 0; /* XXX extract from l_cpu */
	} else
		sau->sau_state.captured.i_sa.sa_context = NULL;

}


/*
 * Called by tsleep(). Block current LWP and switch to another.
 *
 * WE ARE NOT ALLOWED TO SLEEP HERE!  WE ARE CALLED FROM WITHIN
 * TSLEEP() ITSELF!  We are called with sched_lock held, and must
 * hold it right through the mi_switch() call.
 */
void
sa_switch(struct lwp *l, int type)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	struct sadata_upcall *sau;
	struct lwp *l2;
	stack_t st;
	int error;

	DPRINTFN(4,("sa_switch(%d.%d type %d VP %d)\n", p->p_pid, l->l_lid,
	    type, sa->sa_vp ? sa->sa_vp->l_lid : 0));
	SCHED_ASSERT_LOCKED();

	if (p->p_flag & P_WEXIT)
	{
		mi_switch(l,0);
		return;
	}

	if (sa->sa_vp == l) {
		/*
		 * Case 1: we're blocking for the first time; generate
		 * a SA_BLOCKED upcall and allocate resources for the
		 * UNBLOCKED upcall.
		 */
		/*
		 * The process of allocating a new LWP could cause
		 * sleeps. We're called from inside sleep, so that
		 * would be Bad. Therefore, we must use a cached new
		 * LWP. The first thing that this new LWP must do is
		 * allocate another LWP for the cache.  */
		l2 = sa_getcachelwp(p);
		if (l2 == NULL) {
			/* XXXSMP */
			/* No upcall for you! */
			/* XXX The consequences of this are more subtle and
			 * XXX the recovery from this situation deserves
			 * XXX more thought.
			 */

			/* XXXUPSXXX Should only happen with concurrency > 1 */
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): no cached LWP for upcall.\n",
			    p->p_pid, l->l_lid);
#endif
			mi_switch(l, NULL);
			return;
		}

		/*
		 * XXX We need to allocate the sadata_upcall structure here,
		 * XXX since we can't sleep while waiting for memory inside
		 * XXX sa_upcall().  It would be nice if we could safely
		 * XXX allocate the sadata_upcall structure on the stack, here.
		 */

		if (sa->sa_nstacks == 0) {
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d flag %x): Not enough stacks.\n",
			    p->p_pid, l->l_lid, l->l_flag);
#endif
			goto sa_upcall_failed;
		}

		sau = sadata_upcall_alloc(0);
		if (sau == NULL) {
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): "
			    "couldn't allocate upcall data.\n",
			    p->p_pid, l->l_lid);

#endif
			goto sa_upcall_failed;
		}
		st = sa->sa_stacks[--sa->sa_nstacks];
		DPRINTFN(9,("sa_switch(%d.%d) nstacks--   = %2d\n", 
		    l->l_proc->p_pid, l->l_lid, sa->sa_nstacks));

		cpu_setfunc(l2, sa_switchcall, l2);
		error = sa_upcall0(l2, SA_UPCALL_BLOCKED, l, NULL, 0, NULL,
		    sau, &st);
		if (error) {
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): Error %d from sa_upcall()\n",
			    p->p_pid, l->l_lid, error);
#endif
			goto sa_upcall_failed;
		}

		l->l_flag |= L_SA_BLOCKING;
		l2->l_priority = l2->l_usrpri;
		sa->sa_vp = l2;
		setrunnable(l2);
		PRELE(l2); /* Remove the artificial hold-count */

		KDASSERT(l2 != l);
	} else if (sa->sa_vp != NULL) {
		/*
		 * Case 2: We've been woken up while another LWP was
		 * on the VP, but we're going back to sleep without
		 * having returned to userland and delivering the
		 * SA_UNBLOCKED upcall (select and poll cause this
		 * kind of behavior a lot). We just switch back to the
		 * LWP that had been running and let it have another
		 * go. If the LWP on the VP was idling, don't make it
		 * run again, though.
		 */
		if (sa->sa_idle)
			l2 = NULL;
		else {
			l2 = sa->sa_vp; /* XXXUPSXXX Unfair advantage for l2 ? */
			if((l2->l_stat != LSRUN) || ((l2->l_flag & L_INMEM) == 0))
				l2 = NULL;
		}
	} else {

#if 0
		/*
		 * Case 3: The VP is empty. As in case 2, we were
		 * woken up and called tsleep again, but additionally,
		 * the running LWP called sa_yield() between our wakeup() and
		 * when we got to run again (in fact, it probably was
		 * responsible for switching to us, via sa_woken).
		 * The right thing is to pull a LWP off the cache and have
		 * it jump straight back into sa_yield.
		 */
		l2 = sa_getcachelwp(p);
		if (l2 == NULL) {
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): no cached LWP for reidling.\n",
			    p->p_pid, l->l_lid);
#endif
			mi_switch(l, NULL);
			return;
		}
#else
		mi_switch(l, NULL);
		return;
#endif
		
	sa_upcall_failed:
#if 0
		cpu_setfunc(l2, sa_yieldcall, l2);

		l2->l_priority = l2->l_usrpri;
		setrunnable(l2);
		PRELE(l2); /* Remove the artificial hold-count */
#else
		
		/* sa_putcachelwp does not block because we have a hold count on l2 */
		sa_putcachelwp(p, l2);
		PRELE(l2); /* Remove the artificial hold-count */

		mi_switch(l, NULL);
		return;


#endif
	}


	DPRINTFN(4,("sa_switch(%d.%d) switching to LWP %d.\n",
	    p->p_pid, l->l_lid, l2 ? l2->l_lid : 0));
	mi_switch(l, l2);

	DPRINTFN(4,("sa_switch(%d.%d flag %x) returned.\n", p->p_pid, l->l_lid, l->l_flag));
	KDASSERT(l->l_wchan == 0);

	SCHED_ASSERT_UNLOCKED();
	if (sa->sa_woken == l)
		sa->sa_woken = NULL;


	/*
	 * The process is trying to exit. In this case, the last thing
	 * we want to do is put something back on the cache list.
	 * It's also not useful to make the upcall at all, so just punt.
	 */
	if (p->p_flag & P_WEXIT)
		return;

	/*
	 * Okay, now we've been woken up. This means that it's time
	 * for a SA_UNBLOCKED upcall when we get back to userlevel
	 */
	l->l_flag |= L_SA_UPCALL;

}

void
sa_switchcall(void *arg)
{
	struct lwp *l;
	struct proc *p;
	struct sadata *sa;
	int s;

	l = arg;
	p = l->l_proc;
	sa = p->p_sa;
	sa->sa_vp = l;

	DPRINTFN(6,("sa_switchcall(%d.%d)\n", p->p_pid, l->l_lid));

	if (LIST_EMPTY(&sa->sa_lwpcache)) {
		/* Allocate the next cache LWP */
		DPRINTFN(6,("sa_switchcall(%d.%d) allocating LWP\n",
		    p->p_pid, l->l_lid));
		SA_LWP_STATE_LOCK(l,s);
		sa_newcachelwp(l);
		SA_LWP_STATE_UNLOCK(l,s);

	}
	upcallret(l);
}

#if 0
void
sa_yieldcall(void *arg)
{
	struct lwp *l;
	struct proc *p;
	struct sadata *sa;

	l = arg;
	p = l->l_proc;
	sa = p->p_sa;
	sa->sa_vp = l;

	DPRINTFN(6,("sa_yieldcall(%d.%d)\n", p->p_pid, l->l_lid));

	if (LIST_EMPTY(&sa->sa_lwpcache)) {
		/* Allocate the next cache LWP */
		DPRINTFN(6,("sa_yieldcall(%d.%d) allocating LWP\n",
		    p->p_pid, l->l_lid));
		sa_newcachelwp(l);
	}

	sa_yield(l);
	upcallret(l);
}
#endif

static int
sa_newcachelwp(struct lwp *l)
{
	struct proc *p;
	struct lwp *l2;
	vaddr_t uaddr;
	boolean_t inmem;
	int s;

	p = l->l_proc;

	inmem = uvm_uarea_alloc(&uaddr);
	if (__predict_false(uaddr == 0)) {
		return (ENOMEM);
	} else {
		newlwp(l, p, uaddr, inmem, 0, NULL, 0, child_return, 0, &l2);
		/* We don't want this LWP on the process's main LWP list, but
		 * newlwp helpfully puts it there. Unclear if newlwp should
		 * be tweaked.
		 */
		SCHED_LOCK(s);
		sa_putcachelwp(p, l2);
		SCHED_UNLOCK(s);
	}

	return (0);
}

/*
 * Take a normal process LWP and place it in the SA cache.
 * LWP must not be running!
 */
void
sa_putcachelwp(struct proc *p, struct lwp *l)
{
	struct sadata *sa;

	SCHED_ASSERT_LOCKED();

	sa = p->p_sa;

	LIST_REMOVE(l, l_sibling);
	p->p_nlwps--;
	l->l_stat = LSSUSPENDED;
	l->l_flag |= (L_DETACHED | L_SA);
	PHOLD(l);
	/* XXX lock sadata */
	DPRINTFN(5,("sa_putcachelwp(%d.%d) Adding LWP %d to cache\n",
	    p->p_pid, curlwp->l_lid, l->l_lid));
	LIST_INSERT_HEAD(&sa->sa_lwpcache, l, l_sibling);
	sa->sa_ncached++;
	/* XXX unlock */
}

/*
 * Fetch a LWP from the cache.
 */
struct lwp *
sa_getcachelwp(struct proc *p)
{
	struct sadata *sa;
	struct lwp *l;

	SCHED_ASSERT_LOCKED();

	l = NULL;
	sa = p->p_sa;
	/* XXX lock sadata */
	if (sa->sa_ncached > 0) {
		sa->sa_ncached--;
		l = LIST_FIRST(&sa->sa_lwpcache);
		LIST_REMOVE(l, l_sibling);
		LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);
		p->p_nlwps++;
		DPRINTFN(5,("sa_getcachelwp(%d.%d) Got LWP %d from cache.\n",
		    p->p_pid, curlwp->l_lid, l->l_lid));
	}
	/* XXX unlock */
	return l;
}


void
sa_upcall_userret(struct lwp *l)
{
	struct proc *p;
	struct sadata *sa;
	struct sa_t **sapp, *sap;
	struct sadata_upcall *sau;
	struct sa_t self_sa;
	struct sa_t *sas[3];
	stack_t st;
	void *stack, *ap;
	ucontext_t u, *up;
	int i, nsas, nint, nevents, type;
	int s;
	int sig;

	p = l->l_proc;
	sa = p->p_sa;
	
	SCHED_ASSERT_UNLOCKED();

	KERNEL_PROC_LOCK(l);
	SA_LWP_STATE_LOCK(l,s);

	DPRINTFN(7,("sa_upcall_userret(%d.%d %x) \n", p->p_pid, l->l_lid,
	    l->l_flag));

	if (l->l_flag & L_SA_BLOCKING) {
		/* Invoke an "unblocked" upcall */
		struct lwp *l2;
		DPRINTFN(8,("sa_upcall_userret(%d.%d) unblocking\n",
		    p->p_pid, l->l_lid));


		sau = sadata_upcall_alloc(1);
		sau->sau_arg = NULL;

		if (p->p_flag & P_WEXIT) {
			sadata_upcall_free(sau);
			lwp_exit(l);
		}

	
		SCHED_ASSERT_UNLOCKED();

		l2 = sa_vp_repossess(l);

		KDASSERT(sa->sa_nstacks > 0);
		st = sa->sa_stacks[--sa->sa_nstacks];

		
		SCHED_ASSERT_UNLOCKED();
			
		if (l2 == NULL) {
			sadata_upcall_free(sau);
			/* No need to put st back */
			lwp_exit(l);
		}

		DPRINTFN(9,("sa_upcall_userret(%d.%d) nstacks--   = %2d\n",
		    l->l_proc->p_pid, l->l_lid, sa->sa_nstacks));
		if (sa_upcall0(l, SA_UPCALL_UNBLOCKED | SA_UPCALL_DEFER, l, l2, 0, NULL, sau,
		    &st) != 0) {
			/*
			 * We were supposed to deliver an UNBLOCKED
			 * upcall, but don't have resources to do so.
			 */
#ifdef DIAGNOSTIC
			printf("sa_upcall_userret: out of upcall resources"
			    " for %d.%d\n", p->p_pid, l->l_lid);
#endif
			sigexit(l, SIGABRT);
			/* NOTREACHED */
		}
		l->l_flag &= ~L_SA_BLOCKING;

		/* We migth have sneaked past signal handling and userret */
		SA_LWP_STATE_UNLOCK(l,s);
		KERNEL_PROC_UNLOCK(l);
		/* take pending signals */
		while ((sig = CURSIG(l)) != 0)
			postsig(sig);

		/* Invoke per-process kernel-exit handling, if any */
		if (p->p_userret)
			(p->p_userret)(l, p->p_userret_arg);

		KERNEL_PROC_LOCK(l);
		SA_LWP_STATE_LOCK(l,s);



	}

	if (SIMPLEQ_EMPTY(&sa->sa_upcalls))
	{		

		l->l_flag &= ~L_SA_UPCALL;

		sa_vp_donate(l);	
		
		SA_LWP_STATE_UNLOCK(l,s);
		KERNEL_PROC_UNLOCK(l);
		return;
	}	

	sau = SIMPLEQ_FIRST(&sa->sa_upcalls);
	SIMPLEQ_REMOVE_HEAD(&sa->sa_upcalls, sau_next);
	
	if (sau->sau_flags & SAU_FLAG_DEFERRED) {
		sa_upcall_getstate(sau,
		    sau->sau_state.deferred.e_lwp,
		    sau->sau_state.deferred.i_lwp);
	}

	stack = (void *)
	    (((uintptr_t)sau->sau_stack.ss_sp + sau->sau_stack.ss_size)
		& ~ALIGNBYTES);

	self_sa.sa_id = l->l_lid;
	self_sa.sa_cpu = 0; /* XXX l->l_cpu; */
	sas[0] = &self_sa;
	nsas = 1;
	nevents = 0;
	nint = 0;
	if (sau->sau_state.captured.e_sa.sa_context != NULL) {
		if (copyout(&sau->sau_state.captured.e_ctx,
		    sau->sau_state.captured.e_sa.sa_context,
		    sizeof(ucontext_t)) != 0) {
#ifdef DIAGNOSTIC
			printf("sa_upcall_userret(%d.%d): couldn't copyout"
			    " context of event LWP %d\n",
			    p->p_pid, l->l_lid, sau->sau_state.captured.e_sa.sa_id);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		sas[nsas] = &sau->sau_state.captured.e_sa;
		nsas++;
		nevents = 1;
	}
	if (sau->sau_state.captured.i_sa.sa_context != NULL) {
		KDASSERT(sau->sau_state.captured.i_sa.sa_context !=
		    sau->sau_state.captured.e_sa.sa_context);
		if (copyout(&sau->sau_state.captured.i_ctx,
		    sau->sau_state.captured.i_sa.sa_context,
		    sizeof(ucontext_t)) != 0) {
#ifdef DIAGNOSTIC
			printf("sa_upcall_userret(%d.%d): couldn't copyout"
			    " context of interrupted LWP %d\n",
			    p->p_pid, l->l_lid, sau->sau_state.captured.i_sa.sa_id);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		sas[nsas] = &sau->sau_state.captured.i_sa;
		nsas++;
		nint = 1;
	}

	/* Copy out the activation's ucontext */
	u.uc_stack = sau->sau_stack;
	u.uc_flags = _UC_STACK;
	up = stack;
	up--;
	if (copyout(&u, up, sizeof(ucontext_t)) != 0) {
		sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
		printf("sa_upcall_userret: couldn't copyout activation"
		    " ucontext for %d.%d\n", l->l_proc->p_pid, l->l_lid);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
	sas[0]->sa_context = up;

	/* Next, copy out the sa_t's and pointers to them. */
	sap = (struct sa_t *) up;
	sapp = (struct sa_t **) (sap - nsas);
	for (i = nsas - 1; i >= 0; i--) {
		sap--;
		sapp--;
		if ((copyout(sas[i], sap, sizeof(struct sa_t)) != 0) ||
		    (copyout(&sap, sapp, sizeof(struct sa_t *)) != 0)) {
			/* Copying onto the stack didn't work. Die. */
			sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
			printf("sa_upcall_userret: couldn't copyout sa_t "
			    "%d for %d.%d\n", i, p->p_pid, l->l_lid);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	}

	/* Copy out the arg, if any */
	/* xxx assume alignment works out; everything so far has been
	 * a structure, so...
	 */
	if (sau->sau_arg) {
		ap = (char *)sapp - sau->sau_argsize;
		stack = ap;
		if (copyout(sau->sau_arg, ap, sau->sau_argsize) != 0) {
			/* Copying onto the stack didn't work. Die. */
			sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
			printf("sa_upcall_userret(%d.%d): couldn't copyout"
			    " sadata_upcall arg %p size %ld to %p \n",
			    p->p_pid, l->l_lid,
			    sau->sau_arg, (long) sau->sau_argsize, ap);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	} else {
		ap = 0;
		stack = sapp;
	}

	type = sau->sau_type;

	sadata_upcall_free(sau);
	DPRINTFN(7,("sa_upcall_userret(%d.%d): type %d\n",p->p_pid,
	    l->l_lid, type));

	cpu_upcall(l, type, nevents, nint, sapp, ap, stack, sa->sa_upcall);

	if (SIMPLEQ_EMPTY(&sa->sa_upcalls)) 
	{
		l->l_flag &= ~L_SA_UPCALL;
		sa_vp_donate(l);
		/* May not be reached  */
	}


	/* May not be reached  */
	
	SA_LWP_STATE_UNLOCK(l,s);
	KERNEL_PROC_UNLOCK(l);
}

#if 0
static struct lwp *
sa_vp_repossess(struct lwp *l)
{
	struct lwp *l2;
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	int s;

	/*
	 * Put ourselves on the virtual processor and note that the
	 * previous occupant of that position was interrupted.
	 */




	l2 = sa->sa_vp;
	sa->sa_vp = l;
	if (sa->sa_idle == l2)
		sa->sa_idle = NULL;

	KDASSERT(l2 != l);
	if (l2) {
		SCHED_LOCK(s);
		switch (l2->l_stat) {
		case LSRUN:
			remrunqueue(l2);
			p->p_nrlwps--;
			break;
		case LSSLEEP:
			unsleep(l2);
			l2->l_flag &= ~L_SINTR;
			break;
#ifdef DIAGNOSTIC
		default:
			panic("SA VP %d.%d is in state %d, not running"
			    " or sleeping\n", p->p_pid, l2->l_lid,
			    l2->l_stat);
#endif
		}
		sa_putcachelwp(p, l2);
		/*
		 * XXX SMP race! Need to be sure that l2's state is
		 * captured before the upcall before we make it possible
		 * for another processor to grab it.
		 */
		SCHED_UNLOCK(s);
	}
	return l2;
}
#endif

static struct lwp *
sa_vp_repossess(struct lwp *l)
{
	struct lwp *l2;
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	int s;
		
	SCHED_ASSERT_UNLOCKED();

	l->l_flag |= L_SA_WANTS_VP;
	sa->sa_vp_wait_count++;

	if(sa->sa_idle != NULL) {
		/* XXXUPSXXX Simple but slow */
		wakeup(sa->sa_idle);
	} else {
		SCHED_LOCK(s);
		sa->sa_vp->l_flag |= L_SA_UPCALL;
		/* kick the process */
		signotify(p);
		SCHED_UNLOCK(s);
	}

	SCHED_ASSERT_UNLOCKED();

	while(sa->sa_vp != l) {
		tsleep((caddr_t) l, PWAIT, "sa processor", 0);
		
		/* XXXUPSXXX NEED TO STOP THE LWP HERE ON REQUEST ??? */
	       	if (p->p_flag & P_WEXIT) {
			l->l_flag &= ~L_SA_WANTS_VP;
			sa->sa_vp_wait_count--;
			return 0;
		}
	}

	l2 = sa->sa_old_lwp;

	return l2;
}

static void
sa_vp_donate(struct lwp *l)
{
	
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;	
	struct lwp *l2;
	int s;

	SCHED_ASSERT_UNLOCKED();

	/* Nobody wants the vp */
	if (sa->sa_vp_wait_count == 0)
		return;

	/* No stack for an unblock call */
	if (sa->sa_nstacks == 0)
		return;

	LIST_FOREACH(l2, &p->p_lwps, l_sibling) {
		if(l2->l_flag &  L_SA_WANTS_VP) {
			SCHED_LOCK(s);
			
			sa_putcachelwp(p, l);
			sa->sa_vp = l2;
			sa->sa_vp_wait_count--;
			l2->l_flag &= ~L_SA_WANTS_VP;
			sa->sa_old_lwp = l;
			
			sched_wakeup((caddr_t) l2);	

			KERNEL_PROC_UNLOCK(l);


			if((l2->l_stat == LSRUN) && ((l2->l_flag & L_INMEM) != 0))
				mi_switch(l,l2);
			else
				mi_switch(l,NULL);
	
			/*
			 * This isn't quite a NOTREACHED; we may get here if
			 * the process exits before this LWP is reused. In
			 * that case, we want to call lwp_exit(), which will
			 * be done by the userret() hooks.
			 */
			SCHED_ASSERT_UNLOCKED();
			splx(s);

			KERNEL_PROC_LOCK(l);

			KDASSERT(p->p_flag & P_WEXIT);
			/* mostly NOTREACHED */

			lwp_exit(l);
		}
	}
	
#ifdef DIAGNOSTIC
	printf("sa_vp_donate couldn't find someone to donate the CPU to \n");
#endif	
}



#ifdef DEBUG
int debug_print_sa(struct proc *);
int debug_print_lwp(struct lwp *);
int debug_print_proc(int);

int
debug_print_proc(int pid)
{
	struct proc *p;

	p = pfind(pid);
	if (p == NULL)
		printf("No process %d\n", pid);
	else
		debug_print_sa(p);

	return 0;
}

int
debug_print_sa(struct proc *p)
{
	struct lwp *l;
	struct sadata *sa;

	printf("Process %d (%s), state %d, address %p, flags %x\n",
	    p->p_pid, p->p_comm, p->p_stat, p, p->p_flag);
	printf("LWPs: %d (%d running, %d zombies)\n",
	    p->p_nlwps, p->p_nrlwps, p->p_nzlwps);
	LIST_FOREACH(l, &p->p_lwps, l_sibling)
	    debug_print_lwp(l);
	sa = p->p_sa;
	if (sa) {
		if (sa->sa_vp)
			printf("SA VP: %d\n", sa->sa_vp->l_lid);
		if (sa->sa_idle)
			printf("SA idle: %d\n", sa->sa_idle->l_lid);
		printf("SAs: %d cached LWPs\n", sa->sa_ncached);
		printf("%d upcall stacks\n", sa->sa_nstacks);
		LIST_FOREACH(l, &sa->sa_lwpcache, l_sibling)
		    debug_print_lwp(l);
	}

	return 0;
}

int
debug_print_lwp(struct lwp *l)
{
	struct proc *p;

	p = l->l_proc;
	printf("LWP %d address %p ", l->l_lid, l);
	printf("state %d flags %x ", l->l_stat, l->l_flag);
	if (l->l_wchan)
		printf("wait %p %s", l->l_wchan, l->l_wmesg);
	printf("\n");

	return 0;
}

#endif
