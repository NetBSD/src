/*	$NetBSD: uvm_bio.c,v 1.62.14.2 2007/07/27 09:50:38 yamt Exp $	*/

/*
 * Copyright (c) 1998 Chuck Silvers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * uvm_bio.c: buffered i/o object mapping cache
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_bio.c,v 1.62.14.2 2007/07/27 09:50:38 yamt Exp $");

#include "opt_uvmhist.h"
#include "opt_ubc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

/*
 * global data structures
 */

/*
 * local functions
 */

static int	ubc_fault(struct uvm_faultinfo *, vaddr_t, struct vm_page **,
			  int, int, vm_prot_t, int);
static struct ubc_map *ubc_find_mapping(struct uvm_object *, voff_t);

/*
 * local data structues
 */

#define UBC_HASH(uobj, offset) 						\
	(((((u_long)(uobj)) >> 8) + (((u_long)(offset)) >> PAGE_SHIFT)) & \
				ubc_object.hashmask)

#define UBC_QUEUE(offset)						\
	(&ubc_object.inactive[(((u_long)(offset)) >> ubc_winshift) &	\
			     (UBC_NQUEUES - 1)])

#define UBC_UMAP_ADDR(u)						\
	(vaddr_t)(ubc_object.kva + (((u) - ubc_object.umap) << ubc_winshift))


#define UMAP_PAGES_LOCKED	0x0001
#define UMAP_MAPPING_CACHED	0x0002

struct ubc_map
{
	struct uvm_object *	uobj;		/* mapped object */
	voff_t			offset;		/* offset into uobj */
	voff_t			writeoff;	/* write offset */
	vsize_t			writelen;	/* write len */
	int			refcount;	/* refcount on mapping */
	int			flags;		/* extra state */
	int			advice;

	LIST_ENTRY(ubc_map)	hash;		/* hash table */
	TAILQ_ENTRY(ubc_map)	inactive;	/* inactive queue */
};

static struct ubc_object
{
	struct uvm_object uobj;		/* glue for uvm_map() */
	char *kva;			/* where ubc_object is mapped */
	struct ubc_map *umap;		/* array of ubc_map's */

	LIST_HEAD(, ubc_map) *hash;	/* hashtable for cached ubc_map's */
	u_long hashmask;		/* mask for hashtable */

	TAILQ_HEAD(ubc_inactive_head, ubc_map) *inactive;
					/* inactive queues for ubc_map's */

} ubc_object;

struct uvm_pagerops ubc_pager =
{
	.pgo_fault = ubc_fault,
	/* ... rest are NULL */
};

int ubc_nwins = UBC_NWINS;
int ubc_winshift = UBC_WINSHIFT;
int ubc_winsize;
#if defined(PMAP_PREFER)
int ubc_nqueues;
#define UBC_NQUEUES ubc_nqueues
#else
#define UBC_NQUEUES 1
#endif

#if defined(UBC_STATS)

