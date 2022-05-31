/*	$NetBSD: uvm_aobj.c,v 1.156 2022/05/31 08:43:16 andvar Exp $	*/

/*
 * Copyright (c) 1998 Chuck Silvers, Charles D. Cranor and
 *                    Washington University.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from: Id: uvm_aobj.c,v 1.1.2.5 1998/02/06 05:14:38 chs Exp
 */

/*
 * uvm_aobj.c: anonymous memory uvm_object pager
 *
 * author: Chuck Silvers <chuq@chuq.com>
 * started: Jan-1998
 *
 * - design mostly from Chuck Cranor
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_aobj.c,v 1.156 2022/05/31 08:43:16 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_uvmhist.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page_array.h>

/*
 * An anonymous UVM object (aobj) manages anonymous-memory.  In addition to
 * keeping the list of resident pages, it may also keep a list of allocated
 * swap blocks.  Depending on the size of the object, this list is either
 * stored in an array (small objects) or in a hash table (large objects).
 *
 * Lock order
 *
 *	uao_list_lock ->
 *		uvm_object::vmobjlock
 */

/*
 * Note: for hash tables, we break the address space of the aobj into blocks
 * of UAO_SWHASH_CLUSTER_SIZE pages, which shall be a power of two.
 */

#define	UAO_SWHASH_CLUSTER_SHIFT	4
#define	UAO_SWHASH_CLUSTER_SIZE		(1 << UAO_SWHASH_CLUSTER_SHIFT)

/* Get the "tag" for this page index. */
#define	UAO_SWHASH_ELT_TAG(idx)		((idx) >> UAO_SWHASH_CLUSTER_SHIFT)
#define UAO_SWHASH_ELT_PAGESLOT_IDX(idx) \
    ((idx) & (UAO_SWHASH_CLUSTER_SIZE - 1))

/* Given an ELT and a page index, find the swap slot. */
#define	UAO_SWHASH_ELT_PAGESLOT(elt, idx) \
    ((elt)->slots[UAO_SWHASH_ELT_PAGESLOT_IDX(idx)])

/* Given an ELT, return its pageidx base. */
#define	UAO_SWHASH_ELT_PAGEIDX_BASE(ELT) \
    ((elt)->tag << UAO_SWHASH_CLUSTER_SHIFT)

/* The hash function. */
#define	UAO_SWHASH_HASH(aobj, idx) \
    (&(aobj)->u_swhash[(((idx) >> UAO_SWHASH_CLUSTER_SHIFT) \
    & (aobj)->u_swhashmask)])

/*
 * The threshold which determines whether we will use an array or a
 * hash table to store the list of allocated swap blocks.
 */
#define	UAO_SWHASH_THRESHOLD		(UAO_SWHASH_CLUSTER_SIZE * 4)
#define	UAO_USES_SWHASH(aobj) \
    ((aobj)->u_pages > UAO_SWHASH_THRESHOLD)

/* The number of buckets in a hash, with an upper bound. */
#define	UAO_SWHASH_MAXBUCKETS		256
#define	UAO_SWHASH_BUCKETS(aobj) \
    (MIN((aobj)->u_pages >> UAO_SWHASH_CLUSTER_SHIFT, UAO_SWHASH_MAXBUCKETS))

/*
 * uao_swhash_elt: when a hash table is being used, this structure defines
 * the format of an entry in the bucket list.
 */

struct uao_swhash_elt {
	LIST_ENTRY(uao_swhash_elt) list;	/* the hash list */
	voff_t tag;				/* our 'tag' */
	int count;				/* our number of active slots */
	int slots[UAO_SWHASH_CLUSTER_SIZE];	/* the slots */
};

/*
 * uao_swhash: the swap hash table structure
 */

LIST_HEAD(uao_swhash, uao_swhash_elt);

/*
 * uao_swhash_elt_pool: pool of uao_swhash_elt structures.
 * Note: pages for this pool must not come from a pageable kernel map.
 */
static struct pool	uao_swhash_elt_pool	__cacheline_aligned;

/*
 * uvm_aobj: the actual anon-backed uvm_object
 *
 * => the uvm_object is at the top of the structure, this allows
 *   (struct uvm_aobj *) == (struct uvm_object *)
 * => only one of u_swslots and u_swhash is used in any given aobj
 */

struct uvm_aobj {
	struct uvm_object u_obj; /* has: lock, pgops, #pages, #refs */
	pgoff_t u_pages;	 /* number of pages in entire object */
	int u_flags;		 /* the flags (see uvm_aobj.h) */
	int *u_swslots;		 /* array of offset->swapslot mappings */
				 /*
				  * hashtable of offset->swapslot mappings
				  * (u_swhash is an array of bucket heads)
				  */
	struct uao_swhash *u_swhash;
	u_long u_swhashmask;		/* mask for hashtable */
	LIST_ENTRY(uvm_aobj) u_list;	/* global list of aobjs */
	int u_freelist;		  /* freelist to allocate pages from */
};

static void	uao_free(struct uvm_aobj *);
static int	uao_get(struct uvm_object *, voff_t, struct vm_page **,
		    int *, int, vm_prot_t, int, int);
static int	uao_put(struct uvm_object *, voff_t, voff_t, int);

