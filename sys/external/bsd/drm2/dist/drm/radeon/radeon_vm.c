/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 */
#include <drm/drmP.h>
#include <drm/radeon_drm.h>
#include "radeon.h"
#include "radeon_trace.h"

/*
 * GPUVM
 * GPUVM is similar to the legacy gart on older asics, however
 * rather than there being a single global gart table
 * for the entire GPU, there are multiple VM page tables active
 * at any given time.  The VM page tables can contain a mix
 * vram pages and system memory pages and system memory pages
 * can be mapped as snooped (cached system pages) or unsnooped
 * (uncached system pages).
 * Each VM has an ID associated with it and there is a page table
 * associated with each VMID.  When execting a command buffer,
 * the kernel tells the the ring what VMID to use for that command
 * buffer.  VMIDs are allocated dynamically as commands are submitted.
 * The userspace drivers maintain their own address space and the kernel
 * sets up their pages tables accordingly when they submit their
 * command buffers and a VMID is assigned.
 * Cayman/Trinity support up to 8 active VMs at any given time;
 * SI supports 16.
 */

/**
 * radeon_vm_num_pde - return the number of page directory entries
 *
 * @rdev: radeon_device pointer
 *
 * Calculate the number of page directory entries (cayman+).
 */
static unsigned radeon_vm_num_pdes(struct radeon_device *rdev)
{
	return rdev->vm_manager.max_pfn >> RADEON_VM_BLOCK_SIZE;
}

/**
 * radeon_vm_directory_size - returns the size of the page directory in bytes
 *
 * @rdev: radeon_device pointer
 *
 * Calculate the size of the page directory in bytes (cayman+).
 */
static unsigned radeon_vm_directory_size(struct radeon_device *rdev)
{
	return RADEON_GPU_PAGE_ALIGN(radeon_vm_num_pdes(rdev) * 8);
}

/**
 * radeon_vm_manager_init - init the vm manager
 *
 * @rdev: radeon_device pointer
 *
 * Init the vm manager (cayman+).
 * Returns 0 for success, error for failure.
 */
int radeon_vm_manager_init(struct radeon_device *rdev)
{
	int r;

	if (!rdev->vm_manager.enabled) {
		r = radeon_asic_vm_init(rdev);
		if (r)
			return r;

		rdev->vm_manager.enabled = true;
	}
	return 0;
}

/**
 * radeon_vm_manager_fini - tear down the vm manager
 *
 * @rdev: radeon_device pointer
 *
 * Tear down the VM manager (cayman+).
 */
void radeon_vm_manager_fini(struct radeon_device *rdev)
{
	int i;

	if (!rdev->vm_manager.enabled)
		return;

	for (i = 0; i < RADEON_NUM_VM; ++i)
		radeon_fence_unref(&rdev->vm_manager.active[i]);
	radeon_asic_vm_fini(rdev);
	rdev->vm_manager.enabled = false;
}

/**
 * radeon_vm_get_bos - add the vm BOs to a validation list
 *
 * @vm: vm providing the BOs
 * @head: head of validation list
 *
 * Add the page directory to the list of BOs to
 * validate for command submission (cayman+).
 */
struct radeon_cs_reloc *radeon_vm_get_bos(struct radeon_device *rdev,
					  struct radeon_vm *vm,
					  struct list_head *head)
{
	struct radeon_cs_reloc *list;
	unsigned i, idx;

	list = kmalloc_array(vm->max_pde_used + 2,
			     sizeof(struct radeon_cs_reloc), GFP_KERNEL);
	if (!list)
		return NULL;

	/* add the vm page table to the list */
	list[0].gobj = NULL;
	list[0].robj = vm->page_directory;
	list[0].domain = RADEON_GEM_DOMAIN_VRAM;
	list[0].alt_domain = RADEON_GEM_DOMAIN_VRAM;
	list[0].tv.bo = &vm->page_directory->tbo;
	list[0].tiling_flags = 0;
	list[0].handle = 0;
	list_add(&list[0].tv.head, head);

