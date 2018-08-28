/*	$NetBSD: drm_prime.c,v 1.7 2018/08/27 15:32:39 riastradh Exp $	*/

/*
 * Copyright © 2012 Red Hat
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *      Dave Airlie <airlied@redhat.com>
 *      Rob Clark <rob.clark@linaro.org>
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_prime.c,v 1.7 2018/08/27 15:32:39 riastradh Exp $");

#include <linux/export.h>
#include <linux/dma-buf.h>
#include <drm/drmP.h>
#include <drm/drm_gem.h>

#include "drm_internal.h"

#ifdef __NetBSD__

#include <drm/bus_dma_hacks.h>

/*
 * We use struct sg_table just to pass around an array of pages from
 * one device to another in drm prime.  Since this is _not_ a complete
 * implementation of Linux's sg table abstraction (e.g., it does not
 * remember DMA addresses and RAM pages separately, and it doesn't
 * support the nested chained iteration of Linux scatterlists), we
 * isolate it to this file and make all callers go through a few extra
 * subroutines (drm_prime_sg_size, drm_prime_sg_free, &c.) to use it.
 * Don't use this outside drm prime!
 */

struct sg_table {
	paddr_t		*sgt_pgs;
	unsigned	sgt_npgs;
};

static int
sg_alloc_table_from_pages(struct sg_table *sgt, struct page **pages,
    unsigned npages, bus_size_t offset, bus_size_t size, gfp_t gfp)
{
	unsigned i;

	KASSERT(offset == 0);
	KASSERT(size == npages << PAGE_SHIFT);

	sgt->sgt_pgs = kcalloc(npages, sizeof(sgt->sgt_pgs[0]), gfp);
	if (sgt->sgt_pgs == NULL)
		return -ENOMEM;
	sgt->sgt_npgs = npages;

	for (i = 0; i < npages; i++)
		sgt->sgt_pgs[i] = VM_PAGE_TO_PHYS(&pages[i]->p_vmp);

	return 0;
}

static int
sg_alloc_table_from_pglist(struct sg_table *sgt, const struct pglist *pglist,
    unsigned npages, bus_size_t offset, bus_size_t size, gfp_t gfp)
{
	struct vm_page *pg;
	unsigned i;

	KASSERT(offset == 0);
	KASSERT(size == npages << PAGE_SHIFT);

	sgt->sgt_pgs = kcalloc(npages, sizeof(sgt->sgt_pgs[0]), gfp);
	if (sgt->sgt_pgs == NULL)
		return -ENOMEM;
	sgt->sgt_npgs = npages;

	i = 0;
	TAILQ_FOREACH(pg, pglist, pageq.queue) {
		KASSERT(i < npages);
		sgt->sgt_pgs[i] = VM_PAGE_TO_PHYS(pg);
	}
	KASSERT(i == npages);

	return 0;
}

static int
sg_alloc_table_from_bus_dmamem(struct sg_table *sgt, bus_dma_tag_t dmat,
    const bus_dma_segment_t *segs, int nsegs, gfp_t gfp)
{
	int ret;

	KASSERT(nsegs > 0);
	sgt->sgt_pgs = kcalloc(nsegs, sizeof(sgt->sgt_pgs[0]), gfp);
	if (sgt->sgt_pgs == NULL)
		return -ENOMEM;
	sgt->sgt_npgs = nsegs;

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamem_export_pages(dmat, segs, nsegs, sgt->sgt_pgs,
	    sgt->sgt_npgs);
	if (ret)
		return ret;

	return 0;
}

static void
sg_free_table(struct sg_table *sgt)
{

	kfree(sgt->sgt_pgs);
	sgt->sgt_pgs = NULL;
	sgt->sgt_npgs = 0;
}

#endif	/* __NetBSD__ */