#if defined(VMSWAP)
static struct uao_swhash_elt *uao_find_swhash_elt
    (struct uvm_aobj *, int, bool);

static bool uao_pagein(struct uvm_aobj *, int, int);
static bool uao_pagein_page(struct uvm_aobj *, int);
#endif /* defined(VMSWAP) */

static struct vm_page	*uao_pagealloc(struct uvm_object *, voff_t, int);

/*
 * aobj_pager
 *
 * note that some functions (e.g. put) are handled elsewhere
 */

const struct uvm_pagerops aobj_pager = {
	.pgo_reference = uao_reference,
	.pgo_detach = uao_detach,
	.pgo_get = uao_get,
	.pgo_put = uao_put,
};

/*
 * uao_list: global list of active aobjs, locked by uao_list_lock
 */

static LIST_HEAD(aobjlist, uvm_aobj) uao_list	__cacheline_aligned;
static kmutex_t		uao_list_lock		__cacheline_aligned;

/*
 * hash table/array related functions
 */

#if defined(VMSWAP)

/*
 * uao_find_swhash_elt: find (or create) a hash table entry for a page
 * offset.
 *
 * => the object should be locked by the caller
 */

static struct uao_swhash_elt *
uao_find_swhash_elt(struct uvm_aobj *aobj, int pageidx, bool create)
{
	struct uao_swhash *swhash;
	struct uao_swhash_elt *elt;
	voff_t page_tag;

	swhash = UAO_SWHASH_HASH(aobj, pageidx);
	page_tag = UAO_SWHASH_ELT_TAG(pageidx);

	/*
	 * now search the bucket for the requested tag
	 */

	LIST_FOREACH(elt, swhash, list) {
		if (elt->tag == page_tag) {
			return elt;
		}
	}
	if (!create) {
		return NULL;
	}

	/*
	 * allocate a new entry for the bucket and init/insert it in
	 */

	elt = pool_get(&uao_swhash_elt_pool, PR_NOWAIT);
	if (elt == NULL) {
		return NULL;
	}
	LIST_INSERT_HEAD(swhash, elt, list);
	elt->tag = page_tag;
	elt->count = 0;
	memset(elt->slots, 0, sizeof(elt->slots));
	return elt;
}

/*
 * uao_find_swslot: find the swap slot number for an aobj/pageidx
 *
 * => object must be locked by caller
 */

int
uao_find_swslot(struct uvm_object *uobj, int pageidx)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uao_swhash_elt *elt;

	KASSERT(UVM_OBJ_IS_AOBJ(uobj));

	/*
	 * if noswap flag is set, then we never return a slot
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP)
		return 0;

	/*
	 * if hashing, look in hash table.
	 */

	if (UAO_USES_SWHASH(aobj)) {
		elt = uao_find_swhash_elt(aobj, pageidx, false);
		return elt ? UAO_SWHASH_ELT_PAGESLOT(elt, pageidx) : 0;
	}

	/*
	 * otherwise, look in the array
	 */

	return aobj->u_swslots[pageidx];
}

/*
 * uao_set_swslot: set the swap slot for a page in an aobj.
 *
 * => setting a slot to zero frees the slot
 * => object must be locked by caller
 * => we return the old slot number, or -1 if we failed to allocate
 *    memory to record the new slot number
 */

int
uao_set_swslot(struct uvm_object *uobj, int pageidx, int slot)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uao_swhash_elt *elt;
	int oldslot;
	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pdhist, "aobj %#jx pageidx %jd slot %jd",
	    (uintptr_t)aobj, pageidx, slot, 0);

	KASSERT(rw_write_held(uobj->vmobjlock) || uobj->uo_refs == 0);
	KASSERT(UVM_OBJ_IS_AOBJ(uobj));

	/*
	 * if noswap flag is set, then we can't set a non-zero slot.
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP) {
		KASSERTMSG(slot == 0, "uao_set_swslot: no swap object");
		return 0;
	}

	/*
	 * are we using a hash table?  if so, add it in the hash.
	 */

	if (UAO_USES_SWHASH(aobj)) {

		/*
		 * Avoid allocating an entry just to free it again if
		 * the page had not swap slot in the first place, and
		 * we are freeing.
		 */

		elt = uao_find_swhash_elt(aobj, pageidx, slot != 0);
		if (elt == NULL) {
			return slot ? -1 : 0;
		}

		oldslot = UAO_SWHASH_ELT_PAGESLOT(elt, pageidx);
		UAO_SWHASH_ELT_PAGESLOT(elt, pageidx) = slot;

		/*
		 * now adjust the elt's reference counter and free it if we've
		 * dropped it to zero.
		 */

		if (slot) {
			if (oldslot == 0)
				elt->count++;
		} else {
			if (oldslot)
				elt->count--;

			if (elt->count == 0) {
				LIST_REMOVE(elt, list);
				pool_put(&uao_swhash_elt_pool, elt);
			}
		}
	} else {
		/* we are using an array */
		oldslot = aobj->u_swslots[pageidx];
		aobj->u_swslots[pageidx] = slot;
	}
	return oldslot;
}

#endif /* defined(VMSWAP) */

/*
 * end of hash/array functions
 */

/*
 * uao_free: free all resources held by an aobj, and then free the aobj
 *
 * => the aobj should be dead
 */

