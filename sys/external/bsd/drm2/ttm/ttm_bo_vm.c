/*	$NetBSD: ttm_bo_vm.c,v 1.3.2.3 2017/12/03 11:38:01 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ttm_bo_vm.c,v 1.3.2.3 2017/12/03 11:38:01 jdolecek Exp $");

#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_fault.h>

#include <linux/bitops.h>

#include <drm/drm_vma_manager.h>

#include <ttm/ttm_bo_driver.h>

static int	ttm_bo_uvm_fault_idle(struct ttm_buffer_object *,
		    struct uvm_faultinfo *);
static int	ttm_bo_uvm_lookup(struct ttm_bo_device *, unsigned long,
		    unsigned long, struct ttm_buffer_object **);

void
ttm_bo_uvm_reference(struct uvm_object *uobj)
{
	struct ttm_buffer_object *const bo = container_of(uobj,
	    struct ttm_buffer_object, uvmobj);

	(void)ttm_bo_reference(bo);
}

void
ttm_bo_uvm_detach(struct uvm_object *uobj)
{
	struct ttm_buffer_object *bo = container_of(uobj,
	    struct ttm_buffer_object, uvmobj);

	ttm_bo_unref(&bo);
	KASSERT(bo == NULL);
}

int
ttm_bo_uvm_fault(struct uvm_faultinfo *ufi, vaddr_t vaddr,
    struct vm_page **pps, int npages, int centeridx, vm_prot_t access_type,
    int flags)
{
	struct uvm_object *const uobj = ufi->entry->object.uvm_obj;
	struct ttm_buffer_object *const bo = container_of(uobj,
	    struct ttm_buffer_object, uvmobj);
	struct ttm_bo_device *const bdev = bo->bdev;
	struct ttm_mem_type_manager *man =
	    &bdev->man[bo->mem.mem_type];
	union {
		bus_addr_t base;
		struct ttm_tt *ttm;
	} u;
	size_t size __diagused;
	voff_t uoffset;		/* offset in bytes into bo */
	unsigned startpage;	/* offset in pages into bo */
	unsigned i;
	vm_prot_t vm_prot;	/* VM_PROT_* */
	pgprot_t pgprot;	/* VM_PROT_* | PMAP_* cacheability flags */
	unsigned mmapflags;
	int ret;

	/* Thanks, uvm, but we don't need this lock.  */
	mutex_exit(uobj->vmobjlock);

	/* Copy-on-write mappings make no sense for the graphics aperture.  */
	if (UVM_ET_ISCOPYONWRITE(ufi->entry)) {
		ret = -EIO;
		goto out0;
	}

	/* Try to lock the buffer.  */
	ret = ttm_bo_reserve(bo, true, true, false, NULL);
	if (ret) {
		if (ret != -EBUSY)
			goto out0;
		/*
		 * It's currently locked.  Unlock the fault, wait for
		 * it, and start over.
		 */
		uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
		(void)ttm_bo_wait_unreserved(bo);
		return -ERESTART;
	}

	/* drm prime buffers are not mappable.  XXX Catch this earlier?  */
	if (bo->ttm && ISSET(bo->ttm->page_flags, TTM_PAGE_FLAG_SG)) {
		ret = -EINVAL;
		goto out1;
	}

	/* Notify the driver of a fault if it wants.  */
	if (bdev->driver->fault_reserve_notify) {
		ret = (*bdev->driver->fault_reserve_notify)(bo);
		if (ret) {
			if (ret == -ERESTART)
				ret = -EIO;
			goto out1;
		}
	}

	ret = ttm_bo_uvm_fault_idle(bo, ufi);
	if (ret) {
		KASSERT(ret == -ERESTART);
		/* ttm_bo_uvm_fault_idle calls uvmfault_unlockall for us.  */
		ttm_bo_unreserve(bo);
		/* XXX errno Linux->NetBSD */
		return -ret;
	}

	ret = ttm_mem_io_lock(man, true);
	if (ret) {
		ret = -EIO;
		goto out1;
	}
	ret = ttm_mem_io_reserve_vm(bo);
	if (ret) {
		ret = -EIO;
		goto out2;
	}

	vm_prot = ufi->entry->protection;
	if (bo->mem.bus.is_iomem) {
		u.base = (bo->mem.bus.base + bo->mem.bus.offset);
		size = bo->mem.bus.size;
		pgprot = ttm_io_prot(bo->mem.placement, vm_prot);
	} else {
		u.ttm = bo->ttm;
		size = (bo->ttm->num_pages << PAGE_SHIFT);
		if (ISSET(bo->mem.placement, TTM_PL_FLAG_CACHED))
			pgprot = vm_prot;
		else
			pgprot = ttm_io_prot(bo->mem.placement, vm_prot);
		if ((*u.ttm->bdev->driver->ttm_tt_populate)(u.ttm)) {
			ret = -ENOMEM;
			goto out2;
		}
	}

	KASSERT(ufi->entry->start <= vaddr);
	KASSERT((ufi->entry->offset & (PAGE_SIZE - 1)) == 0);
	KASSERT(ufi->entry->offset <= size);
	KASSERT((vaddr - ufi->entry->start) <= (size - ufi->entry->offset));
	KASSERT(npages <= ((size - ufi->entry->offset) -
		(vaddr - ufi->entry->start)));
	uoffset = (ufi->entry->offset + (vaddr - ufi->entry->start));
	startpage = (uoffset >> PAGE_SHIFT);
	for (i = 0; i < npages; i++) {
		paddr_t paddr;

		/* XXX PGO_ALLPAGES?  */
		if (pps[i] == PGO_DONTCARE)
			continue;
		if (bo->mem.bus.is_iomem) {
			const paddr_t cookie = bus_space_mmap(bdev->memt,
			    u.base, ((startpage + i) << PAGE_SHIFT), vm_prot,
			    0);

			paddr = pmap_phys_address(cookie);
			mmapflags = pmap_mmap_flags(cookie);
		} else {
			paddr = page_to_phys(u.ttm->pages[startpage + i]);
			mmapflags = 0;
		}
		ret = -pmap_enter(ufi->orig_map->pmap, vaddr + i*PAGE_SIZE,
		    paddr, vm_prot, (PMAP_CANFAIL | pgprot | mmapflags));
		if (ret)
			goto out3;
	}

