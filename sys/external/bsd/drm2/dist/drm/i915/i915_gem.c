/*
 * Copyright © 2008 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef __NetBSD__
#if 0				/* XXX uvmhist option?  */
#include "opt_uvmhist.h"
#endif

#include <sys/types.h>
#include <sys/param.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_fault.h>
#include <uvm/uvm_page.h>
#include <uvm/uvm_pmap.h>
#include <uvm/uvm_prot.h>

#include <drm/bus_dma_hacks.h>
#endif

#include <drm/drmP.h>
#include <drm/drm_vma_manager.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"
#include "i915_trace.h"
#include "intel_drv.h"
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/pci.h>
#include <linux/dma-buf.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <asm/param.h>
#include <asm/page.h>

static void i915_gem_object_flush_gtt_write_domain(struct drm_i915_gem_object *obj);
static void i915_gem_object_flush_cpu_write_domain(struct drm_i915_gem_object *obj,
						   bool force);
static __must_check int
i915_gem_object_wait_rendering(struct drm_i915_gem_object *obj,
			       bool readonly);

static void i915_gem_write_fence(struct drm_device *dev, int reg,
				 struct drm_i915_gem_object *obj);
static void i915_gem_object_update_fence(struct drm_i915_gem_object *obj,
					 struct drm_i915_fence_reg *fence,
					 bool enable);

static unsigned long i915_gem_inactive_count(struct shrinker *shrinker,
					     struct shrink_control *sc);
static unsigned long i915_gem_inactive_scan(struct shrinker *shrinker,
					    struct shrink_control *sc);
static unsigned long i915_gem_purge(struct drm_i915_private *dev_priv, long target);
static unsigned long i915_gem_shrink_all(struct drm_i915_private *dev_priv);
static void i915_gem_object_truncate(struct drm_i915_gem_object *obj);
static void i915_gem_retire_requests_ring(struct intel_ring_buffer *ring);

static bool cpu_cache_is_coherent(struct drm_device *dev,
				  enum i915_cache_level level)
{
	return HAS_LLC(dev) || level != I915_CACHE_NONE;
}

static bool cpu_write_needs_clflush(struct drm_i915_gem_object *obj)
{
	if (!cpu_cache_is_coherent(obj->base.dev, obj->cache_level))
		return true;

	return obj->pin_display;
}

static inline void i915_gem_object_fence_lost(struct drm_i915_gem_object *obj)
{
	if (obj->tiling_mode)
		i915_gem_release_mmap(obj);

	/* As we do not have an associated fence register, we will force
	 * a tiling change if we ever need to acquire one.
	 */
	obj->fence_dirty = false;
	obj->fence_reg = I915_FENCE_REG_NONE;
}

/* some bookkeeping */
static void i915_gem_info_add_obj(struct drm_i915_private *dev_priv,
				  size_t size)
{
	spin_lock(&dev_priv->mm.object_stat_lock);
	dev_priv->mm.object_count++;
	dev_priv->mm.object_memory += size;
	spin_unlock(&dev_priv->mm.object_stat_lock);
}

static void i915_gem_info_remove_obj(struct drm_i915_private *dev_priv,
				     size_t size)
{
	spin_lock(&dev_priv->mm.object_stat_lock);
	dev_priv->mm.object_count--;
	dev_priv->mm.object_memory -= size;
	spin_unlock(&dev_priv->mm.object_stat_lock);
}

static int
i915_gem_wait_for_error(struct i915_gpu_error *error)
{
	int ret;

#define EXIT_COND (!i915_reset_in_progress(error) || \
		   i915_terminally_wedged(error))
	if (EXIT_COND)
		return 0;

	/*
	 * Only wait 10 seconds for the gpu reset to complete to avoid hanging
	 * userspace. If it takes that long something really bad is going on and
	 * we should simply try to bail out and fail as gracefully as possible.
	 */
#ifdef __NetBSD__
	spin_lock(&error->reset_lock);
	DRM_SPIN_TIMED_WAIT_UNTIL(ret, &error->reset_queue, &error->reset_lock,
	    10*HZ, EXIT_COND);
	spin_unlock(&error->reset_lock);
#else
	ret = wait_event_interruptible_timeout(error->reset_queue,
					       EXIT_COND,
					       10*HZ);
#endif
	if (ret == 0) {
		DRM_ERROR("Timed out waiting for the gpu reset to complete\n");
		return -EIO;
	} else if (ret < 0) {
		return ret;
	}
#undef EXIT_COND

	return 0;
}

int i915_mutex_lock_interruptible(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	ret = i915_gem_wait_for_error(&dev_priv->gpu_error);
	if (ret)
		return ret;

	ret = mutex_lock_interruptible(&dev->struct_mutex);
	if (ret)
		return ret;

	WARN_ON(i915_verify_lists(dev));
	return 0;
}

static inline bool
i915_gem_object_is_inactive(struct drm_i915_gem_object *obj)
{
	return i915_gem_obj_bound_any(obj) && !obj->active;
}

int
i915_gem_init_ioctl(struct drm_device *dev, void *data,
		    struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_init *args = data;

	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return -ENODEV;

	if (args->gtt_start >= args->gtt_end ||
	    (args->gtt_end | args->gtt_start) & (PAGE_SIZE - 1))
		return -EINVAL;

	/* GEM with user mode setting was never supported on ilk and later. */
	if (INTEL_INFO(dev)->gen >= 5)
		return -ENODEV;

	mutex_lock(&dev->struct_mutex);
	i915_gem_setup_global_gtt(dev, args->gtt_start, args->gtt_end,
				  args->gtt_end);
	dev_priv->gtt.mappable_end = args->gtt_end;
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

int
i915_gem_get_aperture_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_get_aperture *args = data;
	struct drm_i915_gem_object *obj;
	size_t pinned;

	pinned = 0;
	mutex_lock(&dev->struct_mutex);
	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_list)
		if (i915_gem_obj_is_pinned(obj))
			pinned += i915_gem_obj_ggtt_size(obj);
	mutex_unlock(&dev->struct_mutex);

	args->aper_size = dev_priv->gtt.base.total;
	args->aper_available_size = args->aper_size - pinned;

	return 0;
}

static void i915_gem_object_detach_phys(struct drm_i915_gem_object *obj)
{
	drm_dma_handle_t *phys = obj->phys_handle;

	if (!phys)
		return;

	if (obj->madv == I915_MADV_WILLNEED) {
#ifdef __NetBSD__
		const char *vaddr = phys->vaddr;
		unsigned i;

		for (i = 0; i < obj->base.size / PAGE_SIZE; i++) {
			struct pglist pages;
			int error;

			TAILQ_INIT(&pages);
			error = uvm_obj_wirepages(obj->base.gemo_shm_uao,
			    i*PAGE_SIZE, (i+1)*PAGE_SIZE, &pages);
			if (error)
				continue;

			struct vm_page *const vm_page = TAILQ_FIRST(&pages);
			struct page *const page = container_of(vm_page,
			    struct page, p_vmp);
			char *const dst = kmap_atomic(page);
			(void)memcpy(dst, vaddr + (i*PAGE_SIZE), PAGE_SIZE);
			drm_clflush_virt_range(dst, PAGE_SIZE);
			kunmap_atomic(dst);

			vm_page->flags &= ~PG_CLEAN;
			/* XXX mark page accessed */
			uvm_obj_unwirepages(obj->base.gemo_shm_uao,
			    i*PAGE_SIZE, (i+1)*PAGE_SIZE);
		}
#else
		struct address_space *mapping = file_inode(obj->base.filp)->i_mapping;
		char *vaddr = phys->vaddr;
		int i;

		for (i = 0; i < obj->base.size / PAGE_SIZE; i++) {
			struct page *page = shmem_read_mapping_page(mapping, i);
			if (!IS_ERR(page)) {
				char *dst = kmap_atomic(page);
				memcpy(dst, vaddr, PAGE_SIZE);
				drm_clflush_virt_range(dst, PAGE_SIZE);
				kunmap_atomic(dst);

				set_page_dirty(page);
				mark_page_accessed(page);
				page_cache_release(page);
			}
			vaddr += PAGE_SIZE;
		}
#endif
		i915_gem_chipset_flush(obj->base.dev);
	}

#ifndef __NetBSD__
#ifdef CONFIG_X86
	set_memory_wb((unsigned long)phys->vaddr, phys->size / PAGE_SIZE);
#endif
#endif
	drm_pci_free(obj->base.dev, phys);
	obj->phys_handle = NULL;
}

int
i915_gem_object_attach_phys(struct drm_i915_gem_object *obj,
			    int align)
{
	drm_dma_handle_t *phys;
#ifndef __NetBSD__
	struct address_space *mapping;
#endif
	char *vaddr;
	int i;

	if (obj->phys_handle) {
		if ((unsigned long)obj->phys_handle->vaddr & (align -1))
			return -EBUSY;

		return 0;
	}

	if (obj->madv != I915_MADV_WILLNEED)
		return -EFAULT;

#ifdef __NetBSD__
	if (obj->base.gemo_shm_uao == NULL)
		return -EINVAL;
#else
	if (obj->base.filp == NULL)
		return -EINVAL;
#endif

	/* create a new object */
	phys = drm_pci_alloc(obj->base.dev, obj->base.size, align);
	if (!phys)
		return -ENOMEM;

	vaddr = phys->vaddr;
#ifndef __NetBSD__
#ifdef CONFIG_X86
	set_memory_wc((unsigned long)vaddr, phys->size / PAGE_SIZE);
#endif
	mapping = file_inode(obj->base.filp)->i_mapping;
#endif
	for (i = 0; i < obj->base.size / PAGE_SIZE; i++) {
		struct page *page;
		char *src;

#ifdef __NetBSD__
		struct pglist pages;
		int ret;

		TAILQ_INIT(&pages);

		/* XXX errno NetBSD->Linux */
		ret = -uvm_obj_wirepages(obj->base.gemo_shm_uao, i*PAGE_SIZE,
		    (i+1)*PAGE_SIZE, &pages);
		if (ret) {
			drm_pci_free(obj->base.dev, phys);
			return ret;
		}
		KASSERT(!TAILQ_EMPTY(&pages));
		page = container_of(TAILQ_FIRST(&pages), struct page, p_vmp);
#else
		page = shmem_read_mapping_page(mapping, i);
		if (IS_ERR(page)) {
#ifdef CONFIG_X86
			set_memory_wb((unsigned long)phys->vaddr, phys->size / PAGE_SIZE);
#endif
			drm_pci_free(obj->base.dev, phys);
			return PTR_ERR(page);
		}
#endif	/* defined(__NetBSD__) */

		src = kmap_atomic(page);
		memcpy(vaddr, src, PAGE_SIZE);
		kunmap_atomic(src);

#ifndef __NetBSD__
		mark_page_accessed(page);
		page_cache_release(page);
#endif

		vaddr += PAGE_SIZE;
	}

	obj->phys_handle = phys;
	return 0;
}

static int
i915_gem_phys_pwrite(struct drm_i915_gem_object *obj,
		     struct drm_i915_gem_pwrite *args,
		     struct drm_file *file_priv)
{
	struct drm_device *dev = obj->base.dev;
	void *vaddr = (char *)obj->phys_handle->vaddr + args->offset;
	char __user *user_data = to_user_ptr(args->data_ptr);

	if (__copy_from_user_inatomic_nocache(vaddr, user_data, args->size)) {
		unsigned long unwritten;

		/* The physical object once assigned is fixed for the lifetime
		 * of the obj, so we can safely drop the lock and continue
		 * to access vaddr.
		 */
		mutex_unlock(&dev->struct_mutex);
		unwritten = copy_from_user(vaddr, user_data, args->size);
		mutex_lock(&dev->struct_mutex);
		if (unwritten)
			return -EFAULT;
	}

	i915_gem_chipset_flush(dev);
	return 0;
}

void *i915_gem_object_alloc(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	return kmem_cache_zalloc(dev_priv->slab, GFP_KERNEL);
}

void i915_gem_object_free(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	kmem_cache_free(dev_priv->slab, obj);
}

static int
i915_gem_create(struct drm_file *file,
		struct drm_device *dev,
		uint64_t size,
		uint32_t *handle_p)
{
	struct drm_i915_gem_object *obj;
	int ret;
	u32 handle;

	size = roundup(size, PAGE_SIZE);
	if (size == 0)
		return -EINVAL;

	/* Allocate the new object */
	obj = i915_gem_alloc_object(dev, size);
	if (obj == NULL)
		return -ENOMEM;

	ret = drm_gem_handle_create(file, &obj->base, &handle);
	/* drop reference from allocate - handle holds it now */
	drm_gem_object_unreference_unlocked(&obj->base);
	if (ret)
		return ret;

	*handle_p = handle;
	return 0;
}

int
i915_gem_dumb_create(struct drm_file *file,
		     struct drm_device *dev,
		     struct drm_mode_create_dumb *args)
{
	/* have to work out size/pitch and return them */
#ifdef __NetBSD__		/* ALIGN means something else.  */
	args->pitch = round_up(args->width * DIV_ROUND_UP(args->bpp, 8), 64);
#else
	args->pitch = ALIGN(args->width * DIV_ROUND_UP(args->bpp, 8), 64);
#endif
	args->size = args->pitch * args->height;
	return i915_gem_create(file, dev,
			       args->size, &args->handle);
}

/**
 * Creates a new mm object and returns a handle to it.
 */
int
i915_gem_create_ioctl(struct drm_device *dev, void *data,
		      struct drm_file *file)
{
	struct drm_i915_gem_create *args = data;

	return i915_gem_create(file, dev,
			       args->size, &args->handle);
}

static inline int
__copy_to_user_swizzled(char __user *cpu_vaddr,
			const char *gpu_vaddr, int gpu_offset,
			int length)
{
	int ret, cpu_offset = 0;

	while (length > 0) {
#ifdef __NetBSD__		/* XXX ALIGN means something else.  */
		int cacheline_end = round_up(gpu_offset + 1, 64);
#else
		int cacheline_end = ALIGN(gpu_offset + 1, 64);
#endif
		int this_length = min(cacheline_end - gpu_offset, length);
		int swizzled_gpu_offset = gpu_offset ^ 64;

		ret = __copy_to_user(cpu_vaddr + cpu_offset,
				     gpu_vaddr + swizzled_gpu_offset,
				     this_length);
		if (ret)
			return ret + length;

		cpu_offset += this_length;
		gpu_offset += this_length;
		length -= this_length;
	}

	return 0;
}

static inline int
__copy_from_user_swizzled(char *gpu_vaddr, int gpu_offset,
			  const char __user *cpu_vaddr,
			  int length)
{
	int ret, cpu_offset = 0;

	while (length > 0) {
#ifdef __NetBSD__		/* XXX ALIGN means something else.  */
		int cacheline_end = round_up(gpu_offset + 1, 64);
#else
		int cacheline_end = ALIGN(gpu_offset + 1, 64);
#endif
		int this_length = min(cacheline_end - gpu_offset, length);
		int swizzled_gpu_offset = gpu_offset ^ 64;

		ret = __copy_from_user(gpu_vaddr + swizzled_gpu_offset,
				       cpu_vaddr + cpu_offset,
				       this_length);
		if (ret)
			return ret + length;

		cpu_offset += this_length;
		gpu_offset += this_length;
		length -= this_length;
	}

	return 0;
}

/*
 * Pins the specified object's pages and synchronizes the object with
 * GPU accesses. Sets needs_clflush to non-zero if the caller should
 * flush the object from the CPU cache.
 */
int i915_gem_obj_prepare_shmem_read(struct drm_i915_gem_object *obj,
				    int *needs_clflush)
{
	int ret;

	*needs_clflush = 0;

#ifdef __NetBSD__
	if (obj->base.gemo_shm_uao == NULL)
		return -EINVAL;
#else
	if (!obj->base.filp)
		return -EINVAL;
#endif

	if (!(obj->base.read_domains & I915_GEM_DOMAIN_CPU)) {
		/* If we're not in the cpu read domain, set ourself into the gtt
		 * read domain and manually flush cachelines (if required). This
		 * optimizes for the case when the gpu will dirty the data
		 * anyway again before the next pread happens. */
		*needs_clflush = !cpu_cache_is_coherent(obj->base.dev,
							obj->cache_level);
		ret = i915_gem_object_wait_rendering(obj, true);
		if (ret)
			return ret;
	}

	ret = i915_gem_object_get_pages(obj);
	if (ret)
		return ret;

	i915_gem_object_pin_pages(obj);

	return ret;
}

/* Per-page copy function for the shmem pread fastpath.
 * Flushes invalid cachelines before reading the target if
 * needs_clflush is set. */
static int
shmem_pread_fast(struct page *page, int shmem_page_offset, int page_length,
		 char __user *user_data,
		 bool page_do_bit17_swizzling, bool needs_clflush)
{
#ifdef __NetBSD__		/* XXX atomic shmem fast path */
	return -EFAULT;
#else
	char *vaddr;
	int ret;

	if (unlikely(page_do_bit17_swizzling))
		return -EINVAL;

	vaddr = kmap_atomic(page);
	if (needs_clflush)
		drm_clflush_virt_range(vaddr + shmem_page_offset,
				       page_length);
	ret = __copy_to_user_inatomic(user_data,
				      vaddr + shmem_page_offset,
				      page_length);
	kunmap_atomic(vaddr);

	return ret ? -EFAULT : 0;
#endif
}

static void
shmem_clflush_swizzled_range(char *addr, unsigned long length,
			     bool swizzled)
{
	if (unlikely(swizzled)) {
		unsigned long start = (unsigned long) addr;
		unsigned long end = (unsigned long) addr + length;

		/* For swizzling simply ensure that we always flush both
		 * channels. Lame, but simple and it works. Swizzled
		 * pwrite/pread is far from a hotpath - current userspace
		 * doesn't use it at all. */
		start = round_down(start, 128);
		end = round_up(end, 128);

		drm_clflush_virt_range((void *)start, end - start);
	} else {
		drm_clflush_virt_range(addr, length);
	}

}

/* Only difference to the fast-path function is that this can handle bit17
 * and uses non-atomic copy and kmap functions. */
static int
shmem_pread_slow(struct page *page, int shmem_page_offset, int page_length,
		 char __user *user_data,
		 bool page_do_bit17_swizzling, bool needs_clflush)
{
	char *vaddr;
	int ret;

	vaddr = kmap(page);
	if (needs_clflush)
		shmem_clflush_swizzled_range(vaddr + shmem_page_offset,
					     page_length,
					     page_do_bit17_swizzling);

	if (page_do_bit17_swizzling)
		ret = __copy_to_user_swizzled(user_data,
					      vaddr, shmem_page_offset,
					      page_length);
	else
		ret = __copy_to_user(user_data,
				     vaddr + shmem_page_offset,
				     page_length);
	kunmap(page);

	return ret ? - EFAULT : 0;
}

static int
i915_gem_shmem_pread(struct drm_device *dev,
		     struct drm_i915_gem_object *obj,
		     struct drm_i915_gem_pread *args,
		     struct drm_file *file)
{
	char __user *user_data;
	ssize_t remain;
	loff_t offset;
	int shmem_page_offset, page_length, ret = 0;
	int obj_do_bit17_swizzling, page_do_bit17_swizzling;
#ifndef __NetBSD__		/* XXX */
	int prefaulted = 0;
#endif
	int needs_clflush = 0;
#ifndef __NetBSD__
	struct sg_page_iter sg_iter;
#endif

	user_data = to_user_ptr(args->data_ptr);
	remain = args->size;

	obj_do_bit17_swizzling = i915_gem_object_needs_bit17_swizzle(obj);

	ret = i915_gem_obj_prepare_shmem_read(obj, &needs_clflush);
	if (ret)
		return ret;

