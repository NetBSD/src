/*
 * Copyright © 2008,2010 Intel Corporation
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
 *    Chris Wilson <chris@chris-wilson.co.uk>
 *
 */

#include <drm/drmP.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"
#include "i915_trace.h"
#include "intel_drv.h"
#include <linux/dma_remapping.h>

struct eb_objects {
	int and;
	struct hlist_head buckets[0];
};

static struct eb_objects *
eb_create(int size)
{
	struct eb_objects *eb;
	int count = PAGE_SIZE / sizeof(struct hlist_head) / 2;
	BUILD_BUG_ON(!is_power_of_2(PAGE_SIZE / sizeof(struct hlist_head)));
	while (count > size)
		count >>= 1;
	eb = kzalloc(count*sizeof(struct hlist_head) +
		     sizeof(struct eb_objects),
		     GFP_KERNEL);
	if (eb == NULL)
		return eb;

	eb->and = count - 1;
	return eb;
}

static void
eb_reset(struct eb_objects *eb)
{
	memset(eb->buckets, 0, (eb->and+1)*sizeof(struct hlist_head));
}

static void
eb_add_object(struct eb_objects *eb, struct drm_i915_gem_object *obj)
{
	hlist_add_head(&obj->exec_node,
		       &eb->buckets[obj->exec_handle & eb->and]);
}

static struct drm_i915_gem_object *
eb_get_object(struct eb_objects *eb, unsigned long handle)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct drm_i915_gem_object *obj;

	head = &eb->buckets[handle & eb->and];
	hlist_for_each(node, head) {
		obj = hlist_entry(node, struct drm_i915_gem_object, exec_node);
		if (obj->exec_handle == handle)
			return obj;
	}

	return NULL;
}

static void
eb_destroy(struct eb_objects *eb)
{
	kfree(eb);
}

static inline int use_cpu_reloc(struct drm_i915_gem_object *obj)
{
	return (obj->base.write_domain == I915_GEM_DOMAIN_CPU ||
		!obj->map_and_fenceable ||
		obj->cache_level != I915_CACHE_NONE);
}

static int
i915_gem_execbuffer_relocate_entry(struct drm_i915_gem_object *obj,
				   struct eb_objects *eb,
				   struct drm_i915_gem_relocation_entry *reloc)
{
	struct drm_device *dev = obj->base.dev;
	struct drm_gem_object *target_obj;
	struct drm_i915_gem_object *target_i915_obj;
	uint32_t target_offset;
	int ret = -EINVAL;

	/* we've already hold a reference to all valid objects */
	target_obj = &eb_get_object(eb, reloc->target_handle)->base;
	if (unlikely(target_obj == NULL))
		return -ENOENT;

	target_i915_obj = to_intel_bo(target_obj);
	target_offset = target_i915_obj->gtt_offset;

	/* Sandybridge PPGTT errata: We need a global gtt mapping for MI and
	 * pipe_control writes because the gpu doesn't properly redirect them
	 * through the ppgtt for non_secure batchbuffers. */
	if (unlikely(IS_GEN6(dev) &&
	    reloc->write_domain == I915_GEM_DOMAIN_INSTRUCTION &&
	    !target_i915_obj->has_global_gtt_mapping)) {
		i915_gem_gtt_bind_object(target_i915_obj,
					 target_i915_obj->cache_level);
	}

	/* Validate that the target is in a valid r/w GPU domain */
	if (unlikely(reloc->write_domain & (reloc->write_domain - 1))) {
		DRM_DEBUG("reloc with multiple write domains: "
			  "obj %p target %d offset %d "
			  "read %08x write %08x",
			  obj, reloc->target_handle,
			  (int) reloc->offset,
			  reloc->read_domains,
			  reloc->write_domain);
		return ret;
	}
	if (unlikely((reloc->write_domain | reloc->read_domains)
		     & ~I915_GEM_GPU_DOMAINS)) {
		DRM_DEBUG("reloc with read/write non-GPU domains: "
			  "obj %p target %d offset %d "
			  "read %08x write %08x",
			  obj, reloc->target_handle,
			  (int) reloc->offset,
			  reloc->read_domains,
			  reloc->write_domain);
		return ret;
	}
	if (unlikely(reloc->write_domain && target_obj->pending_write_domain &&
		     reloc->write_domain != target_obj->pending_write_domain)) {
		DRM_DEBUG("Write domain conflict: "
			  "obj %p target %d offset %d "
			  "new %08x old %08x\n",
			  obj, reloc->target_handle,
			  (int) reloc->offset,
			  reloc->write_domain,
			  target_obj->pending_write_domain);
		return ret;
	}

	target_obj->pending_read_domains |= reloc->read_domains;
	target_obj->pending_write_domain |= reloc->write_domain;

	/* If the relocation already has the right value in it, no
	 * more work needs to be done.
	 */
	if (target_offset == reloc->presumed_offset)
		return 0;

