/*	$NetBSD: rcupdate.h,v 1.8 2021/12/19 11:49:11 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _LINUX_RCUPDATE_H_
#define _LINUX_RCUPDATE_H_

#include <sys/atomic.h>
#include <sys/cdefs.h>
#include <sys/systm.h>

#include <linux/bug.h>

#define	__rcu

#define	RCU_INIT_POINTER(P, V)	((P) = (V))

#define	rcu_assign_pointer(P,V)	atomic_store_release(&(P), (V))
#define	rcu_dereference(P)	atomic_load_consume(&(P))
#define	rcu_access_pointer(P) 	atomic_load_relaxed(&(P))

#define	rcu_dereference_raw	rcu_dereference

#define	rcu_dereference_protected(P, C) ({				      \
	WARN_ON(!(C));							      \
	(P);								      \
})

#define	rcu_replace_pointer(PTRP, NEWPTR, C) ({				      \
	__typeof__(PTRP) __rrp_ptrp = (PTRP);				      \
	__typeof__(PTRP) __rrp_oldptr =					      \
		rcu_dereference_protected(__rrp_ptrp, C);		      \
	rcu_assign_pointer(__rrp_ptrp, NEWPTR);				      \
	__rrp_oldptr;							      \
})


/* kill_dependency */
#define	rcu_pointer_handoff(P)	(P)

struct rcu_head {
	union {
		void		(*callback)(struct rcu_head *);
		void		*obj;
	}		rcuh_u;
	struct rcu_head	*rcuh_next;
};

#define	_kfree_rcu		linux__kfree_rcu
#define	call_rcu		linux_call_rcu
#define	cond_synchronize_rcu	linux_cond_synchronize_rcu
#define	destroy_rcu_head	linux_destroy_rcu_head
#define	init_rcu_head		linux_init_rcu_head
#define	get_state_synchronize_rcu linux_get_state_synchronize_rcu
#define	rcu_barrier		linux_rcu_barrier
#define	synchronize_rcu		linux_synchronize_rcu
#define	synchronize_rcu_expedited linux_synchronize_rcu_expedited

int	linux_rcu_gc_init(void);
void	linux_rcu_gc_fini(void);

void	init_rcu_head(struct rcu_head *);
void	destroy_rcu_head(struct rcu_head *);

void	call_rcu(struct rcu_head *, void (*)(struct rcu_head *));
void	rcu_barrier(void);
void	synchronize_rcu(void);
void	synchronize_rcu_expedited(void);

unsigned long get_state_synchronize_rcu(void);
void	cond_synchronize_rcu(unsigned long);

void	_kfree_rcu(struct rcu_head *, void *);

static inline void
rcu_read_lock(void)
{

	kpreempt_disable();	/* implies __insn_barrier */
}

static inline void
rcu_read_unlock(void)
{

	kpreempt_enable();	/* implies __insn_barrier */
}

#define	kfree_rcu(P, F)							      \
	_kfree_rcu(&(P)->F, (P))

#endif  /* _LINUX_RCUPDATE_H_ */