	offset = args->offset;

#ifdef __NetBSD__
	/*
	 * XXX This is a big #ifdef with a lot of duplicated code, but
	 * factoring out the loop head -- which is all that
	 * substantially differs -- is probably more trouble than it's
	 * worth at the moment.
	 */
	while (0 < remain) {
		/* Get the next page.  */
		shmem_page_offset = offset_in_page(offset);
		KASSERT(shmem_page_offset < PAGE_SIZE);
		page_length = MIN(remain, (PAGE_SIZE - shmem_page_offset));
		struct page *const page = i915_gem_object_get_page(obj,
		    atop(offset));

		/* Decide whether to swizzle bit 17.  */
		page_do_bit17_swizzling = obj_do_bit17_swizzling &&
		    (page_to_phys(page) & (1 << 17)) != 0;

		/* Try the fast path.  */
		ret = shmem_pread_fast(page, shmem_page_offset, page_length,
		    user_data, page_do_bit17_swizzling, needs_clflush);
		if (ret == 0)
			goto next_page;

		/* Fast path failed.  Try the slow path.  */
		mutex_unlock(&dev->struct_mutex);
		/* XXX prefault */
		ret = shmem_pread_slow(page, shmem_page_offset, page_length,
		    user_data, page_do_bit17_swizzling, needs_clflush);
		mutex_lock(&dev->struct_mutex);
		if (ret)
			goto out;

next_page:	KASSERT(page_length <= remain);
		remain -= page_length;
		user_data += page_length;
		offset += page_length;
	}
#else
	for_each_sg_page(obj->pages->sgl, &sg_iter, obj->pages->nents,
			 offset >> PAGE_SHIFT) {
		struct page *page = sg_page_iter_page(&sg_iter);

		if (remain <= 0)
			break;

		/* Operation in this page
		 *
		 * shmem_page_offset = offset within page in shmem file
		 * page_length = bytes to copy for this page
		 */
		shmem_page_offset = offset_in_page(offset);
		page_length = remain;
		if ((shmem_page_offset + page_length) > PAGE_SIZE)
			page_length = PAGE_SIZE - shmem_page_offset;

		page_do_bit17_swizzling = obj_do_bit17_swizzling &&
			(page_to_phys(page) & (1 << 17)) != 0;

		ret = shmem_pread_fast(page, shmem_page_offset, page_length,
				       user_data, page_do_bit17_swizzling,
				       needs_clflush);
		if (ret == 0)
			goto next_page;

		mutex_unlock(&dev->struct_mutex);

		if (likely(!i915.prefault_disable) && !prefaulted) {
			ret = fault_in_multipages_writeable(user_data, remain);
			/* Userspace is tricking us, but we've already clobbered
			 * its pages with the prefault and promised to write the
			 * data up to the first fault. Hence ignore any errors
			 * and just continue. */
			(void)ret;
			prefaulted = 1;
		}

		ret = shmem_pread_slow(page, shmem_page_offset, page_length,
				       user_data, page_do_bit17_swizzling,
				       needs_clflush);

		mutex_lock(&dev->struct_mutex);

		if (ret)
			goto out;

next_page:
		remain -= page_length;
		user_data += page_length;
		offset += page_length;
	}
#endif

out:
	i915_gem_object_unpin_pages(obj);

	return ret;
}

/**
 * Reads data from the object referenced by handle.
 *
 * On error, the contents of *data are undefined.
 */
int
i915_gem_pread_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *file)
{
	struct drm_i915_gem_pread *args = data;
	struct drm_i915_gem_object *obj;
	int ret = 0;

	if (args->size == 0)
		return 0;

	if (!access_ok(VERIFY_WRITE,
		       to_user_ptr(args->data_ptr),
		       args->size))
		return -EFAULT;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Bounds check source.  */
	if (args->offset > obj->base.size ||
	    args->size > obj->base.size - args->offset) {
		ret = -EINVAL;
		goto out;
	}

	/* prime objects have no backing filp to GEM pread/pwrite
	 * pages from.
	 */
#ifdef __NetBSD__
	/* Also stolen objects.  */
	if (obj->base.gemo_shm_uao == NULL) {
		ret = -EINVAL;
		goto out;
	}
#else
	if (!obj->base.filp) {
		ret = -EINVAL;
		goto out;
	}
#endif

	trace_i915_gem_object_pread(obj, args->offset, args->size);

	ret = i915_gem_shmem_pread(dev, obj, args, file);

out:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

/* This is the fast write path which cannot handle
 * page faults in the source data
 */

static inline int
fast_user_write(struct io_mapping *mapping,
		loff_t page_base, int page_offset,
		char __user *user_data,
		int length)
{
#ifdef __NetBSD__		/* XXX atomic shmem fast path */
	return -EFAULT;
#else
	void __iomem *vaddr_atomic;
	void *vaddr;
	unsigned long unwritten;

	vaddr_atomic = io_mapping_map_atomic_wc(mapping, page_base);
	/* We can use the cpu mem copy function because this is X86. */
	vaddr = (void __force*)vaddr_atomic + page_offset;
	unwritten = __copy_from_user_inatomic_nocache(vaddr,
						      user_data, length);
	io_mapping_unmap_atomic(vaddr_atomic);
	return unwritten;
#endif
}

/**
 * This is the fast pwrite path, where we copy the data directly from the
 * user into the GTT, uncached.
 */
static int
i915_gem_gtt_pwrite_fast(struct drm_device *dev,
			 struct drm_i915_gem_object *obj,
			 struct drm_i915_gem_pwrite *args,
			 struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	ssize_t remain;
	loff_t offset, page_base;
	char __user *user_data;
	int page_offset, page_length, ret;

	ret = i915_gem_obj_ggtt_pin(obj, 0, PIN_MAPPABLE | PIN_NONBLOCK);
	if (ret)
		goto out;

	ret = i915_gem_object_set_to_gtt_domain(obj, true);
	if (ret)
		goto out_unpin;

	ret = i915_gem_object_put_fence(obj);
	if (ret)
		goto out_unpin;

	user_data = to_user_ptr(args->data_ptr);
	remain = args->size;

	offset = i915_gem_obj_ggtt_offset(obj) + args->offset;

	while (remain > 0) {
		/* Operation in this page
		 *
		 * page_base = page offset within aperture
		 * page_offset = offset within page
		 * page_length = bytes to copy for this page
		 */
		page_base = offset & PAGE_MASK;
		page_offset = offset_in_page(offset);
		page_length = remain;
		if ((page_offset + remain) > PAGE_SIZE)
			page_length = PAGE_SIZE - page_offset;

		/* If we get a fault while copying data, then (presumably) our
		 * source page isn't available.  Return the error and we'll
		 * retry in the slow path.
		 */
		if (fast_user_write(dev_priv->gtt.mappable, page_base,
				    page_offset, user_data, page_length)) {
			ret = -EFAULT;
			goto out_unpin;
		}

		remain -= page_length;
		user_data += page_length;
		offset += page_length;
	}

out_unpin:
	i915_gem_object_ggtt_unpin(obj);
out:
	return ret;
}

/* Per-page copy function for the shmem pwrite fastpath.
 * Flushes invalid cachelines before writing to the target if
 * needs_clflush_before is set and flushes out any written cachelines after
 * writing if needs_clflush is set. */
static int
shmem_pwrite_fast(struct page *page, int shmem_page_offset, int page_length,
		  char __user *user_data,
		  bool page_do_bit17_swizzling,
		  bool needs_clflush_before,
		  bool needs_clflush_after)
{
#ifdef __NetBSD__
	return -EFAULT;
#else
	char *vaddr;
	int ret;

	if (unlikely(page_do_bit17_swizzling))
		return -EINVAL;

	vaddr = kmap_atomic(page);
	if (needs_clflush_before)
		drm_clflush_virt_range(vaddr + shmem_page_offset,
				       page_length);
	ret = __copy_from_user_inatomic(vaddr + shmem_page_offset,
					user_data, page_length);
	if (needs_clflush_after)
		drm_clflush_virt_range(vaddr + shmem_page_offset,
				       page_length);
	kunmap_atomic(vaddr);

	return ret ? -EFAULT : 0;
#endif
}

/* Only difference to the fast-path function is that this can handle bit17
 * and uses non-atomic copy and kmap functions. */
static int
shmem_pwrite_slow(struct page *page, int shmem_page_offset, int page_length,
		  char __user *user_data,
		  bool page_do_bit17_swizzling,
		  bool needs_clflush_before,
		  bool needs_clflush_after)
{
	char *vaddr;
	int ret;

	vaddr = kmap(page);
	if (unlikely(needs_clflush_before || page_do_bit17_swizzling))
		shmem_clflush_swizzled_range(vaddr + shmem_page_offset,
					     page_length,
					     page_do_bit17_swizzling);
	if (page_do_bit17_swizzling)
		ret = __copy_from_user_swizzled(vaddr, shmem_page_offset,
						user_data,
						page_length);
	else
		ret = __copy_from_user(vaddr + shmem_page_offset,
				       user_data,
				       page_length);
	if (needs_clflush_after)
		shmem_clflush_swizzled_range(vaddr + shmem_page_offset,
					     page_length,
					     page_do_bit17_swizzling);
	kunmap(page);

	return ret ? -EFAULT : 0;
}

static int
i915_gem_shmem_pwrite(struct drm_device *dev,
		      struct drm_i915_gem_object *obj,
		      struct drm_i915_gem_pwrite *args,
		      struct drm_file *file)
{
	ssize_t remain;
	loff_t offset;
	char __user *user_data;
	int shmem_page_offset, page_length, ret = 0;
	int obj_do_bit17_swizzling, page_do_bit17_swizzling;
	int hit_slowpath = 0;
	int needs_clflush_after = 0;
	int needs_clflush_before = 0;
#ifndef __NetBSD__
	struct sg_page_iter sg_iter;
#endif

	user_data = to_user_ptr(args->data_ptr);
	remain = args->size;

	obj_do_bit17_swizzling = i915_gem_object_needs_bit17_swizzle(obj);

	if (obj->base.write_domain != I915_GEM_DOMAIN_CPU) {
		/* If we're not in the cpu write domain, set ourself into the gtt
		 * write domain and manually flush cachelines (if required). This
		 * optimizes for the case when the gpu will use the data
		 * right away and we therefore have to clflush anyway. */
		needs_clflush_after = cpu_write_needs_clflush(obj);
		ret = i915_gem_object_wait_rendering(obj, false);
		if (ret)
			return ret;
	}
	/* Same trick applies to invalidate partially written cachelines read
	 * before writing. */
	if ((obj->base.read_domains & I915_GEM_DOMAIN_CPU) == 0)
		needs_clflush_before =
			!cpu_cache_is_coherent(dev, obj->cache_level);

	ret = i915_gem_object_get_pages(obj);
	if (ret)
		return ret;

	i915_gem_object_pin_pages(obj);

	offset = args->offset;
	obj->dirty = 1;

#ifdef __NetBSD__
	while (0 < remain) {
		/* Get the next page.  */
		shmem_page_offset = offset_in_page(offset);
		KASSERT(shmem_page_offset < PAGE_SIZE);
		page_length = MIN(remain, (PAGE_SIZE - shmem_page_offset));
		struct page *const page = i915_gem_object_get_page(obj,
		    atop(offset));

		/* Decide whether to flush the cache or swizzle bit 17.  */
		const bool partial_cacheline_write = needs_clflush_before &&
		    ((shmem_page_offset | page_length)
			& (cpu_info_primary.ci_cflush_lsize - 1));
		page_do_bit17_swizzling = obj_do_bit17_swizzling &&
		    (page_to_phys(page) & (1 << 17)) != 0;

		/* Try the fast path.  */
		ret = shmem_pwrite_fast(page, shmem_page_offset, page_length,
		    user_data, page_do_bit17_swizzling,
		    partial_cacheline_write, needs_clflush_after);
		if (ret == 0)
			goto next_page;

		/* Fast path failed.  Try the slow path.  */
		hit_slowpath = 1;
		mutex_unlock(&dev->struct_mutex);
		ret = shmem_pwrite_slow(page, shmem_page_offset, page_length,
		    user_data, page_do_bit17_swizzling,
		    partial_cacheline_write, needs_clflush_after);
		mutex_lock(&dev->struct_mutex);
		if (ret)
			goto out;

next_page:	KASSERT(page_length <= remain);
		remain -= page_length;
		user_data += page_length;
		offset += page_length;
	}
#else
	for_each_sg_page(obj->pages->sgl, &sg_iter, obj->pages->nents,
			 offset >> PAGE_SHIFT) {
		struct page *page = sg_page_iter_page(&sg_iter);
		int partial_cacheline_write;

		if (remain <= 0)
			break;

		/* Operation in this page
		 *
		 * shmem_page_offset = offset within page in shmem file
		 * page_length = bytes to copy for this page
		 */
		shmem_page_offset = offset_in_page(offset);

		page_length = remain;
		if ((shmem_page_offset + page_length) > PAGE_SIZE)
			page_length = PAGE_SIZE - shmem_page_offset;

		/* If we don't overwrite a cacheline completely we need to be
		 * careful to have up-to-date data by first clflushing. Don't
		 * overcomplicate things and flush the entire patch. */
		partial_cacheline_write = needs_clflush_before &&
			((shmem_page_offset | page_length)
				& (boot_cpu_data.x86_clflush_size - 1));

		page_do_bit17_swizzling = obj_do_bit17_swizzling &&
			(page_to_phys(page) & (1 << 17)) != 0;

		ret = shmem_pwrite_fast(page, shmem_page_offset, page_length,
					user_data, page_do_bit17_swizzling,
					partial_cacheline_write,
					needs_clflush_after);
		if (ret == 0)
			goto next_page;

		hit_slowpath = 1;
		mutex_unlock(&dev->struct_mutex);
		ret = shmem_pwrite_slow(page, shmem_page_offset, page_length,
					user_data, page_do_bit17_swizzling,
					partial_cacheline_write,
					needs_clflush_after);

		mutex_lock(&dev->struct_mutex);

		if (ret)
			goto out;

next_page:
		remain -= page_length;
		user_data += page_length;
		offset += page_length;
	}
#endif

out:
	i915_gem_object_unpin_pages(obj);

	if (hit_slowpath) {
		/*
		 * Fixup: Flush cpu caches in case we didn't flush the dirty
		 * cachelines in-line while writing and the object moved
		 * out of the cpu write domain while we've dropped the lock.
		 */
		if (!needs_clflush_after &&
		    obj->base.write_domain != I915_GEM_DOMAIN_CPU) {
			if (i915_gem_clflush_object(obj, obj->pin_display))
				i915_gem_chipset_flush(dev);
		}
	}

	if (needs_clflush_after)
		i915_gem_chipset_flush(dev);

	return ret;
}

/**
 * Writes data to the object referenced by handle.
 *
 * On error, the contents of the buffer that were to be modified are undefined.
 */
int
i915_gem_pwrite_ioctl(struct drm_device *dev, void *data,
		      struct drm_file *file)
{
	struct drm_i915_gem_pwrite *args = data;
	struct drm_i915_gem_object *obj;
	int ret;

	if (args->size == 0)
		return 0;

	if (!access_ok(VERIFY_READ,
		       to_user_ptr(args->data_ptr),
		       args->size))
		return -EFAULT;

#ifndef __NetBSD__		/* XXX prefault */
	if (likely(!i915.prefault_disable)) {
		ret = fault_in_multipages_readable(to_user_ptr(args->data_ptr),
						   args->size);
		if (ret)
			return -EFAULT;
	}
#endif

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Bounds check destination. */
	if (args->offset > obj->base.size ||
	    args->size > obj->base.size - args->offset) {
		ret = -EINVAL;
		goto out;
	}

	/* prime objects have no backing filp to GEM pread/pwrite
	 * pages from.
	 */
#ifdef __NetBSD__
	/* Also stolen objects.  */
	if (obj->base.gemo_shm_uao == NULL) {
		ret = -EINVAL;
		goto out;
	}
#else
	if (!obj->base.filp) {
		ret = -EINVAL;
		goto out;
	}
#endif

	trace_i915_gem_object_pwrite(obj, args->offset, args->size);

	ret = -EFAULT;
	/* We can only do the GTT pwrite on untiled buffers, as otherwise
	 * it would end up going through the fenced access, and we'll get
	 * different detiling behavior between reading and writing.
	 * pread/pwrite currently are reading and writing from the CPU
	 * perspective, requiring manual detiling by the client.
	 */
	if (obj->phys_handle) {
		ret = i915_gem_phys_pwrite(obj, args, file);
		goto out;
	}

	if (obj->tiling_mode == I915_TILING_NONE &&
	    obj->base.write_domain != I915_GEM_DOMAIN_CPU &&
	    cpu_write_needs_clflush(obj)) {
		ret = i915_gem_gtt_pwrite_fast(dev, obj, args, file);
		/* Note that the gtt paths might fail with non-page-backed user
		 * pointers (e.g. gtt mappings when moving data between
		 * textures). Fallback to the shmem path in that case. */
	}

	if (ret == -EFAULT || ret == -ENOSPC)
		ret = i915_gem_shmem_pwrite(dev, obj, args, file);

out:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int
i915_gem_check_wedge(struct i915_gpu_error *error,
		     bool interruptible)
{
	if (i915_reset_in_progress(error)) {
		/* Non-interruptible callers can't handle -EAGAIN, hence return
		 * -EIO unconditionally for these. */
		if (!interruptible)
			return -EIO;

		/* Recovery complete, but the reset failed ... */
		if (i915_terminally_wedged(error))
			return -EIO;

		return -EAGAIN;
	}

	return 0;
}

/*
 * Compare seqno against outstanding lazy request. Emit a request if they are
 * equal.
 */
static int
i915_gem_check_olr(struct intel_ring_buffer *ring, u32 seqno)
{
	int ret;

	BUG_ON(!mutex_is_locked(&ring->dev->struct_mutex));

	ret = 0;
	if (seqno == ring->outstanding_lazy_seqno)
		ret = i915_add_request(ring, NULL);

	return ret;
}

#ifndef __NetBSD__
static void fake_irq(unsigned long data)
{
	wake_up_process((struct task_struct *)data);
}
#endif

static bool missed_irq(struct drm_i915_private *dev_priv,
		       struct intel_ring_buffer *ring)
{
	return test_bit(ring->id, &dev_priv->gpu_error.missed_irq_rings);
}

static bool can_wait_boost(struct drm_i915_file_private *file_priv)
{
	if (file_priv == NULL)
		return true;

	return !atomic_xchg(&file_priv->rps_wait_boost, true);
}

/**
 * __wait_seqno - wait until execution of seqno has finished
 * @ring: the ring expected to report seqno
 * @seqno: duh!
 * @reset_counter: reset sequence associated with the given seqno
 * @interruptible: do an interruptible wait (normally yes)
 * @timeout: in - how long to wait (NULL forever); out - how much time remaining
 *
 * Note: It is of utmost importance that the passed in seqno and reset_counter
 * values have been read by the caller in an smp safe manner. Where read-side
 * locks are involved, it is sufficient to read the reset_counter before
 * unlocking the lock that protects the seqno. For lockless tricks, the
 * reset_counter _must_ be read before, and an appropriate smp_rmb must be
 * inserted.
 *
 * Returns 0 if the seqno was found within the alloted time. Else returns the
 * errno with remaining time filled in timeout argument.
 */
#ifdef __NetBSD__
static int
__wait_seqno(struct intel_ring_buffer *ring, u32 seqno, unsigned reset_counter,
    bool interruptible, struct timespec *timeout,
    struct drm_i915_file_private *file_priv)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool irq_test_in_progress;
	struct timespec before, after;
	int ticks;
	bool wedged;
	int ret;

	irq_test_in_progress = (dev_priv->gpu_error.test_irq_rings &
	    intel_ring_flag(ring));
	__insn_barrier();

	if (i915_seqno_passed(ring->get_seqno(ring, true), seqno))
		return 0;

	if (timeout)
		ticks = mstohz(timespec_to_ns(timeout) / 1000000);

	if (INTEL_INFO(dev)->gen >= 6 && can_wait_boost(file_priv)) {
		gen6_rps_boost(dev_priv);
		if (file_priv)
			mod_delayed_work(dev_priv->wq,
					 &file_priv->mm.idle_work,
					 msecs_to_jiffies(100));
	}

	if (!irq_test_in_progress && WARN_ON(!ring->irq_get(ring)))
		return -ENODEV;

	nanotime(&before);
	spin_lock(&dev_priv->irq_lock);
#define	EXIT_COND							      \
	(((reset_counter != atomic_read(&dev_priv->gpu_error.reset_counter))  \
	    ? wedged = true : false) ||					      \
	    i915_seqno_passed(ring->get_seqno(ring, false),		      \
		seqno))

	if (timeout) {
		/*
		 * XXX This missed_irq business smells like unlocked
		 * Linux waitqueue nonsense.
		 */
		if (missed_irq(dev_priv, ring))
			ticks = 1;
		if (interruptible)
			DRM_SPIN_TIMED_WAIT_UNTIL(ret, &ring->irq_queue,
			    &dev_priv->irq_lock, ticks, EXIT_COND);
		else
			DRM_SPIN_TIMED_WAIT_NOINTR_UNTIL(ret, &ring->irq_queue,
			    &dev_priv->irq_lock, ticks, EXIT_COND);
	} else {
		if (interruptible)
			DRM_SPIN_WAIT_UNTIL(ret, &ring->irq_queue,
			    &dev_priv->irq_lock, EXIT_COND);
		else
			DRM_SPIN_WAIT_NOINTR_UNTIL(ret, &ring->irq_queue,
			    &dev_priv->irq_lock, EXIT_COND);
	}
#undef	EXIT_COND
	spin_unlock(&dev_priv->irq_lock);
	nanotime(&after);

	if (!irq_test_in_progress)
		ring->irq_put(ring);
	if (timeout)
		timespecsub(&after, &before, timeout);
	return MAX(ret, 0);	/* ignore remaining ticks */
}
#else
static int __wait_seqno(struct intel_ring_buffer *ring, u32 seqno,
			unsigned reset_counter,
			bool interruptible,
			struct timespec *timeout,
			struct drm_i915_file_private *file_priv)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	const bool irq_test_in_progress =
		ACCESS_ONCE(dev_priv->gpu_error.test_irq_rings) & intel_ring_flag(ring);
	struct timespec before, now;
	DEFINE_WAIT(wait);
	unsigned long timeout_expire;
	int ret;

	WARN(dev_priv->pm.irqs_disabled, "IRQs disabled\n");

	if (i915_seqno_passed(ring->get_seqno(ring, true), seqno))
		return 0;

	timeout_expire = timeout ? jiffies + timespec_to_jiffies_timeout(timeout) : 0;

	if (INTEL_INFO(dev)->gen >= 6 && can_wait_boost(file_priv)) {
		gen6_rps_boost(dev_priv);
		if (file_priv)
			mod_delayed_work(dev_priv->wq,
					 &file_priv->mm.idle_work,
					 msecs_to_jiffies(100));
	}

	if (!irq_test_in_progress && WARN_ON(!ring->irq_get(ring)))
		return -ENODEV;

	/* Record current time in case interrupted by signal, or wedged */
	trace_i915_gem_request_wait_begin(ring, seqno);
	getrawmonotonic(&before);
	for (;;) {
		struct timer_list timer;

		prepare_to_wait(&ring->irq_queue, &wait,
				interruptible ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);

		/* We need to check whether any gpu reset happened in between
		 * the caller grabbing the seqno and now ... */
		if (reset_counter != atomic_read(&dev_priv->gpu_error.reset_counter)) {
			/* ... but upgrade the -EAGAIN to an -EIO if the gpu
			 * is truely gone. */
			ret = i915_gem_check_wedge(&dev_priv->gpu_error, interruptible);
			if (ret == 0)
				ret = -EAGAIN;
			break;
		}

		if (i915_seqno_passed(ring->get_seqno(ring, false), seqno)) {
			ret = 0;
			break;
		}

		if (interruptible && signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		if (timeout && time_after_eq(jiffies, timeout_expire)) {
			ret = -ETIME;
			break;
		}

		timer.function = NULL;
		if (timeout || missed_irq(dev_priv, ring)) {
			unsigned long expire;

			setup_timer_on_stack(&timer, fake_irq, (unsigned long)current);
			expire = missed_irq(dev_priv, ring) ? jiffies + 1 : timeout_expire;
			mod_timer(&timer, expire);
		}

		io_schedule();

		if (timer.function) {
			del_singleshot_timer_sync(&timer);
			destroy_timer_on_stack(&timer);
		}
	}
	getrawmonotonic(&now);
	trace_i915_gem_request_wait_end(ring, seqno);

	if (!irq_test_in_progress)
		ring->irq_put(ring);

	finish_wait(&ring->irq_queue, &wait);

	if (timeout) {
		struct timespec sleep_time = timespec_sub(now, before);
		*timeout = timespec_sub(*timeout, sleep_time);
		if (!timespec_valid(timeout)) /* i.e. negative time remains */
			set_normalized_timespec(timeout, 0, 0);
	}

	return ret;
}
#endif

/**
 * Waits for a sequence number to be signaled, and cleans up the
 * request and object lists appropriately for that event.
 */
int
i915_wait_seqno(struct intel_ring_buffer *ring, uint32_t seqno)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool interruptible = dev_priv->mm.interruptible;
	int ret;

	BUG_ON(!mutex_is_locked(&dev->struct_mutex));
	BUG_ON(seqno == 0);

	ret = i915_gem_check_wedge(&dev_priv->gpu_error, interruptible);
	if (ret)
		return ret;

	ret = i915_gem_check_olr(ring, seqno);
	if (ret)
		return ret;

	return __wait_seqno(ring, seqno,
			    atomic_read(&dev_priv->gpu_error.reset_counter),
			    interruptible, NULL, NULL);
}

