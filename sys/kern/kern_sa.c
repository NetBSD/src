/*	$NetBSD: kern_sa.c,v 1.87.4.7 2007/05/23 02:20:05 wrstuden Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005 The NetBSD Foundation, Inc.
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

#include "opt_ktrace.h"
#include "opt_multiprocessor.h"
__KERNEL_RCSID(0, "$NetBSD: kern_sa.c,v 1.87.4.7 2007/05/23 02:20:05 wrstuden Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/kmem.h>
#include <sys/mount.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>

#include <uvm/uvm_extern.h>

static POOL_INIT(sadata_pool, sizeof(struct sadata), 0, 0, 0, "sadatapl",
    &pool_allocator_nointr); /* memory pool for sadata structures */
static POOL_INIT(saupcall_pool, sizeof(struct sadata_upcall), 0, 0, 0,
    "saupcpl", &pool_allocator_nointr); /* memory pool for pending upcalls */
static POOL_INIT(sastack_pool, sizeof(struct sastack), 0, 0, 0, "sastackpl",
    &pool_allocator_nointr); /* memory pool for sastack structs */
static POOL_INIT(savp_pool, sizeof(struct sadata_vp), 0, 0, 0, "savppl",
    &pool_allocator_nointr); /* memory pool for sadata_vp structures */

static struct sadata_vp *sa_newsavp(struct sadata *);
static inline int sa_stackused(struct sastack *, struct sadata *);
static inline void sa_setstackfree(struct sastack *, struct sadata *);
static struct sastack *sa_getstack(struct sadata *);
static inline struct sastack *sa_getstack0(struct sadata *);
static inline int sast_compare(struct sastack *, struct sastack *);
#ifdef MULTIPROCESSOR
static int sa_increaseconcurrency(struct lwp *, int);
#endif
static void sa_setwoken(struct lwp *);
static void sa_switchcall(void *);
static int sa_newcachelwp(struct lwp *);
static inline void sa_makeupcalls(struct lwp *);
static struct lwp *sa_vp_repossess(struct lwp *l);

static inline int sa_pagefault(struct lwp *, ucontext_t *);

static void sa_upcall0(struct sadata_upcall *, int, struct lwp *, struct lwp *,
    size_t, void *, void (*)(void *));
static void sa_upcall_getstate(union sau_state *, struct lwp *);

#define SA_DEBUG

#ifdef SA_DEBUG
#define DPRINTF(x)	do { if (sadebug) printf_nolog x; } while (0)
#define DPRINTFN(n,x)	do { if (sadebug & (1<<(n-1))) printf_nolog x; } while (0)
int	sadebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif


#define SA_LWP_STATE_LOCK(l, f) do {				\
	(f) = (l)->l_flag;     					\
	(l)->l_flag &= ~L_SA;					\
} while (/*CONSTCOND*/ 0)

#define SA_LWP_STATE_UNLOCK(l, f) do {				\
	(l)->l_flag |= (f) & L_SA;				\
} while (/*CONSTCOND*/ 0)

SPLAY_PROTOTYPE(sasttree, sastack, sast_node, sast_compare);
SPLAY_GENERATE(sasttree, sastack, sast_node, sast_compare);

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

	sau = pool_get(&saupcall_pool, waitok ? PR_WAITOK : PR_NOWAIT);
	if (sau) {
		sau->sau_arg = NULL;
	}
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

	if (sau == NULL) {
		return;
	}
	if (sau->sau_arg) {
		(*sau->sau_argfreefunc)(sau->sau_arg);
	}
	pool_put(&saupcall_pool, sau);
}

/*
 * sa_newsavp
 *
 * Allocate a new virtual processor structure, do some simple
 * initialization and add it to the passed-in sa. Pre-allocate
 * an upcall event data structure for when the main thread on
 * this vp blocks.
 *
 * We lock sa_lock while manipulating the list of vp's.
 *
 * We allocate the lwp to run on this separately. In the case of the
 * first lwp/vp for a process, the lwp already exists. It's the
 * main (only) lwp of the process.
 */
static struct sadata_vp *
sa_newsavp(struct sadata *sa)
{
	struct sadata_vp *vp, *qvp;
	struct sadata_upcall *sau;

	/* Allocate virtual processor data structure */
	vp = pool_get(&savp_pool, PR_WAITOK);
	/* And preallocate an upcall data structure for sleeping */
	sau = sadata_upcall_alloc(1);
	/* Initialize. */
	memset(vp, 0, sizeof(*vp));
	simple_lock_init(&vp->savp_lock);
	vp->savp_lwp = NULL;
	vp->savp_wokenq_head = NULL;
	vp->savp_faultaddr = 0;
	vp->savp_ofaultaddr = 0;
	LIST_INIT(&vp->savp_lwpcache);
	vp->savp_ncached = 0;
	vp->savp_sleeper_upcall = sau;
	SIMPLEQ_INIT(&vp->savp_upcalls);

	simple_lock(&sa->sa_lock);
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
	simple_unlock(&sa->sa_lock);

	return (vp);
}

/*
 * sys_sa_register
 *	Handle copyin and copyout of info for registering the
 * upcall handler address.
 */
