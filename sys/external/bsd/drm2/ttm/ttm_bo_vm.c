/*	$NetBSD: ttm_bo_vm.c,v 1.22.4.1 2024/10/04 11:40:53 martin Exp $	*/

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

/**************************************************************************
 *
 * Copyright (c) 2006-2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Authors: Thomas Hellstrom <thellstrom-at-vmware-dot-com>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ttm_bo_vm.c,v 1.22.4.1 2024/10/04 11:40:53 martin Exp $");

#include <sys/types.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_fault.h>

#include <linux/bitops.h>

#include <drm/drm_vma_manager.h>

#include <ttm/ttm_bo_driver.h>

static int	ttm_bo_uvm_lookup(struct ttm_bo_device *, unsigned long,
		    unsigned long, struct ttm_buffer_object **);

void
ttm_bo_uvm_reference(struct uvm_object *uobj)
{
	struct ttm_buffer_object *const bo = container_of(uobj,
	    struct ttm_buffer_object, uvmobj);

	(void)ttm_bo_get(bo);
}

void
ttm_bo_uvm_detach(struct uvm_object *uobj)
{
	struct ttm_buffer_object *bo = container_of(uobj,
	    struct ttm_buffer_object, uvmobj);

	ttm_bo_put(bo);
}

static int
ttm_bo_vm_fault_idle(struct ttm_buffer_object *bo, struct uvm_faultinfo *vmf)
{
	int err, ret = 0;

	if (__predict_true(!bo->moving))
		goto out_unlock;

	/*
	 * Quick non-stalling check for idle.
	 */
	if (dma_fence_is_signaled(bo->moving))
		goto out_clear;

	/*
	 * If possible, avoid waiting for GPU with mmap_sem
	 * held.
	 */
	if (1) {		/* always retriable in NetBSD */
		ret = ERESTART;

		ttm_bo_get(bo);
		uvmfault_unlockall(vmf, vmf->entry->aref.ar_amap, NULL);
		(void) dma_fence_wait(bo->moving, true);
		dma_resv_unlock(bo->base.resv);
		ttm_bo_put(bo);
		goto out_unlock;
	}

	/*
	 * Ordinary wait.
	 */
	err = dma_fence_wait(bo->moving, true);
	if (__predict_false(err != 0)) {
		ret = (err != -ERESTARTSYS) ? EINVAL/*SIGBUS*/ :
		    0/*retry access in userland*/;
		goto out_unlock;
	}

out_clear:
	dma_fence_put(bo->moving);
	bo->moving = NULL;

out_unlock:
	return ret;
}

static int
ttm_bo_vm_reserve(struct ttm_buffer_object *bo, struct uvm_faultinfo *vmf)
{

	/*
	 * Work around locking order reversal in fault / nopfn
	 * between mmap_sem and bo_reserve: Perform a trylock operation
	 * for reserve, and if it fails, retry the fault after waiting
	 * for the buffer to become unreserved.
	 */
	if (__predict_false(!dma_resv_trylock(bo->base.resv))) {
		ttm_bo_get(bo);
		uvmfault_unlockall(vmf, vmf->entry->aref.ar_amap, NULL);
		if (!dma_resv_lock_interruptible(bo->base.resv, NULL))
			dma_resv_unlock(bo->base.resv);
		ttm_bo_put(bo);
		return ERESTART;
	}

	return 0;
}

