/*	$NetBSD: uvm_loan.c,v 1.81.2.10 2012/01/11 00:08:41 yamt Exp $	*/

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
 *
 * from: Id: uvm_loan.c,v 1.1.6.4 1998/02/06 05:08:43 chs Exp
 */

/*
 * uvm_loan.c: page loanout handler
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_loan.c,v 1.81.2.10 2012/01/11 00:08:41 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/mman.h>

#include <uvm/uvm.h>

bool vm_loan_read = true;

/*
 * "loaned" pages are pages which are (read-only, copy-on-write) loaned
 * from the VM system to other parts of the kernel.   this allows page
 * copying to be avoided (e.g. you can loan pages from objs/anons to
 * the mbuf system).
 *
 * there are 3 types of loans possible:
 *  O->K  uvm_object page to wired kernel page (e.g. mbuf data area)
 *  A->K  anon page to wired kernel page (e.g. mbuf data area)
 *  O->A  uvm_object to anon loan (e.g. vnode page to an anon)
 * note that it possible to have an O page loaned to both an A and K
 * at the same time.
 *
 * loans are tracked by pg->loan_count.  an O->A page will have both
 * a uvm_object and a vm_anon, but PQ_ANON will not be set.   this sort
 * of page is considered "owned" by the uvm_object (not the anon).
 *
 * each loan of a page to the kernel bumps the pg->wire_count.  the
 * kernel mappings for these pages will be read-only and wired.  since
 * the page will also be wired, it will not be a candidate for pageout,
 * and thus will never be pmap_page_protect()'d with VM_PROT_NONE.  a
 * write fault in the kernel to one of these pages will not cause
 * copy-on-write.  instead, the page fault is considered fatal.  this
 * is because the kernel mapping will have no way to look up the
 * object/anon which the page is owned by.  this is a good side-effect,
 * since a kernel write to a loaned page is an error.
 *
 * owners that want to free their pages and discover that they are
 * loaned out simply "disown" them (the page becomes an orphan).  these
 * pages should be freed when the last loan is dropped.   in some cases
 * an anon may "adopt" an orphaned page.
 *
 * locking: to read pg->loan_count either the owner or the page queues
 * must be locked.   to modify pg->loan_count, both the owner of the page
 * and the PQs must be locked.   pg->flags is (as always) locked by
 * the owner of the page.
 *
 * note that locking from the "loaned" side is tricky since the object
 * getting the loaned page has no reference to the page's owner and thus
 * the owner could "die" at any time.   in order to prevent the owner
 * from dying the page queues should be locked.   this forces us to sometimes
 * use "try" locking.
 *
 * loans are typically broken by the following events:
 *  1. user-level xwrite fault to a loaned page
 *  2. pageout of clean+inactive O->A loaned page
 *  3. owner frees page (e.g. pager flush)
 *
 * note that loaning a page causes all mappings of the page to become
 * read-only (via pmap_page_protect).   this could have an unexpected
 * effect on normal "wired" pages if one is not careful (XXX).
 */

/*
 * local prototypes
 */

static int	uvm_loananon(struct uvm_faultinfo *, void ***,
			     int, struct vm_anon *);
static int	uvm_loanuobj(struct uvm_faultinfo *, void ***,
			     int, vaddr_t);
static int	uvm_loanzero(struct uvm_faultinfo *, void ***, int);
static void	uvm_unloananon(struct vm_anon **, int);
static void	uvm_unloanpage(struct vm_page **, int);
static int	uvm_loanpage(struct vm_page **, int);
static int	uvm_loanobj_read(struct vm_map *, vaddr_t, size_t,
			struct uvm_object *, off_t);


/*
 * inlines
 */

/*
 * uvm_loanentry: loan out pages in a map entry (helper fn for uvm_loan())
 *
 * => "ufi" is the result of a successful map lookup (meaning that
 *	on entry the map is locked by the caller)
 * => we may unlock and then relock the map if needed (for I/O)
 * => we put our output result in "output"
 * => we always return with the map unlocked
 * => possible return values:
 *	-1 == error, map is unlocked
 *	 0 == map relock error (try again!), map is unlocked
 *	>0 == number of pages we loaned, map is unlocked
 *
 * NOTE: We can live with this being an inline, because it is only called
 * from one place.
 */

static inline int
uvm_loanentry(struct uvm_faultinfo *ufi, void ***output, int flags)
{
	vaddr_t curaddr = ufi->orig_rvaddr;
	vsize_t togo = ufi->size;
	struct vm_aref *aref = &ufi->entry->aref;
	struct uvm_object *uobj = ufi->entry->object.uvm_obj;
	struct vm_anon *anon;
	int rv, result = 0;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	/*
	 * lock us the rest of the way down (we unlock before return)
	 */
	if (aref->ar_amap) {
		amap_lock(aref->ar_amap);
	}

	/*
	 * loop until done
	 */
	while (togo) {

		/*
		 * find the page we want.   check the anon layer first.
		 */

		if (aref->ar_amap) {
			anon = amap_lookup(aref, curaddr - ufi->entry->start);
		} else {
			anon = NULL;
		}

		/* locked: map, amap, uobj */
		if (anon) {
			rv = uvm_loananon(ufi, output, flags, anon);
		} else if (uobj) {
			rv = uvm_loanuobj(ufi, output, flags, curaddr);
		} else if (UVM_ET_ISCOPYONWRITE(ufi->entry)) {
			rv = uvm_loanzero(ufi, output, flags);
		} else {
			uvmfault_unlockall(ufi, aref->ar_amap, uobj);
			rv = -1;
		}
		/* locked: if (rv > 0) => map, amap, uobj  [o.w. unlocked] */
		KASSERT(rv > 0 || aref->ar_amap == NULL ||
		    !mutex_owned(aref->ar_amap->am_lock));
		KASSERT(rv > 0 || uobj == NULL ||
		    !mutex_owned(uobj->vmobjlock));

		/* total failure */
		if (rv < 0) {
			UVMHIST_LOG(loanhist, "failure %d", rv, 0,0,0);
			return (-1);
		}

		/* relock failed, need to do another lookup */
		if (rv == 0) {
			UVMHIST_LOG(loanhist, "relock failure %d", result
			    ,0,0,0);
			return (result);
		}

		/*
		 * got it... advance to next page
		 */

		result++;
		togo -= PAGE_SIZE;
		curaddr += PAGE_SIZE;
	}

	/*
	 * unlock what we locked, unlock the maps and return
	 */

	if (aref->ar_amap) {
		amap_unlock(aref->ar_amap);
	}
	uvmfault_unlockmaps(ufi, false);
	UVMHIST_LOG(loanhist, "done %d", result, 0,0,0);
	return (result);
}