static void
uao_free(struct uvm_aobj *aobj)
{
	struct uvm_object *uobj = &aobj->u_obj;

	KASSERT(UVM_OBJ_IS_AOBJ(uobj));
	KASSERT(rw_write_held(uobj->vmobjlock));
	uao_dropswap_range(uobj, 0, 0);
	rw_exit(uobj->vmobjlock);

#if defined(VMSWAP)
	if (UAO_USES_SWHASH(aobj)) {

		/*
		 * free the hash table itself.
		 */

		hashdone(aobj->u_swhash, HASH_LIST, aobj->u_swhashmask);
	} else {

		/*
		 * free the array itself.
		 */

		kmem_free(aobj->u_swslots, aobj->u_pages * sizeof(int));
	}
#endif /* defined(VMSWAP) */

	/*
	 * finally free the aobj itself
	 */

	uvm_obj_destroy(uobj, true);
	kmem_free(aobj, sizeof(struct uvm_aobj));
}

/*
 * pager functions
 */

/*
 * uao_create: create an aobj of the given size and return its uvm_object.
 *
 * => for normal use, flags are always zero
 * => for the kernel object, the flags are:
 *	UAO_FLAG_KERNOBJ - allocate the kernel object (can only happen once)
 *	UAO_FLAG_KERNSWAP - enable swapping of kernel object ("           ")
 */

struct uvm_object *
uao_create(voff_t size, int flags)
{
	static struct uvm_aobj kernel_object_store;
	static krwlock_t bootstrap_kernel_object_lock;
	static int kobj_alloced __diagused = 0;
	pgoff_t pages = round_page((uint64_t)size) >> PAGE_SHIFT;
	struct uvm_aobj *aobj;
	int refs;

	/*
	 * Allocate a new aobj, unless kernel object is requested.
	 */

	if (flags & UAO_FLAG_KERNOBJ) {
		KASSERT(!kobj_alloced);
		aobj = &kernel_object_store;
		aobj->u_pages = pages;
		aobj->u_flags = UAO_FLAG_NOSWAP;
		refs = UVM_OBJ_KERN;
		kobj_alloced = UAO_FLAG_KERNOBJ;
	} else if (flags & UAO_FLAG_KERNSWAP) {
		KASSERT(kobj_alloced == UAO_FLAG_KERNOBJ);
		aobj = &kernel_object_store;
		kobj_alloced = UAO_FLAG_KERNSWAP;
		refs = 0xdeadbeaf; /* XXX: gcc */
	} else {
		aobj = kmem_alloc(sizeof(struct uvm_aobj), KM_SLEEP);
		aobj->u_pages = pages;
		aobj->u_flags = 0;
		refs = 1;
	}

	/*
	 * no freelist by default
	 */

	aobj->u_freelist = VM_NFREELIST;

	/*
 	 * allocate hash/array if necessary
 	 *
 	 * note: in the KERNSWAP case no need to worry about locking since
 	 * we are still booting we should be the only thread around.
 	 */

	const int kernswap = (flags & UAO_FLAG_KERNSWAP) != 0;
	if (flags == 0 || kernswap) {
#if defined(VMSWAP)

		/* allocate hash table or array depending on object size */
		if (UAO_USES_SWHASH(aobj)) {
			aobj->u_swhash = hashinit(UAO_SWHASH_BUCKETS(aobj),
			    HASH_LIST, true, &aobj->u_swhashmask);
		} else {
			aobj->u_swslots = kmem_zalloc(pages * sizeof(int),
			    KM_SLEEP);
		}
#endif /* defined(VMSWAP) */

		/*
		 * Replace kernel_object's temporary static lock with
		 * a regular rw_obj.  We cannot use uvm_obj_setlock()
		 * because that would try to free the old lock.
		 */

		if (kernswap) {
			aobj->u_obj.vmobjlock = rw_obj_alloc();
			rw_destroy(&bootstrap_kernel_object_lock);
		}
		if (flags) {
			aobj->u_flags &= ~UAO_FLAG_NOSWAP; /* clear noswap */
			return &aobj->u_obj;
		}
	}

	/*
	 * Initialise UVM object.
	 */

	const bool kernobj = (flags & UAO_FLAG_KERNOBJ) != 0;
	uvm_obj_init(&aobj->u_obj, &aobj_pager, !kernobj, refs);
	if (__predict_false(kernobj)) {
		/* Use a temporary static lock for kernel_object. */
		rw_init(&bootstrap_kernel_object_lock);
		uvm_obj_setlock(&aobj->u_obj, &bootstrap_kernel_object_lock);
	}

	/*
 	 * now that aobj is ready, add it to the global list
 	 */

	mutex_enter(&uao_list_lock);
	LIST_INSERT_HEAD(&uao_list, aobj, u_list);
	mutex_exit(&uao_list_lock);
	return(&aobj->u_obj);
}

/*
 * uao_set_pgfl: allocate pages only from the specified freelist.
 *
 * => must be called before any pages are allocated for the object.
 * => reset by setting it to VM_NFREELIST, meaning any freelist.
 */

void
uao_set_pgfl(struct uvm_object *uobj, int freelist)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;

	KASSERTMSG((0 <= freelist), "invalid freelist %d", freelist);
	KASSERTMSG((freelist <= VM_NFREELIST), "invalid freelist %d",
	    freelist);

	aobj->u_freelist = freelist;
}

