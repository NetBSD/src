/*	$NetBSD: uvm_fault.c,v 1.113 2006/10/03 18:26:03 christos Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * from: Id: uvm_fault.c,v 1.1.2.23 1998/02/06 05:29:05 chs Exp
 */

/*
 * uvm_fault.c: fault handler
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_fault.c,v 1.113 2006/10/03 18:26:03 christos Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/vnode.h>

#include <uvm/uvm.h>

/*
 *
 * a word on page faults:
 *
 * types of page faults we handle:
 *
 * CASE 1: upper layer faults                   CASE 2: lower layer faults
 *
 *    CASE 1A         CASE 1B                  CASE 2A        CASE 2B
 *    read/write1     write>1                  read/write   +-cow_write/zero
 *         |             |                         |        |
 *      +--|--+       +--|--+     +-----+       +  |  +     | +-----+
 * amap |  V  |       |  ----------->new|          |        | |  ^  |
 *      +-----+       +-----+     +-----+       +  |  +     | +--|--+
 *                                                 |        |    |
 *      +-----+       +-----+                   +--|--+     | +--|--+
 * uobj | d/c |       | d/c |                   |  V  |     +----|  |
 *      +-----+       +-----+                   +-----+       +-----+
 *
 * d/c = don't care
 *
 *   case [0]: layerless fault
 *	no amap or uobj is present.   this is an error.
 *
 *   case [1]: upper layer fault [anon active]
 *     1A: [read] or [write with anon->an_ref == 1]
 *		I/O takes place in top level anon and uobj is not touched.
 *     1B: [write with anon->an_ref > 1]
 *		new anon is alloc'd and data is copied off ["COW"]
 *
 *   case [2]: lower layer fault [uobj]
 *     2A: [read on non-NULL uobj] or [write to non-copy_on_write area]
 *		I/O takes place directly in object.
 *     2B: [write to copy_on_write] or [read on NULL uobj]
 *		data is "promoted" from uobj to a new anon.
 *		if uobj is null, then we zero fill.
 *
 * we follow the standard UVM locking protocol ordering:
 *
 * MAPS => AMAP => UOBJ => ANON => PAGE QUEUES (PQ)
 * we hold a PG_BUSY page if we unlock for I/O
 *
 *
 * the code is structured as follows:
 *
 *     - init the "IN" params in the ufi structure
 *   ReFault:
 *     - do lookups [locks maps], check protection, handle needs_copy
 *     - check for case 0 fault (error)
 *     - establish "range" of fault
 *     - if we have an amap lock it and extract the anons
 *     - if sequential advice deactivate pages behind us
 *     - at the same time check pmap for unmapped areas and anon for pages
 *	 that we could map in (and do map it if found)
 *     - check object for resident pages that we could map in
 *     - if (case 2) goto Case2
 *     - >>> handle case 1
 *           - ensure source anon is resident in RAM
 *           - if case 1B alloc new anon and copy from source
 *           - map the correct page in
 *   Case2:
 *     - >>> handle case 2
 *           - ensure source page is resident (if uobj)
 *           - if case 2B alloc new anon and copy from source (could be zero
 *		fill if uobj == NULL)
 *           - map the correct page in
 *     - done!
 *
 * note on paging:
 *   if we have to do I/O we place a PG_BUSY page in the correct object,
 * unlock everything, and do the I/O.   when I/O is done we must reverify
 * the state of the world before assuming that our data structures are
 * valid.   [because mappings could change while the map is unlocked]
 *
 *  alternative 1: unbusy the page in question and restart the page fault
 *    from the top (ReFault).   this is easy but does not take advantage
 *    of the information that we already have from our previous lookup,
 *    although it is possible that the "hints" in the vm_map will help here.
 *
 * alternative 2: the system already keeps track of a "version" number of
 *    a map.   [i.e. every time you write-lock a map (e.g. to change a
 *    mapping) you bump the version number up by one...]   so, we can save
 *    the version number of the map before we release the lock and start I/O.
 *    then when I/O is done we can relock and check the version numbers
 *    to see if anything changed.    this might save us some over 1 because
 *    we don't have to unbusy the page and may be less compares(?).
 *
 * alternative 3: put in backpointers or a way to "hold" part of a map
 *    in place while I/O is in progress.   this could be complex to
 *    implement (especially with structures like amap that can be referenced
 *    by multiple map entries, and figuring out what should wait could be
 *    complex as well...).
 *
 * given that we are not currently multiprocessor or multithreaded we might
 * as well choose alternative 2 now.   maybe alternative 3 would be useful
 * in the future.    XXX keep in mind for future consideration//rechecking.
 */

/*
 * local data structures
 */

struct uvm_advice {
	int advice;
	int nback;
	int nforw;
};

/*
 * page range array:
 * note: index in array must match "advice" value
 * XXX: borrowed numbers from freebsd.   do they work well for us?
 */

static const struct uvm_advice uvmadvice[] = {
	{ MADV_NORMAL, 3, 4 },
	{ MADV_RANDOM, 0, 0 },
	{ MADV_SEQUENTIAL, 8, 7},
};

#define UVM_MAXRANGE 16	/* must be MAX() of nback+nforw+1 */

/*
 * private prototypes
 */

/*
 * inline functions
 */

/*
 * uvmfault_anonflush: try and deactivate pages in specified anons
 *
 * => does not have to deactivate page if it is busy
 */

static inline void
uvmfault_anonflush(struct vm_anon **anons, int n)
{
	int lcv;
	struct vm_page *pg;

	for (lcv = 0 ; lcv < n ; lcv++) {
		if (anons[lcv] == NULL)
			continue;
		simple_lock(&anons[lcv]->an_lock);
		pg = anons[lcv]->an_page;
		if (pg && (pg->flags & PG_BUSY) == 0 && pg->loan_count == 0) {
			uvm_lock_pageq();
			if (pg->wire_count == 0) {
				pmap_clear_reference(pg);
				uvm_pagedeactivate(pg);
			}
			uvm_unlock_pageq();
		}
		simple_unlock(&anons[lcv]->an_lock);
	}
}

/*
 * normal functions
 */

/*
 * uvmfault_amapcopy: clear "needs_copy" in a map.
 *
 * => called with VM data structures unlocked (usually, see below)
 * => we get a write lock on the maps and clear needs_copy for a VA
 * => if we are out of RAM we sleep (waiting for more)
 */

static void
uvmfault_amapcopy(struct uvm_faultinfo *ufi)
{
	for (;;) {

		/*
		 * no mapping?  give up.
		 */

		if (uvmfault_lookup(ufi, TRUE) == FALSE)
			return;

		/*
		 * copy if needed.
		 */

		if (UVM_ET_ISNEEDSCOPY(ufi->entry))
			amap_copy(ufi->map, ufi->entry, AMAP_COPY_NOWAIT,
				ufi->orig_rvaddr, ufi->orig_rvaddr + 1);

		/*
		 * didn't work?  must be out of RAM.   unlock and sleep.
		 */

		if (UVM_ET_ISNEEDSCOPY(ufi->entry)) {
			uvmfault_unlockmaps(ufi, TRUE);
			uvm_wait("fltamapcopy");
			continue;
		}

		/*
		 * got it!   unlock and return.
		 */

		uvmfault_unlockmaps(ufi, TRUE);
		return;
	}
	/*NOTREACHED*/
}

/*
 * uvmfault_anonget: get data in an anon into a non-busy, non-released
 * page in that anon.
 *
 * => maps, amap, and anon locked by caller.
 * => if we fail (result != 0) we unlock everything.
 * => if we are successful, we return with everything still locked.
 * => we don't move the page on the queues [gets moved later]
 * => if we allocate a new page [we_own], it gets put on the queues.
 *    either way, the result is that the page is on the queues at return time
 * => for pages which are on loan from a uvm_object (and thus are not
 *    owned by the anon): if successful, we return with the owning object
 *    locked.   the caller must unlock this object when it unlocks everything
 *    else.
 */

