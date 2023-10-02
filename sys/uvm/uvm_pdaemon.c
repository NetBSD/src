/*	$NetBSD: uvm_pdaemon.c,v 1.133.16.1 2023/10/02 13:01:46 martin Exp $	*/

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
 *	@(#)vm_pageout.c        8.5 (Berkeley) 2/14/94
 * from: Id: uvm_pdaemon.c,v 1.1.2.32 1998/02/06 05:26:30 chs Exp
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
 * uvm_pdaemon.c: the page daemon
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pdaemon.c,v 1.133.16.1 2023/10/02 13:01:46 martin Exp $");

#include "opt_uvmhist.h"
#include "opt_readahead.h"

#define	__RWLOCK_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/buf.h>
#include <sys/module.h>
#include <sys/atomic.h>
#include <sys/kthread.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>
#include <uvm/uvm_pgflcache.h>

#ifdef UVMHIST
#ifndef UVMHIST_PDHIST_SIZE
#define UVMHIST_PDHIST_SIZE 100
#endif
static struct kern_history_ent pdhistbuf[UVMHIST_PDHIST_SIZE];
UVMHIST_DEFINE(pdhist) = UVMHIST_INITIALIZER(pdhisthist, pdhistbuf);
#endif

/*
 * UVMPD_NUMDIRTYREACTS is how many dirty pages the pagedaemon will reactivate
 * in a pass thru the inactive list when swap is full.  the value should be
 * "small"... if it's too large we'll cycle the active pages thru the inactive
 * queue too quickly to for them to be referenced and avoid being freed.
 */

#define	UVMPD_NUMDIRTYREACTS	16

/*
 * local prototypes
 */

static void	uvmpd_scan(void);
static void	uvmpd_scan_queue(void);
static void	uvmpd_tune(void);
static void	uvmpd_pool_drain_thread(void *);
static void	uvmpd_pool_drain_wakeup(void);

static unsigned int uvm_pagedaemon_waiters;

/* State for the pool drainer thread */
static kmutex_t uvmpd_lock __cacheline_aligned;
static kcondvar_t uvmpd_pool_drain_cv;
static bool uvmpd_pool_drain_run = false;

/*
 * XXX hack to avoid hangs when large processes fork.
 */
u_int uvm_extrapages;

/*
 * uvm_wait: wait (sleep) for the page daemon to free some pages
 *
 * => should be called with all locks released
 * => should _not_ be called by the page daemon (to avoid deadlock)
 */

void
uvm_wait(const char *wmsg)
{
	int timo = 0;

	if (uvm.pagedaemon_lwp == NULL)
		panic("out of memory before the pagedaemon thread exists");

	mutex_spin_enter(&uvmpd_lock);

	/*
	 * check for page daemon going to sleep (waiting for itself)
	 */

	if (curlwp == uvm.pagedaemon_lwp && uvmexp.paging == 0) {
		/*
		 * now we have a problem: the pagedaemon wants to go to
		 * sleep until it frees more memory.   but how can it
		 * free more memory if it is asleep?  that is a deadlock.
		 * we have two options:
		 *  [1] panic now
		 *  [2] put a timeout on the sleep, thus causing the
		 *      pagedaemon to only pause (rather than sleep forever)
		 *
		 * note that option [2] will only help us if we get lucky
		 * and some other process on the system breaks the deadlock
		 * by exiting or freeing memory (thus allowing the pagedaemon
		 * to continue).  for now we panic if DEBUG is defined,
		 * otherwise we hope for the best with option [2] (better
		 * yet, this should never happen in the first place!).
		 */

		printf("pagedaemon: deadlock detected!\n");
		timo = hz >> 3;		/* set timeout */
#if defined(DEBUG)
		/* DEBUG: panic so we can debug it */
		panic("pagedaemon deadlock");
#endif
	}

	uvm_pagedaemon_waiters++;
	wakeup(&uvm.pagedaemon);		/* wake the daemon! */
	UVM_UNLOCK_AND_WAIT(&uvmexp.free, &uvmpd_lock, false, wmsg, timo);
}

/*
 * uvm_kick_pdaemon: perform checks to determine if we need to
 * give the pagedaemon a nudge, and do so if necessary.
 */

void
uvm_kick_pdaemon(void)
{
	int fpages = uvm_availmem(false);

	if (fpages + uvmexp.paging < uvmexp.freemin ||
	    (fpages + uvmexp.paging < uvmexp.freetarg &&
	     uvmpdpol_needsscan_p()) ||
	     uvm_km_va_starved_p()) {
	     	mutex_spin_enter(&uvmpd_lock);
		wakeup(&uvm.pagedaemon);
	     	mutex_spin_exit(&uvmpd_lock);
	}
}

/*
 * uvmpd_tune: tune paging parameters
 *
 * => called when ever memory is added (or removed?) to the system
 */

static void
uvmpd_tune(void)
{
	int val;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pdhist);

	/*
	 * try to keep 0.5% of available RAM free, but limit to between
	 * 128k and 1024k per-CPU.  XXX: what are these values good for?
	 */
	val = uvmexp.npages / 200;
	val = MAX(val, (128*1024) >> PAGE_SHIFT);
	val = MIN(val, (1024*1024) >> PAGE_SHIFT);
	val *= ncpu;

	/* Make sure there's always a user page free. */
	if (val < uvmexp.reserve_kernel + 1)
		val = uvmexp.reserve_kernel + 1;
	uvmexp.freemin = val;

	/* Calculate free target. */
	val = (uvmexp.freemin * 4) / 3;
	if (val <= uvmexp.freemin)
		val = uvmexp.freemin + 1;
	uvmexp.freetarg = val + atomic_swap_uint(&uvm_extrapages, 0);

	uvmexp.wiredmax = uvmexp.npages / 3;
	UVMHIST_LOG(pdhist, "<- done, freemin=%jd, freetarg=%jd, wiredmax=%jd",
	      uvmexp.freemin, uvmexp.freetarg, uvmexp.wiredmax, 0);
}