/*
 * normal functions
 */

/*
 * uvm_loan: loan pages in a map out to anons or to the kernel
 *
 * => map should be unlocked
 * => start and len should be multiples of PAGE_SIZE
 * => result is either an array of anon's or vm_pages (depending on flags)
 * => flag values: UVM_LOAN_TOANON - loan to anons
 *                 UVM_LOAN_TOPAGE - loan to wired kernel page
 *    one and only one of these flags must be set!
 * => returns 0 (success), or an appropriate error number
 */

int
uvm_loan(struct vm_map *map, vaddr_t start, vsize_t len, void *v, int flags)
{
	struct uvm_faultinfo ufi;
	void **result, **output;
	int rv, error;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	/*
	 * ensure that one and only one of the flags is set
	 */

	KASSERT(((flags & UVM_LOAN_TOANON) == 0) ^
		((flags & UVM_LOAN_TOPAGE) == 0));
	KASSERT((map->flags & VM_MAP_INTRSAFE) == 0);

	/*
	 * "output" is a pointer to the current place to put the loaned page.
	 */

	result = v;
	output = &result[0];	/* start at the beginning ... */

	/*
	 * while we've got pages to do
	 */

	while (len > 0) {

		/*
		 * fill in params for a call to uvmfault_lookup
		 */

		ufi.orig_map = map;
		ufi.orig_rvaddr = start;
		ufi.orig_size = len;

		/*
		 * do the lookup, the only time this will fail is if we hit on
		 * an unmapped region (an error)
		 */

		if (!uvmfault_lookup(&ufi, false)) {
			error = ENOENT;
			goto fail;
		}

		/*
		 * map now locked.  now do the loanout...
		 */

		rv = uvm_loanentry(&ufi, &output, flags);
		if (rv < 0) {
			/* all unlocked due to error */
			error = EINVAL;
			goto fail;
		}

		/*
		 * done!  the map is unlocked.  advance, if possible.
		 *
		 * XXXCDC: could be recoded to hold the map lock with
		 *	   smarter code (but it only happens on map entry
		 *	   boundaries, so it isn't that bad).
		 */

		if (rv) {
			rv <<= PAGE_SHIFT;
			len -= rv;
			start += rv;
		}
	}
	UVMHIST_LOG(loanhist, "success", 0,0,0,0);
	return 0;

fail:
	/*
	 * failed to complete loans.  drop any loans and return failure code.
	 * map is already unlocked.
	 */

	if (output - result) {
		if (flags & UVM_LOAN_TOANON) {
			uvm_unloananon((struct vm_anon **)result,
			    output - result);
		} else {
			uvm_unloanpage((struct vm_page **)result,
			    output - result);
		}
	}
	UVMHIST_LOG(loanhist, "error %d", error,0,0,0);
	return (error);
}

/*
 * uvm_loananon: loan a page from an anon out
 *
 * => called with map, amap, anon locked
 * => return value:
 *	-1 = fatal error, everything is unlocked, abort.
 *	 0 = lookup in ufi went stale, everything unlocked, relookup and
 *		try again
 *	 1 = got it, everything still locked
 */

int
uvm_loananon(struct uvm_faultinfo *ufi, void ***output, int flags,
    struct vm_anon *anon)
{
	struct uvm_cpu *ucpu;
	struct vm_page *pg;
	int error;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	/*
	 * if we are loaning to "another" anon then it is easy, we just
	 * bump the reference count on the current anon and return a
	 * pointer to it (it becomes copy-on-write shared).
	 */

	if (flags & UVM_LOAN_TOANON) {
		KASSERT(mutex_owned(anon->an_lock));
		pg = anon->an_page;
		if (pg && (pg->pqflags & PQ_ANON) != 0 && anon->an_ref == 1) {
			if (pg->wire_count > 0) {
				UVMHIST_LOG(loanhist, "->A wired %p", pg,0,0,0);
				uvmfault_unlockall(ufi,
				    ufi->entry->aref.ar_amap,
				    ufi->entry->object.uvm_obj);
				return (-1);
			}
			pmap_page_protect(pg, VM_PROT_READ);
		}
		anon->an_ref++;
		**output = anon;
		(*output)++;
		UVMHIST_LOG(loanhist, "->A done", 0,0,0,0);
		return (1);
	}

	/*
	 * we are loaning to a kernel-page.   we need to get the page
	 * resident so we can wire it.   uvmfault_anonget will handle
	 * this for us.
	 */

	KASSERT(mutex_owned(anon->an_lock));
	error = uvmfault_anonget(ufi, ufi->entry->aref.ar_amap, anon);

	/*
	 * if we were unable to get the anon, then uvmfault_anonget has
	 * unlocked everything and returned an error code.
	 */

	if (error) {
		UVMHIST_LOG(loanhist, "error %d", error,0,0,0);

		/* need to refault (i.e. refresh our lookup) ? */
		if (error == ERESTART) {
			return (0);
		}

		/* "try again"?   sleep a bit and retry ... */
		if (error == EAGAIN) {
			kpause("loanagain", false, hz/2, NULL);
			return (0);
		}

		/* otherwise flag it as an error */
		return (-1);
	}

	/*
	 * we have the page and its owner locked: do the loan now.
	 */

	pg = anon->an_page;
	mutex_enter(&uvm_pageqlock);
	if (pg->wire_count > 0) {
		mutex_exit(&uvm_pageqlock);
		UVMHIST_LOG(loanhist, "->K wired %p", pg,0,0,0);
		KASSERT(pg->uobject == NULL);
		uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
		return (-1);
	}
	if (pg->loan_count == 0) {
		pmap_page_protect(pg, VM_PROT_READ);
	}
	pg->loan_count++;
	uvm_pageactivate(pg);
	mutex_exit(&uvm_pageqlock);
	**output = pg;
	(*output)++;

	ucpu = uvm_cpu_get();
	if (pg->uobject != NULL) {
		ucpu->loan_oa++;
	} else {
		ucpu->loan_anon++;
	}
	uvm_cpu_put(ucpu);

	UVMHIST_LOG(loanhist, "->K done", 0,0,0,0);
	return (1);
}

/*
 * uvm_loanpage: loan out pages to kernel (->K)
 *
 * => pages should be object-owned and the object should be locked.
 * => in the case of error, the object might be unlocked and relocked.
 * => caller should busy the pages beforehand.
 * => pages will be unbusied.
 * => fail with EBUSY if meet a wired page.
 */
