/*	$NetBSD: compat_sa.c,v 1.11 2009/09/13 18:45:10 pooka Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, and by Andrew Doran.
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

#include "opt_ktrace.h"
#include "opt_multiprocessor.h"
#include "opt_sa.h"
__KERNEL_RCSID(0, "$NetBSD: compat_sa.c,v 1.11 2009/09/13 18:45:10 pooka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>
#include <sys/sched.h>
#include <sys/sleepq.h>
#include <sys/atomic.h> /* for membar_producer() */

#include <uvm/uvm_extern.h>

/*
 * Now handle building with SA diabled. We always compile this file,
 * just if SA's disabled we merely build in stub routines for call
 * entry points we still need.
 */
#ifdef KERN_SA

/*
 * SA_CONCURRENCY is buggy can lead to kernel crashes.
 */
#ifdef SA_CONCURRENCY
#ifndef MULTIPROCESSOR
	#error "SA_CONCURRENCY is only valid on MULTIPROCESSOR kernels"
#endif
#endif

/*
 * memory pool for sadata structures
 */
static struct pool sadata_pool;

/*
 * memory pool for pending upcalls
 */
static struct pool saupcall_pool;

/*
 * memory pool for sastack structs
 */
static struct pool sastack_pool;

/*
 * memory pool for sadata_vp structures
 */
static struct pool savp_pool;

static struct sadata_vp *sa_newsavp(struct proc *);
static void sa_freevp(struct proc *, struct sadata *, struct sadata_vp *);
static inline int sa_stackused(struct sastack *, struct sadata *);
static inline void sa_setstackfree(struct sastack *, struct sadata *);
static struct sastack *sa_getstack(struct sadata *);
static inline struct sastack *sa_getstack0(struct sadata *);
static inline int sast_compare(struct sastack *, struct sastack *);
#ifdef SA_CONCURRENCY
static int sa_increaseconcurrency(struct lwp *, int);
#endif
static void sa_switchcall(void *);
static void sa_neverrun(void *);
static int sa_newcachelwp(struct lwp *, struct sadata_vp *);
static void sa_makeupcalls(struct lwp *, struct sadata_upcall *);

static inline int sa_pagefault(struct lwp *, ucontext_t *);

static void sa_upcall0(struct sadata_upcall *, int, struct lwp *, struct lwp *,
    size_t, void *, void (*)(void *));
static void sa_upcall_getstate(union sau_state *, struct lwp *, int);

void	sa_putcachelwp(struct proc *, struct lwp *);
struct lwp *sa_getcachelwp(struct proc *, struct sadata_vp *);
static void	sa_setrunning(struct lwp *);

#define SA_DEBUG

#ifdef SA_DEBUG
#define DPRINTF(x)	do { if (sadebug) printf_nolog x; } while (0)
#define DPRINTFN(n,x)	do { if (sadebug & (1<<(n-1))) printf_nolog x; } while (0)
int	sadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

static syncobj_t sa_sobj = {
	SOBJ_SLEEPQ_FIFO,
	sleepq_unsleep,
	sleepq_changepri,
	sleepq_lendpri,
	syncobj_noowner,
};

static const char *sa_lwpcache_wmesg = "lwpcache";
static const char *sa_lwpwoken_wmesg = "lwpublk";

#define SA_LWP_STATE_LOCK(l, f) do {				\
	(f) = ~(l)->l_pflag & LP_SA_NOBLOCK;     		\
	(l)->l_pflag |= LP_SA_NOBLOCK;				\
} while (/*CONSTCOND*/ 0)

#define SA_LWP_STATE_UNLOCK(l, f) do {				\
	(l)->l_pflag ^= (f);					\
} while (/*CONSTCOND*/ 0)

RB_PROTOTYPE(sasttree, sastack, sast_node, sast_compare);
RB_GENERATE(sasttree, sastack, sast_node, sast_compare);

kmutex_t	saupcall_mutex;
SIMPLEQ_HEAD(, sadata_upcall) saupcall_freelist;

void
sa_init(void)
{

	pool_init(&sadata_pool, sizeof(struct sadata), 0, 0, 0, "sadatapl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&saupcall_pool, sizeof(struct sadata_upcall), 0, 0, 0,
	    "saupcpl", &pool_allocator_nointr, IPL_NONE);
	pool_init(&sastack_pool, sizeof(struct sastack), 0, 0, 0, "sastackpl",
	    &pool_allocator_nointr, IPL_NONE);
	pool_init(&savp_pool, sizeof(struct sadata_vp), 0, 0, 0, "savppl",
	    &pool_allocator_nointr, IPL_NONE);
}

/*
 * sa_critpath API
 * permit other parts of the kernel to make SA_LWP_STATE_{UN,}LOCK calls.
 */
void
sa_critpath_enter(struct lwp *l1, sa_critpath_t *f1)
{
	SA_LWP_STATE_LOCK(l1, *f1);
}
void
sa_critpath_exit(struct lwp *l1, sa_critpath_t *f1)
{
	SA_LWP_STATE_UNLOCK(l1, *f1);
}


/*
 * sadata_upcall_alloc:
 *
 *	Allocate an sadata_upcall structure.
 */
struct sadata_upcall *
sadata_upcall_alloc(int waitok)
{
	struct sadata_upcall *sau;

	sau = NULL;
	if (waitok && !SIMPLEQ_EMPTY(&saupcall_freelist)) {
		mutex_enter(&saupcall_mutex);
		if ((sau = SIMPLEQ_FIRST(&saupcall_freelist)) != NULL)
			SIMPLEQ_REMOVE_HEAD(&saupcall_freelist, sau_next);
		mutex_exit(&saupcall_mutex);
		if (sau != NULL && sau->sau_arg != NULL)
			(*sau->sau_argfreefunc)(sau->sau_arg);
	}

	if (sau == NULL)
		sau = pool_get(&saupcall_pool, waitok ? PR_WAITOK : PR_NOWAIT);
	if (sau != NULL)
		sau->sau_arg = NULL;

	return sau;
}

/*
 * sadata_upcall_free:
 *
 *	Free an sadata_upcall structure and any associated argument data.
 */
void
sadata_upcall_free(struct sadata_upcall *sau)
{
	if (sau == NULL)
		return;

	/*
	 * If our current synchronisation object is a sleep queue or
	 * similar, we must not put the object back to the pool as
	 * doing to could acquire sleep locks.  That could trigger
	 * a recursive sleep.
	 */
	if (curlwp->l_syncobj == &sched_syncobj) {
		if (sau->sau_arg)
			(*sau->sau_argfreefunc)(sau->sau_arg);
		pool_put(&saupcall_pool, sau);
		sadata_upcall_drain();
	} else {
		mutex_enter(&saupcall_mutex);
		SIMPLEQ_INSERT_HEAD(&saupcall_freelist, sau, sau_next);
		mutex_exit(&saupcall_mutex);
	}
}

/*
 * sadata_upcall_drain:
 *
 *	Put freed upcall structures back to the pool.
 */
void
sadata_upcall_drain(void)
{
	struct sadata_upcall *sau;

	sau = SIMPLEQ_FIRST(&saupcall_freelist);
	while (sau != NULL) {
		mutex_enter(&saupcall_mutex);
		if ((sau = SIMPLEQ_FIRST(&saupcall_freelist)) != NULL)
			SIMPLEQ_REMOVE_HEAD(&saupcall_freelist, sau_next);
		mutex_exit(&saupcall_mutex);
		if (sau != NULL) /* XXX sau_arg free needs a call! */
			pool_put(&saupcall_pool, sau);
	}
}

/*
 * sa_newsavp
 *
 * Allocate a new virtual processor structure, do some simple
 * initialization and add it to the passed-in sa. Pre-allocate
 * an upcall event data structure for when the main thread on
 * this vp blocks.
 *
 * We lock ??? while manipulating the list of vp's.
 *
 * We allocate the lwp to run on this separately. In the case of the
 * first lwp/vp for a process, the lwp already exists. It's the
 * main (only) lwp of the process.
 */
static struct sadata_vp *
sa_newsavp(struct proc *p)
{
	struct sadata *sa = p->p_sa;
	struct sadata_vp *vp, *qvp;
	struct sadata_upcall *sau;

	/* Allocate virtual processor data structure */
	vp = pool_get(&savp_pool, PR_WAITOK);
	/* And preallocate an upcall data structure for sleeping */
	sau = sadata_upcall_alloc(1);
	/* Initialize. */
	memset(vp, 0, sizeof(*vp));
	/* Lock has to be IPL_SCHED, since we use it in the
	 * hooks from the scheduler code */
	vp->savp_lwp = NULL;
	vp->savp_faultaddr = 0;
	vp->savp_ofaultaddr = 0;
	vp->savp_woken_count = 0;
	vp->savp_lwpcache_count = 0;
	vp->savp_pflags = 0;
	vp->savp_sleeper_upcall = sau;
	mutex_init(&vp->savp_mutex, MUTEX_DEFAULT, IPL_SCHED);
	sleepq_init(&vp->savp_lwpcache);
	sleepq_init(&vp->savp_woken);
	SIMPLEQ_INIT(&vp->savp_upcalls);

	/* We're writing sa_savps, so lock both locks */
	mutex_enter(p->p_lock);
	mutex_enter(&sa->sa_mutex);
	/* find first free savp_id and add vp to sorted slist */
	if (SLIST_EMPTY(&sa->sa_vps) ||
	    SLIST_FIRST(&sa->sa_vps)->savp_id != 0) {
		vp->savp_id = 0;
		SLIST_INSERT_HEAD(&sa->sa_vps, vp, savp_next);
	} else {
		SLIST_FOREACH(qvp, &sa->sa_vps, savp_next) {
			if (SLIST_NEXT(qvp, savp_next) == NULL ||
			    SLIST_NEXT(qvp, savp_next)->savp_id !=
			    qvp->savp_id + 1)
				break;
		}
		vp->savp_id = qvp->savp_id + 1;
		SLIST_INSERT_AFTER(qvp, vp, savp_next);
	}
	mutex_exit(&sa->sa_mutex);
	mutex_exit(p->p_lock);

	DPRINTFN(1, ("sa_newsavp(%d) allocated vp %p\n", p->p_pid, vp));

	return (vp);
}

/*
 * sa_freevp:
 *
 *	Deallocate a vp. Must be called with no locks held.
 * Will lock and unlock p_lock.
 */
static void
sa_freevp(struct proc *p, struct sadata *sa, struct sadata_vp *vp)
{
	DPRINTFN(1, ("sa_freevp(%d) freeing vp %p\n", p->p_pid, vp));

	mutex_enter(p->p_lock);

	DPRINTFN(1, ("sa_freevp(%d) about to unlink in vp %p\n", p->p_pid, vp));
	SLIST_REMOVE(&sa->sa_vps, vp, sadata_vp, savp_next);
	DPRINTFN(1, ("sa_freevp(%d) done unlink in vp %p\n", p->p_pid, vp));

	if (vp->savp_sleeper_upcall) {
		sadata_upcall_free(vp->savp_sleeper_upcall);
		vp->savp_sleeper_upcall = NULL;
	}
	DPRINTFN(1, ("sa_freevp(%d) about to mut_det in vp %p\n", p->p_pid, vp));

	mutex_destroy(&vp->savp_mutex);

	mutex_exit(p->p_lock);

	pool_put(&savp_pool, vp);
}