/*
 * uvm_pageout: the main loop for the pagedaemon
 */

void
uvm_pageout(void *arg)
{
	int npages = 0;
	int extrapages = 0;
	int fpages;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pdhist);

	UVMHIST_LOG(pdhist,"<starting uvm pagedaemon>", 0, 0, 0, 0);

	mutex_init(&uvmpd_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&uvmpd_pool_drain_cv, "pooldrain");

	/* Create the pool drainer kernel thread. */
	if (kthread_create(PRI_VM, KTHREAD_MPSAFE, NULL,
	    uvmpd_pool_drain_thread, NULL, NULL, "pooldrain"))
		panic("fork pooldrain");

	/*
	 * ensure correct priority and set paging parameters...
	 */

	uvm.pagedaemon_lwp = curlwp;
	npages = uvmexp.npages;
	uvmpd_tune();

	/*
	 * main loop
	 */

	for (;;) {
		bool needsscan, needsfree, kmem_va_starved;

		kmem_va_starved = uvm_km_va_starved_p();

		mutex_spin_enter(&uvmpd_lock);
		if ((uvm_pagedaemon_waiters == 0 || uvmexp.paging > 0) &&
		    !kmem_va_starved) {
			UVMHIST_LOG(pdhist,"  <<SLEEPING>>",0,0,0,0);
			UVM_UNLOCK_AND_WAIT(&uvm.pagedaemon,
			    &uvmpd_lock, false, "pgdaemon", 0);
			uvmexp.pdwoke++;
			UVMHIST_LOG(pdhist,"  <<WOKE UP>>",0,0,0,0);
		} else {
			mutex_spin_exit(&uvmpd_lock);
		}

		/*
		 * now recompute inactive count
		 */

		if (npages != uvmexp.npages || extrapages != uvm_extrapages) {
			npages = uvmexp.npages;
			extrapages = uvm_extrapages;
			uvmpd_tune();
		}

		uvmpdpol_tune();

		/*
		 * Estimate a hint.  Note that bufmem are returned to
		 * system only when entire pool page is empty.
		 */
		fpages = uvm_availmem(false);
		UVMHIST_LOG(pdhist,"  free/ftarg=%jd/%jd",
		    fpages, uvmexp.freetarg, 0,0);

		needsfree = fpages + uvmexp.paging < uvmexp.freetarg;
		needsscan = needsfree || uvmpdpol_needsscan_p();

		/*
		 * scan if needed
		 */
		if (needsscan) {
			uvmpd_scan();
		}

		/*
		 * if there's any free memory to be had,
		 * wake up any waiters.
		 */
		if (uvm_availmem(false) > uvmexp.reserve_kernel ||
		    uvmexp.paging == 0) {
			mutex_spin_enter(&uvmpd_lock);
			wakeup(&uvmexp.free);
			uvm_pagedaemon_waiters = 0;
			mutex_spin_exit(&uvmpd_lock);
		}

		/*
		 * scan done.  if we don't need free memory, we're done.
		 */

		if (!needsfree && !kmem_va_starved)
			continue;

		/*
		 * kick the pool drainer thread.
		 */

		uvmpd_pool_drain_wakeup();
	}
	/*NOTREACHED*/
}

