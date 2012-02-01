/*	$NetBSD: uvm_km.c,v 1.115 2012/02/01 02:22:27 matt Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993, The Regents of the University of California.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_kern.c   8.3 (Berkeley) 1/12/94
 * from: Id: uvm_km.c,v 1.1.2.14 1998/02/06 05:19:27 chs Exp
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * uvm_km.c: handle kernel memory allocation and management
 */

/*
 * overview of kernel memory management:
 *
 * the kernel virtual address space is mapped by "kernel_map."   kernel_map
 * starts at VM_MIN_KERNEL_ADDRESS and goes to VM_MAX_KERNEL_ADDRESS.
 * note that VM_MIN_KERNEL_ADDRESS is equal to vm_map_min(kernel_map).
 *
 * the kernel_map has several "submaps."   submaps can only appear in
 * the kernel_map (user processes can't use them).   submaps "take over"
 * the management of a sub-range of the kernel's address space.  submaps
 * are typically allocated at boot time and are never released.   kernel
 * virtual address space that is mapped by a submap is locked by the
 * submap's lock -- not the kernel_map's lock.
 *
 * thus, the useful feature of submaps is that they allow us to break
 * up the locking and protection of the kernel address space into smaller
 * chunks.
 *
 * the vm system has several standard kernel submaps, including:
 *   pager_map => used to map "buf" structures into kernel space
 *   exec_map => used during exec to handle exec args
 *   etc...
 *
 * the kernel allocates its private memory out of special uvm_objects whose
 * reference count is set to UVM_OBJ_KERN (thus indicating that the objects
 * are "special" and never die).   all kernel objects should be thought of
 * as large, fixed-sized, sparsely populated uvm_objects.   each kernel
 * object is equal to the size of kernel virtual address space (i.e. the
 * value "VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS").
 *
 * note that just because a kernel object spans the entire kernel virtual
 * address space doesn't mean that it has to be mapped into the entire space.
 * large chunks of a kernel object's space go unused either because
 * that area of kernel VM is unmapped, or there is some other type of
 * object mapped into that range (e.g. a vnode).    for submap's kernel
 * objects, the only part of the object that can ever be populated is the
 * offsets that are managed by the submap.
 *
 * note that the "offset" in a kernel object is always the kernel virtual
 * address minus the VM_MIN_KERNEL_ADDRESS (aka vm_map_min(kernel_map)).
 * example:
 *   suppose VM_MIN_KERNEL_ADDRESS is 0xf8000000 and the kernel does a
 *   uvm_km_alloc(kernel_map, PAGE_SIZE) [allocate 1 wired down page in the
 *   kernel map].    if uvm_km_alloc returns virtual address 0xf8235000,
 *   then that means that the page at offset 0x235000 in kernel_object is
 *   mapped at 0xf8235000.
 *
 * kernel object have one other special property: when the kernel virtual
 * memory mapping them is unmapped, the backing memory in the object is
 * freed right away.   this is done with the uvm_km_pgremove() function.
 * this has to be done because there is no backing store for kernel pages
 * and no need to save them after they are no longer referenced.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_km.c,v 1.115 2012/02/01 02:22:27 matt Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>
#include <sys/vmem.h>
#include <sys/kmem.h>

#include <uvm/uvm.h>

/*
 * global data structures
 */

struct vm_map *kernel_map = NULL;

/*
 * local data structues
 */

static struct vm_map		kernel_map_store;
static struct vm_map_entry	kernel_image_mapent_store;
static struct vm_map_entry	kernel_kmem_mapent_store;

vaddr_t kmembase;
vsize_t kmemsize;

vmem_t *kmem_arena;
vmem_t *kmem_va_arena;

/*
 * uvm_km_bootstrap: init kernel maps and objects to reflect reality (i.e.
 * KVM already allocated for text, data, bss, and static data structures).
 *
 * => KVM is defined by VM_MIN_KERNEL_ADDRESS/VM_MAX_KERNEL_ADDRESS.
 *    we assume that [vmin -> start] has already been allocated and that
 *    "end" is the end.
 */