/*
 * DMA-BUF/GEM Object references and lifetime overview:
 *
 * On the export the dma_buf holds a reference to the exporting GEM
 * object. It takes this reference in handle_to_fd_ioctl, when it
 * first calls .prime_export and stores the exporting GEM object in
 * the dma_buf priv. This reference is released when the dma_buf
 * object goes away in the driver .release function.
 *
 * On the import the importing GEM object holds a reference to the
 * dma_buf (which in turn holds a ref to the exporting GEM object).
 * It takes that reference in the fd_to_handle ioctl.
 * It calls dma_buf_get, creates an attachment to it and stores the
 * attachment in the GEM object. When this attachment is destroyed
 * when the imported object is destroyed, we remove the attachment
 * and drop the reference to the dma_buf.
 *
 * Thus the chain of references always flows in one direction
 * (avoiding loops): importing_gem -> dmabuf -> exporting_gem
 *
 * Self-importing: if userspace is using PRIME as a replacement for flink
 * then it will get a fd->handle request for a GEM object that it created.
 * Drivers should detect this situation and return back the gem object
 * from the dma-buf private.  Prime will do this automatically for drivers that
 * use the drm_gem_prime_{import,export} helpers.
 */

struct drm_prime_member {
	struct list_head entry;
	struct dma_buf *dma_buf;
	uint32_t handle;
};

struct drm_prime_attachment {
	struct sg_table *sgt;
	enum dma_data_direction dir;
};

static int drm_prime_add_buf_handle(struct drm_prime_file_private *prime_fpriv,
				    struct dma_buf *dma_buf, uint32_t handle)
{
	struct drm_prime_member *member;

	member = kmalloc(sizeof(*member), GFP_KERNEL);
	if (!member)
		return -ENOMEM;

	get_dma_buf(dma_buf);
	member->dma_buf = dma_buf;
	member->handle = handle;
	list_add(&member->entry, &prime_fpriv->head);
	return 0;
}

static struct dma_buf *drm_prime_lookup_buf_by_handle(struct drm_prime_file_private *prime_fpriv,
						      uint32_t handle)
{
	struct drm_prime_member *member;

	list_for_each_entry(member, &prime_fpriv->head, entry) {
		if (member->handle == handle)
			return member->dma_buf;
	}

	return NULL;
}

static int drm_prime_lookup_buf_handle(struct drm_prime_file_private *prime_fpriv,
				       struct dma_buf *dma_buf,
				       uint32_t *handle)
{
	struct drm_prime_member *member;

	list_for_each_entry(member, &prime_fpriv->head, entry) {
		if (member->dma_buf == dma_buf) {
			*handle = member->handle;
			return 0;
		}
	}
	return -ENOENT;
}

static int drm_gem_map_attach(struct dma_buf *dma_buf,
			      struct device *target_dev,
			      struct dma_buf_attachment *attach)
{
	struct drm_prime_attachment *prime_attach;
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;

	prime_attach = kzalloc(sizeof(*prime_attach), GFP_KERNEL);
	if (!prime_attach)
		return -ENOMEM;

	prime_attach->dir = DMA_NONE;
	attach->priv = prime_attach;

	if (!dev->driver->gem_prime_pin)
		return 0;

	return dev->driver->gem_prime_pin(obj);
}

static void drm_gem_map_detach(struct dma_buf *dma_buf,
			       struct dma_buf_attachment *attach)
{
	struct drm_prime_attachment *prime_attach = attach->priv;
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;
	struct sg_table *sgt;

	if (dev->driver->gem_prime_unpin)
		dev->driver->gem_prime_unpin(obj);

	if (!prime_attach)
		return;

	sgt = prime_attach->sgt;
	if (sgt) {
#ifndef __NetBSD__		/* We map/unmap elsewhere.  */
		if (prime_attach->dir != DMA_NONE)
			dma_unmap_sg(attach->dev, sgt->sgl, sgt->nents,
					prime_attach->dir);
#endif
		sg_free_table(sgt);
	}

