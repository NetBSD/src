/*	$NetBSD: uvm_pager.c,v 1.72.2.1 2006/01/15 10:03:05 yamt Exp $	*/

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
 * from: Id: uvm_pager.c,v 1.1.2.23 1998/02/02 20:38:06 chuck Exp
 */

/*
 * uvm_pager.c: generic functions used to assist the pagers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_pager.c,v 1.72.2.1 2006/01/15 10:03:05 yamt Exp $");

#include "opt_uvmhist.h"
#include "opt_readahead.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/vnode.h>

#define UVM_PAGER_C
#include <uvm/uvm.h>

struct pool *uvm_aiobuf_pool;

/*
 * list of uvm pagers in the system
 */

struct uvm_pagerops * const uvmpagerops[] = {
	&aobj_pager,
	&uvm_deviceops,
	&uvm_vnodeops,
	&ubc_pager,
};

/*
 * the pager map: provides KVA for I/O
 */

struct vm_map *pager_map;		/* XXX */
struct simplelock pager_map_wanted_lock;
boolean_t pager_map_wanted;	/* locked by pager map */
static vaddr_t emergva;
static boolean_t emerginuse;

/*
 * uvm_pager_init: init pagers (at boot time)
 */

void
uvm_pager_init(void)
{
	u_int lcv;
	vaddr_t sva, eva;

	/*
	 * init pager map
	 */

	sva = 0;
	pager_map = uvm_km_suballoc(kernel_map, &sva, &eva, PAGER_MAP_SIZE, 0,
	    FALSE, NULL);
	simple_lock_init(&pager_map_wanted_lock);
	pager_map_wanted = FALSE;
	emergva = uvm_km_alloc(kernel_map, round_page(MAXPHYS), 0,
	    UVM_KMF_VAONLY);
#if defined(DEBUG)
	if (emergva == 0)
		panic("emergva");
#endif
	emerginuse = FALSE;

	/*
	 * init ASYNC I/O queue
	 */

	TAILQ_INIT(&uvm.aio_done);

	/*
	 * call pager init functions
	 */
	for (lcv = 0 ; lcv < sizeof(uvmpagerops)/sizeof(struct uvm_pagerops *);
	    lcv++) {
		if (uvmpagerops[lcv]->pgo_init)
			uvmpagerops[lcv]->pgo_init();
	}
}

/*
 * uvm_pagermapin: map pages into KVA (pager_map) for I/O that needs mappings
 *
 * we basically just map in a blank map entry to reserve the space in the
 * map and then use pmap_enter() to put the mappings in by hand.
 */

vaddr_t
uvm_pagermapin(struct vm_page **pps, int npages, int flags)
{
	vsize_t size;
	vaddr_t kva;
	vaddr_t cva;
	struct vm_page *pp;
	vm_prot_t prot;
	UVMHIST_FUNC("uvm_pagermapin"); UVMHIST_CALLED(maphist);

	UVMHIST_LOG(maphist,"(pps=0x%x, npages=%d)", pps, npages,0,0);

	/*
	 * compute protection.  outgoing I/O only needs read
	 * access to the page, whereas incoming needs read/write.
	 */

	prot = VM_PROT_READ;
	if (flags & UVMPAGER_MAPIN_READ)
		prot |= VM_PROT_WRITE;

ReStart:
	size = npages << PAGE_SHIFT;
	kva = 0;			/* let system choose VA */

	if (uvm_map(pager_map, &kva, size, NULL,
	      UVM_UNKNOWN_OFFSET, 0, UVM_FLAG_NOMERGE) != 0) {
		if (curproc == uvm.pagedaemon_proc) {
			simple_lock(&pager_map_wanted_lock);
			if (emerginuse) {
				UVM_UNLOCK_AND_WAIT(&emergva,
				    &pager_map_wanted_lock, FALSE,
				    "emergva", 0);
				goto ReStart;
			}
			emerginuse = TRUE;
			simple_unlock(&pager_map_wanted_lock);
			kva = emergva;
			/* The shift implicitly truncates to PAGE_SIZE */
			KASSERT(npages <= (MAXPHYS >> PAGE_SHIFT));
			goto enter;
		}
		if ((flags & UVMPAGER_MAPIN_WAITOK) == 0) {
			UVMHIST_LOG(maphist,"<- NOWAIT failed", 0,0,0,0);
			return(0);
		}
		simple_lock(&pager_map_wanted_lock);
		pager_map_wanted = TRUE;
		UVMHIST_LOG(maphist, "  SLEEPING on pager_map",0,0,0,0);
		UVM_UNLOCK_AND_WAIT(pager_map, &pager_map_wanted_lock, FALSE,
		    "pager_map", 0);
		goto ReStart;
	}

enter:
	/* got it */
	for (cva = kva ; size != 0 ; size -= PAGE_SIZE, cva += PAGE_SIZE) {
		pp = *pps++;
		KASSERT(pp);
		KASSERT(pp->flags & PG_BUSY);
		pmap_kenter_pa(cva, VM_PAGE_TO_PHYS(pp), prot);
	}
	pmap_update(vm_map_pmap(pager_map));

	UVMHIST_LOG(maphist, "<- done (KVA=0x%x)", kva,0,0,0);
	return(kva);
}