void
uvm_km_bootstrap(vaddr_t start, vaddr_t end)
{
	vaddr_t base = VM_MIN_KERNEL_ADDRESS;

	kmemsize = MIN(((((vsize_t)(end - start)) / 3) * 2),
	    ((((vsize_t)uvmexp.npages) * PAGE_SIZE)));
	kmemsize = round_page(kmemsize);

	/*
	 * next, init kernel memory objects.
	 */

	/* kernel_object: for pageable anonymous kernel memory */
	uvm_kernel_object = uao_create(VM_MAX_KERNEL_ADDRESS -
				VM_MIN_KERNEL_ADDRESS, UAO_FLAG_KERNOBJ);

	/*
	 * init the map and reserve any space that might already
	 * have been allocated kernel space before installing.
	 */

	uvm_map_setup(&kernel_map_store, base, end, VM_MAP_PAGEABLE);
	kernel_map_store.pmap = pmap_kernel();
	if (start != base) {
		int error;
		struct uvm_map_args args;

		error = uvm_map_prepare(&kernel_map_store,
		    base, start - base,
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
		    		UVM_ADV_RANDOM, UVM_FLAG_FIXED), &args);
		if (!error) {
			kernel_image_mapent_store.flags =
			    UVM_MAP_KERNEL | UVM_MAP_STATIC | UVM_MAP_NOMERGE;
			error = uvm_map_enter(&kernel_map_store, &args,
			    &kernel_image_mapent_store);
		}

		if (error)
			panic(
			    "uvm_km_bootstrap: could not reserve space for kernel");

		kmembase = args.uma_start + args.uma_size;
		error = uvm_map_prepare(&kernel_map_store,
		    kmembase, kmemsize,
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
		    		UVM_ADV_RANDOM, UVM_FLAG_FIXED), &args);
		if (!error) {
			kernel_kmem_mapent_store.flags =
			    UVM_MAP_KERNEL | UVM_MAP_STATIC | UVM_MAP_NOMERGE;
			error = uvm_map_enter(&kernel_map_store, &args,
			    &kernel_kmem_mapent_store);
		}

		if (error)
			panic(
			    "uvm_km_bootstrap: could not reserve kernel kmem");
	} else {
		kmembase = base;
	}

	/*
	 * install!
	 */

	kernel_map = &kernel_map_store;

	pool_subsystem_init();
	vmem_bootstrap();

	kmem_arena = vmem_create("kmem", kmembase, kmemsize, PAGE_SIZE,
	    NULL, NULL, NULL,
	    0, VM_NOSLEEP | VM_BOOTSTRAP, IPL_VM);

	vmem_init(kmem_arena);

	kmem_va_arena = vmem_create("kva", 0, 0, PAGE_SIZE,
	    vmem_alloc, vmem_free, kmem_arena,
	    16 * PAGE_SIZE, VM_NOSLEEP | VM_BOOTSTRAP, IPL_VM);
}

/*
 * uvm_km_init: init the kernel maps virtual memory caches
 * and start the pool/kmem allocator.
 */
void
uvm_km_init(void)
{

	kmem_init();

	kmeminit(); // killme
}

/*
 * uvm_km_suballoc: allocate a submap in the kernel map.   once a submap
 * is allocated all references to that area of VM must go through it.  this
 * allows the locking of VAs in kernel_map to be broken up into regions.
 *
 * => if `fixed' is true, *vmin specifies where the region described
 *   pager_map => used to map "buf" structures into kernel space
 *      by the submap must start
 * => if submap is non NULL we use that as the submap, otherwise we
 *	alloc a new map
 */

struct vm_map *
uvm_km_suballoc(struct vm_map *map, vaddr_t *vmin /* IN/OUT */,
    vaddr_t *vmax /* OUT */, vsize_t size, int flags, bool fixed,
    struct vm_map *submap)
{
	int mapflags = UVM_FLAG_NOMERGE | (fixed ? UVM_FLAG_FIXED : 0);

