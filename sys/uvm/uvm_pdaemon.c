/*	$NetBSD: uvm_pdaemon.c,v 1.93.4.2.4.3 2012/02/09 03:05:01 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uvm_pdaemon.c,v 1.93.4.2.4.3 2012/02/09 03:05:01 matt Exp $");

#include "opt_uvmhist.h"
#include "opt_readahead.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/buf.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>

/*
 * UVMPD_NUMDIRTYREACTS is how many dirty pages the pagedaemon will reactivate
 * in a pass thru the inactive list when swap is full.  the value should be
 * "small"... if it's too large we'll cycle the active pages thru the inactive
 * queue too quickly to for them to be referenced and avoid being freed.
 */

#define	UVMPD_NUMDIRTYREACTS	16

#define	UVMPD_NUMTRYLOCKOWNER	16

/*
 * local prototypes
 */

static void	uvmpd_scan(struct uvm_pggroup *);
static void	uvmpd_scan_queue(struct uvm_pggroup *);
static void	uvmpd_tune(void);

static struct uvm_pdinfo {
	unsigned int pd_waiters;
	unsigned int pd_scans_neededs;
	struct uvm_pggrouplist pd_pagingq;
	struct uvm_pggrouplist pd_pendingq;
} uvm_pdinfo =  {
	.pd_pagingq = TAILQ_HEAD_INITIALIZER(uvm_pdinfo.pd_pagingq),
	.pd_pendingq = TAILQ_HEAD_INITIALIZER(uvm_pdinfo.pd_pendingq),
};

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

	mutex_spin_enter(&uvm_fpageqlock);

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

	uvm_pdinfo.pd_waiters++;
	wakeup(&uvm.pagedaemon);		/* wake the daemon! */
	UVM_UNLOCK_AND_WAIT(&uvmexp.free, &uvm_fpageqlock, false, wmsg, timo);
}

/*
 * uvm_kick_pdaemon: perform checks to determine if we need to
 * give the pagedaemon a nudge, and do so if necessary.
 *
 * => called with uvm_fpageqlock held.
 */

void
uvm_kick_pdaemon(void)
{
	struct uvm_pdinfo * const pdinfo = &uvm_pdinfo;
	bool need_wakeup = false;
	UVMHIST_FUNC(__func__); UVMHIST_CALLED(pdhist);

	KASSERT(mutex_owned(&uvm_fpageqlock));

	struct uvm_pggroup *grp;
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		const bool prev_scan_needed = grp->pgrp_scan_needed;

		KASSERT(grp->pgrp_npages > 0);

		grp->pgrp_scan_needed =
		    grp->pgrp_free + grp->pgrp_paging < grp->pgrp_freemin
		    || (grp->pgrp_free + grp->pgrp_paging < grp->pgrp_freetarg
			&& uvmpdpol_needsscan_p(grp));

		if (prev_scan_needed != grp->pgrp_scan_needed) {
			UVMHIST_LOG(pdhist, " [%zd] %d->%d (scan=%d)",
			    grp - uvm.pggroups, prev_scan_needed,
			    grp->pgrp_scan_needed, uvmpdpol_needsscan_p(grp));
			UVMHIST_LOG(pdhist, " [%zd] %d < min(%d,%d)",
			    grp - uvm.pggroups,
			    grp->pgrp_free + grp->pgrp_paging,
			    grp->pgrp_freemin, grp->pgrp_freetarg);
		}

		if (grp->pgrp_paging == 0
		    && prev_scan_needed != grp->pgrp_scan_needed) {
			if (grp->pgrp_scan_needed) {
				TAILQ_INSERT_TAIL(&pdinfo->pd_pendingq,
				    grp, pgrp_pd_link);
				need_wakeup = true;
			} else {
				TAILQ_REMOVE(&pdinfo->pd_pendingq,
				    grp, pgrp_pd_link);
			}
		}
	}

	if (need_wakeup)
		wakeup(&uvm.pagedaemon);

	UVMHIST_LOG(pdhist, " <- done: wakeup=%d!",
	    grp - uvm.pggroups, need_wakeup, 0, 0);
}

