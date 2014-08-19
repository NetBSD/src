/*	$NetBSD: completion.h,v 1.4.4.2 2014/08/20 00:03:56 tls Exp $	*/

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

/*
 * Notes on porting:
 *
 * - Linux does not have destroy_completion.  You must add it yourself
 *   in the appropriate place.
 *
 * - Some Linux code does `completion->done++' or similar.  Convert
 *   that to complete(completion) and suggest the same change upstream,
 *   unless it turns out there actually is a good reason to do that, in
 *   which case the Linux completion API should be extended with a
 *   sensible name for this that doesn't expose the guts of `struct
 *   completion'.
 */

#ifndef _LINUX_COMPLETION_H_
#define _LINUX_COMPLETION_H_

#include <sys/types.h>
#include <sys/condvar.h>
#include <sys/mutex.h>

#include <machine/limits.h>

#include <linux/errno.h>

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

	mutex_init(&completion->c_lock, MUTEX_DEFAULT, IPL_VM);
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
	mutex_destroy(&completion->c_lock);
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

static inline void
_completion_claim(struct completion *completion)
{

	KASSERT(mutex_owned(&completion->c_lock));
	KASSERT(completion->c_done != 0);
	if (completion->c_done > 0)
		completion->c_done--;
	else
		KASSERT(completion->c_done == -1);
}

/*
 * Wait interruptibly with a timeout for someone to call complete or
 * complete_all.
 */
static inline int
wait_for_completion_interruptible_timeout(struct completion *completion,
    unsigned long ticks)
{
	/* XXX Arithmetic overflow...?  */
	unsigned int start = hardclock_ticks, now;
	int error;

	mutex_enter(&completion->c_lock);

	/* Wait until c_done is nonzero.  */
	while (completion->c_done == 0) {
		error = cv_timedwait_sig(&completion->c_cv,
		    &completion->c_lock, ticks);
		if (error)
			goto out;
		now = hardclock_ticks;
		if (ticks < (now - start)) {
			error = EWOULDBLOCK;
			goto out;
		}
		ticks -= (now - start);
		start = now;
	}

	/* Success!  */
	_completion_claim(completion);
	error = 0;

out:	mutex_exit(&completion->c_lock);
	if (error == EWOULDBLOCK) {
		return 0;
	} else if ((error == EINTR) || (error == ERESTART)) {
		return -ERESTARTSYS;
	} else {
		KASSERTMSG((error == 0), "error = %d", error);
		return ticks;
	}
}

/*
 * Wait interruptibly for someone to call complete or complete_all.
 */
static inline int
wait_for_completion_interruptible(struct completion *completion)
{
	int error;

	mutex_enter(&completion->c_lock);

	/* Wait until c_done is nonzero.  */
	while (completion->c_done == 0) {
		error = cv_wait_sig(&completion->c_cv, &completion->c_lock);
		if (error)
			goto out;
	}

	/* Success!  */
	_completion_claim(completion);
	error = 0;

out:	mutex_exit(&completion->c_lock);
	if ((error == EINTR) || (error == ERESTART))
		error = -ERESTARTSYS;
	return error;
}

/*
 * Wait uninterruptibly, except by SIGKILL, for someone to call
 * complete or complete_all.
 *
 * XXX In this implementation, any signal will actually wake us, not
 * just SIGKILL.
 */
static inline int
wait_for_completion_killable(struct completion *completion)
{

	return wait_for_completion_interruptible(completion);
}

/*
 * Try to claim a completion immediately.  Return true on success, false
 * if it would block.
 */
static inline bool
try_wait_for_completion(struct completion *completion)
{
	bool ok;

	mutex_enter(&completion->c_lock);
	if (completion->c_done == 0) {
		ok = false;
	} else {
		_completion_claim(completion);
		ok = true;
	}
	mutex_exit(&completion->c_lock);

	return ok;
}

#endif	/* _LINUX_COMPLETION_H_ */