	for (i = 0, idx = 1; i <= vm->max_pde_used; i++) {
		if (!vm->page_tables[i].bo)
			continue;

		list[idx].gobj = NULL;
		list[idx].robj = vm->page_tables[i].bo;
		list[idx].domain = RADEON_GEM_DOMAIN_VRAM;
		list[idx].alt_domain = RADEON_GEM_DOMAIN_VRAM;
		list[idx].tv.bo = &list[idx].robj->tbo;
		list[idx].tiling_flags = 0;
		list[idx].handle = 0;
		list_add(&list[idx++].tv.head, head);
	}

	return list;
}

/**
 * radeon_vm_grab_id - allocate the next free VMID
 *
 * @rdev: radeon_device pointer
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 *
 * Allocate an id for the vm (cayman+).
 * Returns the fence we need to sync to (if any).
 *
 * Global and local mutex must be locked!
 */
struct radeon_fence *radeon_vm_grab_id(struct radeon_device *rdev,
				       struct radeon_vm *vm, int ring)
{
	struct radeon_fence *best[RADEON_NUM_RINGS] = {};
	unsigned choices[2] = {};
	unsigned i;

	/* check if the id is still valid */
	if (vm->last_id_use && vm->last_id_use == rdev->vm_manager.active[vm->id])
		return NULL;

	/* we definately need to flush */
	radeon_fence_unref(&vm->last_flush);

	/* skip over VMID 0, since it is the system VM */
	for (i = 1; i < rdev->vm_manager.nvm; ++i) {
		struct radeon_fence *fence = rdev->vm_manager.active[i];

		if (fence == NULL) {
			/* found a free one */
			vm->id = i;
			trace_radeon_vm_grab_id(vm->id, ring);
			return NULL;
		}

		if (radeon_fence_is_earlier(fence, best[fence->ring])) {
			best[fence->ring] = fence;
			choices[fence->ring == ring ? 0 : 1] = i;
		}
	}

	for (i = 0; i < 2; ++i) {
		if (choices[i]) {
			vm->id = choices[i];
			trace_radeon_vm_grab_id(vm->id, ring);
			return rdev->vm_manager.active[choices[i]];
		}
	}

	/* should never happen */
	BUG();
	return NULL;
}

/**
 * radeon_vm_flush - hardware flush the vm
 *
 * @rdev: radeon_device pointer
 * @vm: vm we want to flush
 * @ring: ring to use for flush
 *
 * Flush the vm (cayman+).
 *
 * Global and local mutex must be locked!
 */
void radeon_vm_flush(struct radeon_device *rdev,
		     struct radeon_vm *vm,
		     int ring)
{
	uint64_t pd_addr = radeon_bo_gpu_offset(vm->page_directory);

	/* if we can't remember our last VM flush then flush now! */
	/* XXX figure out why we have to flush all the time */
	if (!vm->last_flush || true || pd_addr != vm->pd_gpu_addr) {
		vm->pd_gpu_addr = pd_addr;
		radeon_ring_vm_flush(rdev, ring, vm);
	}
}

/**
 * radeon_vm_fence - remember fence for vm
 *
 * @rdev: radeon_device pointer
 * @vm: vm we want to fence
 * @fence: fence to remember
 *
 * Fence the vm (cayman+).
 * Set the fence used to protect page table and id.
 *
 * Global and local mutex must be locked!
 */
void radeon_vm_fence(struct radeon_device *rdev,
		     struct radeon_vm *vm,
		     struct radeon_fence *fence)
{
	radeon_fence_unref(&vm->fence);
	vm->fence = radeon_fence_ref(fence);

	radeon_fence_unref(&rdev->vm_manager.active[vm->id]);
	rdev->vm_manager.active[vm->id] = radeon_fence_ref(fence);

	radeon_fence_unref(&vm->last_id_use);
	vm->last_id_use = radeon_fence_ref(fence);

        /* we just flushed the VM, remember that */
        if (!vm->last_flush)
                vm->last_flush = radeon_fence_ref(fence);
}

/**
 * radeon_vm_bo_find - find the bo_va for a specific vm & bo
 *
 * @vm: requested vm
 * @bo: requested buffer object
 *
 * Find @bo inside the requested vm (cayman+).
 * Search inside the @bos vm list for the requested vm
 * Returns the found bo_va or NULL if none is found
 *
 * Object has to be reserved!
 */
struct radeon_bo_va *radeon_vm_bo_find(struct radeon_vm *vm,
				       struct radeon_bo *bo)
{
	struct radeon_bo_va *bo_va;