	KASSERT(vm_map_pmap(map) == pmap_kernel());

	size = round_page(size);	/* round up to pagesize */

	/*
	 * first allocate a blank spot in the parent map
	 */

	if (uvm_map(map, vmin, size, NULL, UVM_UNKNOWN_OFFSET, 0,
	    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
	    UVM_ADV_RANDOM, mapflags)) != 0) {
	       panic("uvm_km_suballoc: unable to allocate space in parent map");
	}

	/*
	 * set VM bounds (vmin is filled in by uvm_map)
	 */

	*vmax = *vmin + size;

	/*
	 * add references to pmap and create or init the submap
	 */

	pmap_reference(vm_map_pmap(map));
	if (submap == NULL) {
		submap = kmem_alloc(sizeof(*submap), KM_SLEEP);
		if (submap == NULL)
			panic("uvm_km_suballoc: unable to create submap");
	}
	uvm_map_setup(submap, *vmin, *vmax, flags);
	submap->pmap = vm_map_pmap(map);

	/*
	 * now let uvm_map_submap plug in it...
	 */

	if (uvm_map_submap(map, *vmin, *vmax, submap) != 0)
		panic("uvm_km_suballoc: submap allocation failed");

	return(submap);
}

/*
 * uvm_km_pgremove: remove pages from a kernel uvm_object and KVA.
 */

void
uvm_km_pgremove(vaddr_t startva, vaddr_t endva)
{
	struct uvm_object * const uobj = uvm_kernel_object;
	const voff_t start = startva - vm_map_min(kernel_map);
	const voff_t end = endva - vm_map_min(kernel_map);
	struct vm_page *pg;
	voff_t curoff, nextoff;
	int swpgonlydelta = 0;
	UVMHIST_FUNC("uvm_km_pgremove"); UVMHIST_CALLED(maphist);

	KASSERT(VM_MIN_KERNEL_ADDRESS <= startva);
	KASSERT(startva < endva);
	KASSERT(endva <= VM_MAX_KERNEL_ADDRESS);

	mutex_enter(uobj->vmobjlock);
	pmap_remove(pmap_kernel(), startva, endva);
	for (curoff = start; curoff < end; curoff = nextoff) {
		nextoff = curoff + PAGE_SIZE;
		pg = uvm_pagelookup(uobj, curoff);
		if (pg != NULL && pg->flags & PG_BUSY) {
			pg->flags |= PG_WANTED;
			UVM_UNLOCK_AND_WAIT(pg, uobj->vmobjlock, 0,
				    "km_pgrm", 0);
			mutex_enter(uobj->vmobjlock);
			nextoff = curoff;
			continue;
		}

		/*
		 * free the swap slot, then the page.
		 */

		if (pg == NULL &&
		    uao_find_swslot(uobj, curoff >> PAGE_SHIFT) > 0) {
			swpgonlydelta++;
		}
		uao_dropswap(uobj, curoff >> PAGE_SHIFT);
		if (pg != NULL) {
			mutex_enter(&uvm_pageqlock);
			uvm_pagefree(pg);
			mutex_exit(&uvm_pageqlock);
		}
	}
	mutex_exit(uobj->vmobjlock);

	if (swpgonlydelta > 0) {
		mutex_enter(&uvm_swap_data_lock);
		KASSERT(uvmexp.swpgonly >= swpgonlydelta);
		uvmexp.swpgonly -= swpgonlydelta;
		mutex_exit(&uvm_swap_data_lock);
	}
}


/*
 * uvm_km_pgremove_intrsafe: like uvm_km_pgremove(), but for non object backed
 *    regions.
 *
 * => when you unmap a part of anonymous kernel memory you want to toss
 *    the pages right away.    (this is called from uvm_unmap_...).
 * => none of the pages will ever be busy, and none of them will ever
 *    be on the active or inactive queues (because they have no object).
 */