/*
 * uvm_pagermapout: remove pager_map mapping
 *
 * we remove our mappings by hand and then remove the mapping (waking
 * up anyone wanting space).
 */

void
uvm_pagermapout(vaddr_t kva, int npages)
{
	vsize_t size = npages << PAGE_SHIFT;
	struct vm_map_entry *entries;
	UVMHIST_FUNC("uvm_pagermapout"); UVMHIST_CALLED(maphist);

	UVMHIST_LOG(maphist, " (kva=0x%x, npages=%d)", kva, npages,0,0);

	/*
	 * duplicate uvm_unmap, but add in pager_map_wanted handling.
	 */

	pmap_kremove(kva, npages << PAGE_SHIFT);
	if (kva == emergva) {
		simple_lock(&pager_map_wanted_lock);
		emerginuse = FALSE;
		wakeup(&emergva);
		simple_unlock(&pager_map_wanted_lock);
		return;
	}

	vm_map_lock(pager_map);
	uvm_unmap_remove(pager_map, kva, kva + size, &entries, NULL, 0);
	simple_lock(&pager_map_wanted_lock);
	if (pager_map_wanted) {
		pager_map_wanted = FALSE;
		wakeup(pager_map);
	}
	simple_unlock(&pager_map_wanted_lock);
	vm_map_unlock(pager_map);
	if (entries)
		uvm_unmap_detach(entries, 0);
	pmap_update(pmap_kernel());
	UVMHIST_LOG(maphist,"<- done",0,0,0,0);
}

/*
 * interrupt-context iodone handler for nested i/o bufs.
 *
 * => must be at splbio().
 */

void
uvm_aio_biodone1(struct buf *bp)
{
	struct buf *mbp = bp->b_private;

	KASSERT(mbp != bp);
	if (bp->b_flags & B_ERROR) {
		mbp->b_flags |= B_ERROR;
		mbp->b_error = bp->b_error;
	}
	mbp->b_resid -= bp->b_bcount;
	putiobuf(bp);
	if (mbp->b_resid == 0) {
		biodone(mbp);
	}
}

/*
 * interrupt-context iodone handler for single-buf i/os
 * or the top-level buf of a nested-buf i/o.
 *
 * => must be at splbio().
 */

void
uvm_aio_biodone(struct buf *bp)
{
	/* reset b_iodone for when this is a single-buf i/o. */
	bp->b_iodone = uvm_aio_aiodone;

	simple_lock(&uvm.aiodoned_lock);	/* locks uvm.aio_done */
	TAILQ_INSERT_TAIL(&uvm.aio_done, bp, b_freelist);
	wakeup(&uvm.aiodoned);
	simple_unlock(&uvm.aiodoned_lock);
}

/*
 * uvm_aio_aiodone: do iodone processing for async i/os.
 * this should be called in thread context, not interrupt context.
 */