#define	UBC_EVCNT_DEFINE(name) \
struct evcnt ubc_evcnt_##name = \
EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "ubc", #name); \
EVCNT_ATTACH_STATIC(ubc_evcnt_##name);
#define	UBC_EVCNT_INCR(name) ubc_evcnt_##name.ev_count++

#else /* defined(UBC_STATS) */

#define	UBC_EVCNT_DEFINE(name)	/* nothing */
#define	UBC_EVCNT_INCR(name)	/* nothing */

#endif /* defined(UBC_STATS) */

UBC_EVCNT_DEFINE(wincachehit)
UBC_EVCNT_DEFINE(wincachemiss)
UBC_EVCNT_DEFINE(faultbusy)

/*
 * ubc_init
 *
 * init pager private data structures.
 */

void
ubc_init(void)
{
	struct ubc_map *umap;
	vaddr_t va;
	int i;

	/*
	 * Make sure ubc_winshift is sane.
	 */
	if (ubc_winshift < PAGE_SHIFT)
		ubc_winshift = PAGE_SHIFT;

	/*
	 * init ubc_object.
	 * alloc and init ubc_map's.
	 * init inactive queues.
	 * alloc and init hashtable.
	 * map in ubc_object.
	 */

	UVM_OBJ_INIT(&ubc_object.uobj, &ubc_pager, UVM_OBJ_KERN);

	ubc_object.umap = malloc(ubc_nwins * sizeof(struct ubc_map),
				 M_TEMP, M_NOWAIT);
	if (ubc_object.umap == NULL)
		panic("ubc_init: failed to allocate ubc_map");
	memset(ubc_object.umap, 0, ubc_nwins * sizeof(struct ubc_map));

	if (ubc_winshift < PAGE_SHIFT) {
		ubc_winshift = PAGE_SHIFT;
	}
	va = (vaddr_t)1L;
#ifdef PMAP_PREFER
	PMAP_PREFER(0, &va, 0, 0);	/* kernel is never topdown */
	ubc_nqueues = va >> ubc_winshift;
	if (ubc_nqueues == 0) {
		ubc_nqueues = 1;
	}
#endif
	ubc_winsize = 1 << ubc_winshift;
	ubc_object.inactive = malloc(UBC_NQUEUES *
	    sizeof(struct ubc_inactive_head), M_TEMP, M_NOWAIT);
	if (ubc_object.inactive == NULL)
		panic("ubc_init: failed to allocate inactive queue heads");
	for (i = 0; i < UBC_NQUEUES; i++) {
		TAILQ_INIT(&ubc_object.inactive[i]);
	}
	for (i = 0; i < ubc_nwins; i++) {
		umap = &ubc_object.umap[i];
		TAILQ_INSERT_TAIL(&ubc_object.inactive[i & (UBC_NQUEUES - 1)],
				  umap, inactive);
	}

	ubc_object.hash = hashinit(ubc_nwins, HASH_LIST, M_TEMP, M_NOWAIT,
				   &ubc_object.hashmask);
	for (i = 0; i <= ubc_object.hashmask; i++) {
		LIST_INIT(&ubc_object.hash[i]);
	}

	if (uvm_map(kernel_map, (vaddr_t *)&ubc_object.kva,
		    ubc_nwins << ubc_winshift, &ubc_object.uobj, 0, (vsize_t)va,
		    UVM_MAPFLAG(UVM_PROT_ALL, UVM_PROT_ALL, UVM_INH_NONE,
				UVM_ADV_RANDOM, UVM_FLAG_NOMERGE)) != 0) {
		panic("ubc_init: failed to map ubc_object");
	}
	UVMHIST_INIT(ubchist, 300);
}

/*
 * ubc_fault: fault routine for ubc mapping
 */

static int
ubc_fault(struct uvm_faultinfo *ufi, vaddr_t ign1, struct vm_page **ign2,
    int ign3, int ign4, vm_prot_t access_type, int flags)
{
	struct uvm_object *uobj;
	struct ubc_map *umap;
	vaddr_t va, eva, ubc_offset, slot_offset;
	int i, error, npages;
	struct vm_page *pgs[ubc_winsize >> PAGE_SHIFT], *pg;
	vm_prot_t prot;
	UVMHIST_FUNC("ubc_fault");  UVMHIST_CALLED(ubchist);

	/*
	 * no need to try with PGO_LOCKED...
	 * we don't need to have the map locked since we know that
	 * no one will mess with it until our reference is released.
	 */

	if (flags & PGO_LOCKED) {
		uvmfault_unlockall(ufi, NULL, &ubc_object.uobj, NULL);
		flags &= ~PGO_LOCKED;
	}

	va = ufi->orig_rvaddr;
	ubc_offset = va - (vaddr_t)ubc_object.kva;
	umap = &ubc_object.umap[ubc_offset >> ubc_winshift];
	KASSERT(umap->refcount != 0);
	KASSERT((umap->flags & UMAP_PAGES_LOCKED) == 0);
	slot_offset = ubc_offset & (ubc_winsize - 1);

	/*
	 * some platforms cannot write to individual bytes atomically, so
	 * software has to do read/modify/write of larger quantities instead.
	 * this means that the access_type for "write" operations
	 * can be VM_PROT_READ, which confuses us mightily.
	 *
	 * deal with this by resetting access_type based on the info
	 * that ubc_alloc() stores for us.
	 */

	access_type = umap->writelen ? VM_PROT_WRITE : VM_PROT_READ;
	UVMHIST_LOG(ubchist, "va 0x%lx ubc_offset 0x%lx access_type %d",
	    va, ubc_offset, access_type, 0);

#ifdef DIAGNOSTIC
	if ((access_type & VM_PROT_WRITE) != 0) {
		if (slot_offset < trunc_page(umap->writeoff) ||
		    umap->writeoff + umap->writelen <= slot_offset) {
			panic("ubc_fault: out of range write");
		}
	}
#endif

	/* no umap locking needed since we have a ref on the umap */
	uobj = umap->uobj;

	if ((access_type & VM_PROT_WRITE) == 0) {
		npages = (ubc_winsize - slot_offset) >> PAGE_SHIFT;
	} else {
		npages = (round_page(umap->offset + umap->writeoff +
		    umap->writelen) - (umap->offset + slot_offset))
		    >> PAGE_SHIFT;
		flags |= PGO_PASTEOF;
	}

again:
	memset(pgs, 0, sizeof (pgs));
	simple_lock(&uobj->vmobjlock);

	UVMHIST_LOG(ubchist, "slot_offset 0x%x writeoff 0x%x writelen 0x%x ",
	    slot_offset, umap->writeoff, umap->writelen, 0);
	UVMHIST_LOG(ubchist, "getpages uobj %p offset 0x%x npages %d",
	    uobj, umap->offset + slot_offset, npages, 0);

	error = (*uobj->pgops->pgo_get)(uobj, umap->offset + slot_offset, pgs,
	    &npages, 0, access_type, umap->advice, flags | PGO_NOBLOCKALLOC |
	    PGO_NOTIMESTAMP);
	UVMHIST_LOG(ubchist, "getpages error %d npages %d", error, npages, 0,
	    0);

	if (error == EAGAIN) {
		kpause("ubc_fault", false, hz, NULL);
		goto again;
	}
	if (error) {
		return error;
	}

	va = ufi->orig_rvaddr;
	eva = ufi->orig_rvaddr + (npages << PAGE_SHIFT);

	UVMHIST_LOG(ubchist, "va 0x%lx eva 0x%lx", va, eva, 0, 0);
	for (i = 0; va < eva; i++, va += PAGE_SIZE) {
		bool rdonly;
		vm_prot_t mask;

		/*
		 * for virtually-indexed, virtually-tagged caches we should
		 * avoid creating writable mappings when we don't absolutely
		 * need them, since the "compatible alias" trick doesn't work
		 * on such caches.  otherwise, we can always map the pages
		 * writable.
		 */

#ifdef PMAP_CACHE_VIVT
		prot = VM_PROT_READ | access_type;
#else
		prot = VM_PROT_READ | VM_PROT_WRITE;
#endif
		UVMHIST_LOG(ubchist, "pgs[%d] = %p", i, pgs[i], 0, 0);
		pg = pgs[i];

		if (pg == NULL || pg == PGO_DONTCARE) {
			continue;
		}

		uobj = pg->uobject;
		simple_lock(&uobj->vmobjlock);
		if (pg->flags & PG_WANTED) {
			wakeup(pg);
		}
		KASSERT((pg->flags & PG_FAKE) == 0);
		if (pg->flags & PG_RELEASED) {
			uvm_lock_pageq();
			uvm_pagefree(pg);
			uvm_unlock_pageq();
			simple_unlock(&uobj->vmobjlock);
			continue;
		}
		if (pg->loan_count != 0) {

			/*
			 * avoid unneeded loan break if possible.
			 */

			if ((access_type & VM_PROT_WRITE) == 0)
				prot &= ~VM_PROT_WRITE;

			if (prot & VM_PROT_WRITE) {
				struct vm_page *newpg;

				newpg = uvm_loanbreak(pg);
				if (newpg == NULL) {
					uvm_page_unbusy(&pg, 1);
					simple_unlock(&uobj->vmobjlock);
					uvm_wait("ubc_loanbrk");
					continue; /* will re-fault */
				}
				pg = newpg;
			}
		}

		/*
		 * note that a page whose backing store is partially allocated
		 * is marked as PG_RDONLY.
		 */

		rdonly = ((access_type & VM_PROT_WRITE) == 0 &&
		    (pg->flags & PG_RDONLY) != 0) ||
		    UVM_OBJ_NEEDS_WRITEFAULT(uobj);
		KASSERT((pg->flags & PG_RDONLY) == 0 ||
		    (access_type & VM_PROT_WRITE) == 0 ||
		    pg->offset < umap->writeoff ||
		    pg->offset + PAGE_SIZE > umap->writeoff + umap->writelen);
		mask = rdonly ? ~VM_PROT_WRITE : VM_PROT_ALL;
		error = pmap_enter(ufi->orig_map->pmap, va, VM_PAGE_TO_PHYS(pg),
		    prot & mask, PMAP_CANFAIL | (access_type & mask));
		uvm_lock_pageq();
		uvm_pageactivate(pg);
		uvm_unlock_pageq();
		pg->flags &= ~(PG_BUSY|PG_WANTED);
		UVM_PAGE_OWN(pg, NULL);
		simple_unlock(&uobj->vmobjlock);
		if (error) {
			UVMHIST_LOG(ubchist, "pmap_enter fail %d",
			    error, 0, 0, 0);
			uvm_wait("ubc_pmfail");
			/* will refault */
		}
	}
	pmap_update(ufi->orig_map->pmap);
	return 0;
}

/*
 * local functions
 */

static struct ubc_map *
ubc_find_mapping(struct uvm_object *uobj, voff_t offset)
{
	struct ubc_map *umap;

	LIST_FOREACH(umap, &ubc_object.hash[UBC_HASH(uobj, offset)], hash) {
		if (umap->uobj == uobj && umap->offset == offset) {
			return umap;
		}
	}
	return NULL;
}


/*
 * ubc interface functions
 */

/*
 * ubc_alloc:  allocate a file mapping window
 */

void *
ubc_alloc(struct uvm_object *uobj, voff_t offset, vsize_t *lenp, int advice,
    int flags)
{
	vaddr_t slot_offset, va;
	struct ubc_map *umap;
	voff_t umap_offset;
	int error;
	UVMHIST_FUNC("ubc_alloc"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "uobj %p offset 0x%lx len 0x%lx",
	    uobj, offset, *lenp, 0);

	KASSERT(*lenp > 0);
	umap_offset = (offset & ~((voff_t)ubc_winsize - 1));
	slot_offset = (vaddr_t)(offset & ((voff_t)ubc_winsize - 1));
	*lenp = MIN(*lenp, ubc_winsize - slot_offset);

	/*
	 * the object is always locked here, so we don't need to add a ref.
	 */

again:
	simple_lock(&ubc_object.uobj.vmobjlock);
	umap = ubc_find_mapping(uobj, umap_offset);
	if (umap == NULL) {
		UBC_EVCNT_INCR(wincachemiss);
		umap = TAILQ_FIRST(UBC_QUEUE(offset));
		if (umap == NULL) {
			simple_unlock(&ubc_object.uobj.vmobjlock);
			kpause("ubc_alloc", false, hz, NULL);
			goto again;
		}

		/*
		 * remove from old hash (if any), add to new hash.
		 */

		if (umap->uobj != NULL) {
			LIST_REMOVE(umap, hash);
		}
		umap->uobj = uobj;
		umap->offset = umap_offset;
		LIST_INSERT_HEAD(&ubc_object.hash[UBC_HASH(uobj, umap_offset)],
		    umap, hash);
		va = UBC_UMAP_ADDR(umap);
		if (umap->flags & UMAP_MAPPING_CACHED) {
			umap->flags &= ~UMAP_MAPPING_CACHED;
			pmap_remove(pmap_kernel(), va, va + ubc_winsize);
			pmap_update(pmap_kernel());
		}
	} else {
		UBC_EVCNT_INCR(wincachehit);
		va = UBC_UMAP_ADDR(umap);
	}

	if (umap->refcount == 0) {
		TAILQ_REMOVE(UBC_QUEUE(offset), umap, inactive);
	}

#ifdef DIAGNOSTIC
	if ((flags & UBC_WRITE) && (umap->writeoff || umap->writelen)) {
		panic("ubc_alloc: concurrent writes uobj %p", uobj);
	}
#endif
	if (flags & UBC_WRITE) {
		umap->writeoff = slot_offset;
		umap->writelen = *lenp;
	}

	umap->refcount++;
	umap->advice = advice;
	simple_unlock(&ubc_object.uobj.vmobjlock);
	UVMHIST_LOG(ubchist, "umap %p refs %d va %p flags 0x%x",
	    umap, umap->refcount, va, flags);

	if (flags & UBC_FAULTBUSY) {
		int npages = (*lenp + PAGE_SIZE - 1) >> PAGE_SHIFT;
		struct vm_page *pgs[npages];
		int gpflags =
		    PGO_SYNCIO|PGO_OVERWRITE|PGO_PASTEOF|PGO_NOBLOCKALLOC|
		    PGO_NOTIMESTAMP;
		int i;
		KDASSERT(flags & UBC_WRITE);
		KASSERT(umap->refcount == 1);

		UBC_EVCNT_INCR(faultbusy);
		if (umap->flags & UMAP_MAPPING_CACHED) {
			umap->flags &= ~UMAP_MAPPING_CACHED;
			pmap_remove(pmap_kernel(), va, va + ubc_winsize);
		}
again_faultbusy:
		memset(pgs, 0, sizeof(pgs));
		simple_lock(&uobj->vmobjlock);
		error = (*uobj->pgops->pgo_get)(uobj, trunc_page(offset), pgs,
		    &npages, 0, VM_PROT_READ | VM_PROT_WRITE, advice, gpflags);
		UVMHIST_LOG(ubchist, "faultbusy getpages %d", error, 0, 0, 0);
		if (error) {
			goto out;
		}
		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[i];

			KASSERT(pg->uobject == uobj);
			if (pg->loan_count != 0) {
				simple_lock(&uobj->vmobjlock);
				if (pg->loan_count != 0) {
					pg = uvm_loanbreak(pg);
				}
				simple_unlock(&uobj->vmobjlock);
				if (pg == NULL) {
					pmap_kremove(va, ubc_winsize);
					pmap_update(pmap_kernel());
					simple_lock(&uobj->vmobjlock);
					uvm_page_unbusy(pgs, npages);
					simple_unlock(&uobj->vmobjlock);
					uvm_wait("ubc_alloc");
					goto again_faultbusy;
				}
				pgs[i] = pg;
			}
			pmap_kenter_pa(va + slot_offset + (i << PAGE_SHIFT),
			    VM_PAGE_TO_PHYS(pg), VM_PROT_READ | VM_PROT_WRITE);
		}
		pmap_update(pmap_kernel());
		umap->flags |= UMAP_PAGES_LOCKED;
	} else {
		KASSERT((umap->flags & UMAP_PAGES_LOCKED) == 0);
	}

out:
	return (void *)(va + slot_offset);
}

