/*	$NetBSD: uvm_aobj.c,v 1.39.2.6 2002/04/01 07:49:20 nathanw Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: uvm_aobj.c,v 1.39.2.6 2002/04/01 07:49:20 nathanw Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>

/*
 * an aobj manages anonymous-memory backed uvm_objects.   in addition
 * to keeping the list of resident pages, it also keeps a list of
 * allocated swap blocks.  depending on the size of the aobj this list
 * of allocated swap blocks is either stored in an array (small objects)
 * or in a hash table (large objects).
 */

/*
 * local structures
 */

/*
 * for hash tables, we break the address space of the aobj into blocks
 * of UAO_SWHASH_CLUSTER_SIZE pages.   we require the cluster size to
 * be a power of two.
 */

#define UAO_SWHASH_CLUSTER_SHIFT 4
#define UAO_SWHASH_CLUSTER_SIZE (1 << UAO_SWHASH_CLUSTER_SHIFT)

/* get the "tag" for this page index */
#define UAO_SWHASH_ELT_TAG(PAGEIDX) \
	((PAGEIDX) >> UAO_SWHASH_CLUSTER_SHIFT)

/* given an ELT and a page index, find the swap slot */
#define UAO_SWHASH_ELT_PAGESLOT(ELT, PAGEIDX) \
	((ELT)->slots[(PAGEIDX) & (UAO_SWHASH_CLUSTER_SIZE - 1)])

/* given an ELT, return its pageidx base */
#define UAO_SWHASH_ELT_PAGEIDX_BASE(ELT) \
	((ELT)->tag << UAO_SWHASH_CLUSTER_SHIFT)

/*
 * the swhash hash function
 */

#define UAO_SWHASH_HASH(AOBJ, PAGEIDX) \
	(&(AOBJ)->u_swhash[(((PAGEIDX) >> UAO_SWHASH_CLUSTER_SHIFT) \
			    & (AOBJ)->u_swhashmask)])

/*
 * the swhash threshhold determines if we will use an array or a
 * hash table to store the list of allocated swap blocks.
 */

#define UAO_SWHASH_THRESHOLD (UAO_SWHASH_CLUSTER_SIZE * 4)
#define UAO_USES_SWHASH(AOBJ) \
	((AOBJ)->u_pages > UAO_SWHASH_THRESHOLD)	/* use hash? */

/*
 * the number of buckets in a swhash, with an upper bound
 */

#define UAO_SWHASH_MAXBUCKETS 256
#define UAO_SWHASH_BUCKETS(AOBJ) \
	(MIN((AOBJ)->u_pages >> UAO_SWHASH_CLUSTER_SHIFT, \
	     UAO_SWHASH_MAXBUCKETS))


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
 * uao_swhash_elt_pool: pool of uao_swhash_elt structures
 */

struct pool uao_swhash_elt_pool;

/*
 * uvm_aobj: the actual anon-backed uvm_object
 *
 * => the uvm_object is at the top of the structure, this allows
 *   (struct uvm_aobj *) == (struct uvm_object *)
 * => only one of u_swslots and u_swhash is used in any given aobj
 */

struct uvm_aobj {
	struct uvm_object u_obj; /* has: lock, pgops, memq, #pages, #refs */
	int u_pages;		 /* number of pages in entire object */
	int u_flags;		 /* the flags (see uvm_aobj.h) */
	int *u_swslots;		 /* array of offset->swapslot mappings */
				 /*
				  * hashtable of offset->swapslot mappings
				  * (u_swhash is an array of bucket heads)
				  */
	struct uao_swhash *u_swhash;
	u_long u_swhashmask;		/* mask for hashtable */
	LIST_ENTRY(uvm_aobj) u_list;	/* global list of aobjs */
};

/*
 * uvm_aobj_pool: pool of uvm_aobj structures
 */

struct pool uvm_aobj_pool;

/*
 * local functions
 */

static struct uao_swhash_elt *uao_find_swhash_elt
    __P((struct uvm_aobj *, int, boolean_t));

static void	uao_free __P((struct uvm_aobj *));
static int	uao_get __P((struct uvm_object *, voff_t, struct vm_page **,
		    int *, int, vm_prot_t, int, int));