void
uvm_pageout_start(int npages)
{

	atomic_add_int(&uvmexp.paging, npages);
}

void
uvm_pageout_done(int npages)
{

	KASSERT(atomic_load_relaxed(&uvmexp.paging) >= npages);

	if (npages == 0) {
		return;
	}

	atomic_add_int(&uvmexp.paging, -npages);

	/*
	 * wake up either of pagedaemon or LWPs waiting for it.
	 */

	mutex_spin_enter(&uvmpd_lock);
	if (uvm_availmem(false) <= uvmexp.reserve_kernel) {
		wakeup(&uvm.pagedaemon);
	} else if (uvm_pagedaemon_waiters != 0) {
		wakeup(&uvmexp.free);
		uvm_pagedaemon_waiters = 0;
	}
	mutex_spin_exit(&uvmpd_lock);
}

static krwlock_t *
uvmpd_page_owner_lock(struct vm_page *pg)
{
	struct uvm_object *uobj = pg->uobject;
	struct vm_anon *anon = pg->uanon;
	krwlock_t *slock;

	KASSERT(mutex_owned(&pg->interlock));

#ifdef DEBUG
	if (uobj == (void *)0xdeadbeef || anon == (void *)0xdeadbeef) {
		return NULL;
	}
#endif
	if (uobj != NULL) {
		slock = uobj->vmobjlock;
		KASSERTMSG(slock != NULL, "pg %p uobj %p, NULL lock", pg, uobj);
	} else if (anon != NULL) {
		slock = anon->an_lock;
		KASSERTMSG(slock != NULL, "pg %p anon %p, NULL lock", pg, anon);
	} else {
		slock = NULL;
	}
	return slock;
}

/*
 * uvmpd_trylockowner: trylock the page's owner.
 *
 * => called with page interlock held.
 * => resolve orphaned O->A loaned page.
 * => return the locked mutex on success.  otherwise, return NULL.
 */

krwlock_t *
uvmpd_trylockowner(struct vm_page *pg)
{
	krwlock_t *slock, *heldslock = NULL;

	KASSERT(mutex_owned(&pg->interlock));

	slock = uvmpd_page_owner_lock(pg);
	if (slock == NULL) {
		/* Page may be in state of flux - ignore. */
		mutex_exit(&pg->interlock);
		return NULL;
	}

	if (rw_tryenter(slock, RW_WRITER)) {
		goto success;
	}

	/*
	 * The try-lock didn't work, so now do a blocking lock after
	 * dropping the page interlock.  Prevent the owner lock from
	 * being freed by taking a hold on it first.
	 */

	rw_obj_hold(slock);
	mutex_exit(&pg->interlock);
	rw_enter(slock, RW_WRITER);
	heldslock = slock;

	/*
	 * Now we hold some owner lock.  Check if the lock we hold
	 * is still the lock for the owner of the page.
	 * If it is then return it, otherwise release it and return NULL.
	 */

	mutex_enter(&pg->interlock);
	slock = uvmpd_page_owner_lock(pg);
	if (heldslock != slock) {
		rw_exit(heldslock);
		slock = NULL;
	} else {
success:
		/*
		 * Set PG_ANON if it isn't set already.
		 */
		if (pg->uobject == NULL && (pg->flags & PG_ANON) == 0) {
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->flags |= PG_ANON;
			/* anon now owns it */
		}
	}
	mutex_exit(&pg->interlock);
	if (heldslock != NULL) {
		rw_obj_free(heldslock);
	}
	return slock;
}