static int
i915_gem_object_wait_rendering__tail(struct drm_i915_gem_object *obj,
				     struct intel_ring_buffer *ring)
{
	i915_gem_retire_requests_ring(ring);

	/* Manually manage the write flush as we may have not yet
	 * retired the buffer.
	 *
	 * Note that the last_write_seqno is always the earlier of
	 * the two (read/write) seqno, so if we haved successfully waited,
	 * we know we have passed the last write.
	 */
	obj->last_write_seqno = 0;
	obj->base.write_domain &= ~I915_GEM_GPU_DOMAINS;

	return 0;
}

/**
 * Ensures that all rendering to the object has completed and the object is
 * safe to unbind from the GTT or access from the CPU.
 */
static __must_check int
i915_gem_object_wait_rendering(struct drm_i915_gem_object *obj,
			       bool readonly)
{
	struct intel_ring_buffer *ring = obj->ring;
	u32 seqno;
	int ret;

	seqno = readonly ? obj->last_write_seqno : obj->last_read_seqno;
	if (seqno == 0)
		return 0;

	ret = i915_wait_seqno(ring, seqno);
	if (ret)
		return ret;

	return i915_gem_object_wait_rendering__tail(obj, ring);
}

/* A nonblocking variant of the above wait. This is a highly dangerous routine
 * as the object state may change during this call.
 */
static __must_check int
i915_gem_object_wait_rendering__nonblocking(struct drm_i915_gem_object *obj,
					    struct drm_i915_file_private *file_priv,
					    bool readonly)
{
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_ring_buffer *ring = obj->ring;
	unsigned reset_counter;
	u32 seqno;
	int ret;

	BUG_ON(!mutex_is_locked(&dev->struct_mutex));
	BUG_ON(!dev_priv->mm.interruptible);

	seqno = readonly ? obj->last_write_seqno : obj->last_read_seqno;
	if (seqno == 0)
		return 0;

	ret = i915_gem_check_wedge(&dev_priv->gpu_error, true);
	if (ret)
		return ret;

	ret = i915_gem_check_olr(ring, seqno);
	if (ret)
		return ret;

	reset_counter = atomic_read(&dev_priv->gpu_error.reset_counter);
	mutex_unlock(&dev->struct_mutex);
	ret = __wait_seqno(ring, seqno, reset_counter, true, NULL, file_priv);
	mutex_lock(&dev->struct_mutex);
	if (ret)
		return ret;

	return i915_gem_object_wait_rendering__tail(obj, ring);
}

/**
 * Called when user space prepares to use an object with the CPU, either
 * through the mmap ioctl's mapping or a GTT mapping.
 */
int
i915_gem_set_domain_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file)
{
	struct drm_i915_gem_set_domain *args = data;
	struct drm_i915_gem_object *obj;
	uint32_t read_domains = args->read_domains;
	uint32_t write_domain = args->write_domain;
	int ret;

	/* Only handle setting domains to types used by the CPU. */
	if (write_domain & I915_GEM_GPU_DOMAINS)
		return -EINVAL;

	if (read_domains & I915_GEM_GPU_DOMAINS)
		return -EINVAL;

	/* Having something in the write domain implies it's in the read
	 * domain, and only that read domain.  Enforce that in the request.
	 */
	if (write_domain != 0 && read_domains != write_domain)
		return -EINVAL;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Try to flush the object off the GPU without holding the lock.
	 * We will repeat the flush holding the lock in the normal manner
	 * to catch cases where we are gazumped.
	 */
	ret = i915_gem_object_wait_rendering__nonblocking(obj,
							  file->driver_priv,
							  !write_domain);
	if (ret)
		goto unref;

	if (read_domains & I915_GEM_DOMAIN_GTT) {
		ret = i915_gem_object_set_to_gtt_domain(obj, write_domain != 0);

		/* Silently promote "you're not bound, there was nothing to do"
		 * to success, since the client was just asking us to
		 * make sure everything was done.
		 */
		if (ret == -EINVAL)
			ret = 0;
	} else {
		ret = i915_gem_object_set_to_cpu_domain(obj, write_domain != 0);
	}

unref:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

/**
 * Called when user space has done writes to this buffer
 */
int
i915_gem_sw_finish_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file)
{
	struct drm_i915_gem_sw_finish *args = data;
	struct drm_i915_gem_object *obj;
	int ret = 0;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Pinned buffers may be scanout, so flush the cache */
	if (obj->pin_display)
		i915_gem_object_flush_cpu_write_domain(obj, true);

	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

/**
 * Maps the contents of an object, returning the address it is mapped
 * into.
 *
 * While the mapping holds a reference on the contents of the object, it doesn't
 * imply a ref on the object itself.
 */
int
i915_gem_mmap_ioctl(struct drm_device *dev, void *data,
		    struct drm_file *file)
{
	struct drm_i915_gem_mmap *args = data;
	struct drm_gem_object *obj;
	unsigned long addr;
#ifdef __NetBSD__
	int ret;
#endif

	obj = drm_gem_object_lookup(dev, file, args->handle);
	if (obj == NULL)
		return -ENOENT;

	/* prime objects have no backing filp to GEM mmap
	 * pages from.
	 */
#ifdef __NetBSD__
	/* Also stolen objects (XXX can we get them here?)  */
	if (obj->gemo_shm_uao == NULL) {
		drm_gem_object_unreference_unlocked(obj);
		return -EINVAL;
	}
#else
	if (!obj->filp) {
		drm_gem_object_unreference_unlocked(obj);
		return -EINVAL;
	}
#endif

#ifdef __NetBSD__
	addr = (*curproc->p_emul->e_vm_default_addr)(curproc,
	    (vaddr_t)curproc->p_vmspace->vm_daddr, args->size);
	/* XXX errno NetBSD->Linux */
	ret = -uvm_map(&curproc->p_vmspace->vm_map, &addr, args->size,
	    obj->gemo_shm_uao, args->offset, 0,
	    UVM_MAPFLAG((VM_PROT_READ | VM_PROT_WRITE),
		(VM_PROT_READ | VM_PROT_WRITE), UVM_INH_COPY, UVM_ADV_NORMAL,
		0));
	if (ret) {
		drm_gem_object_unreference_unlocked(obj);
		return ret;
	}
	uao_reference(obj->gemo_shm_uao);
	drm_gem_object_unreference_unlocked(obj);
#else
	addr = vm_mmap(obj->filp, 0, args->size,
		       PROT_READ | PROT_WRITE, MAP_SHARED,
		       args->offset);
	drm_gem_object_unreference_unlocked(obj);
	if (IS_ERR((void *)addr))
		return addr;
#endif

	args->addr_ptr = (uint64_t) addr;

	return 0;
}

#ifdef __NetBSD__		/* XXX gem gtt fault */
static int	i915_udv_fault(struct uvm_faultinfo *, vaddr_t,
		    struct vm_page **, int, int, vm_prot_t, int, paddr_t);

int
i915_gem_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr, struct vm_page **pps,
    int npages, int centeridx, vm_prot_t access_type, int flags)
{
	struct uvm_object *uobj = ufi->entry->object.uvm_obj;
	struct drm_gem_object *gem_obj =
	    container_of(uobj, struct drm_gem_object, gemo_uvmobj);
	struct drm_i915_gem_object *obj = to_intel_bo(gem_obj);
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	voff_t byte_offset;
	pgoff_t page_offset;
	int ret = 0;
	bool write = ISSET(access_type, VM_PROT_WRITE)? 1 : 0;

	byte_offset = (ufi->entry->offset + (vaddr - ufi->entry->start));
	KASSERT(byte_offset <= obj->base.size);
	page_offset = (byte_offset >> PAGE_SHIFT);

	intel_runtime_pm_get(dev_priv);

	/* Thanks, uvm, but we don't need this lock.  */
	mutex_exit(uobj->vmobjlock);

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		goto out;

	trace_i915_gem_object_fault(obj, page_offset, true, write);

	ret = i915_gem_object_wait_rendering__nonblocking(obj, NULL, !write);
	if (ret)
		goto unlock;

	if ((obj->cache_level != I915_CACHE_NONE) && !HAS_LLC(dev)) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = i915_gem_obj_ggtt_pin(obj, 0, PIN_MAPPABLE);
	if (ret)
		goto unlock;

	ret = i915_gem_object_set_to_gtt_domain(obj, write);
	if (ret)
		goto unpin;

	ret = i915_gem_object_get_fence(obj);
	if (ret)
		goto unpin;

	obj->fault_mappable = true;

	/* XXX errno NetBSD->Linux */
	ret = -i915_udv_fault(ufi, vaddr, pps, npages, centeridx, access_type,
	    flags,
	    (dev_priv->gtt.mappable_base + i915_gem_obj_ggtt_offset(obj)));
unpin:
	i915_gem_object_ggtt_unpin(obj);
unlock:
	mutex_unlock(&dev->struct_mutex);
out:
	mutex_enter(uobj->vmobjlock);
	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, uobj);
	if (ret == -ERESTART)
		uvm_wait("i915flt");
	/* XXX Deal with GPU hangs here...  */
	intel_runtime_pm_put(dev_priv);
	/* XXX errno Linux->NetBSD */
	return -ret;
}

/*
 * XXX i915_udv_fault is copypasta of udv_fault from uvm_device.c.
 *
 * XXX pmap_enter_default instead of pmap_enter because of a problem
 * with using weak aliases in kernel modules or something.
 */
int	pmap_enter_default(pmap_t, vaddr_t, paddr_t, vm_prot_t, unsigned);

static int
i915_udv_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr, struct vm_page **pps,
    int npages, int centeridx, vm_prot_t access_type, int flags,
    paddr_t gtt_paddr)
{
	struct vm_map_entry *entry = ufi->entry;
	vaddr_t curr_va;
	off_t curr_offset;
	paddr_t paddr;
	u_int mmapflags;
	int lcv, retval;
	vm_prot_t mapprot;
	UVMHIST_FUNC("i915_udv_fault"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist,"  flags=%d", flags,0,0,0);

	/*
	 * we do not allow device mappings to be mapped copy-on-write
	 * so we kill any attempt to do so here.
	 */

	if (UVM_ET_ISCOPYONWRITE(entry)) {
		UVMHIST_LOG(maphist, "<- failed -- COW entry (etype=0x%x)",
		entry->etype, 0,0,0);
		return(EIO);
	}

	/*
	 * now we must determine the offset in udv to use and the VA to
	 * use for pmap_enter.  note that we always use orig_map's pmap
	 * for pmap_enter (even if we have a submap).   since virtual
	 * addresses in a submap must match the main map, this is ok.
	 */

	/* udv offset = (offset from start of entry) + entry's offset */
	curr_offset = entry->offset + (vaddr - entry->start);
	/* pmap va = vaddr (virtual address of pps[0]) */
	curr_va = vaddr;

	/*
	 * loop over the page range entering in as needed
	 */

	retval = 0;
	for (lcv = 0 ; lcv < npages ; lcv++, curr_offset += PAGE_SIZE,
	    curr_va += PAGE_SIZE) {
		if ((flags & PGO_ALLPAGES) == 0 && lcv != centeridx)
			continue;

		if (pps[lcv] == PGO_DONTCARE)
			continue;

		paddr = (gtt_paddr + curr_offset);
		mmapflags = 0;
		mapprot = ufi->entry->protection;
		UVMHIST_LOG(maphist,
		    "  MAPPING: device: pm=0x%x, va=0x%x, pa=0x%lx, at=%d",
		    ufi->orig_map->pmap, curr_va, paddr, mapprot);
		if (pmap_enter_default(ufi->orig_map->pmap, curr_va, paddr, mapprot,
		    PMAP_CANFAIL | mapprot | mmapflags) != 0) {
			/*
			 * pmap_enter() didn't have the resource to
			 * enter this mapping.  Unlock everything,
			 * wait for the pagedaemon to free up some
			 * pages, and then tell uvm_fault() to start
			 * the fault again.
			 *
			 * XXX Needs some rethinking for the PGO_ALLPAGES
			 * XXX case.
			 */
			pmap_update(ufi->orig_map->pmap);	/* sync what we have so far */
			return (ERESTART);
		}
	}

	pmap_update(ufi->orig_map->pmap);
	return (retval);
}
#else
/**
 * i915_gem_fault - fault a page into the GTT
 * vma: VMA in question
 * vmf: fault info
 *
 * The fault handler is set up by drm_gem_mmap() when a object is GTT mapped
 * from userspace.  The fault handler takes care of binding the object to
 * the GTT (if needed), allocating and programming a fence register (again,
 * only if needed based on whether the old reg is still valid or the object
 * is tiled) and inserting a new PTE into the faulting process.
 *
 * Note that the faulting process may involve evicting existing objects
 * from the GTT and/or fence registers to make room.  So performance may
 * suffer if the GTT working set is large or there are few fence registers
 * left.
 */
int i915_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_i915_gem_object *obj = to_intel_bo(vma->vm_private_data);
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	pgoff_t page_offset;
	unsigned long pfn;
	int ret = 0;
	bool write = !!(vmf->flags & FAULT_FLAG_WRITE);

	intel_runtime_pm_get(dev_priv);

	/* We don't use vmf->pgoff since that has the fake offset */
	page_offset = ((unsigned long)vmf->virtual_address - vma->vm_start) >>
		PAGE_SHIFT;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		goto out;

	trace_i915_gem_object_fault(obj, page_offset, true, write);

	/* Try to flush the object off the GPU first without holding the lock.
	 * Upon reacquiring the lock, we will perform our sanity checks and then
	 * repeat the flush holding the lock in the normal manner to catch cases
	 * where we are gazumped.
	 */
	ret = i915_gem_object_wait_rendering__nonblocking(obj, NULL, !write);
	if (ret)
		goto unlock;

	/* Access to snoopable pages through the GTT is incoherent. */
	if (obj->cache_level != I915_CACHE_NONE && !HAS_LLC(dev)) {
		ret = -EINVAL;
		goto unlock;
	}

	/* Now bind it into the GTT if needed */
	ret = i915_gem_obj_ggtt_pin(obj, 0, PIN_MAPPABLE);
	if (ret)
		goto unlock;

	ret = i915_gem_object_set_to_gtt_domain(obj, write);
	if (ret)
		goto unpin;

	ret = i915_gem_object_get_fence(obj);
	if (ret)
		goto unpin;

	obj->fault_mappable = true;

	pfn = dev_priv->gtt.mappable_base + i915_gem_obj_ggtt_offset(obj);
	pfn >>= PAGE_SHIFT;
	pfn += page_offset;

	/* Finally, remap it using the new GTT offset */
	ret = vm_insert_pfn(vma, (unsigned long)vmf->virtual_address, pfn);
unpin:
	i915_gem_object_ggtt_unpin(obj);
unlock:
	mutex_unlock(&dev->struct_mutex);
out:
	switch (ret) {
	case -EIO:
		/* If this -EIO is due to a gpu hang, give the reset code a
		 * chance to clean up the mess. Otherwise return the proper
		 * SIGBUS. */
		if (i915_terminally_wedged(&dev_priv->gpu_error)) {
			ret = VM_FAULT_SIGBUS;
			break;
		}
	case -EAGAIN:
		/*
		 * EAGAIN means the gpu is hung and we'll wait for the error
		 * handler to reset everything when re-faulting in
		 * i915_mutex_lock_interruptible.
		 */
	case 0:
	case -ERESTARTSYS:
	case -EINTR:
	case -EBUSY:
		/*
		 * EBUSY is ok: this just means that another thread
		 * already did the job.
		 */
		ret = VM_FAULT_NOPAGE;
		break;
	case -ENOMEM:
		ret = VM_FAULT_OOM;
		break;
	case -ENOSPC:
	case -EFAULT:
		ret = VM_FAULT_SIGBUS;
		break;
	default:
		WARN_ONCE(ret, "unhandled error in i915_gem_fault: %i\n", ret);
		ret = VM_FAULT_SIGBUS;
		break;
	}

	intel_runtime_pm_put(dev_priv);
	return ret;
}

void i915_gem_release_all_mmaps(struct drm_i915_private *dev_priv)
{
	struct i915_vma *vma;

	/*
	 * Only the global gtt is relevant for gtt memory mappings, so restrict
	 * list traversal to objects bound into the global address space. Note
	 * that the active list should be empty, but better safe than sorry.
	 */
	WARN_ON(!list_empty(&dev_priv->gtt.base.active_list));
	list_for_each_entry(vma, &dev_priv->gtt.base.active_list, mm_list)
		i915_gem_release_mmap(vma->obj);
	list_for_each_entry(vma, &dev_priv->gtt.base.inactive_list, mm_list)
		i915_gem_release_mmap(vma->obj);
}
#endif

/**
 * i915_gem_release_mmap - remove physical page mappings
 * @obj: obj in question
 *
 * Preserve the reservation of the mmapping with the DRM core code, but
 * relinquish ownership of the pages back to the system.
 *
 * It is vital that we remove the page mapping if we have mapped a tiled
 * object through the GTT and then lose the fence register due to
 * resource pressure. Similarly if the object has been moved out of the
 * aperture, than pages mapped into userspace must be revoked. Removing the
 * mapping will then trigger a page fault on the next user access, allowing
 * fixup by i915_gem_fault().
 */