	list_for_each_entry(bo_va, &bo->va, bo_list) {
		if (bo_va->vm == vm) {
			return bo_va;
		}
	}
	return NULL;
}

/**
 * radeon_vm_bo_add - add a bo to a specific vm
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 *
 * Add @bo into the requested vm (cayman+).
 * Add @bo to the list of bos associated with the vm
 * Returns newly added bo_va or NULL for failure
 *
 * Object has to be reserved!
 */
struct radeon_bo_va *radeon_vm_bo_add(struct radeon_device *rdev,
				      struct radeon_vm *vm,
				      struct radeon_bo *bo)
{
	struct radeon_bo_va *bo_va;

	bo_va = kzalloc(sizeof(struct radeon_bo_va), GFP_KERNEL);
	if (bo_va == NULL) {
		return NULL;
	}
	bo_va->vm = vm;
	bo_va->bo = bo;
	bo_va->soffset = 0;
	bo_va->eoffset = 0;
	bo_va->flags = 0;
	bo_va->valid = false;
	bo_va->ref_count = 1;
	INIT_LIST_HEAD(&bo_va->bo_list);
	INIT_LIST_HEAD(&bo_va->vm_list);

	mutex_lock(&vm->mutex);
	list_add(&bo_va->vm_list, &vm->va);
	list_add_tail(&bo_va->bo_list, &bo->va);
	mutex_unlock(&vm->mutex);

	return bo_va;
}

/**
 * radeon_vm_clear_bo - initially clear the page dir/table
 *
 * @rdev: radeon_device pointer
 * @bo: bo to clear
 */
static int radeon_vm_clear_bo(struct radeon_device *rdev,
			      struct radeon_bo *bo)
{
        struct ttm_validate_buffer tv;
        struct ww_acquire_ctx ticket;
        struct list_head head;
	struct radeon_ib ib;
	unsigned entries;
	uint64_t addr;
	int r;

        memset(&tv, 0, sizeof(tv));
        tv.bo = &bo->tbo;

        INIT_LIST_HEAD(&head);
        list_add(&tv.head, &head);

        r = ttm_eu_reserve_buffers(&ticket, &head);
        if (r)
		return r;

        r = ttm_bo_validate(&bo->tbo, &bo->placement, true, false);
        if (r)
                goto error;

	addr = radeon_bo_gpu_offset(bo);
	entries = radeon_bo_size(bo) / 8;

	r = radeon_ib_get(rdev, R600_RING_TYPE_DMA_INDEX, &ib,
			  NULL, entries * 2 + 64);
	if (r)
                goto error;

	ib.length_dw = 0;

	radeon_asic_vm_set_page(rdev, &ib, addr, 0, entries, 0, 0);

	r = radeon_ib_schedule(rdev, &ib, NULL);
	if (r)
                goto error;

	ttm_eu_fence_buffer_objects(&ticket, &head, ib.fence);
	radeon_ib_free(rdev, &ib);

	return 0;

error:
	ttm_eu_backoff_reservation(&ticket, &head);
	return r;
}

/**
 * radeon_vm_bo_set_addr - set bos virtual address inside a vm
 *
 * @rdev: radeon_device pointer
 * @bo_va: bo_va to store the address
 * @soffset: requested offset of the buffer in the VM address space
 * @flags: attributes of pages (read/write/valid/etc.)
 *
 * Set offset of @bo_va (cayman+).
 * Validate and set the offset requested within the vm address space.
 * Returns 0 for success, error for failure.
 *
 * Object has to be reserved!
 */