static boolean_t uao_put __P((struct uvm_object *, voff_t, voff_t, int));
static boolean_t uao_pagein __P((struct uvm_aobj *, int, int));
static boolean_t uao_pagein_page __P((struct uvm_aobj *, int));

/*
 * aobj_pager
 *
 * note that some functions (e.g. put) are handled elsewhere
 */

struct uvm_pagerops aobj_pager = {
	NULL,			/* init */
	uao_reference,		/* reference */
	uao_detach,		/* detach */
	NULL,			/* fault */
	uao_get,		/* get */
	uao_put,		/* flush */
};

/*
 * uao_list: global list of active aobjs, locked by uao_list_lock
 */

static LIST_HEAD(aobjlist, uvm_aobj) uao_list;
static struct simplelock uao_list_lock;

/*
 * functions
 */

/*
 * hash table/array related functions
 */

/*
 * uao_find_swhash_elt: find (or create) a hash table entry for a page
 * offset.
 *
 * => the object should be locked by the caller
 */

static struct uao_swhash_elt *
uao_find_swhash_elt(aobj, pageidx, create)
	struct uvm_aobj *aobj;
	int pageidx;
	boolean_t create;
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
uao_find_swslot(uobj, pageidx)
	struct uvm_object *uobj;
	int pageidx;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uao_swhash_elt *elt;

	/*
	 * if noswap flag is set, then we never return a slot
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP)
		return(0);

	/*
	 * if hashing, look in hash table.
	 */

	if (UAO_USES_SWHASH(aobj)) {
		elt = uao_find_swhash_elt(aobj, pageidx, FALSE);
		if (elt)
			return(UAO_SWHASH_ELT_PAGESLOT(elt, pageidx));
		else
			return(0);
	}

	/*
	 * otherwise, look in the array
	 */

	return(aobj->u_swslots[pageidx]);
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
uao_set_swslot(uobj, pageidx, slot)
	struct uvm_object *uobj;
	int pageidx, slot;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uao_swhash_elt *elt;
	int oldslot;
	UVMHIST_FUNC("uao_set_swslot"); UVMHIST_CALLED(pdhist);
	UVMHIST_LOG(pdhist, "aobj %p pageidx %d slot %d",
	    aobj, pageidx, slot, 0);

	/*
	 * if noswap flag is set, then we can't set a non-zero slot.
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP) {
		if (slot == 0)
			return(0);

		printf("uao_set_swslot: uobj = %p\n", uobj);
		panic("uao_set_swslot: NOSWAP object");
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
	return (oldslot);
}

/*
 * end of hash/array functions
 */

/*
 * uao_free: free all resources held by an aobj, and then free the aobj
 *
 * => the aobj should be dead
 */

static void
uao_free(aobj)
	struct uvm_aobj *aobj;
{
	int swpgonlydelta = 0;

	simple_unlock(&aobj->u_obj.vmobjlock);
	if (UAO_USES_SWHASH(aobj)) {
		int i, hashbuckets = aobj->u_swhashmask + 1;

		/*
		 * free the swslots from each hash bucket,
		 * then the hash bucket, and finally the hash table itself.
		 */

		for (i = 0; i < hashbuckets; i++) {
			struct uao_swhash_elt *elt, *next;

			for (elt = LIST_FIRST(&aobj->u_swhash[i]);
			     elt != NULL;
			     elt = next) {
				int j;

				for (j = 0; j < UAO_SWHASH_CLUSTER_SIZE; j++) {
					int slot = elt->slots[j];

					if (slot == 0) {
						continue;
					}
					uvm_swap_free(slot, 1);
					swpgonlydelta++;
				}

				next = LIST_NEXT(elt, list);
				pool_put(&uao_swhash_elt_pool, elt);
			}
		}
		free(aobj->u_swhash, M_UVMAOBJ);
	} else {
		int i;

		/*
		 * free the array
		 */

		for (i = 0; i < aobj->u_pages; i++) {
			int slot = aobj->u_swslots[i];

			if (slot) {
				uvm_swap_free(slot, 1);
				swpgonlydelta++;
			}
		}
		free(aobj->u_swslots, M_UVMAOBJ);
	}

	/*
	 * finally free the aobj itself
	 */

	pool_put(&uvm_aobj_pool, aobj);