int
sys_sa_register(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_register_args /* {
		syscallarg(sa_upcall_t) new;
		syscallarg(sa_upcall_t *) old;
		syscallarg(int) flags;
		syscallarg(ssize_t) stackinfo_offset;
	} */ *uap = v;
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

	if (p->p_sa == NULL) {
		/* Allocate scheduler activations data structure */
		sa = pool_get(&sadata_pool, PR_WAITOK);
		/* Initialize. */
		memset(sa, 0, sizeof(*sa));
		simple_lock_init(&sa->sa_lock);
		sa->sa_flag = flags & SA_FLAG_ALL;
		sa->sa_maxconcurrency = 1;
		sa->sa_concurrency = 1;
		SPLAY_INIT(&sa->sa_stackstree);
		sa->sa_stacknext = NULL;
		if (flags & SA_FLAG_STACKINFO)
			sa->sa_stackinfo_offset = stackinfo_offset;
		else
			sa->sa_stackinfo_offset = 0;
		sa->sa_nstacks = 0;
		SLIST_INIT(&sa->sa_vps);
		p->p_sa = sa;
		KASSERT(l->l_savp == NULL);
	}
	if (l->l_savp == NULL) {
		l->l_savp = sa_newsavp(p->p_sa);
		sa_newcachelwp(l);
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
	KDASSERT(sa != NULL);
	KASSERT(p->p_nlwps <= 1);

	for (sast = SPLAY_MIN(sasttree, &sa->sa_stackstree); sast != NULL;
	     sast = next) {
		next = SPLAY_NEXT(sasttree, &sa->sa_stackstree, sast);
		SPLAY_REMOVE(sasttree, &sa->sa_stackstree, sast);
		pool_put(&sastack_pool, sast);
	}

	p->p_flag &= ~P_SA;
	while ((vp = SLIST_FIRST(&p->p_sa->sa_vps)) != NULL) {
		SLIST_REMOVE_HEAD(&p->p_sa->sa_vps, savp_next);
		if (vp->savp_sleeper_upcall) {
			sadata_upcall_free(vp->savp_sleeper_upcall);
			vp->savp_sleeper_upcall = NULL;
		}
		pool_put(&savp_pool, vp);
	}
	pool_put(&sadata_pool, sa);
	p->p_sa = NULL;
	l = LIST_FIRST(&p->p_lwps);
	if (l) {
		KASSERT(LIST_NEXT(l, l_sibling) == NULL);
		l->l_savp = NULL;
	}
}

/*
 * sa_fetchstackgen
 *
 *	copyin the generation number for the stack in question.
 *
 * WRS: I think this routine needs the SA_LWP_STATE_LOCK() dance, either
 * here or in its caller.
 */
static int
sa_fetchstackgen(struct sastack *sast, struct sadata *sa, unsigned int *gen)
{
	int error;

	/* COMPAT_NETBSD32:  believe it or not, but the following is ok */
	error = copyin(&((struct sa_stackinfo_t *)
	    ((char *)sast->sast_stack.ss_sp +
	    sa->sa_stackinfo_offset))->sasi_stackgen, gen, sizeof(*gen));

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

	if (sa_fetchstackgen(sast, sa, &gen)) {
#ifdef DIAGNOSTIC
		printf("sa_stackused: couldn't copyin sasi_stackgen");
#endif
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

	if (sa_fetchstackgen(sast, sa, &gen)) {
#ifdef DIAGNOSTIC
		printf("sa_setstackfree: couldn't copyin sasi_stackgen");
#endif
		sigexit(curlwp, SIGILL);
		/* NOTREACHED */
	}
	sast->sast_gen = gen;
}

/*
 * sa_getstack
 *
 * Find next free stack, starting at sa->sa_stacknext.
 *
 * Caller must have the splay tree locked and should have cleared L_SA for
 * our thread. This is not the time to go generating upcalls as we aren't
 * in a position to deliver another one.
 */
static struct sastack *
sa_getstack(struct sadata *sa)
{
	struct sastack *sast;

	SCHED_ASSERT_UNLOCKED();

	if ((sast = sa->sa_stacknext) == NULL || sa_stackused(sast, sa))
		sast = sa_getstack0(sa);

	if (sast == NULL)
		return NULL;

	sast->sast_gen++;

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

	if (sa->sa_stacknext == NULL) {
		sa->sa_stacknext = SPLAY_MIN(sasttree, &sa->sa_stackstree);
		if (sa->sa_stacknext == NULL)
			return NULL;
	}
	start = sa->sa_stacknext;

	while (sa_stackused(sa->sa_stacknext, sa)) {
		sa->sa_stacknext = SPLAY_NEXT(sasttree, &sa->sa_stackstree,
		    sa->sa_stacknext);
		if (sa->sa_stacknext == NULL)
			sa->sa_stacknext = SPLAY_MIN(sasttree,
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
sys_sa_stacks(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_stacks_args /* {
		syscallarg(int) num;
		syscallarg(stack_t *) stacks;
	} */ *uap = v;

	return sa_stacks1(l, retval, SCARG(uap, num), SCARG(uap, stacks), sa_copyin_stack);
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
	struct sastack *sast, newsast;
	int count, error, f, i;

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	count = num;
	if (count < 0)
		return (EINVAL);

	SA_LWP_STATE_LOCK(l, f);

	error = 0;

	for (i = 0; i < count; i++) {
		error = do_sa_copyin_stack(stacks, i, &newsast.sast_stack);
		if (error) {
			count = i;
			break;
		}
		sast = SPLAY_FIND(sasttree, &sa->sa_stackstree, &newsast);
		if (sast != NULL) {
			DPRINTFN(9, ("sa_stacks(%d.%d) returning stack %p\n",
				     l->l_proc->p_pid, l->l_lid,
				     newsast.sast_stack.ss_sp));
			if (sa_stackused(sast, sa) == 0) {
				count = i;
				error = EEXIST;
				break;
			}
		} else if (sa->sa_nstacks >=
		    SA_MAXNUMSTACKS * sa->sa_concurrency) {
			DPRINTFN(9,
			    ("sa_stacks(%d.%d) already using %d stacks\n",
			    l->l_proc->p_pid, l->l_lid,
			    SA_MAXNUMSTACKS * sa->sa_concurrency));
			count = i;
			error = ENOMEM;
			break;
		} else {
			DPRINTFN(9, ("sa_stacks(%d.%d) adding stack %p\n",
				     l->l_proc->p_pid, l->l_lid,
				     newsast.sast_stack.ss_sp));
			sast = pool_get(&sastack_pool, PR_WAITOK);
			sast->sast_stack = newsast.sast_stack;
			SPLAY_INSERT(sasttree, &sa->sa_stackstree, sast);
			sa->sa_nstacks++;
		}
		sa_setstackfree(sast, sa);
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
sys_sa_enable(struct lwp *l, void *v, register_t *retval)
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

	if (p->p_flag & P_SA) /* Already running! */
		return (EBUSY);

	error = sa_upcall(l, SA_UPCALL_NEWPROC, l, NULL, 0, NULL, NULL);
	if (error)
		return (error);

	/* Assign this LWP to the virtual processor */
	vp->savp_lwp = l;

	p->p_flag |= P_SA;
	l->l_flag |= L_SA; /* We are now an activation LWP */

	/* This will not return to the place in user space it came from. */
	return (0);
}


/*
 * sa_increaseconcurrency
 *	Raise the process's maximum concurrency level to the
 * requested level. Does nothing if the current maximum councurrency
 * is greater than the requested.
 *	Uses the kernel lock to implicitly lock operations. Also
 * uses sa_lock to lock the vp list and uses SCHED_LOCK() to lock the
 * scheduler.
 */
#ifdef MULTIPROCESSOR
static int
sa_increaseconcurrency(struct lwp *l, int concurrency)
{
	struct proc *p;
	struct lwp *l2;
	struct sadata *sa;
	vaddr_t uaddr;
	boolean_t inmem;
	int addedconcurrency, error, s;
	struct sadata_vp *vp;

	p = l->l_proc;
	sa = p->p_sa;

	addedconcurrency = 0;
	simple_lock(&sa->sa_lock);
	while (sa->sa_maxconcurrency < concurrency) {
		sa->sa_maxconcurrency++;
		sa->sa_concurrency++;
		simple_unlock(&sa->sa_lock);

		inmem = uvm_uarea_alloc(&uaddr);
		if (__predict_false(uaddr == 0)) {
			/* reset concurrency */
			simple_lock(&sa->sa_lock);
			sa->sa_maxconcurrency--;
			sa->sa_concurrency--;
			simple_unlock(&sa->sa_lock);
			return (addedconcurrency);
		} else {
			newlwp(l, p, uaddr, inmem, 0, NULL, 0,
			    child_return, 0, &l2);
			l2->l_flag |= L_SA;
			l2->l_savp = vp = sa_newsavp(sa);
			if (vp) {
				vp->savp_lwp = l2;
				cpu_setfunc(l2, sa_switchcall, NULL);
				error = sa_upcall(l2, SA_UPCALL_NEWPROC,
				    NULL, NULL, 0, NULL, NULL);
				if (error) {
					/* free new savp */
					SLIST_REMOVE(&sa->sa_vps, vp,
					    sadata_vp, savp_next);
					if (vp->savp_sleeper_upcall) {
						sadata_upcall_free(
						    vp->savp_sleeper_upcall);
						vp->savp_sleeper_upcall = NULL;
					}
					pool_put(&savp_pool, vp);
				}
			} else
				error = 1;
			if (error) {
				/* put l2 into l's LWP cache */
				l2->l_savp = l->l_savp;
				PHOLD(l2);
				SCHED_LOCK(s);
				sa_putcachelwp(p, l2);
				SCHED_UNLOCK(s);
				/* reset concurrency */
				simple_lock(&sa->sa_lock);
				sa->sa_maxconcurrency--;
				sa->sa_concurrency--;
				simple_unlock(&sa->sa_lock);
				return (addedconcurrency);
			}
			SCHED_LOCK(s);
			setrunnable(l2);
			SCHED_UNLOCK(s);
			addedconcurrency++;
		}
		simple_lock(&sa->sa_lock);
	}
	simple_unlock(&sa->sa_lock);

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
sys_sa_setconcurrency(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_setconcurrency_args /* {
		syscallarg(int) concurrency;
	} */ *uap = v;
	struct sadata *sa = l->l_proc->p_sa;
#ifdef MULTIPROCESSOR
	struct sadata_vp *vp = l->l_savp;
	int ncpus, s;
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
#endif

	DPRINTFN(11,("sys_sa_concurrency(%d.%d)\n", l->l_proc->p_pid,
		     l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	if ((l->l_proc->p_flag & P_SA) == 0)
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
#ifdef MULTIPROCESSOR
	if (SCARG(uap, concurrency) > sa->sa_maxconcurrency) {
		ncpus = 0;
		for (CPU_INFO_FOREACH(cii, ci))
			ncpus++;
		*retval += sa_increaseconcurrency(l,
		    min(SCARG(uap, concurrency), ncpus));
	}
#endif

	DPRINTFN(11,("sys_sa_concurrency(%d.%d) want %d, have %d, max %d\n",
		     l->l_proc->p_pid, l->l_lid, SCARG(uap, concurrency),
		     sa->sa_concurrency, sa->sa_maxconcurrency));
#ifdef MULTIPROCESSOR
	if (SCARG(uap, concurrency) > sa->sa_concurrency) {
		SCHED_LOCK(s);
		SLIST_FOREACH(vp, &sa->sa_vps, savp_next) {
			if (vp->savp_lwp->l_flag & L_SA_IDLE) {
				vp->savp_lwp->l_flag &=
					~(L_SA_IDLE|L_SA_YIELD|L_SINTR);
				SCHED_UNLOCK(s);
				DPRINTFN(11,("sys_sa_concurrency(%d.%d) "
					     "NEWPROC vp %d\n",
					     l->l_proc->p_pid, l->l_lid,
					     vp->savp_id));
				cpu_setfunc(vp->savp_lwp, sa_switchcall, NULL);
				/* error = */ sa_upcall(vp->savp_lwp,
				    SA_UPCALL_NEWPROC,
				    NULL, NULL, 0, NULL, NULL);
				SCHED_LOCK(s);
				sa->sa_concurrency++;
				setrunnable(vp->savp_lwp);
				KDASSERT((vp->savp_lwp->l_flag & L_SINTR) == 0);
				(*retval)++;
			}
			if (sa->sa_concurrency == SCARG(uap, concurrency))
				break;
		}
		SCHED_UNLOCK(s);
	}
#endif

	return (0);
}

/*
 * sys_sa_yield
 *	application has nothing for this lwp to do, so let it linger in
 * the kernel.
 */
int
sys_sa_yield(struct lwp *l, void *v, register_t *retval)
{
	struct proc *p = l->l_proc;

	if (p->p_sa == NULL || !(p->p_flag & P_SA)) {
		DPRINTFN(1,
		    ("sys_sa_yield(%d.%d) proc %p not SA (p_sa %p, flag %s)\n",
		    p->p_pid, l->l_lid, p, p->p_sa,
		    p->p_flag & P_SA ? "T" : "F"));
		return (EINVAL);
	}

	sa_yield(l);

	return (EJUSTRETURN);
}

/*
 * sa_yield
 *	This lwp has nothing to do, so hang around. Assuming we
 * are the lwp "on" our vp, tsleep in "sawait" until there's something
 * to do. If there are upcalls, we deliver them explicitly.
 */
void
sa_yield(struct lwp *l)
{
	struct proc *p = l->l_proc;
	struct sadata *sa = p->p_sa;
	struct sadata_vp *vp = l->l_savp;
	int ret;

	KERNEL_LOCK_ASSERT_LOCKED();

	if (vp->savp_lwp != l) {
		/*
		 * We lost the VP on our way here, this happens for
		 * instance when we sleep in systrace.  This will end
		 * in an SA_UNBLOCKED_UPCALL in sa_setwoken().
		 */
		DPRINTFN(1,("sa_yield(%d.%d) lost VP\n",
			     p->p_pid, l->l_lid));
		KDASSERT(l->l_flag & L_SA_BLOCKING);
		return;
	}

	/*
	 * If we're the last running LWP, stick around to receive
	 * signals.
	 */
	KDASSERT((l->l_flag & L_SA_YIELD) == 0);
	DPRINTFN(1,("sa_yield(%d.%d) going dormant\n",
		     p->p_pid, l->l_lid));
	/*
	 * A signal will probably wake us up. Worst case, the upcall
	 * happens and just causes the process to yield again.
	 */
	/* s = splsched(); */	/* Protect from timer expirations */
	KDASSERT(vp->savp_lwp == l);
	/*
	 * If we were told to make an upcall or exit before
	 * the splsched(), make sure we process it instead of
	 * going to sleep. It might make more sense for this to
	 * be handled inside of tsleep....
	 */
	ret = 0;
	l->l_flag |= L_SA_YIELD;
	if (l->l_flag & L_SA_UPCALL) {
		/* KERNEL_PROC_UNLOCK(l); in upcallret() */
		upcallret(l);
		KERNEL_PROC_LOCK(l);
	}
	while (l->l_flag & L_SA_YIELD) {
		DPRINTFN(1,("sa_yield(%d.%d) really going dormant\n",
			     p->p_pid, l->l_lid));

		simple_lock(&sa->sa_lock);
		sa->sa_concurrency--;
		simple_unlock(&sa->sa_lock);

		ret = tsleep(l, PUSER | PCATCH, "sawait", 0);

		simple_lock(&sa->sa_lock);
		sa->sa_concurrency++;
		simple_unlock(&sa->sa_lock);

		KDASSERT(vp->savp_lwp == l || p->p_flag & P_WEXIT);

		/* KERNEL_PROC_UNLOCK(l); in upcallret() */
		upcallret(l);
		KERNEL_PROC_LOCK(l);
	}
	/* splx(s); */
	DPRINTFN(1,("sa_yield(%d.%d) returned, ret %d, userret %p\n",
		     p->p_pid, l->l_lid, ret, p->p_userret));
}


/*
 * sys_sa_preempt - preempt a running thread
 *
 * Given an lwp id, send it a user upcall. This is a way for libpthread to
 * kick something into the upcall handler.
 */
int
sys_sa_preempt(struct lwp *l, void *v, register_t *retval)
{
	struct sys_sa_preempt_args /* {
		syscallarg(int) sa_id;
	} */ *uap = v;
	struct sadata		*sa = l->l_proc->p_sa;
	struct lwp		*t;
	int			target, s, error;

	DPRINTFN(11,("sys_sa_preempt(%d.%d)\n", l->l_proc->p_pid,
		     l->l_lid));

	/* We have to be using scheduler activations */
	if (sa == NULL)
		return (EINVAL);

	if ((l->l_proc->p_flag & P_SA) == 0)
		return (EINVAL);

	if ((target = SCARG(uap, sa_id)) < 1)
		return (EINVAL);

	SCHED_LOCK(s);

	LIST_FOREACH(t, &l->l_proc->p_lwps, l_sibling)
		if (t->l_lid == target)
			break;

	if (t == NULL) {
		error = ESRCH;
		goto exit_lock;
	}

	/* XXX WRS We really need all of this locking documented */
	SCHED_UNLOCK(s);

	error = sa_upcall(l, SA_UPCALL_USER | SA_UPCALL_DEFER_EVENT, l, NULL,
		0, NULL, NULL);
	if (error)
		return error;

	return 0;

exit_lock:
	SCHED_UNLOCK(s);

	return error;
}


/* XXX Hm, naming collision. */
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

	/* XXX prevent recursive upcalls if we sleep for memory */
	SA_LWP_STATE_LOCK(l, f);
	sast = sa_getstack(sa);
	SA_LWP_STATE_UNLOCK(l, f);
	if (sast == NULL) {
		return (ENOMEM);
	}
	DPRINTFN(9,("sa_upcall(%d.%d) using stack %p\n",
	    l->l_proc->p_pid, l->l_lid, sast->sast_stack.ss_sp));

	if (l->l_proc->p_emul->e_sa->sae_upcallconv) {
		error = (*l->l_proc->p_emul->e_sa->sae_upcallconv)(l, type,
		    &argsize, &arg, &func);
		if (error)
			return error;
	}

	SA_LWP_STATE_LOCK(l, f);
	sau = sadata_upcall_alloc(1);
	SA_LWP_STATE_UNLOCK(l, f);
	sa_upcall0(sau, type, event, interrupted, argsize, arg, func);
	sau->sau_stack = sast->sast_stack;

	SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);
	l->l_flag |= L_SA_UPCALL;

	return (0);
}

static void
sa_upcall0(struct sadata_upcall *sau, int type, struct lwp *event,
    struct lwp *interrupted, size_t argsize, void *arg, void (*func)(void *))
{

	KDASSERT((event == NULL) || (event != interrupted));

	sau->sau_flags = 0;

	if (type & SA_UPCALL_DEFER_EVENT) {
		sau->sau_event.ss_deferred.ss_lwp = event;
		sau->sau_flags |= SAU_FLAG_DEFERRED_EVENT;
	} else
		sa_upcall_getstate(&sau->sau_event, event);
	if (type & SA_UPCALL_DEFER_INTERRUPTED) {
		sau->sau_interrupted.ss_deferred.ss_lwp = interrupted;
		sau->sau_flags |= SAU_FLAG_DEFERRED_INTERRUPTED;
	} else
		sa_upcall_getstate(&sau->sau_interrupted, interrupted);

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
 * WRS: unwaware of any locking before entry!
 *	We set L_SA_SWITCHING on the target lwp. We assume that l_flag is
 * protected by the kernel lock and untouched in interrupt context.
 */
static void
sa_upcall_getstate(union sau_state *ss, struct lwp *l)
{
	caddr_t sp;
	size_t ucsize;

	if (l) {
		l->l_flag |= L_SA_SWITCHING;
		(*l->l_proc->p_emul->e_sa->sae_getucontext)(l,
		    (void *)&ss->ss_captured.ss_ctx);
		l->l_flag &= ~L_SA_SWITCHING;
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

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;

	KDASSERT(vp->savp_lwp == l);

	if (vp->savp_faultaddr == vp->savp_ofaultaddr) {
		DPRINTFN(10,("sa_pagefault(%d.%d) double page fault\n",
			     p->p_pid, l->l_lid));
		return 1;
	}

	sast.sast_stack.ss_sp = (*p->p_emul->e_sa->sae_ucsp)(l_ctx);
	sast.sast_stack.ss_size = 1;

	if (SPLAY_FIND(sasttree, &sa->sa_stackstree, &sast)) {
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
 * Called by tsleep() when it wants to call mi_switch().
 * Block current LWP and switch to another.
 *
 * WE ARE NOT ALLOWED TO SLEEP HERE!  WE ARE CALLED FROM WITHIN
 * TSLEEP() ITSELF!  We are called with sched_lock held, and must
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
sa_switch(struct lwp *l, int type)
{
	struct proc *p = l->l_proc;
	struct sadata_vp *vp = l->l_savp;
	struct sadata_upcall *sau = NULL;
	struct lwp *l2;
	int s;

	DPRINTFN(4,("sa_switch(%d.%d type %d VP %d)\n", p->p_pid, l->l_lid,
	    type, vp->savp_lwp ? vp->savp_lwp->l_lid : 0));

	SCHED_ASSERT_LOCKED();

	if (p->p_flag & P_WEXIT) {
		mi_switch(l, NULL);
		return;
	}

	if (l->l_flag & L_SA_YIELD) {

		/*
		 * Case 0: we're blocking in sa_yield
		 */
		if (vp->savp_wokenq_head == NULL && p->p_userret == NULL) {
			l->l_flag |= L_SA_IDLE;
			mi_switch(l, NULL);
		} else {
			/* make us running again. */
			unsleep(l);
			l->l_stat = LSONPROC;
			l->l_proc->p_nrlwps++;
			s = splsched();
			SCHED_UNLOCK(s);
		}
		return;
	} else if (vp->savp_lwp == l) {
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
			mi_switch(l, NULL);
			return;
		}

		/*
		 * The process of allocating a new LWP could cause
		 * sleeps. We're called from inside sleep, so that
		 * would be Bad. Therefore, we must use a cached new
		 * LWP. The first thing that this new LWP must do is
		 * allocate another LWP for the cache.  */
		l2 = sa_getcachelwp(vp);
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
			sadata_upcall_free(sau);
			return;
		}

		cpu_setfunc(l2, sa_switchcall, sau);
		sa_upcall0(sau, SA_UPCALL_BLOCKED, l, NULL, 0, NULL, NULL);

		/*
		 * Perform the double/upcall pagefault check.
		 * We do this only here since we need l's ucontext to
		 * get l's userspace stack. sa_upcall0 above has saved
		 * it for us.
		 * The L_SA_PAGEFAULT flag is set in the MD
		 * pagefault code to indicate a pagefault.  The MD
		 * pagefault code also saves the faultaddr for us.
		 */
		if ((l->l_flag & L_SA_PAGEFAULT) && sa_pagefault(l,
			&sau->sau_event.ss_captured.ss_ctx) != 0) {
			cpu_setfunc(l2, sa_switchcall, NULL);
			sa_putcachelwp(p, l2); /* PHOLD from sa_getcachelwp */
			mi_switch(l, NULL);
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
			if (vp->savp_faultaddr == vp->savp_ofaultaddr)
				vp->savp_ofaultaddr = -1;
			return;
		}

		DPRINTFN(8,("sa_switch(%d.%d) blocked upcall %d\n",
			     p->p_pid, l->l_lid, l2->l_lid));

		l->l_flag |= L_SA_BLOCKING;
		l2->l_priority = l2->l_usrpri;
		vp->savp_blocker = l;
		vp->savp_lwp = l2;
		setrunnable(l2);
		PRELE(l2); /* Remove the artificial hold-count */

		KDASSERT(l2 != l);
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
		panic("sa_vp empty");
	}

	DPRINTFN(4,("sa_switch(%d.%d) switching to LWP %d.\n",
	    p->p_pid, l->l_lid, l2 ? l2->l_lid : 0));
	mi_switch(l, l2);
	DPRINTFN(4,("sa_switch(%d.%d flag %x) returned.\n",
	    p->p_pid, l->l_lid, l->l_flag));
	KDASSERT(l->l_wchan == 0);

	SCHED_ASSERT_UNLOCKED();
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
	int s;

	l2 = curlwp;
	p = l2->l_proc;
	vp = l2->l_savp;
	sau = arg;

	if (p->p_flag & P_WEXIT) {
		sadata_upcall_free(sau);
		lwp_exit(l2);
	}

	KDASSERT(vp->savp_lwp == l2);
	DPRINTFN(6,("sa_switchcall(%d.%d)\n", p->p_pid, l2->l_lid));

	l2->l_flag &= ~L_SA;
	if (LIST_EMPTY(&vp->savp_lwpcache)) {
		/* Allocate the next cache LWP */
		DPRINTFN(6,("sa_switchcall(%d.%d) allocating LWP\n",
		    p->p_pid, l2->l_lid));
		sa_newcachelwp(l2);
	}
	if (sau) {
		l = vp->savp_blocker;
		sast = sa_getstack(p->p_sa);
		if (sast) {
			sau->sau_stack = sast->sast_stack;
			SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);
			l2->l_flag |= L_SA_UPCALL;
		} else {
			/*
			 * Oops! We're in trouble. The app hasn't
			 * passeed us in any stacks on which to deliver
			 * the upcall.
			 *
			 * WRS: I think this code is wrong. If we can't
			 * get a stack, we are dead. We either need
			 * to block waiting for one (assuming there's a
			 * live vp still in userland so it can hand back
			 * stacks, or we should just kill the process
			 * as we're deadlocked.
			 */
#ifdef DIAGNOSTIC
			printf("sa_switchcall(%d.%d flag %x): Not enough stacks.\n",
			    p->p_pid, l->l_lid, l->l_flag);
#endif
			if (vp->savp_sleeper_upcall == NULL)
				vp->savp_sleeper_upcall = sau;
			else
				sadata_upcall_free(sau);
			PHOLD(l2);
			SCHED_LOCK(s);
			sa_putcachelwp(p, l2); /* sets L_SA */
			vp->savp_lwp = l;
			l->l_flag &= ~L_SA_BLOCKING;
			p->p_nrlwps--;
			mi_switch(l2, NULL);
			/* mostly NOTREACHED */
			SCHED_ASSERT_UNLOCKED();
			splx(s);
		}
	}
	l2->l_flag |= L_SA;

	upcallret(l2);
}

/*
 * sa_newcachelwp
 *	Allocate a new lwp, attach it to l's vp, and add it to
 * the vp's idle cache.
 *	Assumes no locks (other than kernel lock) on entry and exit.
 * Locks scheduler lock during operation.
 *	Returns 0 on success or if process is exiting. Returns ENOMEM
 * if it is unable to allocate a new uarea.
 */
static int
sa_newcachelwp(struct lwp *l)
{
	struct proc *p;
	struct lwp *l2;
	vaddr_t uaddr;
	boolean_t inmem;
	int s;

	p = l->l_proc;
	if (p->p_flag & P_WEXIT)
		return (0);

	inmem = uvm_uarea_alloc(&uaddr);
	if (__predict_false(uaddr == 0)) {
		return (ENOMEM);
	} else {
		newlwp(l, p, uaddr, inmem, 0, NULL, 0, child_return, 0, &l2);
		/* We don't want this LWP on the process's main LWP list, but
		 * newlwp helpfully puts it there. Unclear if newlwp should
		 * be tweaked.
		 */
		PHOLD(l2);
		SCHED_LOCK(s);
		l2->l_savp = l->l_savp;
		sa_putcachelwp(p, l2);
		SCHED_UNLOCK(s);
	}

	return (0);
}

/*
 * sa_putcachelwp
 *	Take a normal process LWP and place it in the SA cache.
 * LWP must not be running!
 *	Scheduler lock held on entry and exit.
 */
void
sa_putcachelwp(struct proc *p, struct lwp *l)
{
	struct sadata_vp *vp;

	SCHED_ASSERT_LOCKED();

	vp = l->l_savp;

	LIST_REMOVE(l, l_sibling);
	p->p_nlwps--;
	l->l_stat = LSSUSPENDED;
	l->l_flag |= (L_DETACHED | L_SA);
	/* XXX lock sadata */
	DPRINTFN(5,("sa_putcachelwp(%d.%d) Adding LWP %d to cache\n",
	    p->p_pid, curlwp->l_lid, l->l_lid));
	LIST_INSERT_HEAD(&vp->savp_lwpcache, l, l_sibling);
	vp->savp_ncached++;
	/* XXX unlock */
}

/*
 * sa_getcachelwp
 *	Fetch a LWP from the cache.
 * Scheduler lock held on entry and exit.
 */
struct lwp *
sa_getcachelwp(struct sadata_vp *vp)
{
	struct lwp *l;
	struct proc *p;

	SCHED_ASSERT_LOCKED();

	l = NULL;
	/* XXX lock sadata */
	if (vp->savp_ncached > 0) {
		vp->savp_ncached--;
		l = LIST_FIRST(&vp->savp_lwpcache);
		LIST_REMOVE(l, l_sibling);
		p = l->l_proc;
		LIST_INSERT_HEAD(&p->p_lwps, l, l_sibling);
		p->p_nlwps++;
		DPRINTFN(5,("sa_getcachelwp(%d.%d) Got LWP %d from cache.\n",
		    p->p_pid, curlwp->l_lid, l->l_lid));
	}
	/* XXX unlock */
	return l;
}


void
sa_unblock_userret(struct lwp *l)
{
	struct proc *p;
	struct lwp *l2;
	struct sadata *sa;
	struct sadata_vp *vp;
	struct sadata_upcall *sau;
	struct sastack *sast;
	int f, s;

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;

	if (p->p_flag & P_WEXIT)
		return;

	SCHED_ASSERT_UNLOCKED();

	KERNEL_PROC_LOCK(l);
	SA_LWP_STATE_LOCK(l, f);

	DPRINTFN(7,("sa_unblock_userret(%d.%d %x) \n", p->p_pid, l->l_lid,
	    l->l_flag));

	sa_setwoken(l);
	/* maybe NOTREACHED */

	SCHED_LOCK(s);
	if (l != vp->savp_lwp) {
		/* Invoke an "unblocked" upcall */
		DPRINTFN(8,("sa_unblock_userret(%d.%d) unblocking\n",
		    p->p_pid, l->l_lid));

		l2 = sa_vp_repossess(l);

		SCHED_UNLOCK(s);

		if (l2 == NULL)
			lwp_exit(l);

		sast = sa_getstack(sa);
		if (p->p_flag & P_WEXIT)
			lwp_exit(l);

		sau = sadata_upcall_alloc(1);
		if (p->p_flag & P_WEXIT) {
			sadata_upcall_free(sau);
			lwp_exit(l);
		}

		KDASSERT(l2 != NULL);
		PHOLD(l2);

		KDASSERT(sast != NULL);
		DPRINTFN(9,("sa_unblock_userret(%d.%d) using stack %p\n",
		    l->l_proc->p_pid, l->l_lid, sast->sast_stack.ss_sp));

		/*
		 * Defer saving the event lwp's state because a
		 * PREEMPT upcall could be on the queue already.
		 */
		sa_upcall0(sau, SA_UPCALL_UNBLOCKED | SA_UPCALL_DEFER_EVENT,
			   l, l2, 0, NULL, NULL);
		sau->sau_stack = sast->sast_stack;

		SCHED_LOCK(s);
		SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);
		l->l_flag |= L_SA_UPCALL;
		l->l_flag &= ~L_SA_BLOCKING;
		sa_putcachelwp(p, l2);
	}
	SCHED_UNLOCK(s);

	SA_LWP_STATE_UNLOCK(l, f);
	KERNEL_PROC_UNLOCK(l);
}

/*
 * sa_upcall_userret
 *	We are about to exit the kernel and return to userland, and
 * userret() noticed we have upcalls pending. So deliver them.
 *
 *	This is the place where unblocking upcalls get generated. We
 * allocate the stack & upcall event here. We may block doing so, but
 * we lock our LWP state (clear L_SA for the moment) while doing so.
 *
 *	In the case of delivering multiple upcall events, we will end up
 * writing multiple stacks out to userland at once. The last one we send
 * out will be the first one run, then it will notice the others and
 * run them.
 *
 * No locks held on entry or exit. We lock the scheduler during processing.
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
	int f, s;

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;

	SCHED_ASSERT_UNLOCKED();

	KERNEL_PROC_LOCK(l);
	SA_LWP_STATE_LOCK(l, f);

	DPRINTFN(7,("sa_upcall_userret(%d.%d %x) \n", p->p_pid, l->l_lid,
	    l->l_flag));

	KDASSERT((l->l_flag & L_SA_BLOCKING) == 0);

	sast = NULL;
	if (SIMPLEQ_EMPTY(&vp->savp_upcalls) && vp->savp_wokenq_head != NULL) {
		sast = sa_getstack(sa);
		if (sast == NULL) {
			SA_LWP_STATE_UNLOCK(l, f);
			KERNEL_PROC_UNLOCK(l);
			preempt(1);
			return;
		}
	}
	SCHED_LOCK(s);
	if (SIMPLEQ_EMPTY(&vp->savp_upcalls) && vp->savp_wokenq_head != NULL &&
	    sast != NULL) {
		/* Invoke an "unblocked" upcall */
		l2 = vp->savp_wokenq_head;
		vp->savp_wokenq_head = l2->l_forw;

		DPRINTFN(9,("sa_upcall_userret(%d.%d) using stack %p\n",
		    l->l_proc->p_pid, l->l_lid, sast->sast_stack.ss_sp));

		SCHED_UNLOCK(s);

		if (p->p_flag & P_WEXIT)
			lwp_exit(l);

		DPRINTFN(8,("sa_upcall_userret(%d.%d) unblocking %d\n",
		    p->p_pid, l->l_lid, l2->l_lid));

		sau = sadata_upcall_alloc(1);
		if (p->p_flag & P_WEXIT) {
			sadata_upcall_free(sau);
			lwp_exit(l);
		}

		sa_upcall0(sau, SA_UPCALL_UNBLOCKED, l2, l, 0, NULL, NULL);
		sau->sau_stack = sast->sast_stack;

		SIMPLEQ_INSERT_TAIL(&vp->savp_upcalls, sau, sau_next);

		l2->l_flag &= ~L_SA_BLOCKING;
		SCHED_LOCK(s);
		sa_putcachelwp(p, l2); /* PHOLD from sa_setwoken */
		SCHED_UNLOCK(s);
	} else {
		SCHED_UNLOCK(s);
		if (sast)
			sa_setstackfree(sast, sa);
	}

	KDASSERT(vp->savp_lwp == l);

	while (!SIMPLEQ_EMPTY(&vp->savp_upcalls))
		sa_makeupcalls(l);

	if (vp->savp_wokenq_head == NULL)
		l->l_flag &= ~L_SA_UPCALL;

	SA_LWP_STATE_UNLOCK(l, f);
	KERNEL_PROC_UNLOCK(l);
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
 *	KERNEL_PROC_LOCK should be held on entry and exit, and
 * SA_LWP_STATE should also be locked (L_SA temporarily disabled).
 *
 *	If the enqueued event was DEVERED, this is the time when we set
 * up the upcall event's state.
 */
static inline void
sa_makeupcalls(struct lwp *l)
{
	struct lwp *l2, *eventq;
	struct proc *p;
	const struct sa_emul *sae;
	struct sadata *sa;
	struct sadata_vp *vp;
	uintptr_t sapp, sap;
	struct sa_t self_sa;
	struct sa_t *sas[3];
	struct sadata_upcall *sau;
	void *stack, *ap;
	union sau_state *e_ss;
	ucontext_t *kup, *up;
	size_t sz, ucsize;
	int i, nint, nevents, s, type, error;

	p = l->l_proc;
	sae = p->p_emul->e_sa;
	sa = p->p_sa;
	vp = l->l_savp;
	ucsize = sae->sae_ucsize;

	sau = SIMPLEQ_FIRST(&vp->savp_upcalls);
	SIMPLEQ_REMOVE_HEAD(&vp->savp_upcalls, sau_next);

	if (sau->sau_flags & SAU_FLAG_DEFERRED_EVENT)
		sa_upcall_getstate(&sau->sau_event,
		    sau->sau_event.ss_deferred.ss_lwp);
	if (sau->sau_flags & SAU_FLAG_DEFERRED_INTERRUPTED)
		sa_upcall_getstate(&sau->sau_interrupted,
		    sau->sau_interrupted.ss_deferred.ss_lwp);

#ifdef __MACHINE_STACK_GROWS_UP
	stack = sau->sau_stack.ss_sp;
#else
	stack = (caddr_t)sau->sau_stack.ss_sp + sau->sau_stack.ss_size;
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
#ifdef DIAGNOSTIC
			printf("sa_makeupcalls(%d.%d): couldn't copyout"
			    " context of event LWP %d\n",
			    p->p_pid, l->l_lid,
			    sau->sau_event.ss_captured.ss_sa.sa_id);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		sas[1] = &sau->sau_event.ss_captured.ss_sa;
		nevents = 1;
	}
	if (sau->sau_interrupted.ss_captured.ss_sa.sa_context != NULL) {
		KDASSERT(sau->sau_interrupted.ss_captured.ss_sa.sa_context !=
		    sau->sau_event.ss_captured.ss_sa.sa_context);
		if (copyout(&sau->sau_interrupted.ss_captured.ss_ctx,
		    sau->sau_interrupted.ss_captured.ss_sa.sa_context,
		    ucsize) != 0) {
#ifdef DIAGNOSTIC
			printf("sa_makeupcalls(%d.%d): couldn't copyout"
			    " context of interrupted LWP %d\n",
			    p->p_pid, l->l_lid,
			    sau->sau_interrupted.ss_captured.ss_sa.sa_id);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
		sas[2] = &sau->sau_interrupted.ss_captured.ss_sa;
		nint = 1;
	}
	eventq = NULL;
	if (sau->sau_type == SA_UPCALL_UNBLOCKED) {
		SCHED_LOCK(s);
		eventq = vp->savp_wokenq_head;
		vp->savp_wokenq_head = NULL;
		SCHED_UNLOCK(s);
		l2 = eventq;
		while (l2 != NULL) {
			nevents++;
			l2 = l2->l_forw;
		}
	}

	/* Copy out the activation's ucontext */
	up = (void *)STACK_ALLOC(stack, ucsize);
	stack = STACK_GROW(stack, ucsize);
	kup = kmem_zalloc(sizeof(*kup), KM_SLEEP);
	kup->uc_stack = sau->sau_stack;
	kup->uc_flags = _UC_STACK;
	error = SACOPYOUT(sae, SAOUT_UCONTEXT, kup, up);
	kmem_free(kup, sizeof(*kup));
	if (error) {
		sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
		printf("sa_makeupcalls: couldn't copyout activation"
		    " ucontext for %d.%d to %p\n", l->l_proc->p_pid, l->l_lid,
		    up);
#endif
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

	KDASSERT(nint <= 1);
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
			KDASSERT(sau->sau_type == SA_UPCALL_UNBLOCKED);
			KDASSERT(eventq != NULL);
			l2 = eventq;
			KDASSERT(l2 != NULL);
			eventq = l2->l_forw;
			DPRINTFN(8,
			    ("sa_makeupcalls(%d.%d) unblocking extra %d\n",
			    p->p_pid, l->l_lid, l2->l_lid));
			if (e_ss == NULL) {
				e_ss = kmem_alloc(sizeof(*e_ss), KM_SLEEP);
			}
			sa_upcall_getstate(e_ss, l2);
			SCHED_LOCK(s);
			l2->l_flag &= ~L_SA_BLOCKING;
			sa_putcachelwp(p, l2); /* PHOLD from sa_setwoken */
			SCHED_UNLOCK(s);

			error = copyout(&e_ss->ss_captured.ss_ctx,
			    e_ss->ss_captured.ss_sa.sa_context, ucsize);
			sasp = &e_ss->ss_captured.ss_sa;
		}
		if (error != 0 ||
		    SACOPYOUT(sae, SAOUT_SA_T, sasp, sap) ||
		    SACOPYOUT(sae, SAOUT_SAP_T, &sap, sapp)) {
			/* Copying onto the stack didn't work. Die. */
			sadata_upcall_free(sau);
#ifdef DIAGNOSTIC
			printf("sa_makeupcalls(%d.%d): couldn't copyout\n",
			    p->p_pid, l->l_lid);
#endif
			if (e_ss != NULL) {
				kmem_free(e_ss, sizeof(*e_ss));
			}
			sigexit(l, SIGILL);
			/* NOTREACHED */
		}
	}
	if (e_ss != NULL) {
		kmem_free(e_ss, sizeof(*e_ss));
	}
	KDASSERT(eventq == NULL);

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
#ifdef DIAGNOSTIC
			printf("sa_makeupcalls(%d.%d): couldn't copyout"
			    " sadata_upcall arg %p size %ld to %p \n",
			    p->p_pid, l->l_lid,
			    sau->sau_arg, (long) sau->sau_argsize, ap);
#endif
			sigexit(l, SIGILL);
			/* NOTREACHED */
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

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SAUPCALL))
		ktrsaupcall(l, type, nevents, nint, (void *)sapp, ap);
#endif
	(*sae->sae_upcall)(l, type, nevents, nint, (void *)sapp, ap, stack,
	    sa->sa_upcall);

	l->l_flag &= ~L_SA_YIELD;
}

static void
sa_setwoken(struct lwp *l)
{
	struct lwp *l2, *vp_lwp;
	struct proc *p;
	struct sadata *sa;
	struct sadata_vp *vp;
	int s;

	SCHED_LOCK(s);

	if ((l->l_flag & L_SA_BLOCKING) == 0) {
		SCHED_UNLOCK(s);
		return;
	}

	p = l->l_proc;
	sa = p->p_sa;
	vp = l->l_savp;
	vp_lwp = vp->savp_lwp;
	l2 = NULL;

	KDASSERT(vp_lwp != NULL);
	DPRINTFN(3,("sa_setwoken(%d.%d) woken, flags %x, vp %d\n",
		     l->l_proc->p_pid, l->l_lid, l->l_flag,
		     vp_lwp->l_lid));

#if notyet
	if (vp_lwp->l_flag & L_SA_IDLE) {
		KDASSERT((vp_lwp->l_flag & L_SA_UPCALL) == 0);
		KDASSERT(vp->savp_wokenq_head == NULL);
		DPRINTFN(3,
		    ("sa_setwoken(%d.%d) repossess: idle vp_lwp %d state %d\n",
		    l->l_proc->p_pid, l->l_lid,
		    vp_lwp->l_lid, vp_lwp->l_stat));
		vp_lwp->l_flag &= ~L_SA_IDLE;
		SCHED_UNLOCK(s);
		return;
	}
#endif

	DPRINTFN(3,("sa_setwoken(%d.%d) put on wokenq: vp_lwp %d state %d\n",
		     l->l_proc->p_pid, l->l_lid, vp_lwp->l_lid,
		     vp_lwp->l_stat));

	PHOLD(l);
	if (vp->savp_wokenq_head == NULL)
		vp->savp_wokenq_head = l;
	else
		*vp->savp_wokenq_tailp = l;
	*(vp->savp_wokenq_tailp = &l->l_forw) = NULL;

	switch (vp_lwp->l_stat) {
	case LSONPROC:
		if (vp_lwp->l_flag & L_SA_UPCALL)
			break;
		vp_lwp->l_flag |= L_SA_UPCALL;
		if (vp_lwp->l_flag & L_SA_YIELD)
			break;
		/* XXX IPI vp_lwp->l_cpu */
		break;
	case LSSLEEP:
		if (vp_lwp->l_flag & L_SA_IDLE) {
			vp_lwp->l_flag &= ~L_SA_IDLE;
			vp_lwp->l_flag |= L_SA_UPCALL;
			setrunnable(vp_lwp);
			break;
		}
		vp_lwp->l_flag |= L_SA_UPCALL;
		break;
	case LSSUSPENDED:
#ifdef DIAGNOSTIC
		printf("sa_setwoken(%d.%d) vp lwp %d LSSUSPENDED\n",
		    l->l_proc->p_pid, l->l_lid, vp_lwp->l_lid);
#endif
		break;
	case LSSTOP:
		vp_lwp->l_flag |= L_SA_UPCALL;
		break;
	case LSRUN:
		if (vp_lwp->l_flag & L_SA_UPCALL)
			break;
		vp_lwp->l_flag |= L_SA_UPCALL;
		if (vp_lwp->l_flag & L_SA_YIELD)
			break;
		if (vp_lwp->l_slptime > 1) {
			void updatepri(struct lwp *);
			updatepri(vp_lwp);
		}
		vp_lwp->l_slptime = 0;
		if (vp_lwp->l_flag & L_INMEM) {
			if (vp_lwp->l_cpu == curcpu())
				l2 = vp_lwp;
			else
				need_resched(vp_lwp->l_cpu);
		} else
			sched_wakeup(&proc0);
		break;
	default:
		panic("sa_vp LWP not sleeping/onproc/runnable");
	}

	l->l_stat = LSSUSPENDED;
	p->p_nrlwps--;
	mi_switch(l, l2);
	/* maybe NOTREACHED */
	SCHED_ASSERT_UNLOCKED();
	splx(s);
	if (p->p_flag & P_WEXIT)
		lwp_exit(l);
}

static struct lwp *
sa_vp_repossess(struct lwp *l)
{
	struct lwp *l2;
	struct proc *p = l->l_proc;
	struct sadata_vp *vp = l->l_savp;

	SCHED_ASSERT_LOCKED();

	/*
	 * Put ourselves on the virtual processor and note that the
	 * previous occupant of that position was interrupted.
	 */
	l2 = vp->savp_lwp;
	vp->savp_lwp = l;
	if (l2) {
		if (l2->l_flag & L_SA_YIELD)
			l2->l_flag &= ~(L_SA_YIELD|L_SA_IDLE);

		DPRINTFN(1,("sa_vp_repossess(%d.%d) vp lwp %d state %d\n",
			     p->p_pid, l->l_lid, l2->l_lid, l2->l_stat));

		KDASSERT(l2 != l);
		switch (l2->l_stat) {
		case LSRUN:
			remrunqueue(l2);
			p->p_nrlwps--;
			break;
		case LSSLEEP:
			unsleep(l2);
			l2->l_flag &= ~L_SINTR;
			break;
		case LSSUSPENDED:
#ifdef DIAGNOSTIC
			printf("sa_vp_repossess(%d.%d) vp lwp %d LSSUSPENDED\n",
			    l->l_proc->p_pid, l->l_lid, l2->l_lid);
#endif
			break;
#ifdef DIAGNOSTIC
		default:
			panic("SA VP %d.%d is in state %d, not running"
			    " or sleeping\n", p->p_pid, l2->l_lid,
			    l2->l_stat);
#endif
		}
		l2->l_stat = LSSUSPENDED;
	}
	return l2;
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
	struct sadata_vp *vp;

	printf("Process %d (%s), state %d, address %p, flags %x\n",
	    p->p_pid, p->p_comm, p->p_stat, p, p->p_flag);
	printf("LWPs: %d (%d running, %d zombies)\n",
	    p->p_nlwps, p->p_nrlwps, p->p_nzlwps);
	LIST_FOREACH(l, &p->p_lwps, l_sibling)
	    debug_print_lwp(l);
	sa = p->p_sa;
	if (sa) {
		SLIST_FOREACH(vp, &sa->sa_vps, savp_next) {
			if (vp->savp_lwp)
				printf("SA VP: %d %s\n", vp->savp_lwp->l_lid,
				    vp->savp_lwp->l_flag & L_SA_YIELD ?
				    (vp->savp_lwp->l_flag & L_SA_IDLE ?
					"idle" : "yielding") : "");
			printf("SAs: %d cached LWPs\n", vp->savp_ncached);
			LIST_FOREACH(l, &vp->savp_lwpcache, l_sibling)
				debug_print_lwp(l);
		}
	}

	return 0;
}

int
debug_print_lwp(struct lwp *l)
{

	printf("LWP %d address %p ", l->l_lid, l);
	printf("state %d flags %x ", l->l_stat, l->l_flag);
	if (l->l_wchan)
		printf("wait %p %s", l->l_wchan, l->l_wmesg);
	printf("\n");

	return 0;
}

#endif