/*
 * uao_pagealloc: allocate a page for aobj.
 */

static inline struct vm_page *
uao_pagealloc(struct uvm_object *uobj, voff_t offset, int flags)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;

	if (__predict_true(aobj->u_freelist == VM_NFREELIST))
		return uvm_pagealloc(uobj, offset, NULL, flags);
	else
		return uvm_pagealloc_strat(uobj, offset, NULL, flags,
		    UVM_PGA_STRAT_ONLY, aobj->u_freelist);
}

/*
 * uao_init: set up aobj pager subsystem
 *
 * => called at boot time from uvm_pager_init()
 */

void
uao_init(void)
{
	static int uao_initialized;

	if (uao_initialized)
		return;
	uao_initialized = true;
	LIST_INIT(&uao_list);
	mutex_init(&uao_list_lock, MUTEX_DEFAULT, IPL_NONE);
	pool_init(&uao_swhash_elt_pool, sizeof(struct uao_swhash_elt),
	    0, 0, 0, "uaoeltpl", NULL, IPL_VM);
}

/*
 * uao_reference: hold a reference to an anonymous UVM object.
 */
void
uao_reference(struct uvm_object *uobj)
{
	/* Kernel object is persistent. */
	if (UVM_OBJ_IS_KERN_OBJECT(uobj)) {
		return;
	}
	atomic_inc_uint(&uobj->uo_refs);
}

/*
 * uao_detach: drop a reference to an anonymous UVM object.
 */
void
uao_detach(struct uvm_object *uobj)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uvm_page_array a;
	struct vm_page *pg;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	/*
	 * Detaching from kernel object is a NOP.
	 */

	if (UVM_OBJ_IS_KERN_OBJECT(uobj))
		return;

	/*
	 * Drop the reference.  If it was the last one, destroy the object.
	 */

	KASSERT(uobj->uo_refs > 0);
	UVMHIST_LOG(maphist,"  (uobj=%#jx)  ref=%jd",
	    (uintptr_t)uobj, uobj->uo_refs, 0, 0);
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_release();
#endif
	if (atomic_dec_uint_nv(&uobj->uo_refs) > 0) {
		UVMHIST_LOG(maphist, "<- done (rc>0)", 0,0,0,0);
		return;
	}
#ifndef __HAVE_ATOMIC_AS_MEMBAR
	membar_acquire();
#endif

	/*
	 * Remove the aobj from the global list.
	 */

	mutex_enter(&uao_list_lock);
	LIST_REMOVE(aobj, u_list);
	mutex_exit(&uao_list_lock);

	/*
	 * Free all the pages left in the aobj.  For each page, when the
	 * page is no longer busy (and thus after any disk I/O that it is
	 * involved in is complete), release any swap resources and free
	 * the page itself.
	 */
	uvm_page_array_init(&a, uobj, 0);
	rw_enter(uobj->vmobjlock, RW_WRITER);
	while ((pg = uvm_page_array_fill_and_peek(&a, 0, 0)) != NULL) {
		uvm_page_array_advance(&a);
		pmap_page_protect(pg, VM_PROT_NONE);
		if (pg->flags & PG_BUSY) {
			uvm_pagewait(pg, uobj->vmobjlock, "uao_det");
			uvm_page_array_clear(&a);
			rw_enter(uobj->vmobjlock, RW_WRITER);
			continue;
		}
		uao_dropswap(&aobj->u_obj, pg->offset >> PAGE_SHIFT);
		uvm_pagefree(pg);
	}
	uvm_page_array_fini(&a);

	/*
	 * Finally, free the anonymous UVM object itself.
	 */

	uao_free(aobj);
}

/*
 * uao_put: flush pages out of a uvm object
 *
 * => object should be locked by caller.  we may _unlock_ the object
 *	if (and only if) we need to clean a page (PGO_CLEANIT).
 *	XXXJRT Currently, however, we don't.  In the case of cleaning
 *	XXXJRT a page, we simply just deactivate it.  Should probably
 *	XXXJRT handle this better, in the future (although "flushing"
 *	XXXJRT anonymous memory isn't terribly important).
 * => if PGO_CLEANIT is not set, then we will neither unlock the object
 *	or block.
 * => if PGO_ALLPAGE is set, then all pages in the object are valid targets
 *	for flushing.
 * => we return 0 unless we encountered some sort of I/O error
 *	XXXJRT currently never happens, as we never directly initiate
 *	XXXJRT I/O
 */