	kfree(sgt);
	kfree(prime_attach);
	attach->priv = NULL;
}

void drm_prime_remove_buf_handle_locked(struct drm_prime_file_private *prime_fpriv,
					struct dma_buf *dma_buf)
{
	struct drm_prime_member *member, *safe;

	list_for_each_entry_safe(member, safe, &prime_fpriv->head, entry) {
		if (member->dma_buf == dma_buf) {
			dma_buf_put(dma_buf);
			list_del(&member->entry);
			kfree(member);
		}
	}
}

static struct sg_table *drm_gem_map_dma_buf(struct dma_buf_attachment *attach,
						enum dma_data_direction dir)
{
	struct drm_prime_attachment *prime_attach = attach->priv;
	struct drm_gem_object *obj = attach->dmabuf->priv;
	struct sg_table *sgt;

	if (WARN_ON(dir == DMA_NONE || !prime_attach))
		return ERR_PTR(-EINVAL);

	/* return the cached mapping when possible */
	if (prime_attach->dir == dir)
		return prime_attach->sgt;

	/*
	 * two mappings with different directions for the same attachment are
	 * not allowed
	 */
	if (WARN_ON(prime_attach->dir != DMA_NONE))
		return ERR_PTR(-EBUSY);

	sgt = obj->dev->driver->gem_prime_get_sg_table(obj);
	if (!IS_ERR(sgt)) {
#ifdef __NetBSD__		/* We map/unmap elsewhere.  */
		prime_attach->sgt = sgt;
		prime_attach->dir = dir;
#else
		if (!dma_map_sg(attach->dev, sgt->sgl, sgt->nents, dir)) {
			sg_free_table(sgt);
			kfree(sgt);
			sgt = ERR_PTR(-ENOMEM);
		} else {
			prime_attach->sgt = sgt;
			prime_attach->dir = dir;
		}
#endif
	}

	return sgt;
}

static void drm_gem_unmap_dma_buf(struct dma_buf_attachment *attach,
				  struct sg_table *sgt,
				  enum dma_data_direction dir)
{
	/* nothing to be done here */
}

/**
 * drm_gem_dmabuf_release - dma_buf release implementation for GEM
 * @dma_buf: buffer to be released
 *
 * Generic release function for dma_bufs exported as PRIME buffers. GEM drivers
 * must use this in their dma_buf ops structure as the release callback.
 */
void drm_gem_dmabuf_release(struct dma_buf *dma_buf)
{
	struct drm_gem_object *obj = dma_buf->priv;

	/* drop the reference on the export fd holds */
	drm_gem_object_unreference_unlocked(obj);
}
EXPORT_SYMBOL(drm_gem_dmabuf_release);

static void *drm_gem_dmabuf_vmap(struct dma_buf *dma_buf)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;

	return dev->driver->gem_prime_vmap(obj);
}

static void drm_gem_dmabuf_vunmap(struct dma_buf *dma_buf, void *vaddr)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;

	dev->driver->gem_prime_vunmap(obj, vaddr);
}

static void *drm_gem_dmabuf_kmap_atomic(struct dma_buf *dma_buf,
					unsigned long page_num)
{
	return NULL;
}

static void drm_gem_dmabuf_kunmap_atomic(struct dma_buf *dma_buf,
					 unsigned long page_num, void *addr)
{

}
static void *drm_gem_dmabuf_kmap(struct dma_buf *dma_buf,
				 unsigned long page_num)
{
	return NULL;
}

static void drm_gem_dmabuf_kunmap(struct dma_buf *dma_buf,
				  unsigned long page_num, void *addr)
{

}

#ifdef __NetBSD__
static int
drm_gem_dmabuf_mmap(struct dma_buf *dma_buf, off_t *offp, size_t size,
    int prot, int *flagsp, int *advicep, struct uvm_object **uobjp,
    int *maxprotp)
#else
static int drm_gem_dmabuf_mmap(struct dma_buf *dma_buf,
			       struct vm_area_struct *vma)