out3:	pmap_update(ufi->orig_map->pmap);
out2:	ttm_mem_io_unlock(man);
out1:	ttm_bo_unreserve(bo);
out0:	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
	/* XXX errno Linux->NetBSD */
	return -ret;
}

static int
ttm_bo_uvm_fault_idle(struct ttm_buffer_object *bo, struct uvm_faultinfo *ufi)
{
	struct ttm_bo_device *const bdev = bo->bdev;
	int ret = 0;

	spin_lock(&bdev->fence_lock);
	if (__predict_true(!test_bit(TTM_BO_PRIV_FLAG_MOVING,
		    &bo->priv_flags)))
		goto out;
	if (ttm_bo_wait(bo, false, false, true) == 0)
		goto out;

	uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
	(void)ttm_bo_wait(bo, false, true, false);
	ret = -ERESTART;

out:	spin_unlock(&bdev->fence_lock);
	return ret;
}

int
ttm_bo_mmap_object(struct ttm_bo_device *bdev, off_t offset, size_t size,
    vm_prot_t prot, struct uvm_object **uobjp, voff_t *uoffsetp,
    struct file *file)
{
	const unsigned long startpage = (offset >> PAGE_SHIFT);
	const unsigned long npages = (size >> PAGE_SHIFT);
	struct ttm_buffer_object *bo;
	int ret;

	KASSERT(0 == (offset & (PAGE_SIZE - 1)));
	KASSERT(0 == (size & (PAGE_SIZE - 1)));

	ret = ttm_bo_uvm_lookup(bdev, startpage, npages, &bo);
	if (ret)
		goto fail0;
	KASSERT(drm_vma_node_start(&bo->vma_node) <= offset);
	/* XXX Just assert this?  */
	if (__predict_false(bdev->driver->verify_access == NULL)) {
		ret = -EPERM;
		goto fail1;
	}
	ret = (*bdev->driver->verify_access)(bo, file);
	if (ret)
		goto fail1;

	/* Success!  */
	*uobjp = &bo->uvmobj;
	*uoffsetp = (offset -
	    (drm_vma_node_start(&bo->vma_node) << PAGE_SHIFT));
	return 0;

fail1:	ttm_bo_unref(&bo);
fail0:	KASSERT(ret);
	return ret;
}

static int
ttm_bo_uvm_lookup(struct ttm_bo_device *bdev, unsigned long startpage,
    unsigned long npages, struct ttm_buffer_object **bop)
{
	struct ttm_buffer_object *bo = NULL;
	struct drm_vma_offset_node *node;

	drm_vma_offset_lock_lookup(&bdev->vma_manager);
	node = drm_vma_offset_lookup_locked(&bdev->vma_manager, startpage,
	    npages);
	if (node != NULL) {
		bo = container_of(node, struct ttm_buffer_object, vma_node);
		if (!kref_get_unless_zero(&bo->kref))
			bo = NULL;
	}
	drm_vma_offset_unlock_lookup(&bdev->vma_manager);

	if (bo == NULL)
		return -ENOENT;

	*bop = bo;
	return 0;
}