static int
uao_put(struct uvm_object *uobj, voff_t start, voff_t stop, int flags)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uvm_page_array a;
	struct vm_page *pg;
	voff_t curoff;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	KASSERT(UVM_OBJ_IS_AOBJ(uobj));
	KASSERT(rw_write_held(uobj->vmobjlock));

	if (flags & PGO_ALLPAGES) {
		start = 0;
		stop = aobj->u_pages << PAGE_SHIFT;
	} else {
		start = trunc_page(start);
		if (stop == 0) {
			stop = aobj->u_pages << PAGE_SHIFT;
		} else {
			stop = round_page(stop);
		}
		if (stop > (uint64_t)(aobj->u_pages << PAGE_SHIFT)) {
			printf("uao_put: strange, got an out of range "
			    "flush %#jx > %#jx (fixed)\n",
			    (uintmax_t)stop,
			    (uintmax_t)(aobj->u_pages << PAGE_SHIFT));
			stop = aobj->u_pages << PAGE_SHIFT;
		}
	}
	UVMHIST_LOG(maphist,
	    " flush start=%#jx, stop=%#jx, flags=%#jx",
	    start, stop, flags, 0);

	/*
	 * Don't need to do any work here if we're not freeing
	 * or deactivating pages.
	 */

	if ((flags & (PGO_DEACTIVATE|PGO_FREE)) == 0) {
		rw_exit(uobj->vmobjlock);
		return 0;
	}

	/* locked: uobj */
	uvm_page_array_init(&a, uobj, 0);
	curoff = start;
	while ((pg = uvm_page_array_fill_and_peek(&a, curoff, 0)) != NULL) {
		if (pg->offset >= stop) {
			break;
		}

		/*
		 * wait and try again if the page is busy.
		 */

		if (pg->flags & PG_BUSY) {
			uvm_pagewait(pg, uobj->vmobjlock, "uao_put");
			uvm_page_array_clear(&a);
			rw_enter(uobj->vmobjlock, RW_WRITER);
			continue;
		}
		uvm_page_array_advance(&a);
		curoff = pg->offset + PAGE_SIZE;

		switch (flags & (PGO_CLEANIT|PGO_FREE|PGO_DEACTIVATE)) {

		/*
		 * XXX In these first 3 cases, we always just
		 * XXX deactivate the page.  We may want to
		 * XXX handle the different cases more specifically
		 * XXX in the future.
		 */

		case PGO_CLEANIT|PGO_FREE:
		case PGO_CLEANIT|PGO_DEACTIVATE:
		case PGO_DEACTIVATE:
 deactivate_it:
 			uvm_pagelock(pg);
			uvm_pagedeactivate(pg);
 			uvm_pageunlock(pg);
			break;

		case PGO_FREE:
			/*
			 * If there are multiple references to
			 * the object, just deactivate the page.
			 */

			if (uobj->uo_refs > 1)
				goto deactivate_it;

			/*
			 * free the swap slot and the page.
			 */

			pmap_page_protect(pg, VM_PROT_NONE);

			/*
			 * freeing swapslot here is not strictly necessary.
			 * however, leaving it here doesn't save much
			 * because we need to update swap accounting anyway.
			 */

			uao_dropswap(uobj, pg->offset >> PAGE_SHIFT);
			uvm_pagefree(pg);
			break;

		default:
			panic("%s: impossible", __func__);
		}
	}
	rw_exit(uobj->vmobjlock);
	uvm_page_array_fini(&a);
	return 0;
}

/*
 * uao_get: fetch me a page
 *
 * we have three cases:
 * 1: page is resident     -> just return the page.
 * 2: page is zero-fill    -> allocate a new page and zero it.
 * 3: page is swapped out  -> fetch the page from swap.
 *
 * case 1 can be handled with PGO_LOCKED, cases 2 and 3 cannot.
 * so, if the "center" page hits case 2/3 then we will need to return EBUSY.
 *
 * => prefer map unlocked (not required)
 * => object must be locked!  we will _unlock_ it before starting any I/O.
 * => flags: PGO_LOCKED: fault data structures are locked
 * => NOTE: offset is the offset of pps[0], _NOT_ pps[centeridx]
 * => NOTE: caller must check for released pages!!
 */