/*
 * uvmpd_tune: tune paging parameters
 *
 * => called when ever memory is added (or removed?) to the system
 * => caller must call with page queues locked
 */

static void
uvmpd_tune(void)
{
	u_int extrapages = atomic_swap_uint(&uvm_extrapages, 0) / uvmexp.ncolors;
	u_int freemin = 0;
	u_int freetarg = 0;
	u_int wiredmax = 0;

	UVMHIST_FUNC("uvmpd_tune"); UVMHIST_CALLED(pdhist);

	extrapages = roundup(extrapages, uvmexp.npggroups);

	struct uvm_pggroup *grp;
	STAILQ_FOREACH(grp, &uvm.page_groups, pgrp_uvm_link) {
		KASSERT(grp->pgrp_npages > 0);

		/*
		 * try to keep 0.5% of available RAM free, but limit
		 * to between 128k and 1024k per-CPU.
		 * XXX: what are these values good for?
		 */
		u_int val = grp->pgrp_npages / 200;
		val = MAX(val, (128*1024) >> PAGE_SHIFT);
		val = MIN(val, (1024*1024) >> PAGE_SHIFT);
		val *= ncpu;

		/* Make sure there's always a user page free. */
		if (val * uvmexp.npggroups <= uvmexp.reserve_kernel)
			val = uvmexp.reserve_kernel / uvmexp.npggroups + 1;

		grp->pgrp_freemin = val;

		/* Calculate freetarg. */
		val = (grp->pgrp_freemin * 4) / 3;
		if (val <= grp->pgrp_freemin)
			val = grp->pgrp_freemin + 1;
		grp->pgrp_freetarg = val + extrapages / uvmexp.npggroups;
		if (grp->pgrp_freetarg > grp->pgrp_npages / 2)
			grp->pgrp_freetarg = grp->pgrp_npages / 2;

		grp->pgrp_wiredmax = grp->pgrp_npages / 3;
		UVMHIST_LOG(pdhist,
		    "[%zd]: freemin=%d, freetarg=%d, wiredmax=%d",
		    grp - uvm.pggroups, grp->pgrp_freemin, grp->pgrp_freetarg,
		    grp->pgrp_wiredmax);

		freemin += grp->pgrp_freemin;
		freetarg += grp->pgrp_freetarg;
		wiredmax += grp->pgrp_wiredmax;
	}

	uvmexp.freemin = freemin;
	uvmexp.freetarg = freetarg;
	uvmexp.wiredmax = wiredmax;

	UVMHIST_LOG(pdhist, "<- done, freemin=%d, freetarg=%d, wiredmax=%d",
	    uvmexp.freemin, uvmexp.freetarg, uvmexp.wiredmax, 0);
}

/*
 * uvm_pageout: the main loop for the pagedaemon
 */