void
i915_gem_release_mmap(struct drm_i915_gem_object *obj)
{
	if (!obj->fault_mappable)
		return;

#ifdef __NetBSD__		/* XXX gem gtt fault */
	{
		struct vm_page *page;

		mutex_enter(obj->base.gemo_shm_uao->vmobjlock);
		KASSERT(obj->pages != NULL);
		/* Force a fresh fault for each page.  */
		/*
		 * XXX OOPS!  This doesn't actually do what we want.
		 * This causes a fresh fault for access to the backing
		 * pages -- but nothing accesses the backing pages
		 * directly!  What is actually entered into CPU page
		 * table entries is aperture addresses which have been
		 * programmed by the GTT to refer to those backing
		 * pages.
		 *
		 * We need to clear those page table entries, but
		 * there's no good way to do that at the moment: nobody
		 * records for us a map from either uvm objects or
		 * physical device addresses to a list of all virtual
		 * pages where they have been mapped.  pmap(9) records
		 * a map only from physical RAM addresses to virtual
		 * pages; it does nothing for physical device
		 * addresses.
		 */
		TAILQ_FOREACH(page, &obj->igo_pageq, pageq.queue)
			pmap_page_protect(page, VM_PROT_NONE);
		mutex_exit(obj->base.gemo_shm_uao->vmobjlock);
	}
#else
	drm_vma_node_unmap(&obj->base.vma_node,
			   obj->base.dev->anon_inode->i_mapping);
#endif
	obj->fault_mappable = false;
}

uint32_t
i915_gem_get_gtt_size(struct drm_device *dev, uint32_t size, int tiling_mode)
{
	uint32_t gtt_size;

	if (INTEL_INFO(dev)->gen >= 4 ||
	    tiling_mode == I915_TILING_NONE)
		return size;

	/* Previous chips need a power-of-two fence region when tiling */
	if (INTEL_INFO(dev)->gen == 3)
		gtt_size = 1024*1024;
	else
		gtt_size = 512*1024;

	while (gtt_size < size)
		gtt_size <<= 1;

	return gtt_size;
}

/**
 * i915_gem_get_gtt_alignment - return required GTT alignment for an object
 * @obj: object to check
 *
 * Return the required GTT alignment for an object, taking into account
 * potential fence register mapping.
 */
uint32_t
i915_gem_get_gtt_alignment(struct drm_device *dev, uint32_t size,
			   int tiling_mode, bool fenced)
{
	/*
	 * Minimum alignment is 4k (GTT page size), but might be greater
	 * if a fence register is needed for the object.
	 */
	if (INTEL_INFO(dev)->gen >= 4 || (!fenced && IS_G33(dev)) ||
	    tiling_mode == I915_TILING_NONE)
		return 4096;

	/*
	 * Previous chips need to be aligned to the size of the smallest
	 * fence register that can contain the object.
	 */
	return i915_gem_get_gtt_size(dev, size, tiling_mode);
}

static int i915_gem_object_create_mmap_offset(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	int ret;

	if (drm_vma_node_has_offset(&obj->base.vma_node))
		return 0;

	dev_priv->mm.shrinker_no_lock_stealing = true;

	ret = drm_gem_create_mmap_offset(&obj->base);
	if (ret != -ENOSPC)
		goto out;

	/* Badly fragmented mmap space? The only way we can recover
	 * space is by destroying unwanted objects. We can't randomly release
	 * mmap_offsets as userspace expects them to be persistent for the
	 * lifetime of the objects. The closest we can is to release the
	 * offsets on purgeable objects by truncating it and marking it purged,
	 * which prevents userspace from ever using that object again.
	 */
	i915_gem_purge(dev_priv, obj->base.size >> PAGE_SHIFT);
	ret = drm_gem_create_mmap_offset(&obj->base);
	if (ret != -ENOSPC)
		goto out;

	i915_gem_shrink_all(dev_priv);
	ret = drm_gem_create_mmap_offset(&obj->base);
out:
	dev_priv->mm.shrinker_no_lock_stealing = false;

	return ret;
}

static void i915_gem_object_free_mmap_offset(struct drm_i915_gem_object *obj)
{
	drm_gem_free_mmap_offset(&obj->base);
}

int
i915_gem_mmap_gtt(struct drm_file *file,
		  struct drm_device *dev,
		  uint32_t handle,
		  uint64_t *offset)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;
	int ret;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	if (obj->base.size > dev_priv->gtt.mappable_end) {
		ret = -E2BIG;
		goto out;
	}

	if (obj->madv != I915_MADV_WILLNEED) {
		DRM_DEBUG("Attempting to mmap a purgeable buffer\n");
		ret = -EFAULT;
		goto out;
	}

	ret = i915_gem_object_create_mmap_offset(obj);
	if (ret)
		goto out;

	*offset = drm_vma_node_offset_addr(&obj->base.vma_node);

out:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

/**
 * i915_gem_mmap_gtt_ioctl - prepare an object for GTT mmap'ing
 * @dev: DRM device
 * @data: GTT mapping ioctl data
 * @file: GEM object info
 *
 * Simply returns the fake offset to userspace so it can mmap it.
 * The mmap call will end up in drm_gem_mmap(), which will set things
 * up so we can get faults in the handler above.
 *
 * The fault handler will take care of binding the object into the GTT
 * (since it may have been evicted to make room for something), allocating
 * a fence register, and mapping the appropriate aperture address into
 * userspace.
 */
int
i915_gem_mmap_gtt_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file)
{
	struct drm_i915_gem_mmap_gtt *args = data;

	return i915_gem_mmap_gtt(file, dev, args->handle, &args->offset);
}

/* Immediately discard the backing storage */
static void
i915_gem_object_truncate(struct drm_i915_gem_object *obj)
{
#ifndef __NetBSD__
	struct inode *inode;
#endif

	i915_gem_object_free_mmap_offset(obj);

#ifdef __NetBSD__
	if (obj->base.gemo_shm_uao == NULL)
		return;

	{
		struct uvm_object *const uobj = obj->base.gemo_shm_uao;

		if (uobj != NULL) {
			/* XXX Calling pgo_put like this is bogus.  */
			mutex_enter(uobj->vmobjlock);
			(*uobj->pgops->pgo_put)(uobj, 0, obj->base.size,
			    (PGO_ALLPAGES | PGO_FREE));
		}
	}
#else
	if (obj->base.filp == NULL)
		return;

	/* Our goal here is to return as much of the memory as
	 * is possible back to the system as we are called from OOM.
	 * To do this we must instruct the shmfs to drop all of its
	 * backing pages, *now*.
	 */
	inode = file_inode(obj->base.filp);
	shmem_truncate_range(inode, 0, (loff_t)-1);
#endif

	obj->madv = __I915_MADV_PURGED;
}

static inline int
i915_gem_object_is_purgeable(struct drm_i915_gem_object *obj)
{
	return obj->madv == I915_MADV_DONTNEED;
}

#ifdef __NetBSD__
static void
i915_gem_object_put_pages_gtt(struct drm_i915_gem_object *obj)
{
	struct drm_device *const dev = obj->base.dev;
	int ret;

	/* XXX Cargo-culted from the Linux code.  */
	BUG_ON(obj->madv == __I915_MADV_PURGED);

	ret = i915_gem_object_set_to_cpu_domain(obj, true);
	if (ret) {
		WARN_ON(ret != -EIO);
		i915_gem_clflush_object(obj, true);
		obj->base.read_domains = obj->base.write_domain =
		    I915_GEM_DOMAIN_CPU;
	}

	if (i915_gem_object_needs_bit17_swizzle(obj))
		i915_gem_object_save_bit_17_swizzle(obj);

	/* XXX Maintain dirty flag?  */

	bus_dmamap_destroy(dev->dmat, obj->igo_dmamap);
	bus_dmamem_unwire_uvm_object(dev->dmat, obj->base.gemo_shm_uao, 0,
	    obj->base.size, obj->pages, obj->igo_nsegs);

	kfree(obj->pages);
}
#else
static void
i915_gem_object_put_pages_gtt(struct drm_i915_gem_object *obj)
{
	struct sg_page_iter sg_iter;
	int ret;

	BUG_ON(obj->madv == __I915_MADV_PURGED);

	ret = i915_gem_object_set_to_cpu_domain(obj, true);
	if (ret) {
		/* In the event of a disaster, abandon all caches and
		 * hope for the best.
		 */
		WARN_ON(ret != -EIO);
		i915_gem_clflush_object(obj, true);
		obj->base.read_domains = obj->base.write_domain = I915_GEM_DOMAIN_CPU;
	}

	if (i915_gem_object_needs_bit17_swizzle(obj))
		i915_gem_object_save_bit_17_swizzle(obj);

	if (obj->madv == I915_MADV_DONTNEED)
		obj->dirty = 0;

	for_each_sg_page(obj->pages->sgl, &sg_iter, obj->pages->nents, 0) {
		struct page *page = sg_page_iter_page(&sg_iter);

		if (obj->dirty)
			set_page_dirty(page);

		if (obj->madv == I915_MADV_WILLNEED)
			mark_page_accessed(page);

		page_cache_release(page);
	}
	obj->dirty = 0;

	sg_free_table(obj->pages);
	kfree(obj->pages);
}
#endif

int
i915_gem_object_put_pages(struct drm_i915_gem_object *obj)
{
	const struct drm_i915_gem_object_ops *ops = obj->ops;

	if (obj->pages == NULL)
		return 0;

	if (obj->pages_pin_count)
		return -EBUSY;

	BUG_ON(i915_gem_obj_bound_any(obj));

	/* ->put_pages might need to allocate memory for the bit17 swizzle
	 * array, hence protect them from being reaped by removing them from gtt
	 * lists early. */
	list_del(&obj->global_list);

	ops->put_pages(obj);
	obj->pages = NULL;

	if (i915_gem_object_is_purgeable(obj))
		i915_gem_object_truncate(obj);

	return 0;
}

static unsigned long
__i915_gem_shrink(struct drm_i915_private *dev_priv, long target,
		  bool purgeable_only)
{
	struct list_head still_bound_list;
	struct drm_i915_gem_object *obj, *next;
	unsigned long count = 0;

	list_for_each_entry_safe(obj, next,
				 &dev_priv->mm.unbound_list,
				 global_list) {
		if ((i915_gem_object_is_purgeable(obj) || !purgeable_only) &&
		    i915_gem_object_put_pages(obj) == 0) {
			count += obj->base.size >> PAGE_SHIFT;
			if (count >= target)
				return count;
		}
	}

	/*
	 * As we may completely rewrite the bound list whilst unbinding
	 * (due to retiring requests) we have to strictly process only
	 * one element of the list at the time, and recheck the list
	 * on every iteration.
	 */
	INIT_LIST_HEAD(&still_bound_list);
	while (count < target && !list_empty(&dev_priv->mm.bound_list)) {
		struct i915_vma *vma, *v;

		obj = list_first_entry(&dev_priv->mm.bound_list,
				       typeof(*obj), global_list);
		list_move_tail(&obj->global_list, &still_bound_list);

		if (!i915_gem_object_is_purgeable(obj) && purgeable_only)
			continue;

		/*
		 * Hold a reference whilst we unbind this object, as we may
		 * end up waiting for and retiring requests. This might
		 * release the final reference (held by the active list)
		 * and result in the object being freed from under us.
		 * in this object being freed.
		 *
		 * Note 1: Shrinking the bound list is special since only active
		 * (and hence bound objects) can contain such limbo objects, so
		 * we don't need special tricks for shrinking the unbound list.
		 * The only other place where we have to be careful with active
		 * objects suddenly disappearing due to retiring requests is the
		 * eviction code.
		 *
		 * Note 2: Even though the bound list doesn't hold a reference
		 * to the object we can safely grab one here: The final object
		 * unreferencing and the bound_list are both protected by the
		 * dev->struct_mutex and so we won't ever be able to observe an
		 * object on the bound_list with a reference count equals 0.
		 */
		drm_gem_object_reference(&obj->base);

		list_for_each_entry_safe(vma, v, &obj->vma_list, vma_link)
			if (i915_vma_unbind(vma))
				break;

		if (i915_gem_object_put_pages(obj) == 0)
			count += obj->base.size >> PAGE_SHIFT;

		drm_gem_object_unreference(&obj->base);
	}
	list_splice(&still_bound_list, &dev_priv->mm.bound_list);

	return count;
}

static unsigned long
i915_gem_purge(struct drm_i915_private *dev_priv, long target)
{
	return __i915_gem_shrink(dev_priv, target, true);
}

static unsigned long
i915_gem_shrink_all(struct drm_i915_private *dev_priv)
{
	struct drm_i915_gem_object *obj, *next;
	long freed = 0;

	i915_gem_evict_everything(dev_priv->dev);

	list_for_each_entry_safe(obj, next, &dev_priv->mm.unbound_list,
				 global_list) {
		if (i915_gem_object_put_pages(obj) == 0)
			freed += obj->base.size >> PAGE_SHIFT;
	}
	return freed;
}

#ifdef __NetBSD__
static int
i915_gem_object_get_pages_gtt(struct drm_i915_gem_object *obj)
{
	struct drm_device *const dev = obj->base.dev;
	struct vm_page *page;
	int error;

	/* XXX Cargo-culted from the Linux code.  */
	BUG_ON(obj->base.read_domains & I915_GEM_GPU_DOMAINS);
	BUG_ON(obj->base.write_domain & I915_GEM_GPU_DOMAINS);

	KASSERT(obj->pages == NULL);
	TAILQ_INIT(&obj->igo_pageq);
	obj->pages = kcalloc((obj->base.size / PAGE_SIZE),
	    sizeof(obj->pages[0]), GFP_KERNEL);
	if (obj->pages == NULL) {
		error = -ENOMEM;
		goto fail0;
	}

	/* XXX errno NetBSD->Linux */
	error = -bus_dmamem_wire_uvm_object(dev->dmat, obj->base.gemo_shm_uao,
	    0, obj->base.size, &obj->igo_pageq, PAGE_SIZE, 0, obj->pages,
	    (obj->base.size / PAGE_SIZE), &obj->igo_nsegs, BUS_DMA_NOWAIT);
	if (error)
		/* XXX Try i915_gem_purge, i915_gem_shrink_all.  */
		goto fail1;
	KASSERT(0 < obj->igo_nsegs);
	KASSERT(obj->igo_nsegs <= (obj->base.size / PAGE_SIZE));

	/*
	 * Check that the paddrs will fit in 40 bits, or 32 bits on i965.
	 *
	 * XXX This is wrong; we ought to pass this constraint to
	 * bus_dmamem_wire_uvm_object instead.
	 */
	TAILQ_FOREACH(page, &obj->igo_pageq, pageq.queue) {
		const uint64_t mask =
		    (IS_BROADWATER(dev) || IS_CRESTLINE(dev)?
			0xffffffffULL : 0xffffffffffULL);
		if (VM_PAGE_TO_PHYS(page) & ~mask) {
			DRM_ERROR("GEM physical address exceeds %u bits"
			    ": %"PRIxMAX"\n",
			    popcount64(mask),
			    (uintmax_t)VM_PAGE_TO_PHYS(page));
			error = -EIO;
			goto fail2;
		}
	}

	/* XXX Should create the DMA map when creating the object.  */

	/* XXX errno NetBSD->Linux */
	error = -bus_dmamap_create(dev->dmat, obj->base.size, obj->igo_nsegs,
	    PAGE_SIZE, 0, BUS_DMA_NOWAIT, &obj->igo_dmamap);
	if (error)
		goto fail2;

	/* XXX Cargo-culted from the Linux code.  */
	if (i915_gem_object_needs_bit17_swizzle(obj))
		i915_gem_object_do_bit_17_swizzle(obj);

	/* Success!  */
	return 0;

fail2:	bus_dmamem_unwire_uvm_object(dev->dmat, obj->base.gemo_shm_uao, 0,
	    obj->base.size, obj->pages, (obj->base.size / PAGE_SIZE));
fail1:	kfree(obj->pages);
	obj->pages = NULL;
fail0:	KASSERT(error);
	return error;
}
#else
static int
i915_gem_object_get_pages_gtt(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	int page_count, i;
	struct address_space *mapping;
	struct sg_table *st;
	struct scatterlist *sg;
	struct sg_page_iter sg_iter;
	struct page *page;
	unsigned long last_pfn = 0;	/* suppress gcc warning */
	gfp_t gfp;

	/* Assert that the object is not currently in any GPU domain. As it
	 * wasn't in the GTT, there shouldn't be any way it could have been in
	 * a GPU cache
	 */
	BUG_ON(obj->base.read_domains & I915_GEM_GPU_DOMAINS);
	BUG_ON(obj->base.write_domain & I915_GEM_GPU_DOMAINS);

	st = kmalloc(sizeof(*st), GFP_KERNEL);
	if (st == NULL)
		return -ENOMEM;

	page_count = obj->base.size / PAGE_SIZE;
	if (sg_alloc_table(st, page_count, GFP_KERNEL)) {
		kfree(st);
		return -ENOMEM;
	}

	/* Get the list of pages out of our struct file.  They'll be pinned
	 * at this point until we release them.
	 *
	 * Fail silently without starting the shrinker
	 */
	mapping = file_inode(obj->base.filp)->i_mapping;
	gfp = mapping_gfp_mask(mapping);
	gfp |= __GFP_NORETRY | __GFP_NOWARN | __GFP_NO_KSWAPD;
	gfp &= ~(__GFP_IO | __GFP_WAIT);
	sg = st->sgl;
	st->nents = 0;
	for (i = 0; i < page_count; i++) {
		page = shmem_read_mapping_page_gfp(mapping, i, gfp);
		if (IS_ERR(page)) {
			i915_gem_purge(dev_priv, page_count);
			page = shmem_read_mapping_page_gfp(mapping, i, gfp);
		}
		if (IS_ERR(page)) {
			/* We've tried hard to allocate the memory by reaping
			 * our own buffer, now let the real VM do its job and
			 * go down in flames if truly OOM.
			 */
			gfp &= ~(__GFP_NORETRY | __GFP_NOWARN | __GFP_NO_KSWAPD);
			gfp |= __GFP_IO | __GFP_WAIT;

			i915_gem_shrink_all(dev_priv);
			page = shmem_read_mapping_page_gfp(mapping, i, gfp);
			if (IS_ERR(page))
				goto err_pages;

			gfp |= __GFP_NORETRY | __GFP_NOWARN | __GFP_NO_KSWAPD;
			gfp &= ~(__GFP_IO | __GFP_WAIT);
		}
#ifdef CONFIG_SWIOTLB
		if (swiotlb_nr_tbl()) {
			st->nents++;
			sg_set_page(sg, page, PAGE_SIZE, 0);
			sg = sg_next(sg);
			continue;
		}
#endif
		if (!i || page_to_pfn(page) != last_pfn + 1) {
			if (i)
				sg = sg_next(sg);
			st->nents++;
			sg_set_page(sg, page, PAGE_SIZE, 0);
		} else {
			sg->length += PAGE_SIZE;
		}
		last_pfn = page_to_pfn(page);

		/* Check that the i965g/gm workaround works. */
		WARN_ON((gfp & __GFP_DMA32) && (last_pfn >= 0x00100000UL));
	}
#ifdef CONFIG_SWIOTLB
	if (!swiotlb_nr_tbl())
#endif
		sg_mark_end(sg);
	obj->pages = st;

	if (i915_gem_object_needs_bit17_swizzle(obj))
		i915_gem_object_do_bit_17_swizzle(obj);

	return 0;

err_pages:
	sg_mark_end(sg);
	for_each_sg_page(st->sgl, &sg_iter, st->nents, 0)
		page_cache_release(sg_page_iter_page(&sg_iter));
	sg_free_table(st);
	kfree(st);
	return PTR_ERR(page);
}
#endif

/* Ensure that the associated pages are gathered from the backing storage
 * and pinned into our object. i915_gem_object_get_pages() may be called
 * multiple times before they are released by a single call to
 * i915_gem_object_put_pages() - once the pages are no longer referenced
 * either as a result of memory pressure (reaping pages under the shrinker)
 * or as the object is itself released.
 */
int
i915_gem_object_get_pages(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	const struct drm_i915_gem_object_ops *ops = obj->ops;
	int ret;

	if (obj->pages)
		return 0;

	if (obj->madv != I915_MADV_WILLNEED) {
		DRM_DEBUG("Attempting to obtain a purgeable object\n");
		return -EFAULT;
	}

	BUG_ON(obj->pages_pin_count);

	ret = ops->get_pages(obj);
	if (ret)
		return ret;

	list_add_tail(&obj->global_list, &dev_priv->mm.unbound_list);
	return 0;
}

static void
i915_gem_object_move_to_active(struct drm_i915_gem_object *obj,
			       struct intel_ring_buffer *ring)
{
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 seqno = intel_ring_get_seqno(ring);

	BUG_ON(ring == NULL);
	if (obj->ring != ring && obj->last_write_seqno) {
		/* Keep the seqno relative to the current ring */
		obj->last_write_seqno = seqno;
	}
	obj->ring = ring;

	/* Add a reference if we're newly entering the active list. */
	if (!obj->active) {
		drm_gem_object_reference(&obj->base);
		obj->active = 1;
	}