int radeon_vm_bo_set_addr(struct radeon_device *rdev,
			  struct radeon_bo_va *bo_va,
			  uint64_t soffset,
			  uint32_t flags)
{
	uint64_t size = radeon_bo_size(bo_va->bo);
	uint64_t eoffset, last_offset = 0;
	struct radeon_vm *vm = bo_va->vm;
	struct radeon_bo_va *tmp;
	struct list_head *head;
	unsigned last_pfn, pt_idx;
	int r;

	if (soffset) {
		/* make sure object fit at this offset */
		eoffset = soffset + size;
		if (soffset >= eoffset) {
			return -EINVAL;
		}

		last_pfn = eoffset / RADEON_GPU_PAGE_SIZE;
		if (last_pfn > rdev->vm_manager.max_pfn) {
			dev_err(rdev->dev, "va above limit (0x%08X > 0x%08X)\n",
				last_pfn, rdev->vm_manager.max_pfn);
			return -EINVAL;
		}

	} else {
		eoffset = last_pfn = 0;
	}

	mutex_lock(&vm->mutex);
	head = &vm->va;
	last_offset = 0;
	list_for_each_entry(tmp, &vm->va, vm_list) {
		if (bo_va == tmp) {
			/* skip over currently modified bo */
			continue;
		}

		if (soffset >= last_offset && eoffset <= tmp->soffset) {
			/* bo can be added before this one */
			break;
		}
		if (eoffset > tmp->soffset && soffset < tmp->eoffset) {
			/* bo and tmp overlap, invalid offset */
			dev_err(rdev->dev, "bo %p va 0x%08X conflict with (bo %p 0x%08X 0x%08X)\n",
				bo_va->bo, (unsigned)bo_va->soffset, tmp->bo,
				(unsigned)tmp->soffset, (unsigned)tmp->eoffset);
			mutex_unlock(&vm->mutex);
			return -EINVAL;
		}
		last_offset = tmp->eoffset;
		head = &tmp->vm_list;
	}

	bo_va->soffset = soffset;
	bo_va->eoffset = eoffset;
	bo_va->flags = flags;
	bo_va->valid = false;
	list_move(&bo_va->vm_list, head);

	soffset = (soffset / RADEON_GPU_PAGE_SIZE) >> RADEON_VM_BLOCK_SIZE;
	eoffset = (eoffset / RADEON_GPU_PAGE_SIZE) >> RADEON_VM_BLOCK_SIZE;

	if (eoffset > vm->max_pde_used)
		vm->max_pde_used = eoffset;

	radeon_bo_unreserve(bo_va->bo);

	/* walk over the address space and allocate the page tables */
	for (pt_idx = soffset; pt_idx <= eoffset; ++pt_idx) {
		struct radeon_bo *pt;

		if (vm->page_tables[pt_idx].bo)
			continue;

		/* drop mutex to allocate and clear page table */
		mutex_unlock(&vm->mutex);

		r = radeon_bo_create(rdev, RADEON_VM_PTE_COUNT * 8,
				     RADEON_GPU_PAGE_SIZE, false, 
				     RADEON_GEM_DOMAIN_VRAM, NULL, &pt);
		if (r)
			return r;

		r = radeon_vm_clear_bo(rdev, pt);
		if (r) {
			radeon_bo_unref(&pt);
			radeon_bo_reserve(bo_va->bo, false);
			return r;
		}

		/* aquire mutex again */
		mutex_lock(&vm->mutex);
		if (vm->page_tables[pt_idx].bo) {
			/* someone else allocated the pt in the meantime */
			mutex_unlock(&vm->mutex);
			radeon_bo_unref(&pt);
			mutex_lock(&vm->mutex);
			continue;
		}

		vm->page_tables[pt_idx].addr = 0;
		vm->page_tables[pt_idx].bo = pt;
	}

	mutex_unlock(&vm->mutex);
	return radeon_bo_reserve(bo_va->bo, false);
}

/**
 * radeon_vm_map_gart - get the physical address of a gart page
 *
 * @rdev: radeon_device pointer
 * @addr: the unmapped addr
 *
 * Look up the physical address of the page that the pte resolves
 * to (cayman+).
 * Returns the physical address of the page.
 */
uint64_t radeon_vm_map_gart(struct radeon_device *rdev, uint64_t addr)
{
	uint64_t result;

	/* page table offset */
	result = rdev->gart.pages_addr[addr >> PAGE_SHIFT];

	/* in case cpu page size != gpu page size*/
	result |= addr & (~PAGE_MASK);

	return result;
}

/**
 * radeon_vm_page_flags - translate page flags to what the hw uses
 *
 * @flags: flags comming from userspace
 *
 * Translate the flags the userspace ABI uses to hw flags.
 */
static uint32_t radeon_vm_page_flags(uint32_t flags)
{
        uint32_t hw_flags = 0;
        hw_flags |= (flags & RADEON_VM_PAGE_VALID) ? R600_PTE_VALID : 0;
        hw_flags |= (flags & RADEON_VM_PAGE_READABLE) ? R600_PTE_READABLE : 0;
        hw_flags |= (flags & RADEON_VM_PAGE_WRITEABLE) ? R600_PTE_WRITEABLE : 0;
        if (flags & RADEON_VM_PAGE_SYSTEM) {
                hw_flags |= R600_PTE_SYSTEM;
                hw_flags |= (flags & RADEON_VM_PAGE_SNOOPED) ? R600_PTE_SNOOPED : 0;
        }
        return hw_flags;
}

