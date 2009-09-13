/*	$NetBSD: savar.h,v 1.29 2009/09/13 18:45:12 pooka Exp $	*/

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

/*
 * Internal data usd by the scheduler activation implementation
 */

#ifndef _SYS_SAVAR_H_
#define _SYS_SAVAR_H_

#include <sys/lock.h>
#include <sys/tree.h>
#include <sys/queue.h>
#include <sys/sleepq.h>
#include <sys/sa.h>	/* needed for struct sa_t */

struct lwp;

union sau_state {
	struct {
		ucontext_t	ss_ctx;
		struct sa_t	ss_sa;
	} ss_captured;
	struct {
		struct lwp	*ss_lwp;
	} ss_deferred;
};

struct sa_emul {
	size_t		sae_ucsize;	/* Size of ucontext_t */
	size_t		sae_sasize;	/* Size of sa_t */
	size_t		sae_sapsize;	/* Size of (sa_t *) */
	int		(*sae_sacopyout)(int, const void *, void *);
	int		(*sae_upcallconv)(struct lwp *, int, size_t *, void **,
			    void (**)(void *));
	void		(*sae_upcall)(struct lwp *, int, int, int, void *,
			    void *, void *, sa_upcall_t);
	void		(*sae_getucontext)(struct lwp *, void *);
	void		*(*sae_ucsp)(void *); /* Stack ptr from an ucontext_t */
};

struct sadata_upcall {
	SIMPLEQ_ENTRY(sadata_upcall)	sau_next;
	int	sau_flags;
	int	sau_type;
	size_t	sau_argsize;
	void	*sau_arg;
	void	(*sau_argfreefunc)(void *);
	stack_t	sau_stack;
	union sau_state	sau_event;
	union sau_state	sau_interrupted;
};

#define	SAU_FLAG_DEFERRED_EVENT		0x1
#define	SAU_FLAG_DEFERRED_INTERRUPTED	0x2

#define	SA_UPCALL_TYPE_MASK		0x00FF

#define	SA_UPCALL_DEFER_EVENT		0x1000
#define	SA_UPCALL_DEFER_INTERRUPTED	0x2000
#define	SA_UPCALL_DEFER			(SA_UPCALL_DEFER_EVENT | \
					 SA_UPCALL_DEFER_INTERRUPTED)
#define	SA_UPCALL_LOCKED_EVENT		0x4000
#define	SA_UPCALL_LOCKED_INTERRUPTED	0x8000

struct sastack {
	stack_t			sast_stack;
	RB_ENTRY(sastack)	sast_node;
	unsigned int		sast_gen;
};

/*
 * Locking:
 *
 * m:	sadata::sa_mutex
 * p:	proc::p_lock
 * v:	sadata_vp::savp_mutex
 * (:	unlocked, stable
 * !:	unlocked, may only be reliably accessed by the blessed LWP itself
 */
struct sadata_vp {
	kmutex_t	savp_mutex;	/* (: mutex */
	int		savp_id;	/* (: "virtual processor" identifier */
	SLIST_ENTRY(sadata_vp)	savp_next; /* m: link to next sadata_vp */
	struct lwp	*savp_lwp;	/* !: lwp on "virtual processor" */
	struct lwp	*savp_blocker;	/* !: recently blocked lwp */
	sleepq_t	savp_woken;	/* m: list of unblocked lwps */
	sleepq_t	savp_lwpcache;  /* m: list of cached lwps */
	vaddr_t		savp_faultaddr;	/* !: page fault address */
	vaddr_t		savp_ofaultaddr; /* !: old page fault address */
	struct sadata_upcall	*savp_sleeper_upcall;
					/* !: cached upcall data */
	SIMPLEQ_HEAD(, sadata_upcall)	savp_upcalls; /* ?: pending upcalls */
	int		savp_woken_count;    /* m: count of woken lwps */
	int		savp_lwpcache_count; /* m: count of cached lwps */
	int		savp_pflags;	/* !: blessed-private flags */
};