int
uvmfault_anonget(struct uvm_faultinfo *ufi, struct vm_amap *amap,
    struct vm_anon *anon)
{
	boolean_t we_own;	/* we own anon's page? */
	boolean_t locked;	/* did we relock? */
	struct vm_page *pg;
	int error;
	UVMHIST_FUNC("uvmfault_anonget"); UVMHIST_CALLED(maphist);

	LOCK_ASSERT(simple_lock_held(&anon->an_lock));

	error = 0;
	uvmexp.fltanget++;
        /* bump rusage counters */
	if (anon->an_page)
		curproc->p_stats->p_ru.ru_minflt++;
	else
		curproc->p_stats->p_ru.ru_majflt++;

	/*
	 * loop until we get it, or fail.
	 */

	for (;;) {
		we_own = FALSE;		/* TRUE if we set PG_BUSY on a page */
		pg = anon->an_page;

		/*
		 * if there is a resident page and it is loaned, then anon
		 * may not own it.   call out to uvm_anon_lockpage() to ensure
		 * the real owner of the page has been identified and locked.
		 */

		if (pg && pg->loan_count)
			pg = uvm_anon_lockloanpg(anon);

		/*
		 * page there?   make sure it is not busy/released.
		 */

		if (pg) {

			/*
			 * at this point, if the page has a uobject [meaning
			 * we have it on loan], then that uobject is locked
			 * by us!   if the page is busy, we drop all the
			 * locks (including uobject) and try again.
			 */

			if ((pg->flags & PG_BUSY) == 0) {
				UVMHIST_LOG(maphist, "<- OK",0,0,0,0);
				return (0);
			}
			pg->flags |= PG_WANTED;
			uvmexp.fltpgwait++;

			/*
			 * the last unlock must be an atomic unlock+wait on
			 * the owner of page
			 */

			if (pg->uobject) {	/* owner is uobject ? */
				uvmfault_unlockall(ufi, amap, NULL, anon);
				UVMHIST_LOG(maphist, " unlock+wait on uobj",0,
				    0,0,0);
				UVM_UNLOCK_AND_WAIT(pg,
				    &pg->uobject->vmobjlock,
				    FALSE, "anonget1",0);
			} else {
				/* anon owns page */
				uvmfault_unlockall(ufi, amap, NULL, NULL);
				UVMHIST_LOG(maphist, " unlock+wait on anon",0,
				    0,0,0);
				UVM_UNLOCK_AND_WAIT(pg,&anon->an_lock,0,
				    "anonget2",0);
			}
		} else {
#if defined(VMSWAP)

			/*
			 * no page, we must try and bring it in.
			 */

			pg = uvm_pagealloc(NULL, 0, anon, 0);
			if (pg == NULL) {		/* out of RAM.  */
				uvmfault_unlockall(ufi, amap, NULL, anon);
				uvmexp.fltnoram++;
				UVMHIST_LOG(maphist, "  noram -- UVM_WAIT",0,
				    0,0,0);
				if (!uvm_reclaimable()) {
					return ENOMEM;
				}
				uvm_wait("flt_noram1");
			} else {
				/* we set the PG_BUSY bit */
				we_own = TRUE;
				uvmfault_unlockall(ufi, amap, NULL, anon);

				/*
				 * we are passing a PG_BUSY+PG_FAKE+PG_CLEAN
				 * page into the uvm_swap_get function with
				 * all data structures unlocked.  note that
				 * it is ok to read an_swslot here because
				 * we hold PG_BUSY on the page.
				 */
				uvmexp.pageins++;
				error = uvm_swap_get(pg, anon->an_swslot,
				    PGO_SYNCIO);

				/*
				 * we clean up after the i/o below in the
				 * "we_own" case
				 */
			}
#else /* defined(VMSWAP) */
			panic("%s: no page", __func__);
#endif /* defined(VMSWAP) */
		}

		/*
		 * now relock and try again
		 */

		locked = uvmfault_relock(ufi);
		if (locked && amap != NULL) {
			amap_lock(amap);
		}
		if (locked || we_own)
			simple_lock(&anon->an_lock);

		/*
		 * if we own the page (i.e. we set PG_BUSY), then we need
		 * to clean up after the I/O. there are three cases to
		 * consider:
		 *   [1] page released during I/O: free anon and ReFault.
		 *   [2] I/O not OK.   free the page and cause the fault
		 *       to fail.
		 *   [3] I/O OK!   activate the page and sync with the
		 *       non-we_own case (i.e. drop anon lock if not locked).
		 */

		if (we_own) {
#if defined(VMSWAP)
			if (pg->flags & PG_WANTED) {
				wakeup(pg);
			}
			if (error) {

				/*
				 * remove the swap slot from the anon
				 * and mark the anon as having no real slot.
				 * don't free the swap slot, thus preventing
				 * it from being used again.
				 */

				if (anon->an_swslot > 0)
					uvm_swap_markbad(anon->an_swslot, 1);
				anon->an_swslot = SWSLOT_BAD;

				if ((pg->flags & PG_RELEASED) != 0)
					goto released;

				/*
				 * note: page was never !PG_BUSY, so it
				 * can't be mapped and thus no need to
				 * pmap_page_protect it...
				 */

				uvm_lock_pageq();
				uvm_pagefree(pg);
				uvm_unlock_pageq();

				if (locked)
					uvmfault_unlockall(ufi, amap, NULL,
					    anon);
				else
					simple_unlock(&anon->an_lock);
				UVMHIST_LOG(maphist, "<- ERROR", 0,0,0,0);
				return error;
			}

			if ((pg->flags & PG_RELEASED) != 0) {
released:
				KASSERT(anon->an_ref == 0);

				/*
				 * released while we unlocked amap.
				 */

				if (locked)
					uvmfault_unlockall(ufi, amap, NULL,
					    NULL);

				uvm_anon_release(anon);

				if (error) {
					UVMHIST_LOG(maphist,
					    "<- ERROR/RELEASED", 0,0,0,0);
					return error;
				}

				UVMHIST_LOG(maphist, "<- RELEASED", 0,0,0,0);
				return ERESTART;
			}

			/*
			 * we've successfully read the page, activate it.
			 */

			uvm_lock_pageq();
			uvm_pageactivate(pg);
			uvm_unlock_pageq();
			pg->flags &= ~(PG_WANTED|PG_BUSY|PG_FAKE);
			UVM_PAGE_OWN(pg, NULL);
			if (!locked)
				simple_unlock(&anon->an_lock);
#else /* defined(VMSWAP) */
			panic("%s: we_own", __func__);
#endif /* defined(VMSWAP) */
		}

		/*
		 * we were not able to relock.   restart fault.
		 */

		if (!locked) {
			UVMHIST_LOG(maphist, "<- REFAULT", 0,0,0,0);
			return (ERESTART);
		}

		/*
		 * verify no one has touched the amap and moved the anon on us.
		 */

		if (ufi != NULL &&
		    amap_lookup(&ufi->entry->aref,
				ufi->orig_rvaddr - ufi->entry->start) != anon) {

			uvmfault_unlockall(ufi, amap, NULL, anon);
			UVMHIST_LOG(maphist, "<- REFAULT", 0,0,0,0);
			return (ERESTART);
		}

		/*
		 * try it again!
		 */

		uvmexp.fltanretry++;
		continue;
	}
	/*NOTREACHED*/
}

/*
 * uvmfault_promote: promote data to a new anon.  used for 1B and 2B.
 *
 *	1. allocate an anon and a page.
 *	2. fill its contents.
 *	3. put it into amap.
 *
 * => if we fail (result != 0) we unlock everything.
 * => on success, return a new locked anon via 'nanon'.
 *    (*nanon)->an_page will be a resident, locked, dirty page.
 */

