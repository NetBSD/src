/*	$NetBSD: savar.h,v 1.14 2004/01/02 18:52:17 cl Exp $	*/

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

#ifndef _SYS_SAVAR_H
#define _SYS_SAVAR_H

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

struct sadata_upcall {
	SIMPLEQ_ENTRY(sadata_upcall)	sau_next;
	int	sau_flags;
	int	sau_type;
	size_t	sau_argsize;
	void	*sau_arg;
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

struct sadata {
	struct simplelock sa_lock;	/* lock on these fields */
	int	sa_flag;		/* SA_* flags */
	sa_upcall_t	sa_upcall;	/* upcall entry point */
	struct lwp	*sa_vp;		/* "virtual processor" allocation */
	struct lwp	*sa_vp_blocker;	/* recently blocked lwp */
	struct lwp	*sa_wokenq_head;	/* list of woken lwps */
	struct lwp	**sa_wokenq_tailp;	/* list of woken lwps */
	vaddr_t	sa_vp_faultaddr;	/* page fault address */
	vaddr_t	sa_vp_ofaultaddr;	/* old page fault address */
	int	sa_concurrency;		/* desired concurrency */
	LIST_HEAD(, lwp)	sa_lwpcache;	/* list of available lwps */
	int	sa_ncached;		/* list length */
	SPLAY_HEAD(sasttree, sastack) sa_stackstree; /* tree of upcall stacks */
	struct sastack	*sa_stacknext;	/* next free stack */
	ssize_t	sa_stackinfo_offset;	/* offset from ss_sp to stackinfo data */
	int	sa_nstacks;		/* number of upcall stacks */
	SIMPLEQ_HEAD(, sadata_upcall)	sa_upcalls; /* pending upcalls */
};

#define SA_FLAG_ALL	SA_FLAG_PREEMPT

extern struct pool sadata_pool;		/* memory pool for sadata structures */
extern struct pool saupcall_pool;	/* memory pool for pending upcalls */
extern struct pool sastack_pool;	/* memory pool for sastack structs */

#ifdef _KERNEL
#include <sys/mallocvar.h>

MALLOC_DECLARE(M_SA);
#endif

#define	SA_MAXNUMSTACKS	16		/* Maximum number of upcall stacks per VP. */

struct sadata_upcall *sadata_upcall_alloc(int);
void	sadata_upcall_free(struct sadata_upcall *);

void	sa_release(struct proc *);
void	sa_switch(struct lwp *, int);
void	sa_preempt(struct lwp *);
void	sa_yield(struct lwp *);
int	sa_upcall(struct lwp *, int, struct lwp *, struct lwp *, size_t, void *);

void	sa_putcachelwp(struct proc *, struct lwp *);
struct lwp *sa_getcachelwp(struct proc *);


void	sa_unblock_userret(struct lwp *);
void	sa_upcall_userret(struct lwp *);
void	cpu_upcall(struct lwp *, int, int, int, void *, void *, void *, sa_upcall_t);

#endif /* !_SYS_SAVAR_H */
