/*	$NetBSD: hashtable.h,v 1.4 2018/08/27 06:23:19 riastradh Exp $	*/

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

#ifndef _LINUX_HASHTABLE_H_
#define _LINUX_HASHTABLE_H_

#include <sys/cdefs.h>

#include <linux/list.h>

/*
 * XXX This assumes that HLIST_HEAD_INIT is just initialization to a
 * null pointer.
 */
#define DECLARE_HASHTABLE(name, bits)					      \
	struct hlist_head name[1u << (bits)] = { HLIST_HEAD_INIT }

static inline void
__hash_init(struct hlist_head *hash, unsigned n)
{

	while (n --> 0)
		INIT_HLIST_HEAD(&hash[n]);
}

static inline bool
__hash_empty(struct hlist_head *hash, unsigned n)
{

	while (n --> 0) {
		if (!hlist_empty(&hash[n]))
			return false;
	}

	return true;
}

#define	hash_init(h)		__hash_init((h), __arraycount(h))
#define	hash_add(h, n, k)	hlist_add_head(n, &(h)[(k) % __arraycount(h)])
#define	hash_del(n)		hlist_del_init(n)
#define	hash_empty(h)		__hash_empty((h), __arraycount(h))
#define	hash_for_each_possible(h, n, f, k)				      \
	hlist_for_each_entry(n, &(h)[(k) % __arraycount(h)], f)
#define	hash_for_each_safe(h, i, t, n, f)				      \
	for ((i) = 0; (i) < __arraycount(h); (i)++)			      \
		hlist_for_each_entry_safe(n, t, &(h)[i], f)

#endif /*_LINUX_HASHTABLE_H_*/
