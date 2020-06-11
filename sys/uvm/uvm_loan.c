/*	$NetBSD: uvm_loan.c,v 1.104 2020/06/11 22:21:05 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uvm_loan.c,v 1.104 2020/06/11 22:21:05 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mman.h>

#include <uvm/uvm.h>

#ifdef UVMHIST
UVMHIST_DEFINE(loanhist);
#endif

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
 * a uvm_object and a vm_anon, but PG_ANON will not be set.   this sort
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
 * locking: to read pg->loan_count either the owner or pg->interlock
 * must be locked.   to modify pg->loan_count, both the owner of the page
 * and pg->interlock must be locked.   pg->flags is (as always) locked by
 * the owner of the page.
 *
 * note that locking from the "loaned" side is tricky since the object
 * getting the loaned page has no reference to the page's owner and thus
 * the owner could "die" at any time.   in order to prevent the owner
 * from dying pg->interlock should be locked.   this forces us to sometimes
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
static int	uvm_loanpage(struct vm_page **, int, bool);


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
		amap_lock(aref->ar_amap, RW_WRITER);
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
		    !rw_write_held(aref->ar_amap->am_lock));
		KASSERT(rv > 0 || uobj == NULL ||
		    !rw_write_held(uobj->vmobjlock));

		/* total failure */
		if (rv < 0) {
			UVMHIST_LOG(loanhist, "failure %jd", rv, 0,0,0);
			return (-1);
		}

		/* relock failed, need to do another lookup */
		if (rv == 0) {
			UVMHIST_LOG(loanhist, "relock failure %jd", result
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
	UVMHIST_LOG(loanhist, "done %jd", result, 0,0,0);
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
	UVMHIST_LOG(loanhist, "error %jd", error,0,0,0);
	return (error);
}

/*
 * uvm_loananon: loan a page from an anon out
 *
 * => called with map, amap, uobj locked
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
	struct vm_page *pg;
	int error;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	/*
	 * if we are loaning to "another" anon then it is easy, we just
	 * bump the reference count on the current anon and return a
	 * pointer to it (it becomes copy-on-write shared).
	 */

	if (flags & UVM_LOAN_TOANON) {
		KASSERT(rw_write_held(anon->an_lock));
		pg = anon->an_page;
		if (pg && (pg->flags & PG_ANON) != 0 && anon->an_ref == 1) {
			if (pg->wire_count > 0) {
				UVMHIST_LOG(loanhist, "->A wired %#jx",
				    (uintptr_t)pg, 0, 0, 0);
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

	KASSERT(rw_write_held(anon->an_lock));
	error = uvmfault_anonget(ufi, ufi->entry->aref.ar_amap, anon);

	/*
	 * if we were unable to get the anon, then uvmfault_anonget has
	 * unlocked everything and returned an error code.
	 */

	if (error) {
		UVMHIST_LOG(loanhist, "error %jd", error,0,0,0);
		KASSERT(error != ENOLCK);

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
	if (pg->wire_count > 0) {
		UVMHIST_LOG(loanhist, "->K wired %#jx", (uintptr_t)pg, 0, 0, 0);
		KASSERT(pg->uobject == NULL);
		uvmfault_unlockall(ufi, ufi->entry->aref.ar_amap, NULL);
		return (-1);
	}
	if (pg->loan_count == 0) {
		pmap_page_protect(pg, VM_PROT_READ);
	}
	uvm_pagelock(pg);
	pg->loan_count++;
	KASSERT(pg->loan_count > 0);	/* detect wrap-around */
	uvm_pageactivate(pg);
	uvm_pageunlock(pg);
	**output = pg;
	(*output)++;

	/* unlock and return success */
	if (pg->uobject)
		rw_exit(pg->uobject->vmobjlock);
	UVMHIST_LOG(loanhist, "->K done", 0,0,0,0);
	return (1);
}

/*
 * uvm_loanpage: loan out pages to kernel (->K)
 *
 * => pages should be object-owned and the object should be locked.
 * => in the case of error, the object might be unlocked and relocked.
 * => pages will be unbusied (if busied is true).
 * => fail with EBUSY if meet a wired page.
 */
static int
uvm_loanpage(struct vm_page **pgpp, int npages, bool busied)
{
	int i;
	int error = 0;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);

	for (i = 0; i < npages; i++) {
		struct vm_page *pg = pgpp[i];

		KASSERT(pg->uobject != NULL);
		KASSERT(pg->uobject == pgpp[0]->uobject);
		KASSERT(!(pg->flags & (PG_RELEASED|PG_PAGEOUT)));
		KASSERT(rw_write_held(pg->uobject->vmobjlock));
		KASSERT(busied == ((pg->flags & PG_BUSY) != 0));

		if (pg->wire_count > 0) {
			UVMHIST_LOG(loanhist, "wired %#jx", (uintptr_t)pg,
			    0, 0, 0);
			error = EBUSY;
			break;
		}
		if (pg->loan_count == 0) {
			pmap_page_protect(pg, VM_PROT_READ);
		}
		uvm_pagelock(pg);
		pg->loan_count++;
		KASSERT(pg->loan_count > 0);	/* detect wrap-around */
		uvm_pageactivate(pg);
		uvm_pageunlock(pg);
	}

	if (busied) {
		uvm_page_unbusy(pgpp, npages);
	}

	if (error) {
		/*
		 * backout what we've done
		 */
		krwlock_t *slock = pgpp[0]->uobject->vmobjlock;

		rw_exit(slock);
		uvm_unloan(pgpp, i, UVM_LOAN_TOPAGE);
		rw_enter(slock, RW_WRITER);
	}

	UVMHIST_LOG(loanhist, "done %jd", error, 0, 0, 0);
	return error;
}

/*
 * XXX UBC temp limit
 * number of pages to get at once.
 * should be <= MAX_READ_AHEAD in genfs_vnops.c
 */
#define	UVM_LOAN_GET_CHUNK	16

/*
 * uvm_loanuobjchunk: helper for uvm_loanuobjpages()
 */
static int
uvm_loanuobjchunk(struct uvm_object *uobj, voff_t pgoff, int orignpages,
    struct vm_page **pgpp)
{
	int error, npages;

	rw_enter(uobj->vmobjlock, RW_WRITER);
 reget:
 	npages = orignpages;
	error = (*uobj->pgops->pgo_get)(uobj, pgoff, pgpp, &npages, 0,
	    VM_PROT_READ, 0, PGO_SYNCIO);
	switch (error) {
	case 0:
		KASSERT(npages == orignpages);

		/* check for released pages */
		rw_enter(uobj->vmobjlock, RW_WRITER);
		for (int i = 0; i < npages; i++) {
			KASSERT(pgpp[i]->uobject->vmobjlock == uobj->vmobjlock);
			if ((pgpp[i]->flags & PG_RELEASED) != 0) {
				/*
				 * release pages and try again.
				 */
				uvm_page_unbusy(pgpp, npages);
				goto reget;
			}
		}

		/* loan out pages.  they will be unbusied whatever happens. */
		error = uvm_loanpage(pgpp, npages, true);
		rw_exit(uobj->vmobjlock);
		if (error != 0) {
			memset(pgpp, 0, sizeof(pgpp[0]) * npages);
		}
		return error;

	case EAGAIN:
		kpause("loanuopg", false, hz/2, NULL);
		rw_enter(uobj->vmobjlock, RW_WRITER);
		goto reget;

	default:
		return error;
	}
}

/*
 * uvm_loanuobjpages: loan pages from a uobj out (O->K)
 *
 * => uobj shouldn't be locked.  (we'll lock it)
 * => fail with EBUSY if we meet a wired page.
 */
int
uvm_loanuobjpages(struct uvm_object *uobj, voff_t pgoff, int npages,
    struct vm_page **pgpp)
{
	int ndone, error, chunk;

	KASSERT(npages > 0);

	memset(pgpp, 0, sizeof(pgpp[0]) * npages);
	for (ndone = 0; ndone < npages; ndone += chunk) {
		chunk = MIN(UVM_LOAN_GET_CHUNK, npages - ndone);
		error = uvm_loanuobjchunk(uobj, pgoff + (ndone << PAGE_SHIFT),
		    chunk, pgpp + ndone);
		if (error != 0) {
			if (ndone != 0) {
				uvm_unloan(pgpp, ndone, UVM_LOAN_TOPAGE);
			}
			break;
		}
	}

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
	rw_enter(uobj->vmobjlock, RW_WRITER);
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
			amap_lock(amap, RW_WRITER);
		uobj = pg->uobject;
		rw_enter(uobj->vmobjlock, RW_WRITER);

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
		 * unbusy the page.
		 */

		if ((pg->flags & PG_RELEASED) == 0) {
			uvm_pagelock(pg);
			uvm_pagewakeup(pg);
			uvm_pageunlock(pg);
			pg->flags &= ~PG_BUSY;
			UVM_PAGE_OWN(pg, NULL);
		}

		/*
		 * didn't get the lock?   release the page and retry.
		 */

 		if (locked == false) {
			if (pg->flags & PG_RELEASED) {
				uvm_pagefree(pg);
			}
			rw_exit(uobj->vmobjlock);
			return (0);
		}
	}

	/*
	 * for tmpfs vnodes, the page will be from a UAO rather than
	 * the vnode.  just check the locks match.
	 */

	KASSERT(uobj->vmobjlock == pg->uobject->vmobjlock);

	/*
	 * at this point we have the page we want ("pg") and we have
	 * all data structures locked.  do the loanout.  page can not
	 * be PG_RELEASED (we caught this above).
	 */

	if ((flags & UVM_LOAN_TOANON) == 0) {
		if (uvm_loanpage(&pg, 1, false)) {
			uvmfault_unlockall(ufi, amap, uobj);
			return (-1);
		}
		rw_exit(uobj->vmobjlock);
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
		uvm_pagelock(pg);
		uvm_pagewakeup(pg);
		uvm_pageunlock(pg);
		pg->flags &= ~PG_BUSY;
		UVM_PAGE_OWN(pg, NULL);
		rw_exit(uobj->vmobjlock);
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
	if (pg->wire_count > 0) {
		UVMHIST_LOG(loanhist, "wired %#jx", (uintptr_t)pg, 0, 0, 0);
		goto fail;
	}
	if (pg->loan_count == 0) {
		pmap_page_protect(pg, VM_PROT_READ);
	}
	uvm_pagelock(pg);
	pg->loan_count++;
	KASSERT(pg->loan_count > 0);	/* detect wrap-around */
	pg->uanon = anon;
	anon->an_page = pg;
	anon->an_lock = /* TODO: share amap lock */
	uvm_pageactivate(pg);
	uvm_pagewakeup(pg);
	uvm_pageunlock(pg);
	pg->flags &= ~PG_BUSY;
	UVM_PAGE_OWN(pg, NULL);
	rw_exit(uobj->vmobjlock);
	rw_exit(&anon->an_lock);
	**output = anon;
	(*output)++;
	return (1);

fail:
	UVMHIST_LOG(loanhist, "fail", 0,0,0,0);
	/*
	 * unlock everything and bail out.
	 */
	uvm_pagelock(pg);
	uvm_pagewakeup(pg);
	uvm_pageunlock(pg);
	pg->flags &= ~PG_BUSY;
	UVM_PAGE_OWN(pg, NULL);
	uvmfault_unlockall(ufi, amap, uobj, NULL);
	if (anon) {
		anon->an_ref--;
		uvm_anfree(anon);
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
static krwlock_t uvm_loanzero_lock __cacheline_aligned;

static int
uvm_loanzero(struct uvm_faultinfo *ufi, void ***output, int flags)
{
	struct vm_page *pg;
	struct vm_amap *amap = ufi->entry->aref.ar_amap;

	UVMHIST_FUNC(__func__); UVMHIST_CALLED(loanhist);
again:
	rw_enter(uvm_loanzero_object.vmobjlock, RW_WRITER);

	/*
	 * first, get ahold of our single zero page.
	 */

	pg = uvm_pagelookup(&uvm_loanzero_object, 0);
	if (__predict_false(pg == NULL)) {
		while ((pg = uvm_pagealloc(&uvm_loanzero_object, 0, NULL,
					   UVM_PGA_ZERO)) == NULL) {
			rw_exit(uvm_loanzero_object.vmobjlock);
			uvmfault_unlockall(ufi, amap, NULL);
			uvm_wait("loanzero");
			if (!uvmfault_relock(ufi)) {
				return (0);
			}
			if (amap) {
				amap_lock(amap, RW_WRITER);
			}
			goto again;
		}

		/* got a zero'd page. */
		pg->flags &= ~(PG_BUSY|PG_FAKE);
		pg->flags |= PG_RDONLY;
		uvm_pagelock(pg);
		uvm_pageactivate(pg);
		uvm_pagewakeup(pg);
		uvm_pageunlock(pg);
		UVM_PAGE_OWN(pg, NULL);
	}

	if ((flags & UVM_LOAN_TOANON) == 0) {	/* loaning to kernel-page */
		mutex_enter(&pg->interlock);
		pg->loan_count++;
		KASSERT(pg->loan_count > 0);	/* detect wrap-around */
		mutex_exit(&pg->interlock);
		rw_exit(uvm_loanzero_object.vmobjlock);
		**output = pg;
		(*output)++;
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
		rw_enter(&anon->an_lock, RW_WRITER);
		anon->an_ref++;
		rw_exit(&anon->an_lock);
		rw_exit(uvm_loanzero_object.vmobjlock);
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
		rw_exit(uvm_loanzero_object.vmobjlock);
		uvmfault_unlockall(ufi, amap, NULL, NULL);
		return (-1);
	}
	anon->an_page = pg;
	pg->uanon = anon;
	uvm_pagelock(pg);
	pg->loan_count++;
	KASSERT(pg->loan_count > 0);	/* detect wrap-around */
	uvm_pageactivate(pg);
	uvm_pageunlock(pg);
	rw_exit(&anon->an_lock);
	rw_exit(uvm_loanzero_object.vmobjlock);
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
	amap_lock(amap, RW_WRITER);
	while (nanons-- > 0) {
		anon = *aloans++;
		if (--anon->an_ref == 0) {
			uvm_anfree(anon);
		}
	}
	amap_unlock(amap);
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
	krwlock_t *slock;

	while (npages-- > 0) {
		pg = *ploans++;

		/*
		 * do a little dance to acquire the object or anon lock
		 * as appropriate.  we are locking in the wrong order,
		 * so we have to do a try-lock here.
		 */

		mutex_enter(&pg->interlock);
		slock = NULL;
		while (pg->uobject != NULL || pg->uanon != NULL) {
			if (pg->uobject != NULL) {
				slock = pg->uobject->vmobjlock;
			} else {
				slock = pg->uanon->an_lock;
			}
			if (rw_tryenter(slock, RW_WRITER)) {
				break;
			}
			/* XXX Better than yielding but inadequate. */
			kpause("livelock", false, 1, &pg->interlock);
			slock = NULL;
		}

		/*
		 * drop our loan.  if page is owned by an anon but
		 * PG_ANON is not set, the page was loaned to the anon
		 * from an object which dropped ownership, so resolve
		 * this by turning the anon's loan into real ownership
		 * (ie. decrement loan_count again and set PG_ANON).
		 * after all this, if there are no loans left, put the
		 * page back a paging queue (if the page is owned by
		 * an anon) or free it (if the page is now unowned).
		 */

		KASSERT(pg->loan_count > 0);
		pg->loan_count--;
		if (pg->uobject == NULL && pg->uanon != NULL &&
		    (pg->flags & PG_ANON) == 0) {
			KASSERT(pg->loan_count > 0);
			pg->loan_count--;
			pg->flags |= PG_ANON;
		}
		mutex_exit(&pg->interlock);
		if (pg->loan_count == 0 && pg->uobject == NULL &&
		    pg->uanon == NULL) {
			KASSERT((pg->flags & PG_BUSY) == 0);
			uvm_pagefree(pg);
		}
		if (slock != NULL) {
			rw_exit(slock);
		}
	}
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
		rw_exit(uobj->vmobjlock);
		return 0;
	}

	/*
	 * we don't actually want to ever free the uvm_loanzero_page, so
	 * just reactivate or dequeue it.
	 */

	pg = uvm_pagelookup(uobj, 0);
	KASSERT(pg != NULL);

	uvm_pagelock(pg);
	if (pg->uanon) {
		uvm_pageactivate(pg);
	} else {
		uvm_pagedequeue(pg);
	}
	uvm_pageunlock(pg);

	rw_exit(uobj->vmobjlock);
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

	rw_init(&uvm_loanzero_lock);
	uvm_obj_init(&uvm_loanzero_object, &ulz_pager, false, 0);
	uvm_obj_setlock(&uvm_loanzero_object, &uvm_loanzero_lock);

	UVMHIST_INIT(loanhist, 300);
}

/*
 * uvm_loanbreak: break loan on a uobj page
 *
 * => called with uobj locked
 * => the page may be busy; if it's busy, it will be unbusied
 * => return value:
 *	newly allocated page if succeeded
 */
struct vm_page *
uvm_loanbreak(struct vm_page *uobjpage)
{
	struct vm_page *pg;
	struct uvm_object *uobj __diagused = uobjpage->uobject;

	KASSERT(uobj != NULL);
	KASSERT(rw_write_held(uobj->vmobjlock));

	/* alloc new un-owned page */
	pg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (pg == NULL)
		return NULL;

	/*
	 * copy the data from the old page to the new
	 * one and clear the fake flags on the new page (keep it busy).
	 * force a reload of the old page by clearing it from all
	 * pmaps.
	 * then rename the pages.
	 */

	uvm_pagecopy(uobjpage, pg);	/* old -> new */
	pg->flags &= ~PG_FAKE;
	KASSERT(uvm_pagegetdirty(pg) == UVM_PAGE_STATUS_DIRTY);
	pmap_page_protect(uobjpage, VM_PROT_NONE);
	/* uobj still locked */
	if ((uobjpage->flags & PG_BUSY) != 0) {
		uobjpage->flags &= ~PG_BUSY;
		UVM_PAGE_OWN(uobjpage, NULL);
	}

	/*
	 * if the page is no longer referenced by
	 * an anon (i.e. we are breaking an O->K
	 * loan), then remove it from any pageq's.
	 */

	uvm_pagelock2(uobjpage, pg);
	uvm_pagewakeup(uobjpage);
	if (uobjpage->uanon == NULL)
		uvm_pagedequeue(uobjpage);

	/*
	 * replace uobjpage with new page.
	 */

	uvm_pagereplace(uobjpage, pg);

	/*
	 * at this point we have absolutely no
	 * control over uobjpage
	 */

	uvm_pageactivate(pg);
	uvm_pageunlock2(uobjpage, pg);

	/*
	 * done!  loan is broken and "pg" is
	 * PG_BUSY.   it can now replace uobjpage.
	 */

	return pg;
}

int
uvm_loanbreak_anon(struct vm_anon *anon, struct uvm_object *uobj)
{
	struct vm_page *newpg, *oldpg;
	unsigned oldstatus;

	KASSERT(rw_write_held(anon->an_lock));
	KASSERT(uobj == NULL || rw_write_held(uobj->vmobjlock));
	KASSERT(anon->an_page->loan_count > 0);

	/* get new un-owned replacement page */
	newpg = uvm_pagealloc(NULL, 0, NULL, 0);
	if (newpg == NULL) {
		return ENOMEM;
	}

	oldpg = anon->an_page;
	/* copy old -> new */
	uvm_pagecopy(oldpg, newpg);
	KASSERT(uvm_pagegetdirty(newpg) == UVM_PAGE_STATUS_DIRTY);

	/* force reload */
	pmap_page_protect(oldpg, VM_PROT_NONE);
	oldstatus = uvm_pagegetdirty(anon->an_page);

	uvm_pagelock2(oldpg, newpg);
	if (uobj == NULL) {
		/*
		 * we were the lender (A->K); need to remove the page from
		 * pageq's.
		 *
		 * PG_ANON is updated by the caller.
		 */
		KASSERT((oldpg->flags & PG_ANON) != 0);
		oldpg->flags &= ~PG_ANON;
		uvm_pagedequeue(oldpg);
	}
	oldpg->uanon = NULL;

	if (uobj) {
		/* if we were receiver of loan */
		KASSERT((oldpg->pqflags & PG_ANON) == 0);
		oldpg->loan_count--;
	}

	/* install new page in anon */
	anon->an_page = newpg;
	newpg->uanon = anon;
	newpg->flags |= PG_ANON;

	uvm_pageactivate(newpg);
	uvm_pageunlock2(oldpg, newpg);

	newpg->flags &= ~(PG_BUSY|PG_FAKE);
	UVM_PAGE_OWN(newpg, NULL);

	if (uobj) {
		rw_exit(uobj->vmobjlock);
	}

	/* done! */
	kpreempt_disable();
	if (uobj == NULL) {
		CPU_COUNT(CPU_COUNT_ANONUNKNOWN + oldstatus, -1);
	}
	CPU_COUNT(CPU_COUNT_ANONDIRTY, 1);
	kpreempt_enable();
	return 0;
}