	/*
	 * adjust the counter of pages only in swap for all
	 * the swap slots we've freed.
	 */

	if (swpgonlydelta > 0) {
		simple_lock(&uvm.swap_data_lock);
		KASSERT(uvmexp.swpgonly >= swpgonlydelta);
		uvmexp.swpgonly -= swpgonlydelta;
		simple_unlock(&uvm.swap_data_lock);
	}
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
uao_create(size, flags)
	vsize_t size;
	int flags;
{
	static struct uvm_aobj kernel_object_store;
	static int kobj_alloced = 0;
	int pages = round_page(size) >> PAGE_SHIFT;
	struct uvm_aobj *aobj;

	/*
	 * malloc a new aobj unless we are asked for the kernel object
	 */

	if (flags & UAO_FLAG_KERNOBJ) {
		KASSERT(!kobj_alloced);
		aobj = &kernel_object_store;
		aobj->u_pages = pages;
		aobj->u_flags = UAO_FLAG_NOSWAP;
		aobj->u_obj.uo_refs = UVM_OBJ_KERN;
		kobj_alloced = UAO_FLAG_KERNOBJ;
	} else if (flags & UAO_FLAG_KERNSWAP) {
		KASSERT(kobj_alloced == UAO_FLAG_KERNOBJ);
		aobj = &kernel_object_store;
		kobj_alloced = UAO_FLAG_KERNSWAP;
	} else {
		aobj = pool_get(&uvm_aobj_pool, PR_WAITOK);
		aobj->u_pages = pages;
		aobj->u_flags = 0;
		aobj->u_obj.uo_refs = 1;
	}

	/*
 	 * allocate hash/array if necessary
 	 *
 	 * note: in the KERNSWAP case no need to worry about locking since
 	 * we are still booting we should be the only thread around.
 	 */

	if (flags == 0 || (flags & UAO_FLAG_KERNSWAP) != 0) {
		int mflags = (flags & UAO_FLAG_KERNSWAP) != 0 ?
		    M_NOWAIT : M_WAITOK;

		/* allocate hash table or array depending on object size */
		if (UAO_USES_SWHASH(aobj)) {
			aobj->u_swhash = hashinit(UAO_SWHASH_BUCKETS(aobj),
			    HASH_LIST, M_UVMAOBJ, mflags, &aobj->u_swhashmask);
			if (aobj->u_swhash == NULL)
				panic("uao_create: hashinit swhash failed");
		} else {
			aobj->u_swslots = malloc(pages * sizeof(int),
			    M_UVMAOBJ, mflags);
			if (aobj->u_swslots == NULL)
				panic("uao_create: malloc swslots failed");
			memset(aobj->u_swslots, 0, pages * sizeof(int));
		}

		if (flags) {
			aobj->u_flags &= ~UAO_FLAG_NOSWAP; /* clear noswap */
			return(&aobj->u_obj);
		}
	}

	/*
 	 * init aobj fields
 	 */

	simple_lock_init(&aobj->u_obj.vmobjlock);
	aobj->u_obj.pgops = &aobj_pager;
	TAILQ_INIT(&aobj->u_obj.memq);
	aobj->u_obj.uo_npages = 0;

	/*
 	 * now that aobj is ready, add it to the global list
 	 */

	simple_lock(&uao_list_lock);
	LIST_INSERT_HEAD(&uao_list, aobj, u_list);
	simple_unlock(&uao_list_lock);
	return(&aobj->u_obj);
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
	uao_initialized = TRUE;
	LIST_INIT(&uao_list);
	simple_lock_init(&uao_list_lock);

	/*
	 * NOTE: Pages fror this pool must not come from a pageable
	 * kernel map!
	 */

	pool_init(&uao_swhash_elt_pool, sizeof(struct uao_swhash_elt),
	    0, 0, 0, "uaoeltpl", NULL);
	pool_init(&uvm_aobj_pool, sizeof(struct uvm_aobj), 0, 0, 0,
	    "aobjpl", &pool_allocator_nointr);
}

/*
 * uao_reference: add a ref to an aobj
 *
 * => aobj must be unlocked
 * => just lock it and call the locked version
 */

void
uao_reference(uobj)
	struct uvm_object *uobj;
{
	simple_lock(&uobj->vmobjlock);
	uao_reference_locked(uobj);
	simple_unlock(&uobj->vmobjlock);
}

/*
 * uao_reference_locked: add a ref to an aobj that is already locked
 *
 * => aobj must be locked
 * this needs to be separate from the normal routine
 * since sometimes we need to add a reference to an aobj when
 * it's already locked.
 */

void
uao_reference_locked(uobj)
	struct uvm_object *uobj;
{
	UVMHIST_FUNC("uao_reference"); UVMHIST_CALLED(maphist);