	list_move_tail(&obj->ring_list, &ring->active_list);

	obj->last_read_seqno = seqno;

	if (obj->fenced_gpu_access) {
		obj->last_fenced_seqno = seqno;

		/* Bump MRU to take account of the delayed flush */
		if (obj->fence_reg != I915_FENCE_REG_NONE) {
			struct drm_i915_fence_reg *reg;

			reg = &dev_priv->fence_regs[obj->fence_reg];
			list_move_tail(&reg->lru_list,
				       &dev_priv->mm.fence_list);
		}
	}
}

void i915_vma_move_to_active(struct i915_vma *vma,
			     struct intel_ring_buffer *ring)
{
	list_move_tail(&vma->mm_list, &vma->vm->active_list);
	return i915_gem_object_move_to_active(vma->obj, ring);
}

static void
i915_gem_object_move_to_inactive(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	struct i915_address_space *vm;
	struct i915_vma *vma;

	BUG_ON(obj->base.write_domain & ~I915_GEM_GPU_DOMAINS);
	BUG_ON(!obj->active);

	list_for_each_entry(vm, &dev_priv->vm_list, global_link) {
		vma = i915_gem_obj_to_vma(obj, vm);
		if (vma && !list_empty(&vma->mm_list))
			list_move_tail(&vma->mm_list, &vm->inactive_list);
	}

	list_del_init(&obj->ring_list);
	obj->ring = NULL;

	obj->last_read_seqno = 0;
	obj->last_write_seqno = 0;
	obj->base.write_domain = 0;

	obj->last_fenced_seqno = 0;
	obj->fenced_gpu_access = false;

	obj->active = 0;
	drm_gem_object_unreference(&obj->base);

	WARN_ON(i915_verify_lists(dev));
}

static int
i915_gem_init_seqno(struct drm_device *dev, u32 seqno)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_ring_buffer *ring;
	int ret, i, j;

	/* Carefully retire all requests without writing to the rings */
	for_each_ring(ring, dev_priv, i) {
		ret = intel_ring_idle(ring);
		if (ret)
			return ret;
	}
	i915_gem_retire_requests(dev);

	/* Finally reset hw state */
	for_each_ring(ring, dev_priv, i) {
		intel_ring_init_seqno(ring, seqno);

		for (j = 0; j < ARRAY_SIZE(ring->sync_seqno); j++)
			ring->sync_seqno[j] = 0;
	}

	return 0;
}

int i915_gem_set_seqno(struct drm_device *dev, u32 seqno)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	if (seqno == 0)
		return -EINVAL;

	/* HWS page needs to be set less than what we
	 * will inject to ring
	 */
	ret = i915_gem_init_seqno(dev, seqno - 1);
	if (ret)
		return ret;

	/* Carefully set the last_seqno value so that wrap
	 * detection still works
	 */
	dev_priv->next_seqno = seqno;
	dev_priv->last_seqno = seqno - 1;
	if (dev_priv->last_seqno == 0)
		dev_priv->last_seqno--;

	return 0;
}

int
i915_gem_get_seqno(struct drm_device *dev, u32 *seqno)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	/* reserve 0 for non-seqno */
	if (dev_priv->next_seqno == 0) {
		int ret = i915_gem_init_seqno(dev, 0);
		if (ret)
			return ret;

		dev_priv->next_seqno = 1;
	}

	*seqno = dev_priv->last_seqno = dev_priv->next_seqno++;
	return 0;
}

int __i915_add_request(struct intel_ring_buffer *ring,
		       struct drm_file *file,
		       struct drm_i915_gem_object *obj,
		       u32 *out_seqno)
{
	struct drm_i915_private *dev_priv = ring->dev->dev_private;
	struct drm_i915_gem_request *request;
	u32 request_ring_position, request_start;
	int ret;

	request_start = intel_ring_get_tail(ring);
	/*
	 * Emit any outstanding flushes - execbuf can fail to emit the flush
	 * after having emitted the batchbuffer command. Hence we need to fix
	 * things up similar to emitting the lazy request. The difference here
	 * is that the flush _must_ happen before the next request, no matter
	 * what.
	 */
	ret = intel_ring_flush_all_caches(ring);
	if (ret)
		return ret;

	request = ring->preallocated_lazy_request;
	if (WARN_ON(request == NULL))
		return -ENOMEM;

	/* Record the position of the start of the request so that
	 * should we detect the updated seqno part-way through the
	 * GPU processing the request, we never over-estimate the
	 * position of the head.
	 */
	request_ring_position = intel_ring_get_tail(ring);

	ret = ring->add_request(ring);
	if (ret)
		return ret;

	request->seqno = intel_ring_get_seqno(ring);
	request->ring = ring;
	request->head = request_start;
	request->tail = request_ring_position;

	/* Whilst this request exists, batch_obj will be on the
	 * active_list, and so will hold the active reference. Only when this
	 * request is retired will the the batch_obj be moved onto the
	 * inactive_list and lose its active reference. Hence we do not need
	 * to explicitly hold another reference here.
	 */
	request->batch_obj = obj;

	/* Hold a reference to the current context so that we can inspect
	 * it later in case a hangcheck error event fires.
	 */
	request->ctx = ring->last_context;
	if (request->ctx)
		i915_gem_context_reference(request->ctx);

	request->emitted_jiffies = jiffies;
	list_add_tail(&request->list, &ring->request_list);
	request->file_priv = NULL;

	if (file) {
		struct drm_i915_file_private *file_priv = file->driver_priv;

		spin_lock(&file_priv->mm.lock);
		request->file_priv = file_priv;
		list_add_tail(&request->client_list,
			      &file_priv->mm.request_list);
		spin_unlock(&file_priv->mm.lock);
	}

	trace_i915_gem_request_add(ring, request->seqno);
	ring->outstanding_lazy_seqno = 0;
	ring->preallocated_lazy_request = NULL;

	if (!dev_priv->ums.mm_suspended) {
		i915_queue_hangcheck(ring->dev);

		cancel_delayed_work_sync(&dev_priv->mm.idle_work);
		queue_delayed_work(dev_priv->wq,
				   &dev_priv->mm.retire_work,
				   round_jiffies_up_relative(HZ));
		intel_mark_busy(dev_priv->dev);
	}

	if (out_seqno)
		*out_seqno = request->seqno;
	return 0;
}

static inline void
i915_gem_request_remove_from_client(struct drm_i915_gem_request *request)
{
	struct drm_i915_file_private *file_priv = request->file_priv;

	if (!file_priv)
		return;

	spin_lock(&file_priv->mm.lock);
	list_del(&request->client_list);
	request->file_priv = NULL;
	spin_unlock(&file_priv->mm.lock);
}

static bool i915_context_is_banned(struct drm_i915_private *dev_priv,
				   const struct i915_hw_context *ctx)
{
	unsigned long elapsed;

	elapsed = get_seconds() - ctx->hang_stats.guilty_ts;

	if (ctx->hang_stats.banned)
		return true;

	if (elapsed <= DRM_I915_CTX_BAN_PERIOD) {
		if (!i915_gem_context_is_default(ctx)) {
			DRM_DEBUG("context hanging too fast, banning!\n");
			return true;
		} else if (dev_priv->gpu_error.stop_rings == 0) {
			DRM_ERROR("gpu hanging too fast, banning!\n");
			return true;
		}
	}

	return false;
}

static void i915_set_reset_status(struct drm_i915_private *dev_priv,
				  struct i915_hw_context *ctx,
				  const bool guilty)
{
	struct i915_ctx_hang_stats *hs;

	if (WARN_ON(!ctx))
		return;

	hs = &ctx->hang_stats;

	if (guilty) {
		hs->banned = i915_context_is_banned(dev_priv, ctx);
		hs->batch_active++;
		hs->guilty_ts = get_seconds();
	} else {
		hs->batch_pending++;
	}
}

static void i915_gem_free_request(struct drm_i915_gem_request *request)
{
	list_del(&request->list);
	i915_gem_request_remove_from_client(request);

	if (request->ctx)
		i915_gem_context_unreference(request->ctx);

	kfree(request);
}

struct drm_i915_gem_request *
i915_gem_find_active_request(struct intel_ring_buffer *ring)
{
	struct drm_i915_gem_request *request;
	u32 completed_seqno;

	completed_seqno = ring->get_seqno(ring, false);

	list_for_each_entry(request, &ring->request_list, list) {
		if (i915_seqno_passed(completed_seqno, request->seqno))
			continue;

		return request;
	}

	return NULL;
}

static void i915_gem_reset_ring_status(struct drm_i915_private *dev_priv,
				       struct intel_ring_buffer *ring)
{
	struct drm_i915_gem_request *request;
	bool ring_hung;

	request = i915_gem_find_active_request(ring);

	if (request == NULL)
		return;

	ring_hung = ring->hangcheck.score >= HANGCHECK_SCORE_RING_HUNG;

	i915_set_reset_status(dev_priv, request->ctx, ring_hung);

	list_for_each_entry_continue(request, &ring->request_list, list)
		i915_set_reset_status(dev_priv, request->ctx, false);
}

static void i915_gem_reset_ring_cleanup(struct drm_i915_private *dev_priv,
					struct intel_ring_buffer *ring)
{
	while (!list_empty(&ring->active_list)) {
		struct drm_i915_gem_object *obj;

		obj = list_first_entry(&ring->active_list,
				       struct drm_i915_gem_object,
				       ring_list);

		i915_gem_object_move_to_inactive(obj);
	}

	/*
	 * We must free the requests after all the corresponding objects have
	 * been moved off active lists. Which is the same order as the normal
	 * retire_requests function does. This is important if object hold
	 * implicit references on things like e.g. ppgtt address spaces through
	 * the request.
	 */
	while (!list_empty(&ring->request_list)) {
		struct drm_i915_gem_request *request;

		request = list_first_entry(&ring->request_list,
					   struct drm_i915_gem_request,
					   list);

		i915_gem_free_request(request);
	}
}

void i915_gem_restore_fences(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int i;

	for (i = 0; i < dev_priv->num_fence_regs; i++) {
		struct drm_i915_fence_reg *reg = &dev_priv->fence_regs[i];

		/*
		 * Commit delayed tiling changes if we have an object still
		 * attached to the fence, otherwise just clear the fence.
		 */
		if (reg->obj) {
			i915_gem_object_update_fence(reg->obj, reg,
						     reg->obj->tiling_mode);
		} else {
			i915_gem_write_fence(dev, i, NULL);
		}
	}
}

void i915_gem_reset(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_ring_buffer *ring;
	int i;

	/*
	 * Before we free the objects from the requests, we need to inspect
	 * them for finding the guilty party. As the requests only borrow
	 * their reference to the objects, the inspection must be done first.
	 */
	for_each_ring(ring, dev_priv, i)
		i915_gem_reset_ring_status(dev_priv, ring);

	for_each_ring(ring, dev_priv, i)
		i915_gem_reset_ring_cleanup(dev_priv, ring);

	i915_gem_cleanup_ringbuffer(dev);

	i915_gem_context_reset(dev);

	i915_gem_restore_fences(dev);
}

/**
 * This function clears the request list as sequence numbers are passed.
 */
static void
i915_gem_retire_requests_ring(struct intel_ring_buffer *ring)
{
	uint32_t seqno;

	if (list_empty(&ring->request_list))
		return;

	WARN_ON(i915_verify_lists(ring->dev));

	seqno = ring->get_seqno(ring, true);

	/* Move any buffers on the active list that are no longer referenced
	 * by the ringbuffer to the flushing/inactive lists as appropriate,
	 * before we free the context associated with the requests.
	 */
	while (!list_empty(&ring->active_list)) {
		struct drm_i915_gem_object *obj;

		obj = list_first_entry(&ring->active_list,
				      struct drm_i915_gem_object,
				      ring_list);

		if (!i915_seqno_passed(seqno, obj->last_read_seqno))
			break;

		i915_gem_object_move_to_inactive(obj);
	}


	while (!list_empty(&ring->request_list)) {
		struct drm_i915_gem_request *request;

		request = list_first_entry(&ring->request_list,
					   struct drm_i915_gem_request,
					   list);

		if (!i915_seqno_passed(seqno, request->seqno))
			break;

		trace_i915_gem_request_retire(ring, request->seqno);
		/* We know the GPU must have read the request to have
		 * sent us the seqno + interrupt, so use the position
		 * of tail of the request to update the last known position
		 * of the GPU head.
		 */
		ring->last_retired_head = request->tail;

		i915_gem_free_request(request);
	}

	if (unlikely(ring->trace_irq_seqno &&
		     i915_seqno_passed(seqno, ring->trace_irq_seqno))) {
		ring->irq_put(ring);
		ring->trace_irq_seqno = 0;
	}

	WARN_ON(i915_verify_lists(ring->dev));
}

bool
i915_gem_retire_requests(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_ring_buffer *ring;
	bool idle = true;
	int i;

	for_each_ring(ring, dev_priv, i) {
		i915_gem_retire_requests_ring(ring);
		idle &= list_empty(&ring->request_list);
	}

	if (idle)
		mod_delayed_work(dev_priv->wq,
				   &dev_priv->mm.idle_work,
				   msecs_to_jiffies(100));

	return idle;
}

static void
i915_gem_retire_work_handler(struct work_struct *work)
{
	struct drm_i915_private *dev_priv =
		container_of(work, typeof(*dev_priv), mm.retire_work.work);
	struct drm_device *dev = dev_priv->dev;
	bool idle;

	/* Come back later if the device is busy... */
	idle = false;
	if (mutex_trylock(&dev->struct_mutex)) {
		idle = i915_gem_retire_requests(dev);
		mutex_unlock(&dev->struct_mutex);
	}
	if (!idle)
		queue_delayed_work(dev_priv->wq, &dev_priv->mm.retire_work,
				   round_jiffies_up_relative(HZ));
}

static void
i915_gem_idle_work_handler(struct work_struct *work)
{
	struct drm_i915_private *dev_priv =
		container_of(work, typeof(*dev_priv), mm.idle_work.work);

	intel_mark_idle(dev_priv->dev);
}

/**
 * Ensures that an object will eventually get non-busy by flushing any required
 * write domains, emitting any outstanding lazy request and retiring and
 * completed requests.
 */
static int
i915_gem_object_flush_active(struct drm_i915_gem_object *obj)
{
	int ret;

	if (obj->active) {
		ret = i915_gem_check_olr(obj->ring, obj->last_read_seqno);
		if (ret)
			return ret;

		i915_gem_retire_requests_ring(obj->ring);
	}

	return 0;
}

/**
 * i915_gem_wait_ioctl - implements DRM_IOCTL_I915_GEM_WAIT
 * @DRM_IOCTL_ARGS: standard ioctl arguments
 *
 * Returns 0 if successful, else an error is returned with the remaining time in
 * the timeout parameter.
 *  -ETIME: object is still busy after timeout
 *  -ERESTARTSYS: signal interrupted the wait
 *  -ENONENT: object doesn't exist
 * Also possible, but rare:
 *  -EAGAIN: GPU wedged
 *  -ENOMEM: damn
 *  -ENODEV: Internal IRQ fail
 *  -E?: The add request failed
 *
 * The wait ioctl with a timeout of 0 reimplements the busy ioctl. With any
 * non-zero timeout parameter the wait ioctl will wait for the given number of
 * nanoseconds on an object becoming unbusy. Since the wait itself does so
 * without holding struct_mutex the object may become re-busied before this
 * function completes. A similar but shorter * race condition exists in the busy
 * ioctl
 */
int
i915_gem_wait_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_wait *args = data;
	struct drm_i915_gem_object *obj;
	struct intel_ring_buffer *ring = NULL;
	struct timespec timeout_stack, *timeout = NULL;
	unsigned reset_counter;
	u32 seqno = 0;
	int ret = 0;

	if (args->timeout_ns >= 0) {
		timeout_stack = ns_to_timespec(args->timeout_ns);
		timeout = &timeout_stack;
	}

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->bo_handle));
	if (&obj->base == NULL) {
		mutex_unlock(&dev->struct_mutex);
		return -ENOENT;
	}

	/* Need to make sure the object gets inactive eventually. */
	ret = i915_gem_object_flush_active(obj);
	if (ret)
		goto out;

	if (obj->active) {
		seqno = obj->last_read_seqno;
		ring = obj->ring;
	}

	if (seqno == 0)
		 goto out;

	/* Do this after OLR check to make sure we make forward progress polling
	 * on this IOCTL with a 0 timeout (like busy ioctl)
	 */
	if (!args->timeout_ns) {
		ret = -ETIME;
		goto out;
	}

	drm_gem_object_unreference(&obj->base);
	reset_counter = atomic_read(&dev_priv->gpu_error.reset_counter);
	mutex_unlock(&dev->struct_mutex);

	ret = __wait_seqno(ring, seqno, reset_counter, true, timeout, file->driver_priv);
	if (timeout)
		args->timeout_ns = timespec_to_ns(timeout);
	return ret;

out:
	drm_gem_object_unreference(&obj->base);
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

/**
 * i915_gem_object_sync - sync an object to a ring.
 *
 * @obj: object which may be in use on another ring.
 * @to: ring we wish to use the object on. May be NULL.
 *
 * This code is meant to abstract object synchronization with the GPU.
 * Calling with NULL implies synchronizing the object with the CPU
 * rather than a particular GPU ring.
 *
 * Returns 0 if successful, else propagates up the lower layer error.
 */
int
i915_gem_object_sync(struct drm_i915_gem_object *obj,
		     struct intel_ring_buffer *to)
{
	struct intel_ring_buffer *from = obj->ring;
	u32 seqno;
	int ret, idx;

	if (from == NULL || to == from)
		return 0;

	if (to == NULL || !i915_semaphore_is_enabled(obj->base.dev))
		return i915_gem_object_wait_rendering(obj, false);

	idx = intel_ring_sync_index(from, to);

	seqno = obj->last_read_seqno;
	if (seqno <= from->sync_seqno[idx])
		return 0;

	ret = i915_gem_check_olr(obj->ring, seqno);
	if (ret)
		return ret;

	trace_i915_gem_ring_sync_to(from, to, seqno);
	ret = to->sync_to(to, from, seqno);
	if (!ret)
		/* We use last_read_seqno because sync_to()
		 * might have just caused seqno wrap under
		 * the radar.
		 */
		from->sync_seqno[idx] = obj->last_read_seqno;

	return ret;
}

static void i915_gem_object_finish_gtt(struct drm_i915_gem_object *obj)
{
	u32 old_write_domain, old_read_domains;

	/* Force a pagefault for domain tracking on next user access */
	i915_gem_release_mmap(obj);

	if ((obj->base.read_domains & I915_GEM_DOMAIN_GTT) == 0)
		return;

	/* Wait for any direct GTT access to complete */
	mb();

	old_read_domains = obj->base.read_domains;
	old_write_domain = obj->base.write_domain;

	obj->base.read_domains &= ~I915_GEM_DOMAIN_GTT;
	obj->base.write_domain &= ~I915_GEM_DOMAIN_GTT;

	trace_i915_gem_object_change_domain(obj,
					    old_read_domains,
					    old_write_domain);
}

int i915_vma_unbind(struct i915_vma *vma)
{
	struct drm_i915_gem_object *obj = vma->obj;
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	int ret;

	if (list_empty(&vma->vma_link))
		return 0;

	if (!drm_mm_node_allocated(&vma->node)) {
		i915_gem_vma_destroy(vma);
		return 0;
	}

	if (vma->pin_count)
		return -EBUSY;

	BUG_ON(obj->pages == NULL);

	ret = i915_gem_object_finish_gpu(obj);
	if (ret)
		return ret;
	/* Continue on if we fail due to EIO, the GPU is hung so we
	 * should be safe and we need to cleanup or else we might
	 * cause memory corruption through use-after-free.
	 */

	i915_gem_object_finish_gtt(obj);

	/* release the fence reg _after_ flushing */
	ret = i915_gem_object_put_fence(obj);
	if (ret)
		return ret;

	trace_i915_vma_unbind(vma);

	vma->unbind_vma(vma);

	i915_gem_gtt_finish_object(obj);

	list_del_init(&vma->mm_list);
	/* Avoid an unnecessary call to unbind on rebind. */
	if (i915_is_ggtt(vma->vm))
		obj->map_and_fenceable = true;

	drm_mm_remove_node(&vma->node);
	i915_gem_vma_destroy(vma);

	/* Since the unbound list is global, only move to that list if
	 * no more VMAs exist. */
	if (list_empty(&obj->vma_list))
		list_move_tail(&obj->global_list, &dev_priv->mm.unbound_list);

	/* And finally now the object is completely decoupled from this vma,
	 * we can drop its hold on the backing storage and allow it to be
	 * reaped by the shrinker.
	 */
	i915_gem_object_unpin_pages(obj);

	return 0;
}

int i915_gpu_idle(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_ring_buffer *ring;
	int ret, i;

	/* Flush everything onto the inactive list. */
	for_each_ring(ring, dev_priv, i) {
		ret = i915_switch_context(ring, ring->default_context);
		if (ret)
			return ret;

		ret = intel_ring_idle(ring);
		if (ret)
			return ret;
	}

	return 0;
}

