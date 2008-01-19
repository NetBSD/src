/*	$NetBSD: uvm_anon.c,v 1.48.6.2 2008/01/19 12:15:47 bouyer Exp $	*/

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
 */

/*
 * uvm_anon.c: uvm anon ops
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_anon.c,v 1.48.6.2 2008/01/19 12:15:47 bouyer Exp $");

#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>
#include <uvm/uvm_swap.h>
#include <uvm/uvm_pdpolicy.h>

static struct pool_cache uvm_anon_cache;

static int uvm_anon_ctor(void *, void *, int);
static void uvm_anon_dtor(void *, void *);

/*
 * allocate anons
 */
void
uvm_anon_init(void)
{

	pool_cache_bootstrap(&uvm_anon_cache, sizeof(struct vm_anon), 0, 0,
	    PR_LARGECACHE, "anonpl", NULL, IPL_NONE, uvm_anon_ctor,
	    uvm_anon_dtor, NULL);
}

static int
uvm_anon_ctor(void *arg, void *object, int flags)
{
	struct vm_anon *anon = object;

	anon->an_ref = 0;
	mutex_init(&anon->an_lock, MUTEX_DEFAULT, IPL_NONE);
	anon->an_page = NULL;
#if defined(VMSWAP)
	anon->an_swslot = 0;
#endif /* defined(VMSWAP) */

	return 0;
}

static void
uvm_anon_dtor(void *arg, void *object)
{
	struct vm_anon *anon = object;

	mutex_destroy(&anon->an_lock);
}

/*
 * allocate an anon
 *
 * => new anon is returned locked!
 */
struct vm_anon *
uvm_analloc(void)
{
	struct vm_anon *anon;

	anon = pool_cache_get(&uvm_anon_cache, PR_NOWAIT);
	if (anon) {
		KASSERT(anon->an_ref == 0);
		KASSERT(anon->an_page == NULL);
#if defined(VMSWAP)
		KASSERT(anon->an_swslot == 0);
#endif /* defined(VMSWAP) */
		anon->an_ref = 1;
		mutex_enter(&anon->an_lock);
	}
	return anon;
}

/*
 * uvm_anfree: free a single anon structure
 *
 * => caller must remove anon from its amap before calling (if it was in
 *	an amap).
 * => anon must be unlocked and have a zero reference count.
 * => we may lock the pageq's.
 */

void
uvm_anfree(struct vm_anon *anon)
{
	struct vm_page *pg;
	UVMHIST_FUNC("uvm_anfree"); UVMHIST_CALLED(maphist);
	UVMHIST_LOG(maphist,"(anon=0x%x)", anon, 0,0,0);

	KASSERT(anon->an_ref == 0);
	KASSERT(!mutex_owned(&anon->an_lock));

	/*
	 * get page
	 */

	pg = anon->an_page;

	/*
	 * if there is a resident page and it is loaned, then anon may not
	 * own it.   call out to uvm_anon_lockpage() to ensure the real owner
 	 * of the page has been identified and locked.
	 */

	if (pg && pg->loan_count) {
		mutex_enter(&anon->an_lock);
		pg = uvm_anon_lockloanpg(anon);
		mutex_exit(&anon->an_lock);
	}

	/*
	 * if we have a resident page, we must dispose of it before freeing
	 * the anon.
	 */

	if (pg) {

		/*
		 * if the page is owned by a uobject (now locked), then we must
		 * kill the loan on the page rather than free it.
		 */

		if (pg->uobject) {
			mutex_enter(&uvm_pageqlock);
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->uanon = NULL;
			mutex_exit(&uvm_pageqlock);
			mutex_exit(&pg->uobject->vmobjlock);
		} else {

			/*
			 * page has no uobject, so we must be the owner of it.
			 */

			KASSERT((pg->flags & PG_RELEASED) == 0);
			mutex_enter(&anon->an_lock);
			pmap_page_protect(pg, VM_PROT_NONE);

			/*
			 * if the page is busy, mark it as PG_RELEASED
			 * so that uvm_anon_release will release it later.
			 */

			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_RELEASED;
				mutex_exit(&anon->an_lock);
				return;
			}
			mutex_enter(&uvm_pageqlock);
			uvm_pagefree(pg);
			mutex_exit(&uvm_pageqlock);
			mutex_exit(&anon->an_lock);
			UVMHIST_LOG(maphist, "anon 0x%x, page 0x%x: "
				    "freed now!", anon, pg, 0, 0);
		}
	}
#if defined(VMSWAP)
	if (pg == NULL && anon->an_swslot > 0) {
		/* this page is no longer only in swap. */
		mutex_enter(&uvm_swap_data_lock);
		KASSERT(uvmexp.swpgonly > 0);
		uvmexp.swpgonly--;
		mutex_exit(&uvm_swap_data_lock);
	}
