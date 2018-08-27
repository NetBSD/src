/*	$NetBSD: fence.h,v 1.10 2018/08/27 07:53:05 riastradh Exp $	*/

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

#ifndef	_LINUX_FENCE_H_
#define	_LINUX_FENCE_H_

#include <sys/types.h>

#include <linux/mutex.h>
#include <linux/rcupdate.h>

struct fence_cb;

struct fence {
	struct kref		refcount;
	spinlock_t		*lock;
	unsigned long		flags;
	unsigned		context;
	unsigned		seqno;
	const struct fence_ops	*ops;
};

#define	FENCE_FLAG_SIGNALED_BIT	__BIT(0)
#define	FENCE_FLAG_USER_BITS	__BIT(1)

struct fence_ops {
	const char	*(*get_driver_name)(struct fence *);
	const char	*(*get_timeline_name)(struct fence *);
	bool		(*enable_signaling)(struct fence *);
	bool		(*signaled)(struct fence *);
	long		(*wait)(struct fence *, bool, long);
	void		(*release)(struct fence *);
};

typedef void (*fence_func_t)(struct fence *, struct fence_cb *);

struct fence_cb {
	fence_func_t	fcb_func;
};

#define	fence_add_callback	linux_fence_add_callback
#define	fence_context_alloc	linux_fence_context_alloc
#define	fence_enable_sw_signaling linux_fence_enable_sw_signaling
#define	fence_get		linux_fence_get
#define	fence_init		linux_fence_init
#define	fence_put		linux_fence_put
#define	fence_remove_callback	linux_fence_remove_callback
#define	fence_signal		linux_fence_signal
#define	fence_signal_locked	linux_fence_signal_locked
#define	fence_wait		linux_fence_wait
#define	fence_wait_timeout	linux_fence_wait_timeout

void	fence_init(struct fence *, const struct fence_ops *, spinlock_t *,
	    unsigned, unsigned);
void	fence_free(struct fence *);

unsigned
	fence_context_alloc(unsigned);

struct fence *
	fence_get(struct fence *);
void	fence_put(struct fence *);

int	fence_add_callback(struct fence *, struct fence_cb *, fence_func_t);
bool	fence_remove_callback(struct fence *, struct fence_cb *);
void	fence_enable_sw_signaling(struct fence *);

bool	fence_is_signaled(struct fence *);
bool	fence_is_signaled_locked(struct fence *);
int	fence_signal(struct fence *);
int	fence_signal_locked(struct fence *);
long	fence_default_wait(struct fence *, bool, long);
long	fence_wait(struct fence *, bool);
long	fence_wait_timeout(struct fence *, bool, int);

static inline void
FENCE_TRACE(struct fence *f, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	printf("fence %u@%u: ", f->context, f->seqno);
	vprintf(fmt, va);
	va_end(va);
}

#endif	/* _LINUX_FENCE_H_ */
