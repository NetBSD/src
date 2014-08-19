/*	$NetBSD: ttm_bus_dma.c,v 1.1.6.2 2014/08/20 00:04:22 tls Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ttm_bus_dma.c,v 1.1.6.2 2014/08/20 00:04:22 tls Exp $");

#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <drm/bus_dma_hacks.h>
#include <ttm/ttm_bo_driver.h>
#include <ttm/ttm_page_alloc.h>

int
ttm_bus_dma_populate(struct ttm_dma_tt *ttm_dma)
{
	int ret;

	/* If it's already populated, nothing to do.  */
	if (ttm_dma->ttm.state != tt_unpopulated)
		return 0;

	/* Wire the pages, allocating them if necessary.  */
	ret = ttm_tt_swapin(&ttm_dma->ttm);
	if (ret)
		goto fail0;

	/* Load the DMA map.  */
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load_pglist(ttm_dma->ttm.bdev->dmat,
	    ttm_dma->dma_address, &ttm_dma->ttm.pglist,
	    (ttm_dma->ttm.num_pages << PAGE_SHIFT), BUS_DMA_NOWAIT);
	if (ret)
		goto fail1;

	/* Success!  */
	ttm_dma->ttm.state = tt_unbound;
	return 0;

fail2: __unused
	bus_dmamap_unload(ttm_dma->ttm.bdev->dmat, ttm_dma->dma_address);
fail1:	ttm_tt_swapout(&ttm_dma->ttm, NULL);
fail0:	KASSERT(ret);
	return ret;
}

void
ttm_bus_dma_unpopulate(struct ttm_dma_tt *ttm_dma)
{
	struct uvm_object *const uobj = ttm_dma->ttm.swap_storage;
	const size_t size = (ttm_dma->ttm.num_pages << PAGE_SHIFT);

	/* Unload the DMA map.  */
	bus_dmamap_unload(ttm_dma->ttm.bdev->dmat, ttm_dma->dma_address);

	/* Unwire the pages.  */
	ttm_tt_swapout(&ttm_dma->ttm, NULL);

	/* We are using uvm_aobj, which had better have a pgo_put.  */
	KASSERT(uobj->pgops->pgo_put);

	/* Release the pages.  */
	mutex_enter(uobj->vmobjlock);
	(void)(*uobj->pgops->pgo_put)(uobj, 0, size, PGO_CLEANIT|PGO_FREE);
	/* pgo_put unlocks uobj->vmobjlock.  */
}
