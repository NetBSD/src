/*	$NetBSD: ttm_bus_dma.c,v 1.1.6.3 2017/12/03 11:38:01 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ttm_bus_dma.c,v 1.1.6.3 2017/12/03 11:38:01 jdolecek Exp $");

#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <drm/bus_dma_hacks.h>
#include <ttm/ttm_bo_driver.h>
#include <ttm/ttm_page_alloc.h>

/*
 * ttm_bus_dma_populate(ttm_dma)
 *
 *	If ttm_dma is not already populated, wire its pages and load
 *	its DMA map.  The wiring and loading are stable as long as the
 *	associated bo is reserved.
 *
 *	Transitions from tt_unpopulated to tt_unbound.  Marks as wired,
 *	a.k.a. !swapped.
 */
int
ttm_bus_dma_populate(struct ttm_dma_tt *ttm_dma)
{
	int ret;

	KASSERT(ttm_dma->ttm.state == tt_unpopulated);

	/* If it's unpopulated, it can't be swapped.  */
	KASSERT(!ISSET(ttm_dma->ttm.page_flags, TTM_PAGE_FLAG_SWAPPED));
	/* Pretend it is now, for the sake of ttm_tt_wire.  */
	ttm_dma->ttm.page_flags |= TTM_PAGE_FLAG_SWAPPED;

	/* Wire the uvm pages and fill the ttm page array.  */
	ret = ttm_tt_wire(&ttm_dma->ttm);
	if (ret)
		goto fail0;

	/* Mark it populated but unbound.  */
	ttm_dma->ttm.state = tt_unbound;

	/* Load the DMA map.  */
	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load_pglist(ttm_dma->ttm.bdev->dmat,
	    ttm_dma->dma_address, &ttm_dma->ttm.pglist,
	    (ttm_dma->ttm.num_pages << PAGE_SHIFT), BUS_DMA_NOWAIT);
	if (ret)
		goto fail1;

	/* Mark it wired.  */
	ttm_dma->ttm.page_flags &= ~TTM_PAGE_FLAG_SWAPPED;

	/* Success!  */
	return 0;

fail2: __unused
	bus_dmamap_unload(ttm_dma->ttm.bdev->dmat, ttm_dma->dma_address);
fail1:	KASSERT(ttm_dma->ttm.state == tt_unbound);
	ttm_tt_unwire(&ttm_dma->ttm);
	ttm_dma->ttm.state = tt_unpopulated;
fail0:	KASSERT(ret);
	return ret;
}

static void
ttm_bus_dma_put(struct ttm_dma_tt *ttm_dma, int flags)
{
	struct uvm_object *const uobj = ttm_dma->ttm.swap_storage;
	const size_t size = (ttm_dma->ttm.num_pages << PAGE_SHIFT);

	/*
	 * Can't be tt_bound -- still in use and needs to be removed
	 * from GPU page tables.  Can't be tt_unpopulated -- if it
	 * were, why are you hnadling this?  Hence tt_unbound.
	 */
	KASSERTMSG((ttm_dma->ttm.state == tt_unbound),
	    "ttm_tt %p in invalid state for unpopulate/swapout: %d",
	    &ttm_dma->ttm, (int)ttm_dma->ttm.state);

	/* If pages are wired and loaded, unload and unwire them.  */
	if (!ISSET(ttm_dma->ttm.page_flags, TTM_PAGE_FLAG_SWAPPED)) {
		bus_dmamap_unload(ttm_dma->ttm.bdev->dmat,
		    ttm_dma->dma_address);
		ttm_tt_unwire(&ttm_dma->ttm);
		ttm_dma->ttm.page_flags |= TTM_PAGE_FLAG_SWAPPED;
	}

	/* We are using uvm_aobj, which had better have a pgo_put.  */
	KASSERT(uobj->pgops->pgo_put);

	/* Release or deactivate the pages.  */
	mutex_enter(uobj->vmobjlock);
	(void)(*uobj->pgops->pgo_put)(uobj, 0, size, flags);
	/* pgo_put unlocks uobj->vmobjlock.  */

	/* Mark it unpopulated.  */
	ttm_dma->ttm.state = tt_unpopulated;
}

/*
 * ttmm_bus_dma_unpopulate(ttm_dma)
 *
 *	Unload any DMA map, unwire any pages, and release any pages
 *	associated with ttm_dma.
 *
 *	Transitions from tt_unbound to tt_unpopulated.  Marks as
 *	unwired, a.k.a. swapped.
 */
void
ttm_bus_dma_unpopulate(struct ttm_dma_tt *ttm_dma)
{

	ttm_bus_dma_put(ttm_dma, PGO_CLEANIT|PGO_FREE);
}

/*
 * ttm_bus_dma_swapout(ttm_dma)
 *
 *	Unload any DMA map, unwire any pages, and deactivate any pages
 *	associated with ttm_dma so that they can be swapped out, but
 *	don't release them.
 *
 *	Transitions from tt_unbound to tt_unpopulated.  Marks as
 *	unwired, a.k.a. swapped.
 */
void
ttm_bus_dma_swapout(struct ttm_dma_tt *ttm_dma)
{

	ttm_bus_dma_put(ttm_dma, PGO_DEACTIVATE);
}
