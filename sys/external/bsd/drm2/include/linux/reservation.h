/*	$NetBSD: reservation.h,v 1.4 2018/08/27 07:34:41 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_RESERVATION_H_
#define _LINUX_RESERVATION_H_

#include <sys/atomic.h>

#include <linux/rcupdate.h>
#include <linux/ww_mutex.h>

struct fence;

extern struct ww_class	reservation_ww_class;

struct reservation_object {
	struct ww_mutex		lock;

	struct reservation_object_list __rcu	*robj_objlist;
	struct fence __rcu			*robj_fence_excl;
};

struct reservation_object_list {
	uint32_t		shared_count;
	struct fence __rcu	*shared[];
};

#define	reservation_object_add_excl_fence	linux_reservation_object_add_excl_fence
#define	reservation_object_add_shared_fence	linux_reservation_object_add_shared_fence
#define	reservation_object_reserve_shared	linux_reservation_object_reserve_shared
#define	reservation_object_test_signaled_rcu	linux_reservation_object_test_signaled_rcu
#define	reservation_object_wait_timeout_rcu	linux_reservation_object_wait_timeout_rcu

void	reservation_object_add_excl_fence(struct reservation_object *,
	    struct fence *);
void	reservation_object_add_shared_fence(struct reservation_object *,
	    struct fence *);
int	reservation_object_reserve_shared(struct reservation_object *);
bool	reservation_object_test_signaled_rcu(struct reservation_object *,
	    bool);
long	reservation_object_wait_timeout_rcu(struct reservation_object *,
	    bool, bool, unsigned long);

static inline void
reservation_object_init(struct reservation_object *reservation)
{

	ww_mutex_init(&reservation->lock, &reservation_ww_class);
}

static inline void
reservation_object_fini(struct reservation_object *reservation)
{

	ww_mutex_destroy(&reservation->lock);
}

static inline bool
reservation_object_held(struct reservation_object *reservation)
{

	return ww_mutex_is_locked(&reservation->lock);
}

static inline struct fence *
reservation_object_get_excl(struct reservation_object *reservation)
{
	struct fence *fence;

	KASSERT(reservation_object_held(reservation));
	fence = reservation->robj_fence_excl;
	membar_datadep_consumer();

	return fence;
}

static inline struct reservation_object_list *
reservation_object_get_list(struct reservation_object *reservation)
{
	struct reservation_object_list *objlist;

	KASSERT(reservation_object_held(reservation));
	objlist = reservation->robj_objlist;
	membar_datadep_consumer();

	return objlist;
}

#endif  /* _LINUX_RESERVATION_H_ */
