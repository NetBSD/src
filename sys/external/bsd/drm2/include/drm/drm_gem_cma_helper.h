/*	$NetBSD: drm_gem_cma_helper.h,v 1.3 2022/07/02 00:26:07 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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

#ifndef	_DRM_DRM_GEM_CMA_HELPER_H_
#define	_DRM_DRM_GEM_CMA_HELPER_H_

#include <sys/types.h>

#include <sys/bus.h>
#include <sys/vmem.h>

#include <uvm/uvm_pager.h>

#include <drm/drm_gem.h>

struct dma_buf_attachment
struct drm_device;
struct drm_file;
struct drm_gem_cma_object;
struct drm_gem_object;
struct drm_mode_create_dumb;
struct sg_table;
struct vmem;

struct drm_gem_cma_object {
	struct drm_gem_object base;
	bus_dmamap_t dmamap;
	bus_dma_segment_t dmasegs[1];
	bus_size_t dmasize;
	bus_dma_tag_t dmat;
	struct sg_table *sgt;
	void *vaddr;
	vmem_addr_t vmem_addr;
	struct vmem *vmem_pool;
};

#define	to_drm_gem_cma_obj(GEM_OBJ)					      \
	container_of((GEM_OBJ), struct drm_gem_cma_object, base)

extern const struct uvm_pagerops drm_gem_cma_uvm_ops;

struct drm_gem_cma_object *drm_gem_cma_create(struct drm_device *, size_t);
int drm_gem_cma_dumb_create(struct drm_file *, struct drm_device *,
    struct drm_mode_create_dumb *);
void drm_gem_cma_free_object(struct drm_gem_object *);
struct sg_table *drm_gem_cma_prime_get_sg_table(struct drm_gem_object *);
struct drm_gem_object *drm_gem_cma_prime_import_sg_table(struct drm_device *,
    struct dma_buf_attachment *, struct sg_table *);
void *drm_gem_cma_prime_vmap(struct drm_gem_object *);
void drm_gem_cma_prime_vunmap(struct drm_gem_object *, void *);

#endif	/* _DRM_DRM_GEM_CMA_HELPER_H_ */