void
uvm_aio_aiodone(struct buf *bp)
{
	int npages = bp->b_bufsize >> PAGE_SHIFT;
	struct vm_page *pg, *pgs[npages];
	struct uvm_object *uobj;
	struct simplelock *slock;
	int s, i, error, swslot;
	boolean_t write, swap;
	UVMHIST_FUNC("uvm_aio_aiodone"); UVMHIST_CALLED(ubchist);
	UVMHIST_LOG(ubchist, "bp %p", bp, 0,0,0);

	error = (bp->b_flags & B_ERROR) ? (bp->b_error ? bp->b_error : EIO) : 0;
	write = (bp->b_flags & B_READ) == 0;
	/* XXXUBC B_NOCACHE is for swap pager, should be done differently */
	if (write && !(bp->b_flags & B_NOCACHE) && bioops.io_pageiodone) {
		(*bioops.io_pageiodone)(bp);
	}

	uobj = NULL;
	for (i = 0; i < npages; i++) {
		pgs[i] = uvm_pageratop((vaddr_t)bp->b_data + (i << PAGE_SHIFT));
		UVMHIST_LOG(ubchist, "pgs[%d] = %p", i, pgs[i],0,0);
	}
	uvm_pagermapout((vaddr_t)bp->b_data, npages);

	swslot = 0;
	slock = NULL;
	pg = pgs[0];
	swap = (pg->uanon != NULL && pg->uobject == NULL) ||
		(pg->pqflags & PQ_AOBJ) != 0;
	if (!swap) {
		uobj = pg->uobject;
		slock = &uobj->vmobjlock;
		simple_lock(slock);
		uvm_lock_pageq();
	} else {
#if defined(VMSWAP)
		if (error) {
			if (pg->uobject != NULL) {
				swslot = uao_find_swslot(pg->uobject,
				    pg->offset >> PAGE_SHIFT);
			} else {
				swslot = pg->uanon->an_swslot;
			}
			KASSERT(swslot);
		}
#else /* defined(VMSWAP) */
		panic("%s: swap", __func__);
#endif /* defined(VMSWAP) */
	}
	for (i = 0; i < npages; i++) {
		pg = pgs[i];
		KASSERT(swap || pg->uobject == uobj);
		UVMHIST_LOG(ubchist, "pg %p", pg, 0,0,0);

#if defined(VMSWAP)
		/*
		 * for swap i/os, lock each page's object (or anon)
		 * individually since each page may need a different lock.
		 */

		if (swap) {
			if (pg->uobject != NULL) {
				slock = &pg->uobject->vmobjlock;
			} else {
				slock = &pg->uanon->an_lock;
			}
			simple_lock(slock);
			uvm_lock_pageq();
		}
#endif /* defined(VMSWAP) */

		/*
		 * process errors.  for reads, just mark the page to be freed.
		 * for writes, if the error was ENOMEM, we assume this was
		 * a transient failure so we mark the page dirty so that
		 * we'll try to write it again later.  for all other write
		 * errors, we assume the error is permanent, thus the data
		 * in the page is lost.  bummer.
		 */

		if (error) {
			int slot;
			if (!write) {
				pg->flags |= PG_RELEASED;
				continue;
			} else if (error == ENOMEM) {
				if (pg->flags & PG_PAGEOUT) {
					pg->flags &= ~PG_PAGEOUT;
					uvmexp.paging--;
				}
				pg->flags &= ~PG_CLEAN;
				uvm_pageactivate(pg);
				slot = 0;
			} else
				slot = SWSLOT_BAD;

#if defined(VMSWAP)
			if (swap) {
				if (pg->uobject != NULL) {
					int oldslot;
					oldslot = uao_set_swslot(pg->uobject,
						pg->offset >> PAGE_SHIFT, slot);
					KASSERT(oldslot == swslot + i);
				} else {
					KASSERT(pg->uanon->an_swslot ==
						swslot + i);
					pg->uanon->an_swslot = slot;
				}
			}
#endif /* defined(VMSWAP) */
		}

		/*
		 * if the page is PG_FAKE, this must have been a read to
		 * initialize the page.  clear PG_FAKE and activate the page.
		 * we must also clear the pmap "modified" flag since it may
		 * still be set from the page's previous identity.
		 */

		if (pg->flags & PG_FAKE) {
			KASSERT(!write);
			pg->flags &= ~PG_FAKE;
#if defined(READAHEAD_STATS)
			pg->flags |= PG_SPECULATIVE;
			uvm_ra_total.ev_count++;
#endif /* defined(READAHEAD_STATS) */
			uvm_pageactivate(pg);
			pmap_clear_modify(pg);
		}

		/*
		 * do accounting for pagedaemon i/o and arrange to free
		 * the pages instead of just unbusying them.
		 */

		if (pg->flags & PG_PAGEOUT) {
			pg->flags &= ~PG_PAGEOUT;
			uvmexp.paging--;
			uvmexp.pdfreed++;
			pg->flags |= PG_RELEASED;
		}

#if defined(VMSWAP)
		/*
		 * for swap pages, unlock everything for this page now.
		 */

		if (swap) {
			if (pg->uobject == NULL && pg->uanon->an_ref == 0 &&
			    (pg->flags & PG_RELEASED) != 0) {
				uvm_unlock_pageq();
				uvm_anon_release(pg->uanon);
			} else {
				uvm_page_unbusy(&pg, 1);
				uvm_unlock_pageq();
				simple_unlock(slock);
			}
		}
#endif /* defined(VMSWAP) */
	}
	if (!swap) {
		uvm_page_unbusy(pgs, npages);
		uvm_unlock_pageq();
		simple_unlock(slock);
	} else {
#if defined(VMSWAP)
		KASSERT(write);

		/* these pages are now only in swap. */
		simple_lock(&uvm.swap_data_lock);
		KASSERT(uvmexp.swpgonly + npages <= uvmexp.swpginuse);
		if (error != ENOMEM)
			uvmexp.swpgonly += npages;
		simple_unlock(&uvm.swap_data_lock);
		if (error) {
			if (error != ENOMEM)
				uvm_swap_markbad(swslot, npages);
			else
				uvm_swap_free(swslot, npages);
		}
		uvmexp.pdpending--;
#endif /* defined(VMSWAP) */
	}
	s = splbio();
	if (write && (bp->b_flags & B_AGE) != 0) {
		vwakeup(bp);
	}
	putiobuf(bp);
	splx(s);
}
