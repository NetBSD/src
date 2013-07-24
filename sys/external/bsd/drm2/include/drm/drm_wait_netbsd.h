/*	$NetBSD: drm_wait_netbsd.h,v 1.1.2.5 2013/07/24 02:36:31 riastradh Exp $	*/

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

#ifndef _DRM_DRM_WAIT_NETBSD_H_
#define _DRM_DRM_WAIT_NETBSD_H_

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <linux/mutex.h>
#include <linux/spinlock.h>

typedef kcondvar_t drm_waitqueue_t;

#define	DRM_HZ	hz		/* XXX Hurk...  */

static inline void
DRM_INIT_WAITQUEUE(drm_waitqueue_t *q, const char *name)
{
	cv_init(q, name);
}

static inline void
DRM_DESTROY_WAITQUEUE(drm_waitqueue_t *q)
{
	cv_destroy(q);
}

static inline void
DRM_WAKEUP_ONE(drm_waitqueue_t *q, struct mutex *interlock)
{
	KASSERT(mutex_is_locked(interlock));
	cv_signal(q);
}

static inline void
DRM_WAKEUP_ALL(drm_waitqueue_t *q, struct mutex *interlock)
{
	KASSERT(mutex_is_locked(interlock));
	cv_broadcast(q);
}

static inline void
DRM_SPIN_WAKEUP_ONE(drm_waitqueue_t *q, spinlock_t *interlock)
{
	KASSERT(spin_is_locked(interlock));
	cv_signal(q);
}

static inline void
DRM_SPIN_WAKEUP_ALL(drm_waitqueue_t *q, spinlock_t *interlock)
{
	KASSERT(spin_is_locked(interlock));
	cv_broadcast(q);
}

#define	DRM_WAIT_UNTIL(RET, Q, INTERLOCK, CONDITION)	do		\
{									\
	KASSERT(mutex_is_locked((INTERLOCK)));				\
	while (!(CONDITION)) {						\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -cv_wait_sig((Q), &(INTERLOCK)->mtx_lock);	\
		if (RET)						\
			break;						\
	}								\
} while (0)

#define	DRM_TIMED_WAIT_UNTIL(RET, Q, INTERLOCK, TICKS, CONDITION) do	\
{									\
	KASSERT(mutex_is_locked((INTERLOCK)));				\
	while (!(CONDITION)) {						\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -cv_timedwait_sig((Q), &(INTERLOCK)->mtx_lock,	\
		    (TICKS));						\
		if (RET)						\
			break;						\
	}								\
} while (0)

#define	DRM_SPIN_WAIT_UNTIL(RET, Q, INTERLOCK, CONDITION)	do	\
{									\
	KASSERT(spin_is_locked((INTERLOCK)));				\
	while (!(CONDITION)) {						\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -cv_wait_sig((Q), &(INTERLOCK)->sl_lock);	\
		if (RET)						\
			break;						\
	}								\
} while (0)

#define	DRM_SPIN_TIMED_WAIT_UNTIL(RET, Q, INTERLOCK, TICKS, CONDITION)	\
	do								\
{									\
	KASSERT(spin_is_locked((INTERLOCK)));				\
	while (!(CONDITION)) {						\
		/* XXX errno NetBSD->Linux */				\
		(RET) = -cv_timedwait_sig((Q), &(INTERLOCK)->sl_lock,	\
		    (TICKS));						\
		if (RET)						\
			break;						\
	}								\
} while (0)

#endif  /* _DRM_DRM_WAIT_NETBSD_H_ */
