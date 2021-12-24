/*	$NetBSD: linux_sgt.c,v 1.4 2021/12/24 15:08:31 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_sgt.c,v 1.4 2021/12/24 15:08:31 riastradh Exp $");

#include <sys/bus.h>
#include <sys/errno.h>

#include <drm/bus_dma_hacks.h>

#include <linux/dma-mapping.h>
#include <linux/gfp.h>
#include <linux/mm_types.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>

int
sg_alloc_table(struct sg_table *sgt, unsigned npgs, gfp_t gfp)
{

	sgt->sgl->sg_pgs = kcalloc(npgs, sizeof(sgt->sgl->sg_pgs[0]), gfp);
	if (sgt->sgl->sg_pgs == NULL)
		return -ENOMEM;
	sgt->sgl->sg_npgs = sgt->nents = npgs;
	sgt->sgl->sg_dmamap = NULL;

	return 0;
}

int
__sg_alloc_table_from_pages(struct sg_table *sgt, struct page **pgs,
    unsigned npgs, bus_size_t offset, bus_size_t size, unsigned maxseg,
    gfp_t gfp)
{
	unsigned i;
	int ret;

	KASSERT(offset == 0);
	KASSERT(size == (bus_size_t)npgs << PAGE_SHIFT);

	ret = sg_alloc_table(sgt, npgs, gfp);
	if (ret)
		return ret;

	for (i = 0; i < npgs; i++)
		sgt->sgl->sg_pgs[i] = pgs[i];

	return 0;
}

int
sg_alloc_table_from_pages(struct sg_table *sgt, struct page **pgs,
    unsigned npgs, bus_size_t offset, bus_size_t size, gfp_t gfp)
{

	return __sg_alloc_table_from_pages(sgt, pgs, npgs, offset, size,
	    -1, gfp);
}

int
sg_alloc_table_from_bus_dmamem(struct sg_table *sgt, bus_dma_tag_t dmat,
    const bus_dma_segment_t *seg, int nseg, gfp_t gfp)
{
	int i, npgs = 0;
	int ret;

	KASSERT(nseg >= 1);

	/*
	 * Count the number of pages.  Some segments may span multiple
	 * contiguous pages.
	 */
	for (i = 0; i < nseg; i++) {
		bus_size_t len = seg[i].ds_len;
		for (; len >= PAGE_SIZE; len -= PAGE_SIZE, npgs++) {
			if (npgs == INT_MAX)
				return -ENOMEM;
		}
		KASSERTMSG(len == 0, "misaligned segment length: %ju\n",
		    (uintmax_t)seg[i].ds_len);
	}

	ret = sg_alloc_table(sgt, npgs, gfp);
	if (ret)
		return ret;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_export_pages(dmat, seg, nseg, sgt->sgl->sg_pgs,
	    sgt->sgl->sg_npgs);
	if (ret)
		goto out;

	/* Success!  */
	ret = 0;

out:	if (ret)
		sg_free_table(sgt);
	return ret;
}

void
sg_free_table(struct sg_table *sgt)
{

	if (sgt->sgl->sg_dmamap) {
		KASSERT(sgt->sgl->sg_dmat);
		bus_dmamap_destroy(sgt->sgl->sg_dmat, sgt->sgl->sg_dmamap);
	}
	kfree(sgt->sgl->sg_pgs);
	sgt->sgl->sg_pgs = NULL;
	sgt->sgl->sg_npgs = 0;
}

int
dma_map_sg(bus_dma_tag_t dmat, struct scatterlist *sg, int nents, int dir)
{

	return dma_map_sg_attrs(dmat, sg, nents, dir, 0);
}

int
dma_map_sg_attrs(bus_dma_tag_t dmat, struct scatterlist *sg, int nents,
    int dir, int attrs)
{
	int flags = 0;
	bool loaded = false;
	int ret, error = 0;

	KASSERT(sg->sg_dmamap == NULL);
	KASSERT(sg->sg_npgs);
	KASSERT(nents >= 1);

	switch (dir) {
	case DMA_TO_DEVICE:
		flags |= BUS_DMA_WRITE;
		break;
	case DMA_FROM_DEVICE:
		flags |= BUS_DMA_READ;
		break;
	case DMA_BIDIRECTIONAL:
		flags |= BUS_DMA_READ|BUS_DMA_WRITE;
		break;
	case DMA_NONE:
	default:
		panic("invalid DMA direction %d", dir);
	}

	error = bus_dmamap_create(dmat, (bus_size_t)sg->sg_npgs << PAGE_SHIFT,
	    nents, PAGE_SIZE, 0, BUS_DMA_WAITOK, &sg->sg_dmamap);
	if (error)
		goto out;
	KASSERT(sg->sg_dmamap);

	error = bus_dmamap_load_pages(dmat, sg->sg_dmamap, sg->sg_pgs,
	    (bus_size_t)sg->sg_npgs << PAGE_SHIFT, BUS_DMA_WAITOK|flags);
	if (error)
		goto out;
	loaded = true;

	/* Success! */
	KASSERT(sg->sg_dmamap->dm_nsegs > 0);
	KASSERT(sg->sg_dmamap->dm_nsegs <= nents);
	sg->sg_dmat = dmat;
	ret = sg->sg_dmamap->dm_nsegs;
	error = 0;

out:	if (error) {
		if (loaded)
			bus_dmamap_unload(dmat, sg->sg_dmamap);
		loaded = false;
		if (sg->sg_dmamap)
			bus_dmamap_destroy(dmat, sg->sg_dmamap);
		sg->sg_dmamap = NULL;
		ret = 0;
	}
	return ret;
}

void
dma_unmap_sg(bus_dma_tag_t dmat, struct scatterlist *sg, int nents, int dir)
{

	dma_unmap_sg_attrs(dmat, sg, nents, dir, 0);
}

void
dma_unmap_sg_attrs(bus_dma_tag_t dmat, struct scatterlist *sg, int nents,
    int dir, int attrs)
{

	KASSERT(sg->sg_dmat == dmat);

	bus_dmamap_unload(dmat, sg->sg_dmamap);
	bus_dmamap_destroy(dmat, sg->sg_dmamap);
	sg->sg_dmamap = NULL;
}

bus_addr_t
sg_dma_address(const struct scatterlist *sg)
{

	KASSERT(sg->sg_dmamap->dm_nsegs == 1);
	return sg->sg_dmamap->dm_segs[0].ds_addr;
}

bus_size_t
sg_dma_len(const struct scatterlist *sg)
{

	KASSERT(sg->sg_dmamap->dm_nsegs == 1);
	return sg->sg_dmamap->dm_segs[0].ds_len;
}