#endif
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct drm_device *dev = obj->dev;

	if (!dev->driver->gem_prime_mmap)
		return -ENOSYS;

#ifdef __NetBSD__
	return dev->driver->gem_prime_mmap(obj, offp, size, prot, flagsp,
	    advicep, uobjp, maxprotp);
#else
	return dev->driver->gem_prime_mmap(obj, vma);
#endif
}

static const struct dma_buf_ops drm_gem_prime_dmabuf_ops =  {
	.attach = drm_gem_map_attach,
	.detach = drm_gem_map_detach,
	.map_dma_buf = drm_gem_map_dma_buf,
	.unmap_dma_buf = drm_gem_unmap_dma_buf,
	.release = drm_gem_dmabuf_release,
	.kmap = drm_gem_dmabuf_kmap,
	.kmap_atomic = drm_gem_dmabuf_kmap_atomic,
	.kunmap = drm_gem_dmabuf_kunmap,
	.kunmap_atomic = drm_gem_dmabuf_kunmap_atomic,
	.mmap = drm_gem_dmabuf_mmap,
	.vmap = drm_gem_dmabuf_vmap,
	.vunmap = drm_gem_dmabuf_vunmap,
};

/**
 * DOC: PRIME Helpers
 *
 * Drivers can implement @gem_prime_export and @gem_prime_import in terms of
 * simpler APIs by using the helper functions @drm_gem_prime_export and
 * @drm_gem_prime_import.  These functions implement dma-buf support in terms of
 * six lower-level driver callbacks:
 *
 * Export callbacks:
 *
 *  - @gem_prime_pin (optional): prepare a GEM object for exporting
 *
 *  - @gem_prime_get_sg_table: provide a scatter/gather table of pinned pages
 *
 *  - @gem_prime_vmap: vmap a buffer exported by your driver
 *
 *  - @gem_prime_vunmap: vunmap a buffer exported by your driver
 *
 *  - @gem_prime_mmap (optional): mmap a buffer exported by your driver
 *
 * Import callback:
 *
 *  - @gem_prime_import_sg_table (import): produce a GEM object from another
 *    driver's scatter/gather table
 */

/**
 * drm_gem_prime_export - helper library implementation of the export callback
 * @dev: drm_device to export from
 * @obj: GEM object to export
 * @flags: flags like DRM_CLOEXEC
 *
 * This is the implementation of the gem_prime_export functions for GEM drivers
 * using the PRIME helpers.
 */
struct dma_buf *drm_gem_prime_export(struct drm_device *dev,
				     struct drm_gem_object *obj,
				     int flags)
{
	struct dma_buf_export_info exp_info = {
#ifndef __NetBSD__
		.exp_name = KBUILD_MODNAME, /* white lie for debug */
		.owner = dev->driver->fops->owner,
#endif
		.ops = &drm_gem_prime_dmabuf_ops,
		.size = obj->size,
		.flags = flags,
		.priv = obj,
	};

	if (dev->driver->gem_prime_res_obj)
		exp_info.resv = dev->driver->gem_prime_res_obj(obj);

	return dma_buf_export(&exp_info);
}
EXPORT_SYMBOL(drm_gem_prime_export);

static struct dma_buf *export_and_register_object(struct drm_device *dev,
						  struct drm_gem_object *obj,
						  uint32_t flags)
{
	struct dma_buf *dmabuf;

	/* prevent races with concurrent gem_close. */
	if (obj->handle_count == 0) {
		dmabuf = ERR_PTR(-ENOENT);
		return dmabuf;
	}

	dmabuf = dev->driver->gem_prime_export(dev, obj, flags);
	if (IS_ERR(dmabuf)) {
		/* normally the created dma-buf takes ownership of the ref,
		 * but if that fails then drop the ref
		 */
		return dmabuf;
	}

	/*
	 * Note that callers do not need to clean up the export cache
	 * since the check for obj->handle_count guarantees that someone
	 * will clean it up.
	 */
	obj->dma_buf = dmabuf;
	get_dma_buf(obj->dma_buf);
	/* Grab a new ref since the callers is now used by the dma-buf */
	drm_gem_object_reference(obj);

	return dmabuf;
}