#if defined(VMSWAP)
struct swapcluster {
	int swc_slot;
	int swc_nallocated;
	int swc_nused;
	struct vm_page *swc_pages[howmany(MAXPHYS, MIN_PAGE_SIZE)];
};

static void
swapcluster_init(struct swapcluster *swc)
{

	swc->swc_slot = 0;
	swc->swc_nused = 0;
}

static int
swapcluster_allocslots(struct swapcluster *swc)
{
	int slot;
	int npages;

	if (swc->swc_slot != 0) {
		return 0;
	}

	/* Even with strange MAXPHYS, the shift
	   implicitly rounds down to a page. */
	npages = MAXPHYS >> PAGE_SHIFT;
	slot = uvm_swap_alloc(&npages, true);
	if (slot == 0) {
		return ENOMEM;
	}
	swc->swc_slot = slot;
	swc->swc_nallocated = npages;
	swc->swc_nused = 0;

	return 0;
}

static int
swapcluster_add(struct swapcluster *swc, struct vm_page *pg)
{
	int slot;
	struct uvm_object *uobj;

	KASSERT(swc->swc_slot != 0);
	KASSERT(swc->swc_nused < swc->swc_nallocated);
	KASSERT((pg->flags & PG_SWAPBACKED) != 0);

	slot = swc->swc_slot + swc->swc_nused;
	uobj = pg->uobject;
	if (uobj == NULL) {
		KASSERT(rw_write_held(pg->uanon->an_lock));
		pg->uanon->an_swslot = slot;
	} else {
		int result;

		KASSERT(rw_write_held(uobj->vmobjlock));
		result = uao_set_swslot(uobj, pg->offset >> PAGE_SHIFT, slot);
		if (result == -1) {
			return ENOMEM;
		}
	}
	swc->swc_pages[swc->swc_nused] = pg;
	swc->swc_nused++;

	return 0;
}

static void
swapcluster_flush(struct swapcluster *swc, bool now)
{
	int slot;
	int nused;
	int nallocated;
	int error __diagused;

	if (swc->swc_slot == 0) {
		return;
	}
	KASSERT(swc->swc_nused <= swc->swc_nallocated);

	slot = swc->swc_slot;
	nused = swc->swc_nused;
	nallocated = swc->swc_nallocated;

	/*
	 * if this is the final pageout we could have a few
	 * unused swap blocks.  if so, free them now.
	 */

	if (nused < nallocated) {
		if (!now) {
			return;
		}
		uvm_swap_free(slot + nused, nallocated - nused);
	}

	/*
	 * now start the pageout.
	 */

	if (nused > 0) {
		uvmexp.pdpageouts++;
		uvm_pageout_start(nused);
		error = uvm_swap_put(slot, swc->swc_pages, nused, 0);
		KASSERT(error == 0 || error == ENOMEM);
	}

	/*
	 * zero swslot to indicate that we are
	 * no longer building a swap-backed cluster.
	 */

	swc->swc_slot = 0;
	swc->swc_nused = 0;
}

static int
swapcluster_nused(struct swapcluster *swc)
{

	return swc->swc_nused;
}

/*
 * uvmpd_dropswap: free any swap allocated to this page.
 *
 * => called with owner locked.
 * => return true if a page had an associated slot.
 */

bool
uvmpd_dropswap(struct vm_page *pg)
{
	bool result = false;
	struct vm_anon *anon = pg->uanon;

	if ((pg->flags & PG_ANON) && anon->an_swslot) {
		uvm_swap_free(anon->an_swslot, 1);
		anon->an_swslot = 0;
		uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
		result = true;
	} else if (pg->flags & PG_AOBJ) {
		int slot = uao_set_swslot(pg->uobject,
		    pg->offset >> PAGE_SHIFT, 0);
		if (slot) {
			uvm_swap_free(slot, 1);
			uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
			result = true;
		}
	}

	return result;
}

#endif /* defined(VMSWAP) */

/*
 * uvmpd_scan_queue: scan an replace candidate list for pages
 * to clean or free.
 *
 * => we work on meeting our free target by converting inactive pages
 *    into free pages.
 * => we handle the building of swap-backed clusters
 */