#define	SAVP_FLAG_NOUPCALLS	0x0001	/* Already did upcalls, don't redo */
#define	SAVP_FLAG_DELIVERING	0x0002	/* Delivering an upcall, no block */

/*
 * Locking:
 *
 * m:	sadata::sa_mutex
 * p:	proc::p_lock
 * (:	unlocked, stable
 *
 *	Locking sadata::sa_vps is special. The list of vps is examined
 * in two locations, signal handling and timer processing, in which
 * proc::p_lock either is the best lock to use (signal handling) or an
 * unacceptable lock to use (timer processing, as we hold spinlocks when
 * locking the list, and p_lock can sleep; spinlocks while sleeping == BAD).
 * Actually changing the list of vps is exceptionally rare; it only happens
 * with concurrency > 1 and at app startup. So use both locks to write &
 * have the code to add vps grab both of them to actually change the list.
 */
struct sadata {
	kmutex_t	sa_mutex;		/* (: lock on these fields */
	int		sa_flag;		/* m: SA_* flags */
	sa_upcall_t	sa_upcall;		/* m: upcall entry point */
	int		sa_concurrency;		/* m: current concurrency */
	int		sa_maxconcurrency;	/* m: requested concurrency */
	int		sa_stackchg;		/* m: stacks change indicator */
	RB_HEAD(sasttree, sastack) sa_stackstree; /* s, m: tree of upcall stacks */
	struct sastack	*sa_stacknext;		/* m: next free stack */
	ssize_t 	sa_stackinfo_offset;	/* m: offset from ss_sp to stackinfo data */
	int		sa_nstacks;		/* m: number of upcall stacks */
	sigset_t	sa_sigmask;		/* p: process-wide masked sigs*/
	SLIST_HEAD(, sadata_vp)	sa_vps;		/* m,p: virtual processors */
	kcondvar_t	sa_cv;			/* m: condvar for sa_yield */
};

#define SA_FLAG_ALL	SA_FLAG_PREEMPT

#define	SA_MAXNUMSTACKS	16		/* Maximum number of upcall stacks per VP. */

void	sa_init(void);

struct sadata_upcall *sadata_upcall_alloc(int);
void	sadata_upcall_free(struct sadata_upcall *);
void	sadata_upcall_drain(void);

void	sa_awaken(struct lwp *);
void	sa_release(struct proc *);
void	sa_switch(struct lwp *);
void	sa_preempt(struct lwp *);
void	sa_yield(struct lwp *);
int	sa_upcall(struct lwp *, int, struct lwp *, struct lwp *, size_t, void *,
		  void (*)(void *));

void	sa_putcachelwp(struct proc *, struct lwp *);
struct lwp *sa_getcachelwp(struct proc *, struct sadata_vp *);

/*
 * API permitting other parts of the kernel to indicate that they
 * are entering code paths in which blocking events should NOT generate
 * upcalls to an SA process. These routines should ONLY be used by code
 * involved in scheduling or process/thread initialization (such as
 * stack copying). These calls must be balanced. They may be nested, but
 * MUST be released in a LIFO order. These calls assume that the lwp is
 * locked.
 */
typedef int sa_critpath_t;
void	sa_critpath_enter(struct lwp *, sa_critpath_t *);
void	sa_critpath_exit(struct lwp *, sa_critpath_t *);


void	sa_unblock_userret(struct lwp *);
void	sa_upcall_userret(struct lwp *);
void	cpu_upcall(struct lwp *, int, int, int, void *, void *, void *, sa_upcall_t);

typedef int (*sa_copyin_stack_t)(stack_t *, int, stack_t *);
int	sa_stacks1(struct lwp *, register_t *, int, stack_t *,
    sa_copyin_stack_t);
int	dosa_register(struct lwp *, sa_upcall_t, sa_upcall_t *, int, ssize_t);

void	*sa_ucsp(void *);

#define SAOUT_UCONTEXT	0
#define SAOUT_SA_T	1
#define SAOUT_SAP_T	2

#endif /* !_SYS_SAVAR_H_ */