/**
 * radeon_vm_update_pdes - make sure that page directory is valid
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @start: start of GPU address range
 * @end: end of GPU address range
 *
 * Allocates new page tables if necessary
 * and updates the page directory (cayman+).
 * Returns 0 for success, error for failure.
 *
 * Global and local mutex must be locked!
 */
int radeon_vm_update_page_directory(struct radeon_device *rdev,
				    struct radeon_vm *vm)
{
	static const uint32_t incr = RADEON_VM_PTE_COUNT * 8;

	struct radeon_bo *pd = vm->page_directory;
	uint64_t pd_addr = radeon_bo_gpu_offset(pd);
	uint64_t last_pde = ~0, last_pt = ~0;
	unsigned count = 0, pt_idx, ndw;
	struct radeon_ib ib;
	int r;

	/* padding, etc. */
	ndw = 64;

	/* assume the worst case */
	ndw += vm->max_pde_used * 16;

	/* update too big for an IB */
	if (ndw > 0xfffff)
		return -ENOMEM;

	r = radeon_ib_get(rdev, R600_RING_TYPE_DMA_INDEX, &ib, NULL, ndw * 4);
	if (r)
		return r;
	ib.length_dw = 0;

	/* walk over the address space and update the page directory */
	for (pt_idx = 0; pt_idx <= vm->max_pde_used; ++pt_idx) {
		struct radeon_bo *bo = vm->page_tables[pt_idx].bo;
		uint64_t pde, pt;

		if (bo == NULL)
			continue;

		pt = radeon_bo_gpu_offset(bo);
		if (vm->page_tables[pt_idx].addr == pt)
			continue;
		vm->page_tables[pt_idx].addr = pt;

		pde = pd_addr + pt_idx * 8;
		if (((last_pde + 8 * count) != pde) ||
		    ((last_pt + incr * count) != pt)) {

			if (count) {
				radeon_asic_vm_set_page(rdev, &ib, last_pde,
							last_pt, count, incr,
							R600_PTE_VALID);
			}

			count = 1;
			last_pde = pde;
			last_pt = pt;
		} else {
			++count;
		}
	}

	if (count)
		radeon_asic_vm_set_page(rdev, &ib, last_pde, last_pt, count,
					incr, R600_PTE_VALID);

	if (ib.length_dw != 0) {
		radeon_semaphore_sync_to(ib.semaphore, pd->tbo.sync_obj);
		radeon_semaphore_sync_to(ib.semaphore, vm->last_id_use);
		r = radeon_ib_schedule(rdev, &ib, NULL);
		if (r) {
			radeon_ib_free(rdev, &ib);
			return r;
		}
		radeon_fence_unref(&vm->fence);
		vm->fence = radeon_fence_ref(ib.fence);
		radeon_fence_unref(&vm->last_flush);
	}
	radeon_ib_free(rdev, &ib);

	return 0;
}

/**
 * radeon_vm_update_ptes - make sure that page tables are valid
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @start: start of GPU address range
 * @end: end of GPU address range
 * @dst: destination address to map to
 * @flags: mapping flags
 *
 * Update the page tables in the range @start - @end (cayman+).
 *
 * Global and local mutex must be locked!
 */
