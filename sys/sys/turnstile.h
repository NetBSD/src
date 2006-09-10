/*	$NetBSD: turnstile.h,v 1.1.2.1 2006/09/10 23:42:41 ad Exp $	*/

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

#ifndef	_SYS_TURNSTILE_H_
#define	_SYS_TURNSTILE_H_

#include <sys/sched.h>		/* for slpque */
#include <sys/queue.h>

/*
 * Turnstile sleep queue for kernel synchronization primitives.
 */
struct turnstile {
	LIST_ENTRY(turnstile) ts_chain;	/* link on hash chain */
	struct turnstile *ts_free;	/* turnstile free list */
	void		*ts_obj;	/* lock object */
	struct turnstile_sleepq {	/* queues of waiters */
		struct slpque tsq_q;
		u_int tsq_waiters;
	} ts_sleepq[2];
};

#define	TS_READER_Q	0		/* reader sleep queue */
#define	TS_WRITER_Q	1		/* writer sleep queue */

#define	TS_WAITERS(ts, q)						\
	(ts)->ts_sleepq[(q)].tsq_waiters

#define	TS_ALL_WAITERS(ts)						\
	((ts)->ts_sleepq[TS_READER_Q].tsq_waiters +			\
	 (ts)->ts_sleepq[TS_WRITER_Q].tsq_waiters)

#define	TS_FIRST(ts, q)	((ts)->ts_sleepq[(q)].tsq_q.sq_head)

#ifdef	_KERNEL

void	turnstile_init(void);

struct turnstile *turnstile_lookup(void *);
void	turnstile_exit(void *);
int	turnstile_block(struct turnstile *, int, int, void *, const char *);
void	turnstile_wakeup(struct turnstile *, int, int, struct lwp *);

extern struct pool_cache turnstile_cache;

#endif	/* _KERNEL */

#endif	/* _SYS_TURNSTILE_H_ */