static int
ttm_bo_uvm_fault_reserved(struct uvm_faultinfo *vmf, vaddr_t vaddr,
    struct vm_page **pps, int npages, int centeridx, vm_prot_t access_type,
    int flags)
{
	struct uvm_object *const uobj = vmf->entry->object.uvm_obj;
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
	vm_prot_t vm_prot = vmf->entry->protection; /* VM_PROT_* */
	pgprot_t prot = vm_prot; /* VM_PROT_* | PMAP_* cacheability flags */
	int err, ret;

	/*
	 * Refuse to fault imported pages. This should be handled
	 * (if at all) by redirecting mmap to the exporter.
	 */
	if (bo->ttm && (bo->ttm->page_flags & TTM_PAGE_FLAG_SG))
		return EINVAL;	/* SIGBUS */

	if (bdev->driver->fault_reserve_notify) {
		struct dma_fence *moving = dma_fence_get(bo->moving);

		err = bdev->driver->fault_reserve_notify(bo);
		switch (err) {
		case 0:
			break;
		case -EBUSY:
		case -ERESTARTSYS:
			return 0;	/* retry access in userland */
		default:
			return EINVAL;	/* SIGBUS */
		}

		if (bo->moving != moving) {
			spin_lock(&ttm_bo_glob.lru_lock);
			ttm_bo_move_to_lru_tail(bo, NULL);
			spin_unlock(&ttm_bo_glob.lru_lock);
		}
		dma_fence_put(moving);
	}

	/*
	 * Wait for buffer data in transit, due to a pipelined
	 * move.
	 */
	ret = ttm_bo_vm_fault_idle(bo, vmf);
	if (__predict_false(ret != 0))
		return ret;

	err = ttm_mem_io_lock(man, true);
	if (__predict_false(err != 0))
		return 0;	/* retry access in userland */
	err = ttm_mem_io_reserve_vm(bo);
	if (__predict_false(err != 0)) {
		ret = EINVAL;	/* SIGBUS */
		goto out_io_unlock;
	}

	prot = ttm_io_prot(bo->mem.placement, prot);
	if (!bo->mem.bus.is_iomem) {
		struct ttm_operation_ctx ctx = {
			.interruptible = false,
			.no_wait_gpu = false,
			.flags = TTM_OPT_FLAG_FORCE_ALLOC

		};

		u.ttm = bo->ttm;
		size = (size_t)bo->ttm->num_pages << PAGE_SHIFT;
		if (ttm_tt_populate(bo->ttm, &ctx)) {
			ret = ENOMEM;
			goto out_io_unlock;
		}
	} else {
		u.base = (bo->mem.bus.base + bo->mem.bus.offset);
		size = bo->mem.bus.size;
	}

	KASSERT(vmf->entry->start <= vaddr);
	KASSERT((vmf->entry->offset & (PAGE_SIZE - 1)) == 0);
	KASSERT(vmf->entry->offset <= size);
	KASSERT((vaddr - vmf->entry->start) <= (size - vmf->entry->offset));
	KASSERTMSG(((size_t)npages << PAGE_SHIFT <=
		((size - vmf->entry->offset) - (vaddr - vmf->entry->start))),
	    "vaddr=%jx npages=%d bo=%p is_iomem=%d size=%zu"
	    " start=%jx offset=%jx",
	    (uintmax_t)vaddr, npages, bo, (int)bo->mem.bus.is_iomem, size,
	    (uintmax_t)vmf->entry->start, (uintmax_t)vmf->entry->offset);
	uoffset = (vmf->entry->offset + (vaddr - vmf->entry->start));
	startpage = (uoffset >> PAGE_SHIFT);
	for (i = 0; i < npages; i++) {
		paddr_t paddr;

		if ((flags & PGO_ALLPAGES) == 0 && i != centeridx)
			continue;
		if (pps[i] == PGO_DONTCARE)
			continue;
		if (!bo->mem.bus.is_iomem) {
			paddr = page_to_phys(u.ttm->pages[startpage + i]);
		} else if (bdev->driver->io_mem_pfn) {
			paddr = (paddr_t)(*bdev->driver->io_mem_pfn)(bo,
			    startpage + i) << PAGE_SHIFT;
		} else {
			const paddr_t cookie = bus_space_mmap(bdev->memt,
			    u.base, (off_t)(startpage + i) << PAGE_SHIFT,
			    vm_prot, 0);

			paddr = pmap_phys_address(cookie);
#if 0				/* XXX Why no PMAP_* flags added here? */
			mmapflags = pmap_mmap_flags(cookie);
#endif
		}
		ret = pmap_enter(vmf->orig_map->pmap, vaddr + i*PAGE_SIZE,
		    paddr, vm_prot, PMAP_CANFAIL | prot);
		if (ret) {
			/*
			 * XXX Continue with ret=0 if i != centeridx,
			 * so we don't fail if only readahead pages
			 * fail?
			 */
			KASSERT(ret != ERESTART);
			break;
		}
	}
	pmap_update(vmf->orig_map->pmap);
	ret = 0;		/* retry access in userland */
out_io_unlock:
	ttm_mem_io_unlock(man);
	KASSERT(ret != ERESTART);
	return ret;
}