static int
uvm_loanpage(struct vm_page **pgpp, int npages)
{
	struct uvm_cpu *ucpu;
	int i;
	int error = 0;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	for (i = 0; i < npages; i++) {
		struct vm_page *pg = pgpp[i];

		KASSERT(pg->uobject != NULL);
		KASSERT(pg->uobject == pgpp[0]->uobject);
		KASSERT(!(pg->flags & (PG_RELEASED|PG_PAGEOUT)));
		KASSERT(mutex_owned(pg->uobject->vmobjlock));
		KASSERT(pg->flags & PG_BUSY);

		mutex_enter(&uvm_pageqlock);
		if (pg->wire_count > 0) {
			mutex_exit(&uvm_pageqlock);
			UVMHIST_LOG(loanhist, "wired %p", pg,0,0,0);
			error = EBUSY;
			break;
		}
		if (pg->loan_count == 0) {
			pmap_page_protect(pg, VM_PROT_READ);
		}
		pg->loan_count++;
		uvm_pageactivate(pg);
		mutex_exit(&uvm_pageqlock);
	}

	uvm_page_unbusy(pgpp, npages);

	if (i > 0) {
		ucpu = uvm_cpu_get();
		ucpu->loan_obj += i;
		uvm_cpu_put(ucpu);
		if (error) {
			/*
			 * backout what we've done
			 */
			kmutex_t *slock = pgpp[0]->uobject->vmobjlock;

			mutex_exit(slock);
			uvm_unloan(pgpp, i, UVM_LOAN_TOPAGE);
			mutex_enter(slock);
		}
	}

	UVMHIST_LOG(loanhist, "done %d", error,0,0,0);
	return error;
}

/*
 * XXX UBC temp limit
 * number of pages to get at once.
 * should be <= MAX_READ_AHEAD in genfs_vnops.c
 */
#define	UVM_LOAN_GET_CHUNK	16

/*
 * uvm_loanuobjpages: loan pages from a uobj out (O->K)
 *
 * => uobj shouldn't be locked.  (we'll lock it)
 * => fail with EBUSY if we meet a wired page.
 */
int
uvm_loanuobjpages(struct uvm_object *uobj, voff_t pgoff, int orignpages,
    struct vm_page **origpgpp)
{
	int ndone; /* # of pages loaned out */
	struct vm_page **pgpp;
	int error;
	int i;
	kmutex_t *slock;

	pgpp = origpgpp;
	for (ndone = 0; ndone < orignpages; ) {
		int npages;
		/* npendloan: # of pages busied but not loand out yet. */
		int npendloan = 0xdead; /* XXX gcc */
reget:
		npages = MIN(UVM_LOAN_GET_CHUNK, orignpages - ndone);
		mutex_enter(uobj->vmobjlock);
		error = (*uobj->pgops->pgo_get)(uobj,
		    pgoff + (ndone << PAGE_SHIFT), pgpp, &npages, 0,
		    VM_PROT_READ, 0, PGO_SYNCIO);
		if (error == EAGAIN) {
			kpause("loanuopg", false, hz/2, NULL);
			continue;
		}
		if (error)
			goto fail;

		KASSERT(npages > 0);

		/* loan and unbusy pages */
		slock = NULL;
		for (i = 0; i < npages; i++) {
			kmutex_t *nextslock; /* slock for next page */
			struct vm_page *pg = *pgpp;

			/* XXX assuming that the page is owned by uobj */
			KASSERT(pg->uobject != NULL);
			nextslock = pg->uobject->vmobjlock;

			if (slock != nextslock) {
				if (slock) {
					KASSERT(npendloan > 0);
					error = uvm_loanpage(pgpp - npendloan,
					    npendloan);
					mutex_exit(slock);
					if (error)
						goto fail;
					ndone += npendloan;
					KASSERT(origpgpp + ndone == pgpp);
				}
				slock = nextslock;
				npendloan = 0;
				mutex_enter(slock);
			}

			if ((pg->flags & PG_RELEASED) != 0) {
				/*
				 * release pages and try again.
				 */
				mutex_exit(slock);
				for (; i < npages; i++) {
					pg = pgpp[i];
					slock = pg->uobject->vmobjlock;

					mutex_enter(slock);
					mutex_enter(&uvm_pageqlock);
					uvm_page_unbusy(&pg, 1);
					mutex_exit(&uvm_pageqlock);
					mutex_exit(slock);
				}
				goto reget;
			}

			npendloan++;
			pgpp++;
			KASSERT(origpgpp + ndone + npendloan == pgpp);
		}
		KASSERT(slock != NULL);
		KASSERT(npendloan > 0);
		error = uvm_loanpage(pgpp - npendloan, npendloan);
		mutex_exit(slock);
		if (error)
			goto fail;
		ndone += npendloan;
		KASSERT(origpgpp + ndone == pgpp);
	}

	return 0;

fail:
	uvm_unloan(origpgpp, ndone, UVM_LOAN_TOPAGE);

	return error;
}

/*
 * uvm_loanuobj: loan a page from a uobj out
 *
 * => called with map, amap, uobj locked
 * => return value:
 *	-1 = fatal error, everything is unlocked, abort.
 *	 0 = lookup in ufi went stale, everything unlocked, relookup and
 *		try again
 *	 1 = got it, everything still locked
 */