	/*
 	 * kernel_object already has plenty of references, leave it alone.
 	 */

	if (UVM_OBJ_IS_KERN_OBJECT(uobj))
		return;

	uobj->uo_refs++;
	UVMHIST_LOG(maphist, "<- done (uobj=0x%x, ref = %d)",
		    uobj, uobj->uo_refs,0,0);
}

/*
 * uao_detach: drop a reference to an aobj
 *
 * => aobj must be unlocked
 * => just lock it and call the locked version
 */

void
uao_detach(uobj)
	struct uvm_object *uobj;
{
	simple_lock(&uobj->vmobjlock);
	uao_detach_locked(uobj);
}

/*
 * uao_detach_locked: drop a reference to an aobj
 *
 * => aobj must be locked, and is unlocked (or freed) upon return.
 * this needs to be separate from the normal routine
 * since sometimes we need to detach from an aobj when
 * it's already locked.
 */

void
uao_detach_locked(uobj)
	struct uvm_object *uobj;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct vm_page *pg;
	UVMHIST_FUNC("uao_detach"); UVMHIST_CALLED(maphist);

	/*
 	 * detaching from kernel_object is a noop.
 	 */

	if (UVM_OBJ_IS_KERN_OBJECT(uobj)) {
		simple_unlock(&uobj->vmobjlock);
		return;
	}

	UVMHIST_LOG(maphist,"  (uobj=0x%x)  ref=%d", uobj,uobj->uo_refs,0,0);
	uobj->uo_refs--;
	if (uobj->uo_refs) {
		simple_unlock(&uobj->vmobjlock);
		UVMHIST_LOG(maphist, "<- done (rc>0)", 0,0,0,0);
		return;
	}

	/*
 	 * remove the aobj from the global list.
 	 */

	simple_lock(&uao_list_lock);
	LIST_REMOVE(aobj, u_list);
	simple_unlock(&uao_list_lock);

	/*
 	 * free all the pages left in the aobj.  for each page,
	 * when the page is no longer busy (and thus after any disk i/o that
	 * it's involved in is complete), release any swap resources and
	 * free the page itself.
 	 */

	uvm_lock_pageq();
	while ((pg = TAILQ_FIRST(&uobj->memq)) != NULL) {
		pmap_page_protect(pg, VM_PROT_NONE);
		if (pg->flags & PG_BUSY) {
			pg->flags |= PG_WANTED;
			uvm_unlock_pageq();
			UVM_UNLOCK_AND_WAIT(pg, &uobj->vmobjlock, FALSE,
			    "uao_det", 0);
			simple_lock(&uobj->vmobjlock);
			uvm_lock_pageq();
			continue;
		}
		uao_dropswap(&aobj->u_obj, pg->offset >> PAGE_SHIFT);
		uvm_pagefree(pg);
	}
	uvm_unlock_pageq();

	/*
 	 * finally, free the aobj itself.
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
 * => NOTE: we rely on the fact that the object's memq is a TAILQ and
 *	that new pages are inserted on the tail end of the list.  thus,
 *	we can make a complete pass through the object in one go by starting
 *	at the head and working towards the tail (new pages are put in
 *	front of us).
 * => NOTE: we are allowed to lock the page queues, so the caller
 *	must not be holding the lock on them [e.g. pagedaemon had
 *	better not call us with the queues locked]
 * => we return TRUE unless we encountered some sort of I/O error
 *	XXXJRT currently never happens, as we never directly initiate
 *	XXXJRT I/O
 *
 * note on page traversal:
 *	we can traverse the pages in an object either by going down the
 *	linked list in "uobj->memq", or we can go over the address range
 *	by page doing hash table lookups for each address.  depending
 *	on how many pages are in the object it may be cheaper to do one
 *	or the other.  we set "by_list" to true if we are using memq.
 *	if the cost of a hash lookup was equal to the cost of the list
 *	traversal we could compare the number of pages in the start->stop
 *	range to the total number of pages in the object.  however, it
 *	seems that a hash table lookup is more expensive than the linked
 *	list traversal, so we multiply the number of pages in the
 *	start->stop range by a penalty which we define below.
 */

int
uao_put(uobj, start, stop, flags)
	struct uvm_object *uobj;
	voff_t start, stop;
	int flags;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct vm_page *pg, *nextpg;
	boolean_t by_list;
	voff_t curoff;
	UVMHIST_FUNC("uao_put"); UVMHIST_CALLED(maphist);