static void i965_write_fence_reg(struct drm_device *dev, int reg,
				 struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int fence_reg;
	int fence_pitch_shift;

	if (INTEL_INFO(dev)->gen >= 6) {
		fence_reg = FENCE_REG_SANDYBRIDGE_0;
		fence_pitch_shift = SANDYBRIDGE_FENCE_PITCH_SHIFT;
	} else {
		fence_reg = FENCE_REG_965_0;
		fence_pitch_shift = I965_FENCE_PITCH_SHIFT;
	}

	fence_reg += reg * 8;

	/* To w/a incoherency with non-atomic 64-bit register updates,
	 * we split the 64-bit update into two 32-bit writes. In order
	 * for a partial fence not to be evaluated between writes, we
	 * precede the update with write to turn off the fence register,
	 * and only enable the fence as the last step.
	 *
	 * For extra levels of paranoia, we make sure each step lands
	 * before applying the next step.
	 */
	I915_WRITE(fence_reg, 0);
	POSTING_READ(fence_reg);

	if (obj) {
		u32 size = i915_gem_obj_ggtt_size(obj);
		uint64_t val;

		val = (uint64_t)((i915_gem_obj_ggtt_offset(obj) + size - 4096) &
				 0xfffff000) << 32;
		val |= i915_gem_obj_ggtt_offset(obj) & 0xfffff000;
		val |= (uint64_t)((obj->stride / 128) - 1) << fence_pitch_shift;
		if (obj->tiling_mode == I915_TILING_Y)
			val |= 1 << I965_FENCE_TILING_Y_SHIFT;
		val |= I965_FENCE_REG_VALID;

		I915_WRITE(fence_reg + 4, val >> 32);
		POSTING_READ(fence_reg + 4);

		I915_WRITE(fence_reg + 0, val);
		POSTING_READ(fence_reg);
	} else {
		I915_WRITE(fence_reg + 4, 0);
		POSTING_READ(fence_reg + 4);
	}
}

static void i915_write_fence_reg(struct drm_device *dev, int reg,
				 struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 val;

	if (obj) {
		u32 size = i915_gem_obj_ggtt_size(obj);
		int pitch_val;
		int tile_width;

		WARN((i915_gem_obj_ggtt_offset(obj) & ~I915_FENCE_START_MASK) ||
		     (size & -size) != size ||
		     (i915_gem_obj_ggtt_offset(obj) & (size - 1)),
		     "object 0x%08lx [fenceable? %d] not 1M or pot-size (0x%08x) aligned\n",
		     i915_gem_obj_ggtt_offset(obj), obj->map_and_fenceable, size);

		if (obj->tiling_mode == I915_TILING_Y && HAS_128_BYTE_Y_TILING(dev))
			tile_width = 128;
		else
			tile_width = 512;

		/* Note: pitch better be a power of two tile widths */
		pitch_val = obj->stride / tile_width;
		pitch_val = ffs(pitch_val) - 1;

		val = i915_gem_obj_ggtt_offset(obj);
		if (obj->tiling_mode == I915_TILING_Y)
			val |= 1 << I830_FENCE_TILING_Y_SHIFT;
		val |= I915_FENCE_SIZE_BITS(size);
		val |= pitch_val << I830_FENCE_PITCH_SHIFT;
		val |= I830_FENCE_REG_VALID;
	} else
		val = 0;

	if (reg < 8)
		reg = FENCE_REG_830_0 + reg * 4;
	else
		reg = FENCE_REG_945_8 + (reg - 8) * 4;

	I915_WRITE(reg, val);
	POSTING_READ(reg);
}

static void i830_write_fence_reg(struct drm_device *dev, int reg,
				struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	uint32_t val;

	if (obj) {
		u32 size = i915_gem_obj_ggtt_size(obj);
		uint32_t pitch_val;

		WARN((i915_gem_obj_ggtt_offset(obj) & ~I830_FENCE_START_MASK) ||
		     (size & -size) != size ||
		     (i915_gem_obj_ggtt_offset(obj) & (size - 1)),
		     "object 0x%08lx not 512K or pot-size 0x%08x aligned\n",
		     i915_gem_obj_ggtt_offset(obj), size);

		pitch_val = obj->stride / 128;
		pitch_val = ffs(pitch_val) - 1;

		val = i915_gem_obj_ggtt_offset(obj);
		if (obj->tiling_mode == I915_TILING_Y)
			val |= 1 << I830_FENCE_TILING_Y_SHIFT;
		val |= I830_FENCE_SIZE_BITS(size);
		val |= pitch_val << I830_FENCE_PITCH_SHIFT;
		val |= I830_FENCE_REG_VALID;
	} else
		val = 0;

	I915_WRITE(FENCE_REG_830_0 + reg * 4, val);
	POSTING_READ(FENCE_REG_830_0 + reg * 4);
}

inline static bool i915_gem_object_needs_mb(struct drm_i915_gem_object *obj)
{
	return obj && obj->base.read_domains & I915_GEM_DOMAIN_GTT;
}

static void i915_gem_write_fence(struct drm_device *dev, int reg,
				 struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	/* Ensure that all CPU reads are completed before installing a fence
	 * and all writes before removing the fence.
	 */
	if (i915_gem_object_needs_mb(dev_priv->fence_regs[reg].obj))
		mb();

	WARN(obj && (!obj->stride || !obj->tiling_mode),
	     "bogus fence setup with stride: 0x%x, tiling mode: %i\n",
	     obj->stride, obj->tiling_mode);

	switch (INTEL_INFO(dev)->gen) {
	case 8:
	case 7:
	case 6:
	case 5:
	case 4: i965_write_fence_reg(dev, reg, obj); break;
	case 3: i915_write_fence_reg(dev, reg, obj); break;
	case 2: i830_write_fence_reg(dev, reg, obj); break;
	default: BUG();
	}

	/* And similarly be paranoid that no direct access to this region
	 * is reordered to before the fence is installed.
	 */
	if (i915_gem_object_needs_mb(obj))
		mb();
}

static inline int fence_number(struct drm_i915_private *dev_priv,
			       struct drm_i915_fence_reg *fence)
{
	return fence - dev_priv->fence_regs;
}

static void i915_gem_object_update_fence(struct drm_i915_gem_object *obj,
					 struct drm_i915_fence_reg *fence,
					 bool enable)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	int reg = fence_number(dev_priv, fence);

	i915_gem_write_fence(obj->base.dev, reg, enable ? obj : NULL);

	if (enable) {
		obj->fence_reg = reg;
		fence->obj = obj;
		list_move_tail(&fence->lru_list, &dev_priv->mm.fence_list);
	} else {
		obj->fence_reg = I915_FENCE_REG_NONE;
		fence->obj = NULL;
		list_del_init(&fence->lru_list);
	}
	obj->fence_dirty = false;
}

static int
i915_gem_object_wait_fence(struct drm_i915_gem_object *obj)
{
	if (obj->last_fenced_seqno) {
		int ret = i915_wait_seqno(obj->ring, obj->last_fenced_seqno);
		if (ret)
			return ret;

		obj->last_fenced_seqno = 0;
	}

	obj->fenced_gpu_access = false;
	return 0;
}

int
i915_gem_object_put_fence(struct drm_i915_gem_object *obj)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	struct drm_i915_fence_reg *fence;
	int ret;

	ret = i915_gem_object_wait_fence(obj);
	if (ret)
		return ret;

	if (obj->fence_reg == I915_FENCE_REG_NONE)
		return 0;

	fence = &dev_priv->fence_regs[obj->fence_reg];

	i915_gem_object_fence_lost(obj);
	i915_gem_object_update_fence(obj, fence, false);

	return 0;
}

static struct drm_i915_fence_reg *
i915_find_fence_reg(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_fence_reg *reg, *avail;
	int i;

	/* First try to find a free reg */
	avail = NULL;
	for (i = dev_priv->fence_reg_start; i < dev_priv->num_fence_regs; i++) {
		reg = &dev_priv->fence_regs[i];
		if (!reg->obj)
			return reg;

		if (!reg->pin_count)
			avail = reg;
	}

	if (avail == NULL)
		goto deadlock;

	/* None available, try to steal one or wait for a user to finish */
	list_for_each_entry(reg, &dev_priv->mm.fence_list, lru_list) {
		if (reg->pin_count)
			continue;

		return reg;
	}

deadlock:
	/* Wait for completion of pending flips which consume fences */
	if (intel_has_pending_fb_unpin(dev))
		return ERR_PTR(-EAGAIN);

	return ERR_PTR(-EDEADLK);
}

/**
 * i915_gem_object_get_fence - set up fencing for an object
 * @obj: object to map through a fence reg
 *
 * When mapping objects through the GTT, userspace wants to be able to write
 * to them without having to worry about swizzling if the object is tiled.
 * This function walks the fence regs looking for a free one for @obj,
 * stealing one if it can't find any.
 *
 * It then sets up the reg based on the object's properties: address, pitch
 * and tiling format.
 *
 * For an untiled surface, this removes any existing fence.
 */
int
i915_gem_object_get_fence(struct drm_i915_gem_object *obj)
{
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	bool enable = obj->tiling_mode != I915_TILING_NONE;
	struct drm_i915_fence_reg *reg;
	int ret;

	/* Have we updated the tiling parameters upon the object and so
	 * will need to serialise the write to the associated fence register?
	 */
	if (obj->fence_dirty) {
		ret = i915_gem_object_wait_fence(obj);
		if (ret)
			return ret;
	}

	/* Just update our place in the LRU if our fence is getting reused. */
	if (obj->fence_reg != I915_FENCE_REG_NONE) {
		reg = &dev_priv->fence_regs[obj->fence_reg];
		if (!obj->fence_dirty) {
			list_move_tail(&reg->lru_list,
				       &dev_priv->mm.fence_list);
			return 0;
		}
	} else if (enable) {
		reg = i915_find_fence_reg(dev);
		if (IS_ERR(reg))
			return PTR_ERR(reg);

		if (reg->obj) {
			struct drm_i915_gem_object *old = reg->obj;

			ret = i915_gem_object_wait_fence(old);
			if (ret)
				return ret;

			i915_gem_object_fence_lost(old);
		}
	} else
		return 0;

	i915_gem_object_update_fence(obj, reg, enable);

	return 0;
}

static bool i915_gem_valid_gtt_space(struct drm_device *dev,
				     struct drm_mm_node *gtt_space,
				     unsigned long cache_level)
{
	struct drm_mm_node *other;

	/* On non-LLC machines we have to be careful when putting differing
	 * types of snoopable memory together to avoid the prefetcher
	 * crossing memory domains and dying.
	 */
	if (HAS_LLC(dev))
		return true;

	if (!drm_mm_node_allocated(gtt_space))
		return true;

	if (list_empty(&gtt_space->node_list))
		return true;

	other = list_entry(gtt_space->node_list.prev, struct drm_mm_node, node_list);
	if (other->allocated && !other->hole_follows && other->color != cache_level)
		return false;

	other = list_entry(gtt_space->node_list.next, struct drm_mm_node, node_list);
	if (other->allocated && !gtt_space->hole_follows && other->color != cache_level)
		return false;

	return true;
}

static void i915_gem_verify_gtt(struct drm_device *dev)
{
#if WATCH_GTT
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;
	int err = 0;

	list_for_each_entry(obj, &dev_priv->mm.gtt_list, global_list) {
		if (obj->gtt_space == NULL) {
			printk(KERN_ERR "object found on GTT list with no space reserved\n");
			err++;
			continue;
		}

		if (obj->cache_level != obj->gtt_space->color) {
			printk(KERN_ERR "object reserved space [%08lx, %08lx] with wrong color, cache_level=%x, color=%lx\n",
			       i915_gem_obj_ggtt_offset(obj),
			       i915_gem_obj_ggtt_offset(obj) + i915_gem_obj_ggtt_size(obj),
			       obj->cache_level,
			       obj->gtt_space->color);
			err++;
			continue;
		}

		if (!i915_gem_valid_gtt_space(dev,
					      obj->gtt_space,
					      obj->cache_level)) {
			printk(KERN_ERR "invalid GTT space found at [%08lx, %08lx] - color=%x\n",
			       i915_gem_obj_ggtt_offset(obj),
			       i915_gem_obj_ggtt_offset(obj) + i915_gem_obj_ggtt_size(obj),
			       obj->cache_level);
			err++;
			continue;
		}
	}

	WARN_ON(err);
#endif
}

/**
 * Finds free space in the GTT aperture and binds the object there.
 */
static struct i915_vma *
i915_gem_object_bind_to_vm(struct drm_i915_gem_object *obj,
			   struct i915_address_space *vm,
			   unsigned alignment,
			   uint64_t flags)
{
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 size, fence_size, fence_alignment, unfenced_alignment;
	unsigned long start =
		flags & PIN_OFFSET_BIAS ? flags & PIN_OFFSET_MASK : 0;
	unsigned long end =
		flags & PIN_MAPPABLE ? dev_priv->gtt.mappable_end : vm->total;
	struct i915_vma *vma;
	int ret;

	fence_size = i915_gem_get_gtt_size(dev,
					   obj->base.size,
					   obj->tiling_mode);
	fence_alignment = i915_gem_get_gtt_alignment(dev,
						     obj->base.size,
						     obj->tiling_mode, true);
	unfenced_alignment =
		i915_gem_get_gtt_alignment(dev,
					   obj->base.size,
					   obj->tiling_mode, false);

	if (alignment == 0)
		alignment = flags & PIN_MAPPABLE ? fence_alignment :
						unfenced_alignment;
	if (flags & PIN_MAPPABLE && alignment & (fence_alignment - 1)) {
		DRM_DEBUG("Invalid object alignment requested %u\n", alignment);
		return ERR_PTR(-EINVAL);
	}

	size = flags & PIN_MAPPABLE ? fence_size : obj->base.size;

	/* If the object is bigger than the entire aperture, reject it early
	 * before evicting everything in a vain attempt to find space.
	 */
	if (obj->base.size > end) {
		DRM_DEBUG("Attempting to bind an object larger than the aperture: object=%zd > %s aperture=%lu\n",
			  obj->base.size,
			  flags & PIN_MAPPABLE ? "mappable" : "total",
			  end);
		return ERR_PTR(-E2BIG);
	}

	ret = i915_gem_object_get_pages(obj);
	if (ret)
		return ERR_PTR(ret);

	i915_gem_object_pin_pages(obj);

	vma = i915_gem_obj_lookup_or_create_vma(obj, vm);
	if (IS_ERR(vma))
		goto err_unpin;

search_free:
	ret = drm_mm_insert_node_in_range_generic(&vm->mm, &vma->node,
						  size, alignment,
						  obj->cache_level,
						  start, end,
						  DRM_MM_SEARCH_DEFAULT,
						  DRM_MM_CREATE_DEFAULT);
	if (ret) {
		ret = i915_gem_evict_something(dev, vm, size, alignment,
					       obj->cache_level,
					       start, end,
					       flags);
		if (ret == 0)
			goto search_free;

		goto err_free_vma;
	}
	if (WARN_ON(!i915_gem_valid_gtt_space(dev, &vma->node,
					      obj->cache_level))) {
		ret = -EINVAL;
		goto err_remove_node;
	}

	ret = i915_gem_gtt_prepare_object(obj);
	if (ret)
		goto err_remove_node;

	list_move_tail(&obj->global_list, &dev_priv->mm.bound_list);
	list_add_tail(&vma->mm_list, &vm->inactive_list);

	if (i915_is_ggtt(vm)) {
		bool mappable, fenceable;

		fenceable = (vma->node.size == fence_size &&
			     (vma->node.start & (fence_alignment - 1)) == 0);

		mappable = (vma->node.start + obj->base.size <=
			    dev_priv->gtt.mappable_end);

		obj->map_and_fenceable = mappable && fenceable;
	}

	WARN_ON(flags & PIN_MAPPABLE && !obj->map_and_fenceable);

	trace_i915_vma_bind(vma, flags);
	vma->bind_vma(vma, obj->cache_level,
		      flags & (PIN_MAPPABLE | PIN_GLOBAL) ? GLOBAL_BIND : 0);

	i915_gem_verify_gtt(dev);
	return vma;

err_remove_node:
	drm_mm_remove_node(&vma->node);
err_free_vma:
	i915_gem_vma_destroy(vma);
	vma = ERR_PTR(ret);
err_unpin:
	i915_gem_object_unpin_pages(obj);
	return vma;
}

bool
i915_gem_clflush_object(struct drm_i915_gem_object *obj,
			bool force)
{
	/* If we don't have a page list set up, then we're not pinned
	 * to GPU, and we can ignore the cache flush because it'll happen
	 * again at bind time.
	 */
	if (obj->pages == NULL)
		return false;

	/*
	 * Stolen memory is always coherent with the GPU as it is explicitly
	 * marked as wc by the system, or the system is cache-coherent.
	 */
	if (obj->stolen)
		return false;

	/* If the GPU is snooping the contents of the CPU cache,
	 * we do not need to manually clear the CPU cache lines.  However,
	 * the caches are only snooped when the render cache is
	 * flushed/invalidated.  As we always have to emit invalidations
	 * and flushes when moving into and out of the RENDER domain, correct
	 * snooping behaviour occurs naturally as the result of our domain
	 * tracking.
	 */
	if (!force && cpu_cache_is_coherent(obj->base.dev, obj->cache_level))
		return false;

	trace_i915_gem_object_clflush(obj);
#ifdef __NetBSD__
	drm_clflush_pglist(&obj->igo_pageq);
#else
	drm_clflush_sg(obj->pages);
#endif

	return true;
}

/** Flushes the GTT write domain for the object if it's dirty. */
static void
i915_gem_object_flush_gtt_write_domain(struct drm_i915_gem_object *obj)
{
	uint32_t old_write_domain;

	if (obj->base.write_domain != I915_GEM_DOMAIN_GTT)
		return;

	/* No actual flushing is required for the GTT write domain.  Writes
	 * to it immediately go to main memory as far as we know, so there's
	 * no chipset flush.  It also doesn't land in render cache.
	 *
	 * However, we do have to enforce the order so that all writes through
	 * the GTT land before any writes to the device, such as updates to
	 * the GATT itself.
	 */
	wmb();

	old_write_domain = obj->base.write_domain;
	obj->base.write_domain = 0;

	trace_i915_gem_object_change_domain(obj,
					    obj->base.read_domains,
					    old_write_domain);
}

/** Flushes the CPU write domain for the object if it's dirty. */
static void
i915_gem_object_flush_cpu_write_domain(struct drm_i915_gem_object *obj,
				       bool force)
{
	uint32_t old_write_domain;

	if (obj->base.write_domain != I915_GEM_DOMAIN_CPU)
		return;

	if (i915_gem_clflush_object(obj, force))
		i915_gem_chipset_flush(obj->base.dev);

	old_write_domain = obj->base.write_domain;
	obj->base.write_domain = 0;

	trace_i915_gem_object_change_domain(obj,
					    obj->base.read_domains,
					    old_write_domain);
}

/**
 * Moves a single object to the GTT read, and possibly write domain.
 *
 * This function returns when the move is complete, including waiting on
 * flushes to occur.
 */
int
i915_gem_object_set_to_gtt_domain(struct drm_i915_gem_object *obj, bool write)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	uint32_t old_write_domain, old_read_domains;
	int ret;

	/* Not valid to be called on unbound objects. */
	if (!i915_gem_obj_bound_any(obj))
		return -EINVAL;

	if (obj->base.write_domain == I915_GEM_DOMAIN_GTT)
		return 0;

	ret = i915_gem_object_wait_rendering(obj, !write);
	if (ret)
		return ret;

	i915_gem_object_flush_cpu_write_domain(obj, false);

	/* Serialise direct access to this object with the barriers for
	 * coherent writes from the GPU, by effectively invalidating the
	 * GTT domain upon first access.
	 */
	if ((obj->base.read_domains & I915_GEM_DOMAIN_GTT) == 0)
		mb();

	old_write_domain = obj->base.write_domain;
	old_read_domains = obj->base.read_domains;

	/* It should now be out of any other write domains, and we can update
	 * the domain values for our changes.
	 */
	BUG_ON((obj->base.write_domain & ~I915_GEM_DOMAIN_GTT) != 0);
	obj->base.read_domains |= I915_GEM_DOMAIN_GTT;
	if (write) {
		obj->base.read_domains = I915_GEM_DOMAIN_GTT;
		obj->base.write_domain = I915_GEM_DOMAIN_GTT;
		obj->dirty = 1;
	}

	trace_i915_gem_object_change_domain(obj,
					    old_read_domains,
					    old_write_domain);

	/* And bump the LRU for this access */
	if (i915_gem_object_is_inactive(obj)) {
		struct i915_vma *vma = i915_gem_obj_to_ggtt(obj);
		if (vma)
			list_move_tail(&vma->mm_list,
				       &dev_priv->gtt.base.inactive_list);

	}

	return 0;
}