static void radeon_vm_update_ptes(struct radeon_device *rdev,
				  struct radeon_vm *vm,
				  struct radeon_ib *ib,
				  uint64_t start, uint64_t end,
				  uint64_t dst, uint32_t flags)
{
	static const uint64_t mask = RADEON_VM_PTE_COUNT - 1;

	uint64_t last_pte = ~0, last_dst = ~0;
	unsigned count = 0;
	uint64_t addr;

	start = start / RADEON_GPU_PAGE_SIZE;
	end = end / RADEON_GPU_PAGE_SIZE;

	/* walk over the address space and update the page tables */
	for (addr = start; addr < end; ) {
		uint64_t pt_idx = addr >> RADEON_VM_BLOCK_SIZE;
		struct radeon_bo *pt = vm->page_tables[pt_idx].bo;
		unsigned nptes;
		uint64_t pte;

		radeon_semaphore_sync_to(ib->semaphore, pt->tbo.sync_obj);

		if ((addr & ~mask) == (end & ~mask))
			nptes = end - addr;
		else
			nptes = RADEON_VM_PTE_COUNT - (addr & mask);

		pte = radeon_bo_gpu_offset(pt);
		pte += (addr & mask) * 8;

		if ((last_pte + 8 * count) != pte) {

			if (count) {
				radeon_asic_vm_set_page(rdev, ib, last_pte,
							last_dst, count,
							RADEON_GPU_PAGE_SIZE,
							flags);
			}

			count = nptes;
			last_pte = pte;
			last_dst = dst;
		} else {
			count += nptes;
		}

		addr += nptes;
		dst += nptes * RADEON_GPU_PAGE_SIZE;
	}

	if (count) {
		radeon_asic_vm_set_page(rdev, ib, last_pte,
					last_dst, count,
					RADEON_GPU_PAGE_SIZE, flags);
	}
}

/**
 * radeon_vm_bo_update - map a bo into the vm page table
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 * @mem: ttm mem
 *
 * Fill in the page table entries for @bo (cayman+).
 * Returns 0 for success, -EINVAL for failure.
 *
 * Object have to be reserved and mutex must be locked!
 */
int radeon_vm_bo_update(struct radeon_device *rdev,
			struct radeon_vm *vm,
			struct radeon_bo *bo,
			struct ttm_mem_reg *mem)
{
	struct radeon_ib ib;
	struct radeon_bo_va *bo_va;
	unsigned nptes, ndw;
	uint64_t addr;
	int r;

	bo_va = radeon_vm_bo_find(vm, bo);
	if (bo_va == NULL) {
		dev_err(rdev->dev, "bo %p not in vm %p\n", bo, vm);
		return -EINVAL;
	}

	if (!bo_va->soffset) {
		dev_err(rdev->dev, "bo %p don't has a mapping in vm %p\n",
			bo, vm);
		return -EINVAL;
	}

	if ((bo_va->valid && mem) || (!bo_va->valid && mem == NULL))
		return 0;

	bo_va->flags &= ~RADEON_VM_PAGE_VALID;
	bo_va->flags &= ~RADEON_VM_PAGE_SYSTEM;
	if (mem) {
		addr = mem->start << PAGE_SHIFT;
		if (mem->mem_type != TTM_PL_SYSTEM) {
			bo_va->flags |= RADEON_VM_PAGE_VALID;
			bo_va->valid = true;
		}
		if (mem->mem_type == TTM_PL_TT) {
			bo_va->flags |= RADEON_VM_PAGE_SYSTEM;
		} else {
			addr += rdev->vm_manager.vram_base_offset;
		}
	} else {
		addr = 0;
		bo_va->valid = false;
	}

	trace_radeon_vm_bo_update(bo_va);

	nptes = radeon_bo_ngpu_pages(bo);

	/* padding, etc. */
	ndw = 64;

	if (RADEON_VM_BLOCK_SIZE > 11)
		/* reserve space for one header for every 2k dwords */
		ndw += (nptes >> 11) * 4;
	else
		/* reserve space for one header for
		    every (1 << BLOCK_SIZE) entries */
		ndw += (nptes >> RADEON_VM_BLOCK_SIZE) * 4;

	/* reserve space for pte addresses */
	ndw += nptes * 2;

	/* update too big for an IB */
	if (ndw > 0xfffff)
		return -ENOMEM;

	r = radeon_ib_get(rdev, R600_RING_TYPE_DMA_INDEX, &ib, NULL, ndw * 4);
	if (r)
		return r;
	ib.length_dw = 0;

	radeon_vm_update_ptes(rdev, vm, &ib, bo_va->soffset, bo_va->eoffset,
			      addr, radeon_vm_page_flags(bo_va->flags));

	radeon_semaphore_sync_to(ib.semaphore, vm->fence);
	r = radeon_ib_schedule(rdev, &ib, NULL);
	if (r) {
		radeon_ib_free(rdev, &ib);
		return r;
	}
	radeon_fence_unref(&vm->fence);
	vm->fence = radeon_fence_ref(ib.fence);
	radeon_ib_free(rdev, &ib);
	radeon_fence_unref(&vm->last_flush);

