/*	$NetBSD: seqlock.h,v 1.2 2021/12/19 00:47:40 riastradh Exp $	*/

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

#ifndef	_LINUX_SEQLOCK_H_
#define	_LINUX_SEQLOCK_H_

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/lock.h>

#include <lib/libkern/libkern.h>

struct seqlock {
	uint64_t	sql_gen;
};

typedef struct seqlock seqlock_t;

static inline void
seqlock_init(struct seqlock *seqlock)
{

	seqlock->sql_gen = 0;
}

static inline void
write_seqlock(struct seqlock *seqlock)
{

	KASSERT((seqlock->sql_gen & 1) == 0);
	seqlock->sql_gen |= 1;
	membar_producer();
}

static inline void
write_sequnlock(struct seqlock *seqlock)
{

	KASSERT((seqlock->sql_gen & 1) == 1);
	membar_producer();
	seqlock->sql_gen |= 1;	/* paraonia */
	seqlock->sql_gen++;
}

#define	write_seqlock_irqsave(SEQLOCK, FLAGS)	do {			      \
	(FLAGS) = (unsigned long)splvm();				      \
	write_seqlock(SEQLOCK);						      \
} while (0)

#define	write_sequnlock_irqrestore(SEQLOCK, FLAGS)	do {		      \
	write_seqlock(SEQLOCK);						      \
	splx((int)(FLAGS));						      \
} while (0)

static inline uint64_t
read_seqbegin(struct seqlock *seqlock)
{
	uint64_t gen;

	while ((gen = seqlock->sql_gen) & 1)
		SPINLOCK_BACKOFF_HOOK;
	membar_consumer();

	return gen;
}

static inline bool
read_seqretry(struct seqlock *seqlock, uint64_t gen)
{

	return gen != seqlock->sql_gen;
}

#endif	/* _LINUX_SEQLOCK_H_ */
