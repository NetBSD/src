/*	$NetBSD: kern_sa.c,v 1.1.2.22 2002/06/20 23:53:06 thorpej Exp $	*/

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
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/sa.h>
#include <sys/savar.h>

#include <uvm/uvm_extern.h>

static int sa_newcachelwp(struct lwp *);

#define SA_DEBUG

#ifdef SA_DEBUG
#define DPRINTF(x)	do { if (sadebug) printf x; } while (0)
#define DPRINTFN(n,x)	do { if (sadebug & (1<<(n-1))) printf x; } while (0)
int	sadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif


/*
 * sadata_upcall_alloc:
 *
 *	Allocate an sadata_upcall structure.
 */
struct sadata_upcall *
sadata_upcall_alloc(int waitok)
{

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
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sadata *s;
	sa_upcall_t prev;
	int error;

	if (p->p_sa == NULL) {
		/* Allocate scheduler activations data structure */
		s = pool_get(&sadata_pool, PR_WAITOK);
		/* Initialize. */
		memset(s, 0, sizeof(*s));
		simple_lock_init(&s->sa_lock);
		s->sa_vp = NULL;
		s->sa_concurrency = 1;
		s->sa_stacks = malloc(sizeof(stack_t) * SA_NUMSTACKS,
		    M_SA, M_WAITOK);
		s->sa_nstackentries = SA_NUMSTACKS;
		LIST_INIT(&s->sa_lwpcache);
		SIMPLEQ_INIT(&s->sa_upcalls);
		p->p_sa = s;
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
	struct sadata *s = l->l_proc->p_sa;
	int error, count;

	/* We have to be using scheduler activations */
	if (s == NULL)
		return (EINVAL);

	count = SCARG(uap, num);
	if (count < 0)
		return (EINVAL);
	count = min(count, s->sa_nstackentries - s->sa_nstacks);

	error = copyin(SCARG(uap, stacks), s->sa_stacks + s->sa_nstacks,
	    sizeof(stack_t) * count);
	if (error)
		return (error);
	s->sa_nstacks += count;
	
	*retval = count;
	return (0);
}


int
sys_sa_enable(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sadata *s = p->p_sa;
	int error;

	DPRINTF(("sys_sa_enable(pid: %d lid: %d)\n", l->l_proc->p_pid, l->l_lid));

	/* We have to be using scheduler activations */
	if (s == NULL)
		return (EINVAL);
	
	if (p->p_flag & P_SA) /* Already running! */
		return (EBUSY);

	error = sa_upcall(l, SA_UPCALL_NEWPROC, l, NULL, 0, NULL);
	if (error)
		return (error);

	p->p_flag |= P_SA;
	l->l_flag |= L_SA; /* We are now an activation LWP */

	/* Assign this LWP to the virtual processor */
	s->sa_vp = l;

	/* This will not return to the place in user space it came from. */
	return (0);
}


int
sys_sa_setconcurrency(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_setconcurrency_args /* {
		syscallarg(int) concurrency;
	} */ *uap = v;
	struct sadata *s = l->l_proc->p_sa;

	DPRINTF(("sys_sa_concurrency(pid: %d lid: %d)\n", l->l_proc->p_pid, l->l_lid));

	/* We have to be using scheduler activations */
	if (s == NULL)
		return (EINVAL);

	if (SCARG(uap, concurrency) < 1)
		return (EINVAL);

	*retval = s->sa_concurrency;
	/*
	 * Concurrency greater than the number of physical CPUs does 
	 * not make sense. 
	 * XXX Should we ever support hot-plug CPUs, this will need 
	 * adjustment.
	 */
	s->sa_concurrency = min(SCARG(uap, concurrency), 1 /* XXX ncpus */);
	    
	return (0);
}


int
sys_sa_yield(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sadata *s = p->p_sa;

	DPRINTFN(1,("sa_yield(%d.%d)\n", p->p_pid, l->l_lid));

	if (s == NULL || !(p->p_flag & P_SA))
		return (EINVAL);

	/* We are no longer using this VP */
	s->sa_vp = NULL;

	/* Don't let a process sa_yield() itself into oblivion. */
	if ((p->p_nlwps - p->p_nzlwps) == 1) {
		DPRINTFN(1,("sa_yield(%d.%d) going dormant\n", 
		    p->p_pid, l->l_lid));
		/* A signal will probably wake us up. Worst case, the upcall
		 * happens and just causes the process to yield again.
		 */
		l->l_flag &= ~L_SA;
		tsleep((caddr_t) p, PUSER | PCATCH, "sawait", 0);
		l->l_flag |= L_SA;
		return (0);
	}
	lwp_exit(l);

	return (0);
}


int
sys_sa_preempt(struct lwp *l, void *v, register_t *retval)
{

	/* XXX Implement me. */
	return (ENOSYS);
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
	struct sadata_upcall *s;

	l->l_flag &= ~L_SA; /* XXX prevent recursive upcalls if we sleep for 
			      memory */
	s = sadata_upcall_alloc(1);
	l->l_flag |= L_SA;

	return (sa_upcall0(l, type, event, interrupted, argsize, arg, s));
}

int
sa_upcall0(struct lwp *l, int type, struct lwp *event, struct lwp *interrupted,
    size_t argsize, void *arg, struct sadata_upcall *s)
{
	struct proc *p = l->l_proc;
	struct sadata *sd = p->p_sa;

	KDASSERT((event == NULL) || (event != interrupted));

	s->sau_type = type;
	s->sau_argsize = argsize;
	s->sau_arg = arg;
	s->sau_event.sa_context = NULL;
	s->sau_interrupted.sa_context = NULL;

	/* Grab a stack */
	if (!sd->sa_nstacks) {
		sadata_upcall_free(s);
		return (ENOMEM);
	}
	s->sau_stack = sd->sa_stacks[--sd->sa_nstacks];

	if (event) {
		getucontext(event, &s->sau_e_ctx);
		s->sau_event.sa_context = (ucontext_t *)
		    (_UC_MACHINE_SP(&s->sau_e_ctx) - sizeof(ucontext_t));
		s->sau_event.sa_id = event->l_lid;
		s->sau_event.sa_cpu = 0; /* XXX extract from l_cpu */
	}
	if (interrupted) {
		getucontext(interrupted, &s->sau_i_ctx);
		s->sau_interrupted.sa_context = (ucontext_t *)
		    (_UC_MACHINE_SP(&s->sau_i_ctx) - sizeof(ucontext_t));
		s->sau_interrupted.sa_id = interrupted->l_lid;
		s->sau_interrupted.sa_cpu = 0; /* XXX extract from l_cpu */
	}

	SIMPLEQ_INSERT_TAIL(&sd->sa_upcalls, s, sau_next);
	l->l_flag |= L_SA_UPCALL;
	
	return (0);
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
	struct sadata *s = p->p_sa;
	struct sadata_upcall *sd;
	struct lwp *l2;
	int error;

	DPRINTFN(4,("sa_switch(type: %d pid: %d.%d)\n", type, p->p_pid, l->l_lid));
	SCHED_ASSERT_LOCKED();

	if (l->l_flag & L_SA_BLOCKING) {
		/* We've already sent a BLOCKED upcall, but the LWP
		 * has been woken up and put to sleep again without
		 * returning to userland. We don't want to send a
		 * second BLOCKED upcall.
		 *
		 * Instead, simply let the LWP that was running before
		 * we woke up have another go.
		 */
		l2 = s->sa_vp;
	} else {
		/* Get an LWP */
		/* The process of allocating a new LWP could cause
		 * sleeps. We're called from inside sleep, so that
		 * would be Bad. Therefore, we must use a cached new
		 * LWP. The first thing that this new LWP must do is
		 * allocate another LWP for the cache.  
		 */
		l2 = sa_getcachelwp(p);
		if (l2 == NULL) {
			/* XXXSMP */
			/* No upcall for you! */
			/* XXX The consequences of this are more subtle and
			 * XXX the recovery from this situation deserves
			 * XXX more thought.
			 */
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): out of upcall resources\n",
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

		sd = sadata_upcall_alloc(0);
		if (sd == NULL)
			goto sa_upcall_failed;

		cpu_setfunc(l2, sa_switchcall, NULL);
		error = sa_upcall0(l2, SA_UPCALL_BLOCKED, l, NULL, 0, NULL, 
		    sd);
		if (error) {
		sa_upcall_failed:
			/* Put the lwp back */
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): Error %d from sa_upcall()\n",
			    p->p_pid, l->l_lid, error);
#endif
			sa_putcachelwp(p, l2);
			mi_switch(l, NULL);
			return;
		}
		
		l->l_flag |= L_SA_BLOCKING;
		l2->l_priority = l2->l_usrpri;
		setrunnable(l2);
		PRELE(l2); /* Remove the artificial hold-count */
		
		KDASSERT(l2 != l);
		KDASSERT(l2->l_wchan == 0);
	}

	DPRINTFN(4,("sa_switch(%d.%d) switching to ", p->p_pid, l->l_lid));

	if (l2)
		DPRINTFN(4,("LWP %d(%x).\n", l2->l_lid, l2->l_flag));
	else
		DPRINTFN(4,("NULL\n"));

	s->sa_vp = l2;
	mi_switch(l, l2);

	DPRINTFN(4,("sa_switch(%d.%d) returned.\n", p->p_pid, l->l_lid));
	KDASSERT(l->l_wchan == 0);

	SCHED_ASSERT_UNLOCKED();

	/*
	 * Okay, now we've been woken up. This means that it's time
	 * for a SA_UNBLOCKED upcall when we get back to userlevel. 
	 */

	/* 
	 * ... unless we're trying to exit. In this case, the last thing
	 * we want to do is put something back on the cache list.
	 * It's also not useful to make the upcall at all, so just punt.
	 */

	if (p->p_flag & P_WEXIT)
		return;

	l->l_flag |= L_SA_UPCALL;
}