void
uvm_pageout(void *arg)
{
	u_int npages = 0;
	u_int extrapages = 0;
	u_int npggroups = 0;
	struct pool *pp;
	uint64_t where;
	struct uvm_pdinfo * const pdinfo = &uvm_pdinfo;
	UVMHIST_FUNC("uvm_pageout"); UVMHIST_CALLED(pdhist);

	UVMHIST_LOG(pdhist,"<starting uvm pagedaemon>", 0, 0, 0, 0);

	/*
	 * ensure correct priority and set paging parameters...
	 */

	uvm.pagedaemon_lwp = curlwp;
	mutex_enter(&uvm_pageqlock);
	npages = uvmexp.npages;
	uvmpd_tune();
	mutex_exit(&uvm_pageqlock);

	/*
	 * main loop
	 */

	for (;;) {
		struct uvm_pggroup *grp;
		bool need_free = false;
		u_int bufcnt = 0;

		mutex_spin_enter(&uvm_fpageqlock);
		/*
		 * If we have no one waiting or all color requests have
		 * active paging, then wait.
		 */
		if (pdinfo->pd_waiters == 0
		    || TAILQ_FIRST(&pdinfo->pd_pendingq) == NULL) {
			UVMHIST_LOG(pdhist,"  <<SLEEPING>>",0,0,0,0);
			UVM_UNLOCK_AND_WAIT(&uvm.pagedaemon,
			    &uvm_fpageqlock, false, "pgdaemon", 0);
			uvmexp.pdwoke++;
			UVMHIST_LOG(pdhist,"  <<WOKE UP>>",0,0,0,0);
		} else {
			mutex_spin_exit(&uvm_fpageqlock);
		}

		/*
		 * now lock page queues and recompute inactive count
		 */

		mutex_enter(&uvm_pageqlock);
		mutex_spin_enter(&uvm_fpageqlock);

		if (npages != uvmexp.npages
		    || extrapages != uvm_extrapages
		    || npggroups != uvmexp.npggroups) {
			npages = uvmexp.npages;
			extrapages = uvm_extrapages;
			npggroups = uvmexp.npggroups;
			uvmpd_tune();
		}

		/*
		 * Estimate a hint.  Note that bufmem are returned to
		 * system only when entire pool page is empty.
		 */
		bool need_wakeup = false;
		while ((grp = TAILQ_FIRST(&pdinfo->pd_pendingq)) != NULL) {
			KASSERT(grp->pgrp_npages > 0);

			uvmpdpol_tune(grp);

			int diff = grp->pgrp_freetarg - grp->pgrp_free;
			if (diff < 0)
				diff = 0;

			bufcnt += diff;

			UVMHIST_LOG(pdhist," [%zu]: "
			    "free/ftarg/fmin=%u/%u/%u",
			    grp - uvm.pggroups, grp->pgrp_free,
			    grp->pgrp_freetarg, grp->pgrp_freemin);


			if (grp->pgrp_paging < diff)
				need_free = true;

			/*
			 * scan if needed
			 */
			if (grp->pgrp_paging < diff
			    || uvmpdpol_needsscan_p(grp)) {
				mutex_spin_exit(&uvm_fpageqlock);
				uvmpd_scan(grp);
				mutex_spin_enter(&uvm_fpageqlock);
			} else {
				UVMHIST_LOG(pdhist,
				    " [%zu]: diff/paging=%u/%u: "
				    "scan skipped",
				    grp - uvm.pggroups, diff,
				    grp->pgrp_paging, 0);
			}

			/*
			 * if there's any free memory to be had,
			 * wake up any waiters.
			 */
			if (grp->pgrp_free * uvmexp.npggroups > uvmexp.reserve_kernel
			    || grp->pgrp_paging == 0) {
				need_wakeup = true;
			}

			/*
			 * We are done, remove it from the queue.
			 */
			TAILQ_REMOVE(&pdinfo->pd_pendingq, grp, pgrp_pd_link);
			grp->pgrp_scan_needed = false;
		}
		if (need_wakeup) {
			pdinfo->pd_waiters = 0;
			wakeup(&uvmexp.free);
		}
		KASSERT (!need_free || need_wakeup);
		mutex_spin_exit(&uvm_fpageqlock);

		/*
		 * scan done.  unlock page queues (the only lock
		 * we are holding)
		 */
		mutex_exit(&uvm_pageqlock);

		/*
		 * if we don't need free memory, we're done.
		 */

		if (!need_free) 
			continue;

		/*
		 * start draining pool resources now that we're not
		 * holding any locks.
		 */
		pool_drain_start(&pp, &where);

		/*
		 * kill unused metadata buffers.
		 */
		if (bufcnt > 0) {
			mutex_enter(&bufcache_lock);
			buf_drain(bufcnt << PAGE_SHIFT);
			mutex_exit(&bufcache_lock);
		}

		/*
		 * complete draining the pools.
		 */
		pool_drain_end(pp, where);
	}
	/*NOTREACHED*/
}


/*
 * uvm_aiodone_worker: a workqueue callback for the aiodone daemon.
 */

void
uvm_aiodone_worker(struct work *wk, void *dummy)
{
	struct buf *bp = (void *)wk;

	KASSERT(&bp->b_work == wk);

	/*
	 * process an i/o that's done.
	 */

	(*bp->b_iodone)(bp);
}