/*
 * ubc_release:  free a file mapping window.
 */

void
ubc_release(void *va, int flags)
{
	struct ubc_map *umap;
	struct uvm_object *uobj;
	vaddr_t umapva;
	bool unmapped;
	UVMHIST_FUNC("ubc_release"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "va %p", va, 0, 0, 0);
	umap = &ubc_object.umap[((char *)va - ubc_object.kva) >> ubc_winshift];
	umapva = UBC_UMAP_ADDR(umap);
	uobj = umap->uobj;
	KASSERT(uobj != NULL);

	if (umap->flags & UMAP_PAGES_LOCKED) {
		int slot_offset = umap->writeoff;
		int endoff = umap->writeoff + umap->writelen;
		int zerolen = round_page(endoff) - endoff;
		int npages = (int)(round_page(umap->writeoff + umap->writelen)
				   - trunc_page(umap->writeoff)) >> PAGE_SHIFT;
		struct vm_page *pgs[npages];
		paddr_t pa;
		int i;
		bool rv;

		KASSERT((umap->flags & UMAP_MAPPING_CACHED) == 0);
		if (zerolen) {
			memset((char *)umapva + endoff, 0, zerolen);
		}
		umap->flags &= ~UMAP_PAGES_LOCKED;
		uvm_lock_pageq();
		for (i = 0; i < npages; i++) {
			rv = pmap_extract(pmap_kernel(),
			    umapva + slot_offset + (i << PAGE_SHIFT), &pa);
			KASSERT(rv);
			pgs[i] = PHYS_TO_VM_PAGE(pa);
			pgs[i]->flags &= ~(PG_FAKE|PG_CLEAN);
			KASSERT(pgs[i]->loan_count == 0);
			uvm_pageactivate(pgs[i]);
		}
		uvm_unlock_pageq();
		pmap_kremove(umapva, ubc_winsize);
		pmap_update(pmap_kernel());
		simple_lock(&uobj->vmobjlock);
		uvm_page_unbusy(pgs, npages);
		simple_unlock(&uobj->vmobjlock);
		unmapped = true;
	} else {
		unmapped = false;
	}

	simple_lock(&ubc_object.uobj.vmobjlock);
	umap->writeoff = 0;
	umap->writelen = 0;
	umap->refcount--;
	if (umap->refcount == 0) {
		if (flags & UBC_UNMAP) {

			/*
			 * Invalidate any cached mappings if requested.
			 * This is typically used to avoid leaving
			 * incompatible cache aliases around indefinitely.
			 */

			pmap_remove(pmap_kernel(), umapva,
				    umapva + ubc_winsize);
			umap->flags &= ~UMAP_MAPPING_CACHED;
			pmap_update(pmap_kernel());
			LIST_REMOVE(umap, hash);
			umap->uobj = NULL;
			TAILQ_INSERT_HEAD(UBC_QUEUE(umap->offset), umap,
			    inactive);
		} else {
			if (!unmapped) {
				umap->flags |= UMAP_MAPPING_CACHED;
			}
			TAILQ_INSERT_TAIL(UBC_QUEUE(umap->offset), umap,
			    inactive);
		}
	}
	UVMHIST_LOG(ubchist, "umap %p refs %d", umap, umap->refcount, 0, 0);
	simple_unlock(&ubc_object.uobj.vmobjlock);
}

