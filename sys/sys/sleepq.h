/*	$NetBSD: sleepq.h,v 1.1.2.1 2006/10/20 19:38:44 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
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

#ifndef	_SYS_SLEEPQ_H_
#define	_SYS_SLEEPQ_H_

#include <sys/queue.h>
#include <sys/mutex.h>

/*
 * Generic sleep queues.
 */

#define	SLEEPTAB_HASH_SIZE	128
#define	SLEEPTAB_HASH_MASK	(SLEEPTAB_HASH_SIZE - 1)
#define	SLEEPTAB_HASH(wchan)	(((uintptr_t)(wchan) >> 8) & SLEEPTAB_HASH_MASK)

struct lwp;

typedef struct sleepq {
	TAILQ_HEAD(, lwp)	sq_queue;	/* queue of waiters */
	kmutex_t		*sq_mutex;	/* mutex on struct */
	u_int			sq_waiters;	/* count of waiters */
} sleepq_t;

void	sleepq_init(sleepq_t *, kmutex_t *);
int	sleepq_remove(sleepq_t *, struct lwp *);
void	sleepq_enter(sleepq_t *, int, wchan_t, const char *, int, int);
int	sleepq_block(sleepq_t *, int);
void	sleepq_unsleep(struct lwp *);
void	sleepq_timeout(void *);
void	sleepq_wakeone(sleepq_t *, wchan_t);
void	sleepq_wakeall(sleepq_t *, wchan_t, u_int);
int	sleepq_abort(kmutex_t *, int);

void	sleeptab_init(void); 

/*
 * Return non-zero if it is unsafe to sleep.
 */
static inline int
sleepq_dontsleep(struct lwp *l)
{
	extern int cold;

	return cold || (doing_shutdown && (panicstr || l == NULL));
}

/*
 * Find the correct sleep queue for the the specified wait channel.  This
 * acquires and holds the per-queue interlock.
 */
static inline sleepq_t *
sleeptab_lookup(wchan_t wchan)
{
	extern sleepq_t sleeptab[];
	sleepq_t *sq;

	sq = &sleeptab[SLEEPTAB_HASH(wchan)];
	mutex_enter(sq->sq_mutex);
	return sq;
}

/*
 * Release the sleep queue interlock as acquired by sleeptab_lookup().
 */
static inline void
sleepq_unlock(sleepq_t *sq)
{

	mutex_exit(sq->sq_mutex);
}

/*
 * Turnstiles, specialized sleep queues for use by kernel locks.
 */

typedef struct turnstile {
	LIST_ENTRY(turnstile)	ts_chain;	/* link on hash chain */
	struct turnstile	*ts_free;	/* turnstile free list */
	volatile const void	*ts_obj;	/* lock object */
	sleepq_t		ts_sleepq[2];	/* sleep queues */
} turnstile_t;

typedef struct tschain {
	kmutex_t		tc_mutex;	/* mutex on hash chain */
	LIST_HEAD(, turnstile)	tc_chain;	/* turnstile chain */
} tschain_t;

#define	TS_READER_Q	0		/* reader sleep queue */
#define	TS_WRITER_Q	1		/* writer sleep queue */

#define	TS_WAITERS(ts, q)						\
	(ts)->ts_sleepq[(q)].sq_waiters

#define	TS_ALL_WAITERS(ts)						\
	((ts)->ts_sleepq[TS_READER_Q].sq_waiters +			\
	 (ts)->ts_sleepq[TS_WRITER_Q].sq_waiters)

#define	TS_FIRST(ts, q)	(TAILQ_FIRST(&(ts)->ts_sleepq[(q)].sq_queue))

#ifdef	_KERNEL

void	turnstile_init(void);
turnstile_t	*turnstile_lookup(wchan_t);
void	turnstile_exit(wchan_t);
void	turnstile_block(turnstile_t *, int, int, wchan_t);
void	turnstile_wakeup(turnstile_t *, int, int, struct lwp *);

extern struct pool_cache turnstile_cache; 

#endif	/* _KERNEL */

#endif	/* _SYS_SLEEPQ_H_ */
