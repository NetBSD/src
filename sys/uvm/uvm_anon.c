/*	$NetBSD: uvm_anon.c,v 1.80 2020/10/25 00:05:26 chs Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uvm_anon.c,v 1.80 2020/10/25 00:05:26 chs Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/kernel.h>
#include <sys/atomic.h>

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
 * uvm_anfree: free a single anon structure
 *
 * => anon must be removed from the amap (if anon was in an amap).
 * => amap must be locked, if anon was owned by amap.
 * => we may drop and re-acquire the lock here (to break loans).
 */
void
uvm_anfree(struct vm_anon *anon)
{
	struct vm_page *pg = anon->an_page, *pg2 __diagused;

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLARGS(maphist,"(anon=%#jx)", (uintptr_t)anon, 0,0,0);

	KASSERT(anon->an_lock == NULL || rw_write_held(anon->an_lock));
	KASSERT(anon->an_ref == 0);

	/*
	 * Dispose of the page, if it is resident.
	 */

	if (__predict_true(pg != NULL)) {
		KASSERT(anon->an_lock != NULL);

		/*
		 * If there is a resident page and it is loaned, then anon
		 * may not own it.  Call out to uvm_anon_lockloanpg() to
		 * identify and lock the real owner of the page.
		 */

		if (__predict_false(pg->loan_count != 0)) {
			pg2 = uvm_anon_lockloanpg(anon);
			KASSERT(pg2 == pg);
		}

		/*
		 * If the page is owned by a UVM object (now locked),
		 * then kill the loan on the page rather than free it,
		 * and release the object lock.
		 */

		if (__predict_false(pg->uobject != NULL)) {
			mutex_enter(&pg->interlock);
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->uanon = NULL;
			mutex_exit(&pg->interlock);
			rw_exit(pg->uobject->vmobjlock);
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

			if (__predict_false((pg->flags & PG_BUSY) != 0)) {
				pg->flags |= PG_RELEASED;
				rw_obj_hold(anon->an_lock);
				return;
			}
			uvm_pagefree(pg);
			UVMHIST_LOG(maphist, "anon %#jx, page %#jx: "
			    "freed now!", (uintptr_t)anon, (uintptr_t)pg,
			    0, 0);
		}
	} else {
#if defined(VMSWAP)
		if (anon->an_swslot > 0) {
			/* This page is no longer only in swap. */
			KASSERT(uvmexp.swpgonly > 0);
			atomic_dec_uint(&uvmexp.swpgonly);
		}
#endif
	}
	anon->an_lock = NULL;

	/*
	 * Free any swap resources, leave a page replacement hint.
	 */

	uvm_anon_dropswap(anon);
	uvmpdpol_anfree(anon);
	UVMHIST_LOG(maphist,"<- done!",0,0,0,0);
	pool_cache_put(&uvm_anon_cache, anon);
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
	krw_t op;

	KASSERT(rw_lock_held(anon->an_lock));

	/*
	 * loop while we have a resident page that has a non-zero loan count.
	 * if we successfully get our lock, we will "break" the loop.
	 * note that the test for pg->loan_count is not protected -- this
	 * may produce false positive results.   note that a false positive
	 * result may cause us to do more work than we need to, but it will
	 * not produce an incorrect result.
	 */

	while (((pg = anon->an_page) != NULL) && pg->loan_count != 0) {
		mutex_enter(&pg->interlock);
		if (pg->uobject) {
			/*
			 * if we didn't get a lock (try lock failed), then we
			 * toggle our anon lock and try again
			 */

			if (!rw_tryenter(pg->uobject->vmobjlock, RW_WRITER)) {
				/*
				 * someone locking the object has a chance to
				 * lock us right now
				 *
				 * XXX Better than yielding but inadequate.
				 */
				mutex_exit(&pg->interlock);
				op = rw_lock_op(anon->an_lock);
				rw_exit(anon->an_lock);
				kpause("lkloanpg", false, 1, NULL);
				rw_enter(anon->an_lock, op);
				continue;
			}
		}

		/*
		 * If page is un-owned i.e. the object dropped its ownership,
		 * then we have to take the ownership.
		 */

		if (pg->uobject == NULL && (pg->flags & PG_ANON) == 0) {
			pg->flags |= PG_ANON;
			pg->loan_count--;
		}
		mutex_exit(&pg->interlock);
		break;
	}
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

	KASSERT(rw_write_held(anon->an_lock));
	KASSERT(anon->an_lock == amap->am_lock);

	/*
	 * Get the page of the anon.
	 */

	switch (uvmfault_anonget(NULL, amap, anon)) {
	case 0:
		/* Success - we have the page. */
		KASSERT(rw_write_held(anon->an_lock));
		break;
	case EIO:
	case ERESTART:
		/*
		 * Nothing more to do on errors.  ERESTART means that the
		 * anon was freed.
		 */
		return false;
	case ENOLCK:
		panic("uvm_anon_pagein");
	default:
		return true;
	}

	/*
	 * Mark the page as dirty and clear its swslot.
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

	uvm_pagelock(pg);
	uvm_pagedeactivate(pg);
	uvm_pageunlock(pg);
	rw_exit(anon->an_lock);
	if (uobj) {
		rw_exit(uobj->vmobjlock);
	}
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
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(maphist);

	if (anon->an_swslot == 0)
		return;

	UVMHIST_LOG(maphist,"freeing swap for anon %#jx, paged to swslot %#jx",
		    (uintptr_t)anon, anon->an_swslot, 0, 0);
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
	krwlock_t *lock;

	KASSERT(rw_write_held(anon->an_lock));
	KASSERT(pg != NULL);
	KASSERT((pg->flags & PG_RELEASED) != 0);
	KASSERT((pg->flags & PG_BUSY) != 0);
	KASSERT(pg->uobject == NULL);
	KASSERT(pg->uanon == anon);
	KASSERT(pg->loan_count == 0);
	KASSERT(anon->an_ref == 0);

	if ((pg->flags & PG_PAGEOUT) != 0) {
		pg->flags &= ~PG_PAGEOUT;
		uvm_pageout_done(1);
	}

	uvm_pagefree(pg);
	KASSERT(anon->an_page == NULL);
	lock = anon->an_lock;
	uvm_anfree(anon);
	rw_exit(lock);
	/* Note: extra reference is held for PG_RELEASED case. */
	rw_obj_free(lock);
}