/**
 * drm_gem_prime_handle_to_fd - PRIME export function for GEM drivers
 * @dev: dev to export the buffer from
 * @file_priv: drm file-private structure
 * @handle: buffer handle to export
 * @flags: flags like DRM_CLOEXEC
 * @prime_fd: pointer to storage for the fd id of the create dma-buf
 *
 * This is the PRIME export function which must be used mandatorily by GEM
 * drivers to ensure correct lifetime management of the underlying GEM object.
 * The actual exporting from GEM object to a dma-buf is done through the
 * gem_prime_export driver callback.
 */
int drm_gem_prime_handle_to_fd(struct drm_device *dev,
			       struct drm_file *file_priv, uint32_t handle,
			       uint32_t flags,
			       int *prime_fd)
{
	struct drm_gem_object *obj;
	int ret = 0;
	struct dma_buf *dmabuf;

	mutex_lock(&file_priv->prime.lock);
	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (!obj)  {
		ret = -ENOENT;
		goto out_unlock;
	}

	dmabuf = drm_prime_lookup_buf_by_handle(&file_priv->prime, handle);
	if (dmabuf) {
		get_dma_buf(dmabuf);
		goto out_have_handle;
	}

	mutex_lock(&dev->object_name_lock);
	/* re-export the original imported object */
	if (obj->import_attach) {
		dmabuf = obj->import_attach->dmabuf;
		get_dma_buf(dmabuf);
		goto out_have_obj;
	}

	if (obj->dma_buf) {
		get_dma_buf(obj->dma_buf);
		dmabuf = obj->dma_buf;
		goto out_have_obj;
	}

	dmabuf = export_and_register_object(dev, obj, flags);
	if (IS_ERR(dmabuf)) {
		/* normally the created dma-buf takes ownership of the ref,
		 * but if that fails then drop the ref
		 */
		ret = PTR_ERR(dmabuf);
		mutex_unlock(&dev->object_name_lock);
		goto out;
	}

out_have_obj:
	/*
	 * If we've exported this buffer then cheat and add it to the import list
	 * so we get the correct handle back. We must do this under the
	 * protection of dev->object_name_lock to ensure that a racing gem close
	 * ioctl doesn't miss to remove this buffer handle from the cache.
	 */
	ret = drm_prime_add_buf_handle(&file_priv->prime,
				       dmabuf, handle);
	mutex_unlock(&dev->object_name_lock);
	if (ret)
		goto fail_put_dmabuf;

out_have_handle:
	ret = dma_buf_fd(dmabuf, flags);
	/*
	 * We must _not_ remove the buffer from the handle cache since the newly
	 * created dma buf is already linked in the global obj->dma_buf pointer,
	 * and that is invariant as long as a userspace gem handle exists.
	 * Closing the handle will clean out the cache anyway, so we don't leak.
	 */
	if (ret < 0) {
		goto fail_put_dmabuf;
	} else {
		*prime_fd = ret;
		ret = 0;
	}

	goto out;

fail_put_dmabuf:
	dma_buf_put(dmabuf);
out:
	drm_gem_object_unreference_unlocked(obj);
out_unlock:
	mutex_unlock(&file_priv->prime.lock);

	return ret;
}
EXPORT_SYMBOL(drm_gem_prime_handle_to_fd);

/**
 * drm_gem_prime_import - helper library implementation of the import callback
 * @dev: drm_device to import into
 * @dma_buf: dma-buf object to import
 *
 * This is the implementation of the gem_prime_import functions for GEM drivers
 * using the PRIME helpers.
 */