void
uvm_km_pgremove_intrsafe(struct vm_map *map, vaddr_t start, vaddr_t end)
{
	struct vm_page *pg;
	paddr_t pa;
	UVMHIST_FUNC("uvm_km_pgremove_intrsafe"); UVMHIST_CALLED(maphist);

	KASSERT(VM_MAP_IS_KERNEL(map));
	KASSERT(vm_map_min(map) <= start);
	KASSERT(start < end);
	KASSERT(end <= vm_map_max(map));

	for (; start < end; start += PAGE_SIZE) {
		if (!pmap_extract(pmap_kernel(), start, &pa)) {
			continue;
		}
		pg = PHYS_TO_VM_PAGE(pa);
		KASSERT(pg);
		KASSERT(pg->uobject == NULL && pg->uanon == NULL);
		KASSERT((pg->flags & PG_BUSY) == 0);
		uvm_pagefree(pg);
	}
}

#if defined(DEBUG)
void
uvm_km_check_empty(struct vm_map *map, vaddr_t start, vaddr_t end)
{
	struct vm_page *pg;
	vaddr_t va;
	paddr_t pa;

	KDASSERT(VM_MAP_IS_KERNEL(map));
	KDASSERT(vm_map_min(map) <= start);
	KDASSERT(start < end);
	KDASSERT(end <= vm_map_max(map));

	for (va = start; va < end; va += PAGE_SIZE) {
		if (pmap_extract(pmap_kernel(), va, &pa)) {
			panic("uvm_km_check_empty: va %p has pa 0x%llx",
			    (void *)va, (long long)pa);
		}
		if ((map->flags & VM_MAP_INTRSAFE) == 0) {
			mutex_enter(uvm_kernel_object->vmobjlock);
			pg = uvm_pagelookup(uvm_kernel_object,
			    va - vm_map_min(kernel_map));
			mutex_exit(uvm_kernel_object->vmobjlock);
			if (pg) {
				panic("uvm_km_check_empty: "
				    "has page hashed at %p", (const void *)va);
			}
		}
	}
}
#endif /* defined(DEBUG) */

/*
 * uvm_km_alloc: allocate an area of kernel memory.
 *
 * => NOTE: we can return 0 even if we can wait if there is not enough
 *	free VM space in the map... caller should be prepared to handle
 *	this case.
 * => we return KVA of memory allocated
 */

