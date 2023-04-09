/*	$NetBSD: uvm_bio.c,v 1.128 2023/04/09 09:00:56 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uvm_bio.c,v 1.128 2023/04/09 09:00:56 riastradh Exp $");

#include "opt_uvmhist.h"
#include "opt_ubc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/vnode.h>
#include <sys/bitops.h>		/* for ilog2() */

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>

#ifdef PMAP_DIRECT
#  define UBC_USE_PMAP_DIRECT
#endif

/*
 * local functions
 */

static int	ubc_fault(struct uvm_faultinfo *, vaddr_t, struct vm_page **,
			  int, int, vm_prot_t, int);
static struct ubc_map *ubc_find_mapping(struct uvm_object *, voff_t);
static int	ubchash_stats(struct hashstat_sysctl *hs, bool fill);
#ifdef UBC_USE_PMAP_DIRECT
static int __noinline ubc_uiomove_direct(struct uvm_object *, struct uio *, vsize_t,
			  int, int);
static void __noinline ubc_zerorange_direct(struct uvm_object *, off_t, size_t, int);

/* XXX disabled by default until the kinks are worked out. */
bool ubc_direct = false;
#endif

/*
 * local data structures
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

struct ubc_map {
	struct uvm_object *	uobj;		/* mapped object */
	voff_t			offset;		/* offset into uobj */
	voff_t			writeoff;	/* write offset */
	vsize_t			writelen;	/* write len */
	int			refcount;	/* refcount on mapping */
	int			flags;		/* extra state */
	int			advice;

	LIST_ENTRY(ubc_map)	hash;		/* hash table */
	TAILQ_ENTRY(ubc_map)	inactive;	/* inactive queue */
	LIST_ENTRY(ubc_map)	list;		/* per-object list */
};

TAILQ_HEAD(ubc_inactive_head, ubc_map);
static struct ubc_object {
	struct uvm_object uobj;		/* glue for uvm_map() */
	char *kva;			/* where ubc_object is mapped */
	struct ubc_map *umap;		/* array of ubc_map's */

	LIST_HEAD(, ubc_map) *hash;	/* hashtable for cached ubc_map's */
	u_long hashmask;		/* mask for hashtable */

	struct ubc_inactive_head *inactive;
					/* inactive queues for ubc_map's */
} ubc_object;

const struct uvm_pagerops ubc_pager = {
	.pgo_fault = ubc_fault,
	/* ... rest are NULL */
};

/* Use value at least as big as maximum page size supported by architecture */
#define UBC_MAX_WINSHIFT	\
    ((1 << UBC_WINSHIFT) > MAX_PAGE_SIZE ? UBC_WINSHIFT : ilog2(MAX_PAGE_SIZE))

int ubc_nwins = UBC_NWINS;
const int ubc_winshift = UBC_MAX_WINSHIFT;
const int ubc_winsize = 1 << UBC_MAX_WINSHIFT;
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
	/*
	 * Make sure ubc_winshift is sane.
	 */
	KASSERT(ubc_winshift >= PAGE_SHIFT);

	/*
	 * init ubc_object.
	 * alloc and init ubc_map's.
	 * init inactive queues.
	 * alloc and init hashtable.
	 * map in ubc_object.
	 */

	uvm_obj_init(&ubc_object.uobj, &ubc_pager, true, UVM_OBJ_KERN);

	ubc_object.umap = kmem_zalloc(ubc_nwins * sizeof(struct ubc_map),
	    KM_SLEEP);
	if (ubc_object.umap == NULL)
		panic("ubc_init: failed to allocate ubc_map");

	vaddr_t va = (vaddr_t)1L;
#ifdef PMAP_PREFER
	PMAP_PREFER(0, &va, 0, 0);	/* kernel is never topdown */
	ubc_nqueues = va >> ubc_winshift;
	if (ubc_nqueues == 0) {
		ubc_nqueues = 1;
	}