/*
 * ubc_uiomove: move data to/from an object.
 */

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo, int advice,
    int flags)
{
	voff_t off;
	const bool overwrite = (flags & UBC_FAULTBUSY) != 0;
	int error;

	KASSERT(todo <= uio->uio_resid);
	KASSERT(((flags & UBC_WRITE) != 0 && uio->uio_rw == UIO_WRITE) ||
	    ((flags & UBC_READ) != 0 && uio->uio_rw == UIO_READ));

	off = uio->uio_offset;
	error = 0;
	while (todo > 0) {
		vsize_t bytelen = todo;
		void *win;

		win = ubc_alloc(uobj, off, &bytelen, advice, flags);
		if (error == 0) {
			error = uiomove(win, bytelen, uio);
		}
		if (error != 0 && overwrite) {
			/*
			 * if we haven't initialized the pages yet,
			 * do it now.  it's safe to use memset here
			 * because we just mapped the pages above.
			 */
			memset(win, 0, bytelen);
		}
		ubc_release(win, flags);
		off += bytelen;
		todo -= bytelen;
		if (error != 0 && (flags & UBC_PARTIALOK) != 0) {
			break;
		}
	}

	return error;
}

#if 0 /* notused */
/*
 * removing a range of mappings from the ubc mapping cache.
 */

