/*	$NetBSD: bus_dma_hacks.h,v 1.9 2018/04/01 04:35:06 ryo Exp $	*/

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

#ifndef	_DRM_BUS_DMA_HACKS_H_
#define	_DRM_BUS_DMA_HACKS_H_

#include <sys/cdefs.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#if defined(__i386__) || defined(__x86_64__)
#include <x86/bus_private.h>
#include <x86/machdep.h>
#elif defined(__arm__) || defined(__aarch64__)
#else
#error DRM GEM/TTM need new MI bus_dma APIs!  Halp!
#endif

static inline int
bus_dmamem_wire_uvm_object(bus_dma_tag_t tag, struct uvm_object *uobj,
    off_t start, bus_size_t size, struct pglist *pages, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	struct pglist pageq;
	struct vm_page *page;
	unsigned i;
	int error;

	/*
	 * XXX `#ifdef __x86_64__' is a horrible way to work around a
	 * completely stupid GCC warning that encourages unsafe,
	 * nonportable code and has no obvious way to be selectively
	 * suppressed.
	 */
#if __x86_64__
	KASSERT(size <= __type_max(off_t));
#endif

	KASSERT(start <= (__type_max(off_t) - size));
	KASSERT(alignment == PAGE_SIZE); /* XXX */
	KASSERT(0 < nsegs);

	if (pages == NULL) {
		TAILQ_INIT(&pageq);
		pages = &pageq;
	}

	error = uvm_obj_wirepages(uobj, start, (start + size), pages);
	if (error)
		goto fail0;

	page = TAILQ_FIRST(pages);
	KASSERT(page != NULL);

	for (i = 0; i < nsegs; i++) {
		if (page == NULL) {
			error = EFBIG;
			goto fail1;
		}
		segs[i].ds_addr = VM_PAGE_TO_PHYS(page);
		segs[i].ds_len = MIN(PAGE_SIZE, size);
		size -= PAGE_SIZE;
		page = TAILQ_NEXT(page, pageq.queue);
	}
	KASSERT(page == NULL);

	/* Success!  */
	*rsegs = nsegs;
	return 0;

fail1:	uvm_obj_unwirepages(uobj, start, (start + size));
fail0:	return error;
}

static inline void
bus_dmamem_unwire_uvm_object(bus_dma_tag_t tag __unused,
    struct uvm_object *uobj, off_t start, bus_size_t size,
    bus_dma_segment_t *segs __unused, int nsegs __unused)
{
	uvm_obj_unwirepages(uobj, start, (start + size));
}

static inline int
bus_dmamem_pgfl(bus_dma_tag_t tag)
{
#if defined(__i386__) || defined(__x86_64__)
	return x86_select_freelist(tag->_bounce_alloc_hi - 1);
#else
	return VM_FREELIST_DEFAULT;
#endif
}

static inline int
bus_dmamap_load_pglist(bus_dma_tag_t tag, bus_dmamap_t map,
    struct pglist *pglist, bus_size_t size, int flags)
{
	km_flag_t kmflags;
	bus_dma_segment_t *segs;
	int nsegs, seg;
	struct vm_page *page;
	int error;

	nsegs = 0;
	TAILQ_FOREACH(page, pglist, pageq.queue) {
		if (nsegs == MIN(INT_MAX, (SIZE_MAX / sizeof(segs[0]))))
			return ENOMEM;
		nsegs++;
	}

	KASSERT(nsegs <= (SIZE_MAX / sizeof(segs[0])));
	switch (flags & (BUS_DMA_WAITOK|BUS_DMA_NOWAIT)) {
	case BUS_DMA_WAITOK:	kmflags = KM_SLEEP;	break;
	case BUS_DMA_NOWAIT:	kmflags = KM_NOSLEEP;	break;
	default:		panic("invalid flags: %d", flags);
	}
	segs = kmem_alloc((nsegs * sizeof(segs[0])), kmflags);
	if (segs == NULL)
		return ENOMEM;

	seg = 0;
	TAILQ_FOREACH(page, pglist, pageq.queue) {
		segs[seg].ds_addr = VM_PAGE_TO_PHYS(page);
		segs[seg].ds_len = PAGE_SIZE;
		seg++;
	}

	error = bus_dmamap_load_raw(tag, map, segs, nsegs, size, flags);
	if (error)
		goto fail0;

	/* Success!  */
	error = 0;
	goto out;

fail1: __unused
	bus_dmamap_unload(tag, map);
fail0:	KASSERT(error);
out:	kmem_free(segs, (nsegs * sizeof(segs[0])));
	return error;
}

#endif	/* _DRM_BUS_DMA_HACKS_H_ */
