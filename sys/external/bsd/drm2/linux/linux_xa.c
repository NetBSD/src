/*	$NetBSD: linux_xa.c,v 1.2 2021/12/19 12:05:18 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_xa.c,v 1.2 2021/12/19 12:05:18 riastradh Exp $");

#include <sys/radixtree.h>

#include <uvm/uvm_pdaemon.h>

#include <linux/xarray.h>

const struct xa_limit xa_limit_32b = { .min = 0, .max = UINT32_MAX };

void
xa_init_flags(struct xarray *xa, gfp_t gfp)
{

	mutex_init(&xa->xa_lock, MUTEX_DEFAULT, IPL_VM);
	radix_tree_init_tree(&xa->xa_tree);
	xa->xa_gfp = gfp;
}

void
xa_destroy(struct xarray *xa)
{

	KASSERT(radix_tree_empty_tree_p(&xa->xa_tree));
	radix_tree_fini_tree(&xa->xa_tree);
	mutex_destroy(&xa->xa_lock);
}

void *
xa_load(struct xarray *xa, unsigned long key)
{
	void *datum;

	/* XXX pserialize */
	mutex_enter(&xa->xa_lock);
	datum = radix_tree_lookup_node(&xa->xa_tree, key);
	mutex_exit(&xa->xa_lock);

	return datum;
}

void *
xa_store(struct xarray *xa, unsigned long key, void *datum, gfp_t gfp)
{
	void *collision;
	int error;

	KASSERT(datum != NULL);
	KASSERT(((uintptr_t)datum & 0x3) == 0);

retry:	mutex_enter(&xa->xa_lock);
	collision = radix_tree_lookup_node(&xa->xa_tree, key);
	error = radix_tree_insert_node(&xa->xa_tree, key, datum);
	mutex_exit(&xa->xa_lock);
	if (error) {
		if (gfp & __GFP_WAIT) {
			uvm_wait("xa");
			goto retry;
		}
		/* XXX errno NetBSD->Linux */
		return XA_ERROR(-error);
	}

	return collision;
}

int
xa_alloc(struct xarray *xa, uint32_t *idp, void *datum, struct xa_limit limit,
    gfp_t gfp)
{
	uint32_t key = limit.min;
	void *collision;
	int error;

	KASSERTMSG(limit.min < limit.max, "min=%"PRIu32" max=%"PRIu32,
	    limit.min, limit.max);

retry:	mutex_enter(&xa->xa_lock);
	/* XXX use the radix tree to make this fast */
	while ((collision = radix_tree_lookup_node(&xa->xa_tree, key))
	    != NULL) {
		if (key++ == limit.max)
			goto out;
	}
	error = radix_tree_insert_node(&xa->xa_tree, key, datum);
out:	mutex_exit(&xa->xa_lock);

	if (collision)
		return -EBUSY;
	if (error) {
		if (gfp & __GFP_WAIT) {
			uvm_wait("xa");
			goto retry;
		}
		/* XXX errno NetBSD->Linux */
		return -error;
	}

	/* Success!  */
	*idp = key;
	return 0;
}

void *
xa_find(struct xarray *xa, unsigned long *startp, unsigned long max,
    unsigned tagmask)
{
	uint64_t key = *startp;
	void *candidate = NULL;

	mutex_enter(&xa->xa_lock);
	for (; key < max; key++, candidate = NULL) {
		candidate = radix_tree_lookup_node(&xa->xa_tree, key);
		if (candidate == NULL)
			continue;
		if (tagmask == -1 ||
		    radix_tree_get_tag(&xa->xa_tree, key, tagmask))
			break;
	}
	mutex_exit(&xa->xa_lock);

	if (candidate)
		*startp = key;
	return candidate;
}

void *
xa_find_after(struct xarray *xa, unsigned long *startp, unsigned long max,
    unsigned tagmask)
{
	unsigned long start = *startp + 1;
	void *found;

	if (start == max)
		return NULL;
	found = xa_find(xa, &start, max, tagmask);
	if (found)
		*startp = start;
	return found;
}

void *
xa_erase(struct xarray *xa, unsigned long key)
{
	void *datum;

	mutex_enter(&xa->xa_lock);
	datum = radix_tree_remove_node(&xa->xa_tree, key);
	mutex_exit(&xa->xa_lock);

	return datum;
}
