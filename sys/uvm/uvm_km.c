/*	$NetBSD: uvm_km.c,v 1.104 2009/11/07 07:27:49 cegger Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles D. Cranor,
 *      Washington University, the University of California, Berkeley and
 *      its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *   kmem_map => contains only wired kernel memory for the kernel
 *		malloc.
 *   mb_map => memory for large mbufs,
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
__KERNEL_RCSID(0, "$NetBSD: uvm_km.c,v 1.104 2009/11/07 07:27:49 cegger Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/pool.h>

#include <uvm/uvm.h>

/*
 * global data structures
 */

struct vm_map *kernel_map = NULL;

/*
 * local data structues
 */

static struct vm_map_kernel	kernel_map_store;
static struct vm_map_entry	kernel_first_mapent_store;

#if !defined(PMAP_MAP_POOLPAGE)

/*
 * kva cache
 *
 * XXX maybe it's better to do this at the uvm_map layer.
 */

#define	KM_VACACHE_SIZE	(32 * PAGE_SIZE) /* XXX tune */

static void *km_vacache_alloc(struct pool *, int);
static void km_vacache_free(struct pool *, void *);
static void km_vacache_init(struct vm_map *, const char *, size_t);

/* XXX */
#define	KM_VACACHE_POOL_TO_MAP(pp) \
	((struct vm_map *)((char *)(pp) - \
	    offsetof(struct vm_map_kernel, vmk_vacache)))

static void *
km_vacache_alloc(struct pool *pp, int flags)
{
	vaddr_t va;
	size_t size;
	struct vm_map *map;
	size = pp->pr_alloc->pa_pagesz;

	map = KM_VACACHE_POOL_TO_MAP(pp);

	va = vm_map_min(map); /* hint */
	if (uvm_map(map, &va, size, NULL, UVM_UNKNOWN_OFFSET, size,
	    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
	    UVM_ADV_RANDOM, UVM_FLAG_QUANTUM |
	    ((flags & PR_WAITOK) ? UVM_FLAG_WAITVA :
	    UVM_FLAG_TRYLOCK | UVM_FLAG_NOWAIT))))
		return NULL;

	return (void *)va;
}

static void
km_vacache_free(struct pool *pp, void *v)
{
	vaddr_t va = (vaddr_t)v;
	size_t size = pp->pr_alloc->pa_pagesz;
	struct vm_map *map;

	map = KM_VACACHE_POOL_TO_MAP(pp);
	uvm_unmap1(map, va, va + size, UVM_FLAG_QUANTUM|UVM_FLAG_VAONLY);
}

/*
 * km_vacache_init: initialize kva cache.
 */

static void
km_vacache_init(struct vm_map *map, const char *name, size_t size)
{
	struct vm_map_kernel *vmk;
	struct pool *pp;
	struct pool_allocator *pa;
	int ipl;

	KASSERT(VM_MAP_IS_KERNEL(map));
	KASSERT(size < (vm_map_max(map) - vm_map_min(map)) / 2); /* sanity */


	vmk = vm_map_to_kernel(map);
	pp = &vmk->vmk_vacache;
	pa = &vmk->vmk_vacache_allocator;
	memset(pa, 0, sizeof(*pa));
	pa->pa_alloc = km_vacache_alloc;
	pa->pa_free = km_vacache_free;
	pa->pa_pagesz = (unsigned int)size;
	pa->pa_backingmap = map;
	pa->pa_backingmapptr = NULL;

	if ((map->flags & VM_MAP_INTRSAFE) != 0)
		ipl = IPL_VM;
	else
		ipl = IPL_NONE;

	pool_init(pp, PAGE_SIZE, 0, 0, PR_NOTOUCH | PR_RECURSIVE, name, pa,
	    ipl);
}

void
uvm_km_vacache_init(struct vm_map *map, const char *name, size_t size)
{

	map->flags |= VM_MAP_VACACHE;
	if (size == 0)
		size = KM_VACACHE_SIZE;
	km_vacache_init(map, name, size);
}

#else /* !defined(PMAP_MAP_POOLPAGE) */

void
uvm_km_vacache_init(struct vm_map *map, const char *name, size_t size)
{

	/* nothing */
}

#endif /* !defined(PMAP_MAP_POOLPAGE) */

void
uvm_km_va_drain(struct vm_map *map, uvm_flag_t flags)
{
	struct vm_map_kernel *vmk = vm_map_to_kernel(map);

	callback_run_roundrobin(&vmk->vmk_reclaim_callback, NULL);
}

/*
 * uvm_km_init: init kernel maps and objects to reflect reality (i.e.
 * KVM already allocated for text, data, bss, and static data structures).
 *
 * => KVM is defined by VM_MIN_KERNEL_ADDRESS/VM_MAX_KERNEL_ADDRESS.
 *    we assume that [vmin -> start] has already been allocated and that
 *    "end" is the end.
 */