int i915_gem_object_set_cache_level(struct drm_i915_gem_object *obj,
				    enum i915_cache_level cache_level)
{
	struct drm_device *dev = obj->base.dev;
	struct i915_vma *vma, *next;
	int ret;

	if (obj->cache_level == cache_level)
		return 0;

	if (i915_gem_obj_is_pinned(obj)) {
		DRM_DEBUG("can not change the cache level of pinned objects\n");
		return -EBUSY;
	}

	list_for_each_entry_safe(vma, next, &obj->vma_list, vma_link) {
		if (!i915_gem_valid_gtt_space(dev, &vma->node, cache_level)) {
			ret = i915_vma_unbind(vma);
			if (ret)
				return ret;
		}
	}

	if (i915_gem_obj_bound_any(obj)) {
		ret = i915_gem_object_finish_gpu(obj);
		if (ret)
			return ret;

		i915_gem_object_finish_gtt(obj);

		/* Before SandyBridge, you could not use tiling or fence
		 * registers with snooped memory, so relinquish any fences
		 * currently pointing to our region in the aperture.
		 */
		if (INTEL_INFO(dev)->gen < 6) {
			ret = i915_gem_object_put_fence(obj);
			if (ret)
				return ret;
		}

		list_for_each_entry(vma, &obj->vma_list, vma_link)
			if (drm_mm_node_allocated(&vma->node))
				vma->bind_vma(vma, cache_level,
					      obj->has_global_gtt_mapping ? GLOBAL_BIND : 0);
	}

	list_for_each_entry(vma, &obj->vma_list, vma_link)
		vma->node.color = cache_level;
	obj->cache_level = cache_level;

	if (cpu_write_needs_clflush(obj)) {
		u32 old_read_domains, old_write_domain;

		/* If we're coming from LLC cached, then we haven't
		 * actually been tracking whether the data is in the
		 * CPU cache or not, since we only allow one bit set
		 * in obj->write_domain and have been skipping the clflushes.
		 * Just set it to the CPU cache for now.
		 */
		WARN_ON(obj->base.write_domain & ~I915_GEM_DOMAIN_CPU);

		old_read_domains = obj->base.read_domains;
		old_write_domain = obj->base.write_domain;

		obj->base.read_domains = I915_GEM_DOMAIN_CPU;
		obj->base.write_domain = I915_GEM_DOMAIN_CPU;

		trace_i915_gem_object_change_domain(obj,
						    old_read_domains,
						    old_write_domain);
	}

	i915_gem_verify_gtt(dev);
	return 0;
}

int i915_gem_get_caching_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file)
{
	struct drm_i915_gem_caching *args = data;
	struct drm_i915_gem_object *obj;
	int ret;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	switch (obj->cache_level) {
	case I915_CACHE_LLC:
	case I915_CACHE_L3_LLC:
		args->caching = I915_CACHING_CACHED;
		break;

	case I915_CACHE_WT:
		args->caching = I915_CACHING_DISPLAY;
		break;

	default:
		args->caching = I915_CACHING_NONE;
		break;
	}

	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int i915_gem_set_caching_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file)
{
	struct drm_i915_gem_caching *args = data;
	struct drm_i915_gem_object *obj;
	enum i915_cache_level level;
	int ret;

	switch (args->caching) {
	case I915_CACHING_NONE:
		level = I915_CACHE_NONE;
		break;
	case I915_CACHING_CACHED:
		level = I915_CACHE_LLC;
		break;
	case I915_CACHING_DISPLAY:
		level = HAS_WT(dev) ? I915_CACHE_WT : I915_CACHE_NONE;
		break;
	default:
		return -EINVAL;
	}

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	ret = i915_gem_object_set_cache_level(obj, level);

	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

static bool is_pin_display(struct drm_i915_gem_object *obj)
{
	/* There are 3 sources that pin objects:
	 *   1. The display engine (scanouts, sprites, cursors);
	 *   2. Reservations for execbuffer;
	 *   3. The user.
	 *
	 * We can ignore reservations as we hold the struct_mutex and
	 * are only called outside of the reservation path.  The user
	 * can only increment pin_count once, and so if after
	 * subtracting the potential reference by the user, any pin_count
	 * remains, it must be due to another use by the display engine.
	 */
	return i915_gem_obj_to_ggtt(obj)->pin_count - !!obj->user_pin_count;
}

/*
 * Prepare buffer for display plane (scanout, cursors, etc).
 * Can be called from an uninterruptible phase (modesetting) and allows
 * any flushes to be pipelined (for pageflips).
 */
int
i915_gem_object_pin_to_display_plane(struct drm_i915_gem_object *obj,
				     u32 alignment,
				     struct intel_ring_buffer *pipelined)
{
	u32 old_read_domains, old_write_domain;
	int ret;

	if (pipelined != obj->ring) {
		ret = i915_gem_object_sync(obj, pipelined);
		if (ret)
			return ret;
	}

	/* Mark the pin_display early so that we account for the
	 * display coherency whilst setting up the cache domains.
	 */
	obj->pin_display = true;

	/* The display engine is not coherent with the LLC cache on gen6.  As
	 * a result, we make sure that the pinning that is about to occur is
	 * done with uncached PTEs. This is lowest common denominator for all
	 * chipsets.
	 *
	 * However for gen6+, we could do better by using the GFDT bit instead
	 * of uncaching, which would allow us to flush all the LLC-cached data
	 * with that bit in the PTE to main memory with just one PIPE_CONTROL.
	 */
	ret = i915_gem_object_set_cache_level(obj,
					      HAS_WT(obj->base.dev) ? I915_CACHE_WT : I915_CACHE_NONE);
	if (ret)
		goto err_unpin_display;

	/* As the user may map the buffer once pinned in the display plane
	 * (e.g. libkms for the bootup splash), we have to ensure that we
	 * always use map_and_fenceable for all scanout buffers.
	 */
	ret = i915_gem_obj_ggtt_pin(obj, alignment, PIN_MAPPABLE);
	if (ret)
		goto err_unpin_display;

	i915_gem_object_flush_cpu_write_domain(obj, true);

	old_write_domain = obj->base.write_domain;
	old_read_domains = obj->base.read_domains;

	/* It should now be out of any other write domains, and we can update
	 * the domain values for our changes.
	 */
	obj->base.write_domain = 0;
	obj->base.read_domains |= I915_GEM_DOMAIN_GTT;

	trace_i915_gem_object_change_domain(obj,
					    old_read_domains,
					    old_write_domain);

	return 0;

err_unpin_display:
	obj->pin_display = is_pin_display(obj);
	return ret;
}

void
i915_gem_object_unpin_from_display_plane(struct drm_i915_gem_object *obj)
{
	i915_gem_object_ggtt_unpin(obj);
	obj->pin_display = is_pin_display(obj);
}

int
i915_gem_object_finish_gpu(struct drm_i915_gem_object *obj)
{
	int ret;

	if ((obj->base.read_domains & I915_GEM_GPU_DOMAINS) == 0)
		return 0;

	ret = i915_gem_object_wait_rendering(obj, false);
	if (ret)
		return ret;

	/* Ensure that we invalidate the GPU's caches and TLBs. */
	obj->base.read_domains &= ~I915_GEM_GPU_DOMAINS;
	return 0;
}

/**
 * Moves a single object to the CPU read, and possibly write domain.
 *
 * This function returns when the move is complete, including waiting on
 * flushes to occur.
 */
int
i915_gem_object_set_to_cpu_domain(struct drm_i915_gem_object *obj, bool write)
{
	uint32_t old_write_domain, old_read_domains;
	int ret;

	if (obj->base.write_domain == I915_GEM_DOMAIN_CPU)
		return 0;

	ret = i915_gem_object_wait_rendering(obj, !write);
	if (ret)
		return ret;

	i915_gem_object_flush_gtt_write_domain(obj);

	old_write_domain = obj->base.write_domain;
	old_read_domains = obj->base.read_domains;

	/* Flush the CPU cache if it's still invalid. */
	if ((obj->base.read_domains & I915_GEM_DOMAIN_CPU) == 0) {
		i915_gem_clflush_object(obj, false);

		obj->base.read_domains |= I915_GEM_DOMAIN_CPU;
	}

	/* It should now be out of any other write domains, and we can update
	 * the domain values for our changes.
	 */
	BUG_ON((obj->base.write_domain & ~I915_GEM_DOMAIN_CPU) != 0);

	/* If we're writing through the CPU, then the GPU read domains will
	 * need to be invalidated at next use.
	 */
	if (write) {
		obj->base.read_domains = I915_GEM_DOMAIN_CPU;
		obj->base.write_domain = I915_GEM_DOMAIN_CPU;
	}

	trace_i915_gem_object_change_domain(obj,
					    old_read_domains,
					    old_write_domain);

	return 0;
}

/* Throttle our rendering by waiting until the ring has completed our requests
 * emitted over 20 msec ago.
 *
 * Note that if we were to use the current jiffies each time around the loop,
 * we wouldn't escape the function with any frames outstanding if the time to
 * render a frame was over 20ms.
 *
 * This should get us reasonable parallelism between CPU and GPU but also
 * relatively low latency when blocking on a particular request to finish.
 */
static int
i915_gem_ring_throttle(struct drm_device *dev, struct drm_file *file)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_file_private *file_priv = file->driver_priv;
	unsigned long recent_enough = jiffies - msecs_to_jiffies(20);
	struct drm_i915_gem_request *request;
	struct intel_ring_buffer *ring = NULL;
	unsigned reset_counter;
	u32 seqno = 0;
	int ret;

	ret = i915_gem_wait_for_error(&dev_priv->gpu_error);
	if (ret)
		return ret;

	ret = i915_gem_check_wedge(&dev_priv->gpu_error, false);
	if (ret)
		return ret;

	spin_lock(&file_priv->mm.lock);
	list_for_each_entry(request, &file_priv->mm.request_list, client_list) {
		if (time_after_eq(request->emitted_jiffies, recent_enough))
			break;

		ring = request->ring;
		seqno = request->seqno;
	}
	reset_counter = atomic_read(&dev_priv->gpu_error.reset_counter);
	spin_unlock(&file_priv->mm.lock);

	if (seqno == 0)
		return 0;

	ret = __wait_seqno(ring, seqno, reset_counter, true, NULL, NULL);
	if (ret == 0)
		queue_delayed_work(dev_priv->wq, &dev_priv->mm.retire_work, 0);

	return ret;
}

static bool
i915_vma_misplaced(struct i915_vma *vma, uint32_t alignment, uint64_t flags)
{
	struct drm_i915_gem_object *obj = vma->obj;

	if (alignment &&
	    vma->node.start & (alignment - 1))
		return true;

	if (flags & PIN_MAPPABLE && !obj->map_and_fenceable)
		return true;

	if (flags & PIN_OFFSET_BIAS &&
	    vma->node.start < (flags & PIN_OFFSET_MASK))
		return true;

	return false;
}

int
i915_gem_object_pin(struct drm_i915_gem_object *obj,
		    struct i915_address_space *vm,
		    uint32_t alignment,
		    uint64_t flags)
{
	struct i915_vma *vma;
	int ret;

	if (WARN_ON(flags & (PIN_GLOBAL | PIN_MAPPABLE) && !i915_is_ggtt(vm)))
		return -EINVAL;

	vma = i915_gem_obj_to_vma(obj, vm);
	if (vma) {
		if (WARN_ON(vma->pin_count == DRM_I915_GEM_OBJECT_MAX_PIN_COUNT))
			return -EBUSY;

		if (i915_vma_misplaced(vma, alignment, flags)) {
			WARN(vma->pin_count,
			     "bo is already pinned with incorrect alignment:"
			     " offset=%lx, req.alignment=%x, req.map_and_fenceable=%d,"
			     " obj->map_and_fenceable=%d\n",
			     i915_gem_obj_offset(obj, vm), alignment,
			     !!(flags & PIN_MAPPABLE),
			     obj->map_and_fenceable);
			ret = i915_vma_unbind(vma);
			if (ret)
				return ret;

			vma = NULL;
		}
	}

	if (vma == NULL || !drm_mm_node_allocated(&vma->node)) {
		vma = i915_gem_object_bind_to_vm(obj, vm, alignment, flags);
		if (IS_ERR(vma))
			return PTR_ERR(vma);
	}

	if (flags & PIN_GLOBAL && !obj->has_global_gtt_mapping)
		vma->bind_vma(vma, obj->cache_level, GLOBAL_BIND);

	vma->pin_count++;
	if (flags & PIN_MAPPABLE)
		obj->pin_mappable |= true;

	return 0;
}

void
i915_gem_object_ggtt_unpin(struct drm_i915_gem_object *obj)
{
	struct i915_vma *vma = i915_gem_obj_to_ggtt(obj);

	BUG_ON(!vma);
	BUG_ON(vma->pin_count == 0);
	BUG_ON(!i915_gem_obj_ggtt_bound(obj));

	if (--vma->pin_count == 0)
		obj->pin_mappable = false;
}

int
i915_gem_pin_ioctl(struct drm_device *dev, void *data,
		   struct drm_file *file)
{
	struct drm_i915_gem_pin *args = data;
	struct drm_i915_gem_object *obj;
	int ret;

	if (INTEL_INFO(dev)->gen >= 6)
		return -ENODEV;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	if (obj->madv != I915_MADV_WILLNEED) {
		DRM_DEBUG("Attempting to pin a purgeable buffer\n");
		ret = -EFAULT;
		goto out;
	}

	if (obj->pin_filp != NULL && obj->pin_filp != file) {
		DRM_DEBUG("Already pinned in i915_gem_pin_ioctl(): %d\n",
			  args->handle);
		ret = -EINVAL;
		goto out;
	}

	if (obj->user_pin_count == ULONG_MAX) {
		ret = -EBUSY;
		goto out;
	}

	if (obj->user_pin_count == 0) {
		ret = i915_gem_obj_ggtt_pin(obj, args->alignment, PIN_MAPPABLE);
		if (ret)
			goto out;
	}

	obj->user_pin_count++;
	obj->pin_filp = file;