/*
 *
 */
int sa_system_disabled = 1;

/*
 * sys_sa_register
 *	Handle copyin and copyout of info for registering the
 * upcall handler address.
 */
int
sys_sa_register(struct lwp *l, const struct sys_sa_register_args *uap,
    register_t *retval)
{
	int error;
	sa_upcall_t prev;

	error = dosa_register(l, SCARG(uap, new), &prev, SCARG(uap, flags),
	    SCARG(uap, stackinfo_offset));
	if (error)
		return error;

	if (SCARG(uap, old))
		return copyout(&prev, SCARG(uap, old),
		    sizeof(prev));
	return 0;
}

/*
 * dosa_register
 *
 *	Change the upcall address for the process. If needed, allocate
 * an sadata structure (and initialize it) for the process. If initializing,
 * set the flags in the sadata structure to those passed in. Flags will
 * be ignored if the sadata structure already exists (dosa_regiister was
 * already called).
 *
 * Note: changing the upcall handler address for a process that has
 * concurrency greater than one can yield ambiguous results. The one
 * guarantee we can offer is that any upcalls generated on all CPUs
 * after this routine finishes will use the new upcall handler. Note
 * that any upcalls delivered upon return to user level by the
 * sys_sa_register() system call that called this routine will use the
 * new upcall handler. Note that any such upcalls will be delivered
 * before the old upcall handling address has been returned to
 * the application.
 */
int
dosa_register(struct lwp *l, sa_upcall_t new, sa_upcall_t *prev, int flags,
    ssize_t stackinfo_offset)
{
	struct proc *p = l->l_proc;
	struct sadata *sa;

	if (sa_system_disabled)
		return EINVAL;

	if (p->p_sa == NULL) {
		/* Allocate scheduler activations data structure */
		sa = pool_get(&sadata_pool, PR_WAITOK);
		memset(sa, 0, sizeof(*sa));

		/* WRS: not sure if need SCHED. need to audit lockers */
		mutex_init(&sa->sa_mutex, MUTEX_DEFAULT, IPL_SCHED);
		mutex_enter(p->p_lock);
		if ((p->p_sflag & PS_NOSA) != 0) {
			mutex_exit(p->p_lock);
			mutex_destroy(&sa->sa_mutex);
			pool_put(&sadata_pool, sa);
			return EINVAL;
		}

		/* Initialize. */
		sa->sa_flag = flags & SA_FLAG_ALL;
		sa->sa_maxconcurrency = 1;
		sa->sa_concurrency = 1;
		RB_INIT(&sa->sa_stackstree);
		sa->sa_stacknext = NULL;
		if (flags & SA_FLAG_STACKINFO)
			sa->sa_stackinfo_offset = stackinfo_offset;
		else
			sa->sa_stackinfo_offset = 0;
		sa->sa_nstacks = 0;
		sigemptyset(&sa->sa_sigmask);
		sigplusset(&l->l_sigmask, &sa->sa_sigmask);
		sigemptyset(&l->l_sigmask);
		SLIST_INIT(&sa->sa_vps);
		cv_init(&sa->sa_cv, "sawait");
		membar_producer();
		p->p_sa = sa;
		KASSERT(l->l_savp == NULL);
		mutex_exit(p->p_lock);
	}
	if (l->l_savp == NULL) {	/* XXXSMP */
		l->l_savp = sa_newsavp(p);
		sa_newcachelwp(l, NULL);
	}

	*prev = p->p_sa->sa_upcall;
	p->p_sa->sa_upcall = new;

	return (0);
}

void
sa_release(struct proc *p)
{
	struct sadata *sa;
	struct sastack *sast, *next;
	struct sadata_vp *vp;
	struct lwp *l;

	sa = p->p_sa;
	KASSERT(sa != NULL);
	KASSERT(p->p_nlwps <= 1);

	for (sast = RB_MIN(sasttree, &sa->sa_stackstree); sast != NULL;
	     sast = next) {
		next = RB_NEXT(sasttree, &sa->sa_stackstree, sast);
		RB_REMOVE(sasttree, &sa->sa_stackstree, sast);
		pool_put(&sastack_pool, sast);
	}

	mutex_enter(p->p_lock);
	p->p_sflag = (p->p_sflag & ~PS_SA) | PS_NOSA;
	p->p_sa = NULL;
	l = LIST_FIRST(&p->p_lwps);
	if (l) {
		lwp_lock(l);
		KASSERT(LIST_NEXT(l, l_sibling) == NULL);
		l->l_savp = NULL;
		lwp_unlock(l);
	}
	mutex_exit(p->p_lock);

	while ((vp = SLIST_FIRST(&sa->sa_vps)) != NULL) {
		sa_freevp(p, sa, vp);
	}

	DPRINTFN(1, ("sa_release(%d) done vps\n", p->p_pid));

	mutex_destroy(&sa->sa_mutex);
	cv_destroy(&sa->sa_cv);
	pool_put(&sadata_pool, sa);

	DPRINTFN(1, ("sa_release(%d) put sa\n", p->p_pid));

	mutex_enter(p->p_lock);
	p->p_sflag &= ~PS_NOSA;
	mutex_exit(p->p_lock);
}

/*
 * sa_fetchstackgen
 *
 *	copyin the generation number for the stack in question.
 *
 * WRS: I think this routine needs the SA_LWP_STATE_LOCK() dance, either
 * here or in its caller.
 *
 * Must be called with sa_mutex locked.
 */
static int
sa_fetchstackgen(struct sastack *sast, struct sadata *sa, unsigned int *gen)
{
	int error;

	/* COMPAT_NETBSD32:  believe it or not, but the following is ok */
	mutex_exit(&sa->sa_mutex);
	error = copyin(&((struct sa_stackinfo_t *)
	    ((char *)sast->sast_stack.ss_sp +
	    sa->sa_stackinfo_offset))->sasi_stackgen, gen, sizeof(*gen));
	mutex_enter(&sa->sa_mutex);

	return error;
}

/*
 * sa_stackused
 *
 *	Convenience routine to determine if a given stack has been used
 * or not. We consider a stack to be unused if the kernel's concept
 * of its generation number matches that of userland.
 *	We kill the application with SIGILL if there is an error copying
 * in the userland generation number.
 */
static inline int
sa_stackused(struct sastack *sast, struct sadata *sa)
{
	unsigned int gen;

	KASSERT(mutex_owned(&sa->sa_mutex));

	if (sa_fetchstackgen(sast, sa, &gen)) {
		sigexit(curlwp, SIGILL);
		/* NOTREACHED */
	}
	return (sast->sast_gen != gen);
}

/*
 * sa_setstackfree
 *
 *	Convenience routine to mark a stack as unused in the kernel's
 * eyes. We do this by setting the kernel's generation number for the stack
 * to that of userland.
 *	We kill the application with SIGILL if there is an error copying
 * in the userland generation number.
 */
static inline void
sa_setstackfree(struct sastack *sast, struct sadata *sa)
{
	unsigned int gen;

	KASSERT(mutex_owned(&sa->sa_mutex));

	if (sa_fetchstackgen(sast, sa, &gen)) {
		sigexit(curlwp, SIGILL);
		/* NOTREACHED */
	}
	sast->sast_gen = gen;
}

/*
 * sa_getstack
 *
 * Find next free stack, starting at sa->sa_stacknext.  Must be called
 * with sa->sa_mutex held, and will release while checking for stack
 * availability.
 *
 * Caller should have set LP_SA_NOBLOCK for our thread. This is not the time
 * to go generating upcalls as we aren't in a position to deliver another one.
 */
static struct sastack *
sa_getstack(struct sadata *sa)
{
	struct sastack *sast;
	int chg;

	KASSERT(mutex_owned(&sa->sa_mutex));

	do {
		chg = sa->sa_stackchg;
		sast = sa->sa_stacknext;
		if (sast == NULL || sa_stackused(sast, sa))
			sast = sa_getstack0(sa);
	} while (chg != sa->sa_stackchg);

	if (sast == NULL)
		return NULL;

	sast->sast_gen++;
	sa->sa_stackchg++;

	return sast;
}

/*
 * sa_getstack0 -- get the lowest numbered sa stack
 *
 *	We walk the splay tree in order and find the lowest-numbered
 * (as defined by SPLAY_MIN() and SPLAY_NEXT() ordering) stack that
 * is unused.
 */
static inline struct sastack *
sa_getstack0(struct sadata *sa)
{
	struct sastack *start;
	int chg;

	KASSERT(mutex_owned(&sa->sa_mutex));

 retry:
	chg = sa->sa_stackchg;
	if (sa->sa_stacknext == NULL) {
		sa->sa_stacknext = RB_MIN(sasttree, &sa->sa_stackstree);
		if (sa->sa_stacknext == NULL)
			return NULL;
	}
	start = sa->sa_stacknext;

	while (sa_stackused(sa->sa_stacknext, sa)) {
		if (sa->sa_stackchg != chg)
			goto retry;
		sa->sa_stacknext = RB_NEXT(sasttree, &sa->sa_stackstree,
		    sa->sa_stacknext);
		if (sa->sa_stacknext == NULL)
			sa->sa_stacknext = RB_MIN(sasttree,
			    &sa->sa_stackstree);
		if (sa->sa_stacknext == start)
			return NULL;
	}
	return sa->sa_stacknext;
}

/*
 * sast_compare - compare two sastacks
 *
 *	We sort stacks according to their userspace addresses.
 * Stacks are "equal" if their start + size overlap.
 */
static inline int
sast_compare(struct sastack *a, struct sastack *b)
{

	if ((vaddr_t)a->sast_stack.ss_sp + a->sast_stack.ss_size <=
	    (vaddr_t)b->sast_stack.ss_sp)
		return (-1);
	if ((vaddr_t)a->sast_stack.ss_sp >=
	    (vaddr_t)b->sast_stack.ss_sp + b->sast_stack.ss_size)
		return (1);
	return (0);
}

/*
 * sa_copyin_stack -- copyin a stack.
 */
static int
sa_copyin_stack(stack_t *stacks, int index, stack_t *dest)
{
	return copyin(stacks + index, dest, sizeof(stack_t));
}

/*
 * sys_sa_stacks -- the user level threading library is passing us stacks
 *
 * We copy in some arguments then call sa_stacks1() to do the main
 * work. NETBSD32 has its own front-end for this call.
 */
int
sys_sa_stacks(struct lwp *l, const struct sys_sa_stacks_args *uap,
	register_t *retval)
{
	return sa_stacks1(l, retval, SCARG(uap, num), SCARG(uap, stacks),
	    sa_copyin_stack);
}