static int
uvm_loanuobj(struct uvm_faultinfo *ufi, void ***output, int flags, vaddr_t va)
{
	struct vm_amap *amap = ufi->entry->aref.ar_amap;
	struct uvm_object *uobj = ufi->entry->object.uvm_obj;
	struct vm_page *pg;
	int error, npages;
	bool locked;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	/*
	 * first we must make sure the page is resident.
	 *
	 * XXXCDC: duplicate code with uvm_fault().
	 */

	/* locked: maps(read), amap(if there) */
	mutex_enter(uobj->vmobjlock);
	/* locked: maps(read), amap(if there), uobj */

	if (uobj->pgops->pgo_get) {	/* try locked pgo_get */
		npages = 1;
		pg = NULL;
		error = (*uobj->pgops->pgo_get)(uobj,
		    va - ufi->entry->start + ufi->entry->offset,
		    &pg, &npages, 0, VM_PROT_READ, MADV_NORMAL, PGO_LOCKED);
	} else {
		error = EIO;		/* must have pgo_get op */
	}

	/*
	 * check the result of the locked pgo_get.  if there is a problem,
	 * then we fail the loan.
	 */

	if (error && error != EBUSY) {
		uvmfault_unlockall(ufi, amap, uobj);
		return (-1);
	}

	/*
	 * if we need to unlock for I/O, do so now.
	 */

	if (error == EBUSY) {
		uvmfault_unlockall(ufi, amap, NULL);

		/* locked: uobj */
		npages = 1;
		error = (*uobj->pgops->pgo_get)(uobj,
		    va - ufi->entry->start + ufi->entry->offset,
		    &pg, &npages, 0, VM_PROT_READ, MADV_NORMAL, PGO_SYNCIO);
		/* locked: <nothing> */

		if (error) {
			if (error == EAGAIN) {
				kpause("fltagain2", false, hz/2, NULL);
				return (0);
			}
			return (-1);
		}

		/*
		 * pgo_get was a success.   attempt to relock everything.
		 */

		locked = uvmfault_relock(ufi);
		if (locked && amap)
			amap_lock(amap);
		uobj = pg->uobject;
		mutex_enter(uobj->vmobjlock);

		/*
		 * verify that the page has not be released and re-verify
		 * that amap slot is still free.   if there is a problem we
		 * drop our lock (thus force a lookup refresh/retry).
		 */

		if ((pg->flags & PG_RELEASED) != 0 ||
		    (locked && amap && amap_lookup(&ufi->entry->aref,
		    ufi->orig_rvaddr - ufi->entry->start))) {
			if (locked)
				uvmfault_unlockall(ufi, amap, NULL);
			locked = false;
		}

		/*
		 * didn't get the lock?   release the page and retry.
		 */

		if (locked == false) {
			if (pg->flags & PG_WANTED) {
				wakeup(pg);
			}
			if (pg->flags & PG_RELEASED) {
				mutex_enter(&uvm_pageqlock);
				uvm_pagefree(pg);
				mutex_exit(&uvm_pageqlock);
				mutex_exit(uobj->vmobjlock);
				return (0);
			}
			mutex_enter(&uvm_pageqlock);
			uvm_pageactivate(pg);
			mutex_exit(&uvm_pageqlock);
			pg->flags &= ~(PG_BUSY|PG_WANTED);
			UVM_PAGE_OWN(pg, NULL);
			mutex_exit(uobj->vmobjlock);
			return (0);
		}
	}

	KASSERT(uobj == pg->uobject);

	/*
	 * at this point we have the page we want ("pg") marked PG_BUSY for us
	 * and we have all data structures locked.  do the loanout.  page can
	 * not be PG_RELEASED (we caught this above).
	 */

	if ((flags & UVM_LOAN_TOANON) == 0) {
		if (uvm_loanpage(&pg, 1)) {
			uvmfault_unlockall(ufi, amap, uobj);
			return (-1);
		}
		mutex_exit(uobj->vmobjlock);
		**output = pg;
		(*output)++;
		return (1);
	}

#ifdef notdef
	/*
	 * must be a loan to an anon.   check to see if there is already
	 * an anon associated with this page.  if so, then just return
	 * a reference to this object.   the page should already be
	 * mapped read-only because it is already on loan.
	 */

	if (pg->uanon) {
		/* XXX: locking */
		anon = pg->uanon;
		anon->an_ref++;
		if (pg->flags & PG_WANTED) {
			wakeup(pg);
		}
		pg->flags &= ~(PG_WANTED|PG_BUSY);
		UVM_PAGE_OWN(pg, NULL);
		mutex_exit(uobj->vmobjlock);
		**output = anon;
		(*output)++;
		return (1);
	}

	/*
	 * need to allocate a new anon
	 */

	anon = uvm_analloc();
	if (anon == NULL) {
		goto fail;
	}
	mutex_enter(&uvm_pageqlock);
	if (pg->wire_count > 0) {
		mutex_exit(&uvm_pageqlock);
		UVMHIST_LOG(loanhist, "wired %p", pg,0,0,0);
		goto fail;
	}
	if (pg->loan_count == 0) {
		pmap_page_protect(pg, VM_PROT_READ);
	}
	pg->loan_count++;
	pg->uanon = anon;
	anon->an_page = pg;
	anon->an_lock = /* TODO: share amap lock */
	uvm_pageactivate(pg);
	mutex_exit(&uvm_pageqlock);
	if (pg->flags & PG_WANTED) {
		wakeup(pg);
	}
	pg->flags &= ~(PG_WANTED|PG_BUSY);
	UVM_PAGE_OWN(pg, NULL);
	mutex_exit(uobj->vmobjlock);
	mutex_exit(&anon->an_lock);
	**output = anon;
	(*output)++;
	return (1);

fail:
	UVMHIST_LOG(loanhist, "fail", 0,0,0,0);
	/*
	 * unlock everything and bail out.
	 */
	if (pg->flags & PG_WANTED) {
		wakeup(pg);
	}
	pg->flags &= ~(PG_WANTED|PG_BUSY);
	UVM_PAGE_OWN(pg, NULL);
	uvmfault_unlockall(ufi, amap, uobj, NULL);
	if (anon) {
		anon->an_ref--;
		uvm_anon_free(anon);
	}
#endif	/* notdef */
	return (-1);
}

/*
 * uvm_loanzero: loan a zero-fill page out
 *
 * => called with map, amap, uobj locked
 * => return value:
 *	-1 = fatal error, everything is unlocked, abort.
 *	 0 = lookup in ufi went stale, everything unlocked, relookup and
 *		try again
 *	 1 = got it, everything still locked
 */

static struct uvm_object uvm_loanzero_object;

static int
uvm_loanzero(struct uvm_faultinfo *ufi, void ***output, int flags)
{
	struct vm_page *pg;
	struct vm_amap *amap = ufi->entry->aref.ar_amap;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);
again:
	mutex_enter(uvm_loanzero_object.vmobjlock);

	/*
	 * first, get ahold of our single zero page.
	 */

	pg = uvm_pagelookup(&uvm_loanzero_object, 0);
	if (__predict_false(pg == NULL)) {
		while ((pg = uvm_pagealloc(&uvm_loanzero_object, 0, NULL,
					   UVM_PGA_ZERO)) == NULL) {
			mutex_exit(uvm_loanzero_object.vmobjlock);
			uvmfault_unlockall(ufi, amap, NULL);
			uvm_wait("loanzero");
			if (!uvmfault_relock(ufi)) {
				return (0);
			}
			if (amap) {
				amap_lock(amap);
			}
			goto again;
		}

		/* got a zero'd page. */
		pg->flags &= ~(PG_WANTED|PG_BUSY|PG_FAKE);
		pg->flags |= PG_RDONLY;
		mutex_enter(&uvm_pageqlock);
		uvm_pageactivate(pg);
		mutex_exit(&uvm_pageqlock);
		UVM_PAGE_OWN(pg, NULL);
	}

	if ((flags & UVM_LOAN_TOANON) == 0) {	/* loaning to kernel-page */
		struct uvm_cpu *ucpu;

		mutex_enter(&uvm_pageqlock);
		pg->loan_count++;
		mutex_exit(&uvm_pageqlock);
		mutex_exit(uvm_loanzero_object.vmobjlock);
		**output = pg;
		(*output)++;
		ucpu = uvm_cpu_get();
		ucpu->loan_zero++;
		uvm_cpu_put(ucpu);
		return (1);
	}

