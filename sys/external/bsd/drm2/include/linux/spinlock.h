/*	$NetBSD: spinlock.h,v 1.1.2.3 2013/07/24 02:09:58 riastradh Exp $	*/

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

#ifndef _LINUX_SPINLOCK_H_
#define _LINUX_SPINLOCK_H_

#include <sys/cdefs.h>
#include <sys/mutex.h>

typedef struct {
	kmutex_t sl_lock;
} spinlock_t;

static inline void
spin_lock(spinlock_t *spinlock)
{
	mutex_enter(&spinlock->sl_lock);
}

static inline void
spin_unlock(spinlock_t *spinlock)
{
	mutex_exit(&spinlock->sl_lock);
}

/* Must be a macro because the second argument is to be assigned.  */
#define	spin_lock_irqsave(SPINLOCK, FLAGS)				\
	do {								\
		(FLAGS) = 0;						\
		mutex_enter(&((spinlock_t *)(SPINLOCK))->sl_lock);	\
	} while (0)

static inline void
spin_unlock_irqrestore(spinlock_t *spinlock, unsigned long __unused flags)
{
	mutex_exit(&spinlock->sl_lock);
}

static inline void
spin_lock_init(spinlock_t *spinlock)
{
	/* XXX Need to identify which need to block intrs.  */
	mutex_init(&spinlock->sl_lock, MUTEX_DEFAULT, IPL_NONE);
}

#endif  /* _LINUX_SPINLOCK_H_ */