void
uvm_pageout_start(struct uvm_pggroup *grp, u_int npages)
{
	struct uvm_pdinfo * const pdinfo = &uvm_pdinfo;

	mutex_spin_enter(&uvm_fpageqlock);

	uvmexp.paging += npages;
	if (grp->pgrp_paging == 0) {
		KASSERT(grp->pgrp_scan_needed);
		TAILQ_REMOVE(&pdinfo->pd_pendingq, grp, pgrp_pd_link);
		TAILQ_INSERT_TAIL(&pdinfo->pd_pagingq, grp, pgrp_pd_link);
	}
	grp->pgrp_paging += npages;
	mutex_spin_exit(&uvm_fpageqlock);
}

void
uvm_pageout_done(struct vm_page *pg, bool freed)
{
	struct uvm_pdinfo * const pdinfo = &uvm_pdinfo;

	KASSERT(pg->flags & PG_PAGEOUT);

	mutex_spin_enter(&uvm_fpageqlock);
	struct uvm_pggroup * const grp = uvm_page_to_pggroup(pg);

	KASSERT(grp->pgrp_paging > 0);
	if (--grp->pgrp_paging == 0) {
		TAILQ_REMOVE(&pdinfo->pd_pagingq, grp, pgrp_pd_link);
		if (grp->pgrp_scan_needed) {
			TAILQ_INSERT_TAIL(&pdinfo->pd_pendingq, grp, pgrp_pd_link);
		}
	}

	KASSERT(uvmexp.paging > 0);
	uvmexp.paging--;
	grp->pgrp_pdfreed += freed;

	/*
	 * wake up either of pagedaemon or LWPs waiting for it.
	 */
	if (grp->pgrp_free * uvmexp.npggroups <= uvmexp.reserve_kernel) {
		wakeup(&uvm.pagedaemon);
	} else {
		pdinfo->pd_waiters = 0;
		wakeup(&uvmexp.free);
	}

	mutex_spin_exit(&uvm_fpageqlock);
}

/*
 * uvmpd_trylockowner: trylock the page's owner.
 *
 * => called with pageq locked.
 * => resolve orphaned O->A loaned page.
 * => return the locked mutex on success.  otherwise, return NULL.
 */