#ifdef notdef
	/*
	 * loaning to an anon.  check to see if there is already an anon
	 * associated with this page.  if so, then just return a reference
	 * to this object.
	 */

	if (pg->uanon) {
		anon = pg->uanon;
		mutex_enter(&anon->an_lock);
		anon->an_ref++;
		mutex_exit(&anon->an_lock);
		mutex_exit(uvm_loanzero_object.vmobjlock);
		**output = anon;
		(*output)++;
		return (1);
	}

	/*
	 * need to allocate a new anon
	 */

	anon = uvm_analloc();
	if (anon == NULL) {
		/* out of swap causes us to fail */
		mutex_exit(uvm_loanzero_object.vmobjlock);
		uvmfault_unlockall(ufi, amap, NULL, NULL);
		return (-1);
	}
	anon->an_page = pg;
	pg->uanon = anon;
	mutex_enter(&uvm_pageqlock);
	pg->loan_count++;
	uvm_pageactivate(pg);
	mutex_exit(&uvm_pageqlock);
	mutex_exit(&anon->an_lock);
	mutex_exit(uvm_loanzero_object.vmobjlock);
	**output = anon;
	(*output)++;
	return (1);
#else
	return (-1);
#endif
}


/*
 * uvm_unloananon: kill loans on anons (basically a normal ref drop)
 *
 * => we expect all our resources to be unlocked
 */

static void
uvm_unloananon(struct vm_anon **aloans, int nanons)
{
#ifdef notdef
	struct vm_anon *anon, *to_free = NULL;

	/* TODO: locking */
	amap_lock(amap);
	while (nanons-- > 0) {
		anon = *aloans++;
		if (--anon->an_ref == 0) {
			anon->an_link = to_free;
			to_free = anon;
		}
	}
	uvm_anon_freelst(amap, to_free);
#endif	/* notdef */
}

/*
 * uvm_unloanpage: kill loans on pages loaned out to the kernel
 *
 * => we expect all our resources to be unlocked
 */

static void
uvm_unloanpage(struct vm_page **ploans, int npages)
{
	struct vm_page *pg;
	kmutex_t *slock;

	mutex_enter(&uvm_pageqlock);
	while (npages-- > 0) {
		struct uvm_object *obj;
		struct vm_anon *anon;
		struct uvm_cpu *ucpu;

		pg = *ploans++;

		/*
		 * do a little dance to acquire the object or anon lock
		 * as appropriate.  we are locking in the wrong order,
		 * so we have to do a try-lock here.
		 */

		slock = NULL;
		while (pg->uobject != NULL || pg->uanon != NULL) {
			if (pg->uobject != NULL) {
				slock = pg->uobject->vmobjlock;
			} else {
				slock = pg->uanon->an_lock;
			}
			if (mutex_tryenter(slock)) {
				break;
			}
			mutex_obj_pause(slock, &uvm_pageqlock);
			slock = NULL;
		}

		obj = pg->uobject;
		anon = pg->uanon;
		/*
		 * drop our loan. (->K)
		 */
		KASSERT(pg->loan_count > 0);
		pg->loan_count--;
		/*
		 * if there are no loans left, put the page back a paging queue
		 * (if the page is owned by an anon) or free it (if the page
		 * is now unowned).
		 */
		uvm_loan_resolve_orphan(pg, true);
		if (pg->loan_count == 0) {
			if (obj == NULL && anon == NULL) {
				KASSERT((pg->flags & PG_BUSY) == 0);
				uvm_pagefree(pg);
			}
			if (anon != NULL) {
				uvm_pageactivate(pg);
			}
		}
		if (slock != NULL) {
			mutex_exit(slock);
		}
		ucpu = uvm_cpu_get();
		if (obj != NULL) {
			if (anon != NULL) {
				ucpu->unloan_oa++;
			} else if (obj == &uvm_loanzero_object) {
				ucpu->unloan_zero++;
			} else {
				ucpu->unloan_obj++;
			}
		} else if (anon != NULL) {
			ucpu->unloan_anon++;
		}
		uvm_cpu_put(ucpu);
	}
	mutex_exit(&uvm_pageqlock);
}

/*
 * uvm_unloan: kill loans on pages or anons.
 */

void
uvm_unloan(void *v, int npages, int flags)
{
	if (flags & UVM_LOAN_TOANON) {
		uvm_unloananon(v, npages);
	} else {
		uvm_unloanpage(v, npages);
	}
}

/*
 * Minimal pager for uvm_loanzero_object.  We need to provide a "put"
 * method, because the page can end up on a paging queue, and the
 * page daemon will want to call pgo_put when it encounters the page
 * on the inactive list.
 */

static int
ulz_put(struct uvm_object *uobj, voff_t start, voff_t stop, int flags)
{
	struct vm_page *pg;

	KDASSERT(uobj == &uvm_loanzero_object);

	/*
	 * Don't need to do any work here if we're not freeing pages.
	 */

	if ((flags & PGO_FREE) == 0) {
		mutex_exit(uobj->vmobjlock);
		return 0;
	}

	/*
	 * we don't actually want to ever free the uvm_loanzero_page, so
	 * just reactivate or dequeue it.
	 */

	pg = uvm_pagelookup(uobj, 0);
	KASSERT(pg != NULL);
	KASSERT(uobj->uo_npages == 1);

	mutex_enter(&uvm_pageqlock);
	if (pg->uanon)
		uvm_pageactivate(pg);
	else
		uvm_pagedequeue(pg);
	mutex_exit(&uvm_pageqlock);

	mutex_exit(uobj->vmobjlock);
	return 0;
}

static const struct uvm_pagerops ulz_pager = {
	.pgo_put = ulz_put,
};

/*
 * uvm_loan_init(): initialize the uvm_loan() facility.
 */

void
uvm_loan_init(void)
{

	uvm_obj_init(&uvm_loanzero_object, &ulz_pager, true, 0);
	UVMHIST_INIT(loanhist, 300);
}

/*
 * uvm_loanbreak: break loan on a uobj page
 *
 * => called with uobj locked
 * => the page should be busy
 * => return value:
 *	newly allocated page if succeeded
 */
