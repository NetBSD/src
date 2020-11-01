/*	$NetBSD: sleeptab.h,v 1.2 2020/11/01 21:00:20 christos Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007, 2008, 2009, 2019, 2020
 *     The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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

#ifndef	_SYS_SLEEPTAB_H_
#define	_SYS_SLEEPTAB_H_

#define	SLEEPTAB_HASH_SHIFT	7
#define	SLEEPTAB_HASH_SIZE	(1 << SLEEPTAB_HASH_SHIFT)
#define	SLEEPTAB_HASH_MASK	(SLEEPTAB_HASH_SIZE - 1)
#define	SLEEPTAB_HASH(wchan)	(((uintptr_t)(wchan) >> 8) & SLEEPTAB_HASH_MASK)

LIST_HEAD(sleepq, lwp);

typedef struct sleeptab {
	sleepq_t	st_queue[SLEEPTAB_HASH_SIZE];
} sleeptab_t;

void	sleeptab_init(sleeptab_t *);

extern sleeptab_t	sleeptab;

#ifdef _KERNEL
/*
 * Find the correct sleep queue for the specified wait channel.  This
 * acquires and holds the per-queue interlock.
 */
static __inline sleepq_t *
sleeptab_lookup(sleeptab_t *st, wchan_t wchan, kmutex_t **mp)
{
	extern sleepqlock_t sleepq_locks[SLEEPTAB_HASH_SIZE];
	sleepq_t *sq;
	u_int hash;

	hash = SLEEPTAB_HASH(wchan);
	sq = &st->st_queue[hash];
	*mp = &sleepq_locks[hash].lock;
	mutex_spin_enter(*mp);
	return sq;
}

static __inline kmutex_t *
sleepq_hashlock(wchan_t wchan)
{
	extern sleepqlock_t sleepq_locks[SLEEPTAB_HASH_SIZE];
	kmutex_t *mp;

	mp = &sleepq_locks[SLEEPTAB_HASH(wchan)].lock;
	mutex_spin_enter(mp);
	return mp;
}

#define sleepq_destroy(a) __nothing

#endif

/*
 * Turnstiles, specialized sleep queues for use by kernel locks.
 */

typedef struct turnstile {
	LIST_ENTRY(turnstile)	ts_chain;	/* link on hash chain */
	struct turnstile	*ts_free;	/* turnstile free list */
	wchan_t			ts_obj;		/* lock object */
	sleepq_t		ts_sleepq[2];	/* sleep queues */
	u_int			ts_waiters[2];	/* count of waiters */

	/* priority inheritance */
	pri_t			ts_eprio;
	lwp_t			*ts_inheritor;
	SLIST_ENTRY(turnstile)	ts_pichain;
} turnstile_t;

LIST_HEAD(tschain, turnstile);

typedef struct tschain tschain_t;

#define	TS_READER_Q	0		/* reader sleep queue */
#define	TS_WRITER_Q	1		/* writer sleep queue */

#define	TS_WAITERS(ts, q)						\
	(ts)->ts_waiters[(q)]

#define	TS_ALL_WAITERS(ts)						\
	((ts)->ts_waiters[TS_READER_Q] +				\
	 (ts)->ts_waiters[TS_WRITER_Q])

#define	TS_FIRST(ts, q)	(LIST_FIRST(&(ts)->ts_sleepq[(q)]))

#ifdef	_KERNEL

void	turnstile_init(void);
turnstile_t	*turnstile_lookup(wchan_t);
void	turnstile_ctor(turnstile_t *);
void	turnstile_exit(wchan_t);
void	turnstile_block(turnstile_t *, int, wchan_t, syncobj_t *);
void	turnstile_wakeup(turnstile_t *, int, int, lwp_t *);
void	turnstile_print(volatile void *, void (*)(const char *, ...)
    __printflike(1, 2));
void	turnstile_unsleep(lwp_t *, bool);
void	turnstile_changepri(lwp_t *, pri_t);

extern struct pool turnstile_pool;
extern turnstile_t turnstile0;

#endif	/* _KERNEL */

#endif	/* _SYS_SLEEPTAB_H_ */
