/*	$NetBSD: kref.h,v 1.2.2.1 2014/08/10 06:55:39 tls Exp $	*/

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

#ifndef _LINUX_KREF_H_
#define _LINUX_KREF_H_

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/systm.h>

#include <linux/mutex.h>

struct kref {
	unsigned int kr_count;
};

static inline void
kref_init(struct kref *kref)
{
	kref->kr_count = 1;
}

static inline void
kref_get(struct kref *kref)
{
	const unsigned int count __unused =
	    atomic_inc_uint_nv(&kref->kr_count);

	KASSERTMSG((count > 1), "getting released kref");
}

static inline bool
kref_get_unless_zero(struct kref *kref)
{
	unsigned count;

	do {
		count = kref->kr_count;
		if ((count == 0) || (count == UINT_MAX))
			return false;
	} while (atomic_cas_uint(&kref->kr_count, count, (count + 1)) !=
	    count);

	return true;
}

static inline int
kref_sub(struct kref *kref, unsigned int count, void (*release)(struct kref *))
{
	unsigned int old, new;

	do {
		old = kref->kr_count;
		KASSERTMSG((count <= old), "overreleasing kref: %u - %u",
		    old, count);
		new = (old - count);
	} while (atomic_cas_uint(&kref->kr_count, old, new) != old);

	if (new == 0) {
		(*release)(kref);
		return 1;
	}

	return 0;
}

static inline int
kref_put(struct kref *kref, void (*release)(struct kref *))
{

	return kref_sub(kref, 1, release);
}

static inline int
kref_put_mutex(struct kref *kref, void (*release)(struct kref *),
    struct mutex *interlock)
{
	unsigned int old, new;

	do {
		old = kref->kr_count;
		KASSERT(old > 0);
		if (old == 1) {
			mutex_lock(interlock);
			if (atomic_add_int_nv(&kref->kr_count, -1) == 0) {
				(*release)(kref);
				return 1;
			}
			mutex_unlock(interlock);
			return 0;
		}
		new = (old - 1);
	} while (atomic_cas_uint(&kref->kr_count, old, new) != old);

	return 0;
}

/*
 * Not native to Linux.  Mostly used for assertions...
 */

static inline bool
kref_referenced_p(struct kref *kref)
{

	return (0 < kref->kr_count);
}

static inline bool
kref_exclusive_p(struct kref *kref)
{

	KASSERT(0 < kref->kr_count);
	return (kref->kr_count == 1);
}

#endif  /* _LINUX_KREF_H_ */