static int
uvmfault_promote(struct uvm_faultinfo *ufi,
    struct vm_anon *oanon,
    struct vm_page *uobjpage,
    struct vm_anon **nanon, /* OUT: allocated anon */
    struct vm_anon **spare)
{
	struct vm_amap *amap = ufi->entry->aref.ar_amap;
	struct uvm_object *uobj;
	struct vm_anon *anon;
	struct vm_page *pg;
	struct vm_page *opg;
	int error;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	if (oanon) {
		/* anon COW */
		opg = oanon->an_page;
		KASSERT(opg != NULL);
		KASSERT(opg->uobject == NULL || opg->loan_count > 0);
	} else if (uobjpage != PGO_DONTCARE) {
		/* object-backed COW */
		opg = uobjpage;
	} else {
		/* ZFOD */
		opg = NULL;
	}
	if (opg != NULL) {
		uobj = opg->uobject;
	} else {
		uobj = NULL;
	}

	KASSERT(amap != NULL);
	KASSERT(uobjpage != NULL);
	KASSERT(uobjpage == PGO_DONTCARE || (uobjpage->flags & PG_BUSY) != 0);
	LOCK_ASSERT(simple_lock_held(&amap->am_l));
	LOCK_ASSERT(oanon == NULL || simple_lock_held(&oanon->an_lock));
	LOCK_ASSERT(uobj == NULL || simple_lock_held(&uobj->vmobjlock));
	LOCK_ASSERT(*spare == NULL || !simple_lock_held(&(*spare)->an_lock));

	if (*spare != NULL) {
		anon = *spare;
		*spare = NULL;
		simple_lock(&anon->an_lock);
	} else if (ufi->map != kernel_map) {
		anon = uvm_analloc();
	} else {
		UVMHIST_LOG(maphist, "kernel_map, unlock and retry", 0,0,0,0);

		/*
		 * we can't allocate anons with kernel_map locked.
		 */

		uvm_page_unbusy(&uobjpage, 1);
		uvmfault_unlockall(ufi, amap, uobj, oanon);

		*spare = uvm_analloc();
		if (*spare == NULL) {
			goto nomem;
		}
		simple_unlock(&(*spare)->an_lock);
		error = ERESTART;
		goto done;
	}
	if (anon) {

		/*
		 * The new anon is locked.
		 *
		 * if opg == NULL, we want a zero'd, dirty page,
		 * so have uvm_pagealloc() do that for us.
		 */

		pg = uvm_pagealloc(NULL, 0, anon,
		    (opg == NULL) ? UVM_PGA_ZERO : 0);
	} else {
		pg = NULL;
	}

	/*
	 * out of memory resources?
	 */

	if (pg == NULL) {
		/* save anon for the next try. */
		if (anon != NULL) {
			simple_unlock(&anon->an_lock);
			*spare = anon;
		}

		/* unlock and fail ... */
		uvm_page_unbusy(&uobjpage, 1);
		uvmfault_unlockall(ufi, amap, uobj, oanon);
nomem:
		if (!uvm_reclaimable()) {
			UVMHIST_LOG(maphist, "out of VM", 0,0,0,0);
			uvmexp.fltnoanon++;
			error = ENOMEM;
			goto done;
		}

		UVMHIST_LOG(maphist, "out of RAM, waiting for more", 0,0,0,0);
		uvmexp.fltnoram++;
		uvm_wait("flt_noram5");
		error = ERESTART;
		goto done;
	}

	/* copy page [pg now dirty] */
	if (opg) {
		uvm_pagecopy(opg, pg);
	}

	amap_add(&ufi->entry->aref, ufi->orig_rvaddr - ufi->entry->start, anon,
	    oanon != NULL);

	*nanon = anon;
	error = 0;
done:
	return error;
}


/*
 *   F A U L T   -   m a i n   e n t r y   p o i n t
 */

/*
 * uvm_fault: page fault handler
 *
 * => called from MD code to resolve a page fault
 * => VM data structures usually should be unlocked.   however, it is
 *	possible to call here with the main map locked if the caller
 *	gets a write lock, sets it recusive, and then calls us (c.f.
 *	uvm_map_pageable).   this should be avoided because it keeps
 *	the map locked off during I/O.
 * => MUST NEVER BE CALLED IN INTERRUPT CONTEXT
 */

#define MASK(entry)     (UVM_ET_ISCOPYONWRITE(entry) ? \
			 ~VM_PROT_WRITE : VM_PROT_ALL)

/* fault_flag values passed from uvm_fault_wire to uvm_fault_internal */
#define UVM_FAULT_WIRE 1
#define UVM_FAULT_WIREMAX 2