void
sa_switchcall(void *arg)
{
	struct lwp *l;
	struct proc *p;
	struct sadata *sa;

	l = curproc;
	p = l->l_proc;
	sa = p->p_sa;
	DPRINTFN(6,("sa_switchcall(pid: %d.%d)\n", p->p_pid, l->l_lid));

	if (LIST_EMPTY(&sa->sa_lwpcache)) {
		/* Allocate the next cache LWP */
		sa_newcachelwp(l);
	}
	upcallret(l);
}

static int
sa_newcachelwp(struct lwp *l)
{
	struct proc *p;
	struct sadata *sa;
	struct lwp *l2;
	vaddr_t uaddr;
	int s;

	p = l->l_proc;
	sa = p->p_sa;

	uaddr = uvm_km_valloc(kernel_map, USPACE);
	if (__predict_false(uaddr == 0)) {
		return (ENOMEM);
	} else {
		newlwp(l, p, uaddr, 0, NULL, NULL, child_return, 0, &l2);
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
	DPRINTFN(5,("sa_addcachelwp(%d) Adding LWP %d to cache\n",
	    p->p_pid, l->l_lid));
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
		DPRINTFN(5,("sa_getcachelwp(%d) Got LWP %d from cache.\n",
		    p->p_pid,l->l_lid));

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
	void *stack, *ap;
	ucontext_t u, *up;
	int i, nsas, nint, nevents, type;

	p = l->l_proc;
	sa = p->p_sa;

	DPRINTFN(7,("sa_upcall_userret(%d.%d)\n",p->p_pid,l->l_lid));

	if (l->l_flag & L_SA_BLOCKING) {
		/* Invoke an "unblocked" upcall */
		struct lwp *l2;
		int s;
		DPRINTFN(8,("sa_upcall_userret(%d.%d) unblocking ",p->p_pid, l->l_lid));
		/* Put ourselves on the virtual processor and note that the
		 * previous occupant of that position was interrupted.
		 */
		l2 = sa->sa_vp;
		sa->sa_vp = l;

		if (l2) {
			SCHED_LOCK(s);
			remrunqueue(l2);
			p->p_nrlwps--;
			sa_putcachelwp(p, l2);
			SCHED_UNLOCK(s);
		}
		if (sa_upcall(l, SA_UPCALL_UNBLOCKED, l, l2, 0, NULL) != 0) {
			/* We were supposed to deliver an UNBLOCKED
			 * upcall, but don't have resources to do so.
			 */
#ifdef DIAGNOSTIC
			printf("sa_upcall_userret: out of upcall resources"
			    " for %d.%d\n", p->p_pid, l->l_lid);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}	
		l->l_flag &= ~L_SA_BLOCKING;
	}

	KDASSERT(SIMPLEQ_EMPTY(&sa->sa_upcalls) == 0);

	sau = SIMPLEQ_FIRST(&sa->sa_upcalls);
	SIMPLEQ_REMOVE_HEAD(&sa->sa_upcalls, sau_next);
	if (SIMPLEQ_EMPTY(&sa->sa_upcalls))
		l->l_flag &= ~L_SA_UPCALL;

	stack = (void *) 
	    (((uintptr_t)sau->sau_stack.ss_sp + sau->sau_stack.ss_size) 
		& ~ALIGNBYTES);

	self_sa.sa_id = l->l_lid;
	self_sa.sa_cpu = 0; /* XXX l->l_cpu; */
	sas[0] = &self_sa;
	nsas = 1;
	nevents = 0;
	nint = 0;
	if (sau->sau_event.sa_context != NULL) {
		if (copyout(&sau->sau_e_ctx, sau->sau_event.sa_context, 
		    sizeof(ucontext_t)) != 0) {
#ifdef DIAGNOSTIC
		printf("sa_upcall_userret(%d.%d): couldn't copyout"
		    " context of event LWP %d\n",
		    p->p_pid, l->l_lid, sau->sau_event.sa_id);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
		}
		sas[nsas] = &sau->sau_event;
		nsas++;
		nevents = 1;
	}
	if (sau->sau_interrupted.sa_context != NULL) {
		KDASSERT(sau->sau_interrupted.sa_context !=
		    sau->sau_event.sa_context);
		if (copyout(&sau->sau_i_ctx, sau->sau_interrupted.sa_context, 
		    sizeof(ucontext_t)) != 0) {
#ifdef DIAGNOSTIC
		printf("sa_upcall_userret(%d.%d): couldn't copyout"
		    " context of interrupted LWP %d\n",
		    p->p_pid, l->l_lid, sau->sau_interrupted.sa_id);
#endif
		sigexit(l, SIGILL);
		/* NOTREACHED */
		}
		sas[nsas] = &sau->sau_interrupted;
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
		printf("SAs: %d cached LWPs\n", sa->sa_ncached);
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