static int
uao_get(struct uvm_object *uobj, voff_t offset, struct vm_page **pps,
    int *npagesp, int centeridx, vm_prot_t access_type, int advice, int flags)
{
	voff_t current_offset;
	struct vm_page *ptmp;
	int lcv, gotpages, maxpages, swslot, pageidx;
	bool overwrite = ((flags & PGO_OVERWRITE) != 0);
	struct uvm_page_array a;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(pdhist, "aobj=%#jx offset=%jd, flags=%#jx",
		    (uintptr_t)uobj, offset, flags,0);

	/*
	 * the object must be locked.  it can only be a read lock when
	 * processing a read fault with PGO_LOCKED.
	 */

	KASSERT(UVM_OBJ_IS_AOBJ(uobj));
	KASSERT(rw_lock_held(uobj->vmobjlock));
	KASSERT(rw_write_held(uobj->vmobjlock) ||
	   ((flags & PGO_LOCKED) != 0 && (access_type & VM_PROT_WRITE) == 0));

	/*
 	 * get number of pages
 	 */

	maxpages = *npagesp;

	/*
 	 * step 1: handled the case where fault data structures are locked.
 	 */

	if (flags & PGO_LOCKED) {

		/*
 		 * step 1a: get pages that are already resident.   only do
		 * this if the data structures are locked (i.e. the first
		 * time through).
 		 */

		uvm_page_array_init(&a, uobj, 0);
		gotpages = 0;	/* # of pages we got so far */
		for (lcv = 0; lcv < maxpages; lcv++) {
			ptmp = uvm_page_array_fill_and_peek(&a,
			    offset + (lcv << PAGE_SHIFT), maxpages);
			if (ptmp == NULL) {
				break;
			}
			KASSERT(ptmp->offset >= offset);
			lcv = (ptmp->offset - offset) >> PAGE_SHIFT;
			if (lcv >= maxpages) {
				break;
			}
			uvm_page_array_advance(&a);

			/*
			 * to be useful must get a non-busy page
			 */

			if ((ptmp->flags & PG_BUSY) != 0) {
				continue;
			}

			/*
			 * useful page: plug it in our result array
			 */

			KASSERT(uvm_pagegetdirty(ptmp) !=
			    UVM_PAGE_STATUS_CLEAN);
			pps[lcv] = ptmp;
			gotpages++;
		}
		uvm_page_array_fini(&a);

		/*
 		 * step 1b: now we've either done everything needed or we
		 * to unlock and do some waiting or I/O.
 		 */

		UVMHIST_LOG(pdhist, "<- done (done=%jd)",
		    (pps[centeridx] != NULL), 0,0,0);
		*npagesp = gotpages;
		return pps[centeridx] != NULL ? 0 : EBUSY;
	}

	/*
 	 * step 2: get non-resident or busy pages.
 	 * object is locked.   data structures are unlocked.
 	 */

	if ((flags & PGO_SYNCIO) == 0) {
		goto done;
	}

	uvm_page_array_init(&a, uobj, 0);
	for (lcv = 0, current_offset = offset ; lcv < maxpages ;) {

		/*
 		 * we have yet to locate the current page (pps[lcv]).   we
		 * first look for a page that is already at the current offset.
		 * if we find a page, we check to see if it is busy or
		 * released.  if that is the case, then we sleep on the page
		 * until it is no longer busy or released and repeat the lookup.
		 * if the page we found is neither busy nor released, then we
		 * busy it (so we own it) and plug it into pps[lcv].   we are
		 * ready to move on to the next page.
 		 */

		ptmp = uvm_page_array_fill_and_peek(&a, current_offset,
		    maxpages - lcv);

		if (ptmp != NULL && ptmp->offset == current_offset) {
			/* page is there, see if we need to wait on it */
			if ((ptmp->flags & PG_BUSY) != 0) {
				UVMHIST_LOG(pdhist,
				    "sleeping, ptmp->flags %#jx\n",
				    ptmp->flags,0,0,0);
				uvm_pagewait(ptmp, uobj->vmobjlock, "uao_get");
				rw_enter(uobj->vmobjlock, RW_WRITER);
				uvm_page_array_clear(&a);
				continue;
			}

			/*
 			 * if we get here then the page is resident and
			 * unbusy.  we busy it now (so we own it).  if
			 * overwriting, mark the page dirty up front as
			 * it will be zapped via an unmanaged mapping.
 			 */

			KASSERT(uvm_pagegetdirty(ptmp) !=
			    UVM_PAGE_STATUS_CLEAN);
			if (overwrite) {
				uvm_pagemarkdirty(ptmp, UVM_PAGE_STATUS_DIRTY);
			}
			/* we own it, caller must un-busy */
			ptmp->flags |= PG_BUSY;
			UVM_PAGE_OWN(ptmp, "uao_get2");
			pps[lcv++] = ptmp;
			current_offset += PAGE_SIZE;
			uvm_page_array_advance(&a);
			continue;
		} else {
			KASSERT(ptmp == NULL || ptmp->offset > current_offset);
		}

		/*
		 * not resident.  allocate a new busy/fake/clean page in the
		 * object.  if it's in swap we need to do I/O to fill in the
		 * data, otherwise the page needs to be cleared: if it's not
		 * destined to be overwritten, then zero it here and now.
		 */

		pageidx = current_offset >> PAGE_SHIFT;
		swslot = uao_find_swslot(uobj, pageidx);
		ptmp = uao_pagealloc(uobj, current_offset,
		    swslot != 0 || overwrite ? 0 : UVM_PGA_ZERO);

		/* out of RAM? */
		if (ptmp == NULL) {
			rw_exit(uobj->vmobjlock);
			UVMHIST_LOG(pdhist, "sleeping, ptmp == NULL",0,0,0,0);
			uvm_wait("uao_getpage");
			rw_enter(uobj->vmobjlock, RW_WRITER);
			uvm_page_array_clear(&a);
			continue;
		}

		/*
 		 * if swslot == 0, page hasn't existed before and is zeroed.
 		 * otherwise we have a "fake/busy/clean" page that we just
 		 * allocated.  do the needed "i/o", reading from swap.
 		 */

		if (swslot != 0) {
#if defined(VMSWAP)
			int error;

			UVMHIST_LOG(pdhist, "pagein from swslot %jd",
			     swslot, 0,0,0);

			/*
			 * page in the swapped-out page.
			 * unlock object for i/o, relock when done.
			 */

			uvm_page_array_clear(&a);
			rw_exit(uobj->vmobjlock);
			error = uvm_swap_get(ptmp, swslot, PGO_SYNCIO);
			rw_enter(uobj->vmobjlock, RW_WRITER);

			/*
			 * I/O done.  check for errors.
			 */

			if (error != 0) {
				UVMHIST_LOG(pdhist, "<- done (error=%jd)",
				    error,0,0,0);

				/*
				 * remove the swap slot from the aobj
				 * and mark the aobj as having no real slot.
				 * don't free the swap slot, thus preventing
				 * it from being used again.
				 */

				swslot = uao_set_swslot(uobj, pageidx,
				    SWSLOT_BAD);
				if (swslot > 0) {
					uvm_swap_markbad(swslot, 1);
				}

				uvm_pagefree(ptmp);
				rw_exit(uobj->vmobjlock);
				UVMHIST_LOG(pdhist, "<- done (error)",
				    error,lcv,0,0);
				if (lcv != 0) {
					uvm_page_unbusy(pps, lcv);
				}
				memset(pps, 0, maxpages * sizeof(pps[0]));
				uvm_page_array_fini(&a);
				return error;
			}
#else /* defined(VMSWAP) */
			panic("%s: pagein", __func__);
#endif /* defined(VMSWAP) */
		}

		/*
		 * note that we will allow the page being writably-mapped
		 * (!PG_RDONLY) regardless of access_type.  if overwrite,
		 * the page can be modified through an unmanaged mapping
		 * so mark it dirty up front.
		 */
		if (overwrite) {
			uvm_pagemarkdirty(ptmp, UVM_PAGE_STATUS_DIRTY);
		} else {
			uvm_pagemarkdirty(ptmp, UVM_PAGE_STATUS_UNKNOWN);
		}

		/*
 		 * we got the page!   clear the fake flag (indicates valid
		 * data now in page) and plug into our result array.   note
		 * that page is still busy.
 		 *
 		 * it is the callers job to:
 		 * => check if the page is released
 		 * => unbusy the page
 		 * => activate the page
 		 */
		KASSERT(uvm_pagegetdirty(ptmp) != UVM_PAGE_STATUS_CLEAN);
		KASSERT((ptmp->flags & PG_FAKE) != 0);
		KASSERT(ptmp->offset == current_offset);
		ptmp->flags &= ~PG_FAKE;
		pps[lcv++] = ptmp;
		current_offset += PAGE_SIZE;
	}
	uvm_page_array_fini(&a);

	/*
 	 * finally, unlock object and return.
 	 */

done:
	rw_exit(uobj->vmobjlock);
	UVMHIST_LOG(pdhist, "<- done (OK)",0,0,0,0);
	return 0;
}