	/* Check that the relocation address is valid... */
	if (unlikely(reloc->offset > obj->base.size - 4)) {
		DRM_DEBUG("Relocation beyond object bounds: "
			  "obj %p target %d offset %d size %d.\n",
			  obj, reloc->target_handle,
			  (int) reloc->offset,
			  (int) obj->base.size);
		return ret;
	}
	if (unlikely(reloc->offset & 3)) {
		DRM_DEBUG("Relocation not 4-byte aligned: "
			  "obj %p target %d offset %d.\n",
			  obj, reloc->target_handle,
			  (int) reloc->offset);
		return ret;
	}

	/* We can't wait for rendering with pagefaults disabled */
	if (obj->active && in_atomic())
		return -EFAULT;

	reloc->delta += target_offset;
	if (use_cpu_reloc(obj)) {
		uint32_t page_offset = reloc->offset & ~PAGE_MASK;
		char *vaddr;

		ret = i915_gem_object_set_to_cpu_domain(obj, 1);
		if (ret)
			return ret;

		vaddr = kmap_atomic(i915_gem_object_get_page(obj,
							     reloc->offset >> PAGE_SHIFT));
		*(uint32_t *)(vaddr + page_offset) = reloc->delta;
		kunmap_atomic(vaddr);
	} else {
		struct drm_i915_private *dev_priv = dev->dev_private;
		uint32_t __iomem *reloc_entry;
		void __iomem *reloc_page;

		ret = i915_gem_object_set_to_gtt_domain(obj, true);
		if (ret)
			return ret;

		ret = i915_gem_object_put_fence(obj);
		if (ret)
			return ret;

		/* Map the page containing the relocation we're going to perform.  */
		reloc->offset += obj->gtt_offset;
		reloc_page = io_mapping_map_atomic_wc(dev_priv->mm.gtt_mapping,
						      reloc->offset & PAGE_MASK);
		reloc_entry = (uint32_t __iomem *)
			(reloc_page + (reloc->offset & ~PAGE_MASK));
		iowrite32(reloc->delta, reloc_entry);
		io_mapping_unmap_atomic(reloc_page);
	}

	/* and update the user's relocation entry */
	reloc->presumed_offset = target_offset;

	return 0;
}

static int
i915_gem_execbuffer_relocate_object(struct drm_i915_gem_object *obj,
				    struct eb_objects *eb)
{
#define N_RELOC(x) ((x) / sizeof(struct drm_i915_gem_relocation_entry))
	struct drm_i915_gem_relocation_entry stack_reloc[N_RELOC(512)];
	struct drm_i915_gem_relocation_entry __user *user_relocs;
	struct drm_i915_gem_exec_object2 *entry = obj->exec_entry;
	int remain, ret;

	user_relocs = (void __user *)(uintptr_t)entry->relocs_ptr;

	remain = entry->relocation_count;
	while (remain) {
		struct drm_i915_gem_relocation_entry *r = stack_reloc;
		int count = remain;
		if (count > ARRAY_SIZE(stack_reloc))
			count = ARRAY_SIZE(stack_reloc);
		remain -= count;

		if (__copy_from_user_inatomic(r, user_relocs, count*sizeof(r[0])))
			return -EFAULT;

		do {
			u64 offset = r->presumed_offset;

			ret = i915_gem_execbuffer_relocate_entry(obj, eb, r);
			if (ret)
				return ret;

			if (r->presumed_offset != offset &&
			    __copy_to_user_inatomic(&user_relocs->presumed_offset,
						    &r->presumed_offset,
						    sizeof(r->presumed_offset))) {
				return -EFAULT;
			}

			user_relocs++;
			r++;
		} while (--count);
	}

	return 0;
#undef N_RELOC
}

static int
i915_gem_execbuffer_relocate_object_slow(struct drm_i915_gem_object *obj,
					 struct eb_objects *eb,
					 struct drm_i915_gem_relocation_entry *relocs)
{
	const struct drm_i915_gem_exec_object2 *entry = obj->exec_entry;
	int i, ret;

