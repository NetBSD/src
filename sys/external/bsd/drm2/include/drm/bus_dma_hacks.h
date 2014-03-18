/*	$NetBSD: bus_dma_hacks.h,v 1.2 2014/03/18 18:20:43 riastradh Exp $	*/

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

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

/* XXX This is x86-specific bollocks.  */

static inline int
bus_dmamem_wire_uvm_object(bus_dma_tag_t tag, struct uvm_object *uobj,
    off_t start, bus_size_t size, struct pglist *pages, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	struct pglist pageq;
	struct vm_page *page;
	bus_addr_t prev_addr, addr;
	unsigned int i;
	int error;

	KASSERT(size <= __type_max(off_t));
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

	addr = VM_PAGE_TO_PHYS(page);
	segs[0].ds_addr = addr;
	segs[0].ds_len = PAGE_SIZE;

	i = 0;
	while ((page = TAILQ_NEXT(page, pageq.queue)) != NULL) {
		prev_addr = addr;
		addr = VM_PAGE_TO_PHYS(page);
		if ((addr == (prev_addr + PAGE_SIZE)) &&
		    ((addr & boundary) == (prev_addr & boundary))) {
			segs[i].ds_len += PAGE_SIZE;
		} else {
			i += 1;
			if (i >= nsegs) {
				error = EFBIG;
				goto fail1;
			}
			segs[i].ds_addr = addr;
			segs[i].ds_len = PAGE_SIZE;
		}
	}

	/* Success!  */
	*rsegs = (i + 1);
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

#endif	/* _DRM_BUS_DMA_HACKS_H_ */