	curoff = 0;
	if (flags & PGO_ALLPAGES) {
		start = 0;
		stop = aobj->u_pages << PAGE_SHIFT;
		by_list = TRUE;		/* always go by the list */
	} else {
		start = trunc_page(start);
		stop = round_page(stop);
		if (stop > (aobj->u_pages << PAGE_SHIFT)) {
			printf("uao_flush: strange, got an out of range "
			    "flush (fixed)\n");
			stop = aobj->u_pages << PAGE_SHIFT;
		}
		by_list = (uobj->uo_npages <=
		    ((stop - start) >> PAGE_SHIFT) * UVM_PAGE_HASH_PENALTY);
	}
	UVMHIST_LOG(maphist,
	    " flush start=0x%lx, stop=0x%x, by_list=%d, flags=0x%x",
	    start, stop, by_list, flags);

	/*
	 * Don't need to do any work here if we're not freeing
	 * or deactivating pages.
	 */

	if ((flags & (PGO_DEACTIVATE|PGO_FREE)) == 0) {
		simple_unlock(&uobj->vmobjlock);
		return 0;
	}

	/*
	 * now do it.  note: we must update nextpg in the body of loop or we
	 * will get stuck.  we need to use nextpg because we may free "pg"
	 * before doing the next loop.
	 */

	if (by_list) {
		pg = TAILQ_FIRST(&uobj->memq);
	} else {
		curoff = start;
		pg = uvm_pagelookup(uobj, curoff);
	}

	nextpg = NULL;
	uvm_lock_pageq();

	/* locked: both page queues and uobj */
	for ( ; (by_list && pg != NULL) ||
	    (!by_list && curoff < stop) ; pg = nextpg) {
		if (by_list) {
			nextpg = TAILQ_NEXT(pg, listq);
			if (pg->offset < start || pg->offset >= stop)
				continue;
		} else {
			curoff += PAGE_SIZE;
			if (curoff < stop)
				nextpg = uvm_pagelookup(uobj, curoff);
			if (pg == NULL)
				continue;
		}
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
			/* skip the page if it's loaned or wired */
			if (pg->loan_count != 0 || pg->wire_count != 0)
				continue;

			/* ...and deactivate the page. */
			pmap_clear_reference(pg);
			uvm_pagedeactivate(pg);
			continue;

		case PGO_FREE:

			/*
			 * If there are multiple references to
			 * the object, just deactivate the page.
			 */

			if (uobj->uo_refs > 1)
				goto deactivate_it;

			/* XXX skip the page if it's loaned or wired */
			if (pg->loan_count != 0 || pg->wire_count != 0)
				continue;

			/*
			 * wait if the page is busy, then free the swap slot
			 * and the page.
			 */

			pmap_page_protect(pg, VM_PROT_NONE);
			while (pg->flags & PG_BUSY) {
				pg->flags |= PG_WANTED;
				uvm_unlock_pageq();
				UVM_UNLOCK_AND_WAIT(pg, &uobj->vmobjlock, 0,
				    "uao_put", 0);
				simple_lock(&uobj->vmobjlock);
				uvm_lock_pageq();
			}
			uao_dropswap(uobj, pg->offset >> PAGE_SHIFT);
			uvm_pagefree(pg);
			continue;
		}
	}
	uvm_unlock_pageq();
	simple_unlock(&uobj->vmobjlock);
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
 * cases 1 and 2 can be handled with PGO_LOCKED, case 3 cannot.
 * so, if the "center" page hits case 3 (or any page, with PGO_ALLPAGES),
 * then we will need to return EBUSY.
 *
 * => prefer map unlocked (not required)
 * => object must be locked!  we will _unlock_ it before starting any I/O.
 * => flags: PGO_ALLPAGES: get all of the pages
 *           PGO_LOCKED: fault data structures are locked
 * => NOTE: offset is the offset of pps[0], _NOT_ pps[centeridx]
 * => NOTE: caller must check for released pages!!
 */