vaddr_t
uvm_km_alloc(struct vm_map *map, vsize_t size, vsize_t align, uvm_flag_t flags)
{
	vaddr_t kva, loopva;
	vaddr_t offset;
	vsize_t loopsize;
	struct vm_page *pg;
	struct uvm_object *obj;
	int pgaflags;
	vm_prot_t prot;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	KASSERT(vm_map_pmap(map) == pmap_kernel());
	KASSERT((flags & UVM_KMF_TYPEMASK) == UVM_KMF_WIRED ||
		(flags & UVM_KMF_TYPEMASK) == UVM_KMF_PAGEABLE ||
		(flags & UVM_KMF_TYPEMASK) == UVM_KMF_VAONLY);
	KASSERT((flags & UVM_KMF_VAONLY) != 0 || (flags & UVM_KMF_COLORMATCH) == 0);
	KASSERT((flags & UVM_KMF_COLORMATCH) == 0 || (flags & UVM_KMF_VAONLY) != 0);

	/*
	 * setup for call
	 */

	kva = vm_map_min(map);	/* hint */
	size = round_page(size);
	obj = (flags & UVM_KMF_PAGEABLE) ? uvm_kernel_object : NULL;
	UVMHIST_LOG(maphist,"  (map=0x%x, obj=0x%x, size=0x%x, flags=%d)",
		    map, obj, size, flags);

	/*
	 * allocate some virtual space
	 */

	if (__predict_false(uvm_map(map, &kva, size, obj, UVM_UNKNOWN_OFFSET,
	    align, UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
	    UVM_ADV_RANDOM,
	    (flags & (UVM_KMF_TRYLOCK | UVM_KMF_NOWAIT | UVM_KMF_WAITVA
	     | UVM_KMF_COLORMATCH)))) != 0)) {
		UVMHIST_LOG(maphist, "<- done (no VM)",0,0,0,0);
		return(0);
	}

	/*
	 * if all we wanted was VA, return now
	 */

	if (flags & (UVM_KMF_VAONLY | UVM_KMF_PAGEABLE)) {
		UVMHIST_LOG(maphist,"<- done valloc (kva=0x%x)", kva,0,0,0);
		return(kva);
	}

	/*
	 * recover object offset from virtual address
	 */

	offset = kva - vm_map_min(kernel_map);
	UVMHIST_LOG(maphist, "  kva=0x%x, offset=0x%x", kva, offset,0,0);

	/*
	 * now allocate and map in the memory... note that we are the only ones
	 * whom should ever get a handle on this area of VM.
	 */

	loopva = kva;
	loopsize = size;

	pgaflags = UVM_FLAG_COLORMATCH;
	if (flags & UVM_KMF_NOWAIT)
		pgaflags |= UVM_PGA_USERESERVE;
	if (flags & UVM_KMF_ZERO)
		pgaflags |= UVM_PGA_ZERO;
	prot = VM_PROT_READ | VM_PROT_WRITE;
	if (flags & UVM_KMF_EXEC)
		prot |= VM_PROT_EXECUTE;
	while (loopsize) {
		KASSERTMSG(!pmap_extract(pmap_kernel(), loopva, NULL),
		    "loopva=%#"PRIxVADDR, loopva);

		pg = uvm_pagealloc_strat(NULL, offset, NULL, pgaflags,
#ifdef UVM_KM_VMFREELIST
		   UVM_PGA_STRAT_ONLY, UVM_KM_VMFREELIST
#else
		   UVM_PGA_STRAT_NORMAL, 0
#endif
		   );

		/*
		 * out of memory?
		 */

		if (__predict_false(pg == NULL)) {
			if ((flags & UVM_KMF_NOWAIT) ||
			    ((flags & UVM_KMF_CANFAIL) && !uvm_reclaimable())) {
				/* free everything! */
				uvm_km_free(map, kva, size,
				    flags & UVM_KMF_TYPEMASK);
				return (0);
			} else {
				uvm_wait("km_getwait2");	/* sleep here */
				continue;
			}
		}

		pg->flags &= ~PG_BUSY;	/* new page */
		UVM_PAGE_OWN(pg, NULL);

		/*
		 * map it in
		 */

		pmap_kenter_pa(loopva, VM_PAGE_TO_PHYS(pg),
		    prot, PMAP_KMPAGE);
		loopva += PAGE_SIZE;
		offset += PAGE_SIZE;
		loopsize -= PAGE_SIZE;
	}

	pmap_update(pmap_kernel());

	UVMHIST_LOG(maphist,"<- done (kva=0x%x)", kva,0,0,0);
	return(kva);
}

/*
 * uvm_km_free: free an area of kernel memory
 */

void
uvm_km_free(struct vm_map *map, vaddr_t addr, vsize_t size, uvm_flag_t flags)
{

	KASSERT((flags & UVM_KMF_TYPEMASK) == UVM_KMF_WIRED ||
		(flags & UVM_KMF_TYPEMASK) == UVM_KMF_PAGEABLE ||
		(flags & UVM_KMF_TYPEMASK) == UVM_KMF_VAONLY);
	KASSERT((addr & PAGE_MASK) == 0);
	KASSERT(vm_map_pmap(map) == pmap_kernel());

	size = round_page(size);

	if (flags & UVM_KMF_PAGEABLE) {
		uvm_km_pgremove(addr, addr + size);
	} else if (flags & UVM_KMF_WIRED) {
		/*
		 * Note: uvm_km_pgremove_intrsafe() extracts mapping, thus
		 * remove it after.  See comment below about KVA visibility.
		 */
		uvm_km_pgremove_intrsafe(map, addr, addr + size);
		pmap_kremove(addr, size);
	}

	/*
	 * Note: uvm_unmap_remove() calls pmap_update() for us, before
	 * KVA becomes globally available.
	 */

	uvm_unmap1(map, addr, addr + size, UVM_FLAG_VAONLY);
}