void
ubc_flush(struct uvm_object *uobj, voff_t start, voff_t end)
{
	struct ubc_map *umap;
	vaddr_t va;
	UVMHIST_FUNC("ubc_flush");  UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "uobj %p start 0x%lx end 0x%lx",
		    uobj, start, end, 0);

	simple_lock(&ubc_object.uobj.vmobjlock);
	for (umap = ubc_object.umap;
	     umap < &ubc_object.umap[ubc_nwins];
	     umap++) {

		if (umap->uobj != uobj || umap->offset < start ||
		    (umap->offset >= end && end != 0) ||
		    umap->refcount > 0) {
			continue;
		}

		/*
		 * remove from hash,
		 * move to head of inactive queue.
		 */

		va = (vaddr_t)(ubc_object.kva +
		    ((umap - ubc_object.umap) << ubc_winshift));
		pmap_remove(pmap_kernel(), va, va + ubc_winsize);

		LIST_REMOVE(umap, hash);
		umap->uobj = NULL;
		TAILQ_REMOVE(UBC_QUEUE(umap->offset), umap, inactive);
		TAILQ_INSERT_HEAD(UBC_QUEUE(umap->offset), umap, inactive);
	}
	pmap_update(pmap_kernel());
	simple_unlock(&ubc_object.uobj.vmobjlock);
}
#endif /* notused */
