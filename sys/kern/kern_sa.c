/*	$NetBSD: kern_sa.c,v 1.1.2.5 2001/08/24 04:20:08 nathanw Exp $	*/

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
#define DPRINTF(x)	if (sadebug) printf x
#define DPRINTFN(n,x)	if (sadebug>(n)) printf x
int	sadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif


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
		s->sa_concurrency = 1;
		s->sa_stacks = malloc(sizeof(stack_t) * SA_NUMSTACKS,
		    M_SA, M_WAITOK);
		s->sa_nstackentries = SA_NUMSTACKS;
		LIST_INIT(&s->sa_lwpcache);
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

	error = sa_upcall(l, SA_UPCALL_NEWPROC, l, NULL, 0, 0, NULL);
	if (error)
		return (error);

	p->p_flag |= P_SA;
	l->l_flag |= L_SA; /* We are now an activation LWP */

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

	DPRINTF(("sa_yield(pid: %d.%d)\n", p->p_pid, l->l_lid));

	if (s == NULL || !(p->p_flag & P_SA))
		return (EINVAL);
	
	/* Don't let a process sa_yield() itself into oblivion. */
	if ((p->p_nlwps - p->p_nzlwps) == 1)
		sigexit(l, SIGABRT);

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
 */

int
sa_upcall(struct lwp *l, int type, struct lwp *event, struct lwp *interrupted,
	int sig, u_long code, void *arg)
{
	struct proc *p = l->l_proc;
	struct sadata *sd = p->p_sa;
	struct sadata_upcall *s;

	l->l_flag &= ~L_SA; /* XXX prevent recursive upcalls if we sleep for 
			      memory */
	s = pool_get(&saupcall_pool, PR_WAITOK);
	l->l_flag |= L_SA;

	/* Grab a stack */
	if (!sd->sa_nstacks)
		return (ENOMEM);
	s->sau_stack = sd->sa_stacks[--sd->sa_nstacks];

	s->sau_type = type;
	s->sau_sig = sig;
	s->sau_code = code;
	s->sau_arg = arg;
	s->sau_event = event;
	s->sau_interrupted = interrupted;

	LIST_INSERT_HEAD(&sd->sa_upcalls, s, sau_next);
	l->l_flag |= L_SA_UPCALL;
	
	return (0);
}


/* 
 * Called by tsleep(). Block current LWP and switch to another.
 */
void
sa_switch(struct lwp *l, int type)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	struct lwp *l2;
	int s, error;

	DPRINTF(("sa_switch(type: %d pid: %d.%d)\n", type, p->p_pid, l->l_lid));

	/* Get an LWP */
	/* The process of allocating a new LWP could cause
         * sleeps. We're called from inside sleep, so that would be
         * Bad. Therefore, we must use a cached new LWP. The first
         * thing that this new LWP must do is allocate another LWP for
         * the cache.
	 */
	if (sa->sa_ncached == 0) {
		/* XXXSMP */
		/* No upcall for you! */
		/* XXX The consequences of this are more subtle and
		 * XXX the recovery from this situation deserves
		 * XXX more thought.
		 */
		mi_switch(l, NULL);
		return;
	}

	/* XXX lock sadata */
	sa->sa_ncached--;
	l2 = LIST_FIRST(&sa->sa_lwpcache);
	LIST_REMOVE(l2, l_sibling);
	/* XXX unlock */

	cpu_setfunc(l2, sa_switchcall, l);
	error = sa_upcall(l2, SA_UPCALL_BLOCKED, l, NULL, 0, 0, NULL);
	if (error) {
		/* Put the lwp back */
		/* XXX lock sadata */
		LIST_INSERT_HEAD(&sa->sa_lwpcache, l2, l_sibling);
		sa->sa_ncached++;
		/* XXX unlock */
		mi_switch(l, NULL);
		return;
	}
	LIST_INSERT_HEAD(&p->p_lwps, l2, l_sibling);
	p->p_nlwps++;
	p->p_nrlwps++;

	SCHED_LOCK(s);
	l2->l_priority = l2->l_usrpri;
	setrunnable(l2);
	PRELE(l2); /* Remove the artificial hold-count */
	SCHED_UNLOCK(s);

	KDASSERT(l2 != l);
	KDASSERT(l2->l_wchan == 0);

	DPRINTF(("sa_switch: pid %d switching from %d to %d.\n", 
	    p->p_pid, l->l_lid, l2->l_lid));

	mi_switch(l, l2);

	DPRINTF(("sa_switch: pid %d returned to %d.\n", p->p_pid,
	    l->l_lid));
	KDASSERT(l->l_wchan == 0);

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

	/* Find the interrupted LWP */
	LIST_FOREACH(l2, &p->p_lwps, l_sibling)
	    if (l2->l_stat == LSRUN)
		    break;
	if (l2) {
		SCHED_LOCK(s);
		remrunqueue(l2);
		l2->l_stat = LSSUSPENDED;
		SCHED_UNLOCK(s);
		p->p_nlwps--;
		p->p_nrlwps--;
		LIST_REMOVE(l2, l_sibling);
		PHOLD(l2);
		/* XXX lock sadata */
		LIST_INSERT_HEAD(&sa->sa_lwpcache, l2, l_sibling);
		sa->sa_ncached++;
		/* XXX unlock */
	}

	sa_upcall(l, SA_UPCALL_UNBLOCKED, l, l2, 0, 0, NULL);
}

void
sa_switchcall(void *arg)
{
	struct lwp *l = curproc;
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;

	DPRINTF(("sa_switchcall(pid: %d.%d)\n", p->p_pid, l->l_lid));

	if (LIST_EMPTY(&sa->sa_lwpcache)) {
		/* Allocate the next cache LWP */
		sa_newcachelwp(l);
	}
	upcallret(NULL);
}

static int
sa_newcachelwp(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;

	vaddr_t uaddr;
	struct lwp *l2;

	uaddr = uvm_km_valloc(kernel_map, USPACE);
	if (__predict_false(uaddr == 0)) {
		return (ENOMEM);
	} else {
		newlwp(l, p, uaddr, 0, NULL, NULL, child_return, 0, &l2);
		/* We don't want this LWP on the process's main LWP list, but
		 * newlwp helpfully puts it there. Unclear if newlwp should
		 * be tweaked.
		 */
		LIST_REMOVE(l2, l_sibling);
		p->p_nlwps--;
		l2->l_stat = LSSUSPENDED;
		l2->l_flag |= (L_DETACHED | L_SA);
		PHOLD(l2);
		/* XXX lock sadata */	
		LIST_INSERT_HEAD(&sa->sa_lwpcache, l2, l_sibling);
		sa->sa_ncached++;
		/* XXX unlock */
	}

	return (0);
}