static void
uvmpd_scan_queue(void)
{
	struct vm_page *p;
	struct uvm_object *uobj;
	struct vm_anon *anon;
#if defined(VMSWAP)
	struct swapcluster swc;
#endif /* defined(VMSWAP) */
	int dirtyreacts;
	krwlock_t *slock;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pdhist);

	/*
	 * swslot is non-zero if we are building a swap cluster.  we want
	 * to stay in the loop while we have a page to scan or we have
	 * a swap-cluster to build.
	 */

#if defined(VMSWAP)
	swapcluster_init(&swc);
#endif /* defined(VMSWAP) */

	dirtyreacts = 0;
	uvmpdpol_scaninit();

	while (/* CONSTCOND */ 1) {

		/*
		 * see if we've met the free target.
		 */

		if (uvm_availmem(false) + uvmexp.paging
#if defined(VMSWAP)
		    + swapcluster_nused(&swc)
#endif /* defined(VMSWAP) */
		    >= uvmexp.freetarg << 2 ||
		    dirtyreacts == UVMPD_NUMDIRTYREACTS) {
			UVMHIST_LOG(pdhist,"  met free target: "
				    "exit loop", 0, 0, 0, 0);
			break;
		}

		/*
		 * first we have the pdpolicy select a victim page
		 * and attempt to lock the object that the page
		 * belongs to.  if our attempt fails we skip on to
		 * the next page (no harm done).  it is important to
		 * "try" locking the object as we are locking in the
		 * wrong order (pageq -> object) and we don't want to
		 * deadlock.
		 *
		 * the only time we expect to see an ownerless page
		 * (i.e. a page with no uobject and !PG_ANON) is if an
		 * anon has loaned a page from a uvm_object and the
		 * uvm_object has dropped the ownership.  in that
		 * case, the anon can "take over" the loaned page
		 * and make it its own.
		 */

		p = uvmpdpol_selectvictim(&slock);
		if (p == NULL) {
			break;
		}
		KASSERT(uvmpdpol_pageisqueued_p(p));
		KASSERT(uvm_page_owner_locked_p(p, true));
		KASSERT(p->wire_count == 0);

		/*
		 * we are below target and have a new page to consider.
		 */

		anon = p->uanon;
		uobj = p->uobject;

		if (p->flags & PG_BUSY) {
			rw_exit(slock);
			uvmexp.pdbusy++;
			continue;
		}

		/* does the page belong to an object? */
		if (uobj != NULL) {
			uvmexp.pdobscan++;
		} else {
#if defined(VMSWAP)
			KASSERT(anon != NULL);
			uvmexp.pdanscan++;
#else /* defined(VMSWAP) */
			panic("%s: anon", __func__);
#endif /* defined(VMSWAP) */
		}


		/*
		 * we now have the object locked.
		 * if the page is not swap-backed, call the object's
		 * pager to flush and free the page.
		 */

#if defined(READAHEAD_STATS)
		if ((p->flags & PG_READAHEAD) != 0) {
			p->flags &= ~PG_READAHEAD;
			uvm_ra_miss.ev_count++;
		}
#endif /* defined(READAHEAD_STATS) */

		if ((p->flags & PG_SWAPBACKED) == 0) {
			KASSERT(uobj != NULL);
			(void) (uobj->pgops->pgo_put)(uobj, p->offset,
			    p->offset + PAGE_SIZE, PGO_CLEANIT|PGO_FREE);
			continue;
		}

		/*
		 * the page is swap-backed.  remove all the permissions
		 * from the page so we can sync the modified info
		 * without any race conditions.  if the page is clean
		 * we can free it now and continue.
		 */

		pmap_page_protect(p, VM_PROT_NONE);
		if (uvm_pagegetdirty(p) == UVM_PAGE_STATUS_UNKNOWN) {
			if (pmap_clear_modify(p)) {
				uvm_pagemarkdirty(p, UVM_PAGE_STATUS_DIRTY);
			} else {
				uvm_pagemarkdirty(p, UVM_PAGE_STATUS_CLEAN);
			}
		}
		if (uvm_pagegetdirty(p) != UVM_PAGE_STATUS_DIRTY) {
			int slot;
			int pageidx;

			pageidx = p->offset >> PAGE_SHIFT;
			uvm_pagefree(p);
			atomic_inc_uint(&uvmexp.pdfreed);

			/*
			 * for anons, we need to remove the page
			 * from the anon ourselves.  for aobjs,
			 * pagefree did that for us.
			 */

			if (anon) {
				KASSERT(anon->an_swslot != 0);
				anon->an_page = NULL;
				slot = anon->an_swslot;
			} else {
				slot = uao_find_swslot(uobj, pageidx);
			}
			if (slot > 0) {
				/* this page is now only in swap. */
				KASSERT(uvmexp.swpgonly < uvmexp.swpginuse);
				atomic_inc_uint(&uvmexp.swpgonly);
			}
			rw_exit(slock);
			continue;
		}

#if defined(VMSWAP)
		/*
		 * this page is dirty, skip it if we'll have met our
		 * free target when all the current pageouts complete.
		 */

		if (uvm_availmem(false) + uvmexp.paging >
		    uvmexp.freetarg << 2) {
			rw_exit(slock);
			continue;
		}

		/*
		 * free any swap space allocated to the page since
		 * we'll have to write it again with its new data.
		 */

		uvmpd_dropswap(p);

		/*
		 * start new swap pageout cluster (if necessary).
		 *
		 * if swap is full reactivate this page so that
		 * we eventually cycle all pages through the
		 * inactive queue.
		 */

		if (swapcluster_allocslots(&swc)) {
			dirtyreacts++;
			uvm_pagelock(p);
			uvm_pageactivate(p);
			uvm_pageunlock(p);
			rw_exit(slock);
			continue;
		}

		/*
		 * at this point, we're definitely going reuse this
		 * page.  mark the page busy and delayed-free.
		 * we should remove the page from the page queues
		 * so we don't ever look at it again.
		 * adjust counters and such.
		 */

		p->flags |= PG_BUSY;
		UVM_PAGE_OWN(p, "scan_queue");
		p->flags |= PG_PAGEOUT;
		uvmexp.pgswapout++;

		uvm_pagelock(p);
		uvm_pagedequeue(p);
		uvm_pageunlock(p);

		/*
		 * add the new page to the cluster.
		 */

		if (swapcluster_add(&swc, p)) {
			p->flags &= ~(PG_BUSY|PG_PAGEOUT);
			UVM_PAGE_OWN(p, NULL);
			dirtyreacts++;
			uvm_pagelock(p);
			uvm_pageactivate(p);
			uvm_pageunlock(p);
			rw_exit(slock);
			continue;
		}
		rw_exit(slock);

		swapcluster_flush(&swc, false);

		/*
		 * the pageout is in progress.  bump counters and set up
		 * for the next loop.
		 */

		atomic_inc_uint(&uvmexp.pdpending);

#else /* defined(VMSWAP) */
		uvm_pagelock(p);
		uvm_pageactivate(p);
		uvm_pageunlock(p);
		rw_exit(slock);
#endif /* defined(VMSWAP) */
	}

	uvmpdpol_scanfini();