#endif /* defined(VMSWAP) */

	/*
	 * free any swap resources.
	 */

	uvm_anon_dropswap(anon);

	/*
	 * give a page replacement hint.
	 */

	uvmpdpol_anfree(anon);

	/*
	 * now that we've stripped the data areas from the anon,
	 * free the anon itself.
	 */

	KASSERT(anon->an_page == NULL);
#if defined(VMSWAP)
	KASSERT(anon->an_swslot == 0);
#endif /* defined(VMSWAP) */

	pool_cache_put(&uvm_anon_cache, anon);
	UVMHIST_LOG(maphist,"<- done!",0,0,0,0);
}

#if defined(VMSWAP)

/*
 * uvm_anon_dropswap:  release any swap resources from this anon.
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

#endif /* defined(VMSWAP) */

/*
 * uvm_anon_lockloanpg: given a locked anon, lock its resident page
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
	bool locked = false;

	KASSERT(mutex_owned(&anon->an_lock));

	/*
	 * loop while we have a resident page that has a non-zero loan count.
	 * if we successfully get our lock, we will "break" the loop.
	 * note that the test for pg->loan_count is not protected -- this
	 * may produce false positive results.   note that a false positive
	 * result may cause us to do more work than we need to, but it will
	 * not produce an incorrect result.
	 */

	while (((pg = anon->an_page) != NULL) && pg->loan_count != 0) {

		/*
		 * quickly check to see if the page has an object before
		 * bothering to lock the page queues.   this may also produce
		 * a false positive result, but that's ok because we do a real
		 * check after that.
		 */

		if (pg->uobject) {
			mutex_enter(&uvm_pageqlock);
			if (pg->uobject) {
				locked =
				    mutex_tryenter(&pg->uobject->vmobjlock);
			} else {
				/* object disowned before we got PQ lock */
				locked = true;
			}
			mutex_exit(&uvm_pageqlock);

			/*
			 * if we didn't get a lock (try lock failed), then we
			 * toggle our anon lock and try again
			 */

			if (!locked) {
				mutex_exit(&anon->an_lock);

				/*
				 * someone locking the object has a chance to
				 * lock us right now
				 */
				/* XXX Better than yielding but inadequate. */
				kpause("livelock", false, 1, NULL);

				mutex_enter(&anon->an_lock);
				continue;
			}
		}

		/*
		 * if page is un-owned [i.e. the object dropped its ownership],
		 * then we can take over as owner!
		 */

		if (pg->uobject == NULL && (pg->pqflags & PQ_ANON) == 0) {
			mutex_enter(&uvm_pageqlock);
			pg->pqflags |= PQ_ANON;
			pg->loan_count--;
			mutex_exit(&uvm_pageqlock);
		}
		break;
	}
	return(pg);
}

#if defined(VMSWAP)

/*
 * fetch an anon's page.
 *
 * => anon must be locked, and is unlocked upon return.
 * => returns true if pagein was aborted due to lack of memory.
 */

bool
uvm_anon_pagein(struct vm_anon *anon)
{
	struct vm_page *pg;
	struct uvm_object *uobj;
	int rv;

	/* locked: anon */
	KASSERT(mutex_owned(&anon->an_lock));

	rv = uvmfault_anonget(NULL, NULL, anon);

	/*
	 * if rv == 0, anon is still locked, else anon
	 * is unlocked
	 */

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

	pg = anon->an_page;
	uobj = pg->uobject;
	if (anon->an_swslot > 0)
		uvm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;
	pg->flags &= ~(PG_CLEAN);

	/*
	 * deactivate the page (to put it on a page queue)
	 */

	mutex_enter(&uvm_pageqlock);
	if (pg->wire_count == 0)
		uvm_pagedeactivate(pg);
	mutex_exit(&uvm_pageqlock);

	if (pg->flags & PG_WANTED) {
		wakeup(pg);
		pg->flags &= ~(PG_WANTED);
	}

	/*
	 * unlock the anon and we're done.
	 */

	mutex_exit(&anon->an_lock);
	if (uobj) {
		mutex_exit(&uobj->vmobjlock);
	}
	return false;
}

#endif /* defined(VMSWAP) */

/*
 * uvm_anon_release: release an anon and its page.
 *
 * => caller must lock the anon.
 */

void
uvm_anon_release(struct vm_anon *anon)
{
	struct vm_page *pg = anon->an_page;

	KASSERT(mutex_owned(&anon->an_lock));
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
	mutex_exit(&anon->an_lock);

	KASSERT(anon->an_page == NULL);

	uvm_anfree(anon);
}
