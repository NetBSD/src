/*	$NetBSD: bus_dma_hacks.h,v 1.12 2018/08/27 15:26:50 riastradh Exp $	*/

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
#  include <x86/bus_private.h>
#  include <x86/machdep.h>
#  define	PHYS_TO_BUS_MEM(dmat, paddr)	((bus_addr_t)(paddr))
#  define	BUS_MEM_TO_PHYS(dmat, baddr)	((paddr_t)(baddr))
#elif defined(__arm__) || defined(__aarch64__)
#  define	PHYS_TO_BUS_MEM(dmat, paddr)	((bus_addr_t)(paddr))
#  define	BUS_MEM_TO_PHYS(dmat, baddr)	((paddr_t)(baddr))
#elif defined(__powerpc__)
#else
#  error DRM GEM/TTM need new MI bus_dma APIs!  Halp!
#endif

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
		paddr_t paddr = VM_PAGE_TO_PHYS(page);
		bus_addr_t baddr = PHYS_TO_BUS_MEM(tag, paddr);

		segs[seg].ds_addr = baddr;
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

static inline int
bus_dmamap_load_pgarray(bus_dma_tag_t tag, bus_dmamap_t map,
    struct vm_page **pgs, unsigned npgs, bus_size_t size, int flags)
{
	km_flag_t kmflags;
	bus_dma_segment_t *segs;
	unsigned i;
	int error;

	KASSERT((int)npgs <= (SIZE_MAX / sizeof(segs[0])));
	switch (flags & (BUS_DMA_WAITOK|BUS_DMA_NOWAIT)) {
	case BUS_DMA_WAITOK:	kmflags = KM_SLEEP;	break;
	case BUS_DMA_NOWAIT:	kmflags = KM_NOSLEEP;	break;
	default:		panic("invalid flags: %d", flags);
	}
	segs = kmem_alloc((npgs * sizeof(segs[0])), kmflags);
	if (segs == NULL)
		return ENOMEM;

	for (i = 0; i < npgs; i++) {
		paddr_t paddr = VM_PAGE_TO_PHYS(pgs[i]);
		bus_addr_t baddr = PHYS_TO_BUS_MEM(tag, paddr);

		segs[i].ds_addr = baddr;
		segs[i].ds_len = PAGE_SIZE;
	}

	error = bus_dmamap_load_raw(tag, map, segs, npgs, size, flags);
	if (error)
		goto fail0;

	/* Success!  */
	error = 0;
	goto out;

fail1: __unused
	bus_dmamap_unload(tag, map);
fail0:	KASSERT(error);
out:	kmem_free(segs, (npgs * sizeof(segs[0])));
	return error;
}

static inline int
bus_dmamem_export_pages(bus_dma_tag_t dmat, const bus_dma_segment_t *segs,
    int nsegs, struct vm_page **pgs, unsigned npgs)
{
	int seg;
	unsigned i;

	i = 0;
	for (seg = 0; seg < nsegs; seg++) {
		bus_addr_t baddr = segs[i].ds_addr;
		bus_size_t len = segs[i].ds_len;

		while (len >= PAGE_SIZE) {
			paddr_t paddr = BUS_MEM_TO_PHYS(dmat, baddr);

			KASSERT(i < npgs);
			pgs[i++] = PHYS_TO_VM_PAGE(paddr);

			baddr += PAGE_SIZE;
			len -= PAGE_SIZE;
		}
		KASSERT(len == 0);
	}
	KASSERT(i == npgs);

	return 0;
}

static inline int
bus_dmamem_import_pages(bus_dma_tag_t dmat, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, struct vm_page *const *pgs, unsigned npgs)
{
	int seg;
	unsigned i;

	seg = 0;
	for (i = 0; i < npgs; i++) {
		paddr_t paddr = VM_PAGE_TO_PHYS(pgs[i]);
		bus_addr_t baddr = PHYS_TO_BUS_MEM(dmat, paddr);

		if (seg > 0 && segs[seg - 1].ds_addr + PAGE_SIZE == baddr) {
			segs[seg - 1].ds_len += PAGE_SIZE;
		} else {
			KASSERT(seg < nsegs);
			segs[seg].ds_addr = baddr;
			segs[seg].ds_len = PAGE_SIZE;
			seg++;
			KASSERT(seg <= nsegs);
		}
	}
	KASSERT(seg <= nsegs);
	*rsegs = seg;

	return 0;
}

#endif	/* _DRM_BUS_DMA_HACKS_H_ */