int
uvm_fault_internal(struct vm_map *orig_map, vaddr_t vaddr,
    vm_prot_t access_type, int fault_flag)
{
	struct uvm_faultinfo ufi;
	vm_prot_t enter_prot, check_prot;
	boolean_t wired, narrow, promote, locked, shadowed, wire_fault, cow_now;
	int npages, nback, nforw, centeridx, error, lcv, gotpages;
	vaddr_t startva, currva;
	voff_t uoff;
	struct vm_amap *amap;
	struct uvm_object *uobj;
	struct vm_anon *anons_store[UVM_MAXRANGE], **anons, *anon, *oanon;
	struct vm_anon *anon_spare;
	struct vm_page *pages[UVM_MAXRANGE], *pg, *uobjpage;
	UVMHIST_FUNC("uvm_fault"); UVMHIST_CALLED(maphist);

	UVMHIST_LOG(maphist, "(map=0x%x, vaddr=0x%x, at=%d, ff=%d)",
	      orig_map, vaddr, access_type, fault_flag);

	anon = anon_spare = NULL;
	pg = NULL;

	uvmexp.faults++;	/* XXX: locking? */

	/*
	 * init the IN parameters in the ufi
	 */

	ufi.orig_map = orig_map;
	ufi.orig_rvaddr = trunc_page(vaddr);
	ufi.orig_size = PAGE_SIZE;	/* can't get any smaller than this */
	wire_fault = (fault_flag > 0);
	if (wire_fault)
		narrow = TRUE;		/* don't look for neighborhood
					 * pages on wire */
	else
		narrow = FALSE;		/* normal fault */

	/*
	 * "goto ReFault" means restart the page fault from ground zero.
	 */
ReFault:

	/*
	 * lookup and lock the maps
	 */

	if (uvmfault_lookup(&ufi, FALSE) == FALSE) {
		UVMHIST_LOG(maphist, "<- no mapping @ 0x%x", vaddr, 0,0,0);
		error = EFAULT;
		goto done;
	}
	/* locked: maps(read) */

#ifdef DIAGNOSTIC
	if ((ufi.map->flags & VM_MAP_PAGEABLE) == 0) {
		printf("Page fault on non-pageable map:\n");
		printf("ufi.map = %p\n", ufi.map);
		printf("ufi.orig_map = %p\n", ufi.orig_map);
		printf("ufi.orig_rvaddr = 0x%lx\n", (u_long) ufi.orig_rvaddr);
		panic("uvm_fault: (ufi.map->flags & VM_MAP_PAGEABLE) == 0");
	}
#endif

	/*
	 * check protection
	 */

	check_prot = fault_flag == UVM_FAULT_WIREMAX ?
	    ufi.entry->max_protection : ufi.entry->protection;
	if ((check_prot & access_type) != access_type) {
		UVMHIST_LOG(maphist,
		    "<- protection failure (prot=0x%x, access=0x%x)",
		    ufi.entry->protection, access_type, 0, 0);
		uvmfault_unlockmaps(&ufi, FALSE);
		error = EACCES;
		goto done;
	}

	/*
	 * "enter_prot" is the protection we want to enter the page in at.
	 * for certain pages (e.g. copy-on-write pages) this protection can
	 * be more strict than ufi.entry->protection.  "wired" means either
	 * the entry is wired or we are fault-wiring the pg.
	 */

	enter_prot = ufi.entry->protection;
	wired = VM_MAPENT_ISWIRED(ufi.entry) || wire_fault;
	if (wired) {
		access_type = enter_prot; /* full access for wired */
		cow_now = (check_prot & VM_PROT_WRITE) != 0;
	} else {
		cow_now = (access_type & VM_PROT_WRITE) != 0;
	}

	/*
	 * handle "needs_copy" case.   if we need to copy the amap we will
	 * have to drop our readlock and relock it with a write lock.  (we
	 * need a write lock to change anything in a map entry [e.g.
	 * needs_copy]).
	 */

	if (UVM_ET_ISNEEDSCOPY(ufi.entry)) {
		KASSERT(fault_flag != UVM_FAULT_WIREMAX);
		if (cow_now || (ufi.entry->object.uvm_obj == NULL)) {
			/* need to clear */
			UVMHIST_LOG(maphist,
			    "  need to clear needs_copy and refault",0,0,0,0);
			uvmfault_unlockmaps(&ufi, FALSE);
			uvmfault_amapcopy(&ufi);
			uvmexp.fltamcopy++;
			goto ReFault;

		} else {

			/*
			 * ensure that we pmap_enter page R/O since
			 * needs_copy is still true
			 */

			enter_prot &= ~VM_PROT_WRITE;
		}
	}

	/*
	 * identify the players
	 */

	amap = ufi.entry->aref.ar_amap;		/* top layer */
	uobj = ufi.entry->object.uvm_obj;	/* bottom layer */

	/*
	 * check for a case 0 fault.  if nothing backing the entry then
	 * error now.
	 */

	if (amap == NULL && uobj == NULL) {
		uvmfault_unlockmaps(&ufi, FALSE);
		UVMHIST_LOG(maphist,"<- no backing store, no overlay",0,0,0,0);
		error = EFAULT;
		goto done;
	}

	/*
	 * establish range of interest based on advice from mapper
	 * and then clip to fit map entry.   note that we only want
	 * to do this the first time through the fault.   if we
	 * ReFault we will disable this by setting "narrow" to true.
	 */

	if (narrow == FALSE) {

		/* wide fault (!narrow) */
		KASSERT(uvmadvice[ufi.entry->advice].advice ==
			 ufi.entry->advice);
		nback = MIN(uvmadvice[ufi.entry->advice].nback,
			    (ufi.orig_rvaddr - ufi.entry->start) >> PAGE_SHIFT);
		startva = ufi.orig_rvaddr - (nback << PAGE_SHIFT);
		nforw = MIN(uvmadvice[ufi.entry->advice].nforw,
			    ((ufi.entry->end - ufi.orig_rvaddr) >>
			     PAGE_SHIFT) - 1);
		/*
		 * note: "-1" because we don't want to count the
		 * faulting page as forw
		 */
		npages = nback + nforw + 1;
		centeridx = nback;

		narrow = TRUE;	/* ensure only once per-fault */

	} else {

		/* narrow fault! */
		nback = nforw = 0;
		startva = ufi.orig_rvaddr;
		npages = 1;
		centeridx = 0;

	}

	/* locked: maps(read) */
	UVMHIST_LOG(maphist, "  narrow=%d, back=%d, forw=%d, startva=0x%x",
		    narrow, nback, nforw, startva);
	UVMHIST_LOG(maphist, "  entry=0x%x, amap=0x%x, obj=0x%x", ufi.entry,
		    amap, uobj, 0);

	/*
	 * if we've got an amap, lock it and extract current anons.
	 */

	if (amap) {
		amap_lock(amap);
		anons = anons_store;
		amap_lookups(&ufi.entry->aref, startva - ufi.entry->start,
		    anons, npages);
	} else {
		anons = NULL;	/* to be safe */
	}

	/* locked: maps(read), amap(if there) */
	LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));

	/*
	 * for MADV_SEQUENTIAL mappings we want to deactivate the back pages
	 * now and then forget about them (for the rest of the fault).
	 */

	if (ufi.entry->advice == MADV_SEQUENTIAL && nback != 0) {

		UVMHIST_LOG(maphist, "  MADV_SEQUENTIAL: flushing backpages",
		    0,0,0,0);
		/* flush back-page anons? */
		if (amap)
			uvmfault_anonflush(anons, nback);

		/* flush object? */
		if (uobj) {
			uoff = (startva - ufi.entry->start) + ufi.entry->offset;
			simple_lock(&uobj->vmobjlock);
			(void) (uobj->pgops->pgo_put)(uobj, uoff, uoff +
				    (nback << PAGE_SHIFT), PGO_DEACTIVATE);
		}

		/* now forget about the backpages */
		if (amap)
			anons += nback;
		startva += (nback << PAGE_SHIFT);
		npages -= nback;
		nback = centeridx = 0;
	}

	/* locked: maps(read), amap(if there) */
	LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));

	/*
	 * map in the backpages and frontpages we found in the amap in hopes
	 * of preventing future faults.    we also init the pages[] array as
	 * we go.
	 */

	currva = startva;
	shadowed = FALSE;
	for (lcv = 0 ; lcv < npages ; lcv++, currva += PAGE_SIZE) {

		/*
		 * dont play with VAs that are already mapped
		 * except for center)
		 */
		if (lcv != centeridx &&
		    pmap_extract(ufi.orig_map->pmap, currva, NULL)) {
			pages[lcv] = PGO_DONTCARE;
			continue;
		}

		/*
		 * unmapped or center page.   check if any anon at this level.
		 */
		if (amap == NULL || anons[lcv] == NULL) {
			pages[lcv] = NULL;
			continue;
		}

		/*
		 * check for present page and map if possible.   re-activate it.
		 */

		pages[lcv] = PGO_DONTCARE;
		if (lcv == centeridx) {		/* save center for later! */
			shadowed = TRUE;
			continue;
		}
		anon = anons[lcv];
		simple_lock(&anon->an_lock);
		/* ignore loaned pages */
		if (anon->an_page && anon->an_page->loan_count == 0 &&
		    (anon->an_page->flags & PG_BUSY) == 0) {
			uvm_lock_pageq();
			uvm_pageenqueue(anon->an_page);
			uvm_unlock_pageq();
			UVMHIST_LOG(maphist,
			    "  MAPPING: n anon: pm=0x%x, va=0x%x, pg=0x%x",
			    ufi.orig_map->pmap, currva, anon->an_page, 0);
			uvmexp.fltnamap++;

			/*
			 * Since this isn't the page that's actually faulting,
			 * ignore pmap_enter() failures; it's not critical
			 * that we enter these right now.
			 */

			(void) pmap_enter(ufi.orig_map->pmap, currva,
			    VM_PAGE_TO_PHYS(anon->an_page),
			    (anon->an_ref > 1) ? (enter_prot & ~VM_PROT_WRITE) :
			    enter_prot,
			    PMAP_CANFAIL |
			     (VM_MAPENT_ISWIRED(ufi.entry) ? PMAP_WIRED : 0));
		}
		simple_unlock(&anon->an_lock);
		pmap_update(ufi.orig_map->pmap);
	}

	/* locked: maps(read), amap(if there) */
	LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));
	/* (shadowed == TRUE) if there is an anon at the faulting address */
	UVMHIST_LOG(maphist, "  shadowed=%d, will_get=%d", shadowed,
	    (uobj && shadowed == FALSE),0,0);

	/*
	 * note that if we are really short of RAM we could sleep in the above
	 * call to pmap_enter with everything locked.   bad?
	 *
	 * XXX Actually, that is bad; pmap_enter() should just fail in that
	 * XXX case.  --thorpej
	 */

	/*
	 * if the desired page is not shadowed by the amap and we have a
	 * backing object, then we check to see if the backing object would
	 * prefer to handle the fault itself (rather than letting us do it
	 * with the usual pgo_get hook).  the backing object signals this by
	 * providing a pgo_fault routine.
	 */

	if (uobj && shadowed == FALSE && uobj->pgops->pgo_fault != NULL) {
		simple_lock(&uobj->vmobjlock);

		/* locked: maps(read), amap (if there), uobj */
		error = uobj->pgops->pgo_fault(&ufi, startva, pages, npages,
		    centeridx, access_type, PGO_LOCKED|PGO_SYNCIO);

		/* locked: nothing, pgo_fault has unlocked everything */

		if (error == ERESTART)
			goto ReFault;		/* try again! */
		/*
		 * object fault routine responsible for pmap_update().
		 */
		goto done;
	}

	/*
	 * now, if the desired page is not shadowed by the amap and we have
	 * a backing object that does not have a special fault routine, then
	 * we ask (with pgo_get) the object for resident pages that we care
	 * about and attempt to map them in.  we do not let pgo_get block
	 * (PGO_LOCKED).
	 */

	if (uobj && shadowed == FALSE) {
		simple_lock(&uobj->vmobjlock);

		/* locked (!shadowed): maps(read), amap (if there), uobj */
		/*
		 * the following call to pgo_get does _not_ change locking state
		 */

		uvmexp.fltlget++;
		gotpages = npages;
		(void) uobj->pgops->pgo_get(uobj, ufi.entry->offset +
				(startva - ufi.entry->start),
				pages, &gotpages, centeridx,
				access_type & MASK(ufi.entry),
				ufi.entry->advice, PGO_LOCKED);

		/*
		 * check for pages to map, if we got any
		 */

		uobjpage = NULL;

		if (gotpages) {
			currva = startva;
			for (lcv = 0; lcv < npages;
			     lcv++, currva += PAGE_SIZE) {
				struct vm_page *curpg;
				boolean_t readonly;

				curpg = pages[lcv];
				if (curpg == NULL || curpg == PGO_DONTCARE) {
					continue;
				}
				KASSERT(curpg->uobject == uobj);

				/*
				 * if center page is resident and not
				 * PG_BUSY|PG_RELEASED then pgo_get
				 * made it PG_BUSY for us and gave
				 * us a handle to it.   remember this
				 * page as "uobjpage." (for later use).
				 */

				if (lcv == centeridx) {
					uobjpage = curpg;
					UVMHIST_LOG(maphist, "  got uobjpage "
					    "(0x%x) with locked get",
					    uobjpage, 0,0,0);
					continue;
				}

				/*
				 * calling pgo_get with PGO_LOCKED returns us
				 * pages which are neither busy nor released,
				 * so we don't need to check for this.
				 * we can just directly enter the pages.
				 */

				uvm_lock_pageq();
				uvm_pageenqueue(curpg);
				uvm_unlock_pageq();
				UVMHIST_LOG(maphist,
				  "  MAPPING: n obj: pm=0x%x, va=0x%x, pg=0x%x",
				  ufi.orig_map->pmap, currva, curpg, 0);
				uvmexp.fltnomap++;

				/*
				 * Since this page isn't the page that's
				 * actually faulting, ignore pmap_enter()
				 * failures; it's not critical that we
				 * enter these right now.
				 */
				KASSERT((curpg->flags & PG_PAGEOUT) == 0);
				KASSERT((curpg->flags & PG_RELEASED) == 0);
				KASSERT(!UVM_OBJ_IS_CLEAN(curpg->uobject) ||
				    (curpg->flags & PG_CLEAN) != 0);
				readonly = (curpg->flags & PG_RDONLY)
				    || (curpg->loan_count > 0)
				    || UVM_OBJ_NEEDS_WRITEFAULT(curpg->uobject);

				(void) pmap_enter(ufi.orig_map->pmap, currva,
				    VM_PAGE_TO_PHYS(curpg),
				    readonly ?
				    enter_prot & ~VM_PROT_WRITE :
				    enter_prot & MASK(ufi.entry),
				    PMAP_CANFAIL |
				     (wired ? PMAP_WIRED : 0));

				/*
				 * NOTE: page can't be PG_WANTED or PG_RELEASED
				 * because we've held the lock the whole time
				 * we've had the handle.
				 */
				KASSERT((curpg->flags & PG_WANTED) == 0);
				KASSERT((curpg->flags & PG_RELEASED) == 0);

				curpg->flags &= ~(PG_BUSY);
				UVM_PAGE_OWN(curpg, NULL);
			}
			pmap_update(ufi.orig_map->pmap);
		}
	} else {
		uobjpage = NULL;
	}

	/* locked (shadowed): maps(read), amap */
	/* locked (!shadowed): maps(read), amap(if there),
		 uobj(if !null), uobjpage(if !null) */
	if (shadowed) {
		LOCK_ASSERT(simple_lock_held(&amap->am_l));
	} else {
		LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));
		LOCK_ASSERT(uobj == NULL || simple_lock_held(&uobj->vmobjlock));
		KASSERT(uobjpage == NULL || (uobjpage->flags & PG_BUSY) != 0);
	}

	/*
	 * note that at this point we are done with any front or back pages.
	 * we are now going to focus on the center page (i.e. the one we've
	 * faulted on).  if we have faulted on the top (anon) layer
	 * [i.e. case 1], then the anon we want is anons[centeridx] (we have
	 * not touched it yet).  if we have faulted on the bottom (uobj)
	 * layer [i.e. case 2] and the page was both present and available,
	 * then we've got a pointer to it as "uobjpage" and we've already
	 * made it BUSY.
	 */

	/*
	 * there are four possible cases we must address: 1A, 1B, 2A, and 2B
	 */

	/*
	 * redirect case 2: if we are not shadowed, go to case 2.
	 */

	if (shadowed == FALSE)
		goto Case2;

	/* locked: maps(read), amap */

	/*
	 * handle case 1: fault on an anon in our amap
	 */

	anon = anons[centeridx];
	UVMHIST_LOG(maphist, "  case 1 fault: anon=0x%x", anon, 0,0,0);
	simple_lock(&anon->an_lock);

	/* locked: maps(read), amap, anon */
	LOCK_ASSERT(simple_lock_held(&amap->am_l));
	LOCK_ASSERT(simple_lock_held(&anon->an_lock));

	/*
	 * no matter if we have case 1A or case 1B we are going to need to
	 * have the anon's memory resident.   ensure that now.
	 */

	/*
	 * let uvmfault_anonget do the dirty work.
	 * if it fails (!OK) it will unlock everything for us.
	 * if it succeeds, locks are still valid and locked.
	 * also, if it is OK, then the anon's page is on the queues.
	 * if the page is on loan from a uvm_object, then anonget will
	 * lock that object for us if it does not fail.
	 */

	error = uvmfault_anonget(&ufi, amap, anon);
	switch (error) {
	case 0:
		break;

	case ERESTART:
		goto ReFault;

	case EAGAIN:
		tsleep(&lbolt, PVM, "fltagain1", 0);
		goto ReFault;

	default:
		goto done;
	}

	/*
	 * uobj is non null if the page is on loan from an object (i.e. uobj)
	 */

	uobj = anon->an_page->uobject;	/* locked by anonget if !NULL */

	/* locked: maps(read), amap, anon, uobj(if one) */
	LOCK_ASSERT(simple_lock_held(&amap->am_l));
	LOCK_ASSERT(simple_lock_held(&anon->an_lock));
	LOCK_ASSERT(uobj == NULL || simple_lock_held(&uobj->vmobjlock));

	/*
	 * special handling for loaned pages
	 */

	if (anon->an_page->loan_count) {

		if (!cow_now) {

			/*
			 * for read faults on loaned pages we just cap the
			 * protection at read-only.
			 */

			enter_prot = enter_prot & ~VM_PROT_WRITE;

		} else {
			/*
			 * note that we can't allow writes into a loaned page!
			 *
			 * if we have a write fault on a loaned page in an
			 * anon then we need to look at the anon's ref count.
			 * if it is greater than one then we are going to do
			 * a normal copy-on-write fault into a new anon (this
			 * is not a problem).  however, if the reference count
			 * is one (a case where we would normally allow a
			 * write directly to the page) then we need to kill
			 * the loan before we continue.
			 */

			/* >1 case is already ok */
			if (anon->an_ref == 1) {

				/* get new un-owned replacement page */
				pg = uvm_pagealloc(NULL, 0, NULL, 0);
				if (pg == NULL) {
					uvmfault_unlockall(&ufi, amap, uobj,
					    anon);
					uvm_wait("flt_noram2");
					goto ReFault;
				}

				/*
				 * copy data, kill loan, and drop uobj lock
				 * (if any)
				 */
				/* copy old -> new */
				uvm_pagecopy(anon->an_page, pg);

				/* force reload */
				pmap_page_protect(anon->an_page,
						  VM_PROT_NONE);
				uvm_lock_pageq();	  /* KILL loan */

				anon->an_page->uanon = NULL;
				/* in case we owned */
				anon->an_page->pqflags &= ~PQ_ANON;

				if (uobj) {
					/* if we were receiver of loan */
					anon->an_page->loan_count--;
				} else {
					/*
					 * we were the lender (A->K); need
					 * to remove the page from pageq's.
					 */
					uvm_pagedequeue(anon->an_page);
				}

				if (uobj) {
					simple_unlock(&uobj->vmobjlock);
					uobj = NULL;
				}

				/* install new page in anon */
				anon->an_page = pg;
				pg->uanon = anon;
				pg->pqflags |= PQ_ANON;

				uvm_pageactivate(pg);
				uvm_unlock_pageq();

				pg->flags &= ~(PG_BUSY|PG_FAKE);
				UVM_PAGE_OWN(pg, NULL);

				/* done! */
			}     /* ref == 1 */
		}       /* write fault */
	}         /* loan count */

	/*
	 * if we are case 1B then we will need to allocate a new blank
	 * anon to transfer the data into.   note that we have a lock
	 * on anon, so no one can busy or release the page until we are done.
	 * also note that the ref count can't drop to zero here because
	 * it is > 1 and we are only dropping one ref.
	 *
	 * in the (hopefully very rare) case that we are out of RAM we
	 * will unlock, wait for more RAM, and refault.
	 *
	 * if we are out of anon VM we kill the process (XXX: could wait?).
	 */

	if (cow_now && anon->an_ref > 1) {

		UVMHIST_LOG(maphist, "  case 1B: COW fault",0,0,0,0);
		uvmexp.flt_acow++;
		oanon = anon;		/* oanon = old, locked anon */

		error = uvmfault_promote(&ufi, oanon, PGO_DONTCARE,
		    &anon, &anon_spare);
		switch (error) {
		case 0:
			break;
		case ERESTART:
			goto ReFault;
		default:
			goto done;
		}

		pg = anon->an_page;
		uvm_lock_pageq();
		uvm_pageactivate(pg);
		uvm_unlock_pageq();
		pg->flags &= ~(PG_BUSY|PG_FAKE);
		UVM_PAGE_OWN(pg, NULL);

		/* deref: can not drop to zero here by defn! */
		oanon->an_ref--;

		/*
		 * note: oanon is still locked, as is the new anon.  we
		 * need to check for this later when we unlock oanon; if
		 * oanon != anon, we'll have to unlock anon, too.
		 */

	} else {

		uvmexp.flt_anon++;
		oanon = anon;		/* old, locked anon is same as anon */
		pg = anon->an_page;
		if (anon->an_ref > 1)     /* disallow writes to ref > 1 anons */
			enter_prot = enter_prot & ~VM_PROT_WRITE;

	}

	/* locked: maps(read), amap, oanon, anon (if different from oanon) */
	LOCK_ASSERT(simple_lock_held(&amap->am_l));
	LOCK_ASSERT(simple_lock_held(&anon->an_lock));
	LOCK_ASSERT(simple_lock_held(&oanon->an_lock));

	/*
	 * now map the page in.
	 */

	UVMHIST_LOG(maphist, "  MAPPING: anon: pm=0x%x, va=0x%x, pg=0x%x",
	    ufi.orig_map->pmap, ufi.orig_rvaddr, pg, 0);
	if (pmap_enter(ufi.orig_map->pmap, ufi.orig_rvaddr, VM_PAGE_TO_PHYS(pg),
	    enter_prot, access_type | PMAP_CANFAIL | (wired ? PMAP_WIRED : 0))
	    != 0) {

		/*
		 * No need to undo what we did; we can simply think of
		 * this as the pmap throwing away the mapping information.
		 *
		 * We do, however, have to go through the ReFault path,
		 * as the map may change while we're asleep.
		 */

		if (anon != oanon)
			simple_unlock(&anon->an_lock);
		uvmfault_unlockall(&ufi, amap, uobj, oanon);
		if (!uvm_reclaimable()) {
			UVMHIST_LOG(maphist,
			    "<- failed.  out of VM",0,0,0,0);
			/* XXX instrumentation */
			error = ENOMEM;
			goto done;
		}
		/* XXX instrumentation */
		uvm_wait("flt_pmfail1");
		goto ReFault;
	}

	/*
	 * ... update the page queues.
	 */

	uvm_lock_pageq();
	if (wire_fault) {
		uvm_pagewire(pg);

		/*
		 * since the now-wired page cannot be paged out,
		 * release its swap resources for others to use.
		 * since an anon with no swap cannot be PG_CLEAN,
		 * clear its clean flag now.
		 */

		pg->flags &= ~(PG_CLEAN);
		uvm_anon_dropswap(anon);
	} else {
		uvm_pageactivate(pg);
	}
	uvm_unlock_pageq();

	/*
	 * done case 1!  finish up by unlocking everything and returning success
	 */

	if (anon != oanon)
		simple_unlock(&anon->an_lock);
	uvmfault_unlockall(&ufi, amap, uobj, oanon);
	pmap_update(ufi.orig_map->pmap);
	error = 0;
	goto done;