/*
 * sa_stacks1
 *	Process stacks passed-in by the user threading library. At
 * present we use the kernel lock to lock the SPLAY tree, which we
 * manipulate to load in the stacks.
 *
 * 	It is an error to pass in a stack that we already know about
 * and which hasn't been used. Passing in a known-but-used one is fine.
 * We accept up to SA_MAXNUMSTACKS per desired vp (concurrency level).
 */
int
sa_stacks1(struct lwp *l, register_t *retval, int num, stack_t *stacks,
    sa_copyin_stack_t do_sa_copyin_stack)
{
	struct sadata *sa = l->l_proc->p_sa;
	struct sastack *sast, *new;
	int count, error, f, i, chg;

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	count = num;
	if (count < 0)
		return (EINVAL);

	SA_LWP_STATE_LOCK(l, f);

	error = 0;

	for (i = 0; i < count; i++) {
		new = pool_get(&sastack_pool, PR_WAITOK);
		error = do_sa_copyin_stack(stacks, i, &new->sast_stack);
		if (error) {
			count = i;
			break;
		}
		mutex_enter(&sa->sa_mutex);
	 restart:
	 	chg = sa->sa_stackchg;
		sa_setstackfree(new, sa);
		sast = RB_FIND(sasttree, &sa->sa_stackstree, new);
		if (sast != NULL) {
			DPRINTFN(9, ("sa_stacks(%d.%d) returning stack %p\n",
				     l->l_proc->p_pid, l->l_lid,
				     new->sast_stack.ss_sp));
			if (sa_stackused(sast, sa) == 0) {
				count = i;
				error = EEXIST;
				mutex_exit(&sa->sa_mutex);
				pool_put(&sastack_pool, new);
				break;
			}
			if (chg != sa->sa_stackchg)
				goto restart;
		} else if (sa->sa_nstacks >=
		    SA_MAXNUMSTACKS * sa->sa_concurrency) {
			DPRINTFN(9,
			    ("sa_stacks(%d.%d) already using %d stacks\n",
			    l->l_proc->p_pid, l->l_lid,
			    SA_MAXNUMSTACKS * sa->sa_concurrency));
			count = i;
			error = ENOMEM;
			mutex_exit(&sa->sa_mutex);
			pool_put(&sastack_pool, new);
			break;
		} else {
			DPRINTFN(9, ("sa_stacks(%d.%d) adding stack %p\n",
				     l->l_proc->p_pid, l->l_lid,
				     new->sast_stack.ss_sp));
			RB_INSERT(sasttree, &sa->sa_stackstree, new);
			sa->sa_nstacks++;
			sa->sa_stackchg++;
		}
		mutex_exit(&sa->sa_mutex);
	}

	SA_LWP_STATE_UNLOCK(l, f);

	*retval = count;
	return (error);
}


/*
 * sys_sa_enable - throw the switch & enable SA
 *
 * Fairly simple. Make sure the sadata and vp've been set up for this
 * process, assign this thread to the vp and initiate the first upcall
 * (SA_UPCALL_NEWPROC).
 */