void
uvm_km_init(vaddr_t start, vaddr_t end)
{
	vaddr_t base = VM_MIN_KERNEL_ADDRESS;

	/*
	 * next, init kernel memory objects.
	 */

	/* kernel_object: for pageable anonymous kernel memory */
	uao_init();
	uvm_kernel_object = uao_create(VM_MAX_KERNEL_ADDRESS -
				 VM_MIN_KERNEL_ADDRESS, UAO_FLAG_KERNOBJ);

	/*
	 * init the map and reserve any space that might already
	 * have been allocated kernel space before installing.
	 */

	uvm_map_setup_kernel(&kernel_map_store, base, end, VM_MAP_PAGEABLE);
	kernel_map_store.vmk_map.pmap = pmap_kernel();
	if (start != base) {
		int error;
		struct uvm_map_args args;

		error = uvm_map_prepare(&kernel_map_store.vmk_map,
		    base, start - base,
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
		    		UVM_ADV_RANDOM, UVM_FLAG_FIXED), &args);
		if (!error) {
			kernel_first_mapent_store.flags =
			    UVM_MAP_KERNEL | UVM_MAP_FIRST;
			error = uvm_map_enter(&kernel_map_store.vmk_map, &args,
			    &kernel_first_mapent_store);
		}

		if (error)
			panic(
			    "uvm_km_init: could not reserve space for kernel");
	}

	/*
	 * install!
	 */

	kernel_map = &kernel_map_store.vmk_map;
	uvm_km_vacache_init(kernel_map, "kvakernel", 0);
}

/*
 * uvm_km_suballoc: allocate a submap in the kernel map.   once a submap
 * is allocated all references to that area of VM must go through it.  this
 * allows the locking of VAs in kernel_map to be broken up into regions.
 *
 * => if `fixed' is true, *vmin specifies where the region described
 *      by the submap must start
 * => if submap is non NULL we use that as the submap, otherwise we
 *	alloc a new map
 */

struct vm_map *
uvm_km_suballoc(struct vm_map *map, vaddr_t *vmin /* IN/OUT */,
    vaddr_t *vmax /* OUT */, vsize_t size, int flags, bool fixed,
    struct vm_map_kernel *submap)
{
	int mapflags = UVM_FLAG_NOMERGE | (fixed ? UVM_FLAG_FIXED : 0);

	KASSERT(vm_map_pmap(map) == pmap_kernel());

	size = round_page(size);	/* round up to pagesize */
	size += uvm_mapent_overhead(size, flags);

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
		submap = malloc(sizeof(*submap), M_VMMAP, M_WAITOK);
		if (submap == NULL)
			panic("uvm_km_suballoc: unable to create submap");
	}
	uvm_map_setup_kernel(submap, *vmin, *vmax, flags);
	submap->vmk_map.pmap = vm_map_pmap(map);

	/*
	 * now let uvm_map_submap plug in it...
	 */

	if (uvm_map_submap(map, *vmin, *vmax, &submap->vmk_map) != 0)
		panic("uvm_km_suballoc: submap allocation failed");

	return(&submap->vmk_map);
}

/*
 * uvm_km_pgremove: remove pages from a kernel uvm_object.
 *
 * => when you unmap a part of anonymous kernel memory you want to toss
 *    the pages right away.    (this gets called from uvm_unmap_...).
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

	mutex_enter(&uobj->vmobjlock);

	for (curoff = start; curoff < end; curoff = nextoff) {
		nextoff = curoff + PAGE_SIZE;
		pg = uvm_pagelookup(uobj, curoff);
		if (pg != NULL && pg->flags & PG_BUSY) {
			pg->flags |= PG_WANTED;
			UVM_UNLOCK_AND_WAIT(pg, &uobj->vmobjlock, 0,
				    "km_pgrm", 0);
			mutex_enter(&uobj->vmobjlock);
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
	mutex_exit(&uobj->vmobjlock);

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
			mutex_enter(&uvm_kernel_object->vmobjlock);
			pg = uvm_pagelookup(uvm_kernel_object,
			    va - vm_map_min(kernel_map));
			mutex_exit(&uvm_kernel_object->vmobjlock);
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
	    (flags & (UVM_KMF_TRYLOCK | UVM_KMF_NOWAIT | UVM_KMF_WAITVA))
	    | UVM_FLAG_QUANTUM)) != 0)) {
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

	pgaflags = 0;
	if (flags & UVM_KMF_NOWAIT)
		pgaflags |= UVM_PGA_USERESERVE;
	if (flags & UVM_KMF_ZERO)
		pgaflags |= UVM_PGA_ZERO;
	prot = VM_PROT_READ | VM_PROT_WRITE;
	if (flags & UVM_KMF_EXEC)
		prot |= VM_PROT_EXECUTE;
	while (loopsize) {
		KASSERT(!pmap_extract(pmap_kernel(), loopva, NULL));

		pg = uvm_pagealloc(NULL, offset, NULL, pgaflags);

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
		    prot | PMAP_KMPAGE, 0);
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
		pmap_remove(pmap_kernel(), addr, addr + size);
	} else if (flags & UVM_KMF_WIRED) {
		uvm_km_pgremove_intrsafe(map, addr, addr + size);
		pmap_kremove(addr, size);
	}

	/*
	 * uvm_unmap_remove calls pmap_update for us.
	 */

	uvm_unmap1(map, addr, addr + size, UVM_FLAG_QUANTUM|UVM_FLAG_VAONLY);
}