#endif
	ubc_object.inactive = kmem_alloc(UBC_NQUEUES *
	    sizeof(struct ubc_inactive_head), KM_SLEEP);
	for (int i = 0; i < UBC_NQUEUES; i++) {
		TAILQ_INIT(&ubc_object.inactive[i]);
	}
	for (int i = 0; i < ubc_nwins; i++) {
		struct ubc_map *umap;
		umap = &ubc_object.umap[i];
		TAILQ_INSERT_TAIL(&ubc_object.inactive[i & (UBC_NQUEUES - 1)],
				  umap, inactive);
	}

	ubc_object.hash = hashinit(ubc_nwins, HASH_LIST, true,
	    &ubc_object.hashmask);
	for (int i = 0; i <= ubc_object.hashmask; i++) {
		LIST_INIT(&ubc_object.hash[i]);
	}

	if (uvm_map(kernel_map, (vaddr_t *)&ubc_object.kva,
		    ubc_nwins << ubc_winshift, &ubc_object.uobj, 0, (vsize_t)va,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_NONE,
				UVM_ADV_RANDOM, UVM_FLAG_NOMERGE)) != 0) {
		panic("ubc_init: failed to map ubc_object");
	}

	hashstat_register("ubchash", ubchash_stats);
}

void
ubchist_init(void)
{

	UVMHIST_INIT(ubchist, 300);
}

/*
 * ubc_fault_page: helper of ubc_fault to handle a single page.
 *
 * => Caller has UVM object locked.
 * => Caller will perform pmap_update().
 */

static inline int
ubc_fault_page(const struct uvm_faultinfo *ufi, const struct ubc_map *umap,
    struct vm_page *pg, vm_prot_t prot, vm_prot_t access_type, vaddr_t va)
{
	vm_prot_t mask;
	int error;
	bool rdonly;

	KASSERT(rw_write_held(pg->uobject->vmobjlock));

	KASSERT((pg->flags & PG_FAKE) == 0);
	if (pg->flags & PG_RELEASED) {
		uvm_pagefree(pg);
		return 0;
	}
	if (pg->loan_count != 0) {

		/*
		 * Avoid unneeded loan break, if possible.
		 */

		if ((access_type & VM_PROT_WRITE) == 0) {
			prot &= ~VM_PROT_WRITE;
		}
		if (prot & VM_PROT_WRITE) {
			struct vm_page *newpg;

			newpg = uvm_loanbreak(pg);
			if (newpg == NULL) {
				uvm_page_unbusy(&pg, 1);
				return ENOMEM;
			}
			pg = newpg;
		}
	}

	/*
	 * Note that a page whose backing store is partially allocated
	 * is marked as PG_RDONLY.
	 *
	 * it's a responsibility of ubc_alloc's caller to allocate backing
	 * blocks before writing to the window.
	 */

	KASSERT((pg->flags & PG_RDONLY) == 0 ||
	    (access_type & VM_PROT_WRITE) == 0 ||
	    pg->offset < umap->writeoff ||
	    pg->offset + PAGE_SIZE > umap->writeoff + umap->writelen);

	rdonly = uvm_pagereadonly_p(pg);
	mask = rdonly ? ~VM_PROT_WRITE : VM_PROT_ALL;

	error = pmap_enter(ufi->orig_map->pmap, va, VM_PAGE_TO_PHYS(pg),
	    prot & mask, PMAP_CANFAIL | (access_type & mask));

	uvm_pagelock(pg);
	uvm_pageactivate(pg);
	uvm_pagewakeup(pg);
	uvm_pageunlock(pg);
	pg->flags &= ~PG_BUSY;
	UVM_PAGE_OWN(pg, NULL);

	return error;
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
	struct vm_page *pgs[howmany(ubc_winsize, MIN_PAGE_SIZE)];
	int i, error, npages;
	vm_prot_t prot;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	/*
	 * no need to try with PGO_LOCKED...
	 * we don't need to have the map locked since we know that
	 * no one will mess with it until our reference is released.
	 */

