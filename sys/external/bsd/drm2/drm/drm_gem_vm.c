/*	$NetBSD: drm_gem_vm.c,v 1.1.2.2 2013/09/08 15:44:14 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_gem_vm.c,v 1.1.2.2 2013/09/08 15:44:14 riastradh Exp $");

#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <drm/drmP.h>

static int	drm_gem_mmap_object_locked(struct drm_device *, off_t *,
		    size_t, struct uvm_object **);

void
drm_gem_pager_reference(struct uvm_object *uobj)
{
	struct drm_gem_object *const obj = container_of(uobj,
	    struct drm_gem_object, gemo_uvmobj);

	drm_gem_object_reference(obj);
}

void
drm_gem_pager_detach(struct uvm_object *uobj)
{
	struct drm_gem_object *const obj = container_of(uobj,
	    struct drm_gem_object, gemo_uvmobj);

	drm_gem_object_unreference(obj);
}

int
drm_gem_mmap_object(struct drm_device *dev, off_t *byte_offsetp, size_t nbytes,
    struct uvm_object **uobjp)
{
	int error;

	mutex_lock(&dev->struct_mutex);
	error = drm_gem_mmap_object_locked(dev, byte_offsetp, nbytes, uobjp);
	mutex_unlock(&dev->struct_mutex);

	return error;
}

static int
drm_gem_mmap_object_locked(struct drm_device *dev, off_t *byte_offsetp,
    size_t nbytes, struct uvm_object **uobjp)
{
	struct drm_gem_mm *const mm = dev->mm_private;
	const off_t byte_offset = *byte_offsetp;
	const off_t page_offset = (byte_offset >> PAGE_SHIFT);
	struct drm_hash_item *hash;

	KASSERT(mutex_is_locked(&dev->struct_mutex));
	KASSERT(drm_core_check_feature(dev, DRIVER_GEM));
	KASSERT(dev->driver->gem_uvm_ops != NULL);

	if (byte_offset != (byte_offset & ~(PAGE_SIZE-1))) /* XXX kassert?  */
		return -EINVAL;

	if (drm_ht_find_item(&mm->offset_hash, page_offset, &hash) != 0) {
		/* Fall back to vanilla device mappings.  */
		*uobjp = NULL;
		return 0;
	}

	struct drm_local_map *const map = drm_hash_entry(hash,
	    struct drm_map_list, hash)->map;

	if (map == NULL)	/* XXX How can this happen?  */
		return -EINVAL;
	if (map->size < nbytes)
		return -EOVERFLOW;

	struct drm_gem_object *const obj = map->handle;
	KASSERT(obj->dev == dev);

	/* Success!  */
	drm_gem_object_reference(obj); /* XXX Locking?  */
	*uobjp = &obj->gemo_uvmobj;
	return 0;
}