struct drm_gem_object *drm_gem_prime_import(struct drm_device *dev,
					    struct dma_buf *dma_buf)
{
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct drm_gem_object *obj;
	int ret;

	if (dma_buf->ops == &drm_gem_prime_dmabuf_ops) {
		obj = dma_buf->priv;
		if (obj->dev == dev) {
			/*
			 * Importing dmabuf exported from out own gem increases
			 * refcount on gem itself instead of f_count of dmabuf.
			 */
			drm_gem_object_reference(obj);
			return obj;
		}
	}

	if (!dev->driver->gem_prime_import_sg_table)
		return ERR_PTR(-EINVAL);

	attach = dma_buf_attach(dma_buf, dev->dev);
	if (IS_ERR(attach))
		return ERR_CAST(attach);

	get_dma_buf(dma_buf);

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		goto fail_detach;
	}

	obj = dev->driver->gem_prime_import_sg_table(dev, attach, sgt);
	if (IS_ERR(obj)) {
		ret = PTR_ERR(obj);
		goto fail_unmap;
	}

	obj->import_attach = attach;

	return obj;

fail_unmap:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
fail_detach:
	dma_buf_detach(dma_buf, attach);
	dma_buf_put(dma_buf);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(drm_gem_prime_import);

/**
 * drm_gem_prime_fd_to_handle - PRIME import function for GEM drivers
 * @dev: dev to export the buffer from
 * @file_priv: drm file-private structure
 * @prime_fd: fd id of the dma-buf which should be imported
 * @handle: pointer to storage for the handle of the imported buffer object
 *
 * This is the PRIME import function which must be used mandatorily by GEM
 * drivers to ensure correct lifetime management of the underlying GEM object.
 * The actual importing of GEM object from the dma-buf is done through the
 * gem_import_export driver callback.
 */
int drm_gem_prime_fd_to_handle(struct drm_device *dev,
			       struct drm_file *file_priv, int prime_fd,
			       uint32_t *handle)
{
	struct dma_buf *dma_buf;
	struct drm_gem_object *obj;
	int ret;

	dma_buf = dma_buf_get(prime_fd);
	if (IS_ERR(dma_buf))
		return PTR_ERR(dma_buf);

	mutex_lock(&file_priv->prime.lock);

	ret = drm_prime_lookup_buf_handle(&file_priv->prime,
			dma_buf, handle);
	if (ret == 0)
		goto out_put;

	/* never seen this one, need to import */
	mutex_lock(&dev->object_name_lock);
	obj = dev->driver->gem_prime_import(dev, dma_buf);
	if (IS_ERR(obj)) {
		ret = PTR_ERR(obj);
		goto out_unlock;
	}

	if (obj->dma_buf) {
		WARN_ON(obj->dma_buf != dma_buf);
	} else {
		obj->dma_buf = dma_buf;
		get_dma_buf(dma_buf);
	}

	/* drm_gem_handle_create_tail unlocks dev->object_name_lock. */
	ret = drm_gem_handle_create_tail(file_priv, obj, handle);
	drm_gem_object_unreference_unlocked(obj);
	if (ret)
		goto out_put;

	ret = drm_prime_add_buf_handle(&file_priv->prime,
			dma_buf, *handle);
	if (ret)
		goto fail;

	mutex_unlock(&file_priv->prime.lock);

	dma_buf_put(dma_buf);

	return 0;

fail:
	/* hmm, if driver attached, we are relying on the free-object path
	 * to detach.. which seems ok..
	 */
	drm_gem_handle_delete(file_priv, *handle);
out_unlock:
	mutex_unlock(&dev->object_name_lock);
out_put:
	dma_buf_put(dma_buf);
	mutex_unlock(&file_priv->prime.lock);
	return ret;
}
EXPORT_SYMBOL(drm_gem_prime_fd_to_handle);