Case2:
	/*
	 * handle case 2: faulting on backing object or zero fill
	 */

	/*
	 * locked:
	 * maps(read), amap(if there), uobj(if !null), uobjpage(if !null)
	 */
	LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));
	LOCK_ASSERT(uobj == NULL || simple_lock_held(&uobj->vmobjlock));
	LOCK_ASSERT(uobjpage == NULL || (uobjpage->flags & PG_BUSY) != 0);

	/*
	 * note that uobjpage can not be PGO_DONTCARE at this point.  we now
	 * set uobjpage to PGO_DONTCARE if we are doing a zero fill.  if we
	 * have a backing object, check and see if we are going to promote
	 * the data up to an anon during the fault.
	 */

	if (uobj == NULL) {
		uobjpage = PGO_DONTCARE;
		promote = TRUE;		/* always need anon here */
	} else {
		KASSERT(uobjpage != PGO_DONTCARE);
		promote = cow_now && UVM_ET_ISCOPYONWRITE(ufi.entry);
	}
	UVMHIST_LOG(maphist, "  case 2 fault: promote=%d, zfill=%d",
	    promote, (uobj == NULL), 0,0);

	/*
	 * if uobjpage is not null then we do not need to do I/O to get the
	 * uobjpage.
	 *
	 * if uobjpage is null, then we need to unlock and ask the pager to
	 * get the data for us.   once we have the data, we need to reverify
	 * the state the world.   we are currently not holding any resources.
	 */

	if (uobjpage) {
		/* update rusage counters */
		curproc->p_stats->p_ru.ru_minflt++;
	} else {
		/* update rusage counters */
		curproc->p_stats->p_ru.ru_majflt++;

		/* locked: maps(read), amap(if there), uobj */
		uvmfault_unlockall(&ufi, amap, NULL, NULL);
		/* locked: uobj */

		uvmexp.fltget++;
		gotpages = 1;
		uoff = (ufi.orig_rvaddr - ufi.entry->start) + ufi.entry->offset;
		error = uobj->pgops->pgo_get(uobj, uoff, &uobjpage, &gotpages,
		    0, access_type & MASK(ufi.entry), ufi.entry->advice,
		    PGO_SYNCIO);
		/* locked: uobjpage(if no error) */
		LOCK_ASSERT(error != 0 || (uobjpage->flags & PG_BUSY) != 0);

		/*
		 * recover from I/O
		 */

		if (error) {
			if (error == EAGAIN) {
				UVMHIST_LOG(maphist,
				    "  pgo_get says TRY AGAIN!",0,0,0,0);
				tsleep(&lbolt, PVM, "fltagain2", 0);
				goto ReFault;
			}

			UVMHIST_LOG(maphist, "<- pgo_get failed (code %d)",
			    error, 0,0,0);
			goto done;
		}

		/* locked: uobjpage */

		uvm_lock_pageq();
		uvm_pageactivate(uobjpage);
		uvm_unlock_pageq();

		/*
		 * re-verify the state of the world by first trying to relock
		 * the maps.  always relock the object.
		 */

		locked = uvmfault_relock(&ufi);
		if (locked && amap)
			amap_lock(amap);
		uobj = uobjpage->uobject;
		simple_lock(&uobj->vmobjlock);

		/* locked(locked): maps(read), amap(if !null), uobj, uobjpage */
		/* locked(!locked): uobj, uobjpage */

		/*
		 * verify that the page has not be released and re-verify
		 * that amap slot is still free.   if there is a problem,
		 * we unlock and clean up.
		 */

		if ((uobjpage->flags & PG_RELEASED) != 0 ||
		    (locked && amap &&
		    amap_lookup(&ufi.entry->aref,
		      ufi.orig_rvaddr - ufi.entry->start))) {
			if (locked)
				uvmfault_unlockall(&ufi, amap, NULL, NULL);
			locked = FALSE;
		}

		/*
		 * didn't get the lock?   release the page and retry.
		 */

		if (locked == FALSE) {
			UVMHIST_LOG(maphist,
			    "  wasn't able to relock after fault: retry",
			    0,0,0,0);
			if (uobjpage->flags & PG_WANTED)
				wakeup(uobjpage);
			if (uobjpage->flags & PG_RELEASED) {
				uvmexp.fltpgrele++;
				uvm_pagefree(uobjpage);
				goto ReFault;
			}
			uobjpage->flags &= ~(PG_BUSY|PG_WANTED);
			UVM_PAGE_OWN(uobjpage, NULL);
			simple_unlock(&uobj->vmobjlock);
			goto ReFault;
		}

		/*
		 * we have the data in uobjpage which is busy and
		 * not released.  we are holding object lock (so the page
		 * can't be released on us).
		 */

		/* locked: maps(read), amap(if !null), uobj, uobjpage */
	}

	/*
	 * locked:
	 * maps(read), amap(if !null), uobj(if !null), uobjpage(if uobj)
	 */
	LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));
	LOCK_ASSERT(uobj == NULL || simple_lock_held(&uobj->vmobjlock));
	LOCK_ASSERT(uobj == NULL || (uobjpage->flags & PG_BUSY) != 0);

	/*
	 * notes:
	 *  - at this point uobjpage can not be NULL
	 *  - at this point uobjpage can not be PG_RELEASED (since we checked
	 *  for it above)
	 *  - at this point uobjpage could be PG_WANTED (handle later)
	 */

	KASSERT(uobj == NULL || uobj == uobjpage->uobject);
	KASSERT(uobj == NULL || !UVM_OBJ_IS_CLEAN(uobjpage->uobject) ||
	    (uobjpage->flags & PG_CLEAN) != 0);
	if (promote == FALSE) {

		/*
		 * we are not promoting.   if the mapping is COW ensure that we
		 * don't give more access than we should (e.g. when doing a read
		 * fault on a COPYONWRITE mapping we want to map the COW page in
		 * R/O even though the entry protection could be R/W).
		 *
		 * set "pg" to the page we want to map in (uobjpage, usually)
		 */

		/* no anon in this case. */
		anon = NULL;

		uvmexp.flt_obj++;
		if (UVM_ET_ISCOPYONWRITE(ufi.entry) ||
		    UVM_OBJ_NEEDS_WRITEFAULT(uobjpage->uobject))
			enter_prot &= ~VM_PROT_WRITE;
		pg = uobjpage;		/* map in the actual object */

		KASSERT(uobjpage != PGO_DONTCARE);

		/*
		 * we are faulting directly on the page.   be careful
		 * about writing to loaned pages...
		 */

		if (uobjpage->loan_count) {
			if (!cow_now) {
				/* read fault: cap the protection at readonly */
				/* cap! */
				enter_prot = enter_prot & ~VM_PROT_WRITE;
			} else {
				/* write fault: must break the loan here */

				pg = uvm_loanbreak(uobjpage);
				if (pg == NULL) {

					/*
					 * drop ownership of page, it can't
					 * be released
					 */

					if (uobjpage->flags & PG_WANTED)
						wakeup(uobjpage);
					uobjpage->flags &= ~(PG_BUSY|PG_WANTED);
					UVM_PAGE_OWN(uobjpage, NULL);

					uvmfault_unlockall(&ufi, amap, uobj,
					  NULL);
					UVMHIST_LOG(maphist,
					  "  out of RAM breaking loan, waiting",
					  0,0,0,0);
					uvmexp.fltnoram++;
					uvm_wait("flt_noram4");
					goto ReFault;
				}
				uobjpage = pg;
			}
		}
	} else {

		/*
		 * if we are going to promote the data to an anon we
		 * allocate a blank anon here and plug it into our amap.
		 */
#if DIAGNOSTIC
		if (amap == NULL)
			panic("uvm_fault: want to promote data, but no anon");
#endif
		error = uvmfault_promote(&ufi, NULL, uobjpage,
		    &anon, &anon_spare);
		switch (error) {
		case 0:
			break;
		case ERESTART:
			goto ReFault;
		default:
			goto done;
		}

		pg = anon->an_page;

		/*
		 * fill in the data
		 */

		if (uobjpage != PGO_DONTCARE) {
			uvmexp.flt_prcopy++;

			/*
			 * promote to shared amap?  make sure all sharing
			 * procs see it
			 */

			if ((amap_flags(amap) & AMAP_SHARED) != 0) {
				pmap_page_protect(uobjpage, VM_PROT_NONE);
				/*
				 * XXX: PAGE MIGHT BE WIRED!
				 */
			}

			/*
			 * dispose of uobjpage.  it can't be PG_RELEASED
			 * since we still hold the object lock.
			 * drop handle to uobj as well.
			 */

			if (uobjpage->flags & PG_WANTED)
				/* still have the obj lock */
				wakeup(uobjpage);
			uobjpage->flags &= ~(PG_BUSY|PG_WANTED);
			UVM_PAGE_OWN(uobjpage, NULL);
			simple_unlock(&uobj->vmobjlock);
			uobj = NULL;

			UVMHIST_LOG(maphist,
			    "  promote uobjpage 0x%x to anon/page 0x%x/0x%x",
			    uobjpage, anon, pg, 0);

		} else {
			uvmexp.flt_przero++;

			/*
			 * Page is zero'd and marked dirty by
			 * uvmfault_promote().
			 */

			UVMHIST_LOG(maphist,"  zero fill anon/page 0x%x/0%x",
			    anon, pg, 0, 0);
		}
	}

	/*
	 * locked:
	 * maps(read), amap(if !null), uobj(if !null), uobjpage(if uobj),
	 *   anon(if !null), pg(if anon)
	 *
	 * note: pg is either the uobjpage or the new page in the new anon
	 */
	LOCK_ASSERT(amap == NULL || simple_lock_held(&amap->am_l));
	LOCK_ASSERT(uobj == NULL || simple_lock_held(&uobj->vmobjlock));
	LOCK_ASSERT(uobj == NULL || (uobjpage->flags & PG_BUSY) != 0);
	LOCK_ASSERT(anon == NULL || simple_lock_held(&anon->an_lock));
	LOCK_ASSERT((pg->flags & PG_BUSY) != 0);

	/*
	 * all resources are present.   we can now map it in and free our
	 * resources.
	 */

	UVMHIST_LOG(maphist,
	    "  MAPPING: case2: pm=0x%x, va=0x%x, pg=0x%x, promote=%d",
	    ufi.orig_map->pmap, ufi.orig_rvaddr, pg, promote);
	KASSERT((access_type & VM_PROT_WRITE) == 0 ||
		(pg->flags & PG_RDONLY) == 0);
	if (pmap_enter(ufi.orig_map->pmap, ufi.orig_rvaddr, VM_PAGE_TO_PHYS(pg),
	    pg->flags & PG_RDONLY ? enter_prot & ~VM_PROT_WRITE : enter_prot,
	    access_type | PMAP_CANFAIL | (wired ? PMAP_WIRED : 0)) != 0) {

		/*
		 * No need to undo what we did; we can simply think of
		 * this as the pmap throwing away the mapping information.
		 *
		 * We do, however, have to go through the ReFault path,
		 * as the map may change while we're asleep.
		 */

		if (pg->flags & PG_WANTED)
			wakeup(pg);

		/*
		 * note that pg can't be PG_RELEASED since we did not drop
		 * the object lock since the last time we checked.
		 */
		KASSERT((pg->flags & PG_RELEASED) == 0);

		pg->flags &= ~(PG_BUSY|PG_FAKE|PG_WANTED);
		UVM_PAGE_OWN(pg, NULL);
		uvmfault_unlockall(&ufi, amap, uobj, anon);
		if (!uvm_reclaimable()) {
			UVMHIST_LOG(maphist,
			    "<- failed.  out of VM",0,0,0,0);
			/* XXX instrumentation */
			error = ENOMEM;
			goto done;
		}
		/* XXX instrumentation */
		uvm_wait("flt_pmfail2");
		goto ReFault;
	}

	uvm_lock_pageq();
	if (wire_fault) {
		uvm_pagewire(pg);
		if (pg->pqflags & PQ_AOBJ) {

			/*
			 * since the now-wired page cannot be paged out,
			 * release its swap resources for others to use.
			 * since an aobj page with no swap cannot be PG_CLEAN,
			 * clear its clean flag now.
			 */

			KASSERT(uobj != NULL);
			pg->flags &= ~(PG_CLEAN);
			uao_dropswap(uobj, pg->offset >> PAGE_SHIFT);
		}
	} else {
		uvm_pageactivate(pg);
	}
	uvm_unlock_pageq();
	if (pg->flags & PG_WANTED)
		wakeup(pg);

	/*
	 * note that pg can't be PG_RELEASED since we did not drop the object
	 * lock since the last time we checked.
	 */
	KASSERT((pg->flags & PG_RELEASED) == 0);

	pg->flags &= ~(PG_BUSY|PG_FAKE|PG_WANTED);
	UVM_PAGE_OWN(pg, NULL);
	uvmfault_unlockall(&ufi, amap, uobj, anon);
	pmap_update(ufi.orig_map->pmap);
	UVMHIST_LOG(maphist, "<- done (SUCCESS!)",0,0,0,0);
	error = 0;