#if defined(VMSWAP)
	swapcluster_flush(&swc, true);
#endif /* defined(VMSWAP) */
}

/*
 * uvmpd_scan: scan the page queues and attempt to meet our targets.
 */

static void
uvmpd_scan(void)
{
	int swap_shortage, pages_freed, fpages;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pdhist);

	uvmexp.pdrevs++;

	/*
	 * work on meeting our targets.   first we work on our free target
	 * by converting inactive pages into free pages.  then we work on
	 * meeting our inactive target by converting active pages to
	 * inactive ones.
	 */

	UVMHIST_LOG(pdhist, "  starting 'free' loop",0,0,0,0);

	pages_freed = uvmexp.pdfreed;
	uvmpd_scan_queue();
	pages_freed = uvmexp.pdfreed - pages_freed;

	/*
	 * detect if we're not going to be able to page anything out
	 * until we free some swap resources from active pages.
	 */

	swap_shortage = 0;
	fpages = uvm_availmem(false);
	if (fpages < uvmexp.freetarg &&
	    uvmexp.swpginuse >= uvmexp.swpgavail &&
	    !uvm_swapisfull() &&
	    pages_freed == 0) {
		swap_shortage = uvmexp.freetarg - fpages;
	}

	uvmpdpol_balancequeue(swap_shortage);

	/*
	 * if still below the minimum target, try unloading kernel
	 * modules.
	 */

	if (uvm_availmem(false) < uvmexp.freemin) {
		module_thread_kick();
	}
}