#if defined(VMSWAP)

/*
 * uao_dropswap:  release any swap resources from this aobj page.
 *
 * => aobj must be locked or have a reference count of 0.
 */

void
uao_dropswap(struct uvm_object *uobj, int pageidx)
{
	int slot;

	KASSERT(UVM_OBJ_IS_AOBJ(uobj));

	slot = uao_set_swslot(uobj, pageidx, 0);
	if (slot) {
		uvm_swap_free(slot, 1);
	}
}

/*
 * page in every page in every aobj that is paged-out to a range of swslots.
 *
 * => nothing should be locked.
 * => returns true if pagein was aborted due to lack of memory.
 */

bool
uao_swap_off(int startslot, int endslot)
{
	struct uvm_aobj *aobj;

	/*
	 * Walk the list of all anonymous UVM objects.  Grab the first.
	 */
	mutex_enter(&uao_list_lock);
	if ((aobj = LIST_FIRST(&uao_list)) == NULL) {
		mutex_exit(&uao_list_lock);
		return false;
	}
	uao_reference(&aobj->u_obj);

	do {
		struct uvm_aobj *nextaobj;
		bool rv;

		/*
		 * Prefetch the next object and immediately hold a reference
		 * on it, so neither the current nor the next entry could
		 * disappear while we are iterating.
		 */
		if ((nextaobj = LIST_NEXT(aobj, u_list)) != NULL) {
			uao_reference(&nextaobj->u_obj);
		}
		mutex_exit(&uao_list_lock);

		/*
		 * Page in all pages in the swap slot range.
		 */
		rw_enter(aobj->u_obj.vmobjlock, RW_WRITER);
		rv = uao_pagein(aobj, startslot, endslot);
		rw_exit(aobj->u_obj.vmobjlock);

		/* Drop the reference of the current object. */
		uao_detach(&aobj->u_obj);
		if (rv) {
			if (nextaobj) {
				uao_detach(&nextaobj->u_obj);
			}
			return rv;
		}

		aobj = nextaobj;
		mutex_enter(&uao_list_lock);
	} while (aobj);

	mutex_exit(&uao_list_lock);
	return false;
}

/*
 * page in any pages from aobj in the given range.
 *
 * => aobj must be locked and is returned locked.
 * => returns true if pagein was aborted due to lack of memory.
 */
static bool
uao_pagein(struct uvm_aobj *aobj, int startslot, int endslot)
{
	bool rv;

	if (UAO_USES_SWHASH(aobj)) {
		struct uao_swhash_elt *elt;
		int buck;

restart:
		for (buck = aobj->u_swhashmask; buck >= 0; buck--) {
			for (elt = LIST_FIRST(&aobj->u_swhash[buck]);
			     elt != NULL;
			     elt = LIST_NEXT(elt, list)) {
				int i;

				for (i = 0; i < UAO_SWHASH_CLUSTER_SIZE; i++) {
					int slot = elt->slots[i];

					/*
					 * if the slot isn't in range, skip it.
					 */

					if (slot < startslot ||
					    slot >= endslot) {
						continue;
					}

					/*
					 * process the page,
					 * the start over on this object
					 * since the swhash elt
					 * may have been freed.
					 */

					rv = uao_pagein_page(aobj,
					  UAO_SWHASH_ELT_PAGEIDX_BASE(elt) + i);
					if (rv) {
						return rv;
					}
					goto restart;
				}
			}
		}
	} else {
		int i;

		for (i = 0; i < aobj->u_pages; i++) {
			int slot = aobj->u_swslots[i];

			/*
			 * if the slot isn't in range, skip it
			 */

			if (slot < startslot || slot >= endslot) {
				continue;
			}

			/*
			 * process the page.
			 */

			rv = uao_pagein_page(aobj, i);
			if (rv) {
				return rv;
			}
		}
	}

	return false;
}

