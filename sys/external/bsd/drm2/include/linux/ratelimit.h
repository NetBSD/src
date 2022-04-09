/*	$NetBSD: ratelimit.h,v 1.6 2022/04/09 23:44:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_RATELIMIT_H_
#define _LINUX_RATELIMIT_H_

#include <sys/atomic.h>
#include <sys/stdbool.h>
#include <sys/time.h>

#define	ratelimit_state	linux_ratelimit_state

struct ratelimit_state {
	volatile int		missed;

	volatile unsigned	rl_lock;
	struct timeval		rl_lasttime;
	int			rl_curpps;
	unsigned		rl_maxpps;
};

enum {
	RATELIMIT_MSG_ON_RELEASE,
};

/*
 * XXX Assumes hz=100 so this works in static initializers, and/or
 * hopes the caller just uses DEFAULT_RATELIMIT_INTERVAL and doesn't
 * mention hz.
 */
#define	DEFAULT_RATELIMIT_INTERVAL	(5*100)
#define	DEFAULT_RATELIMIT_BURST		10

#define	DEFINE_RATELIMIT_STATE(n, i, b)					      \
	struct ratelimit_state n = {					      \
		.missed = 0,						      \
		.rl_lock = 0,						      \
		.rl_lasttime = { .tv_sec = 0, .tv_usec = 0 },		      \
		.rl_curpps = 0,						      \
		.rl_maxpps = (b)/((i)/100),				      \
	}

static inline void
ratelimit_state_init(struct ratelimit_state *r, int interval, int burst)
{

	memset(r, 0, sizeof(*r));
	r->rl_maxpps = burst/(interval/hz);
}

static inline void
ratelimit_set_flags(struct ratelimit_state *r, unsigned long flags)
{
}

static inline bool
__ratelimit(struct ratelimit_state *r)
{
	int ok;

	if (atomic_swap_uint(&r->rl_lock, 1)) {
		ok = false;
		goto out;
	}
	membar_acquire();
	ok = ppsratecheck(&r->rl_lasttime, &r->rl_curpps, r->rl_maxpps);
	atomic_store_release(&r->rl_lock, 0);

out:	if (!ok)
		atomic_store_relaxed(&r->missed, 1);
	return ok;
}

#endif  /* _LINUX_RATELIMIT_H_ */