struct vm_page *
uvm_loanbreak(struct vm_page *uobjpage)
{
	struct uvm_cpu *ucpu;
	struct vm_page *pg;
#ifdef DIAGNOSTIC
	struct uvm_object *uobj = uobjpage->uobject;
#endif
	struct vm_anon * const anon = uobjpage->uanon;
	const unsigned int count = uobjpage->loan_count;

	KASSERT(uobj != NULL);
	KASSERT(mutex_owned(uobj->vmobjlock));
	KASSERT(uobjpage->flags & PG_BUSY);
	KASSERT(count > 0);

	/* alloc new un-owned page */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL)
		return NULL;

	/*
	 * copy the data from the old page to the new
	 * one and clear the fake flags on the new page (keep it busy).
	 * force a reload of the old page by clearing it from all
	 * pmaps.
	 * then lock the page queues to rename the pages.
	 */

	uvm_pagecopy(uobjpage, pg);	/* old -> new */
	pg->flags &= ~PG_FAKE;
	KASSERT(uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_DIRTY);
	pmap_page_protect(uobjpage, VM_PROT_NONE);
	if (uobjpage->flags & PG_WANTED)
		wakeup(uobjpage);
	/* uobj still locked */
	uobjpage->flags &= ~(PG_WANTED|PG_BUSY);
	UVM_PAGE_OWN(uobjpage, NULL);

	mutex_enter(&uvm_pageqlock);

	/*
	 * if the page is no longer referenced by an anon (i.e. we are breaking
	 * O->K loans), then remove it from any pageq's.
	 */
	if (anon == NULL)
		uvm_pagedequeue(uobjpage);

	/*
	 * replace uobjpage with new page.
	 *
	 * this will update the page dirtiness statistics.
	 */

	uvm_pagereplace(uobjpage, pg);

	/*
	 * at this point we have absolutely no control over uobjpage
	 */

	/* install new page */
	uvm_pageactivate(pg);
	mutex_exit(&uvm_pageqlock);

	/*
	 * update statistics.
	 */
	ucpu = uvm_cpu_get();
	if (anon != NULL) {
		ucpu->loanbreak_oa_obj++;
		ucpu->loanbreak_orphaned += count - 1;
	} else {
		ucpu->loanbreak_obj += count;
	}
	uvm_cpu_put(ucpu);

	/*
	 * done!  loan is broken and "pg" is PG_BUSY.
	 * it can now replace uobjpage.
	 */
	return pg;
}

/*
 * uvm_loanbreak_anon:
 */

int
uvm_loanbreak_anon(struct vm_anon *anon)
{
	struct uvm_cpu *ucpu;
	struct vm_page *pg;
	unsigned int oldstatus;
	struct uvm_object * const uobj = anon->an_page->uobject;
	const unsigned int count = anon->an_page->loan_count;

	KASSERT(mutex_owned(anon->an_lock));
	KASSERT(uobj == NULL || mutex_owned(uobj->vmobjlock));
	KASSERT(count > 0);

	/* get new un-owned replacement page */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL) {
		return ENOMEM;
	}

	/* copy old -> new */
	uvm_pagecopy(anon->an_page, pg);
	KASSERT(uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_DIRTY);

	/* force reload */
	pmap_page_protect(anon->an_page, VM_PROT_NONE);
	oldstatus = uvm_pagegetdirty(anon->an_page);
	mutex_enter(&uvm_pageqlock);	  /* KILL loan */

	anon->an_page->uanon = NULL;
	if (uobj != NULL) {
		/*
		 * if we were receiver of loan (O->A)
		 */
		KASSERT((anon->an_page->pqflags & PQ_ANON) == 0);
		anon->an_page->loan_count--;
	} else {
		/*
		 * we were the lender (A->K); need to remove the page from
		 * pageq's.
		 *
		 * PQ_ANON is updated by the caller.
		 */
		KASSERT((anon->an_page->pqflags & PQ_ANON) != 0);
		anon->an_page->pqflags &= ~PQ_ANON;
		uvm_pagedequeue(anon->an_page);
	}

	/* install new page in anon */
	anon->an_page = pg;
	pg->uanon = anon;
	pg->pqflags |= PQ_ANON;

	uvm_pageactivate(pg);
	mutex_exit(&uvm_pageqlock);

	pg->flags &= ~(PG_BUSY|PG_FAKE);
	UVM_PAGE_OWN(pg, NULL);

	/* done! */
	ucpu = uvm_cpu_get();
	if (uobj != NULL) {
		ucpu->loanbreak_oa_anon++;
		ucpu->loanbreak_orphaned_anon += count - 1;
		atomic_inc_uint(&uvmexp.anonpages);
	} else {
		ucpu->loanbreak_anon += count;
		ucpu->pagestate[1][oldstatus]--;
	}
	ucpu->pagestate[1][UVM_PAGE_STATUS_DIRTY]++;
	uvm_cpu_put(ucpu);
	return 0;
}

int
uvm_loanobj(struct uvm_object *uobj, struct uio *uio)
{
	struct iovec *iov;
	struct vm_map *map;
	vaddr_t va;
	size_t len;
	int i, error = 0;

	if (!vm_loan_read) {
		return ENOSYS;
	}

	/*
	 * This interface is only for loaning to user space.
	 * Loans to the kernel should be done with the kernel-specific
	 * loaning interfaces.
	 */

	if (VMSPACE_IS_KERNEL_P(uio->uio_vmspace)) {
		return ENOSYS;
	}

	if (uio->uio_rw != UIO_READ) {
		return ENOSYS;
	}

	/*
	 * Check that the uio is aligned properly for loaning.
	 */

	if (uio->uio_offset & PAGE_MASK || uio->uio_resid & PAGE_MASK) {
		return EINVAL;
	}
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (((vaddr_t)uio->uio_iov[i].iov_base & PAGE_MASK) ||
		    (uio->uio_iov[i].iov_len & PAGE_MASK)) {
			return EINVAL;
		}
	}

	/*
	 * Process the uio.
	 */

	map = &uio->uio_vmspace->vm_map;
	while (uio->uio_resid) {
		iov = uio->uio_iov;
		while (iov->iov_len) {
			va = (vaddr_t)iov->iov_base;
			len = MIN(iov->iov_len, MAXPHYS);
			error = uvm_loanobj_read(map, va, len, uobj,
						 uio->uio_offset);
			if (error) {
				goto out;
			}
			iov->iov_base = (char *)iov->iov_base + len;
			iov->iov_len -= len;
			uio->uio_offset += len;
			uio->uio_resid -= len;
		}
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

out:
	pmap_update(map->pmap);
	return error;
}

/*
 * Loan object pages to a user process.
 */

/* XXX an arbitrary number larger than MAXPHYS/PAGE_SIZE */
#define	MAXPAGES	16