	if (flags & PGO_LOCKED) {
		uvmfault_unlockall(ufi, NULL, &ubc_object.uobj);
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
	UVMHIST_LOG(ubchist, "va %#jx ubc_offset %#jx access_type %jd",
	    va, ubc_offset, access_type, 0);

	if ((access_type & VM_PROT_WRITE) != 0) {
#ifndef PRIxOFF		/* XXX */
#define PRIxOFF "jx"	/* XXX */
#endif			/* XXX */
		KASSERTMSG((trunc_page(umap->writeoff) <= slot_offset),
		    "out of range write: slot=%#"PRIxVSIZE" off=%#"PRIxOFF,
		    slot_offset, (intmax_t)umap->writeoff);
		KASSERTMSG((slot_offset < umap->writeoff + umap->writelen),
		    "out of range write: slot=%#"PRIxVADDR
		        " off=%#"PRIxOFF" len=%#"PRIxVSIZE,
		    slot_offset, (intmax_t)umap->writeoff, umap->writelen);
	}

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
	rw_enter(uobj->vmobjlock, RW_WRITER);

	UVMHIST_LOG(ubchist, "slot_offset %#jx writeoff %#jx writelen %#jx ",
	    slot_offset, umap->writeoff, umap->writelen, 0);
	UVMHIST_LOG(ubchist, "getpages uobj %#jx offset %#jx npages %jd",
	    (uintptr_t)uobj, umap->offset + slot_offset, npages, 0);

	error = (*uobj->pgops->pgo_get)(uobj, umap->offset + slot_offset, pgs,
	    &npages, 0, access_type, umap->advice, flags | PGO_NOBLOCKALLOC |
	    PGO_NOTIMESTAMP);
	UVMHIST_LOG(ubchist, "getpages error %jd npages %jd", error, npages, 0,
	    0);

	if (error == EAGAIN) {
		kpause("ubc_fault", false, hz >> 2, NULL);
		goto again;
	}
	if (error) {
		return error;
	}

	/*
	 * For virtually-indexed, virtually-tagged caches we should avoid
	 * creating writable mappings when we do not absolutely need them,
	 * since the "compatible alias" trick does not work on such caches.
	 * Otherwise, we can always map the pages writable.
	 */

#ifdef PMAP_CACHE_VIVT
	prot = VM_PROT_READ | access_type;
#else
	prot = VM_PROT_READ | VM_PROT_WRITE;
#endif

	va = ufi->orig_rvaddr;
	eva = ufi->orig_rvaddr + (npages << PAGE_SHIFT);

	UVMHIST_LOG(ubchist, "va %#jx eva %#jx", va, eva, 0, 0);

	/*
	 * Note: normally all returned pages would have the same UVM object.
	 * However, layered file-systems and e.g. tmpfs, may return pages
	 * which belong to underlying UVM object.  In such case, lock is
	 * shared amongst the objects.
	 */
	rw_enter(uobj->vmobjlock, RW_WRITER);
	for (i = 0; va < eva; i++, va += PAGE_SIZE) {
		struct vm_page *pg;

		UVMHIST_LOG(ubchist, "pgs[%jd] = %#jx", i, (uintptr_t)pgs[i],
		    0, 0);
		pg = pgs[i];

		if (pg == NULL || pg == PGO_DONTCARE) {
			continue;
		}
		KASSERT(uobj->vmobjlock == pg->uobject->vmobjlock);
		error = ubc_fault_page(ufi, umap, pg, prot, access_type, va);
		if (error) {
			/*
			 * Flush (there might be pages entered), drop the lock,
			 * and perform uvm_wait().  Note: page will re-fault.
			 */
			pmap_update(ufi->orig_map->pmap);
			rw_exit(uobj->vmobjlock);
			uvm_wait("ubc_fault");
			rw_enter(uobj->vmobjlock, RW_WRITER);
		}
	}
	/* Must make VA visible before the unlock. */
	pmap_update(ufi->orig_map->pmap);
	rw_exit(uobj->vmobjlock);

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

static void * __noinline
ubc_alloc(struct uvm_object *uobj, voff_t offset, vsize_t *lenp, int advice,
    int flags, struct vm_page **pgs, int *npagesp)
{
	vaddr_t slot_offset, va;
	struct ubc_map *umap;
	voff_t umap_offset;
	int error;
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(ubchist, "uobj %#jx offset %#jx len %#jx",
	    (uintptr_t)uobj, offset, *lenp, 0);

	KASSERT(*lenp > 0);
	umap_offset = (offset & ~((voff_t)ubc_winsize - 1));
	slot_offset = (vaddr_t)(offset & ((voff_t)ubc_winsize - 1));
	*lenp = MIN(*lenp, ubc_winsize - slot_offset);
	KASSERT(*lenp > 0);

	rw_enter(ubc_object.uobj.vmobjlock, RW_WRITER);
again:
	/*
	 * The UVM object is already referenced.
	 * Lock order: UBC object -> ubc_map::uobj.
	 */
	umap = ubc_find_mapping(uobj, umap_offset);
	if (umap == NULL) {
		struct uvm_object *oobj;

		UBC_EVCNT_INCR(wincachemiss);
		umap = TAILQ_FIRST(UBC_QUEUE(offset));
		if (umap == NULL) {
			rw_exit(ubc_object.uobj.vmobjlock);
			kpause("ubc_alloc", false, hz >> 2, NULL);
			rw_enter(ubc_object.uobj.vmobjlock, RW_WRITER);
			goto again;
		}

		va = UBC_UMAP_ADDR(umap);
		oobj = umap->uobj;

		/*
		 * Remove from old hash (if any), add to new hash.
		 */

		if (oobj != NULL) {
			/*
			 * Mapping must be removed before the list entry,
			 * since there is a race with ubc_purge().
			 */
			if (umap->flags & UMAP_MAPPING_CACHED) {
				umap->flags &= ~UMAP_MAPPING_CACHED;
				rw_enter(oobj->vmobjlock, RW_WRITER);
				pmap_remove(pmap_kernel(), va,
				    va + ubc_winsize);
				pmap_update(pmap_kernel());
				rw_exit(oobj->vmobjlock);
			}
			LIST_REMOVE(umap, hash);
			LIST_REMOVE(umap, list);
		} else {
			KASSERT((umap->flags & UMAP_MAPPING_CACHED) == 0);
		}
		umap->uobj = uobj;
		umap->offset = umap_offset;
		LIST_INSERT_HEAD(&ubc_object.hash[UBC_HASH(uobj, umap_offset)],
		    umap, hash);
		LIST_INSERT_HEAD(&uobj->uo_ubc, umap, list);
	} else {
		UBC_EVCNT_INCR(wincachehit);
		va = UBC_UMAP_ADDR(umap);
	}

	if (umap->refcount == 0) {
		TAILQ_REMOVE(UBC_QUEUE(offset), umap, inactive);
	}

	if (flags & UBC_WRITE) {
		KASSERTMSG(umap->writeoff == 0,
		    "ubc_alloc: concurrent writes to uobj %p", uobj);
		KASSERTMSG(umap->writelen == 0,
		    "ubc_alloc: concurrent writes to uobj %p", uobj);
		umap->writeoff = slot_offset;
		umap->writelen = *lenp;
	}

	umap->refcount++;
	umap->advice = advice;
	rw_exit(ubc_object.uobj.vmobjlock);
	UVMHIST_LOG(ubchist, "umap %#jx refs %jd va %#jx flags %#jx",
	    (uintptr_t)umap, umap->refcount, (uintptr_t)va, flags);

	if (flags & UBC_FAULTBUSY) {
		int npages = (*lenp + (offset & (PAGE_SIZE - 1)) +
		    PAGE_SIZE - 1) >> PAGE_SHIFT;
		int gpflags =
		    PGO_SYNCIO|PGO_OVERWRITE|PGO_PASTEOF|PGO_NOBLOCKALLOC|
		    PGO_NOTIMESTAMP;
		int i;
		KDASSERT(flags & UBC_WRITE);
		KASSERT(npages <= *npagesp);
		KASSERT(umap->refcount == 1);

		UBC_EVCNT_INCR(faultbusy);
again_faultbusy:
		rw_enter(uobj->vmobjlock, RW_WRITER);
		if (umap->flags & UMAP_MAPPING_CACHED) {
			umap->flags &= ~UMAP_MAPPING_CACHED;
			pmap_remove(pmap_kernel(), va, va + ubc_winsize);
		}
		memset(pgs, 0, *npagesp * sizeof(pgs[0]));

		error = (*uobj->pgops->pgo_get)(uobj, trunc_page(offset), pgs,
		    &npages, 0, VM_PROT_READ | VM_PROT_WRITE, advice, gpflags);
		UVMHIST_LOG(ubchist, "faultbusy getpages %jd", error, 0, 0, 0);
		if (error) {
			/*
			 * Flush: the mapping above might have been removed.
			 */
			pmap_update(pmap_kernel());
			goto out;
		}
		for (i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[i];

			KASSERT(pg->uobject == uobj);
			if (pg->loan_count != 0) {
				rw_enter(uobj->vmobjlock, RW_WRITER);
				if (pg->loan_count != 0) {
					pg = uvm_loanbreak(pg);
				}
				if (pg == NULL) {
					pmap_kremove(va, ubc_winsize);
					pmap_update(pmap_kernel());
					uvm_page_unbusy(pgs, npages);
					rw_exit(uobj->vmobjlock);
					uvm_wait("ubc_alloc");
					goto again_faultbusy;
				}
				rw_exit(uobj->vmobjlock);
				pgs[i] = pg;
			}
			pmap_kenter_pa(
			    va + trunc_page(slot_offset) + (i << PAGE_SHIFT),
			    VM_PAGE_TO_PHYS(pg),
			    VM_PROT_READ | VM_PROT_WRITE, 0);
		}
		pmap_update(pmap_kernel());
		umap->flags |= UMAP_PAGES_LOCKED;
		*npagesp = npages;
	} else {
		KASSERT((umap->flags & UMAP_PAGES_LOCKED) == 0);
	}

out:
	return (void *)(va + slot_offset);
}

/*
 * ubc_release:  free a file mapping window.
 */

static void __noinline
ubc_release(void *va, int flags, struct vm_page **pgs, int npages)
{
	struct ubc_map *umap;
	struct uvm_object *uobj;
	vaddr_t umapva;
	bool unmapped;
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(ubchist, "va %#jx", (uintptr_t)va, 0, 0, 0);

	umap = &ubc_object.umap[((char *)va - ubc_object.kva) >> ubc_winshift];
	umapva = UBC_UMAP_ADDR(umap);
	uobj = umap->uobj;
	KASSERT(uobj != NULL);

	if (umap->flags & UMAP_PAGES_LOCKED) {
		const voff_t endoff = umap->writeoff + umap->writelen;
		const voff_t zerolen = round_page(endoff) - endoff;

		KASSERT(npages == (round_page(endoff) -
		    trunc_page(umap->writeoff)) >> PAGE_SHIFT);
		KASSERT((umap->flags & UMAP_MAPPING_CACHED) == 0);
		if (zerolen) {
			memset((char *)umapva + endoff, 0, zerolen);
		}
		umap->flags &= ~UMAP_PAGES_LOCKED;
		rw_enter(uobj->vmobjlock, RW_WRITER);
		for (u_int i = 0; i < npages; i++) {
			struct vm_page *pg = pgs[i];
#ifdef DIAGNOSTIC
			paddr_t pa;
			bool rv;
			rv = pmap_extract(pmap_kernel(), umapva +
			    umap->writeoff + (i << PAGE_SHIFT), &pa);
			KASSERT(rv);
			KASSERT(PHYS_TO_VM_PAGE(pa) == pg);
#endif
			pg->flags &= ~PG_FAKE;
			KASSERTMSG(uvm_pagegetdirty(pg) ==
			    UVM_PAGE_STATUS_DIRTY,
			    "page %p not dirty", pg);
			KASSERT(pg->loan_count == 0);
			if (uvmpdpol_pageactivate_p(pg)) {
				uvm_pagelock(pg);
				uvm_pageactivate(pg);
				uvm_pageunlock(pg);
			}
		}
		pmap_kremove(umapva, ubc_winsize);
		pmap_update(pmap_kernel());
		uvm_page_unbusy(pgs, npages);
		rw_exit(uobj->vmobjlock);
		unmapped = true;
	} else {
		unmapped = false;
	}

	rw_enter(ubc_object.uobj.vmobjlock, RW_WRITER);
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
			rw_enter(uobj->vmobjlock, RW_WRITER);
			pmap_remove(pmap_kernel(), umapva,
				    umapva + ubc_winsize);
			pmap_update(pmap_kernel());
			rw_exit(uobj->vmobjlock);

			umap->flags &= ~UMAP_MAPPING_CACHED;
			LIST_REMOVE(umap, hash);
			LIST_REMOVE(umap, list);
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
	UVMHIST_LOG(ubchist, "umap %#jx refs %jd", (uintptr_t)umap,
	    umap->refcount, 0, 0);
	rw_exit(ubc_object.uobj.vmobjlock);
}

/*
 * ubc_uiomove: move data to/from an object.
 */

int
ubc_uiomove(struct uvm_object *uobj, struct uio *uio, vsize_t todo, int advice,
    int flags)
{
	const bool overwrite = (flags & UBC_FAULTBUSY) != 0;
	struct vm_page *pgs[howmany(ubc_winsize, MIN_PAGE_SIZE)];
	voff_t off;
	int error, npages;

	KASSERT(todo <= uio->uio_resid);
	KASSERT(((flags & UBC_WRITE) != 0 && uio->uio_rw == UIO_WRITE) ||
	    ((flags & UBC_READ) != 0 && uio->uio_rw == UIO_READ));

#ifdef UBC_USE_PMAP_DIRECT
	/*
	 * during direct access pages need to be held busy to prevent them
	 * changing identity, and therefore if we read or write an object
	 * into a mapped view of same we could deadlock while faulting.
	 *
	 * avoid the problem by disallowing direct access if the object
	 * might be visible somewhere via mmap().
	 *
	 * XXX concurrent reads cause thundering herd issues with PG_BUSY.
	 * In the future enable by default for writes or if ncpu<=2, and
	 * make the toggle override that.
	 */
	if ((ubc_direct && (flags & UBC_ISMAPPED) == 0) ||
	    (flags & UBC_FAULTBUSY) != 0) {
		return ubc_uiomove_direct(uobj, uio, todo, advice, flags);
	}
#endif

	off = uio->uio_offset;
	error = 0;
	while (todo > 0) {
		vsize_t bytelen = todo;
		void *win;

		npages = __arraycount(pgs);
		win = ubc_alloc(uobj, off, &bytelen, advice, flags, pgs,
		    &npages);
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
		ubc_release(win, flags, pgs, npages);
		off += bytelen;
		todo -= bytelen;
		if (error != 0 && (flags & UBC_PARTIALOK) != 0) {
			break;
		}
	}

	return error;
}

/*
 * ubc_zerorange: set a range of bytes in an object to zero.
 */

void
ubc_zerorange(struct uvm_object *uobj, off_t off, size_t len, int flags)
{
	struct vm_page *pgs[howmany(ubc_winsize, MIN_PAGE_SIZE)];
	int npages;

#ifdef UBC_USE_PMAP_DIRECT
	if (ubc_direct || (flags & UBC_FAULTBUSY) != 0) {
		ubc_zerorange_direct(uobj, off, len, flags);
		return;
	}
#endif

	/*
	 * XXXUBC invent kzero() and use it
	 */

	while (len) {
		void *win;
		vsize_t bytelen = len;

		npages = __arraycount(pgs);
		win = ubc_alloc(uobj, off, &bytelen, UVM_ADV_NORMAL, UBC_WRITE,
		    pgs, &npages);
		memset(win, 0, bytelen);
		ubc_release(win, flags, pgs, npages);

		off += bytelen;
		len -= bytelen;
	}
}

#ifdef UBC_USE_PMAP_DIRECT
/* Copy data using direct map */

/*
 * ubc_alloc_direct:  allocate a file mapping window using direct map
 */
static int __noinline
ubc_alloc_direct(struct uvm_object *uobj, voff_t offset, vsize_t *lenp,
    int advice, int flags, struct vm_page **pgs, int *npages)
{
	voff_t pgoff;
	int error;
	int gpflags = flags | PGO_NOTIMESTAMP | PGO_SYNCIO;
	int access_type = VM_PROT_READ;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(ubchist);

	if (flags & UBC_WRITE) {
		if (flags & UBC_FAULTBUSY)
			gpflags |= PGO_OVERWRITE | PGO_NOBLOCKALLOC;
#if 0
		KASSERT(!UVM_OBJ_NEEDS_WRITEFAULT(uobj));
#endif

		/*
		 * Tell genfs_getpages() we already have the journal lock,
		 * allow allocation past current EOF.
		 */
		gpflags |= PGO_JOURNALLOCKED | PGO_PASTEOF;
		access_type |= VM_PROT_WRITE;
	} else {
		/* Don't need the empty blocks allocated, PG_RDONLY is okay */
		gpflags |= PGO_NOBLOCKALLOC;
	}

	pgoff = (offset & PAGE_MASK);
	*lenp = MIN(*lenp, ubc_winsize - pgoff);

again:
	*npages = (*lenp + pgoff + PAGE_SIZE - 1) >> PAGE_SHIFT;
	KASSERT((*npages * PAGE_SIZE) <= ubc_winsize);
	KASSERT(*lenp + pgoff <= ubc_winsize);
	memset(pgs, 0, *npages * sizeof(pgs[0]));

	rw_enter(uobj->vmobjlock, RW_WRITER);
	error = (*uobj->pgops->pgo_get)(uobj, trunc_page(offset), pgs,
	    npages, 0, access_type, advice, gpflags);
	UVMHIST_LOG(ubchist, "alloc_direct getpages %jd", error, 0, 0, 0);
	if (error) {
		if (error == EAGAIN) {
			kpause("ubc_alloc_directg", false, hz >> 2, NULL);
			goto again;
		}
		return error;
	}

	rw_enter(uobj->vmobjlock, RW_WRITER);
	for (int i = 0; i < *npages; i++) {
		struct vm_page *pg = pgs[i];

		KASSERT(pg != NULL);
		KASSERT(pg != PGO_DONTCARE);
		KASSERT((pg->flags & PG_FAKE) == 0 || (gpflags & PGO_OVERWRITE));
		KASSERT(pg->uobject->vmobjlock == uobj->vmobjlock);

		/* Avoid breaking loan if possible, only do it on write */
		if ((flags & UBC_WRITE) && pg->loan_count != 0) {
			pg = uvm_loanbreak(pg);
			if (pg == NULL) {
				uvm_page_unbusy(pgs, *npages);
				rw_exit(uobj->vmobjlock);
				uvm_wait("ubc_alloc_directl");
				goto again;
			}
			pgs[i] = pg;
		}

		/* Page must be writable by now */
		KASSERT((pg->flags & PG_RDONLY) == 0 || (flags & UBC_WRITE) == 0);

		/*
		 * XXX For aobj pages.  No managed mapping - mark the page
		 * dirty.
		 */
		if ((flags & UBC_WRITE) != 0) {
			uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
		}
	}
	rw_exit(uobj->vmobjlock);

	return 0;
}

static void __noinline
ubc_direct_release(struct uvm_object *uobj,
	int flags, struct vm_page **pgs, int npages)
{
	rw_enter(uobj->vmobjlock, RW_WRITER);
	for (int i = 0; i < npages; i++) {
		struct vm_page *pg = pgs[i];

		pg->flags &= ~PG_BUSY;
		UVM_PAGE_OWN(pg, NULL);
		if (pg->flags & PG_RELEASED) {
			pg->flags &= ~PG_RELEASED;
			uvm_pagefree(pg);
			continue;
		}

		if (uvm_pagewanted_p(pg) || uvmpdpol_pageactivate_p(pg)) {
			uvm_pagelock(pg);
			uvm_pageactivate(pg);
			uvm_pagewakeup(pg);
			uvm_pageunlock(pg);
		}

		/* Page was changed, no longer fake and neither clean. */
		if (flags & UBC_WRITE) {
			KASSERTMSG(uvm_pagegetdirty(pg) ==
			    UVM_PAGE_STATUS_DIRTY,
			    "page %p not dirty", pg);
			pg->flags &= ~PG_FAKE;
		}
	}
	rw_exit(uobj->vmobjlock);
}

static int
ubc_uiomove_process(void *win, size_t len, void *arg)
{
	struct uio *uio = (struct uio *)arg;

	return uiomove(win, len, uio);
}

static int
ubc_zerorange_process(void *win, size_t len, void *arg)
{
	memset(win, 0, len);
	return 0;
}

static int __noinline
ubc_uiomove_direct(struct uvm_object *uobj, struct uio *uio, vsize_t todo, int advice,
    int flags)
{
	const bool overwrite = (flags & UBC_FAULTBUSY) != 0;
	voff_t off;
	int error, npages;
	struct vm_page *pgs[howmany(ubc_winsize, MIN_PAGE_SIZE)];

	KASSERT(todo <= uio->uio_resid);
	KASSERT(((flags & UBC_WRITE) != 0 && uio->uio_rw == UIO_WRITE) ||
	    ((flags & UBC_READ) != 0 && uio->uio_rw == UIO_READ));

	off = uio->uio_offset;
	error = 0;
	while (todo > 0) {
		vsize_t bytelen = todo;

		error = ubc_alloc_direct(uobj, off, &bytelen, advice, flags,
		    pgs, &npages);
		if (error != 0) {
			/* can't do anything, failed to get the pages */
			break;
		}

		if (error == 0) {
			error = uvm_direct_process(pgs, npages, off, bytelen,
			    ubc_uiomove_process, uio);
		}

		if (overwrite) {
			voff_t endoff;

			/*
			 * if we haven't initialized the pages yet due to an
			 * error above, do it now.
			 */
			if (error != 0) {
				(void) uvm_direct_process(pgs, npages, off,
				    bytelen, ubc_zerorange_process, NULL);
			}

			off += bytelen;
			todo -= bytelen;
			endoff = off & (PAGE_SIZE - 1);

			/*
			 * zero out the remaining portion of the final page
			 * (if any).
			 */
			if (todo == 0 && endoff != 0) {
				vsize_t zlen = PAGE_SIZE - endoff;
				(void) uvm_direct_process(pgs + npages - 1, 1,
				    off, zlen, ubc_zerorange_process, NULL);
			}
		} else {
			off += bytelen;
			todo -= bytelen;
		}

		ubc_direct_release(uobj, flags, pgs, npages);

		if (error != 0 && ISSET(flags, UBC_PARTIALOK)) {
			break;
		}
	}

	return error;
}

static void __noinline
ubc_zerorange_direct(struct uvm_object *uobj, off_t off, size_t todo, int flags)
{
	int error, npages;
	struct vm_page *pgs[howmany(ubc_winsize, MIN_PAGE_SIZE)];

	flags |= UBC_WRITE;

	error = 0;
	while (todo > 0) {
		vsize_t bytelen = todo;

		error = ubc_alloc_direct(uobj, off, &bytelen, UVM_ADV_NORMAL,
		    flags, pgs, &npages);
		if (error != 0) {
			/* can't do anything, failed to get the pages */
			break;
		}

		error = uvm_direct_process(pgs, npages, off, bytelen,
		    ubc_zerorange_process, NULL);

		ubc_direct_release(uobj, flags, pgs, npages);

		off += bytelen;
		todo -= bytelen;
	}
}

#endif /* UBC_USE_PMAP_DIRECT */

/*
 * ubc_purge: disassociate ubc_map structures from an empty uvm_object.
 */

void
ubc_purge(struct uvm_object *uobj)
{
	struct ubc_map *umap;
	vaddr_t va;

	KASSERT(uobj->uo_npages == 0);

	/*
	 * Safe to check without lock held, as ubc_alloc() removes
	 * the mapping and list entry in the correct order.
	 */
	if (__predict_true(LIST_EMPTY(&uobj->uo_ubc))) {
		return;
	}
	rw_enter(ubc_object.uobj.vmobjlock, RW_WRITER);
	while ((umap = LIST_FIRST(&uobj->uo_ubc)) != NULL) {
		KASSERT(umap->refcount == 0);
		for (va = 0; va < ubc_winsize; va += PAGE_SIZE) {
			KASSERT(!pmap_extract(pmap_kernel(),
			    va + UBC_UMAP_ADDR(umap), NULL));
		}
		LIST_REMOVE(umap, list);
		LIST_REMOVE(umap, hash);
		umap->flags &= ~UMAP_MAPPING_CACHED;
		umap->uobj = NULL;
	}
	rw_exit(ubc_object.uobj.vmobjlock);
}

static int
ubchash_stats(struct hashstat_sysctl *hs, bool fill)
{
	struct ubc_map *umap;
	uint64_t chain;

	strlcpy(hs->hash_name, "ubchash", sizeof(hs->hash_name));
	strlcpy(hs->hash_desc, "ubc object hash", sizeof(hs->hash_desc));
	if (!fill)
		return 0;

	hs->hash_size = ubc_object.hashmask + 1;

	for (size_t i = 0; i < hs->hash_size; i++) {
		chain = 0;
		rw_enter(ubc_object.uobj.vmobjlock, RW_READER);
		LIST_FOREACH(umap, &ubc_object.hash[i], hash) {
			chain++;
		}
		rw_exit(ubc_object.uobj.vmobjlock);
		if (chain > 0) {
			hs->hash_used++;
			hs->hash_items += chain;
			if (chain > hs->hash_maxchain)
				hs->hash_maxchain = chain;
		}
		preempt_point();
	}

	return 0;
}
