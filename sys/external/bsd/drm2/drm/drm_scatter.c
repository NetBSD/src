/*	$NetBSD: drm_scatter.c,v 1.4 2018/02/07 06:18:46 mrg Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_scatter.c,v 1.4 2018/02/07 06:18:46 mrg Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <linux/slab.h>

#include <drm/drmP.h>

static int	drm_sg_alloc_mem(struct drm_device *, size_t,
		    struct drm_sg_mem **);
static void	drm_sg_free_mem(struct drm_device *, struct drm_sg_mem *);

int
drm_sg_alloc(struct drm_device *dev, void *data,
    struct drm_file *file __unused)
{
	struct drm_scatter_gather *const request = data;
	struct drm_sg_mem *sg = NULL;
	int error;

	/*
	 * XXX Should not hold this mutex during drm_sg_alloc.  For
	 * now, we'll just use non-blocking allocations.
	 */
	KASSERT(mutex_is_locked(&drm_global_mutex));

	/*
	 * Sanity-check the inputs.
	 */
	if (!drm_core_check_feature(dev, DRIVER_SG))
		return -ENODEV;
	if (dev->sg != NULL)
		return -EBUSY;
	if (request->size > 0xffffffffUL)
		return -ENOMEM;

	/*
	 * Do the allocation.
	 *
	 * XXX Would it be safe to drop drm_global_mutex here to avoid
	 * holding it during allocation?
	 */
	error = drm_sg_alloc_mem(dev, request->size, &sg);
	if (error)
		return error;

	/* Set it up to be the device's scatter-gather memory.  */
	dev->sg = sg;

	/* Success!  */
	request->handle = sg->handle;
	return 0;
}

int
drm_sg_free(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_scatter_gather *const request = data;
	struct drm_sg_mem *sg;

	KASSERT(mutex_is_locked(&drm_global_mutex));

	/*
	 * Sanity-check the inputs.
	 */
	if (!drm_core_check_feature(dev, DRIVER_SG))
		return -ENODEV;

	sg = dev->sg;
	if (sg == NULL)
		return -ENXIO;
	if (sg->handle != request->handle)
		return -EINVAL;

	/* Remove dev->sg.  */
	dev->sg = NULL;

	/* Free it.  */
	drm_sg_free_mem(dev, sg);

	/* Success!  */
	return 0;
}

void
drm_legacy_sg_cleanup(struct drm_device *dev)
{

	if (!drm_core_check_feature(dev, DRIVER_SG))
		return;
	if (dev->sg == NULL)
		return;
	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return;

	drm_sg_free_mem(dev, dev->sg);
}

static int
drm_sg_alloc_mem(struct drm_device *dev, size_t size, struct drm_sg_mem **sgp)
{
	int nsegs;
	int error;

	KASSERT(drm_core_check_feature(dev, DRIVER_SG));

	KASSERT(size <= (size_t)0xffffffffUL); /* XXX 32-bit sizes only?  */
	const size_t nbytes = PAGE_ALIGN(size);
	const size_t npages = nbytes >> PAGE_SHIFT;
	KASSERT(npages <= (size_t)INT_MAX);

	/*
	 * Allocate a drm_sg_mem record.
	 */
	struct drm_sg_mem *const sg =
	    kmem_zalloc(offsetof(struct drm_sg_mem, sg_segs[npages]),
		KM_NOSLEEP);
	if (sg == NULL)
		return -ENOMEM;
	sg->sg_tag = dev->dmat;
	sg->sg_nsegs_max = (unsigned int)npages;

	/*
	 * Allocate the requested amount of DMA-safe memory.
	 */
	KASSERT(sg->sg_nsegs_max <= (unsigned int)INT_MAX);
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamem_alloc(sg->sg_tag, nbytes, PAGE_SIZE, 0,
	    sg->sg_segs, (int)sg->sg_nsegs_max, &nsegs, BUS_DMA_NOWAIT);
	if (error)
		goto fail0;
	KASSERT(0 <= nsegs);
	sg->sg_nsegs = (unsigned int)nsegs;

	/*
	 * XXX Old drm passed DRM_BUS_NOWAIT below but BUS_DMA_WAITOK
	 * above.  WTF?
	 */

	/*
	 * Map the DMA-safe memory into kernel virtual address space.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamem_map(sg->sg_tag, sg->sg_segs, nsegs, nbytes,
	    &sg->virtual,
	    (BUS_DMA_NOWAIT | BUS_DMA_COHERENT | BUS_DMA_NOCACHE));
	if (error)
		goto fail1;
	sg->sg_size = nbytes;

	/*
	 * Create a map for DMA transfers.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamap_create(sg->sg_tag, nbytes, nsegs, nbytes, 0,
	    BUS_DMA_NOWAIT, &sg->sg_map);
	if (error)
		goto fail2;

	/*
	 * Load the kva buffer into the map for DMA transfers.
	 */
	/* XXX errno NetBSD->Linux */
	error = -bus_dmamap_load(sg->sg_tag, sg->sg_map, sg->virtual, nbytes,
	    NULL, (BUS_DMA_NOWAIT | BUS_DMA_NOCACHE));
	if (error)
		goto fail3;

	/*
	 * Use the kernel virtual address as the userland handle.
	 *
	 * XXX This leaks some information about kernel virtual
	 * addresses to userland; should we use an idr instead?
	 */
	sg->handle = (unsigned long)(uintptr_t)sg->virtual;
	KASSERT(sg->virtual == (void *)(uintptr_t)sg->handle);

	/* Success!  */
	*sgp = sg;
	return 0;

fail3:	bus_dmamap_destroy(sg->sg_tag, sg->sg_map);
fail2:	bus_dmamem_unmap(sg->sg_tag, sg->virtual, sg->sg_size);
fail1:	KASSERT(sg->sg_nsegs <= (unsigned int)INT_MAX);
	bus_dmamem_free(sg->sg_tag, sg->sg_segs, (int)sg->sg_nsegs);
fail0:	sg->sg_tag = NULL;	/* XXX paranoia */
	kmem_free(sg, offsetof(struct drm_sg_mem, sg_segs[sg->sg_nsegs_max]));
	return error;
}

static void
drm_sg_free_mem(struct drm_device *dev, struct drm_sg_mem *sg)
{

	bus_dmamap_unload(sg->sg_tag, sg->sg_map);
	bus_dmamap_destroy(sg->sg_tag, sg->sg_map);
	bus_dmamem_unmap(sg->sg_tag, sg->virtual, sg->sg_size);
	KASSERT(sg->sg_nsegs <= (unsigned int)INT_MAX);
	bus_dmamem_free(sg->sg_tag, sg->sg_segs, (int)sg->sg_nsegs);
	sg->sg_tag = NULL;	/* XXX paranoia */
	kmem_free(sg, offsetof(struct drm_sg_mem, sg_segs[sg->sg_nsegs_max]));
}