/*
 * uvm_reclaimable: decide whether to wait for pagedaemon.
 *
 * => return true if it seems to be worth to do uvm_wait.
 *
 * XXX should be tunable.
 * XXX should consider pools, etc?
 */

bool
uvm_reclaimable(void)
{
	int filepages;
	int active, inactive;

	/*
	 * if swap is not full, no problem.
	 */

	if (!uvm_swapisfull()) {
		return true;
	}

	/*
	 * file-backed pages can be reclaimed even when swap is full.
	 * if we have more than 1/16 of pageable memory or 5MB, try to reclaim.
	 * NB: filepages calculation does not exclude EXECPAGES - intentional.
	 *
	 * XXX assume the worst case, ie. all wired pages are file-backed.
	 *
	 * XXX should consider about other reclaimable memory.
	 * XXX ie. pools, traditional buffer cache.
	 */

	cpu_count_sync(false);
	filepages = (int)(cpu_count_get(CPU_COUNT_FILECLEAN) +
	    cpu_count_get(CPU_COUNT_FILEUNKNOWN) +
	    cpu_count_get(CPU_COUNT_FILEDIRTY) - uvmexp.wired);
	uvm_estimatepageable(&active, &inactive);
	if (filepages >= MIN((active + inactive) >> 4,
	    5 * 1024 * 1024 >> PAGE_SHIFT)) {
		return true;
	}

	/*
	 * kill the process, fail allocation, etc..
	 */

	return false;
}

void
uvm_estimatepageable(int *active, int *inactive)
{

	uvmpdpol_estimatepageable(active, inactive);
}


/*
 * Use a separate thread for draining pools.
 * This work can't done from the main pagedaemon thread because
 * some pool allocators need to take vm_map locks.
 */

static void
uvmpd_pool_drain_thread(void *arg)
{
	struct pool *firstpool, *curpool;
	int bufcnt, lastslept;
	bool cycled;

	firstpool = NULL;
	cycled = true;
	for (;;) {
		/*
		 * sleep until awoken by the pagedaemon.
		 */
		mutex_enter(&uvmpd_lock);
		if (!uvmpd_pool_drain_run) {
			lastslept = getticks();
			cv_wait(&uvmpd_pool_drain_cv, &uvmpd_lock);
			if (getticks() != lastslept) {
				cycled = false;
				firstpool = NULL;
			}
		}
		uvmpd_pool_drain_run = false;
		mutex_exit(&uvmpd_lock);

		/*
		 * rate limit draining, otherwise in desperate circumstances
		 * this can totally saturate the system with xcall activity.
		 */
		if (cycled) {
			kpause("uvmpdlmt", false, 1, NULL);
			cycled = false;
			firstpool = NULL;
		}

		/*
		 * drain and temporarily disable the freelist cache.
		 */
		uvm_pgflcache_pause();

		/*
		 * kill unused metadata buffers.
		 */
		bufcnt = uvmexp.freetarg - uvm_availmem(false);
		if (bufcnt < 0)
			bufcnt = 0;

		mutex_enter(&bufcache_lock);
		buf_drain(bufcnt << PAGE_SHIFT);
		mutex_exit(&bufcache_lock);

		/*
		 * drain a pool, and then re-enable the freelist cache.
		 */
		(void)pool_drain(&curpool);
		KASSERT(curpool != NULL);
		if (firstpool == NULL) {
			firstpool = curpool;
		} else if (firstpool == curpool) {
			cycled = true;
		}
		uvm_pgflcache_resume();
	}
	/*NOTREACHED*/
}

static void
uvmpd_pool_drain_wakeup(void)
{

	mutex_enter(&uvmpd_lock);
	uvmpd_pool_drain_run = true;
	cv_signal(&uvmpd_pool_drain_cv);
	mutex_exit(&uvmpd_lock);
}