/* Sanity; must specify both or none. */
#if (defined(PMAP_MAP_POOLPAGE) || defined(PMAP_UNMAP_POOLPAGE)) && \
    (!defined(PMAP_MAP_POOLPAGE) || !defined(PMAP_UNMAP_POOLPAGE))
#error Must specify MAP and UNMAP together.
#endif

/*
 * uvm_km_alloc_poolpage: allocate a page for the pool allocator
 *
 * => if the pmap specifies an alternate mapping method, we use it.
 */

/* ARGSUSED */
vaddr_t
uvm_km_alloc_poolpage_cache(struct vm_map *map, bool waitok)
{
#if defined(PMAP_MAP_POOLPAGE)
	return uvm_km_alloc_poolpage(map, waitok);
#else
	struct vm_page *pg;
	struct pool *pp = &vm_map_to_kernel(map)->vmk_vacache;
	vaddr_t va;

	if ((map->flags & VM_MAP_VACACHE) == 0)
		return uvm_km_alloc_poolpage(map, waitok);

	va = (vaddr_t)pool_get(pp, waitok ? PR_WAITOK : PR_NOWAIT);
	if (va == 0)
		return 0;
	KASSERT(!pmap_extract(pmap_kernel(), va, NULL));
again:
	pg = uvm_pagealloc(NULL, 0, NULL, waitok ? 0 : UVM_PGA_USERESERVE);
	if (__predict_false(pg == NULL)) {
		if (waitok) {
			uvm_wait("plpg");
			goto again;
		} else {
			pool_put(pp, (void *)va);
			return 0;
		}
	}
	pmap_kenter_pa(va, VM_PAGE_TO_PHYS(pg),
	    VM_PROT_READ|VM_PROT_WRITE|PMAP_KMPAGE, 0);
	pmap_update(pmap_kernel());

	return va;
#endif /* PMAP_MAP_POOLPAGE */
}

vaddr_t
uvm_km_alloc_poolpage(struct vm_map *map, bool waitok)
{
#if defined(PMAP_MAP_POOLPAGE)
	struct vm_page *pg;
	vaddr_t va;

 again:
	pg = uvm_pagealloc(NULL, 0, NULL, waitok ? 0 : UVM_PGA_USERESERVE);
	if (__predict_false(pg == NULL)) {
		if (waitok) {
			uvm_wait("plpg");
			goto again;
		} else
			return (0);
	}
	va = PMAP_MAP_POOLPAGE(VM_PAGE_TO_PHYS(pg));
	if (__predict_false(va == 0))
		uvm_pagefree(pg);
	return (va);
#else
	vaddr_t va;

	va = uvm_km_alloc(map, PAGE_SIZE, 0,
	    (waitok ? 0 : UVM_KMF_NOWAIT | UVM_KMF_TRYLOCK) | UVM_KMF_WIRED);
	return (va);
#endif /* PMAP_MAP_POOLPAGE */
}

/*
 * uvm_km_free_poolpage: free a previously allocated pool page
 *
 * => if the pmap specifies an alternate unmapping method, we use it.
 */

/* ARGSUSED */
void
uvm_km_free_poolpage_cache(struct vm_map *map, vaddr_t addr)
{
#if defined(PMAP_UNMAP_POOLPAGE)
	uvm_km_free_poolpage(map, addr);
#else
	struct pool *pp;

	if ((map->flags & VM_MAP_VACACHE) == 0) {
		uvm_km_free_poolpage(map, addr);
		return;
	}

	KASSERT(pmap_extract(pmap_kernel(), addr, NULL));
	uvm_km_pgremove_intrsafe(map, addr, addr + PAGE_SIZE);
	pmap_kremove(addr, PAGE_SIZE);
#if defined(DEBUG)
	pmap_update(pmap_kernel());
#endif
	KASSERT(!pmap_extract(pmap_kernel(), addr, NULL));
	pp = &vm_map_to_kernel(map)->vmk_vacache;
	pool_put(pp, (void *)addr);
#endif
}

/* ARGSUSED */
void
uvm_km_free_poolpage(struct vm_map *map, vaddr_t addr)
{
#if defined(PMAP_UNMAP_POOLPAGE)
	paddr_t pa;

	pa = PMAP_UNMAP_POOLPAGE(addr);
	uvm_pagefree(PHYS_TO_VM_PAGE(pa));
#else
	uvm_km_free(map, addr, PAGE_SIZE, UVM_KMF_WIRED);
#endif /* PMAP_UNMAP_POOLPAGE */
}