static int
uao_get(uobj, offset, pps, npagesp, centeridx, access_type, advice, flags)
	struct uvm_object *uobj;
	voff_t offset;
	struct vm_page **pps;
	int *npagesp;
	int centeridx, advice, flags;
	vm_prot_t access_type;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	voff_t current_offset;
	struct vm_page *ptmp;
	int lcv, gotpages, maxpages, swslot, error, pageidx;
	boolean_t done;
	UVMHIST_FUNC("uao_get"); UVMHIST_CALLED(pdhist);

	UVMHIST_LOG(pdhist, "aobj=%p offset=%d, flags=%d",
		    aobj, offset, flags,0);

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

		done = TRUE;	/* be optimistic */
		gotpages = 0;	/* # of pages we got so far */
		for (lcv = 0, current_offset = offset ; lcv < maxpages ;
		    lcv++, current_offset += PAGE_SIZE) {
			/* do we care about this page?  if not, skip it */
			if (pps[lcv] == PGO_DONTCARE)
				continue;
			ptmp = uvm_pagelookup(uobj, current_offset);

			/*
 			 * if page is new, attempt to allocate the page,
			 * zero-fill'd.
 			 */

			if (ptmp == NULL && uao_find_swslot(&aobj->u_obj,
			    current_offset >> PAGE_SHIFT) == 0) {
				ptmp = uvm_pagealloc(uobj, current_offset,
				    NULL, UVM_PGA_ZERO);
				if (ptmp) {
					/* new page */
					ptmp->flags &= ~(PG_FAKE);
					ptmp->pqflags |= PQ_AOBJ;
					goto gotpage;
				}
			}

			/*
			 * to be useful must get a non-busy page
			 */

			if (ptmp == NULL || (ptmp->flags & PG_BUSY) != 0) {
				if (lcv == centeridx ||
				    (flags & PGO_ALLPAGES) != 0)
					/* need to do a wait or I/O! */
					done = FALSE;
					continue;
			}

			/*
			 * useful page: busy/lock it and plug it in our
			 * result array
			 */

			/* caller must un-busy this page */
			ptmp->flags |= PG_BUSY;
			UVM_PAGE_OWN(ptmp, "uao_get1");
gotpage:
			pps[lcv] = ptmp;
			gotpages++;
		}

		/*
 		 * step 1b: now we've either done everything needed or we
		 * to unlock and do some waiting or I/O.
 		 */

		UVMHIST_LOG(pdhist, "<- done (done=%d)", done, 0,0,0);
		*npagesp = gotpages;
		if (done)
			return 0;
		else
			return EBUSY;
	}

	/*
 	 * step 2: get non-resident or busy pages.
 	 * object is locked.   data structures are unlocked.
 	 */

	for (lcv = 0, current_offset = offset ; lcv < maxpages ;
	    lcv++, current_offset += PAGE_SIZE) {

		/*
		 * - skip over pages we've already gotten or don't want
		 * - skip over pages we don't _have_ to get
		 */

		if (pps[lcv] != NULL ||
		    (lcv != centeridx && (flags & PGO_ALLPAGES) == 0))
			continue;

		pageidx = current_offset >> PAGE_SHIFT;

		/*
 		 * we have yet to locate the current page (pps[lcv]).   we
		 * first look for a page that is already at the current offset.
		 * if we find a page, we check to see if it is busy or
		 * released.  if that is the case, then we sleep on the page
		 * until it is no longer busy or released and repeat the lookup.
		 * if the page we found is neither busy nor released, then we
		 * busy it (so we own it) and plug it into pps[lcv].   this
		 * 'break's the following while loop and indicates we are
		 * ready to move on to the next page in the "lcv" loop above.
 		 *
 		 * if we exit the while loop with pps[lcv] still set to NULL,
		 * then it means that we allocated a new busy/fake/clean page
		 * ptmp in the object and we need to do I/O to fill in the data.
 		 */

		/* top of "pps" while loop */
		while (pps[lcv] == NULL) {
			/* look for a resident page */
			ptmp = uvm_pagelookup(uobj, current_offset);

			/* not resident?   allocate one now (if we can) */
			if (ptmp == NULL) {

				ptmp = uvm_pagealloc(uobj, current_offset,
				    NULL, 0);

				/* out of RAM? */
				if (ptmp == NULL) {
					simple_unlock(&uobj->vmobjlock);
					UVMHIST_LOG(pdhist,
					    "sleeping, ptmp == NULL\n",0,0,0,0);
					uvm_wait("uao_getpage");
					simple_lock(&uobj->vmobjlock);
					continue;
				}

				/*
				 * safe with PQ's unlocked: because we just
				 * alloc'd the page
				 */

				ptmp->pqflags |= PQ_AOBJ;

				/*
				 * got new page ready for I/O.  break pps while
				 * loop.  pps[lcv] is still NULL.
				 */

				break;
			}

			/* page is there, see if we need to wait on it */
			if ((ptmp->flags & PG_BUSY) != 0) {
				ptmp->flags |= PG_WANTED;
				UVMHIST_LOG(pdhist,
				    "sleeping, ptmp->flags 0x%x\n",
				    ptmp->flags,0,0,0);
				UVM_UNLOCK_AND_WAIT(ptmp, &uobj->vmobjlock,
				    FALSE, "uao_get", 0);
				simple_lock(&uobj->vmobjlock);
				continue;
			}

			/*
 			 * if we get here then the page has become resident and
			 * unbusy between steps 1 and 2.  we busy it now (so we
			 * own it) and set pps[lcv] (so that we exit the while
			 * loop).
 			 */

			/* we own it, caller must un-busy */
			ptmp->flags |= PG_BUSY;
			UVM_PAGE_OWN(ptmp, "uao_get2");
			pps[lcv] = ptmp;
		}

		/*
 		 * if we own the valid page at the correct offset, pps[lcv] will
 		 * point to it.   nothing more to do except go to the next page.
 		 */

		if (pps[lcv])
			continue;			/* next lcv */

		/*
 		 * we have a "fake/busy/clean" page that we just allocated.
 		 * do the needed "i/o", either reading from swap or zeroing.
 		 */

		swslot = uao_find_swslot(&aobj->u_obj, pageidx);

		/*
 		 * just zero the page if there's nothing in swap.
 		 */

		if (swslot == 0) {

			/*
			 * page hasn't existed before, just zero it.
			 */

			uvm_pagezero(ptmp);
		} else {
			UVMHIST_LOG(pdhist, "pagein from swslot %d",
			     swslot, 0,0,0);

			/*
			 * page in the swapped-out page.
			 * unlock object for i/o, relock when done.
			 */

			simple_unlock(&uobj->vmobjlock);
			error = uvm_swap_get(ptmp, swslot, PGO_SYNCIO);
			simple_lock(&uobj->vmobjlock);

			/*
			 * I/O done.  check for errors.
			 */

			if (error != 0) {
				UVMHIST_LOG(pdhist, "<- done (error=%d)",
				    error,0,0,0);
				if (ptmp->flags & PG_WANTED)
					wakeup(ptmp);

				/*
				 * remove the swap slot from the aobj
				 * and mark the aobj as having no real slot.
				 * don't free the swap slot, thus preventing
				 * it from being used again.
				 */

				swslot = uao_set_swslot(&aobj->u_obj, pageidx,
							SWSLOT_BAD);
				if (swslot != -1) {
					uvm_swap_markbad(swslot, 1);
				}

				uvm_lock_pageq();
				uvm_pagefree(ptmp);
				uvm_unlock_pageq();
				simple_unlock(&uobj->vmobjlock);
				return error;
			}
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

		ptmp->flags &= ~PG_FAKE;
		pps[lcv] = ptmp;
	}

	/*
 	 * finally, unlock object and return.
 	 */

	simple_unlock(&uobj->vmobjlock);
	UVMHIST_LOG(pdhist, "<- done (OK)",0,0,0,0);
	return 0;
}

