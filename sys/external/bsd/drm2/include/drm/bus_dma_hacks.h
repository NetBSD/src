/*	$NetBSD: bus_dma_hacks.h,v 1.25 2022/07/19 23:19:44 riastradh Exp $	*/

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

#include <linux/mm_types.h>	/* XXX struct page */

#if defined(__i386__) || defined(__x86_64__)
#  include <x86/bus_private.h>
#  include <x86/machdep.h>
#  define	PHYS_TO_BUS_MEM(dmat, paddr)	((bus_addr_t)(paddr))
#  define	BUS_MEM_TO_PHYS(dmat, baddr)	((paddr_t)(baddr))
#elif defined(__arm__) || defined(__aarch64__)
static inline bus_addr_t
PHYS_TO_BUS_MEM(bus_dma_tag_t dmat, paddr_t pa)
{
	unsigned i;

	if (dmat->_nranges == 0)
		return (bus_addr_t)pa;

	for (i = 0; i < dmat->_nranges; i++) {
		const struct arm32_dma_range *dr = &dmat->_ranges[i];

		if (dr->dr_sysbase <= pa && pa - dr->dr_sysbase <= dr->dr_len)
			return pa - dr->dr_sysbase + dr->dr_busbase;
	}
	panic("paddr has no bus address in dma tag %p: %"PRIxPADDR, dmat, pa);
}
static inline paddr_t
BUS_MEM_TO_PHYS(bus_dma_tag_t dmat, bus_addr_t ba)
{
	unsigned i;

	if (dmat->_nranges == 0)
		return (paddr_t)ba;

	for (i = 0; i < dmat->_nranges; i++) {
		const struct arm32_dma_range *dr = &dmat->_ranges[i];

		if (dr->dr_busbase <= ba && ba - dr->dr_busbase <= dr->dr_len)
			return ba - dr->dr_busbase + dr->dr_sysbase;
	}
	panic("bus addr has no bus address in dma tag %p: %"PRIxPADDR, dmat,
	    ba);
}
#elif defined(__sparc__) || defined(__sparc64__)
#  define	PHYS_TO_BUS_MEM(dmat, paddr)	((bus_addr_t)(paddr))
#  define	BUS_MEM_TO_PHYS(dmat, baddr)	((paddr_t)(baddr))
#elif defined(__powerpc__)
#elif defined(__alpha__)
#  define	PHYS_TO_BUS_MEM(dmat, paddr)				      \
	((bus_addr_t)(paddr) | (dmat)->_wbase)
#  define	BUS_MEM_TO_PHYS(dmat, baddr)				      \
	((paddr_t)((baddr) & ~(bus_addr_t)(dmat)->_wbase))
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

static inline bool
bus_dmatag_bounces_paddr(bus_dma_tag_t dmat, paddr_t pa)
{
#if defined(__i386__) || defined(__x86_64__)
	return pa < dmat->_bounce_alloc_lo || dmat->_bounce_alloc_hi <= pa;
#elif defined(__arm__) || defined(__aarch64__)
	unsigned i;

	for (i = 0; i < dmat->_nranges; i++) {
		const struct arm32_dma_range *dr = &dmat->_ranges[i];
		if (dr->dr_sysbase <= pa && pa - dr->dr_sysbase <= dr->dr_len)
			return false;
	}
	return true;
#elif defined(__powerpc__)
	return dmat->_bounce_thresh && pa >= dmat->_bounce_thresh;
#elif defined(__sparc__) || defined(__sparc64__)
	return false;		/* no bounce buffers ever */
#elif defined(__alpha__)
	return (dmat->_wsize == 0 ? false : pa >= dmat->_wsize);
#endif
}

#define MAX_STACK_SEGS 32	/* XXXMRG: 512 bytes on 16 byte seg platforms */

/*
 * XXX This should really take an array of struct vm_page pointers, but
 * Linux drm code stores arrays of struct page pointers, and these two
 * types (struct page ** and struct vm_page **) are not compatible so
 * naive conversion would violate strict aliasing rules.
 */
static inline int
bus_dmamap_load_pages(bus_dma_tag_t tag, bus_dmamap_t map,
    struct page **pgs, bus_size_t size, int flags)
{
	km_flag_t kmflags;
	bus_dma_segment_t *segs;
	bus_dma_segment_t stacksegs[MAX_STACK_SEGS];
	int nsegs, seg;
	struct vm_page *page;
	int error;

	KASSERT((size & (PAGE_SIZE - 1)) == 0);

	if ((size >> PAGE_SHIFT) > INT_MAX)
		return ENOMEM;
	nsegs = size >> PAGE_SHIFT;

	KASSERT(nsegs <= (SIZE_MAX / sizeof(segs[0])));
	if (nsegs > MAX_STACK_SEGS) {
		switch (flags & (BUS_DMA_WAITOK|BUS_DMA_NOWAIT)) {
		case BUS_DMA_WAITOK:
			kmflags = KM_SLEEP;
			break;
		case BUS_DMA_NOWAIT:
			kmflags = KM_NOSLEEP;
			break;
		default:
			panic("invalid flags: %d", flags);
		}
		segs = kmem_alloc((nsegs * sizeof(segs[0])), kmflags);
		if (segs == NULL)
			return ENOMEM;
	} else {
		segs = stacksegs;
	}

	for (seg = 0; seg < nsegs; seg++) {
		page = &pgs[seg]->p_vmp;
		paddr_t paddr = VM_PAGE_TO_PHYS(page);
		bus_addr_t baddr = PHYS_TO_BUS_MEM(tag, paddr);

		segs[seg].ds_addr = baddr;
		segs[seg].ds_len = PAGE_SIZE;
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
out:	if (segs != stacksegs) {
		KASSERT(nsegs > MAX_STACK_SEGS);
		kmem_free(segs, (nsegs * sizeof(segs[0])));
	}
	return error;
}

static inline int
bus_dmamem_export_pages(bus_dma_tag_t dmat, const bus_dma_segment_t *segs,
    int nsegs, struct page **pgs, unsigned npgs)
{
	int seg;
	unsigned pg;

	pg = 0;
	for (seg = 0; seg < nsegs; seg++) {
		bus_addr_t baddr = segs[seg].ds_addr;
		bus_size_t len = segs[seg].ds_len;

		while (len >= PAGE_SIZE) {
			paddr_t paddr = BUS_MEM_TO_PHYS(dmat, baddr);

			KASSERT(pg < npgs);
			pgs[pg++] = container_of(PHYS_TO_VM_PAGE(paddr),
			    struct page, p_vmp);

			baddr += PAGE_SIZE;
			len -= PAGE_SIZE;
		}
		KASSERT(len == 0);
	}
	KASSERT(pg == npgs);

	return 0;
}

static inline int
bus_dmamem_import_pages(bus_dma_tag_t dmat, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, struct page *const *pgs, unsigned npgs)
{
	int seg;
	unsigned i;

	seg = 0;
	for (i = 0; i < npgs; i++) {
		paddr_t paddr = VM_PAGE_TO_PHYS(&pgs[i]->p_vmp);
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