int drm_prime_handle_to_fd_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv)
{
	struct drm_prime_handle *args = data;
	uint32_t flags;

	if (!drm_core_check_feature(dev, DRIVER_PRIME))
		return -EINVAL;

	if (!dev->driver->prime_handle_to_fd)
		return -ENOSYS;

	/* check flags are valid */
	if (args->flags & ~DRM_CLOEXEC)
		return -EINVAL;

	/* we only want to pass DRM_CLOEXEC which is == O_CLOEXEC */
	flags = args->flags & DRM_CLOEXEC;

	return dev->driver->prime_handle_to_fd(dev, file_priv,
			args->handle, flags, &args->fd);
}

int drm_prime_fd_to_handle_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv)
{
	struct drm_prime_handle *args = data;

	if (!drm_core_check_feature(dev, DRIVER_PRIME))
		return -EINVAL;

	if (!dev->driver->prime_fd_to_handle)
		return -ENOSYS;

	return dev->driver->prime_fd_to_handle(dev, file_priv,
			args->fd, &args->handle);
}

/**
 * drm_prime_pages_to_sg - converts a page array into an sg list
 * @pages: pointer to the array of page pointers to convert
 * @nr_pages: length of the page vector
 *
 * This helper creates an sg table object from a set of pages
 * the driver is responsible for mapping the pages into the
 * importers address space for use with dma_buf itself.
 */
struct sg_table *drm_prime_pages_to_sg(struct page **pages, unsigned int nr_pages)
{
	struct sg_table *sg = NULL;
	int ret;

	sg = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!sg) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sg_alloc_table_from_pages(sg, pages, nr_pages, 0,
				nr_pages << PAGE_SHIFT, GFP_KERNEL);
	if (ret)
		goto out;

	return sg;
out:
	kfree(sg);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(drm_prime_pages_to_sg);

#ifdef __NetBSD__