kmutex_t *
uvmpd_trylockowner(struct vm_page *pg)
{
	struct uvm_object *uobj = pg->uobject;
	kmutex_t *slock;

	KASSERT(mutex_owned(&uvm_pageqlock));

	if (uobj != NULL) {
		slock = &uobj->vmobjlock;
	} else {
		struct vm_anon *anon = pg->uanon;

		KASSERT(anon != NULL);
		slock = &anon->an_lock;
	}

	if (!mutex_tryenter(slock)) {
		return NULL;
	}

	if (uobj == NULL) {

		/*
		 * set PQ_ANON if it isn't set already.
		 */

		if ((pg->pqflags & PQ_ANON) == 0) {
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->pqflags |= PQ_ANON;
			/* anon now owns it */
		}
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
	KASSERT((pg->pqflags & PQ_SWAPBACKED) != 0);

	slot = swc->swc_slot + swc->swc_nused;
	uobj = pg->uobject;
	if (uobj == NULL) {
		KASSERT(mutex_owned(&pg->uanon->an_lock));
		pg->uanon->an_swslot = slot;
	} else {
		int result;

		KASSERT(mutex_owned(&uobj->vmobjlock));
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
swapcluster_flush(struct uvm_pggroup *grp, struct swapcluster *swc, bool now)
{
	int slot;
	u_int nused;
	int nallocated;
	int error;

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
		grp->pgrp_pdpageouts++;
		uvmexp.pdpageouts++;	/* procfs */
		uvm_pageout_start(grp, nused);
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

static bool
uvmpd_dropswap(struct vm_page *pg)
{
	bool result = false;
	struct vm_anon *anon = pg->uanon;

	if ((pg->pqflags & PQ_ANON) && anon->an_swslot) {
		uvm_swap_free(anon->an_swslot, 1);
		anon->an_swslot = 0;
		pg->flags &= ~PG_CLEAN;
		result = true;
	} else if (pg->pqflags & PQ_AOBJ) {
		int slot = uao_set_swslot(pg->uobject,
		    pg->offset >> PAGE_SHIFT, 0);
		if (slot) {
			uvm_swap_free(slot, 1);
			pg->flags &= ~PG_CLEAN;
			result = true;
		}
	}

	return result;
}

/*
 * uvmpd_trydropswap: try to free any swap allocated to this page.
 *
 * => return true if a slot is successfully freed.
 */

bool
uvmpd_trydropswap(struct vm_page *pg)
{
	kmutex_t *slock;
	bool result;

	if ((pg->flags & PG_BUSY) != 0) {
		return false;
	}

	/*
	 * lock the page's owner.
	 */

	slock = uvmpd_trylockowner(pg);
	if (slock == NULL) {
		return false;
	}

	/*
	 * skip this page if it's busy.
	 */

	if ((pg->flags & PG_BUSY) != 0) {
		mutex_exit(slock);
		return false;
	}

	result = uvmpd_dropswap(pg);

	mutex_exit(slock);

	return result;
}

#endif /* defined(VMSWAP) */

/*
 * uvmpd_scan_queue: scan an replace candidate list for pages
 * to clean or free.
 *
 * => called with page queues locked
 * => we work on meeting our free target by converting inactive pages
 *    into free pages.
 * => we handle the building of swap-backed clusters
 */

static void
uvmpd_scan_queue(struct uvm_pggroup *grp)
{
	struct vm_page *pg;
	struct uvm_object *uobj;
	struct vm_anon *anon;
#if defined(VMSWAP)
	struct swapcluster swc;
#endif /* defined(VMSWAP) */
	int dirtyreacts;
	int lockownerfail;
	kmutex_t *slock;
	UVMHIST_FUNC("uvmpd_scan_queue"); UVMHIST_CALLED(pdhist);

	/*
	 * swslot is non-zero if we are building a swap cluster.  we want
	 * to stay in the loop while we have a page to scan or we have
	 * a swap-cluster to build.
	 */

#if defined(VMSWAP)
	swapcluster_init(&swc);
#endif /* defined(VMSWAP) */

	dirtyreacts = 0;
	lockownerfail = 0;
	uvmpdpol_scaninit(grp);

	while (/* CONSTCOND */ 1) {

		/*
		 * see if we've met the free target.
		 */

		if (grp->pgrp_free + grp->pgrp_paging
#if defined(VMSWAP)
		    + swapcluster_nused(&swc)
#endif /* defined(VMSWAP) */
		    >= grp->pgrp_freetarg << 2 ||
		    dirtyreacts == UVMPD_NUMDIRTYREACTS) {
			UVMHIST_LOG(pdhist,"  [%zd]: met free target (%u + %u >= %u): "
			    "exit loop", grp - uvm.pggroups,
			    grp->pgrp_free, grp->pgrp_paging,
			    grp->pgrp_freetarg << 2);
			break;
		}

		pg = uvmpdpol_selectvictim(grp);
		if (pg == NULL) {
			UVMHIST_LOG(pdhist,"  [%zd]: selectvictim didn't: "
			    "exit loop", grp - uvm.pggroups, 0, 0, 0);
			break;
		}
		KASSERT(uvmpdpol_pageisqueued_p(pg));
		KASSERT(pg->wire_count == 0);

		/*
		 * we are below target and have a new page to consider.
		 */

		anon = pg->uanon;
		uobj = pg->uobject;

		/*
		 * first we attempt to lock the object that this page
		 * belongs to.  if our attempt fails we skip on to
		 * the next page (no harm done).  it is important to
		 * "try" locking the object as we are locking in the
		 * wrong order (pageq -> object) and we don't want to
		 * deadlock.
		 *
		 * the only time we expect to see an ownerless page
		 * (i.e. a page with no uobject and !PQ_ANON) is if an
		 * anon has loaned a page from a uvm_object and the
		 * uvm_object has dropped the ownership.  in that
		 * case, the anon can "take over" the loaned page
		 * and make it its own.
		 */

		slock = uvmpd_trylockowner(pg);
		if (slock == NULL) {
			/*
			 * yield cpu to make a chance for an LWP holding
			 * the lock run.  otherwise we can busy-loop too long
			 * if the page queue is filled with a lot of pages
			 * from few objects.
			 */
			lockownerfail++;
			if (lockownerfail > UVMPD_NUMTRYLOCKOWNER) {
				mutex_exit(&uvm_pageqlock);
				/* XXX Better than yielding but inadequate. */
				kpause("livelock", false, 1, NULL);
				mutex_enter(&uvm_pageqlock);
				lockownerfail = 0;
			}
			continue;
		}
		if (pg->flags & PG_BUSY) {
			mutex_exit(slock);
			grp->pgrp_pdbusy++;
			continue;
		}

		/* does the page belong to an object? */
		if (uobj != NULL) {
			grp->pgrp_pdobscan++;
		} else {
#if defined(VMSWAP)
			KASSERT(anon != NULL);
			grp->pgrp_pdanscan++;
#else /* defined(VMSWAP) */
			panic("%s: anon", __func__);
#endif /* defined(VMSWAP) */
		}


		/*
		 * we now have the object and the page queues locked.
		 * if the page is not swap-backed, call the object's
		 * pager to flush and free the page.
		 */

#if defined(READAHEAD_STATS)
		if ((pg->pqflags & PQ_READAHEAD) != 0) {
			pg->pqflags &= ~PQ_READAHEAD;
			uvm_ra_miss.ev_count++;
		}
#endif /* defined(READAHEAD_STATS) */

		if ((pg->pqflags & PQ_SWAPBACKED) == 0) {
			KASSERT(uobj != NULL);
			mutex_exit(&uvm_pageqlock);
			(void) (uobj->pgops->pgo_put)(uobj, pg->offset,
			    pg->offset + PAGE_SIZE, PGO_CLEANIT|PGO_FREE);
			mutex_enter(&uvm_pageqlock);
			continue;
		}

		/*
		 * the page is swap-backed.  remove all the permissions
		 * from the page so we can sync the modified info
		 * without any race conditions.  if the page is clean
		 * we can free it now and continue.
		 */

		pmap_page_protect(pg, VM_PROT_NONE);
		if ((pg->flags & PG_CLEAN) && pmap_clear_modify(pg)) {
			pg->flags &= ~(PG_CLEAN);
		}
		if (pg->flags & PG_CLEAN) {
			int slot;
			int pageidx;

			pageidx = pg->offset >> PAGE_SHIFT;
			KASSERT(!uvmpdpol_pageisqueued_p(pg));
			uvm_pagefree(pg);
			grp->pgrp_pdfreed++;

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
			mutex_exit(slock);

			if (slot > 0) {
				/* this page is now only in swap. */
				mutex_enter(&uvm_swap_data_lock);
				KASSERT(uvmexp.swpgonly < uvmexp.swpginuse);
				uvmexp.swpgonly++;
				mutex_exit(&uvm_swap_data_lock);
			}
			continue;
		}

#if defined(VMSWAP)
		/*
		 * this page is dirty, skip it if we'll have met our
		 * free target when all the current pageouts complete.
		 */

		if (grp->pgrp_free + grp->pgrp_paging > grp->pgrp_freetarg << 2) {
			mutex_exit(slock);
			continue;
		}

		/*
		 * free any swap space allocated to the page since
		 * we'll have to write it again with its new data.
		 */

		uvmpd_dropswap(pg);

		/*
		 * start new swap pageout cluster (if necessary).
		 *
		 * if swap is full reactivate this page so that
		 * we eventually cycle all pages through the
		 * inactive queue.
		 */

		if (swapcluster_allocslots(&swc)) {
			dirtyreacts++;
			uvm_pageactivate(pg);
			mutex_exit(slock);
			continue;
		}

		/*
		 * at this point, we're definitely going reuse this
		 * page.  mark the page busy and delayed-free.
		 * we should remove the page from the page queues
		 * so we don't ever look at it again.
		 * adjust counters and such.
		 */

		pg->flags |= PG_BUSY;
		UVM_PAGE_OWN(pg, "scan_queue");

		pg->flags |= PG_PAGEOUT;
		uvm_pagedequeue(pg);

		grp->pgrp_pgswapout++;
		mutex_exit(&uvm_pageqlock);

		/*
		 * add the new page to the cluster.
		 */

		if (swapcluster_add(&swc, pg)) {
			pg->flags &= ~(PG_BUSY|PG_PAGEOUT);
			UVM_PAGE_OWN(pg, NULL);
			mutex_enter(&uvm_pageqlock);
			dirtyreacts++;
			uvm_pageactivate(pg);
			mutex_exit(slock);
			continue;
		}
		mutex_exit(slock);

		swapcluster_flush(grp, &swc, false);
		mutex_enter(&uvm_pageqlock);

		/*
		 * the pageout is in progress.  bump counters and set up
		 * for the next loop.
		 */

		uvmexp.pdpending++;
#else /* defined(VMSWAP) */
		uvm_pageactivate(pg);
		mutex_exit(slock);
#endif /* defined(VMSWAP) */
	}

#if defined(VMSWAP)
	mutex_exit(&uvm_pageqlock);
	swapcluster_flush(grp, &swc, true);
	mutex_enter(&uvm_pageqlock);
#endif /* defined(VMSWAP) */
}

/*
 * uvmpd_scan: scan the page queues and attempt to meet our targets.
 *
 * => called with pageq's locked
 */

static void
uvmpd_scan(struct uvm_pggroup *grp)
{
	u_int swap_shortage, pages_freed;
	UVMHIST_FUNC("uvmpd_scan"); UVMHIST_CALLED(pdhist);

	grp->pgrp_pdrevs++;

	/*
	 * work on meeting our targets.   first we work on our free target
	 * by converting inactive pages into free pages.  then we work on
	 * meeting our inactive target by converting active pages to
	 * inactive ones.
	 */

	UVMHIST_LOG(pdhist, "  starting 'free' loop",0,0,0,0);

	pages_freed = grp->pgrp_pdfreed;
	uvmpd_scan_queue(grp);
	pages_freed = grp->pgrp_pdfreed - pages_freed;

	/*
	 * detect if we're not going to be able to page anything out
	 * until we free some swap resources from active pages.
	 */

	swap_shortage = 0;
	if (grp->pgrp_free < grp->pgrp_freetarg &&
	    uvmexp.swpginuse >= uvmexp.swpgavail &&
	    !uvm_swapisfull() &&
	    pages_freed == 0) {
		swap_shortage = grp->pgrp_freetarg - grp->pgrp_free;
	}

	uvmpdpol_balancequeue(grp, swap_shortage);

	/*
	 * swap out some processes if we are still below the minimum
	 * free target.  we need to unlock the page queues for this.
	 */

	if (grp->pgrp_free < grp->pgrp_freemin
	    && uvmexp.nswapdev != 0 && uvm.swapout_enabled) {
		grp->pgrp_pdswout++;
		UVMHIST_LOG(pdhist,"  free %d < min %d: swapout",
		    uvmexp.free, uvmexp.freemin, 0, 0);
		mutex_exit(&uvm_pageqlock);
		uvm_swapout_threads();
		mutex_enter(&uvm_pageqlock);

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
	 *
	 * XXX assume the worst case, ie. all wired pages are file-backed.
	 *
	 * XXX should consider about other reclaimable memory.
	 * XXX ie. pools, traditional buffer cache.
	 */

	filepages = uvmexp.filepages + uvmexp.execpages - uvmexp.wired;
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
uvm_estimatepageable(u_int *active, u_int *inactive)
{

	uvmpdpol_estimatepageable(active, inactive);
}