done:
	if (anon_spare != NULL) {
		anon_spare->an_ref--;
		uvm_anfree(anon_spare);
	}
	return error;
}


/*
 * uvm_fault_wire: wire down a range of virtual addresses in a map.
 *
 * => map may be read-locked by caller, but MUST NOT be write-locked.
 * => if map is read-locked, any operations which may cause map to
 *	be write-locked in uvm_fault() must be taken care of by
 *	the caller.  See uvm_map_pageable().
 */

int
uvm_fault_wire(struct vm_map *map, vaddr_t start, vaddr_t end,
    vm_prot_t access_type, int wiremax)
{
	vaddr_t va;
	int error;

	/*
	 * now fault it in a page at a time.   if the fault fails then we have
	 * to undo what we have done.   note that in uvm_fault VM_PROT_NONE
	 * is replaced with the max protection if fault_type is VM_FAULT_WIRE.
	 */

	/*
	 * XXX work around overflowing a vaddr_t.  this prevents us from
	 * wiring the last page in the address space, though.
	 */
	if (start > end) {
		return EFAULT;
	}

	for (va = start ; va < end ; va += PAGE_SIZE) {
		error = uvm_fault_internal(map, va, access_type,
				wiremax ? UVM_FAULT_WIREMAX : UVM_FAULT_WIRE);
		if (error) {
			if (va != start) {
				uvm_fault_unwire(map, start, va);
			}
			return error;
		}
	}
	return 0;
}