struct sg_table *
drm_prime_bus_dmamem_to_sg(bus_dma_tag_t dmat, const bus_dma_segment_t *segs,
    int nsegs)
{
	struct sg_table *sg;
	int ret;

	sg = kmalloc(sizeof(*sg), GFP_KERNEL);
	if (sg == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sg_alloc_table_from_bus_dmamem(sg, dmat, segs, nsegs,
	    GFP_KERNEL);
	if (ret)
		goto out;

	return sg;
out:
	kfree(sg);
	return ERR_PTR(ret);
}

struct sg_table *
drm_prime_pglist_to_sg(struct pglist *pglist, unsigned npages)
{
	struct sg_table *sg;
	int ret;

	sg = kmalloc(sizeof(*sg), GFP_KERNEL);
	if (sg == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ret = sg_alloc_table_from_pglist(sg, pglist, 0, npages << PAGE_SHIFT,
	    npages, GFP_KERNEL);
	if (ret)
		goto out;

	return sg;

out:
	kfree(sg);
	return ERR_PTR(ret);
}

bus_size_t
drm_prime_sg_size(struct sg_table *sg)
{

	return sg->sgt_npgs << PAGE_SHIFT;
}

void
drm_prime_sg_free(struct sg_table *sg)
{

	sg_free_table(sg);
	kfree(sg);
}

int
drm_prime_sg_to_bus_dmamem(bus_dma_tag_t dmat, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, const struct sg_table *sgt)
{

	/* XXX errno NetBSD->Linux */
	return -bus_dmamem_import_pages(dmat, segs, nsegs, rsegs, sgt->sgt_pgs,
	    sgt->sgt_npgs);
}

int
drm_prime_bus_dmamap_load_sgt(bus_dma_tag_t dmat, bus_dmamap_t map,
    struct sg_table *sgt)
{
	bus_dma_segment_t *segs;
	bus_size_t size = drm_prime_sg_size(sgt);
	int nsegs = sgt->sgt_npgs;
	int ret;

	segs = kcalloc(sgt->sgt_npgs, sizeof(segs[0]), GFP_KERNEL);
	if (segs == NULL) {
		ret = -ENOMEM;
		goto out0;
	}

	ret = drm_prime_sg_to_bus_dmamem(dmat, segs, nsegs, &nsegs, sgt);
	if (ret)
		goto out1;
	KASSERT(nsegs <= sgt->sgt_npgs);

	/* XXX errno NetBSD->Linux */
	ret = -bus_dmamap_load_raw(dmat, map, segs, nsegs, size,
	    BUS_DMA_NOWAIT);
	if (ret)
		goto out1;

out1:	kfree(segs);
out0:	return ret;
}

bool
drm_prime_sg_importable(bus_dma_tag_t dmat, struct sg_table *sgt)
{
	unsigned i;

	for (i = 0; i < sgt->sgt_npgs; i++) {
		if (bus_dmatag_bounces_paddr(dmat, sgt->sgt_pgs[i]))
			return false;
	}
	return true;
}

#else  /* !__NetBSD__ */

/**
 * drm_prime_sg_to_page_addr_arrays - convert an sg table into a page array
 * @sgt: scatter-gather table to convert
 * @pages: array of page pointers to store the page array in
 * @addrs: optional array to store the dma bus address of each page
 * @max_pages: size of both the passed-in arrays
 *
 * Exports an sg table into an array of pages and addresses. This is currently
 * required by the TTM driver in order to do correct fault handling.
 */
int drm_prime_sg_to_page_addr_arrays(struct sg_table *sgt, struct page **pages,
				     dma_addr_t *addrs, int max_pages)
{
	unsigned count;
	struct scatterlist *sg;
	struct page *page;
	u32 len;
	int pg_index;
	dma_addr_t addr;

	pg_index = 0;
	for_each_sg(sgt->sgl, sg, sgt->nents, count) {
		len = sg->length;
		page = sg_page(sg);
		addr = sg_dma_address(sg);

		while (len > 0) {
			if (WARN_ON(pg_index >= max_pages))
				return -1;
			pages[pg_index] = page;
			if (addrs)
				addrs[pg_index] = addr;

			page++;
			addr += PAGE_SIZE;
			len -= PAGE_SIZE;
			pg_index++;
		}
	}
	return 0;
}
EXPORT_SYMBOL(drm_prime_sg_to_page_addr_arrays);

#endif	/* __NetBSD__ */

/**
 * drm_prime_gem_destroy - helper to clean up a PRIME-imported GEM object
 * @obj: GEM object which was created from a dma-buf
 * @sg: the sg-table which was pinned at import time
 *
 * This is the cleanup functions which GEM drivers need to call when they use
 * @drm_gem_prime_import to import dma-bufs.
 */
void drm_prime_gem_destroy(struct drm_gem_object *obj, struct sg_table *sg)
{
	struct dma_buf_attachment *attach;
	struct dma_buf *dma_buf;
	attach = obj->import_attach;
	if (sg)
		dma_buf_unmap_attachment(attach, sg, DMA_BIDIRECTIONAL);
	dma_buf = attach->dmabuf;
	dma_buf_detach(attach->dmabuf, attach);
	/* remove the reference */
	dma_buf_put(dma_buf);
}
EXPORT_SYMBOL(drm_prime_gem_destroy);

void drm_prime_init_file_private(struct drm_prime_file_private *prime_fpriv)
{
	INIT_LIST_HEAD(&prime_fpriv->head);
#ifdef __NetBSD__
	linux_mutex_init(&prime_fpriv->lock);
#else
	mutex_init(&prime_fpriv->lock);
#endif
}

void drm_prime_destroy_file_private(struct drm_prime_file_private *prime_fpriv)
{
	/* by now drm_gem_release should've made sure the list is empty */
	WARN_ON(!list_empty(&prime_fpriv->head));
#ifdef __NetBSD__
	linux_mutex_destroy(&prime_fpriv->lock);
#else
	mutex_destroy(&prime_fpriv->lock);
#endif
}