int
ttm_bo_uvm_fault(struct uvm_faultinfo *vmf, vaddr_t vaddr,
    struct vm_page **pps, int npages, int centeridx, vm_prot_t access_type,
    int flags)
{
	struct uvm_object *const uobj = vmf->entry->object.uvm_obj;
	struct ttm_buffer_object *const bo = container_of(uobj,
	    struct ttm_buffer_object, uvmobj);
	int ret;

	/* Thanks, uvm, but we don't need this lock.  */
	rw_exit(uobj->vmobjlock);

	/* Copy-on-write mappings make no sense for the graphics aperture.  */
	if (UVM_ET_ISCOPYONWRITE(vmf->entry)) {
		ret = EINVAL;	/* SIGBUS */
		goto out;
	}

	ret = ttm_bo_vm_reserve(bo, vmf);
	if (ret) {
		/* ttm_bo_vm_reserve already unlocked on ERESTART */
		KASSERTMSG(ret == ERESTART, "ret=%d", ret);
		return ret;
	}

	ret = ttm_bo_uvm_fault_reserved(vmf, vaddr, pps, npages, centeridx,
	    access_type, flags);
	if (ret == ERESTART)	/* already unlocked on ERESTART */
		return ret;

	dma_resv_unlock(bo->base.resv);

out:	uvmfault_unlockall(vmf, vmf->entry->aref.ar_amap, NULL);
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
	KASSERTMSG((drm_vma_node_start(&bo->base.vma_node) <= startpage),
	    "mapping npages=0x%jx @ pfn=0x%jx"
	    " from vma npages=0x%jx @ pfn=0x%jx",
	    (uintmax_t)npages,
	    (uintmax_t)startpage,
	    (uintmax_t)drm_vma_node_size(&bo->base.vma_node),
	    (uintmax_t)drm_vma_node_start(&bo->base.vma_node));
	KASSERTMSG((npages <= drm_vma_node_size(&bo->base.vma_node)),
	    "mapping npages=0x%jx @ pfn=0x%jx"
	    " from vma npages=0x%jx @ pfn=0x%jx",
	    (uintmax_t)npages,
	    (uintmax_t)startpage,
	    (uintmax_t)drm_vma_node_size(&bo->base.vma_node),
	    (uintmax_t)drm_vma_node_start(&bo->base.vma_node));
	KASSERTMSG(((startpage - drm_vma_node_start(&bo->base.vma_node))
		<= (drm_vma_node_size(&bo->base.vma_node) - npages)),
	    "mapping npages=0x%jx @ pfn=0x%jx"
	    " from vma npages=0x%jx @ pfn=0x%jx",
	    (uintmax_t)npages,
	    (uintmax_t)startpage,
	    (uintmax_t)drm_vma_node_size(&bo->base.vma_node),
	    (uintmax_t)drm_vma_node_start(&bo->base.vma_node));

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
	    ((off_t)drm_vma_node_start(&bo->base.vma_node) << PAGE_SHIFT));
	return 0;

fail1:	ttm_bo_put(bo);
fail0:	KASSERT(ret);
	return ret;
}

static int
ttm_bo_uvm_lookup(struct ttm_bo_device *bdev, unsigned long startpage,
    unsigned long npages, struct ttm_buffer_object **bop)
{
	struct ttm_buffer_object *bo = NULL;
	struct drm_vma_offset_node *node;

	drm_vma_offset_lock_lookup(bdev->vma_manager);
	node = drm_vma_offset_lookup_locked(bdev->vma_manager, startpage,
	    npages);
	if (node != NULL) {
		bo = container_of(node, struct ttm_buffer_object,
		    base.vma_node);
		if (!kref_get_unless_zero(&bo->kref))
			bo = NULL;
	}
	drm_vma_offset_unlock_lookup(bdev->vma_manager);

	if (bo == NULL)
		return -ENOENT;

	*bop = bo;
	return 0;
}