/* Sanity; must specify both or none. */
#if (defined(PMAP_MAP_POOLPAGE) || defined(PMAP_UNMAP_POOLPAGE)) && \
    (!defined(PMAP_MAP_POOLPAGE) || !defined(PMAP_UNMAP_POOLPAGE))
#error Must specify MAP and UNMAP together.
#endif

int
uvm_km_kmem_alloc(vmem_t *vm, vmem_size_t size, vm_flag_t flags,
    vmem_addr_t *addr)
{
	struct vm_page *pg;
	vmem_addr_t va;
	int rc;
	vaddr_t loopva;
	vsize_t loopsize;

	size = round_page(size);

#if defined(PMAP_MAP_POOLPAGE)
	if (size == PAGE_SIZE) {
again:
#ifdef PMAP_ALLOC_POOLPAGE
		pg = PMAP_ALLOC_POOLPAGE((flags & VM_SLEEP) ?
		   0 : UVM_PGA_USERESERVE);
#else
		pg = uvm_pagealloc(NULL, 0, NULL,
		   (flags & VM_SLEEP) ? 0 : UVM_PGA_USERESERVE);
#endif /* PMAP_ALLOC_POOLPAGE */
		if (__predict_false(pg == NULL)) {
			if (flags & VM_SLEEP) {
				uvm_wait("plpg");
				goto again;
			}
		}
		va = PMAP_MAP_POOLPAGE(VM_PAGE_TO_PHYS(pg));
		if (__predict_false(va == 0)) {
			uvm_pagefree(pg);
			return ENOMEM;
		}
		*addr = va;
		return 0;
	}
#endif /* PMAP_MAP_POOLPAGE */

	rc = vmem_alloc(vm, size, flags, &va);
	if (rc != 0)
		return rc;

	loopva = va;
	loopsize = size;

	while (loopsize) {
		KASSERTMSG(!pmap_extract(pmap_kernel(), loopva, NULL),
		    "loopva=%#"PRIxVADDR" loopsize=%#"PRIxVSIZE" vmem=%p",
		    loopva, loopsize, vm);

		pg = uvm_pagealloc(NULL, loopva, NULL,
		    UVM_FLAG_COLORMATCH
		    | ((flags & VM_SLEEP) ? 0 : UVM_PGA_USERESERVE));
		if (__predict_false(pg == NULL)) {
			if (flags & VM_SLEEP) {
				uvm_wait("plpg");
				continue;
			} else {
				uvm_km_pgremove_intrsafe(kernel_map, va,
				    va + size);
				pmap_kremove(va, size);
				vmem_free(kmem_va_arena, va, size);
				return ENOMEM;
			}
		}
	
		pg->flags &= ~PG_BUSY;	/* new page */
		UVM_PAGE_OWN(pg, NULL);
		pmap_kenter_pa(loopva, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ|VM_PROT_WRITE, PMAP_KMPAGE);

		loopva += PAGE_SIZE;
		loopsize -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	*addr = va;

	return 0;
}

void
uvm_km_kmem_free(vmem_t *vm, vmem_addr_t addr, size_t size)
{

	size = round_page(size);
#if defined(PMAP_UNMAP_POOLPAGE)
	if (size == PAGE_SIZE) {
		paddr_t pa;

		pa = PMAP_UNMAP_POOLPAGE(addr);
		uvm_pagefree(PHYS_TO_VM_PAGE(pa));
		return;
	}
#endif /* PMAP_UNMAP_POOLPAGE */
	uvm_km_pgremove_intrsafe(kernel_map, addr, addr + size);
	pmap_kremove(addr, size);
	pmap_update(pmap_kernel());

	vmem_free(vm, addr, size);
}

bool
uvm_km_va_starved_p(void)
{
	vmem_size_t total;
	vmem_size_t free;

	total = vmem_size(kmem_arena, VMEM_ALLOC|VMEM_FREE);
	free = vmem_size(kmem_arena, VMEM_FREE);

	return (free < (total / 10));
}