/*
 * uao_dropswap:  release any swap resources from this aobj page.
 *
 * => aobj must be locked or have a reference count of 0.
 */

void
uao_dropswap(uobj, pageidx)
	struct uvm_object *uobj;
	int pageidx;
{
	int slot;

	slot = uao_set_swslot(uobj, pageidx, 0);
	if (slot) {
		uvm_swap_free(slot, 1);
	}
}

/*
 * page in every page in every aobj that is paged-out to a range of swslots.
 *
 * => nothing should be locked.
 * => returns TRUE if pagein was aborted due to lack of memory.
 */

boolean_t
uao_swap_off(startslot, endslot)
	int startslot, endslot;
{
	struct uvm_aobj *aobj, *nextaobj;
	boolean_t rv;

	/*
	 * walk the list of all aobjs.
	 */

restart:
	simple_lock(&uao_list_lock);
	for (aobj = LIST_FIRST(&uao_list);
	     aobj != NULL;
	     aobj = nextaobj) {

		/*
		 * try to get the object lock, start all over if we fail.
		 * most of the time we'll get the aobj lock,
		 * so this should be a rare case.
		 */

		if (!simple_lock_try(&aobj->u_obj.vmobjlock)) {
			simple_unlock(&uao_list_lock);
			goto restart;
		}

		/*
		 * add a ref to the aobj so it doesn't disappear
		 * while we're working.
		 */

		uao_reference_locked(&aobj->u_obj);

		/*
		 * now it's safe to unlock the uao list.
		 */

		simple_unlock(&uao_list_lock);

		/*
		 * page in any pages in the swslot range.
		 * if there's an error, abort and return the error.
		 */

		rv = uao_pagein(aobj, startslot, endslot);
		if (rv) {
			uao_detach_locked(&aobj->u_obj);
			return rv;
		}

		/*
		 * we're done with this aobj.
		 * relock the list and drop our ref on the aobj.
		 */

		simple_lock(&uao_list_lock);
		nextaobj = LIST_NEXT(aobj, u_list);
		uao_detach_locked(&aobj->u_obj);
	}

	/*
	 * done with traversal, unlock the list
	 */
	simple_unlock(&uao_list_lock);
	return FALSE;
}


