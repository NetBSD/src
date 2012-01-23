/*	$NetBSD: uvm_anon.c,v 1.62.2.3 2012/01/23 03:21:18 yamt Exp $	*/

/*
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
 */

/*
 * uvm_anon.c: uvm anon ops
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_anon.c,v 1.62.2.3 2012/01/23 03:21:18 yamt Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>
#include <uvm/uvm_swap.h>
#include <uvm/uvm_pdpolicy.h>

static struct pool_cache	uvm_anon_cache;

static int			uvm_anon_ctor(void *, void *, int);

void
uvm_anon_init(void)
{

	pool_cache_bootstrap(&uvm_anon_cache, sizeof(struct vm_anon), 0, 0,
	    PR_LARGECACHE, "anonpl", NULL, IPL_NONE, uvm_anon_ctor,
	    NULL, NULL);
}

static int
uvm_anon_ctor(void *arg, void *object, int flags)
{
	struct vm_anon *anon = object;

	anon->an_ref = 0;
	anon->an_lock = NULL;
	anon->an_page = NULL;
#if defined(VMSWAP)
	anon->an_swslot = 0;
#endif
	return 0;
}

/*
 * uvm_analloc: allocate a new anon.
 *
 * => anon will have no lock associated.
 */
struct vm_anon *
uvm_analloc(void)
{
	struct vm_anon *anon;

	anon = pool_cache_get(&uvm_anon_cache, PR_NOWAIT);
	if (anon) {
		KASSERT(anon->an_ref == 0);
		KASSERT(anon->an_lock == NULL);
		KASSERT(anon->an_page == NULL);
#if defined(VMSWAP)
		KASSERT(anon->an_swslot == 0);
#endif
		anon->an_ref = 1;
	}
	return anon;
}

/*
 * uvm_anon_dispose: free any resident page or swap resources of anon.
 *
 * => anon must be removed from the amap (if anon was in an amap).
 * => amap must be locked; we may drop and re-acquire the lock here.
 */
static bool
uvm_anon_dispose(struct vm_anon *anon)
{
	struct vm_page *pg = anon->an_page;

	UVMHIST_FUNC("uvm_anon_dispose"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist,"(anon=0x%x)", anon, 0,0,0);

	KASSERT(mutex_owned(anon->an_lock));

	/*
	 * If there is a resident page and it is loaned, then anon may not
	 * own it.  Call out to uvm_anon_lockloanpg() to identify and lock
	 * the real owner of the page.
	 */

	if (pg && pg->loan_count) {
		KASSERT(anon->an_lock != NULL);
		pg = uvm_anon_lockloanpg(anon);
	}

	/*
	 * Dispose the page, if it is resident.
	 */

	if (pg) {
		KASSERT(anon->an_lock != NULL);

		/*
		 * If the page is owned by a UVM object (now locked),
		 * then kill the loan on the page rather than free it,
		 * and release the object lock.
		 */

		if (pg->uobject) {
			struct uvm_cpu *ucpu;

			mutex_enter(&uvm_pageqlock);
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->uanon = NULL;
			anon->an_page = NULL;
			mutex_exit(&uvm_pageqlock);
			ucpu = uvm_cpu_get();
			ucpu->loanfree_orphaned_anon += pg->loan_count;
			ucpu->loanfree_oa_anon++;
			uvm_cpu_put(ucpu);
		} else {

			/*
			 * If page has no UVM object, then anon is the owner,
			 * and it is already locked.
			 */

			KASSERT((pg->flags & PG_RELEASED) == 0);
			pmap_page_protect(pg, VM_PROT_NONE);

			/*
			 * If the page is busy, mark it as PG_RELEASED, so
			 * that uvm_anon_release(9) would release it later.
			 */

			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_RELEASED;
				mutex_obj_hold(anon->an_lock);
				return false;
			}
			mutex_enter(&uvm_pageqlock);
			uvm_pagefree(pg);
			mutex_exit(&uvm_pageqlock);
			UVMHIST_LOG(maphist, "anon 0x%x, page 0x%x: "
				    "freed now!", anon, pg, 0, 0);
		}
	}

#if defined(VMSWAP)
	if (pg == NULL && anon->an_swslot > 0) {
		/* This page is no longer only in swap. */
		mutex_enter(&uvm_swap_data_lock);
		KASSERT(uvmexp.swpgonly > 0);
		uvmexp.swpgonly--;
		mutex_exit(&uvm_swap_data_lock);
	}
#endif

	/*
	 * Free any swap resources, leave a page replacement hint.
	 */

	uvm_anon_dropswap(anon);
	uvmpdpol_anfree(anon);
	UVMHIST_LOG(maphist,"<- done!",0,0,0,0);
	return true;
}

/*
 * uvm_anon_free: free a single anon.
 *
 * => anon must be already disposed.
 */
void
uvm_anon_free(struct vm_anon *anon)
{

	KASSERT(anon->an_ref == 0);
	KASSERT(anon->an_lock == NULL);
	KASSERT(anon->an_page == NULL);
#if defined(VMSWAP)
	KASSERT(anon->an_swslot == 0);
#endif
	pool_cache_put(&uvm_anon_cache, anon);
}

/*
 * uvm_anon_freelst: free a linked list of anon structures.
 *
 * => anon must be locked, we will unlock it.
 */