static int
uvm_loanobj_read(struct vm_map *map, vaddr_t va, size_t len,
    struct uvm_object *uobj, off_t off)
{
	unsigned int npages = len >> PAGE_SHIFT;
	struct vm_page *pgs[MAXPAGES];
	struct vm_amap *amap;
	struct vm_anon *anon, *oanons[MAXPAGES], *nanons[MAXPAGES];
	struct vm_map_entry *entry;
	struct vm_anon *anon_tofree;
	unsigned int maptime;
	unsigned int i, refs, aoff, pgoff;
	unsigned int loaned; /* # of newly created O->A loan */
	int error;
	UVMHIST_FUNC("uvm_vnp_loanread"); UVMHIST_CALLED(ubchist);

	UVMHIST_LOG(ubchist, "map %p va 0x%x npages %d", map, va, npages, 0);
	UVMHIST_LOG(ubchist, "uobj %p off 0x%x", uobj, off, 0, 0);

	if (npages > MAXPAGES) {
		return EINVAL;
	}
#if defined(PMAP_PREFER)
	/*
	 * avoid creating VAC aliases.
	 */
	{
		const vaddr_t origva = va;

		PMAP_PREFER(off, &va, len, 0);
		if (va != origva) {
			/*
			 * pmap's suggestion was different from the requested
			 * address.  punt.
			 */
			return EINVAL;
		}
	}
#endif /* defined(PMAP_PREFER) */
retry:
	vm_map_lock_read(map);
	if (!uvm_map_lookup_entry(map, va, &entry)) {
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "no entry", 0,0,0,0);
		return EINVAL;
	}
	if ((entry->protection & VM_PROT_WRITE) == 0) {
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "no write access", 0,0,0,0);
		return EACCES;
	}
	if (VM_MAPENT_ISWIRED(entry)) {
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "entry is wired", 0,0,0,0);
		return EBUSY;
	}
	if (!UVM_ET_ISCOPYONWRITE(entry)) {
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "entry is not COW", 0,0,0,0);
		return EINVAL;
	}
	if (UVM_ET_ISOBJ(entry)) {
		/*
		 * avoid locking order difficulty between
		 * am_obj_lock and backing object's lock.
		 */
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "entry is obj backed", 0,0,0,0);
		return EINVAL;
	}
	if (entry->end < va + len) {
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "chunk longer than entry", 0,0,0,0);
		return EINVAL;
	}
	amap = entry->aref.ar_amap;
	if (amap != NULL && (amap->am_flags & AMAP_SHARED) != 0) {
		vm_map_unlock_read(map);
		UVMHIST_LOG(ubchist, "amap is shared", 0,0,0,0);
		return EINVAL;
	}

	/*
	 * None of the trivial reasons why we might not be able to do the loan
	 * are true.  If we need to COW the amap, try to do it now.
	 */

	KASSERT(amap || UVM_ET_ISNEEDSCOPY(entry));
	if (UVM_ET_ISNEEDSCOPY(entry)) {
		maptime = map->timestamp;
		vm_map_unlock_read(map);
		vm_map_lock(map);
		if (maptime + 1 != map->timestamp) {
			vm_map_unlock(map);
			goto retry;
		}
		amap_copy(map, entry, 0, va, va + len);
		if (UVM_ET_ISNEEDSCOPY(entry)) {
			vm_map_unlock(map);
			UVMHIST_LOG(ubchist, "amap COW failed", 0,0,0,0);
			return ENOMEM;
		}
		UVMHIST_LOG(ubchist, "amap has been COWed", 0,0,0,0);
		aoff = va - entry->start;
		maptime = map->timestamp;
		vm_map_unlock(map);
	} else {
		aoff = va - entry->start;
		maptime = map->timestamp;
		vm_map_unlock_read(map);
	}

	/*
	 * The map is all ready for us, now fetch the obj pages.
	 * If the map changes out from under us, start over.
	 *
	 * XXX worth trying PGO_LOCKED?
	 */

	memset(pgs, 0, sizeof(pgs));
	mutex_enter(uobj->vmobjlock);
	error = (*uobj->pgops->pgo_get)(uobj, off, pgs, &npages, 0,
	    VM_PROT_READ, 0, PGO_SYNCIO);
	if (error) {
		UVMHIST_LOG(ubchist, "getpages -> %d", error,0,0,0);
		return error;
	}
	vm_map_lock_read(map);
	if (map->timestamp != maptime) {
		vm_map_unlock_read(map);
		mutex_enter(uobj->vmobjlock);
		mutex_enter(&uvm_pageqlock);
		for (i = 0; i < npages; i++) {
			uvm_pageactivate(pgs[i]);
		}
		uvm_page_unbusy(pgs, npages);
		mutex_exit(&uvm_pageqlock);
		mutex_exit(uobj->vmobjlock);
		goto retry;
	}
	amap = entry->aref.ar_amap;
	KASSERT(amap != NULL);

	/*
	 * Prepare each object page for loaning.  Allocate an anon for each page
	 * that doesn't already have one.  If any of the pages are wired,
	 * undo everything and fail.
	 */

	memset(nanons, 0, sizeof(nanons));
	amap_lock(amap);
	if (amap->am_obj_lock != NULL) {
		if (amap->am_obj_lock != uobj->vmobjlock) {
			/*
			 * the amap might already have pages loaned from
			 * another object.  give up.
			 *
			 * XXX worth clipping amap?
			 */
			error = EBUSY;
			amap_unlock(amap);
			amap = NULL;
			mutex_enter(uobj->vmobjlock);
			goto fail_amap_unlocked;
		}
	} else {
		mutex_enter(uobj->vmobjlock);
	}
	KASSERT(mutex_owned(amap->am_lock));
	KASSERT(mutex_owned(uobj->vmobjlock));
	loaned = 0;
	for (i = 0; i < npages; i++) {
		struct vm_page * const pg = pgs[i];
		KASSERT(uvm_page_locked_p(pg));
		if (pg->wire_count) {
			error = EBUSY;
			goto fail;
		}
		pmap_page_protect(pg, VM_PROT_READ);
		mutex_enter(&uvm_pageqlock);
		uvm_pageactivate(pgs[i]);
		mutex_exit(&uvm_pageqlock);
		if (pg->uanon != NULL) {
			KASSERTMSG(pg->loan_count > 0, "pg %p loan_count %u",
			    pg, (unsigned int)pg->loan_count);
			anon = pg->uanon;
			if (anon->an_lock != amap->am_lock) {
				/*
				 * the page is already loaned to another amap
				 * whose lock is incompatible with ours.
				 * give up.
				 */
				error = EBUSY;
				goto fail;
			}
			anon->an_ref++;
		} else {
			anon = uvm_analloc();
			if (anon == NULL) {
				error = ENOMEM;
				goto fail;
			}
			mutex_enter(&uvm_pageqlock);
			anon->an_page = pg;
			pg->uanon = anon;
			pg->loan_count++;
			mutex_exit(&uvm_pageqlock);
			loaned++;
		}
		nanons[i] = anon;
	}

	/*
	 * Look for any existing anons in the amap.  These will be replaced
	 * by the new loan anons we just set up.  If any of these anon pages
	 * are wired then we can't replace them.
	 */

	memset(oanons, 0, sizeof(oanons));
	for (i = 0; i < npages; i++) {
		UVMHIST_LOG(ubchist, "pgs[%d] %p", i, pgs[i], 0,0);
		anon = amap_lookup(&entry->aref, aoff + (i << PAGE_SHIFT));
		oanons[i] = anon;
		if (anon && anon->an_page && anon->an_page->wire_count) {
			error = EBUSY;
			goto fail;
		}
	}

	/*
	 * Everything is good to go.  Remove any existing anons and insert
	 * the loaned object anons.
	 */

	anon_tofree = NULL;
	for (i = 0; i < npages; i++) {
		pgoff = i << PAGE_SHIFT;
		anon = oanons[i];
		if (anon != NULL) {
			amap_unadd(&entry->aref, aoff + pgoff);
			refs = --anon->an_ref;
			if (refs == 0) {
				anon->an_link = anon_tofree;
				anon_tofree = anon;
			}
		}
		anon = nanons[i];
		if (anon->an_lock == NULL) {
			anon->an_lock = amap->am_lock;
		}
		amap_add(&entry->aref, aoff + pgoff, anon, FALSE);
	}

	/*
	 * The map has all the new information now.
	 * Enter the pages into the pmap to save likely faults later.
	 */

	for (i = 0; i < npages; i++) {
		error = pmap_enter(map->pmap, va + (i << PAGE_SHIFT),
		    VM_PAGE_TO_PHYS(pgs[i]), VM_PROT_READ, PMAP_CANFAIL);
		if (error != 0) {
			/*
			 * while the failure of pmap_enter here is not critical,
			 * we should not leave the mapping of the oanon page.
			 */
			pmap_remove(map->pmap, va + (i << PAGE_SHIFT),
			    va + (i << PAGE_SHIFT) + PAGE_SIZE);
		}
	}

	/*
	 * At this point we're done with the pages, unlock them now.
	 */

	mutex_enter(&uvm_pageqlock);
	uvm_page_unbusy(pgs, npages);
	mutex_exit(&uvm_pageqlock);
	if (amap->am_obj_lock == NULL) {
		mutex_obj_hold(uobj->vmobjlock);
		amap->am_obj_lock = uobj->vmobjlock;
	} else {
		KASSERT(amap->am_obj_lock == uobj->vmobjlock);
	}
	uvm_anon_freelst(amap, anon_tofree);
	vm_map_unlock_read(map);

	/*
	 * update statistics
	 */
	if (loaned) {
		struct uvm_cpu *ucpu;

		ucpu = uvm_cpu_get();
		ucpu->loan_obj_read += loaned;
		uvm_cpu_put(ucpu);
	}
	return 0;

	/*
	 * We couldn't complete the loan for some reason.
	 * Undo any work we did so far.
	 */

