/*	$NetBSD: uvm_page_array.c,v 1.9 2020/05/26 21:52:12 ad Exp $	*/

/*-
 * Copyright (c)2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_page_array.c,v 1.9 2020/05/26 21:52:12 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_page_array.h>

/*
 * uvm_page_array_init: initialize the array.
 */

void
uvm_page_array_init(struct uvm_page_array *ar, struct uvm_object *uobj,
    unsigned int flags)
{

	ar->ar_idx = 0;
	ar->ar_npages = 0;
	ar->ar_uobj = uobj;
	ar->ar_flags = flags;
}

/*
 * uvm_page_array_fini: clean up the array.
 */

void
uvm_page_array_fini(struct uvm_page_array *ar)
{

	/*
	 * currently nothing to do.
	 */
#if defined(DIAGNOSTIC)
	/*
	 * poison to trigger assertion in uvm_page_array_peek to
	 * detect usage errors.
	 */
	ar->ar_npages = 1;
	ar->ar_idx = 1000;
#endif /* defined(DIAGNOSTIC) */
}

/*
 * uvm_page_array_clear: forget the cached pages and initialize the array.
 */

void
uvm_page_array_clear(struct uvm_page_array *ar)
{

	KASSERT(ar->ar_idx <= ar->ar_npages);
	ar->ar_idx = 0;
	ar->ar_npages = 0;
}

/*
 * uvm_page_array_peek: return the next cached page.
 */

struct vm_page *
uvm_page_array_peek(struct uvm_page_array *ar)
{

	KASSERT(ar->ar_idx <= ar->ar_npages);
	if (ar->ar_idx == ar->ar_npages) {
		return NULL;
	}
	return ar->ar_pages[ar->ar_idx];
}

/*
 * uvm_page_array_advance: advance the array to the next cached page
 */

void
uvm_page_array_advance(struct uvm_page_array *ar)
{

	KASSERT(ar->ar_idx <= ar->ar_npages);
	ar->ar_idx++;
	KASSERT(ar->ar_idx <= ar->ar_npages);
}

/*
 * uvm_page_array_fill: lookup pages and keep them cached.
 *
 * return 0 on success.  in that case, cache the result in the array
 * so that they will be picked by later uvm_page_array_peek.
 *
 * nwant is a number of pages to fetch.  a caller should consider it a hint.
 * nwant == 0 means a caller have no specific idea.
 *
 * return ENOENT if no pages are found.
 *
 * called with object lock held.
 */

int
uvm_page_array_fill(struct uvm_page_array *ar, voff_t off, unsigned int nwant)
{
	unsigned int npages;
#if defined(DEBUG)
	unsigned int i;
#endif /* defined(DEBUG) */
	unsigned int maxpages = __arraycount(ar->ar_pages);
	struct uvm_object *uobj = ar->ar_uobj;
	const int flags = ar->ar_flags;
	const bool dense = (flags & UVM_PAGE_ARRAY_FILL_DENSE) != 0;
	const bool backward = (flags & UVM_PAGE_ARRAY_FILL_BACKWARD) != 0;
	int error = 0;

	if (nwant != 0 && nwant < maxpages) {
		maxpages = nwant;
	}
#if 0 /* called from DDB for "show obj/f" without lock */
	KASSERT(rw_lock_held(uobj->vmobjlock));
#endif
	KASSERT(uvm_page_array_peek(ar) == NULL);
	if ((flags & UVM_PAGE_ARRAY_FILL_DIRTY) != 0) {
		unsigned int tagmask = UVM_PAGE_DIRTY_TAG;

		if ((flags & UVM_PAGE_ARRAY_FILL_WRITEBACK) != 0) {
			tagmask |= UVM_PAGE_WRITEBACK_TAG;
		}
		npages =
		    (backward ? radix_tree_gang_lookup_tagged_node_reverse :
		    radix_tree_gang_lookup_tagged_node)(
		    &uobj->uo_pages, off >> PAGE_SHIFT, (void **)ar->ar_pages,
		    maxpages, dense, tagmask);
	} else {
		npages =
		    (backward ? radix_tree_gang_lookup_node_reverse :
		    radix_tree_gang_lookup_node)(
		    &uobj->uo_pages, off >> PAGE_SHIFT, (void **)ar->ar_pages,
		    maxpages, dense);
	}
	if (npages == 0) {
		if (flags != 0) {
			/*
			 * if dense or looking for tagged entries (or
			 * working backwards), fail right away.
			 */
			npages = 0;
		} else {
			/*
			 * there's nothing else to be found with the current
			 * set of arguments, in the current version of the
			 * tree.
			 *
			 * minimize repeated tree lookups by "finding" a
			 * null pointer, in case the caller keeps looping (a
			 * common use case).
			 */
			npages = 1;
			ar->ar_pages[0] = NULL;
		}
		error = ENOENT;
	}
	KASSERT(npages <= maxpages);
	ar->ar_npages = npages;
	ar->ar_idx = 0;
#if defined(DEBUG)
	for (i = 0; error == 0 && i < ar->ar_npages; i++) {
		struct vm_page * const pg = ar->ar_pages[i];

		KASSERT(pg != NULL);
		KDASSERT(pg->uobject == uobj);
		if (backward) {
			KDASSERT(pg->offset <= off);
			KDASSERT(i == 0 ||
			    pg->offset < ar->ar_pages[i - 1]->offset);
		} else {
			KDASSERT(pg->offset >= off);
			KDASSERT(i == 0 ||
			    pg->offset > ar->ar_pages[i - 1]->offset);
		}
	}
#endif /* defined(DEBUG) */
	return error;
}

/*
 * uvm_page_array_fill_and_peek:
 * same as uvm_page_array_peek except that, if the array is empty, try to fill
 * it first.
 */

struct vm_page *
uvm_page_array_fill_and_peek(struct uvm_page_array *ar, voff_t off,
    unsigned int nwant)
{
	int error;

	if (ar->ar_idx != ar->ar_npages) {
		return ar->ar_pages[ar->ar_idx];
	}
	error = uvm_page_array_fill(ar, off, nwant);
	if (error != 0) {
		return NULL;
	}
	return uvm_page_array_peek(ar);
}