void
uvm_anon_freelst(struct vm_amap *amap, struct vm_anon *anonlst)
{
	struct vm_anon *anon;
	struct vm_anon **anonp = &anonlst;

	KASSERT(mutex_owned(amap->am_lock));
	while ((anon = *anonp) != NULL) {
		if (!uvm_anon_dispose(anon)) {
			/* Do not free this anon. */
			*anonp = anon->an_link;
			/* Note: clears an_ref as well. */
			anon->an_link = NULL;
		} else {
			anonp = &anon->an_link;
		}
	}
	amap_unlock(amap);

	while (anonlst) {
		anon = anonlst->an_link;
		/* Note: clears an_ref as well. */
		anonlst->an_link = NULL;
		anonlst->an_lock = NULL;
		uvm_anon_free(anonlst);
		anonlst = anon;
	}
}

/*
 * uvm_anon_lockloanpg: given a locked anon, lock its resident page owner.
 *
 * => anon is locked by caller
 * => on return: anon is locked
 *		 if there is a resident page:
 *			if it has a uobject, it is locked by us
 *			if it is ownerless, we take over as owner
 *		 we return the resident page (it can change during
 *		 this function)
 * => note that the only time an anon has an ownerless resident page
 *	is if the page was loaned from a uvm_object and the uvm_object
 *	disowned it
 * => this only needs to be called when you want to do an operation
 *	on an anon's resident page and that page has a non-zero loan
 *	count.
 */
struct vm_page *
uvm_anon_lockloanpg(struct vm_anon *anon)
{
	struct vm_page *pg;

	KASSERT(mutex_owned(anon->an_lock));
	pg = anon->an_page;
	if (pg == NULL) {
		return NULL;
	}
	if (pg->uobject) {
		/*
		 * locked via amap->am_obj_lock
		 */
		KASSERT(mutex_owned(pg->uobject->vmobjlock));
	}
	uvm_loan_resolve_orphan(pg, false);
	return pg;
}

#if defined(VMSWAP)

/*
 * uvm_anon_pagein: fetch an anon's page.
 *
 * => anon must be locked, and is unlocked upon return.
 * => returns true if pagein was aborted due to lack of memory.
 */

bool
uvm_anon_pagein(struct vm_amap *amap, struct vm_anon *anon)
{
	struct vm_page *pg;
	struct uvm_object *uobj;

	KASSERT(mutex_owned(anon->an_lock));
	KASSERT(anon->an_lock == amap->am_lock);

	/*
	 * Get the page of the anon.
	 */

	switch (uvmfault_anonget(NULL, amap, anon)) {
	case 0:
		/* Success - we have the page. */
		KASSERT(mutex_owned(anon->an_lock));
		break;
	case EIO:
	case ERESTART:
		/*
		 * Nothing more to do on errors.  ERESTART means that the
		 * anon was freed.
		 */
		return false;
	default:
		return true;
	}

	/*
	 * Mark the page as dirty, clear its swslot and un-busy it.
	 */

	pg = anon->an_page;
	uobj = pg->uobject;
	if (anon->an_swslot > 0) {
		uvm_swap_free(anon->an_swslot, 1);
	}
	anon->an_swslot = 0;
	uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);

	/*
	 * Deactivate the page (to put it on a page queue).
	 */

	mutex_enter(&uvm_pageqlock);
	if (pg->wire_count == 0) {
		uvm_pagedeactivate(pg);
	}
	mutex_exit(&uvm_pageqlock);

	if (pg->flags & PG_WANTED) {
		pg->flags &= ~PG_WANTED;
		wakeup(pg);
	}

	amap_unlock(amap);
	return false;
}

/*
 * uvm_anon_dropswap: release any swap resources from this anon.
 *
 * => anon must be locked or have a reference count of 0.
 */
void
uvm_anon_dropswap(struct vm_anon *anon)
{
	UVMHIST_FUNC("uvm_anon_dropswap"); UVMHIST_CALLED(maphist);

	if (anon->an_swslot == 0)
		return;

	UVMHIST_LOG(maphist,"freeing swap for anon %p, paged to swslot 0x%x",
		    anon, anon->an_swslot, 0, 0);
	uvm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;
}

#endif

/*
 * uvm_anon_release: release an anon and its page.
 *
 * => anon should not have any references.
 * => anon must be locked.
 */

void
uvm_anon_release(struct vm_anon *anon)
{
	struct vm_page *pg = anon->an_page;
	bool success;

	KASSERT(mutex_owned(anon->an_lock));
	KASSERT(pg != NULL);
	KASSERT((pg->flags & PG_RELEASED) != 0);
	KASSERT((pg->flags & PG_BUSY) != 0);
	KASSERT(pg->uobject == NULL);
	KASSERT(pg->uanon == anon);
	KASSERT(pg->loan_count == 0);
	KASSERT(anon->an_ref == 0);

	mutex_enter(&uvm_pageqlock);
	uvm_pagefree(pg);
	mutex_exit(&uvm_pageqlock);
	KASSERT(anon->an_page == NULL);
	/* dispose should succeed as no one can reach this anon anymore. */
	success = uvm_anon_dispose(anon);
	KASSERT(success);
	mutex_exit(anon->an_lock);
	/* Note: extra reference is held for PG_RELEASED case. */
	mutex_obj_free(anon->an_lock);
	anon->an_lock = NULL;
	uvm_anon_free(anon);
}