/*
 * uvm_fault_unwire(): unwire range of virtual space.
 */

void
uvm_fault_unwire(struct vm_map *map, vaddr_t start, vaddr_t end)
{
	vm_map_lock_read(map);
	uvm_fault_unwire_locked(map, start, end);
	vm_map_unlock_read(map);
}

/*
 * uvm_fault_unwire_locked(): the guts of uvm_fault_unwire().
 *
 * => map must be at least read-locked.
 */

void
uvm_fault_unwire_locked(struct vm_map *map, vaddr_t start, vaddr_t end)
{
	struct vm_map_entry *entry;
	pmap_t pmap = vm_map_pmap(map);
	vaddr_t va;
	paddr_t pa;
	struct vm_page *pg;

	KASSERT((map->flags & VM_MAP_INTRSAFE) == 0);

	/*
	 * we assume that the area we are unwiring has actually been wired
	 * in the first place.   this means that we should be able to extract
	 * the PAs from the pmap.   we also lock out the page daemon so that
	 * we can call uvm_pageunwire.
	 */

	uvm_lock_pageq();

	/*
	 * find the beginning map entry for the region.
	 */

	KASSERT(start >= vm_map_min(map) && end <= vm_map_max(map));
	if (uvm_map_lookup_entry(map, start, &entry) == FALSE)
		panic("uvm_fault_unwire_locked: address not in map");

	for (va = start; va < end; va += PAGE_SIZE) {
		if (pmap_extract(pmap, va, &pa) == FALSE)
			continue;

		/*
		 * find the map entry for the current address.
		 */

		KASSERT(va >= entry->start);
		while (va >= entry->end) {
			KASSERT(entry->next != &map->header &&
				entry->next->start <= entry->end);
			entry = entry->next;
		}

		/*
		 * if the entry is no longer wired, tell the pmap.
		 */

		if (VM_MAPENT_ISWIRED(entry) == 0)
			pmap_unwire(pmap, va);

		pg = PHYS_TO_VM_PAGE(pa);
		if (pg)
			uvm_pageunwire(pg);
	}

	uvm_unlock_pageq();
}