	return 0;
}

/**
 * radeon_vm_bo_rmv - remove a bo to a specific vm
 *
 * @rdev: radeon_device pointer
 * @bo_va: requested bo_va
 *
 * Remove @bo_va->bo from the requested vm (cayman+).
 * Remove @bo_va->bo from the list of bos associated with the bo_va->vm and
 * remove the ptes for @bo_va in the page table.
 * Returns 0 for success.
 *
 * Object have to be reserved!
 */
int radeon_vm_bo_rmv(struct radeon_device *rdev,
		     struct radeon_bo_va *bo_va)
{
	int r = 0;

	mutex_lock(&bo_va->vm->mutex);
	if (bo_va->soffset)
		r = radeon_vm_bo_update(rdev, bo_va->vm, bo_va->bo, NULL);

	list_del(&bo_va->vm_list);
	mutex_unlock(&bo_va->vm->mutex);
	list_del(&bo_va->bo_list);

	kfree(bo_va);
	return r;
}

/**
 * radeon_vm_bo_invalidate - mark the bo as invalid
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 * @bo: radeon buffer object
 *
 * Mark @bo as invalid (cayman+).
 */
void radeon_vm_bo_invalidate(struct radeon_device *rdev,
			     struct radeon_bo *bo)
{
	struct radeon_bo_va *bo_va;

	list_for_each_entry(bo_va, &bo->va, bo_list) {
		bo_va->valid = false;
	}
}

/**
 * radeon_vm_init - initialize a vm instance
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 *
 * Init @vm fields (cayman+).
 */
int radeon_vm_init(struct radeon_device *rdev, struct radeon_vm *vm)
{
	unsigned pd_size, pd_entries, pts_size;
	int r;

	vm->id = 0;
	vm->fence = NULL;
	vm->last_flush = NULL;
	vm->last_id_use = NULL;
	mutex_init(&vm->mutex);
	INIT_LIST_HEAD(&vm->va);

	pd_size = radeon_vm_directory_size(rdev);
	pd_entries = radeon_vm_num_pdes(rdev);

	/* allocate page table array */
	pts_size = pd_entries * sizeof(struct radeon_vm_pt);
	vm->page_tables = kzalloc(pts_size, GFP_KERNEL);
	if (vm->page_tables == NULL) {
		DRM_ERROR("Cannot allocate memory for page table array\n");
		return -ENOMEM;
	}

	r = radeon_bo_create(rdev, pd_size, RADEON_VM_PTB_ALIGN_SIZE, false,
			     RADEON_GEM_DOMAIN_VRAM, NULL,
			     &vm->page_directory);
	if (r)
		return r;

	r = radeon_vm_clear_bo(rdev, vm->page_directory);
	if (r) {
		radeon_bo_unref(&vm->page_directory);
		vm->page_directory = NULL;
		return r;
	}

	return 0;
}

/**
 * radeon_vm_fini - tear down a vm instance
 *
 * @rdev: radeon_device pointer
 * @vm: requested vm
 *
 * Tear down @vm (cayman+).
 * Unbind the VM and remove all bos from the vm bo list
 */
void radeon_vm_fini(struct radeon_device *rdev, struct radeon_vm *vm)
{
	struct radeon_bo_va *bo_va, *tmp;
	int i, r;

	if (!list_empty(&vm->va)) {
		dev_err(rdev->dev, "still active bo inside vm\n");
	}
	list_for_each_entry_safe(bo_va, tmp, &vm->va, vm_list) {
		list_del_init(&bo_va->vm_list);
		r = radeon_bo_reserve(bo_va->bo, false);
		if (!r) {
			list_del_init(&bo_va->bo_list);
			radeon_bo_unreserve(bo_va->bo);
			kfree(bo_va);
		}
	}


	for (i = 0; i < radeon_vm_num_pdes(rdev); i++)
		radeon_bo_unref(&vm->page_tables[i].bo);
	kfree(vm->page_tables);

	radeon_bo_unref(&vm->page_directory);

	radeon_fence_unref(&vm->fence);
	radeon_fence_unref(&vm->last_flush);
	radeon_fence_unref(&vm->last_id_use);

	mutex_destroy(&vm->mutex);
}
