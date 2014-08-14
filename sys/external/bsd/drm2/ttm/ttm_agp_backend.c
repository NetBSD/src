/*	$NetBSD: ttm_agp_backend.c,v 1.3 2014/08/14 16:50:22 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ttm_agp_backend.c,v 1.3 2014/08/14 16:50:22 riastradh Exp $");

#include <sys/types.h>
#include <sys/kmem.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>

#include <ttm/ttm_bo_driver.h>
#include <ttm/ttm_page_alloc.h>

struct ttm_agp {
	struct ttm_dma_tt ttm_dma;
	struct agp_softc *agp;
	unsigned long agp_pgno;
	bool bound;
};

static const struct ttm_backend_func ttm_agp_backend_func;

struct ttm_tt *
ttm_agp_tt_create(struct ttm_bo_device *bdev, struct agp_bridge_data *bridge,
    unsigned long size, uint32_t page_flags, struct page *dummy_read_page)
{
	struct ttm_agp *ttm_agp;

	ttm_agp = kmem_zalloc(sizeof(*ttm_agp), KM_SLEEP);
	ttm_agp->agp = &bridge->abd_sc;
	ttm_agp->ttm_dma.ttm.func = &ttm_agp_backend_func;

	if (ttm_dma_tt_init(&ttm_agp->ttm_dma, bdev, size, page_flags,
		dummy_read_page) != 0)
		goto fail;

	/* Success!  */
	return &ttm_agp->ttm_dma.ttm;

fail:	kmem_free(ttm_agp, sizeof(*ttm_agp));
	return NULL;
}

int
ttm_agp_tt_populate(struct ttm_tt *ttm)
{

	if (ttm->state != tt_unpopulated)
		return 0;

	return ttm_bus_dma_populate(container_of(ttm, struct ttm_dma_tt, ttm));
}

void
ttm_agp_tt_unpopulate(struct ttm_tt *ttm)
{

	ttm_bus_dma_unpopulate(container_of(ttm, struct ttm_dma_tt, ttm));
}

static int
ttm_agp_bind(struct ttm_tt *ttm, struct ttm_mem_reg *bo_mem)
{
	struct ttm_agp *const ttm_agp = container_of(ttm, struct ttm_agp,
	    ttm_dma.ttm);
	struct agp_softc *const sc = ttm_agp->agp;
	struct drm_mm_node *const node = bo_mem->mm_node;
	const unsigned long agp_pgno = node->start;
	unsigned i, j;
	int ret;

	KASSERT(!ttm_agp->bound);
	KASSERT(ttm_agp->ttm_dma.dma_address->dm_nsegs == ttm->num_pages);
	for (i = 0; i < ttm->num_pages; i++) {
		KASSERT(ttm_agp->ttm_dma.dma_address->dm_segs[i].ds_len ==
		    AGP_PAGE_SIZE);
		/* XXX errno NetBSD->Linux */
		ret = -AGP_BIND_PAGE(sc, (agp_pgno + i) << AGP_PAGE_SHIFT,
		    ttm_agp->ttm_dma.dma_address->dm_segs[i].ds_len);
		if (ret)
			goto fail;
	}
	agp_flush_cache();
	AGP_FLUSH_TLB(sc);

	/* Success!  */
	ttm_agp->agp_pgno = agp_pgno;
	ttm_agp->bound = true;
	return 0;

fail:	KASSERT(ret);
	for (j = 0; j < i; j++)
		(void)AGP_UNBIND_PAGE(sc, (agp_pgno + j) << AGP_PAGE_SHIFT);
	return ret;
}

static int
ttm_agp_unbind(struct ttm_tt *ttm)
{
	struct ttm_agp *const ttm_agp = container_of(ttm, struct ttm_agp,
	    ttm_dma.ttm);
	struct agp_softc *const sc = ttm_agp->agp;
	const unsigned long agp_pgno = ttm_agp->agp_pgno;
	unsigned i;

	/* XXX Can this happen?  */
	if (!ttm_agp->bound)
		return 0;

	for (i = 0; i < ttm->num_pages; i++)
		(void)AGP_UNBIND_PAGE(sc, (agp_pgno + i) << AGP_PAGE_SHIFT);

	ttm_agp->agp_pgno = 0;
	ttm_agp->bound = false;
	return 0;
}

static void
ttm_agp_destroy(struct ttm_tt *ttm)
{
	struct ttm_agp *const ttm_agp = container_of(ttm, struct ttm_agp,
	    ttm_dma.ttm);

	/* XXX Can this happen?  */
	if (ttm_agp->bound)
		ttm_agp_unbind(ttm);
	ttm_tt_fini(ttm);
	kmem_free(ttm, sizeof(*ttm));
}

static const struct ttm_backend_func ttm_agp_backend_func = {
	.bind = &ttm_agp_bind,
	.unbind = &ttm_agp_unbind,
	.destroy = &ttm_agp_destroy,
};