int
sys_sa_enable(struct lwp *l, const void *v, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	struct sadata_vp *vp = l->l_savp;
	int error;

	DPRINTF(("sys_sa_enable(%d.%d)\n", l->l_proc->p_pid,
	    l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL || vp == NULL)
		return (EINVAL);

	if (p->p_sflag & PS_SA) /* Already running! */
		return (EBUSY);

	error = sa_upcall(l, SA_UPCALL_NEWPROC, l, NULL, 0, NULL, NULL);
	if (error)
		return (error);

	/* Assign this LWP to the virtual processor */
	mutex_enter(p->p_lock);
	vp->savp_lwp = l;
	p->p_sflag |= PS_SA;
	lwp_lock(l);
	l->l_flag |= LW_SA; /* We are now an activation LWP */
	lwp_unlock(l);
	mutex_exit(p->p_lock);

	/*
	 * This will return to the SA handler previously registered.
	 */
	return (0);
}


/*
 * sa_increaseconcurrency
 *	Raise the process's maximum concurrency level to the
 * requested level. Does nothing if the current maximum councurrency
 * is greater than the requested.
 *	Must be called with sa_mutex locked. Will unlock and relock as
 * needed, and will lock p_lock. Will exit with sa_mutex locked.
 */
#ifdef SA_CONCURRENCY

static int
sa_increaseconcurrency(struct lwp *l, int concurrency)
{
	struct proc *p;
	struct lwp *l2;
	struct sadata *sa;
	struct sadata_vp *vp;
	struct sadata_upcall *sau;
	int addedconcurrency, error;

	p = l->l_proc;
	sa = p->p_sa;

	KASSERT(mutex_owned(&sa->sa_mutex));

	addedconcurrency = 0;
	while (sa->sa_maxconcurrency < concurrency) {
		sa->sa_maxconcurrency++;
		sa->sa_concurrency++;
		mutex_exit(&sa->sa_mutex);

		vp = sa_newsavp(p);
		error = sa_newcachelwp(l, vp);
		if (error) {
			/* reset concurrency */
			mutex_enter(&sa->sa_mutex);
			sa->sa_maxconcurrency--;
			sa->sa_concurrency--;
			return (addedconcurrency);
		}
		mutex_enter(&vp->savp_mutex);
		l2 = sa_getcachelwp(p, vp);
		vp->savp_lwp = l2;

		sau = vp->savp_sleeper_upcall;
		vp->savp_sleeper_upcall = NULL;
		KASSERT(sau != NULL);

		cpu_setfunc(l2, sa_switchcall, sau);
		sa_upcall0(sau, SA_UPCALL_NEWPROC, NULL, NULL,
		    0, NULL, NULL);

		if (error) {
			/* put l2 into l's VP LWP cache */
			mutex_exit(&vp->savp_mutex);
			lwp_lock(l2);
			l2->l_savp = l->l_savp;
			cpu_setfunc(l2, sa_neverrun, NULL);
			lwp_unlock(l2);
			mutex_enter(&l->l_savp->savp_mutex);
			sa_putcachelwp(p, l2);
			mutex_exit(&l->l_savp->savp_mutex);

			/* Free new savp */
			sa_freevp(p, sa, vp);

			/* reset concurrency */
			mutex_enter(&sa->sa_mutex);
			sa->sa_maxconcurrency--;
			sa->sa_concurrency--;
			return (addedconcurrency);
		}
		/* Run the LWP, locked since its mutex is still savp_mutex */
		sa_setrunning(l2);
		uvm_lwp_rele(l2);
		mutex_exit(&vp->savp_mutex);

		mutex_enter(&sa->sa_mutex);
		addedconcurrency++;
	}

	return (addedconcurrency);
}
#endif

/*
 * sys_sa_setconcurrency
 *	The user threading library wants to increase the number
 * of active virtual CPUS we assign to it. We return the number of virt
 * CPUs we assigned to the process. We limit concurrency to the number
 * of CPUs in the system.
 *
 * WRS: at present, this system call serves two purposes. The first is
 * for an application to indicate that it wants a certain concurrency
 * level. The second is for the application to request that the kernel
 * reactivate previously allocated virtual CPUs.
 */
int
sys_sa_setconcurrency(struct lwp *l, const struct sys_sa_setconcurrency_args *uap,
	register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
#ifdef SA_CONCURRENCY
	struct sadata_vp *vp = l->l_savp;
	struct lwp *l2;
	int ncpus;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
#endif

	DPRINTFN(11,("sys_sa_concurrency(%d.%d)\n", p->p_pid,
		     l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	if ((p->p_sflag & PS_SA) == 0)
		return (EINVAL);

	if (SCARG(uap, concurrency) < 1)
		return (EINVAL);

	*retval = 0;
	/*
	 * Concurrency greater than the number of physical CPUs does
	 * not make sense.
	 * XXX Should we ever support hot-plug CPUs, this will need
	 * adjustment.
	 */
#ifdef SA_CONCURRENCY
	mutex_enter(&sa->sa_mutex);

	if (SCARG(uap, concurrency) > sa->sa_maxconcurrency) {
		ncpus = 0;
		for (CPU_INFO_FOREACH(cii, ci))
			ncpus++;
		*retval += sa_increaseconcurrency(l,
		    min(SCARG(uap, concurrency), ncpus));
	}
#endif

	DPRINTFN(11,("sys_sa_concurrency(%d.%d) want %d, have %d, max %d\n",
		     p->p_pid, l->l_lid, SCARG(uap, concurrency),
		     sa->sa_concurrency, sa->sa_maxconcurrency));
#ifdef SA_CONCURRENCY
	if (SCARG(uap, concurrency) <= sa->sa_concurrency) {
		mutex_exit(&sa->sa_mutex);
		return 0;
	}
	SLIST_FOREACH(vp, &sa->sa_vps, savp_next) {
		l2 = vp->savp_lwp;
		lwp_lock(l2);
		if (l2->l_flag & LW_SA_IDLE) {
			l2->l_flag &= ~(LW_SA_IDLE|LW_SA_YIELD|LW_SINTR);
			lwp_unlock(l2);
			DPRINTFN(11,("sys_sa_concurrency(%d.%d) NEWPROC vp %d\n",
				     p->p_pid, l->l_lid, vp->savp_id));
			sa->sa_concurrency++;
			mutex_exit(&sa->sa_mutex);
			/* error = */ sa_upcall(l2, SA_UPCALL_NEWPROC, NULL,
			    NULL, 0, NULL, NULL);
			lwp_lock(l2);
			/* lwp_unsleep() will unlock the LWP */
			lwp_unsleep(vp->savp_lwp, true);
			KASSERT((l2->l_flag & LW_SINTR) == 0);
			(*retval)++;
			mutex_enter(&sa->sa_mutex);
		} else
			lwp_unlock(l2);
		if (sa->sa_concurrency == SCARG(uap, concurrency))
			break;
	}
	mutex_exit(&sa->sa_mutex);
#endif
	return 0;
}

/*
 * sys_sa_yield
 *	application has nothing for this lwp to do, so let it linger in
 * the kernel.
 */
int
sys_sa_yield(struct lwp *l, const void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	mutex_enter(p->p_lock);
	if (p->p_sa == NULL || !(p->p_sflag & PS_SA)) {
		mutex_exit(p->p_lock);
		DPRINTFN(2,
		    ("sys_sa_yield(%d.%d) proc %p not SA (p_sa %p, flag %s)\n",
		    p->p_pid, l->l_lid, p, p->p_sa,
		    p->p_sflag & PS_SA ? "T" : "F"));
		return (EINVAL);
	}

	mutex_exit(p->p_lock);

	sa_yield(l);

	return (EJUSTRETURN);
}

/*
 * sa_yield
 *	This lwp has nothing to do, so hang around. Assuming we
 * are the lwp "on" our vp, sleep in "sawait" until there's something
 * to do.
 *
 *	Unfortunately some subsystems can't directly tell us if there's an
 * upcall going to happen when we get worken up. Work gets deferred to
 * userret() and that work may trigger an upcall. So we have to try
 * calling userret() (by calling upcallret()) and see if makeupcalls()
 * delivered an upcall. It will clear LW_SA_YIELD if it did.
 */
void
sa_yield(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	struct sadata_vp *vp = l->l_savp;
	int ret;

	lwp_lock(l);

	if (vp->savp_lwp != l) {
		lwp_unlock(l);

		/*
		 * We lost the VP on our way here, this happens for
		 * instance when we sleep in systrace.  This will end
		 * in an SA_UNBLOCKED_UPCALL in sa_unblock_userret().
		 */
		DPRINTFN(2,("sa_yield(%d.%d) lost VP\n",
			     p->p_pid, l->l_lid));
		KASSERT(l->l_flag & LW_SA_BLOCKING);
		return;
	}

	/*
	 * If we're the last running LWP, stick around to receive
	 * signals.
	 */
	KASSERT((l->l_flag & LW_SA_YIELD) == 0);
	DPRINTFN(2,("sa_yield(%d.%d) going dormant\n",
		     p->p_pid, l->l_lid));
	/*
	 * A signal will probably wake us up. Worst case, the upcall
	 * happens and just causes the process to yield again.
	 */
	KASSERT(vp->savp_lwp == l);

	/*
	 * If we were told to make an upcall or exit already
	 * make sure we process it (by returning and letting userret() do
	 * the right thing). Otherwise set LW_SA_YIELD and go to sleep.
	 */
	ret = 0;
	if (l->l_flag & LW_SA_UPCALL) {
		lwp_unlock(l);
		return;
	}
	l->l_flag |= LW_SA_YIELD;

	do {
		lwp_unlock(l);
		DPRINTFN(2,("sa_yield(%d.%d) really going dormant\n",
			     p->p_pid, l->l_lid));

		mutex_enter(&sa->sa_mutex);
		sa->sa_concurrency--;
		ret = cv_wait_sig(&sa->sa_cv, &sa->sa_mutex);
		sa->sa_concurrency++;
		mutex_exit(&sa->sa_mutex);
		DPRINTFN(2,("sa_yield(%d.%d) woke\n",
			     p->p_pid, l->l_lid));

		KASSERT(vp->savp_lwp == l || p->p_sflag & PS_WEXIT);

		/*
		 * We get woken in two different ways. Most code
		 * calls setrunnable() which clears LW_SA_IDLE,
		 * but leaves LW_SA_YIELD. Some call points
		 * (in this file) however also clear LW_SA_YIELD, mainly
		 * as the code knows there is an upcall to be delivered.
		 *
		 * As noted above, except in the cases where other code
		 * in this file cleared LW_SA_YIELD already, we have to
		 * try calling upcallret() & seeing if upcalls happen.
		 * if so, tell userret() NOT to deliver more upcalls on
		 * the way out!
		 */
		if (l->l_flag & LW_SA_YIELD) {
			upcallret(l);
			if (~l->l_flag & LW_SA_YIELD) {
				/*
				 * Ok, we made an upcall. We will exit. Tell
				 * sa_upcall_userret() to NOT make any more
				 * upcalls.
				 */
				vp->savp_pflags |= SAVP_FLAG_NOUPCALLS;
				/*
				 * Now force us to call into sa_upcall_userret()
				 * which will clear SAVP_FLAG_NOUPCALLS
				 */
				lwp_lock(l);
				l->l_flag |= LW_SA_UPCALL;
				lwp_unlock(l);
			}
		}

		lwp_lock(l);
	} while (l->l_flag & LW_SA_YIELD);

	DPRINTFN(2,("sa_yield(%d.%d) returned, ret %d\n",
		     p->p_pid, l->l_lid, ret));

	lwp_unlock(l);
}


/*
 * sys_sa_preempt - preempt a running thread
 *
 * Given an lwp id, send it a user upcall. This is a way for libpthread to
 * kick something into the upcall handler.
 */
int
sys_sa_preempt(struct lwp *l, const struct sys_sa_preempt_args *uap,
    register_t *retval)
{
/* Not yet ready */
#if 0
	struct proc		*p = l->l_proc;
	struct sadata		*sa = p->p_sa;
	struct lwp		*t;
	int			target, error;

	DPRINTFN(11,("sys_sa_preempt(%d.%d)\n", l->l_proc->p_pid,
		     l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	if ((p->p_sflag & PS_SA) == 0)
		return (EINVAL);

	if ((target = SCARG(uap, sa_id)) < 1)
		return (EINVAL);

	mutex_enter(p->p_lock);

	LIST_FOREACH(t, &l->l_proc->p_lwps, l_sibling)
		if (t->l_lid == target)
			break;

	if (t == NULL) {
		error = ESRCH;
		goto exit_lock;
	}

	/* XXX WRS We really need all of this locking documented */
	mutex_exit(p->p_lock);

	error = sa_upcall(l, SA_UPCALL_USER | SA_UPCALL_DEFER_EVENT, l, NULL,
		0, NULL, NULL);
	if (error)
		return error;

	return 0;

exit_lock:
	mutex_exit(p->p_lock);

	return error;
#else
	/* Just return an error */
	return (sys_nosys(l, (const void *)uap, retval));
#endif
}


/* XXX Hm, naming collision. */
/*
 * sa_preempt(). In the 4.0 code, this routine is called when we 
 * are in preempt() and the caller informed us it does NOT
 * have more work to do (it's going to userland after we return).
 * If mi_switch() tells us we switched to another thread, we
 * generate a BLOCKED upcall. Since we are returning to userland
 * we then will immediately generate an UNBLOCKED upcall as well.
 *	The only place that actually didn't tell preempt() that
 * we had more to do was sys_sched_yield() (well, midi did too, but
 * that was a bug).
 *
 * For simplicitly, in 5.0+ code, just call this routine in
 * sys_sched_yield after we preempt(). The BLOCKED/UNBLOCKED
 * upcall sequence will get delivered when we return to userland
 * and will ensure that the SA scheduler has an opportunity to
 * effectively preempt the thread that was running in userland.
 *
 * Of course, it would be simpler for libpthread to just intercept
 * this call, but we do this to ensure binary compatability. Plus
 * it's not hard to do.
 *
 * We are called and return with no locks held.
 */
void
sa_preempt(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;

	/*
	 * Defer saving the lwp's state because on some ports
	 * preemption can occur between generating an unblocked upcall
	 * and processing the upcall queue.
	 */
	if (sa->sa_flag & SA_FLAG_PREEMPT)
		sa_upcall(l, SA_UPCALL_PREEMPTED | SA_UPCALL_DEFER_EVENT,
		    l, NULL, 0, NULL, NULL);
}


/*
 * Set up the user-level stack and trapframe to do an upcall.
 *
 * NOTE: This routine WILL FREE "arg" in the case of failure!  Callers
 * should not touch the "arg" pointer once calling sa_upcall().
 */
int
sa_upcall(struct lwp *l, int type, struct lwp *event, struct lwp *interrupted,
	size_t argsize, void *arg, void (*func)(void *))
{
	struct sadata_upcall *sau;
	struct sadata *sa = l->l_proc->p_sa;
	struct sadata_vp *vp = l->l_savp;
	struct sastack *sast;
	int f, error;

	KASSERT((type & (SA_UPCALL_LOCKED_EVENT | SA_UPCALL_LOCKED_INTERRUPTED))
		== 0);

	/* XXX prevent recursive upcalls if we sleep for memory */
	SA_LWP_STATE_LOCK(curlwp, f);
	sau = sadata_upcall_alloc(1);
	mutex_enter(&sa->sa_mutex);
	sast = sa_getstack(sa);
	mutex_exit(&sa->sa_mutex);
	SA_LWP_STATE_UNLOCK(curlwp, f);

	if (sau == NULL || sast == NULL) {
		if (sast != NULL) {
			mutex_enter(&sa->sa_mutex);
			sa_setstackfree(sast, sa);
			mutex_exit(&sa->sa_mutex);
		}
		if (sau != NULL)
			sadata_upcall_free(sau);
		return (ENOMEM);
	}
	DPRINTFN(9,("sa_upcall(%d.%d) using stack %p\n",
	    l->l_proc->p_pid, l->l_lid, sast->sast_stack.ss_sp));

	if (l->l_proc->p_emul->e_sa->sae_upcallconv) {
		error = (*l->l_proc->p_emul->e_sa->sae_upcallconv)(l, type,
		    &argsize, &arg, &func);
		if (error) {
			mutex_enter(&sa->sa_mutex);
			sa_setstackfree(sast, sa);
			mutex_exit(&sa->sa_mutex);
			sadata_upcall_free(sau);
			return error;
		}
	}

	sa_upcall0(sau, type, event, interrupted, argsize, arg, func);
	sau->sau_stack = sast->sast_stack;
	mutex_enter(&vp->savp_mutex);
	SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);
	lwp_lock(l);
	l->l_flag |= LW_SA_UPCALL;
	lwp_unlock(l);
	mutex_exit(&vp->savp_mutex);

	return (0);
}

static void
sa_upcall0(struct sadata_upcall *sau, int type, struct lwp *event,
    struct lwp *interrupted, size_t argsize, void *arg, void (*func)(void *))
{
	DPRINTFN(12,("sa_upcall0: event %p interrupted %p type %x\n",
	    event, interrupted, type));

	KASSERT((event == NULL) || (event != interrupted));

	sau->sau_flags = 0;

	if (type & SA_UPCALL_DEFER_EVENT) {
		sau->sau_event.ss_deferred.ss_lwp = event;
		sau->sau_flags |= SAU_FLAG_DEFERRED_EVENT;
	} else
		sa_upcall_getstate(&sau->sau_event, event,
			type & SA_UPCALL_LOCKED_EVENT);
	if (type & SA_UPCALL_DEFER_INTERRUPTED) {
		sau->sau_interrupted.ss_deferred.ss_lwp = interrupted;
		sau->sau_flags |= SAU_FLAG_DEFERRED_INTERRUPTED;
	} else
		sa_upcall_getstate(&sau->sau_interrupted, interrupted,
			type & SA_UPCALL_LOCKED_INTERRUPTED);

	sau->sau_type = type & SA_UPCALL_TYPE_MASK;
	sau->sau_argsize = argsize;
	sau->sau_arg = arg;
	sau->sau_argfreefunc = func;
}

/*
 * sa_ucsp
 *	return the stack pointer (??) for a given context as
 * reported by the _UC_MACHINE_SP() macro.
 */
void *
sa_ucsp(void *arg)
{
	ucontext_t *uc = arg;

	return (void *)(uintptr_t)_UC_MACHINE_SP(uc);
}

/*
 * sa_upcall_getstate
 *	Fill in the given sau_state with info for the passed-in
 * lwp, and update the lwp accordingly.
 *	We set LW_SA_SWITCHING on the target lwp, and so we have to hold
 * l's lock in this call. l must be already locked, or it must be unlocked
 * and locking it must not cause deadlock.
 */
static void
sa_upcall_getstate(union sau_state *ss, struct lwp *l, int isLocked)
{
	uint8_t *sp;
	size_t ucsize;

	if (l) {
		if (isLocked == 0)
			lwp_lock(l);
		l->l_flag |= LW_SA_SWITCHING;
		if (isLocked == 0)
			lwp_unlock(l);
		(*l->l_proc->p_emul->e_sa->sae_getucontext)(l,
		    (void *)&ss->ss_captured.ss_ctx);
		if (isLocked == 0)
			lwp_lock(l);
		l->l_flag &= ~LW_SA_SWITCHING;
		if (isLocked == 0)
			lwp_unlock(l);
		sp = (*l->l_proc->p_emul->e_sa->sae_ucsp)
		    (&ss->ss_captured.ss_ctx);
		/* XXX COMPAT_NETBSD32: _UC_UCONTEXT_ALIGN */
		sp = STACK_ALIGN(sp, ~_UC_UCONTEXT_ALIGN);
		ucsize = roundup(l->l_proc->p_emul->e_sa->sae_ucsize,
		    (~_UC_UCONTEXT_ALIGN) + 1);
		ss->ss_captured.ss_sa.sa_context =
		    (ucontext_t *)STACK_ALLOC(sp, ucsize);
		ss->ss_captured.ss_sa.sa_id = l->l_lid;
		ss->ss_captured.ss_sa.sa_cpu = l->l_savp->savp_id;
	} else
		ss->ss_captured.ss_sa.sa_context = NULL;
}


/*
 * sa_pagefault
 *
 * Detect double pagefaults and pagefaults on upcalls.
 * - double pagefaults are detected by comparing the previous faultaddr
 *   against the current faultaddr
 * - pagefaults on upcalls are detected by checking if the userspace
 *   thread is running on an upcall stack
 */
static inline int
sa_pagefault(struct lwp *l, ucontext_t *l_ctx)
{
	struct proc *p;
	struct sadata *sa;
	struct sadata_vp *vp;
	struct sastack sast;
	int found;

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;

	KASSERT(mutex_owned(&sa->sa_mutex));
	KASSERT(vp->savp_lwp == l);

	if (vp->savp_faultaddr == vp->savp_ofaultaddr) {
		DPRINTFN(10,("sa_pagefault(%d.%d) double page fault\n",
			     p->p_pid, l->l_lid));
		return 1;
	}

	sast.sast_stack.ss_sp = (*p->p_emul->e_sa->sae_ucsp)(l_ctx);
	sast.sast_stack.ss_size = 1;
	found = (RB_FIND(sasttree, &sa->sa_stackstree, &sast) != NULL);

	if (found) {
		DPRINTFN(10,("sa_pagefault(%d.%d) upcall page fault\n",
			     p->p_pid, l->l_lid));
		return 1;
	}

	vp->savp_ofaultaddr = vp->savp_faultaddr;
	return 0;
}


/*
 * sa_switch
 *
 * Called by sleepq_block() when it wants to call mi_switch().
 * Block current LWP and switch to another.
 *
 * WE ARE NOT ALLOWED TO SLEEP HERE!  WE ARE CALLED FROM WITHIN
 * SLEEPQ_BLOCK() ITSELF!  We are called with sched_lock held, and must
 * hold it right through the mi_switch() call.
 *
 * We return with the scheduler unlocked.
 *
 * We are called in one of three conditions:
 *
 * 1:		We are an sa_yield thread. If there are any UNBLOCKED
 *	upcalls to deliver, deliver them (by exiting) instead of sleeping.
 * 2:		We are the main lwp (we're the lwp on our vp). Trigger
 *	delivery of a BLOCKED upcall.
 * 3:		We are not the main lwp on our vp. Chances are we got
 *	woken up but the sleeper turned around and went back to sleep.
 *	It seems that select and poll do this a lot. So just go back to sleep.
 */

void
sa_switch(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata_vp *vp = l->l_savp;
	struct sadata_upcall *sau = NULL;
	struct lwp *l2;

	KASSERT(lwp_locked(l, NULL));

	DPRINTFN(4,("sa_switch(%d.%d VP %d)\n", p->p_pid, l->l_lid,
	    vp->savp_lwp ? vp->savp_lwp->l_lid : 0));

	if ((l->l_flag & LW_WEXIT) || (p->p_sflag & (PS_WCORE | PS_WEXIT))) {
		mi_switch(l);
		return;
	}

	/*
	 * We need to hold two locks from here on out. Since you can
	 * sleepq_block() on ANY lock, there really can't be a locking
	 * hierarcy relative to savp_mutex. So if we can't get the mutex,
	 * drop the lwp lock, get the mutex, and carry on.
	 *
	 * Assumes the lwp lock can never be a sleeping mutex.
	 *
	 * We do however try hard to never not get savp_mutex. The only
	 * times we lock it are either when we are the blessed lwp for
	 * our vp, or when a blocked lwp is adding itself to the savp_worken
	 * list. So contention should be rare.
	 */
	if (!mutex_tryenter(&vp->savp_mutex)) {
		lwp_unlock(l);
		mutex_enter(&vp->savp_mutex);
		lwp_lock(l);
	}
	if (l->l_stat == LSONPROC) {
		/* Oops! We woke before we got to sleep. Ok, back we go! */
		lwp_unlock(l);
		mutex_exit(&vp->savp_mutex);
		return;
	}

	if (l->l_flag & LW_SA_YIELD) {
		/*
		 * Case 0: we're blocking in sa_yield
		 */
		DPRINTFN(4,("sa_switch(%d.%d) yield, flags %x pflag %x\n",
		    p->p_pid, l->l_lid, l->l_flag, l->l_pflag));
		if (vp->savp_woken_count == 0 && p->p_timerpend == 0) {
			DPRINTFN(4,("sa_switch(%d.%d) setting idle\n",
			    p->p_pid, l->l_lid));
			l->l_flag |= LW_SA_IDLE;
			mutex_exit(&vp->savp_mutex);
			mi_switch(l);
		} else {
			/*
			 * Make us running again. lwp_unsleep() will
			 * release the lock.
			 */
			mutex_exit(&vp->savp_mutex);
			lwp_unsleep(l, true);
		}
		return;
	}

	if (vp->savp_lwp == l) {
		if (vp->savp_pflags & SAVP_FLAG_DELIVERING) {
			/*
			 * We've exited sa_switchcall() but NOT
			 * made it into a new systemcall. Don't make
			 * a BLOCKED upcall.
			 */
			mutex_exit(&vp->savp_mutex);
			mi_switch(l);
			return;
		}
		/*
		 * Case 1: we're blocking for the first time; generate
		 * a SA_BLOCKED upcall and allocate resources for the
		 * UNBLOCKED upcall.
		 */
		if (vp->savp_sleeper_upcall) {
			sau = vp->savp_sleeper_upcall;
			vp->savp_sleeper_upcall = NULL;
		}

		if (sau == NULL) {
#ifdef DIAGNOSTIC
			printf("sa_switch(%d.%d): no upcall data.\n",
			    p->p_pid, l->l_lid);
#endif
			panic("Oops! Don't have a sleeper!\n");
			/* XXXWRS Shouldn't we just kill the app here? */
			mutex_exit(&vp->savp_mutex);
			mi_switch(l);
			return;
		}

		/*
		 * The process of allocating a new LWP could cause
		 * sleeps. We're called from inside sleep, so that
		 * would be Bad. Therefore, we must use a cached new
		 * LWP. The first thing that this new LWP must do is
		 * allocate another LWP for the cache.
		 */
		l2 = sa_getcachelwp(p, vp);
		if (l2 == NULL) {
			/* XXXSMP */
			/* No upcall for you! */
			/* XXX The consequences of this are more subtle and
			 * XXX the recovery from this situation deserves
			 * XXX more thought.
			 */

			/* XXXUPSXXX Should only happen with concurrency > 1 */
			mutex_exit(&vp->savp_mutex);
			mi_switch(l);
			sadata_upcall_free(sau);
			return;
		}

		cpu_setfunc(l2, sa_switchcall, sau);
		sa_upcall0(sau, SA_UPCALL_BLOCKED | SA_UPCALL_LOCKED_EVENT, l,
			NULL, 0, NULL, NULL);

		/*
		 * Perform the double/upcall pagefault check.
		 * We do this only here since we need l's ucontext to
		 * get l's userspace stack. sa_upcall0 above has saved
		 * it for us.
		 * The LP_SA_PAGEFAULT flag is set in the MD
		 * pagefault code to indicate a pagefault.  The MD
		 * pagefault code also saves the faultaddr for us.
		 *
		 * If the double check is true, turn this into a non-upcall
		 * block.
		 */
		if ((l->l_flag & LP_SA_PAGEFAULT) && sa_pagefault(l,
			&sau->sau_event.ss_captured.ss_ctx) != 0) {
			cpu_setfunc(l2, sa_neverrun, NULL);
			sa_putcachelwp(p, l2); /* uvm_lwp_hold from sa_getcachelwp */
			mutex_exit(&vp->savp_mutex);
			DPRINTFN(4,("sa_switch(%d.%d) Pagefault\n",
			    p->p_pid, l->l_lid));
			mi_switch(l);
			/*
			 * WRS Not sure how vp->savp_sleeper_upcall != NULL
			 * but be careful none the less
			 */
			if (vp->savp_sleeper_upcall == NULL)
				vp->savp_sleeper_upcall = sau;
			else
				sadata_upcall_free(sau);
			DPRINTFN(10,("sa_switch(%d.%d) page fault resolved\n",
				     p->p_pid, l->l_lid));
			mutex_enter(&vp->savp_mutex);
			if (vp->savp_faultaddr == vp->savp_ofaultaddr)
				vp->savp_ofaultaddr = -1;
			mutex_exit(&vp->savp_mutex);
			return;
		}

		DPRINTFN(8,("sa_switch(%d.%d) blocked upcall %d\n",
			     p->p_pid, l->l_lid, l2->l_lid));

		l->l_flag |= LW_SA_BLOCKING;
		vp->savp_blocker = l;
		vp->savp_lwp = l2;

		sa_setrunning(l2);

		/* Remove the artificial hold-count */
		uvm_lwp_rele(l2);

		KASSERT(l2 != l);
	} else if (vp->savp_lwp != NULL) {

		/*
		 * Case 2: We've been woken up while another LWP was
		 * on the VP, but we're going back to sleep without
		 * having returned to userland and delivering the
		 * SA_UNBLOCKED upcall (select and poll cause this
		 * kind of behavior a lot).
		 */
		l2 = NULL;
	} else {
		/* NOTREACHED */
		mutex_exit(&vp->savp_mutex);
		lwp_unlock(l);
		panic("sa_vp empty");
	}

	DPRINTFN(4,("sa_switch(%d.%d) switching to LWP %d.\n",
	    p->p_pid, l->l_lid, l2 ? l2->l_lid : 0));
	/* WRS need to add code to make sure we switch to l2 */
	mutex_exit(&vp->savp_mutex);
	mi_switch(l);
	DPRINTFN(4,("sa_switch(%d.%d flag %x) returned.\n",
	    p->p_pid, l->l_lid, l->l_flag));
	KASSERT(l->l_wchan == 0);
}

/*
 * sa_neverrun
 *
 *	Routine for threads that have never run. Calls lwp_exit.
 * New, never-run cache threads get pointed at this routine, which just runs
 * and calls lwp_exit().
 */
static void
sa_neverrun(void *arg)
{
	struct lwp *l;

	l = curlwp;

	DPRINTFN(1,("sa_neverrun(%d.%d %x) exiting\n", l->l_proc->p_pid,
	    l->l_lid, l->l_flag));

	lwp_exit(l);
}

/*
 * sa_switchcall
 *
 * We need to pass an upcall to userland. We are now
 * running on a spare stack and need to allocate a new
 * one. Also, if we are passed an sa upcall, we need to dispatch
 * it to the app.
 */
static void
sa_switchcall(void *arg)
{
	struct lwp *l, *l2;
	struct proc *p;
	struct sadata_vp *vp;
	struct sadata_upcall *sau;
	struct sastack *sast;
	struct sadata *sa;

	l2 = curlwp;
	p = l2->l_proc;
	vp = l2->l_savp;
	sau = arg;
	sa = p->p_sa;

	lwp_lock(l2);
	KASSERT(vp->savp_lwp == l2);
	if ((l2->l_flag & LW_WEXIT) || (p->p_sflag & (PS_WCORE | PS_WEXIT))) {
		lwp_unlock(l2);
		sadata_upcall_free(sau);
		lwp_exit(l2);
	}

	KASSERT(vp->savp_lwp == l2);
	DPRINTFN(6,("sa_switchcall(%d.%d)\n", p->p_pid, l2->l_lid));

	l2->l_flag |= LW_SA;
	lwp_unlock(l2);
	l2->l_pflag |= LP_SA_NOBLOCK;

	if (vp->savp_lwpcache_count == 0) {
		/* Allocate the next cache LWP */
		DPRINTFN(6,("sa_switchcall(%d.%d) allocating LWP\n",
		    p->p_pid, l2->l_lid));
		sa_newcachelwp(l2, NULL);
	}

	if (sau) {
		mutex_enter(&sa->sa_mutex);
		sast = sa_getstack(p->p_sa);
		mutex_exit(&sa->sa_mutex);
		mutex_enter(&vp->savp_mutex);
		l = vp->savp_blocker;
		if (sast) {
			sau->sau_stack = sast->sast_stack;
			SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);
			mutex_exit(&vp->savp_mutex);
			lwp_lock(l2);
			l2->l_flag |= LW_SA_UPCALL;
			lwp_unlock(l2);
		} else {
			/*
			 * Oops! We're in trouble. The app hasn't
			 * passed us in any stacks on which to deliver
			 * the upcall.
			 *
			 * WRS: I think this code is wrong. If we can't
			 * get a stack, we are dead. We either need
			 * to block waiting for one (assuming there's a
			 * live vp still in userland so it can hand back
			 * stacks, or we should just kill the process
			 * as we're deadlocked.
			 */
			if (vp->savp_sleeper_upcall == NULL)
				vp->savp_sleeper_upcall = sau;
			else
				sadata_upcall_free(sau);
			uvm_lwp_hold(l2);
			sa_putcachelwp(p, l2); /* sets LW_SA */
			mutex_exit(&vp->savp_mutex);
			lwp_lock(l);
			vp->savp_lwp = l;
			l->l_flag &= ~LW_SA_BLOCKING;
			lwp_unlock(l);
			//mutex_enter(p->p_lock);	/* XXXAD */
			//p->p_nrlwps--;
			//mutex_exit(p->p_lock);
			lwp_lock(l2);
			mi_switch(l2);
			/* mostly NOTREACHED */
			lwp_exit(l2);
		}
	}

	upcallret(l2);

	/*
	 * Ok, clear LP_SA_NOBLOCK. However it'd be VERY BAD to generate
	 * a blocked upcall before this upcall makes it to libpthread.
	 * So disable BLOCKED upcalls until this vp enters a syscall.
	 */
	l2->l_pflag &= ~LP_SA_NOBLOCK;
	vp->savp_pflags |= SAVP_FLAG_DELIVERING;
}

/*
 * sa_newcachelwp
 *	Allocate a new lwp, attach it to either the given vp or to l's vp,
 * and add it to its vp's idle cache.
 *	Assumes no locks (other than kernel lock) on entry and exit.
 * Locks scheduler lock during operation.
 *	Returns 0 on success or if process is exiting. Returns ENOMEM
 * if it is unable to allocate a new uarea.
 */
static int
sa_newcachelwp(struct lwp *l, struct sadata_vp *targ_vp)
{
	struct proc *p;
	struct lwp *l2;
	struct sadata_vp *vp;
	vaddr_t uaddr;
	boolean_t inmem;
	int error;

	p = l->l_proc;
	if (p->p_sflag & (PS_WCORE | PS_WEXIT))
		return (0);

	inmem = uvm_uarea_alloc(&uaddr);
	if (__predict_false(uaddr == 0)) {
		return (ENOMEM);
	}

	error = lwp_create(l, p, uaddr, inmem, 0, NULL, 0,
	    sa_neverrun, NULL, &l2, l->l_class);
	if (error) {
		uvm_uarea_free(uaddr, curcpu());
		return error;
	}

	/* We don't want this LWP on the process's main LWP list, but
	 * newlwp helpfully puts it there. Unclear if newlwp should
	 * be tweaked.
	 */
	mutex_enter(p->p_lock);
	p->p_nrlwps++;
	mutex_exit(p->p_lock);
	uvm_lwp_hold(l2);
	vp = (targ_vp) ? targ_vp : l->l_savp;
	mutex_enter(&vp->savp_mutex);
	l2->l_savp = vp;
	sa_putcachelwp(p, l2);
	mutex_exit(&vp->savp_mutex);

	return 0;
}

/*
 * sa_putcachelwp
 *	Take a normal process LWP and place it in the SA cache.
 * LWP must not be running, or it must be our caller.
 *	sadat_vp::savp_mutex held on entry and exit.
 *
 *	Previous NetBSD versions removed queued lwps from the list of
 * visible lwps. This made ps cleaner, and hid implementation details.
 * At present, this implementation no longer does that.
 */
void
sa_putcachelwp(struct proc *p, struct lwp *l)
{
	struct sadata_vp *vp;
	sleepq_t	*sq;

	vp = l->l_savp;
	sq = &vp->savp_lwpcache;

	KASSERT(mutex_owned(&vp->savp_mutex));

#if 0 /* not now, leave lwp visible to all */
	LIST_REMOVE(l, l_sibling);
	p->p_nlwps--;
	l->l_prflag |= LPR_DETACHED;
#endif
	l->l_flag |= LW_SA;
	membar_producer();
	DPRINTFN(5,("sa_putcachelwp(%d.%d) Adding LWP %d to cache\n",
	    p->p_pid, curlwp->l_lid, l->l_lid));

	/*
	 * Hand-rolled call of the form:
	 * sleepq_enter(&vp->savp_woken, l, &vp->savp_mutex);
	 * adapted to take into account the fact that (1) l and the mutex
	 * we want to lend it are both locked, and (2) we don't have
	 * any other locks.
	 */
	l->l_mutex = &vp->savp_mutex;

	/*
	 * XXXWRS: Following is a hand-rolled call of the form:
	 * sleepq_enqueue(sq, (void *)sq, "lwpcache", sa_sobj); but
	 * hand-done since l might not be curlwp.
	 */

        l->l_syncobj = &sa_sobj;
        l->l_wchan = sq;
        l->l_sleepq = sq;
        l->l_wmesg = sa_lwpcache_wmesg;
        l->l_slptime = 0;
        l->l_stat = LSSLEEP;
        l->l_sleeperr = 0;
         
        vp->savp_lwpcache_count++;
        sleepq_insert(sq, l, &sa_sobj);
}

/*
 * sa_getcachelwp
 *	Fetch a LWP from the cache.
 * Called with savp_mutex held.
 */
struct lwp *
sa_getcachelwp(struct proc *p, struct sadata_vp *vp)
{
	struct lwp	*l;
	sleepq_t	*sq = &vp->savp_lwpcache;

	KASSERT(mutex_owned(&vp->savp_mutex));
	KASSERT(vp->savp_lwpcache_count > 0);

	vp->savp_lwpcache_count--;
	l= TAILQ_FIRST(sq);

	/*
	 * Now we have a hand-unrolled version of part of sleepq_remove.
	 * The main issue is we do NOT want to make the lwp runnable yet
	 * since we need to set up the upcall first (we know our caller(s)).
	 */

	TAILQ_REMOVE(sq, l, l_sleepchain);
	l->l_syncobj = &sched_syncobj;
	l->l_wchan = NULL;
	l->l_sleepq = NULL;
	l->l_flag &= ~LW_SINTR;

#if 0 /* Not now, for now leave lwps in lwp list */
	LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);
#endif
	DPRINTFN(5,("sa_getcachelwp(%d.%d) Got LWP %d from cache.\n",
		p->p_pid, curlwp->l_lid, l->l_lid));
	return l;
}

/*
 * sa_setrunning:
 *
 *	Make an lwp we pulled out of the cache, with sa_getcachelwp()
 * above. This routine and sa_getcachelwp() must perform all the work
 * of sleepq_remove().
 */
static void
sa_setrunning(struct lwp *l)
{
	struct schedstate_percpu *spc;
	struct cpu_info *ci;

	KASSERT(mutex_owned(&l->l_savp->savp_mutex));

	/* Update sleep time delta, call the wake-up handler of scheduler */
	l->l_slpticksum += (hardclock_ticks - l->l_slpticks);
	sched_wakeup(l);

	/*
	 * Since l was on the sleep queue, we locked it
	 * when we locked savp_mutex. Now set it running.
	 * This is the second-part of sleepq_remove().
	 */
	l->l_priority = MAXPRI_USER; /* XXX WRS needs thought, used to be l_usrpri */
	/* Look for a CPU to wake up */
	l->l_cpu = sched_takecpu(l);
	ci = l->l_cpu;
	spc = &ci->ci_schedstate;

	spc_lock(ci);
	lwp_setlock(l, spc->spc_mutex);
	sched_setrunnable(l);
	l->l_stat = LSRUN;
	l->l_slptime = 0;
	sched_enqueue(l, true);
	spc_unlock(ci);
}

/*
 * sa_upcall_userret
 *	We are about to exit the kernel and return to userland, and
 * userret() noticed we have upcalls pending. So deliver them.
 *
 *	This is the place where unblocking upcalls get generated. We
 * allocate the stack & upcall event here. We may block doing so, but
 * we lock our LWP state (clear LW_SA for the moment) while doing so.
 *
 *	In the case of delivering multiple upcall events, we will end up
 * writing multiple stacks out to userland at once. The last one we send
 * out will be the first one run, then it will notice the others and
 * run them.
 *
 * No locks held on entry or exit. We lock varied processing.
 */
void
sa_upcall_userret(struct lwp *l)
{
	struct lwp *l2;
	struct proc *p;
	struct sadata *sa;
	struct sadata_vp *vp;
	struct sadata_upcall *sau;
	struct sastack *sast;
	sleepq_t *sq;
	int f;

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;

	if (vp->savp_pflags & SAVP_FLAG_NOUPCALLS) {
		int	do_clear = 0;
		/*
		 * We made upcalls in sa_yield() (otherwise we would
		 * still be in the loop there!). Don't do it again.
		 * Clear LW_SA_UPCALL, unless there are upcalls to deliver.
		 * they will get delivered next time we return to user mode.
		 */
		vp->savp_pflags &= ~SAVP_FLAG_NOUPCALLS;
		mutex_enter(&vp->savp_mutex);
		if ((vp->savp_woken_count == 0)
		    && SIMPLEQ_EMPTY(&vp->savp_upcalls)) {
			do_clear = 1;
		}
		mutex_exit(&vp->savp_mutex);
		if (do_clear) {
			lwp_lock(l);
			l->l_flag &= ~LW_SA_UPCALL;
			lwp_unlock(l);
		}
		DPRINTFN(7,("sa_upcall_userret(%d.%d %x) skipping processing\n",
		    p->p_pid, l->l_lid, l->l_flag));
		return;
	}

	SA_LWP_STATE_LOCK(l, f);

	DPRINTFN(7,("sa_upcall_userret(%d.%d %x) empty %d, woken %d\n",
	    p->p_pid, l->l_lid, l->l_flag, SIMPLEQ_EMPTY(&vp->savp_upcalls),
	    vp->savp_woken_count));

	KASSERT((l->l_flag & LW_SA_BLOCKING) == 0);

	mutex_enter(&vp->savp_mutex);
	sast = NULL;
	if (SIMPLEQ_EMPTY(&vp->savp_upcalls) &&
	    vp->savp_woken_count != 0) {
		mutex_exit(&vp->savp_mutex);
		mutex_enter(&sa->sa_mutex);
		sast = sa_getstack(sa);
		mutex_exit(&sa->sa_mutex);
		if (sast == NULL) {
			lwp_lock(l);
			SA_LWP_STATE_UNLOCK(l, f);
			lwp_unlock(l);
			preempt();
			return;
		}
		mutex_enter(&vp->savp_mutex);
	}
	if (SIMPLEQ_EMPTY(&vp->savp_upcalls) &&
	    vp->savp_woken_count != 0 && sast != NULL) {
		/*
		 * Invoke an "unblocked" upcall. We create a message
		 * with the first unblock listed here, and then
		 * string along a number of other unblocked stacks when
		 * we deliver the call.
		 */
		l2 = TAILQ_FIRST(&vp->savp_woken);
		TAILQ_REMOVE(&vp->savp_woken, l2, l_sleepchain);
		vp->savp_woken_count--;
		mutex_exit(&vp->savp_mutex);

		DPRINTFN(9,("sa_upcall_userret(%d.%d) using stack %p\n",
		    l->l_proc->p_pid, l->l_lid, sast->sast_stack.ss_sp));

		if ((l->l_flag & LW_WEXIT)
		    || (p->p_sflag & (PS_WCORE | PS_WEXIT))) {
			lwp_exit(l);
			/* NOTREACHED */
		}

		DPRINTFN(8,("sa_upcall_userret(%d.%d) unblocking %d\n",
		    p->p_pid, l->l_lid, l2->l_lid));

		sau = sadata_upcall_alloc(1);
		if ((l->l_flag & LW_WEXIT)
		    || (p->p_sflag & (PS_WCORE | PS_WEXIT))) {
			sadata_upcall_free(sau);
			lwp_exit(l);
			/* NOTREACHED */
		}

		sa_upcall0(sau, SA_UPCALL_UNBLOCKED, l2, l, 0, NULL, NULL);
		sau->sau_stack = sast->sast_stack;
		mutex_enter(&vp->savp_mutex);
		SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);
		l2->l_flag &= ~LW_SA_BLOCKING;

		/* Now return l2 to the cache. Mutex already set */
		sq = &vp->savp_lwpcache;
		l2->l_wchan = sq;
		l2->l_wmesg = sa_lwpcache_wmesg;
       		vp->savp_lwpcache_count++;
       		sleepq_insert(sq, l2, &sa_sobj);
			/* uvm_lwp_hold from sa_unblock_userret */
	} else if (sast)
		sa_setstackfree(sast, sa);

	KASSERT(vp->savp_lwp == l);

	while ((sau = SIMPLEQ_FIRST(&vp->savp_upcalls)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&vp->savp_upcalls, sau_next);
		mutex_exit(&vp->savp_mutex);
		sa_makeupcalls(l, sau);
		mutex_enter(&vp->savp_mutex);
	}
	mutex_exit(&vp->savp_mutex);

	lwp_lock(l);

	if (vp->savp_woken_count == 0) {
		l->l_flag &= ~LW_SA_UPCALL;
	}

	lwp_unlock(l);

	SA_LWP_STATE_UNLOCK(l, f);

	return;
}

#define	SACOPYOUT(sae, type, kp, up) \
	(((sae)->sae_sacopyout != NULL) ? \
	(*(sae)->sae_sacopyout)((type), (kp), (void *)(up)) : \
	copyout((kp), (void *)(up), sizeof(*(kp))))

/*
 * sa_makeupcalls
 *	We're delivering the first upcall on lwp l, so
 * copy everything out. We assigned the stack for this upcall
 * when we enqueued it.
 *
 * SA_LWP_STATE should be locked (LP_SA_NOBLOCK set).
 *
 *	If the enqueued event was DEFERRED, this is the time when we set
 * up the upcall event's state.
 */
static void
sa_makeupcalls(struct lwp *l, struct sadata_upcall *sau)
{
	struct lwp *l2;
	struct proc *p;
	const struct sa_emul *sae;
	struct sadata *sa;
	struct sadata_vp *vp;
	sleepq_t *sq;
	uintptr_t sapp, sap;
	struct sa_t self_sa;
	struct sa_t *sas[3];
	struct sa_t **ksapp = NULL;
	void *stack, *ap;
	union sau_state *e_ss;
	ucontext_t *kup, *up;
	size_t sz, ucsize;
	int i, nint, nevents, type, error;

	p = l->l_proc;
	sae = p->p_emul->e_sa;
	sa = p->p_sa;
	vp = l->l_savp;
	ucsize = sae->sae_ucsize;

	if (sau->sau_flags & SAU_FLAG_DEFERRED_EVENT)
		sa_upcall_getstate(&sau->sau_event,
		    sau->sau_event.ss_deferred.ss_lwp, 0);
	if (sau->sau_flags & SAU_FLAG_DEFERRED_INTERRUPTED)
		sa_upcall_getstate(&sau->sau_interrupted,
		    sau->sau_interrupted.ss_deferred.ss_lwp, 0);

#ifdef __MACHINE_STACK_GROWS_UP
	stack = sau->sau_stack.ss_sp;
#else
	stack = (char *)sau->sau_stack.ss_sp + sau->sau_stack.ss_size;
#endif
	stack = STACK_ALIGN(stack, ALIGNBYTES);

	self_sa.sa_id = l->l_lid;
	self_sa.sa_cpu = vp->savp_id;
	sas[0] = &self_sa;
	nevents = 0;
	nint = 0;
	if (sau->sau_event.ss_captured.ss_sa.sa_context != NULL) {
		if (copyout(&sau->sau_event.ss_captured.ss_ctx,
		    sau->sau_event.ss_captured.ss_sa.sa_context,
		    ucsize) != 0) {
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		sas[1] = &sau->sau_event.ss_captured.ss_sa;
		nevents = 1;
	}
	if (sau->sau_interrupted.ss_captured.ss_sa.sa_context != NULL) {
		KASSERT(sau->sau_interrupted.ss_captured.ss_sa.sa_context !=
		    sau->sau_event.ss_captured.ss_sa.sa_context);
		if (copyout(&sau->sau_interrupted.ss_captured.ss_ctx,
		    sau->sau_interrupted.ss_captured.ss_sa.sa_context,
		    ucsize) != 0) {
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		sas[2] = &sau->sau_interrupted.ss_captured.ss_sa;
		nint = 1;
	}
#if 0
	/* For now, limit ourselves to one unblock at once. */
	if (sau->sau_type == SA_UPCALL_UNBLOCKED) {
		mutex_enter(&vp->savp_mutex);
		nevents += vp->savp_woken_count;
		mutex_exit(&vp->savp_mutex);
		/* XXX WRS Need to limit # unblocks we copy out at once! */
	}
#endif

	/* Copy out the activation's ucontext */
	up = (void *)STACK_ALLOC(stack, ucsize);
	stack = STACK_GROW(stack, ucsize);
	kup = kmem_zalloc(sizeof(*kup), KM_SLEEP);
	KASSERT(kup != NULL);
	kup->uc_stack = sau->sau_stack;
	kup->uc_flags = _UC_STACK;
	error = SACOPYOUT(sae, SAOUT_UCONTEXT, kup, up);
	kmem_free(kup, sizeof(*kup));
	if (error) {
		sadata_upcall_free(sau);
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}
	sas[0]->sa_context = up;

	/* Next, copy out the sa_t's and pointers to them. */

	sz = (1 + nevents + nint) * sae->sae_sasize;
	sap = (uintptr_t)STACK_ALLOC(stack, sz);
	sap += sz;
	stack = STACK_GROW(stack, sz);

	sz = (1 + nevents + nint) * sae->sae_sapsize;
	sapp = (uintptr_t)STACK_ALLOC(stack, sz);
	sapp += sz;
	stack = STACK_GROW(stack, sz);

	if (KTRPOINT(p, KTR_SAUPCALL))
		ksapp = kmem_alloc(sizeof(struct sa_t *) * (nevents + nint + 1),
		    KM_SLEEP);

	KASSERT(nint <= 1);
	e_ss = NULL;
	for (i = nevents + nint; i >= 0; i--) {
		struct sa_t *sasp;

		sap -= sae->sae_sasize;
		sapp -= sae->sae_sapsize;
		error = 0;
		if (i == 1 + nevents)	/* interrupted sa */
			sasp = sas[2];
		else if (i <= 1)	/* self_sa and event sa */
			sasp = sas[i];
		else {			/* extra sas */
			KASSERT(sau->sau_type == SA_UPCALL_UNBLOCKED);

			if (e_ss == NULL) {
				e_ss = kmem_alloc(sizeof(*e_ss), KM_SLEEP);
			}
			/* Lock vp and all savp_woken lwps */
			mutex_enter(&vp->savp_mutex);
			sq = &vp->savp_woken;
			KASSERT(vp->savp_woken_count > 0);
			l2 = TAILQ_FIRST(sq);
			KASSERT(l2 != NULL);
			TAILQ_REMOVE(sq, l2, l_sleepchain);
			vp->savp_woken_count--;

			DPRINTFN(8,
			    ("sa_makeupcalls(%d.%d) unblocking extra %d\n",
			    p->p_pid, l->l_lid, l2->l_lid));
			/*
			 * Since l2 was on savp_woken, we locked it when
			 * we locked savp_mutex
			 */
			sa_upcall_getstate(e_ss, l2, 1);
			l2->l_flag &= ~LW_SA_BLOCKING;

			/* Now return l2 to the cache. Mutex already set */
			sq = &vp->savp_lwpcache;
			l2->l_wchan = sq;
			l2->l_wmesg = sa_lwpcache_wmesg;
        		vp->savp_lwpcache_count++;
       	 		sleepq_insert(sq, l2, &sa_sobj);
				/* uvm_lwp_hold from sa_unblock_userret */
			mutex_exit(&vp->savp_mutex);

			error = copyout(&e_ss->ss_captured.ss_ctx,
			    e_ss->ss_captured.ss_sa.sa_context, ucsize);
			sasp = &e_ss->ss_captured.ss_sa;
		}
		if (error != 0 ||
		    SACOPYOUT(sae, SAOUT_SA_T, sasp, sap) ||
		    SACOPYOUT(sae, SAOUT_SAP_T, &sap, sapp)) {
			/* Copying onto the stack didn't work. Die. */
			sadata_upcall_free(sau);
			if (e_ss != NULL) {
				kmem_free(e_ss, sizeof(*e_ss));
			}
			goto fail;
		}
		if (KTRPOINT(p, KTR_SAUPCALL))
			ksapp[i] = sasp;
	}
	if (e_ss != NULL) {
		kmem_free(e_ss, sizeof(*e_ss));
	}

	/* Copy out the arg, if any */
	/* xxx assume alignment works out; everything so far has been
	 * a structure, so...
	 */
	if (sau->sau_arg) {
		ap = STACK_ALLOC(stack, sau->sau_argsize);
		stack = STACK_GROW(stack, sau->sau_argsize);
		if (copyout(sau->sau_arg, ap, sau->sau_argsize) != 0) {
			/* Copying onto the stack didn't work. Die. */
			sadata_upcall_free(sau);
			goto fail;
		}
	} else {
		ap = NULL;
#ifdef __hppa__
		stack = STACK_ALIGN(stack, HPPA_FRAME_SIZE);
#endif
	}
	type = sau->sau_type;

	if (vp->savp_sleeper_upcall == NULL)
		vp->savp_sleeper_upcall = sau;
	else
		sadata_upcall_free(sau);

	DPRINTFN(7,("sa_makeupcalls(%d.%d): type %d\n", p->p_pid,
	    l->l_lid, type));

	if (KTRPOINT(p, KTR_SAUPCALL)) {
		ktrsaupcall(l, type, nevents, nint, (void *)sapp, ap, ksapp);
		kmem_free(ksapp, sizeof(struct sa_t *) * (nevents + nint + 1));
	}

	(*sae->sae_upcall)(l, type, nevents, nint, (void *)sapp, ap, stack,
	    sa->sa_upcall);

	lwp_lock(l);
	l->l_flag &= ~LW_SA_YIELD;
	lwp_unlock(l);
	return;

fail:
	if (KTRPOINT(p, KTR_SAUPCALL))
		kmem_free(ksapp, sizeof(struct sa_t) * (nevents + nint + 1));
	sigexit(l, SIGILL);
	/* NOTREACHED */
}

/*
 * sa_unblock_userret:
 *
 *	Our lwp is in the process of returning to userland, and
 * userret noticed LW_SA_BLOCKING is set for us. This indicates that
 * we were at one time the blessed lwp for our vp and we blocked.
 * An upcall was delivered to our process indicating that we blocked.
 *	Since then, we have unblocked in the kernel, and proceeded
 * to finish whatever work needed to be done. For instance, pages
 * have been faulted in for a trap or system call results have been
 * saved out for a systemcall.
 *	We now need to simultaneously do two things. First, we have to
 * cause an UNBLOCKED upcall to be generated. Second, we actually
 * have to STOP executing. When the blocked upcall was generated, a
 * new lwp was given to our application. Thus if we simply returned,
 * we would be exceeding our concurrency.
 *	 So we put ourself on our vp's savp_woken list and take
 * steps to make sure the blessed lwp will notice us. Note: we maintain
 * loose concurrency controls, so the blessed lwp for our vp could in
 * fact be running on another cpu in the system.
 */
void
sa_unblock_userret(struct lwp *l)
{
	struct lwp *l2, *vp_lwp;
	struct proc *p;
	struct sadata *sa;
	struct sadata_vp *vp;
	int swapper;

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;

	if ((l->l_flag & LW_WEXIT) || (p->p_sflag & (PS_WCORE | PS_WEXIT)))
		return;

	if ((l->l_flag & LW_SA_BLOCKING) == 0)
		return;

	DPRINTFN(7,("sa_unblock_userret(%d.%d %x) \n", p->p_pid, l->l_lid,
	    l->l_flag));

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;
	vp_lwp = vp->savp_lwp;
	l2 = NULL;
	swapper = 0;

	KASSERT(vp_lwp != NULL);
	DPRINTFN(3,("sa_unblock_userret(%d.%d) woken, flags %x, vp %d\n",
		     l->l_proc->p_pid, l->l_lid, l->l_flag,
		     vp_lwp->l_lid));

#if notyet
	if (vp_lwp->l_flag & LW_SA_IDLE) {
		KASSERT((vp_lwp->l_flag & LW_SA_UPCALL) == 0);
		KASSERT(vp->savp_wokenq_head == NULL);
		DPRINTFN(3,
		    ("sa_unblock_userret(%d.%d) repossess: idle vp_lwp %d state %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    vp_lwp->l_lid, vp_lwp->l_stat));
		vp_lwp->l_flag &= ~LW_SA_IDLE;
		uvm_lwp_rele(l);
		return;
	}
#endif

	DPRINTFN(3,(
	    "sa_unblock_userret(%d.%d) put on wokenq: vp_lwp %d state %d flags %x\n",
		     l->l_proc->p_pid, l->l_lid, vp_lwp->l_lid,
		     vp_lwp->l_stat, vp_lwp->l_flag));

	lwp_lock(vp_lwp);

	if (!mutex_tryenter(&vp->savp_mutex)) {
		lwp_unlock(vp_lwp);
		mutex_enter(&vp->savp_mutex);
		/* savp_lwp may have changed. We'll be ok even if it did */
		vp_lwp = vp->savp_lwp;
		lwp_lock(vp_lwp);
	}
	

	switch (vp_lwp->l_stat) {
	case LSONPROC:
		if (vp_lwp->l_flag & LW_SA_UPCALL)
			break;
		vp_lwp->l_flag |= LW_SA_UPCALL;
		if (vp_lwp->l_flag & LW_SA_YIELD)
			break;
		spc_lock(vp_lwp->l_cpu);
		cpu_need_resched(vp_lwp->l_cpu, RESCHED_IMMED);
		spc_unlock(vp_lwp->l_cpu);
		break;
	case LSSLEEP:
		if (vp_lwp->l_flag & LW_SA_IDLE) {
			vp_lwp->l_flag &= ~(LW_SA_IDLE|LW_SA_YIELD|LW_SINTR);
			vp_lwp->l_flag |= LW_SA_UPCALL;
			/* lwp_unsleep() will unlock the LWP */
			lwp_unsleep(vp_lwp, true);
			DPRINTFN(3,(
			    "sa_unblock_userret(%d.%d) woke vp: %d state %d\n",
			     l->l_proc->p_pid, l->l_lid, vp_lwp->l_lid,
			     vp_lwp->l_stat));
			vp_lwp = NULL;
			break;
		}
		vp_lwp->l_flag |= LW_SA_UPCALL;
		break;
	case LSSUSPENDED:
		break;
	case LSSTOP:
		vp_lwp->l_flag |= LW_SA_UPCALL;
		break;
	case LSRUN:
		if (vp_lwp->l_flag & LW_SA_UPCALL)
			break;
		vp_lwp->l_flag |= LW_SA_UPCALL;
		if (vp_lwp->l_flag & LW_SA_YIELD)
			break;
#if 0
		if (vp_lwp->l_slptime > 1) {
			void updatepri(struct lwp *);
			updatepri(vp_lwp);
		}
#endif
		vp_lwp->l_slptime = 0;
		if (vp_lwp->l_flag & LW_INMEM) {
			if (vp_lwp->l_cpu == curcpu())
				l2 = vp_lwp;
			else {
				/*
				 * don't need to spc_lock the other cpu
				 * as runable lwps have the cpu as their
				 * mutex.
				 */
				/* spc_lock(vp_lwp->l_cpu); */
				cpu_need_resched(vp_lwp->l_cpu, 0);
				/* spc_unlock(vp_lwp->l_cpu); */
			}
		} else
			swapper = 1;
		break;
	default:
		panic("sa_vp LWP not sleeping/onproc/runnable");
	}

	if (vp_lwp != NULL)
		lwp_unlock(vp_lwp);

	if (swapper)
		wakeup(&proc0);

	/*
	 * Add ourselves to the savp_woken queue. Still on p_lwps.
	 *
	 * We now don't unlock savp_mutex since it now is l's mutex,
	 * and it will be released in mi_switch().
	 */
	sleepq_enter(&vp->savp_woken, l, &vp->savp_mutex);
	sleepq_enqueue(&vp->savp_woken, &vp->savp_woken, sa_lwpwoken_wmesg,
	    &sa_sobj);
	uvm_lwp_hold(l);
	vp->savp_woken_count++;
	//l->l_stat = LSSUSPENDED;
	mi_switch(l);

	/*
	 * We suspended ourself and put ourself on the savp_woken
	 * list. The only way we come back from mi_switch() to this
	 * routine is if we were put back on the run queues, which only
	 * happens if the process is exiting. So just exit.
	 *
	 * In the normal lwp lifecycle, cpu_setfunc() will make this lwp
	 * run in a different routine by the time we next run.
	 */
	lwp_exit(l);
	/* NOTREACHED */
}



#ifdef DEBUG
int debug_print_sa(struct proc *);
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
	struct sadata *sa;
	struct sadata_vp *vp;

	printf("Process %d (%s), state %d, address %p, flags %x\n",
	    p->p_pid, p->p_comm, p->p_stat, p, p->p_sflag);
	printf("LWPs: %d (%d running, %d zombies)\n", p->p_nlwps, p->p_nrlwps,
	    p->p_nzlwps);
	sa = p->p_sa;
	if (sa) {
		SLIST_FOREACH(vp, &sa->sa_vps, savp_next) {
			if (vp->savp_lwp)
				printf("SA VP: %d %s\n", vp->savp_lwp->l_lid,
				    vp->savp_lwp->l_flag & LW_SA_YIELD ?
				    (vp->savp_lwp->l_flag & LW_SA_IDLE ?
					"idle" : "yielding") : "");
			printf("SAs: %d cached LWPs\n",
					vp->savp_lwpcache_count);
			printf("SAs: %d woken LWPs\n",
					vp->savp_woken_count);
		}
	}

	return 0;
}

#endif

#endif /* KERN_SA */