fail:
	KASSERT(mutex_owned(amap->am_lock));
fail_amap_unlocked:
	KASSERT(mutex_owned(uobj->vmobjlock));
	for (i = 0; i < npages; i++) {
		anon = nanons[i];
		if (anon != NULL) {
			KASSERT(amap != NULL);
			KASSERT(uvm_page_locked_p(anon->an_page));
			if (anon->an_lock == NULL) {
				struct vm_page * const pg = anon->an_page;

				KASSERT(anon->an_ref == 1);
				KASSERT(pg != NULL);
				KASSERT(pg->loan_count > 0);
				KASSERT(pg->uanon == anon);
				mutex_enter(&uvm_pageqlock);
				pg->loan_count--;
				pg->uanon = NULL;
				anon->an_page = NULL;
				mutex_exit(&uvm_pageqlock);
				anon->an_ref--;
				uvm_anon_free(anon);
			} else {
				KASSERT(anon->an_lock == amap->am_lock);
				KASSERT(anon->an_page->loan_count > 0);
				KASSERT(anon->an_ref > 1);
				anon->an_ref--;
			}
		} else {
			mutex_enter(&uvm_pageqlock);
			uvm_pageenqueue(pgs[i]);
			mutex_exit(&uvm_pageqlock);
		}
	}
	mutex_enter(&uvm_pageqlock);
	uvm_page_unbusy(pgs, npages);
	mutex_exit(&uvm_pageqlock);
	if (amap != NULL) {
		if (amap->am_obj_lock == NULL) {
			mutex_exit(uobj->vmobjlock);
		}
		amap_unlock(amap);
	} else {
		mutex_exit(uobj->vmobjlock);
	}
	vm_map_unlock_read(map);
	return error;
}

/*
 * uvm_loan_resolve_orphan: update the state of the page after a possible
 * ownership change
 *
 * if page is owned by an anon but PQ_ANON is not set, the page was loaned
 * to the anon from an object which dropped ownership, so resolve this by
 * turning the anon's loan into real ownership (ie. decrement loan_count
 * again and set PQ_ANON).
 */

void
uvm_loan_resolve_orphan(struct vm_page *pg, bool pageqlocked)
{
	struct uvm_object * const uobj = pg->uobject;
	struct vm_anon * const anon = pg->uanon;
	struct uvm_cpu *ucpu;

	KASSERT(!pageqlocked || mutex_owned(&uvm_pageqlock));
	KASSERT(uvm_page_locked_p(pg));
	if (uobj != NULL) {
		return;
	}
	if (anon == NULL) {
		return;
	}
	if ((pg->pqflags & PQ_ANON) != 0) {
		return;
	}
	KASSERT(pg->loan_count > 0);
	uvm_pagemarkdirty(pg, UVM_PAGE_STATUS_DIRTY);
	if (!pageqlocked) {
		mutex_enter(&uvm_pageqlock);
	}
	pg->loan_count--;
	pg->pqflags |= PQ_ANON;
	if (!pageqlocked) {
		mutex_exit(&uvm_pageqlock);
	}

	/*
	 * adjust statistics after the owner change.
	 *
	 * the pagestate should have been decremented when uobj dropped the
	 * ownership.
	 */
	ucpu = uvm_cpu_get();
	ucpu->loan_resolve_orphan++;
	ucpu->pagestate[1][UVM_PAGE_STATUS_DIRTY]++;
	uvm_cpu_put(ucpu);
	atomic_inc_uint(&uvmexp.anonpages);
}
