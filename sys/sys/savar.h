/*	$NetBSD: savar.h,v 1.17.6.1 2006/04/22 11:40:19 simonb Exp $	*/

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

struct sastack {
	stack_t			sast_stack;
	SPLAY_ENTRY(sastack)	sast_node;
	unsigned int		sast_gen;
};

struct sadata_vp {
	int	savp_id;		/* "virtual processor" identifier */
	SLIST_ENTRY(sadata_vp)	savp_next; /* link to next sadata_vp */
	struct simplelock	savp_lock; /* lock on these fields */
	struct lwp	*savp_lwp;	/* lwp on "virtual processor" */
	struct lwp	*savp_blocker;	/* recently blocked lwp */
	struct lwp	*savp_wokenq_head; /* list of woken lwps */
	struct lwp	**savp_wokenq_tailp; /* list of woken lwps */
	vaddr_t	savp_faultaddr;		/* page fault address */
	vaddr_t	savp_ofaultaddr;	/* old page fault address */
	LIST_HEAD(, lwp)	savp_lwpcache; /* list of available lwps */
	int	savp_ncached;		/* list length */
	SIMPLEQ_HEAD(, sadata_upcall)	savp_upcalls; /* pending upcalls */
};

struct sadata {
	struct simplelock sa_lock;	/* lock on these fields */
	int	sa_flag;		/* SA_* flags */
	sa_upcall_t	sa_upcall;	/* upcall entry point */
	int	sa_concurrency;		/* current concurrency */
	int	sa_maxconcurrency;	/* requested concurrency */
	SPLAY_HEAD(sasttree, sastack) sa_stackstree; /* tree of upcall stacks */
	struct sastack	*sa_stacknext;	/* next free stack */
	ssize_t	sa_stackinfo_offset;	/* offset from ss_sp to stackinfo data */
	int	sa_nstacks;		/* number of upcall stacks */
	SLIST_HEAD(, sadata_vp)	sa_vps;	/* list of "virtual processors" */
};

#define SA_FLAG_ALL	SA_FLAG_PREEMPT

extern struct pool sadata_pool;		/* memory pool for sadata structures */
extern struct pool saupcall_pool;	/* memory pool for pending upcalls */
extern struct pool sastack_pool;	/* memory pool for sastack structs */
extern struct pool savp_pool;		/* memory pool for sadata_vp structures */

#ifdef _KERNEL
#include <sys/mallocvar.h>

MALLOC_DECLARE(M_SA);
#endif

#define	SA_MAXNUMSTACKS	16		/* Maximum number of upcall stacks per VP. */

struct sadata_upcall *sadata_upcall_alloc(int);
void	sadata_upcall_free(struct sadata_upcall *);

void	sa_release(struct proc *);
void	sa_switch(struct lwp *, struct sadata_upcall *, int);
void	sa_preempt(struct lwp *);
void	sa_yield(struct lwp *);
int	sa_upcall(struct lwp *, int, struct lwp *, struct lwp *, size_t, void *,
		  void (*)(void *));

void	sa_putcachelwp(struct proc *, struct lwp *);
struct lwp *sa_getcachelwp(struct sadata_vp *);


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