	for (i = 0; i < entry->relocation_count; i++) {
		ret = i915_gem_execbuffer_relocate_entry(obj, eb, &relocs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int
i915_gem_execbuffer_relocate(struct drm_device *dev,
			     struct eb_objects *eb,
			     struct list_head *objects)
{
	struct drm_i915_gem_object *obj;
	int ret = 0;

	/* This is the fast path and we cannot handle a pagefault whilst
	 * holding the struct mutex lest the user pass in the relocations
	 * contained within a mmaped bo. For in such a case we, the page
	 * fault handler would call i915_gem_fault() and we would try to
	 * acquire the struct mutex again. Obviously this is bad and so
	 * lockdep complains vehemently.
	 */
	pagefault_disable();
	list_for_each_entry(obj, objects, exec_list) {
		ret = i915_gem_execbuffer_relocate_object(obj, eb);
		if (ret)
			break;
	}
	pagefault_enable();

	return ret;
}

#define  __EXEC_OBJECT_HAS_PIN (1<<31)
#define  __EXEC_OBJECT_HAS_FENCE (1<<30)

static int
need_reloc_mappable(struct drm_i915_gem_object *obj)
{
	struct drm_i915_gem_exec_object2 *entry = obj->exec_entry;
	return entry->relocation_count && !use_cpu_reloc(obj);
}

static int
i915_gem_execbuffer_reserve_object(struct drm_i915_gem_object *obj,
				   struct intel_ring_buffer *ring)
{
	struct drm_i915_private *dev_priv = obj->base.dev->dev_private;
	struct drm_i915_gem_exec_object2 *entry = obj->exec_entry;
	bool has_fenced_gpu_access = INTEL_INFO(ring->dev)->gen < 4;
	bool need_fence, need_mappable;
	int ret;

	need_fence =
		has_fenced_gpu_access &&
		entry->flags & EXEC_OBJECT_NEEDS_FENCE &&
		obj->tiling_mode != I915_TILING_NONE;
	need_mappable = need_fence || need_reloc_mappable(obj);

	ret = i915_gem_object_pin(obj, entry->alignment, need_mappable, false);
	if (ret)
		return ret;

	entry->flags |= __EXEC_OBJECT_HAS_PIN;

	if (has_fenced_gpu_access) {
		if (entry->flags & EXEC_OBJECT_NEEDS_FENCE) {
			ret = i915_gem_object_get_fence(obj);
			if (ret)
				return ret;

			if (i915_gem_object_pin_fence(obj))
				entry->flags |= __EXEC_OBJECT_HAS_FENCE;

			obj->pending_fenced_gpu_access = true;
		}
	}

	/* Ensure ppgtt mapping exists if needed */
	if (dev_priv->mm.aliasing_ppgtt && !obj->has_aliasing_ppgtt_mapping) {
		i915_ppgtt_bind_object(dev_priv->mm.aliasing_ppgtt,
				       obj, obj->cache_level);

		obj->has_aliasing_ppgtt_mapping = 1;
	}

	entry->offset = obj->gtt_offset;
	return 0;
}

static void
i915_gem_execbuffer_unreserve_object(struct drm_i915_gem_object *obj)
{
	struct drm_i915_gem_exec_object2 *entry;

	if (!obj->gtt_space)
		return;

	entry = obj->exec_entry;

	if (entry->flags & __EXEC_OBJECT_HAS_FENCE)
		i915_gem_object_unpin_fence(obj);

	if (entry->flags & __EXEC_OBJECT_HAS_PIN)
		i915_gem_object_unpin(obj);

	entry->flags &= ~(__EXEC_OBJECT_HAS_FENCE | __EXEC_OBJECT_HAS_PIN);
}

static int
i915_gem_execbuffer_reserve(struct intel_ring_buffer *ring,
			    struct drm_file *file,
			    struct list_head *objects)
{
	struct drm_i915_gem_object *obj;
	struct list_head ordered_objects;
	bool has_fenced_gpu_access = INTEL_INFO(ring->dev)->gen < 4;
	int retry;

	INIT_LIST_HEAD(&ordered_objects);
	while (!list_empty(objects)) {
		struct drm_i915_gem_exec_object2 *entry;
		bool need_fence, need_mappable;

		obj = list_first_entry(objects,
				       struct drm_i915_gem_object,
				       exec_list);
		entry = obj->exec_entry;

		need_fence =
			has_fenced_gpu_access &&
			entry->flags & EXEC_OBJECT_NEEDS_FENCE &&
			obj->tiling_mode != I915_TILING_NONE;
		need_mappable = need_fence || need_reloc_mappable(obj);

		if (need_mappable)
			list_move(&obj->exec_list, &ordered_objects);
		else
			list_move_tail(&obj->exec_list, &ordered_objects);

		obj->base.pending_read_domains = 0;
		obj->base.pending_write_domain = 0;
		obj->pending_fenced_gpu_access = false;
	}
	list_splice(&ordered_objects, objects);

	/* Attempt to pin all of the buffers into the GTT.
	 * This is done in 3 phases:
	 *
	 * 1a. Unbind all objects that do not match the GTT constraints for
	 *     the execbuffer (fenceable, mappable, alignment etc).
	 * 1b. Increment pin count for already bound objects.
	 * 2.  Bind new objects.
	 * 3.  Decrement pin count.
	 *
	 * This avoid unnecessary unbinding of later objects in order to make
	 * room for the earlier objects *unless* we need to defragment.
	 */
	retry = 0;
	do {
		int ret = 0;

		/* Unbind any ill-fitting objects or pin. */
		list_for_each_entry(obj, objects, exec_list) {
			struct drm_i915_gem_exec_object2 *entry = obj->exec_entry;
			bool need_fence, need_mappable;

			if (!obj->gtt_space)
				continue;

			need_fence =
				has_fenced_gpu_access &&
				entry->flags & EXEC_OBJECT_NEEDS_FENCE &&
				obj->tiling_mode != I915_TILING_NONE;
			need_mappable = need_fence || need_reloc_mappable(obj);

			if ((entry->alignment && obj->gtt_offset & (entry->alignment - 1)) ||
			    (need_mappable && !obj->map_and_fenceable))
				ret = i915_gem_object_unbind(obj);
			else
				ret = i915_gem_execbuffer_reserve_object(obj, ring);
			if (ret)
				goto err;
		}

		/* Bind fresh objects */
		list_for_each_entry(obj, objects, exec_list) {
			if (obj->gtt_space)
				continue;

			ret = i915_gem_execbuffer_reserve_object(obj, ring);
			if (ret)
				goto err;
		}

err:		/* Decrement pin count for bound objects */
		list_for_each_entry(obj, objects, exec_list)
			i915_gem_execbuffer_unreserve_object(obj);

		if (ret != -ENOSPC || retry++)
			return ret;

		ret = i915_gem_evict_everything(ring->dev);
		if (ret)
			return ret;
	} while (1);
}

static int
i915_gem_execbuffer_relocate_slow(struct drm_device *dev,
				  struct drm_file *file,
				  struct intel_ring_buffer *ring,
				  struct list_head *objects,
				  struct eb_objects *eb,
				  struct drm_i915_gem_exec_object2 *exec,
				  int count)
{
	struct drm_i915_gem_relocation_entry *reloc;
	struct drm_i915_gem_object *obj;
	int *reloc_offset;
	int i, total, ret;

	/* We may process another execbuffer during the unlock... */
	while (!list_empty(objects)) {
		obj = list_first_entry(objects,
				       struct drm_i915_gem_object,
				       exec_list);
		list_del_init(&obj->exec_list);
		drm_gem_object_unreference(&obj->base);
	}

	mutex_unlock(&dev->struct_mutex);

	total = 0;
	for (i = 0; i < count; i++)
		total += exec[i].relocation_count;

	reloc_offset = drm_malloc_ab(count, sizeof(*reloc_offset));
	reloc = drm_malloc_ab(total, sizeof(*reloc));
	if (reloc == NULL || reloc_offset == NULL) {
		drm_free_large(reloc);
		drm_free_large(reloc_offset);
		mutex_lock(&dev->struct_mutex);
		return -ENOMEM;
	}

	total = 0;
	for (i = 0; i < count; i++) {
		struct drm_i915_gem_relocation_entry __user *user_relocs;
		u64 invalid_offset = (u64)-1;
		int j;

		user_relocs = (void __user *)(uintptr_t)exec[i].relocs_ptr;

		if (copy_from_user(reloc+total, user_relocs,
				   exec[i].relocation_count * sizeof(*reloc))) {
			ret = -EFAULT;
			mutex_lock(&dev->struct_mutex);
			goto err;
		}

		/* As we do not update the known relocation offsets after
		 * relocating (due to the complexities in lock handling),
		 * we need to mark them as invalid now so that we force the
		 * relocation processing next time. Just in case the target
		 * object is evicted and then rebound into its old
		 * presumed_offset before the next execbuffer - if that
		 * happened we would make the mistake of assuming that the
		 * relocations were valid.
		 */
		for (j = 0; j < exec[i].relocation_count; j++) {
			if (copy_to_user(&user_relocs[j].presumed_offset,
					 &invalid_offset,
					 sizeof(invalid_offset))) {
				ret = -EFAULT;
				mutex_lock(&dev->struct_mutex);
				goto err;
			}
		}

		reloc_offset[i] = total;
		total += exec[i].relocation_count;
	}

	ret = i915_mutex_lock_interruptible(dev);
	if (ret) {
		mutex_lock(&dev->struct_mutex);
		goto err;
	}

	/* reacquire the objects */
	eb_reset(eb);
	for (i = 0; i < count; i++) {
		obj = to_intel_bo(drm_gem_object_lookup(dev, file,
							exec[i].handle));
		if (&obj->base == NULL) {
			DRM_DEBUG("Invalid object handle %d at index %d\n",
				   exec[i].handle, i);
			ret = -ENOENT;
			goto err;
		}

		list_add_tail(&obj->exec_list, objects);
		obj->exec_handle = exec[i].handle;
		obj->exec_entry = &exec[i];
		eb_add_object(eb, obj);
	}

	ret = i915_gem_execbuffer_reserve(ring, file, objects);
	if (ret)
		goto err;

	list_for_each_entry(obj, objects, exec_list) {
		int offset = obj->exec_entry - exec;
		ret = i915_gem_execbuffer_relocate_object_slow(obj, eb,
							       reloc + reloc_offset[offset]);
		if (ret)
			goto err;
	}

	/* Leave the user relocations as are, this is the painfully slow path,
	 * and we want to avoid the complication of dropping the lock whilst
	 * having buffers reserved in the aperture and so causing spurious
	 * ENOSPC for random operations.
	 */

err:
	drm_free_large(reloc);
	drm_free_large(reloc_offset);
	return ret;
}

static int
i915_gem_execbuffer_wait_for_flips(struct intel_ring_buffer *ring, u32 flips)
{
	u32 plane, flip_mask;
	int ret;

	/* Check for any pending flips. As we only maintain a flip queue depth
	 * of 1, we can simply insert a WAIT for the next display flip prior
	 * to executing the batch and avoid stalling the CPU.
	 */

	for (plane = 0; flips >> plane; plane++) {
		if (((flips >> plane) & 1) == 0)
			continue;

		if (plane)
			flip_mask = MI_WAIT_FOR_PLANE_B_FLIP;
		else
			flip_mask = MI_WAIT_FOR_PLANE_A_FLIP;

		ret = intel_ring_begin(ring, 2);
		if (ret)
			return ret;

		intel_ring_emit(ring, MI_WAIT_FOR_EVENT | flip_mask);
		intel_ring_emit(ring, MI_NOOP);
		intel_ring_advance(ring);
	}

	return 0;
}

static int
i915_gem_execbuffer_move_to_gpu(struct intel_ring_buffer *ring,
				struct list_head *objects)
{
	struct drm_i915_gem_object *obj;
	uint32_t flush_domains = 0;
	uint32_t flips = 0;
	int ret;

	list_for_each_entry(obj, objects, exec_list) {
		ret = i915_gem_object_sync(obj, ring);
		if (ret)
			return ret;

		if (obj->base.write_domain & I915_GEM_DOMAIN_CPU)
			i915_gem_clflush_object(obj);

		if (obj->base.pending_write_domain)
			flips |= atomic_read(&obj->pending_flip);

		flush_domains |= obj->base.write_domain;
	}

	if (flips) {
		ret = i915_gem_execbuffer_wait_for_flips(ring, flips);
		if (ret)
			return ret;
	}

	if (flush_domains & I915_GEM_DOMAIN_CPU)
		i915_gem_chipset_flush(ring->dev);

	if (flush_domains & I915_GEM_DOMAIN_GTT)
		wmb();

	/* Unconditionally invalidate gpu caches and ensure that we do flush
	 * any residual writes from the previous batch.
	 */
	return intel_ring_invalidate_all_caches(ring);
}

static bool
i915_gem_check_execbuffer(struct drm_i915_gem_execbuffer2 *exec)
{
	return ((exec->batch_start_offset | exec->batch_len) & 0x7) == 0;
}

static int
validate_exec_list(struct drm_i915_gem_exec_object2 *exec,
		   int count)
{
	int i;

	for (i = 0; i < count; i++) {
		char __user *ptr = (char __user *)(uintptr_t)exec[i].relocs_ptr;
		int length; /* limited by fault_in_pages_readable() */

		/* First check for malicious input causing overflow */
		if (exec[i].relocation_count >
		    INT_MAX / sizeof(struct drm_i915_gem_relocation_entry))
			return -EINVAL;

		length = exec[i].relocation_count *
			sizeof(struct drm_i915_gem_relocation_entry);
		if (!access_ok(VERIFY_READ, ptr, length))
			return -EFAULT;

		/* we may also need to update the presumed offsets */
		if (!access_ok(VERIFY_WRITE, ptr, length))
			return -EFAULT;

		if (fault_in_multipages_readable(ptr, length))
			return -EFAULT;
	}

	return 0;
}

static void
i915_gem_execbuffer_move_to_active(struct list_head *objects,
				   struct intel_ring_buffer *ring)
{
	struct drm_i915_gem_object *obj;

	list_for_each_entry(obj, objects, exec_list) {
		u32 old_read = obj->base.read_domains;
		u32 old_write = obj->base.write_domain;

		obj->base.read_domains = obj->base.pending_read_domains;
		obj->base.write_domain = obj->base.pending_write_domain;
		obj->fenced_gpu_access = obj->pending_fenced_gpu_access;

		i915_gem_object_move_to_active(obj, ring);
		if (obj->base.write_domain) {
			obj->dirty = 1;
			obj->last_write_seqno = intel_ring_get_seqno(ring);
			if (obj->pin_count) /* check for potential scanout */
				intel_mark_fb_busy(obj);
		}

		trace_i915_gem_object_change_domain(obj, old_read, old_write);
	}
}

static void
i915_gem_execbuffer_retire_commands(struct drm_device *dev,
				    struct drm_file *file,
				    struct intel_ring_buffer *ring)
{
	/* Unconditionally force add_request to emit a full flush. */
	ring->gpu_caches_dirty = true;

	/* Add a breadcrumb for the completion of the batch buffer */
	(void)i915_add_request(ring, file, NULL);
}

static int
i915_reset_gen7_sol_offsets(struct drm_device *dev,
			    struct intel_ring_buffer *ring)
{
	drm_i915_private_t *dev_priv = dev->dev_private;
	int ret, i;

	if (!IS_GEN7(dev) || ring != &dev_priv->ring[RCS])
		return 0;

	ret = intel_ring_begin(ring, 4 * 3);
	if (ret)
		return ret;

	for (i = 0; i < 4; i++) {
		intel_ring_emit(ring, MI_LOAD_REGISTER_IMM(1));
		intel_ring_emit(ring, GEN7_SO_WRITE_OFFSET(i));
		intel_ring_emit(ring, 0);
	}

	intel_ring_advance(ring);

	return 0;
}

static int
i915_gem_do_execbuffer(struct drm_device *dev, void *data,
		       struct drm_file *file,
		       struct drm_i915_gem_execbuffer2 *args,
		       struct drm_i915_gem_exec_object2 *exec)
{
	drm_i915_private_t *dev_priv = dev->dev_private;
	struct list_head objects;
	struct eb_objects *eb;
	struct drm_i915_gem_object *batch_obj;
	struct drm_clip_rect *cliprects = NULL;
	struct intel_ring_buffer *ring;
	u32 ctx_id = i915_execbuffer2_get_context_id(*args);
	u32 exec_start, exec_len;
	u32 mask;
	u32 flags;
	int ret, mode, i;

	if (!i915_gem_check_execbuffer(args)) {
		DRM_DEBUG("execbuf with invalid offset/length\n");
		return -EINVAL;
	}

	ret = validate_exec_list(exec, args->buffer_count);
	if (ret)
		return ret;

	flags = 0;
	if (args->flags & I915_EXEC_SECURE) {
		if (!file->is_master || !capable(CAP_SYS_ADMIN))
		    return -EPERM;

		flags |= I915_DISPATCH_SECURE;
	}
	if (args->flags & I915_EXEC_IS_PINNED)
		flags |= I915_DISPATCH_PINNED;

	switch (args->flags & I915_EXEC_RING_MASK) {
	case I915_EXEC_DEFAULT:
	case I915_EXEC_RENDER:
		ring = &dev_priv->ring[RCS];
		break;
	case I915_EXEC_BSD:
		ring = &dev_priv->ring[VCS];
		if (ctx_id != 0) {
			DRM_DEBUG("Ring %s doesn't support contexts\n",
				  ring->name);
			return -EPERM;
		}
		break;
	case I915_EXEC_BLT:
		ring = &dev_priv->ring[BCS];
		if (ctx_id != 0) {
			DRM_DEBUG("Ring %s doesn't support contexts\n",
				  ring->name);
			return -EPERM;
		}
		break;
	default:
		DRM_DEBUG("execbuf with unknown ring: %d\n",
			  (int)(args->flags & I915_EXEC_RING_MASK));
		return -EINVAL;
	}
	if (!intel_ring_initialized(ring)) {
		DRM_DEBUG("execbuf with invalid ring: %d\n",
			  (int)(args->flags & I915_EXEC_RING_MASK));
		return -EINVAL;
	}

	mode = args->flags & I915_EXEC_CONSTANTS_MASK;
	mask = I915_EXEC_CONSTANTS_MASK;
	switch (mode) {
	case I915_EXEC_CONSTANTS_REL_GENERAL:
	case I915_EXEC_CONSTANTS_ABSOLUTE:
	case I915_EXEC_CONSTANTS_REL_SURFACE:
		if (ring == &dev_priv->ring[RCS] &&
		    mode != dev_priv->relative_constants_mode) {
			if (INTEL_INFO(dev)->gen < 4)
				return -EINVAL;

			if (INTEL_INFO(dev)->gen > 5 &&
			    mode == I915_EXEC_CONSTANTS_REL_SURFACE)
				return -EINVAL;

			/* The HW changed the meaning on this bit on gen6 */
			if (INTEL_INFO(dev)->gen >= 6)
				mask &= ~I915_EXEC_CONSTANTS_REL_SURFACE;
		}
		break;
	default:
		DRM_DEBUG("execbuf with unknown constants: %d\n", mode);
		return -EINVAL;
	}

	if (args->buffer_count < 1) {
		DRM_DEBUG("execbuf with %d buffers\n", args->buffer_count);
		return -EINVAL;
	}

	if (args->num_cliprects != 0) {
		if (ring != &dev_priv->ring[RCS]) {
			DRM_DEBUG("clip rectangles are only valid with the render ring\n");
			return -EINVAL;
		}

		if (INTEL_INFO(dev)->gen >= 5) {
			DRM_DEBUG("clip rectangles are only valid on pre-gen5\n");
			return -EINVAL;
		}

		if (args->num_cliprects > UINT_MAX / sizeof(*cliprects)) {
			DRM_DEBUG("execbuf with %u cliprects\n",
				  args->num_cliprects);
			return -EINVAL;
		}

		cliprects = kmalloc(args->num_cliprects * sizeof(*cliprects),
				    GFP_KERNEL);
		if (cliprects == NULL) {
			ret = -ENOMEM;
			goto pre_mutex_err;
		}

		if (copy_from_user(cliprects,
				     (struct drm_clip_rect __user *)(uintptr_t)
				     args->cliprects_ptr,
				     sizeof(*cliprects)*args->num_cliprects)) {
			ret = -EFAULT;
			goto pre_mutex_err;
		}
	}

	ret = i915_mutex_lock_interruptible(dev);
	if (ret)
		goto pre_mutex_err;

	if (dev_priv->mm.suspended) {
		mutex_unlock(&dev->struct_mutex);
		ret = -EBUSY;
		goto pre_mutex_err;
	}

	eb = eb_create(args->buffer_count);
	if (eb == NULL) {
		mutex_unlock(&dev->struct_mutex);
		ret = -ENOMEM;
		goto pre_mutex_err;
	}

	/* Look up object handles */
	INIT_LIST_HEAD(&objects);
	for (i = 0; i < args->buffer_count; i++) {
		struct drm_i915_gem_object *obj;

		obj = to_intel_bo(drm_gem_object_lookup(dev, file,
							exec[i].handle));
		if (&obj->base == NULL) {
			DRM_DEBUG("Invalid object handle %d at index %d\n",
				   exec[i].handle, i);
			/* prevent error path from reading uninitialized data */
			ret = -ENOENT;
			goto err;
		}

		if (!list_empty(&obj->exec_list)) {
			DRM_DEBUG("Object %p [handle %d, index %d] appears more than once in object list\n",
				   obj, exec[i].handle, i);
			ret = -EINVAL;
			goto err;
		}

		list_add_tail(&obj->exec_list, &objects);
		obj->exec_handle = exec[i].handle;
		obj->exec_entry = &exec[i];
		eb_add_object(eb, obj);
	}

	/* take note of the batch buffer before we might reorder the lists */
	batch_obj = list_entry(objects.prev,
			       struct drm_i915_gem_object,
			       exec_list);

	/* Move the objects en-masse into the GTT, evicting if necessary. */
	ret = i915_gem_execbuffer_reserve(ring, file, &objects);
	if (ret)
		goto err;

	/* The objects are in their final locations, apply the relocations. */
	ret = i915_gem_execbuffer_relocate(dev, eb, &objects);
	if (ret) {
		if (ret == -EFAULT) {
			ret = i915_gem_execbuffer_relocate_slow(dev, file, ring,
								&objects, eb,
								exec,
								args->buffer_count);
			BUG_ON(!mutex_is_locked(&dev->struct_mutex));
		}
		if (ret)
			goto err;
	}

	/* Set the pending read domains for the batch buffer to COMMAND */
	if (batch_obj->base.pending_write_domain) {
		DRM_DEBUG("Attempting to use self-modifying batch buffer\n");
		ret = -EINVAL;
		goto err;
	}
	batch_obj->base.pending_read_domains |= I915_GEM_DOMAIN_COMMAND;

	/* snb/ivb/vlv conflate the "batch in ppgtt" bit with the "non-secure
	 * batch" bit. Hence we need to pin secure batches into the global gtt.
	 * hsw should have this fixed, but let's be paranoid and do it
	 * unconditionally for now. */
	if (flags & I915_DISPATCH_SECURE && !batch_obj->has_global_gtt_mapping)
		i915_gem_gtt_bind_object(batch_obj, batch_obj->cache_level);

	ret = i915_gem_execbuffer_move_to_gpu(ring, &objects);
	if (ret)
		goto err;

	ret = i915_switch_context(ring, file, ctx_id);
	if (ret)
		goto err;

	if (ring == &dev_priv->ring[RCS] &&
	    mode != dev_priv->relative_constants_mode) {
		ret = intel_ring_begin(ring, 4);
		if (ret)
				goto err;

		intel_ring_emit(ring, MI_NOOP);
		intel_ring_emit(ring, MI_LOAD_REGISTER_IMM(1));
		intel_ring_emit(ring, INSTPM);
		intel_ring_emit(ring, mask << 16 | mode);
		intel_ring_advance(ring);

		dev_priv->relative_constants_mode = mode;
	}

	if (args->flags & I915_EXEC_GEN7_SOL_RESET) {
		ret = i915_reset_gen7_sol_offsets(dev, ring);
		if (ret)
			goto err;
	}

	exec_start = batch_obj->gtt_offset + args->batch_start_offset;
	exec_len = args->batch_len;
	if (cliprects) {
		for (i = 0; i < args->num_cliprects; i++) {
			ret = i915_emit_box(dev, &cliprects[i],
					    args->DR1, args->DR4);
			if (ret)
				goto err;

			ret = ring->dispatch_execbuffer(ring,
							exec_start, exec_len,
							flags);
			if (ret)
				goto err;
		}
	} else {
		ret = ring->dispatch_execbuffer(ring,
						exec_start, exec_len,
						flags);
		if (ret)
			goto err;
	}

	trace_i915_gem_ring_dispatch(ring, intel_ring_get_seqno(ring), flags);

	i915_gem_execbuffer_move_to_active(&objects, ring);
	i915_gem_execbuffer_retire_commands(dev, file, ring);

err:
	eb_destroy(eb);
	while (!list_empty(&objects)) {
		struct drm_i915_gem_object *obj;

		obj = list_first_entry(&objects,
				       struct drm_i915_gem_object,
				       exec_list);
		list_del_init(&obj->exec_list);
		drm_gem_object_unreference(&obj->base);
	}

	mutex_unlock(&dev->struct_mutex);

pre_mutex_err:
	kfree(cliprects);
	return ret;
}

/*
 * Legacy execbuffer just creates an exec2 list from the original exec object
 * list array and passes it to the real function.
 */
int
i915_gem_execbuffer(struct drm_device *dev, void *data,
		    struct drm_file *file)
{
	struct drm_i915_gem_execbuffer *args = data;
	struct drm_i915_gem_execbuffer2 exec2;
	struct drm_i915_gem_exec_object *exec_list = NULL;
	struct drm_i915_gem_exec_object2 *exec2_list = NULL;
	int ret, i;

	if (args->buffer_count < 1) {
		DRM_DEBUG("execbuf with %d buffers\n", args->buffer_count);
		return -EINVAL;
	}

	/* Copy in the exec list from userland */
	exec_list = drm_malloc_ab(sizeof(*exec_list), args->buffer_count);
	exec2_list = drm_malloc_ab(sizeof(*exec2_list), args->buffer_count);
	if (exec_list == NULL || exec2_list == NULL) {
		DRM_DEBUG("Failed to allocate exec list for %d buffers\n",
			  args->buffer_count);
		drm_free_large(exec_list);
		drm_free_large(exec2_list);
		return -ENOMEM;
	}
	ret = copy_from_user(exec_list,
			     (void __user *)(uintptr_t)args->buffers_ptr,
			     sizeof(*exec_list) * args->buffer_count);
	if (ret != 0) {
		DRM_DEBUG("copy %d exec entries failed %d\n",
			  args->buffer_count, ret);
		drm_free_large(exec_list);
		drm_free_large(exec2_list);
		return -EFAULT;
	}

	for (i = 0; i < args->buffer_count; i++) {
		exec2_list[i].handle = exec_list[i].handle;
		exec2_list[i].relocation_count = exec_list[i].relocation_count;
		exec2_list[i].relocs_ptr = exec_list[i].relocs_ptr;
		exec2_list[i].alignment = exec_list[i].alignment;
		exec2_list[i].offset = exec_list[i].offset;
		if (INTEL_INFO(dev)->gen < 4)
			exec2_list[i].flags = EXEC_OBJECT_NEEDS_FENCE;
		else
			exec2_list[i].flags = 0;
	}

	exec2.buffers_ptr = args->buffers_ptr;
	exec2.buffer_count = args->buffer_count;
	exec2.batch_start_offset = args->batch_start_offset;
	exec2.batch_len = args->batch_len;
	exec2.DR1 = args->DR1;
	exec2.DR4 = args->DR4;
	exec2.num_cliprects = args->num_cliprects;
	exec2.cliprects_ptr = args->cliprects_ptr;
	exec2.flags = I915_EXEC_RENDER;
	i915_execbuffer2_set_context_id(exec2, 0);

	ret = i915_gem_do_execbuffer(dev, data, file, &exec2, exec2_list);
	if (!ret) {
		/* Copy the new buffer offsets back to the user's exec list. */
		for (i = 0; i < args->buffer_count; i++)
			exec_list[i].offset = exec2_list[i].offset;
		/* ... and back out to userspace */
		ret = copy_to_user((void __user *)(uintptr_t)args->buffers_ptr,
				   exec_list,
				   sizeof(*exec_list) * args->buffer_count);
		if (ret) {
			ret = -EFAULT;
			DRM_DEBUG("failed to copy %d exec entries "
				  "back to user (%d)\n",
				  args->buffer_count, ret);
		}
	}

	drm_free_large(exec_list);
	drm_free_large(exec2_list);
	return ret;
}

int
i915_gem_execbuffer2(struct drm_device *dev, void *data,
		     struct drm_file *file)
{
	struct drm_i915_gem_execbuffer2 *args = data;
	struct drm_i915_gem_exec_object2 *exec2_list = NULL;
	int ret;

	if (args->buffer_count < 1 ||
	    args->buffer_count > UINT_MAX / sizeof(*exec2_list)) {
		DRM_DEBUG("execbuf2 with %d buffers\n", args->buffer_count);
		return -EINVAL;
	}

	exec2_list = kmalloc(sizeof(*exec2_list)*args->buffer_count,
			     GFP_KERNEL | __GFP_NOWARN | __GFP_NORETRY);
	if (exec2_list == NULL)
		exec2_list = drm_malloc_ab(sizeof(*exec2_list),
					   args->buffer_count);
	if (exec2_list == NULL) {
		DRM_DEBUG("Failed to allocate exec list for %d buffers\n",
			  args->buffer_count);
		return -ENOMEM;
	}
	ret = copy_from_user(exec2_list,
			     (struct drm_i915_relocation_entry __user *)
			     (uintptr_t) args->buffers_ptr,
			     sizeof(*exec2_list) * args->buffer_count);
	if (ret != 0) {
		DRM_DEBUG("copy %d exec entries failed %d\n",
			  args->buffer_count, ret);
		drm_free_large(exec2_list);
		return -EFAULT;
	}

	ret = i915_gem_do_execbuffer(dev, data, file, args, exec2_list);
	if (!ret) {
		/* Copy the new buffer offsets back to the user's exec list. */
		ret = copy_to_user((void __user *)(uintptr_t)args->buffers_ptr,
				   exec2_list,
				   sizeof(*exec2_list) * args->buffer_count);
		if (ret) {
			ret = -EFAULT;
			DRM_DEBUG("failed to copy %d exec entries "
				  "back to user (%d)\n",
				  args->buffer_count, ret);
		}
	}

	drm_free_large(exec2_list);
	return ret;
}