/*
 * uao_pagein_page: page in a single page from an anonymous UVM object.
 *
 * => Returns true if pagein was aborted due to lack of memory.
 * => Object must be locked and is returned locked.
 */

static bool
uao_pagein_page(struct uvm_aobj *aobj, int pageidx)
{
	struct uvm_object *uobj = &aobj->u_obj;
	struct vm_page *pg;
	int rv, npages;

	pg = NULL;
	npages = 1;

	KASSERT(rw_write_held(uobj->vmobjlock));
	rv = uao_get(uobj, (voff_t)pageidx << PAGE_SHIFT, &pg, &npages,
	    0, VM_PROT_READ | VM_PROT_WRITE, 0, PGO_SYNCIO);

	/*
	 * relock and finish up.
	 */

	rw_enter(uobj->vmobjlock, RW_WRITER);
	switch (rv) {
	case 0:
		break;

	case EIO:
	case ERESTART:

		/*
		 * nothing more to do on errors.
		 * ERESTART can only mean that the anon was freed,
		 * so again there's nothing to do.
		 */

		return false;

	default:
		return true;
	}

	/*
	 * ok, we've got the page now.
	 * mark it as dirty, clear its swslot and un-busy it.
	 */
	uao_dropswap(&aobj->u_obj, pageidx);

	/*
	 * make sure it's on a page queue.
	 */
	uvm_pagelock(pg);
	uvm_pageenqueue(pg);
	uvm_pagewakeup(pg);
	uvm_pageunlock(pg);

	pg->flags &= ~(PG_BUSY|PG_FAKE);
	uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
	UVM_PAGE_OWN(pg, NULL);

	return false;
}

/*
 * uao_dropswap_range: drop swapslots in the range.
 *
 * => aobj must be locked and is returned locked.
 * => start is inclusive.  end is exclusive.
 */

void
uao_dropswap_range(struct uvm_object *uobj, voff_t start, voff_t end)
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	int swpgonlydelta = 0;

	KASSERT(UVM_OBJ_IS_AOBJ(uobj));
	KASSERT(rw_write_held(uobj->vmobjlock));

	if (end == 0) {
		end = INT64_MAX;
	}

	if (UAO_USES_SWHASH(aobj)) {
		int i, hashbuckets = aobj->u_swhashmask + 1;
		voff_t taghi;
		voff_t taglo;

		taglo = UAO_SWHASH_ELT_TAG(start);
		taghi = UAO_SWHASH_ELT_TAG(end);

		for (i = 0; i < hashbuckets; i++) {
			struct uao_swhash_elt *elt, *next;

			for (elt = LIST_FIRST(&aobj->u_swhash[i]);
			     elt != NULL;
			     elt = next) {
				int startidx, endidx;
				int j;

				next = LIST_NEXT(elt, list);

				if (elt->tag < taglo || taghi < elt->tag) {
					continue;
				}

				if (elt->tag == taglo) {
					startidx =
					    UAO_SWHASH_ELT_PAGESLOT_IDX(start);
				} else {
					startidx = 0;
				}

				if (elt->tag == taghi) {
					endidx =
					    UAO_SWHASH_ELT_PAGESLOT_IDX(end);
				} else {
					endidx = UAO_SWHASH_CLUSTER_SIZE;
				}

				for (j = startidx; j < endidx; j++) {
					int slot = elt->slots[j];

					KASSERT(uvm_pagelookup(&aobj->u_obj,
					    (UAO_SWHASH_ELT_PAGEIDX_BASE(elt)
					    + j) << PAGE_SHIFT) == NULL);
					if (slot > 0) {
						uvm_swap_free(slot, 1);
						swpgonlydelta++;
						KASSERT(elt->count > 0);
						elt->slots[j] = 0;
						elt->count--;
					}
				}

				if (elt->count == 0) {
					LIST_REMOVE(elt, list);
					pool_put(&uao_swhash_elt_pool, elt);
				}
			}
		}
	} else {
		int i;

		if (aobj->u_pages < end) {
			end = aobj->u_pages;
		}
		for (i = start; i < end; i++) {
			int slot = aobj->u_swslots[i];

			if (slot > 0) {
				uvm_swap_free(slot, 1);
				swpgonlydelta++;
			}
		}
	}

	/*
	 * adjust the counter of pages only in swap for all
	 * the swap slots we've freed.
	 */

	if (swpgonlydelta > 0) {
		KASSERT(uvmexp.swpgonly >= swpgonlydelta);
		atomic_add_int(&uvmexp.swpgonly, -swpgonlydelta);
	}
}

#endif /* defined(VMSWAP) */