/*
 * page in any pages from aobj in the given range.
 *
 * => aobj must be locked and is returned locked.
 * => returns TRUE if pagein was aborted due to lack of memory.
 */
static boolean_t
uao_pagein(aobj, startslot, endslot)
	struct uvm_aobj *aobj;
	int startslot, endslot;
{
	boolean_t rv;

	if (UAO_USES_SWHASH(aobj)) {
		struct uao_swhash_elt *elt;
		int bucket;

restart:
		for (bucket = aobj->u_swhashmask; bucket >= 0; bucket--) {
			for (elt = LIST_FIRST(&aobj->u_swhash[bucket]);
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

	return FALSE;
}

/*
 * page in a page from an aobj.  used for swap_off.
 * returns TRUE if pagein was aborted due to lack of memory.
 *
 * => aobj must be locked and is returned locked.
 */

static boolean_t
uao_pagein_page(aobj, pageidx)
	struct uvm_aobj *aobj;
	int pageidx;
{
	struct vm_page *pg;
	int rv, slot, npages;

	pg = NULL;
	npages = 1;
	/* locked: aobj */
	rv = uao_get(&aobj->u_obj, pageidx << PAGE_SHIFT,
		     &pg, &npages, 0, VM_PROT_READ|VM_PROT_WRITE, 0, 0);
	/* unlocked: aobj */

	/*
	 * relock and finish up.
	 */

	simple_lock(&aobj->u_obj.vmobjlock);
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

		return FALSE;
	}

	/*
	 * ok, we've got the page now.
	 * mark it as dirty, clear its swslot and un-busy it.
	 */

	slot = uao_set_swslot(&aobj->u_obj, pageidx, 0);
	uvm_swap_free(slot, 1);
	pg->flags &= ~(PG_BUSY|PG_CLEAN|PG_FAKE);
	UVM_PAGE_OWN(pg, NULL);

	/*
	 * deactivate the page (to make sure it's on a page queue).
	 */

	uvm_lock_pageq();
	uvm_pagedeactivate(pg);
	uvm_unlock_pageq();
	return FALSE;
}
