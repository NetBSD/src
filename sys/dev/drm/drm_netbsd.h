/* $NetBSD: drm_netbsd.h,v 1.2.2.2 2008/06/02 13:23:15 mjf Exp $ */

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

#ifndef _DEV_DRM_DRMNETBSD_H_
#define _DEV_DRM_DRMNETBSD_H_

#include <sys/types.h>
#include <sys/bus.h>

#define	DRM_DMAMEM_INIT		0x000
#define DRM_DMAMAP_CREATE	0x001
#define DRM_DMAMEM_ALLOC	0x002
#define DRM_DMAMEM_MAP		0x004
#define DRM_DMAMAP_LOAD		0x008
#define	DRM_DMAMEM_FAIL		0x800

#define DRM_DMAMEM_BUSADDR(__m)		((__m)->dd_dmam->dm_segs[0].ds_addr)
#define DRM_DMAMEM_SEGADDR(__m, __s)	((__m)->dd_dmam->dm_segs[__s].ds_addr)
#define DRM_DMAMEM_SEGSIZE(__m, __s)	((__m)->dd_dmam->dm_segs[__s].ds_len)
#define DRM_DMAMEM_KERNADDR(__m)	((__m)->dd_kva)

/* for dma_lock */
#define RWLOCK_CONTENTION(rwlock)	 do { } while (!rw_tryupgrade(rwlock))

struct drm_dmamem {
	bus_dma_tag_t 		dd_dmat;
	bus_dmamap_t 		dd_dmam;
	bus_dma_segment_t 	*dd_segs;
	int 			dd_nsegs;
	size_t 			dd_size;
	void 			*dd_kva;
	unsigned int		phase;
};

struct	drm_dmamem *drm_dmamem_pgalloc(struct drm_device *, size_t);
void	drm_dmamem_free(struct drm_dmamem *);

#endif /* _DEV_DRM_DRMNETBSD_H_ */
