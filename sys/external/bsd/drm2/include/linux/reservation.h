/*	$NetBSD: reservation.h,v 1.8 2018/08/27 15:25:13 riastradh Exp $	*/

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

#ifndef	_LINUX_RESERVATION_H_
#define	_LINUX_RESERVATION_H_

#include <linux/fence.h>
#include <linux/rcupdate.h>
#include <linux/ww_mutex.h>

struct reservation_object {
	struct ww_mutex		lock;

	unsigned				robj_version;
	struct fence __rcu			*robj_fence;
	struct reservation_object_list __rcu	*robj_list;
	struct reservation_object_list __rcu	*robj_prealloc;
};

struct reservation_object_list {
	struct rcu_head		rol_rcu;

	uint32_t		shared_count;
	uint32_t		shared_max;
	struct fence __rcu	*shared[];
};

/* NetBSD addition */
struct reservation_poll {
	kmutex_t		rp_lock;
	struct selinfo		rp_selq;
	struct fence_cb		rp_fcb;
	bool			rp_claimed;
};

#define	reservation_object_add_excl_fence	linux_reservation_object_add_excl_fence
#define	reservation_object_add_shared_fence	linux_reservation_object_add_shared_fence
#define	reservation_object_fini			linux_reservation_object_fini
#define	reservation_object_get_excl		linux_reservation_object_get_excl
#define	reservation_object_get_fences_rcu	linux_reservation_object_get_fences_rcu
#define	reservation_object_get_list		linux_reservation_object_get_list
#define	reservation_object_held			linux_reservation_object_held
#define	reservation_object_init			linux_reservation_object_init
#define	reservation_object_kqfilter		linux_reservation_object_kqfilter
#define	reservation_object_poll			linux_reservation_object_poll
#define	reservation_object_reserve_shared	linux_reservation_object_reserve_shared
#define	reservation_object_test_signaled_rcu	linux_reservation_object_test_signaled_rcu
#define	reservation_object_wait_timeout_rcu	linux_reservation_object_wait_timeout_rcu
#define	reservation_poll_fini			linux_reservation_poll_fini
#define	reservation_poll_init			linux_reservation_poll_init
#define	reservation_ww_class			linux_reservation_ww_class

extern struct ww_class	reservation_ww_class;

void	reservation_object_init(struct reservation_object *);
void	reservation_object_fini(struct reservation_object *);
bool	reservation_object_held(struct reservation_object *);
struct fence *
	reservation_object_get_excl(struct reservation_object *);
struct reservation_object_list *
	reservation_object_get_list(struct reservation_object *);
int	reservation_object_reserve_shared(struct reservation_object *);
void	reservation_object_add_excl_fence(struct reservation_object *,
	    struct fence *);
void	reservation_object_add_shared_fence(struct reservation_object *,
	    struct fence *);

int	reservation_object_get_fences_rcu(struct reservation_object *,
	    struct fence **, unsigned *, struct fence ***);

bool	reservation_object_test_signaled_rcu(struct reservation_object *,
	    bool);
long	reservation_object_wait_timeout_rcu(struct reservation_object *,
	    bool, bool, unsigned long);

/* NetBSD additions */
void	reservation_poll_init(struct reservation_poll *);
void	reservation_poll_fini(struct reservation_poll *);
int	reservation_object_poll(struct reservation_object *, int,
	    struct reservation_poll *);
int	reservation_object_kqfilter(struct reservation_object *,
	    struct knote *, struct reservation_poll *);

#endif	/* _LINUX_RESERVATION_H_ */
