/*	$NetBSD: drm_gem_vm.c,v 1.7 2018/08/27 06:56:22 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: drm_gem_vm.c,v 1.7 2018/08/27 06:56:22 riastradh Exp $");

#include <sys/types.h>

#include <uvm/uvm_extern.h>

#include <drm/drmP.h>
#include <drm/drm_gem.h>
#include <drm/drm_vma_manager.h>

static int	drm_gem_mmap_object_locked(struct drm_device *, off_t, size_t,
		    int, struct uvm_object **, voff_t *, struct file *);

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

	drm_gem_object_unreference_unlocked(obj);
}

int
drm_gem_or_legacy_mmap_object(struct drm_device *dev, off_t byte_offset,
    size_t nbytes, int prot, struct uvm_object **uobjp, voff_t *uoffsetp,
    struct file *file)
{
	int ret;

	ret = drm_gem_mmap_object(dev, byte_offset, nbytes, prot, uobjp,
	    uoffsetp, file);
	if (ret)
		return ret;
	if (*uobjp != NULL)
		return 0;

	return drm_mmap_object(dev, byte_offset, nbytes, prot, uobjp,
	    uoffsetp, file);
}

int
drm_gem_mmap_object(struct drm_device *dev, off_t byte_offset, size_t nbytes,
    int prot, struct uvm_object **uobjp, voff_t *uoffsetp, struct file *file)
{
	int ret;

	mutex_lock(&dev->struct_mutex);
	ret = drm_gem_mmap_object_locked(dev, byte_offset, nbytes, prot,
	    uobjp, uoffsetp, file);
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

static int
drm_gem_mmap_object_locked(struct drm_device *dev, off_t byte_offset,
    size_t nbytes, int prot __unused, struct uvm_object **uobjp,
    voff_t *uoffsetp, struct file *file)
{
	const unsigned long startpage = (byte_offset >> PAGE_SHIFT);
	const unsigned long npages = (nbytes >> PAGE_SHIFT);

	KASSERT(mutex_is_locked(&dev->struct_mutex));
	KASSERT(drm_core_check_feature(dev, DRIVER_GEM));
	KASSERT(dev->driver->gem_uvm_ops != NULL);
	KASSERT(prot == (prot & (PROT_READ | PROT_WRITE)));
	KASSERT(0 <= byte_offset);
	KASSERT(byte_offset == (byte_offset & ~(PAGE_SIZE-1)));
	KASSERT(nbytes == (npages << PAGE_SHIFT));

	struct drm_vma_offset_node *const node =
	    drm_vma_offset_exact_lookup(dev->vma_offset_manager, startpage,
		npages);
	if (node == NULL) {
		/* Fall back to vanilla device mappings.  */
		*uobjp = NULL;
		*uoffsetp = (voff_t)-1;
		return 0;
	}

	if (!drm_vma_node_is_allowed(node, file))
		return -EACCES;

	struct drm_gem_object *const obj = container_of(node,
	    struct drm_gem_object, vma_node);
	KASSERT(obj->dev == dev);

	/* Success!  */
	drm_gem_object_reference(obj);
	*uobjp = &obj->gemo_uvmobj;
	*uoffsetp = 0;
	return 0;
}
