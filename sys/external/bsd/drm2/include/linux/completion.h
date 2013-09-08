/*	$NetBSD: completion.h,v 1.1.2.4 2013/09/08 16:00:50 riastradh Exp $	*/

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

#ifndef _LINUX_COMPLETION_H_
#define _LINUX_COMPLETION_H_

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/mutex.h>

#include <machine/limits.h>

struct completion {
	kmutex_t	c_lock;
	kcondvar_t	c_cv;

	/*
	 * c_done is either
	 *
	 *   . -1, meaning it's open season and we're done for good and
	 *     nobody need wait any more;
	 *
	 *   . 0, meaning nothing is done, so waiters must block; or
	 *
	 *   . a positive integer, meaning that many waiters can
	 *     proceed before further waiters must block.
	 *
	 * Negative values other than -1 are not allowed.
	 */
	int		c_done;
};

/*
 * Initialize a new completion object.
 */
static inline void
init_completion(struct completion *completion)
{

	mutex_init(&completion->c_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&completion->c_cv, "lnxcmplt");
	completion->c_done = 0;
}

/*
 * Destroy a completion object.
 */
static inline void
destroy_completion(struct completion *completion)
{
	KASSERT(!cv_has_waiters(&completion->c_cv));
	cv_destroy(&completion->c_cv);
}

/*
 * Notify one waiter of completion, but not any future ones.
 */
static inline void
complete(struct completion *completion)
{

	mutex_enter(&completion->c_lock);

	/* If it's not open season, wake one waiter.  */
	if (completion->c_done >= 0) {
		KASSERT(completion->c_done < INT_MAX); /* XXX check */
		completion->c_done++;
		cv_signal(&completion->c_cv);
	} else {
		KASSERT(completion->c_done == -1);
	}

	mutex_exit(&completion->c_lock);
}

/*
 * Notify all waiters, present and future (until INIT_COMPLETION), of
 * completion.
 */
static inline void
complete_all(struct completion *completion)
{

	mutex_enter(&completion->c_lock);

	/* If it's not open season, make it open season and wake everyone.  */
	if (completion->c_done >= 0) {
		completion->c_done = -1;
		cv_broadcast(&completion->c_cv);
	} else {
		KASSERT(completion->c_done == -1);
	}

	mutex_exit(&completion->c_lock);
}

/*
 * Reverse the effect of complete_all so that subsequent waiters block
 * until someone calls complete or complete_all.
 *
 * This operation is very different from its lowercase counterpart.
 *
 * For some reason this works on the completion object itself, not on a
 * pointer thereto, so it must be a macro.
 */
#define	INIT_COMPLETION(COMPLETION)	INIT_COMPLETION_blorp(&(COMPLETION))

static inline void
INIT_COMPLETION_blorp(struct completion *completion)
{

	mutex_enter(&completion->c_lock);
	completion->c_done = 0;
	/* No notify -- waiters are interested only in nonzero values.  */
	mutex_exit(&completion->c_lock);
}

/*
 * Wait interruptibly with a timeout for someone to call complete or
 * complete_all.
 */
static inline int
wait_for_completion_interruptible_timeout(struct completion *completion,
    unsigned long ticks)
{
	int error;

	mutex_enter(&completion->c_lock);

	/* Wait until c_done is nonzero.  */
	while (completion->c_done == 0) {
		/* XXX errno NetBSD->Linux */
		error = -cv_timedwait_sig(&completion->c_cv,
		    &completion->c_lock, ticks);
		if (error)
			goto out;
	}

	/* Claim a completion if it's not open season.  */
	if (completion->c_done > 0)
		completion->c_done--;
	else
		KASSERT(completion->c_done == -1);

	/* Success!  */
	error = 0;

out:	mutex_exit(&completion->c_lock);
	return error;
}

#endif	/* _LINUX_COMPLETION_H_ */