	args->offset = i915_gem_obj_ggtt_offset(obj);
out:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int
i915_gem_unpin_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *file)
{
	struct drm_i915_gem_pin *args = data;
	struct drm_i915_gem_object *obj;
	int ret;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	if (obj->pin_filp != file) {
		DRM_DEBUG("Not pinned by caller in i915_gem_pin_ioctl(): %d\n",
			  args->handle);
		ret = -EINVAL;
		goto out;
	}
	obj->user_pin_count--;
	if (obj->user_pin_count == 0) {
		obj->pin_filp = NULL;
		i915_gem_object_ggtt_unpin(obj);
	}

out:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int
i915_gem_busy_ioctl(struct drm_device *dev, void *data,
		    struct drm_file *file)
{
	struct drm_i915_gem_busy *args = data;
	struct drm_i915_gem_object *obj;
	int ret;

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	/* Count all active objects as busy, even if they are currently not used
	 * by the gpu. Users of this interface expect objects to eventually
	 * become non-busy without any further actions, therefore emit any
	 * necessary flushes here.
	 */
	ret = i915_gem_object_flush_active(obj);

	args->busy = obj->active;
	if (obj->ring) {
		BUILD_BUG_ON(I915_NUM_RINGS > 16);
		args->busy |= intel_ring_flag(obj->ring) << 16;
	}

	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int
i915_gem_throttle_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	return i915_gem_ring_throttle(dev, file_priv);
}

int
i915_gem_madvise_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_i915_gem_madvise *args = data;
	struct drm_i915_gem_object *obj;
	int ret;

	switch (args->madv) {
	case I915_MADV_DONTNEED:
	case I915_MADV_WILLNEED:
	    break;
	default:
	    return -EINVAL;
	}

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		return ret;

	obj = to_intel_bo(drm_gem_object_lookup(dev, file_priv, args->handle));
	if (&obj->base == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	if (i915_gem_obj_is_pinned(obj)) {
		ret = -EINVAL;
		goto out;
	}

	if (obj->madv != __I915_MADV_PURGED)
		obj->madv = args->madv;

	/* if the object is no longer attached, discard its backing storage */
	if (i915_gem_object_is_purgeable(obj) && obj->pages == NULL)
		i915_gem_object_truncate(obj);

	args->retained = obj->madv != __I915_MADV_PURGED;

out:
	drm_gem_object_unreference(&obj->base);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

void i915_gem_object_init(struct drm_i915_gem_object *obj,
			  const struct drm_i915_gem_object_ops *ops)
{
	INIT_LIST_HEAD(&obj->global_list);
	INIT_LIST_HEAD(&obj->ring_list);
	INIT_LIST_HEAD(&obj->obj_exec_link);
	INIT_LIST_HEAD(&obj->vma_list);

	obj->ops = ops;

	obj->fence_reg = I915_FENCE_REG_NONE;
	obj->madv = I915_MADV_WILLNEED;
	/* Avoid an unnecessary call to unbind on the first bind. */
	obj->map_and_fenceable = true;

	i915_gem_info_add_obj(obj->base.dev->dev_private, obj->base.size);
}

static const struct drm_i915_gem_object_ops i915_gem_object_ops = {
	.get_pages = i915_gem_object_get_pages_gtt,
	.put_pages = i915_gem_object_put_pages_gtt,
};

struct drm_i915_gem_object *i915_gem_alloc_object(struct drm_device *dev,
						  size_t size)
{
#ifdef __NetBSD__
	struct drm_i915_private *const dev_priv = dev->dev_private;
#endif
	struct drm_i915_gem_object *obj;
#ifndef __NetBSD__
	struct address_space *mapping;
	gfp_t mask;
#endif

	obj = i915_gem_object_alloc(dev);
	if (obj == NULL)
		return NULL;

	if (drm_gem_object_init(dev, &obj->base, size) != 0) {
		i915_gem_object_free(obj);
		return NULL;
	}

#ifdef __NetBSD__
	uao_set_pgfl(obj->base.gemo_shm_uao, dev_priv->gtt.pgfl);
#else
	mask = GFP_HIGHUSER | __GFP_RECLAIMABLE;
	if (IS_CRESTLINE(dev) || IS_BROADWATER(dev)) {
		/* 965gm cannot relocate objects above 4GiB. */
		mask &= ~__GFP_HIGHMEM;
		mask |= __GFP_DMA32;
	}

	mapping = file_inode(obj->base.filp)->i_mapping;
	mapping_set_gfp_mask(mapping, mask);
#endif

	i915_gem_object_init(obj, &i915_gem_object_ops);

	obj->base.write_domain = I915_GEM_DOMAIN_CPU;
	obj->base.read_domains = I915_GEM_DOMAIN_CPU;

	if (HAS_LLC(dev)) {
		/* On some devices, we can have the GPU use the LLC (the CPU
		 * cache) for about a 10% performance improvement
		 * compared to uncached.  Graphics requests other than
		 * display scanout are coherent with the CPU in
		 * accessing this cache.  This means in this mode we
		 * don't need to clflush on the CPU side, and on the
		 * GPU side we only need to flush internal caches to
		 * get data visible to the CPU.
		 *
		 * However, we maintain the display planes as UC, and so
		 * need to rebind when first used as such.
		 */
		obj->cache_level = I915_CACHE_LLC;
	} else
		obj->cache_level = I915_CACHE_NONE;

	trace_i915_gem_object_create(obj);

	return obj;
}

void i915_gem_free_object(struct drm_gem_object *gem_obj)
{
	struct drm_i915_gem_object *obj = to_intel_bo(gem_obj);
	struct drm_device *dev = obj->base.dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct i915_vma *vma, *next;

	intel_runtime_pm_get(dev_priv);

	trace_i915_gem_object_destroy(obj);

	list_for_each_entry_safe(vma, next, &obj->vma_list, vma_link) {
		int ret;

		vma->pin_count = 0;
		ret = i915_vma_unbind(vma);
		if (WARN_ON(ret == -ERESTARTSYS)) {
			bool was_interruptible;

			was_interruptible = dev_priv->mm.interruptible;
			dev_priv->mm.interruptible = false;

			WARN_ON(i915_vma_unbind(vma));

			dev_priv->mm.interruptible = was_interruptible;
		}
	}

	i915_gem_object_detach_phys(obj);

	/* Stolen objects don't hold a ref, but do hold pin count. Fix that up
	 * before progressing. */
	if (obj->stolen)
		i915_gem_object_unpin_pages(obj);

	if (WARN_ON(obj->pages_pin_count))
		obj->pages_pin_count = 0;
	i915_gem_object_put_pages(obj);
	i915_gem_object_free_mmap_offset(obj);
	i915_gem_object_release_stolen(obj);

	BUG_ON(obj->pages);

#ifndef __NetBSD__		/* XXX drm prime */
	if (obj->base.import_attach)
		drm_prime_gem_destroy(&obj->base, NULL);
#endif

	drm_gem_object_release(&obj->base);
	i915_gem_info_remove_obj(dev_priv, obj->base.size);

	kfree(obj->bit_17);
	i915_gem_object_free(obj);

	intel_runtime_pm_put(dev_priv);
}

struct i915_vma *i915_gem_obj_to_vma(struct drm_i915_gem_object *obj,
				     struct i915_address_space *vm)
{
	struct i915_vma *vma;
	list_for_each_entry(vma, &obj->vma_list, vma_link)
		if (vma->vm == vm)
			return vma;

	return NULL;
}

void i915_gem_vma_destroy(struct i915_vma *vma)
{
	WARN_ON(vma->node.allocated);

	/* Keep the vma as a placeholder in the execbuffer reservation lists */
	if (!list_empty(&vma->exec_list))
		return;

	list_del(&vma->vma_link);

	kfree(vma);
}

int
i915_gem_suspend(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret = 0;

	mutex_lock(&dev->struct_mutex);
	if (dev_priv->ums.mm_suspended)
		goto err;

	ret = i915_gpu_idle(dev);
	if (ret)
		goto err;

	i915_gem_retire_requests(dev);

	/* Under UMS, be paranoid and evict. */
	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		i915_gem_evict_everything(dev);

	i915_kernel_lost_context(dev);
	i915_gem_cleanup_ringbuffer(dev);

	/* Hack!  Don't let anybody do execbuf while we don't control the chip.
	 * We need to replace this with a semaphore, or something.
	 * And not confound ums.mm_suspended!
	 */
	dev_priv->ums.mm_suspended = !drm_core_check_feature(dev,
							     DRIVER_MODESET);
	mutex_unlock(&dev->struct_mutex);

	del_timer_sync(&dev_priv->gpu_error.hangcheck_timer);
	cancel_delayed_work_sync(&dev_priv->mm.retire_work);
	cancel_delayed_work_sync(&dev_priv->mm.idle_work);

	return 0;

err:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int i915_gem_l3_remap(struct intel_ring_buffer *ring, int slice)
{
	struct drm_device *dev = ring->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 reg_base = GEN7_L3LOG_BASE + (slice * 0x200);
	u32 *remap_info = dev_priv->l3_parity.remap_info[slice];
	int i, ret;

	if (!HAS_L3_DPF(dev) || !remap_info)
		return 0;

	ret = intel_ring_begin(ring, GEN7_L3LOG_SIZE / 4 * 3);
	if (ret)
		return ret;

	/*
	 * Note: We do not worry about the concurrent register cacheline hang
	 * here because no other code should access these registers other than
	 * at initialization time.
	 */
	for (i = 0; i < GEN7_L3LOG_SIZE; i += 4) {
		intel_ring_emit(ring, MI_LOAD_REGISTER_IMM(1));
		intel_ring_emit(ring, reg_base + i);
		intel_ring_emit(ring, remap_info[i/4]);
	}

	intel_ring_advance(ring);

	return ret;
}

void i915_gem_init_swizzling(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;

	if (INTEL_INFO(dev)->gen < 5 ||
	    dev_priv->mm.bit_6_swizzle_x == I915_BIT_6_SWIZZLE_NONE)
		return;

	I915_WRITE(DISP_ARB_CTL, I915_READ(DISP_ARB_CTL) |
				 DISP_TILE_SURFACE_SWIZZLING);

	if (IS_GEN5(dev))
		return;

	I915_WRITE(TILECTL, I915_READ(TILECTL) | TILECTL_SWZCTL);
	if (IS_GEN6(dev))
		I915_WRITE(ARB_MODE, _MASKED_BIT_ENABLE(ARB_MODE_SWIZZLE_SNB));
	else if (IS_GEN7(dev))
		I915_WRITE(ARB_MODE, _MASKED_BIT_ENABLE(ARB_MODE_SWIZZLE_IVB));
	else if (IS_GEN8(dev))
		I915_WRITE(GAMTARBMODE, _MASKED_BIT_ENABLE(ARB_MODE_SWIZZLE_BDW));
	else
		BUG();
}

static bool
intel_enable_blt(struct drm_device *dev)
{
	if (!HAS_BLT(dev))
		return false;

	/* The blitter was dysfunctional on early prototypes */
	if (IS_GEN6(dev) && dev->pdev->revision < 8) {
		DRM_INFO("BLT not supported on this pre-production hardware;"
			 " graphics performance will be degraded.\n");
		return false;
	}

	return true;
}

static int i915_gem_init_rings(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	ret = intel_init_render_ring_buffer(dev);
	if (ret)
		return ret;

	if (HAS_BSD(dev)) {
		ret = intel_init_bsd_ring_buffer(dev);
		if (ret)
			goto cleanup_render_ring;
	}

	if (intel_enable_blt(dev)) {
		ret = intel_init_blt_ring_buffer(dev);
		if (ret)
			goto cleanup_bsd_ring;
	}

	if (HAS_VEBOX(dev)) {
		ret = intel_init_vebox_ring_buffer(dev);
		if (ret)
			goto cleanup_blt_ring;
	}


	ret = i915_gem_set_seqno(dev, ((u32)~0 - 0x1000));
	if (ret)
		goto cleanup_vebox_ring;

	return 0;

cleanup_vebox_ring:
	intel_cleanup_ring_buffer(&dev_priv->ring[VECS]);
cleanup_blt_ring:
	intel_cleanup_ring_buffer(&dev_priv->ring[BCS]);
cleanup_bsd_ring:
	intel_cleanup_ring_buffer(&dev_priv->ring[VCS]);
cleanup_render_ring:
	intel_cleanup_ring_buffer(&dev_priv->ring[RCS]);

	return ret;
}

int
i915_gem_init_hw(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret, i;

	if (INTEL_INFO(dev)->gen < 6 && !intel_enable_gtt())
		return -EIO;

	if (dev_priv->ellc_size)
		I915_WRITE(HSW_IDICR, I915_READ(HSW_IDICR) | IDIHASHMSK(0xf));

	if (IS_HASWELL(dev))
		I915_WRITE(MI_PREDICATE_RESULT_2, IS_HSW_GT3(dev) ?
			   LOWER_SLICE_ENABLED : LOWER_SLICE_DISABLED);

	if (HAS_PCH_NOP(dev)) {
		if (IS_IVYBRIDGE(dev)) {
			u32 temp = I915_READ(GEN7_MSG_CTL);
			temp &= ~(WAIT_FOR_PCH_FLR_ACK | WAIT_FOR_PCH_RESET_ACK);
			I915_WRITE(GEN7_MSG_CTL, temp);
		} else if (INTEL_INFO(dev)->gen >= 7) {
			u32 temp = I915_READ(HSW_NDE_RSTWRN_OPT);
			temp &= ~RESET_PCH_HANDSHAKE_ENABLE;
			I915_WRITE(HSW_NDE_RSTWRN_OPT, temp);
		}
	}

	i915_gem_init_swizzling(dev);

	ret = i915_gem_init_rings(dev);
	if (ret)
		return ret;

	for (i = 0; i < NUM_L3_SLICES(dev); i++)
		i915_gem_l3_remap(&dev_priv->ring[RCS], i);

	/*
	 * XXX: Contexts should only be initialized once. Doing a switch to the
	 * default context switch however is something we'd like to do after
	 * reset or thaw (the latter may not actually be necessary for HW, but
	 * goes with our code better). Context switching requires rings (for
	 * the do_switch), but before enabling PPGTT. So don't move this.
	 */
	ret = i915_gem_context_enable(dev_priv);
	if (ret) {
		DRM_ERROR("Context enable failed %d\n", ret);
		goto err_out;
	}

	return 0;

err_out:
	i915_gem_cleanup_ringbuffer(dev);
	return ret;
}

int i915_gem_init(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	mutex_lock(&dev->struct_mutex);

	if (IS_VALLEYVIEW(dev)) {
		/* VLVA0 (potential hack), BIOS isn't actually waking us */
		I915_WRITE(VLV_GTLC_WAKE_CTRL, 1);
		if (wait_for((I915_READ(VLV_GTLC_PW_STATUS) & 1) == 1, 10))
			DRM_DEBUG_DRIVER("allow wake ack timed out\n");
	}
	i915_gem_init_global_gtt(dev);

	ret = i915_gem_context_init(dev);
	if (ret) {
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}

	ret = i915_gem_init_hw(dev);
	mutex_unlock(&dev->struct_mutex);
	if (ret) {
		WARN_ON(dev_priv->mm.aliasing_ppgtt);
		i915_gem_context_fini(dev);
		drm_mm_takedown(&dev_priv->gtt.base.mm);
		return ret;
	}

	/* Allow hardware batchbuffers unless told otherwise, but not for KMS. */
	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		dev_priv->dri1.allow_batchbuffer = 1;
	return 0;
}

void
i915_gem_cleanup_ringbuffer(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_ring_buffer *ring;
	int i;

	for_each_ring(ring, dev_priv, i)
		intel_cleanup_ring_buffer(ring);
}

int
i915_gem_entervt_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return 0;

	if (i915_reset_in_progress(&dev_priv->gpu_error)) {
		DRM_ERROR("Reenabling wedged hardware, good luck\n");
		atomic_set(&dev_priv->gpu_error.reset_counter, 0);
	}

	mutex_lock(&dev->struct_mutex);
	dev_priv->ums.mm_suspended = 0;

	ret = i915_gem_init_hw(dev);
	if (ret != 0) {
		mutex_unlock(&dev->struct_mutex);
		return ret;
	}

	BUG_ON(!list_empty(&dev_priv->gtt.base.active_list));
	mutex_unlock(&dev->struct_mutex);

	ret = drm_irq_install(dev);
	if (ret)
		goto cleanup_ringbuffer;

	return 0;

cleanup_ringbuffer:
	mutex_lock(&dev->struct_mutex);
	i915_gem_cleanup_ringbuffer(dev);
	dev_priv->ums.mm_suspended = 1;
	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int
i915_gem_leavevt_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return 0;

	drm_irq_uninstall(dev);

	return i915_gem_suspend(dev);
}

void
i915_gem_lastclose(struct drm_device *dev)
{
	int ret;

	if (drm_core_check_feature(dev, DRIVER_MODESET))
		return;

	ret = i915_gem_suspend(dev);
	if (ret)
		DRM_ERROR("failed to idle hardware: %d\n", ret);
}

static void
init_ring_lists(struct intel_ring_buffer *ring)
{
	INIT_LIST_HEAD(&ring->active_list);
	INIT_LIST_HEAD(&ring->request_list);
}

void i915_init_vm(struct drm_i915_private *dev_priv,
		  struct i915_address_space *vm)
{
	if (!i915_is_ggtt(vm))
		drm_mm_init(&vm->mm, vm->start, vm->total);
	vm->dev = dev_priv->dev;
	INIT_LIST_HEAD(&vm->active_list);
	INIT_LIST_HEAD(&vm->inactive_list);
	INIT_LIST_HEAD(&vm->global_link);
	list_add_tail(&vm->global_link, &dev_priv->vm_list);
}

void
i915_gem_load(struct drm_device *dev)
{
	struct drm_i915_private *dev_priv = dev->dev_private;
	int i;

	dev_priv->slab =
		kmem_cache_create("i915_gem_object",
				  sizeof(struct drm_i915_gem_object), 0,
				  SLAB_HWCACHE_ALIGN,
				  NULL);

	INIT_LIST_HEAD(&dev_priv->vm_list);
	i915_init_vm(dev_priv, &dev_priv->gtt.base);

	INIT_LIST_HEAD(&dev_priv->context_list);
	INIT_LIST_HEAD(&dev_priv->mm.unbound_list);
	INIT_LIST_HEAD(&dev_priv->mm.bound_list);
	INIT_LIST_HEAD(&dev_priv->mm.fence_list);
	for (i = 0; i < I915_NUM_RINGS; i++)
		init_ring_lists(&dev_priv->ring[i]);
	for (i = 0; i < I915_MAX_NUM_FENCES; i++)
		INIT_LIST_HEAD(&dev_priv->fence_regs[i].lru_list);
	INIT_DELAYED_WORK(&dev_priv->mm.retire_work,
			  i915_gem_retire_work_handler);
	INIT_DELAYED_WORK(&dev_priv->mm.idle_work,
			  i915_gem_idle_work_handler);
#ifdef __NetBSD__
	spin_lock_init(&dev_priv->gpu_error.reset_lock);
	DRM_INIT_WAITQUEUE(&dev_priv->gpu_error.reset_queue, "i915errst");
#else
	init_waitqueue_head(&dev_priv->gpu_error.reset_queue);
#endif

	/* On GEN3 we really need to make sure the ARB C3 LP bit is set */
	if (IS_GEN3(dev)) {
		I915_WRITE(MI_ARB_STATE,
			   _MASKED_BIT_ENABLE(MI_ARB_C3_LP_WRITE_ENABLE));
	}

	dev_priv->relative_constants_mode = I915_EXEC_CONSTANTS_REL_GENERAL;

	/* Old X drivers will take 0-2 for front, back, depth buffers */
	if (!drm_core_check_feature(dev, DRIVER_MODESET))
		dev_priv->fence_reg_start = 3;

	if (INTEL_INFO(dev)->gen >= 7 && !IS_VALLEYVIEW(dev))
		dev_priv->num_fence_regs = 32;
	else if (INTEL_INFO(dev)->gen >= 4 || IS_I945G(dev) || IS_I945GM(dev) || IS_G33(dev))
		dev_priv->num_fence_regs = 16;
	else
		dev_priv->num_fence_regs = 8;

	/* Initialize fence registers to zero */
	INIT_LIST_HEAD(&dev_priv->mm.fence_list);
	i915_gem_restore_fences(dev);

	i915_gem_detect_bit_6_swizzle(dev);
#ifdef __NetBSD__
	DRM_INIT_WAITQUEUE(&dev_priv->pending_flip_queue, "i915flip");
	spin_lock_init(&dev_priv->pending_flip_lock);
#else
	init_waitqueue_head(&dev_priv->pending_flip_queue);
#endif

	dev_priv->mm.interruptible = true;

	dev_priv->mm.inactive_shrinker.scan_objects = i915_gem_inactive_scan;
	dev_priv->mm.inactive_shrinker.count_objects = i915_gem_inactive_count;
	dev_priv->mm.inactive_shrinker.seeks = DEFAULT_SEEKS;
	register_shrinker(&dev_priv->mm.inactive_shrinker);
}

void i915_gem_release(struct drm_device *dev, struct drm_file *file)
{
	struct drm_i915_file_private *file_priv = file->driver_priv;

	cancel_delayed_work_sync(&file_priv->mm.idle_work);

	/* Clean up our request list when the client is going away, so that
	 * later retire_requests won't dereference our soon-to-be-gone
	 * file_priv.
	 */
	spin_lock(&file_priv->mm.lock);
	while (!list_empty(&file_priv->mm.request_list)) {
		struct drm_i915_gem_request *request;

		request = list_first_entry(&file_priv->mm.request_list,
					   struct drm_i915_gem_request,
					   client_list);
		list_del(&request->client_list);
		request->file_priv = NULL;
	}
	spin_unlock(&file_priv->mm.lock);
}

static void
i915_gem_file_idle_work_handler(struct work_struct *work)
{
	struct drm_i915_file_private *file_priv =
		container_of(work, typeof(*file_priv), mm.idle_work.work);

	atomic_set(&file_priv->rps_wait_boost, false);
}

int i915_gem_open(struct drm_device *dev, struct drm_file *file)
{
	struct drm_i915_file_private *file_priv;
	int ret;

	DRM_DEBUG_DRIVER("\n");

	file_priv = kzalloc(sizeof(*file_priv), GFP_KERNEL);
	if (!file_priv)
		return -ENOMEM;

	file->driver_priv = file_priv;
	file_priv->dev_priv = dev->dev_private;
	file_priv->file = file;

	spin_lock_init(&file_priv->mm.lock);
	INIT_LIST_HEAD(&file_priv->mm.request_list);
	INIT_DELAYED_WORK(&file_priv->mm.idle_work,
			  i915_gem_file_idle_work_handler);

	ret = i915_gem_context_open(dev, file);
	if (ret)
		kfree(file_priv);

	return ret;
}

#ifndef __NetBSD__
static bool mutex_is_locked_by(struct mutex *mutex, struct task_struct *task)
{
	if (!mutex_is_locked(mutex))
		return false;

#if defined(CONFIG_SMP) || defined(CONFIG_DEBUG_MUTEXES)
	return mutex->owner == task;
#else
	/* Since UP may be pre-empted, we cannot assume that we own the lock */
	return false;
#endif
}
#endif

static unsigned long
i915_gem_inactive_count(struct shrinker *shrinker, struct shrink_control *sc)
{
#ifdef __NetBSD__		/* XXX shrinkers */
	return 0;
#else
	struct drm_i915_private *dev_priv =
		container_of(shrinker,
			     struct drm_i915_private,
			     mm.inactive_shrinker);
	struct drm_device *dev = dev_priv->dev;
	struct drm_i915_gem_object *obj;
	bool unlock = true;
	unsigned long count;

	if (!mutex_trylock(&dev->struct_mutex)) {
		if (!mutex_is_locked_by(&dev->struct_mutex, current))
			return 0;

		if (dev_priv->mm.shrinker_no_lock_stealing)
			return 0;

		unlock = false;
	}

	count = 0;
	list_for_each_entry(obj, &dev_priv->mm.unbound_list, global_list)
		if (obj->pages_pin_count == 0)
			count += obj->base.size >> PAGE_SHIFT;

	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_list) {
		if (obj->active)
			continue;

		if (!i915_gem_obj_is_pinned(obj) && obj->pages_pin_count == 0)
			count += obj->base.size >> PAGE_SHIFT;
	}

	if (unlock)
		mutex_unlock(&dev->struct_mutex);

	return count;
#endif
}

/* All the new VM stuff */
unsigned long i915_gem_obj_offset(struct drm_i915_gem_object *o,
				  struct i915_address_space *vm)
{
	struct drm_i915_private *dev_priv = o->base.dev->dev_private;
	struct i915_vma *vma;

	if (!dev_priv->mm.aliasing_ppgtt ||
	    vm == &dev_priv->mm.aliasing_ppgtt->base)
		vm = &dev_priv->gtt.base;

	BUG_ON(list_empty(&o->vma_list));
	list_for_each_entry(vma, &o->vma_list, vma_link) {
		if (vma->vm == vm)
			return vma->node.start;

	}
	return -1;
}

bool i915_gem_obj_bound(struct drm_i915_gem_object *o,
			struct i915_address_space *vm)
{
	struct i915_vma *vma;

	list_for_each_entry(vma, &o->vma_list, vma_link)
		if (vma->vm == vm && drm_mm_node_allocated(&vma->node))
			return true;

	return false;
}

bool i915_gem_obj_bound_any(struct drm_i915_gem_object *o)
{
	struct i915_vma *vma;

	list_for_each_entry(vma, &o->vma_list, vma_link)
		if (drm_mm_node_allocated(&vma->node))
			return true;

	return false;
}

unsigned long i915_gem_obj_size(struct drm_i915_gem_object *o,
				struct i915_address_space *vm)
{
	struct drm_i915_private *dev_priv = o->base.dev->dev_private;
	struct i915_vma *vma;

	if (!dev_priv->mm.aliasing_ppgtt ||
	    vm == &dev_priv->mm.aliasing_ppgtt->base)
		vm = &dev_priv->gtt.base;

	BUG_ON(list_empty(&o->vma_list));

	list_for_each_entry(vma, &o->vma_list, vma_link)
		if (vma->vm == vm)
			return vma->node.size;

	return 0;
}

static unsigned long
i915_gem_inactive_scan(struct shrinker *shrinker, struct shrink_control *sc)
{
#ifdef __NetBSD__		/* XXX shrinkers */
	return 0;
#else
	struct drm_i915_private *dev_priv =
		container_of(shrinker,
			     struct drm_i915_private,
			     mm.inactive_shrinker);
	struct drm_device *dev = dev_priv->dev;
	unsigned long freed;
	bool unlock = true;

	if (!mutex_trylock(&dev->struct_mutex)) {
		if (!mutex_is_locked_by(&dev->struct_mutex, current))
			return SHRINK_STOP;

		if (dev_priv->mm.shrinker_no_lock_stealing)
			return SHRINK_STOP;

		unlock = false;
	}

	freed = i915_gem_purge(dev_priv, sc->nr_to_scan);
	if (freed < sc->nr_to_scan)
		freed += __i915_gem_shrink(dev_priv,
					   sc->nr_to_scan - freed,
					   false);
	if (freed < sc->nr_to_scan)
		freed += i915_gem_shrink_all(dev_priv);

	if (unlock)
		mutex_unlock(&dev->struct_mutex);

	return freed;
#endif
}

struct i915_vma *i915_gem_obj_to_ggtt(struct drm_i915_gem_object *obj)
{
	struct i915_vma *vma;

	if (WARN_ON(list_empty(&obj->vma_list)))
		return NULL;

	vma = list_first_entry(&obj->vma_list, typeof(*vma), vma_link);
	if (vma->vm != obj_to_ggtt(obj))
		return NULL;

	return vma;
}
