/*	$NetBSD: linux_idr.c,v 1.1.2.10 2013/12/30 04:52:11 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_idr.c,v 1.1.2.10 2013/12/30 04:52:11 riastradh Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/pserialize.h>

#include <linux/err.h>
#include <linux/idr.h>
#include <linux/slab.h>

struct idr_state {
	size_t			is_n_allocated;
	size_t			is_n_used;
	void			**is_data;
};

void
idr_init(struct idr *idr)
{

	mutex_init(&idr->idr_lock, MUTEX_DEFAULT, IPL_VM);
	idr->idr_psz = pserialize_create();
	idr->idr_state = kmalloc(sizeof(*idr->idr_state), GFP_KERNEL);
	KASSERT(idr->idr_state != NULL);
	idr->idr_state->is_n_allocated = 1;
	idr->idr_state->is_n_used = 0;
	idr->idr_state->is_data = kcalloc(1, sizeof(void *), GFP_KERNEL);
	KASSERT(idr->idr_state->is_data != NULL);
	idr->idr_temp = NULL;
}

static struct idr_state *
idr_state(struct idr *idr)
{
	struct idr_state *const is = idr->idr_state;

	membar_consumer();		/* match state */

	return is;
}

void
idr_destroy(struct idr *idr)
{
	struct idr_state *const is = idr_state(idr);

	kfree(is->is_data);
	kfree(is);
	KASSERT(idr->idr_temp == NULL);

	pserialize_destroy(idr->idr_psz);
	mutex_destroy(&idr->idr_lock);
}

static void **
idr_find_ptr(struct idr *idr, int id)
{
	if (id < 0)
		return NULL;

	struct idr_state *const is = idr_state(idr);
	if (is->is_n_used <= id)
		return NULL;

	return &is->is_data[id];
}

void *
idr_find(struct idr *idr, int id)
{
	void **const ptr = idr_find_ptr(idr, id);

	if (ptr == NULL)
		return NULL;

	void *const datum = *ptr;

	membar_consumer();		/* match datum */
	return datum;
}

void *
idr_replace(struct idr *idr, void *replacement, int id)
{
	void **const ptr = idr_find_ptr(idr, id);

	if (ptr == NULL)
		return ERR_PTR(-ENOENT);

	void *const datum = *ptr;

	membar_producer();		/* match datum */
	*ptr = replacement;

	return datum;
}

void
idr_remove(struct idr *idr, int id)
{
	if (id < 0)
		return;

	struct idr_state *const is = idr_state(idr);

	if (is->is_n_used < id)
		return;

	is->is_data[id] = NULL;

	if ((id + 1) == is->is_n_used) {
		while ((0 < id--) && (is->is_data[id] == NULL))
			continue;
		is->is_n_used = id;
	}
}

void
idr_remove_all(struct idr *idr)
{
	struct idr_state *const is = idr_state(idr);

	is->is_n_used = 0;
}

int
idr_pre_get(struct idr *idr, gfp_t gfp)
{
	struct idr_state *old_is, *new_is, *temp_is;
	size_t n_used;

	old_is = idr_state(idr);
	n_used = old_is->is_n_used;

	new_is = kmalloc(sizeof(*new_is), gfp);
	if (new_is == NULL)
		return 0;
	new_is->is_data = kcalloc((n_used + 1), sizeof(*new_is->is_data), gfp);
	if (new_is->is_data == NULL) {
		kfree(new_is);
		return 0;
	}

	new_is->is_n_allocated = (n_used + 1);

	membar_producer();		/* match temp */

	/*
	 * If someone already put one in, replace it -- we're probably
	 * more up-to-date.
	 */
	temp_is = atomic_swap_ptr(&idr->idr_temp, new_is);
	if (temp_is != NULL) {
		membar_consumer();	/* match temp */
		kfree(temp_is->is_data);
		kfree(temp_is);
	}

	return 1;
}

int
idr_get_new_above(struct idr *idr, void *datum, int min_id, int *idp)
{
	struct idr_state *is = idr_state(idr);
	const size_t n_used = is->is_n_used;
	const size_t n_allocated = is->is_n_allocated;
	int id;

	if (min_id < 0)
		min_id = 0;

	for (id = min_id; id < n_used; id++)
		if (is->is_data[id] == NULL)
			goto win;
	if (id < n_allocated) {
		is->is_n_used++;
		goto win;
	}

	struct idr_state *const it = atomic_swap_ptr(&idr->idr_temp, NULL);
	if (it == NULL)
		return -EAGAIN;

	membar_consumer();		/* match temp */
	if (id < it->is_n_allocated) {
		(void)memcpy(it->is_data, is->is_data, sizeof(void *) *
		    n_used);
		it->is_n_used = (is->is_n_used + 1);
		membar_producer();	/* match state */
		idr->idr_state = it;
		pserialize_perform(idr->idr_psz);
		kfree(is->is_data);
		kfree(is);
		is = it;
		goto win;
	}

win:	membar_producer();		/* match datum */
	is->is_data[id] = datum;
	*idp = id;
	return 0;
}

int
idr_for_each(struct idr *idr, int (*proc)(int, void *, void *), void *arg)
{
	struct idr_state *const is = idr_state(idr);
	int id;
	int error = 0;

	for (id = 0; id < is->is_n_used; id++) {
		void *const datum = is->is_data[id];

		membar_consumer();	/* match datum */
		error = (*proc)(id, datum, arg);
		if (error)
			break;
	}

	return error;
}
