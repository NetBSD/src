/* $NetBSD: drm_netbsd.c,v 1.2.8.2 2008/06/23 05:02:13 wrstuden Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Blair Sadewitz.
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

#include <dev/drm/drmP.h>
#include <sys/hash.h>

/* XXX Most everything the DRM wants is expressed in pages.  There are possible 
   scenarios where e.g. the page size does not equal the page size of the GART,
   but our DRM does not work on those platforms yet. */

struct drm_dmamem *
drm_dmamem_pgalloc(drm_device_t *dev, size_t pages)
{
	struct drm_dmamem	*mem = NULL;
	size_t	  	 	size = pages << PAGE_SHIFT;
	int			 ret = 0;

	mem = malloc(sizeof(*mem), M_DRM, M_NOWAIT | M_ZERO);
	if (mem == NULL)
		return NULL;

	mem->phase = DRM_DMAMEM_INIT;

	mem->dd_segs = malloc(sizeof(*mem->dd_segs) * pages, M_DRM,
	    M_NOWAIT | M_ZERO);
	if (mem->dd_segs == NULL)
		goto error;

	mem->dd_dmat = dev->pa.pa_dmat;
	mem->dd_size = size;

	if (bus_dmamap_create(dev->pa.pa_dmat, size, pages, PAGE_SIZE, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &mem->dd_dmam) != 0)
		goto error;
	mem->phase = DRM_DMAMAP_CREATE;

	if ((ret = bus_dmamem_alloc(dev->pa.pa_dmat, size, PAGE_SIZE, 0,
	    mem->dd_segs, pages, &mem->dd_nsegs, BUS_DMA_NOWAIT)) != 0) {
		goto error;
	}
	mem->phase = DRM_DMAMEM_ALLOC;

	if (bus_dmamem_map(dev->pa.pa_dmat, mem->dd_segs, mem->dd_nsegs, size, 
	    &mem->dd_kva, BUS_DMA_COHERENT|BUS_DMA_NOWAIT) != 0)
		goto error;
	mem->phase = DRM_DMAMEM_MAP;

	if (bus_dmamap_load(dev->pa.pa_dmat, mem->dd_dmam, mem->dd_kva, size,
	    NULL, BUS_DMA_NOWAIT) != 0)
		goto error;
	mem->phase = DRM_DMAMAP_LOAD;

	memset(mem->dd_kva, 0, size);

	return mem;
error:
	mem->phase &= DRM_DMAMEM_FAIL;
	drm_dmamem_free(mem);
	return NULL;
}

void
drm_dmamem_free(struct drm_dmamem *mem)
{
	if (mem == NULL)
		return;

	if (mem->phase & DRM_DMAMEM_FAIL) {
		DRM_DEBUG("attempted allocation failed; teardown sequence follows:\n");
		mem->phase &= ~DRM_DMAMEM_FAIL;
	} else if (mem->phase & ~DRM_DMAMAP_LOAD) {
		DRM_DEBUG("invoked by another function on unloaded map; teardown sequence follows:\n");
	} else {
		DRM_DEBUG("freeing DMA memory; teardown sequence follows:\n");
	}


	switch (mem->phase) {
	case DRM_DMAMAP_LOAD: 
		DRM_DEBUG("bus_dmamap_unload: tag (%p), map (%p)\n",
			(void *)mem->dd_dmat, (void *)mem->dd_dmam);
		bus_dmamap_unload(mem->dd_dmat, mem->dd_dmam);
		/* FALLTHRU */
	case DRM_DMAMEM_MAP: 
		DRM_DEBUG("bus_dmamem_unmap: tag (%p), kva (%p), size (%zd)\n",
			mem->dd_dmat, mem->dd_kva, mem->dd_size);
		bus_dmamem_unmap(mem->dd_dmat, mem->dd_kva, mem->dd_size); 
		/* FALLTHRU */
	case DRM_DMAMEM_ALLOC: 
		DRM_DEBUG("bus_dmamem_free: tag (%p), segs (%p), nsegs (%i)\n",
			mem->dd_dmat, mem->dd_segs, mem->dd_nsegs);
		bus_dmamem_free(mem->dd_dmat, mem->dd_segs, 
			mem->dd_nsegs);
		/* FALLTHRU */
	case DRM_DMAMAP_CREATE: 
		DRM_DEBUG("bus_dmamap_destroy: tag (%p), map (%p)\n",
			(void *)mem->dd_dmat, (void *)mem->dd_dmam);
		bus_dmamap_destroy(mem->dd_dmat, mem->dd_dmam);
		/* FALLTHRU */
	case DRM_DMAMEM_INIT:
		if (mem->dd_segs != NULL) {
			free(mem->dd_segs, M_DRM);
			mem->dd_segs = NULL;
		}
	break;
	}

	free(mem, M_DRM);
	mem = NULL;

	return;
}
