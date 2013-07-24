/*	$NetBSD: completion.h,v 1.1.2.1 2013/07/24 03:07:28 riastradh Exp $	*/

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

struct completion {
	kmutex_t c_lock;
	kcondvar_t c_cv;
	bool c_done;
};

static inline void
INIT_COMPLETION(struct completion *completion)
{

	mutex_enter(&completion->c_lock);
	completion->c_done = false;
	mutex_exit(&completion->c_lock);
}

static inline void
init_completion(struct completion *completion)
{

	mutex_init(&completion->c_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&completion->c_cv, "lnxcmplt");
	completion->c_done = false;
}

static inline void
complete_all(struct completion *completion)
{

	mutex_enter(&completion->c_lock);
	completion->c_done = true;
	cv_broadcast(&completion->c_cv);
	mutex_exit(&completion->c_lock);
}

static inline int
wait_for_completion_interruptible_timeout(struct completion *completion,
    unsigned long ticks)
{
	int error = 0;

	mutex_enter(&completion->c_lock);
	while (!completion->c_done) {
		error = cv_timedwait_sig(&completion->c_cv,
		    &completion->c_lock, ticks);
		if (error)
			break;
	}
	mutex_exit(&completion->c_lock);

	return error;
}

#endif	/* _LINUX_COMPLETION_H_ */
