/*	$NetBSD: uvm_page.h,v 1.93.2.1 2020/01/17 21:47:38 ad Exp $	*/

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
 *	@(#)vm_page.h   7.3 (Berkeley) 4/21/91
 * from: Id: uvm_page.h,v 1.1.2.6 1998/02/04 02:31:42 chuck Exp
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

#ifndef _UVM_UVM_PAGE_H_
#define _UVM_UVM_PAGE_H_

#include <uvm/uvm_extern.h>
#include <uvm/uvm_pglist.h>

/*
 * Management of resident (logical) pages.
 *
 * Each resident page has a vm_page structure, indexed by page number.
 * There are several lists in the structure:
 *
 * - A red-black tree rooted with the containing object is used to
 *   quickly perform object+offset lookups.
 * - A list of all pages for a given object, for a quick deactivation
 *   at a time of deallocation.
 * - An ordered list of pages due for pageout.
 *
 * In addition, the structure contains the object and offset to which
 * this page belongs (for pageout) and sundry status bits.
 *
 * Note that the page structure has no lock of its own.  The page is
 * generally protected by its owner's lock (UVM object or amap/anon).
 * It should be noted that UVM has to serialize pmap(9) operations on
 * the managed pages, e.g. for pmap_enter() calls.  Hence, the lock
 * order is as follows:
 *
 *	[vmpage-owner-lock] ->
 *		any pmap locks (e.g. PV hash lock)
 *
 * Since the kernel is always self-consistent, no serialization is
 * required for unmanaged mappings, e.g. for pmap_kenter_pa() calls.
 *
 * Field markings and the corresponding locks:
 *
 * f:	free page queue lock, uvm_fpageqlock
 * o:	page owner (uvm_object::vmobjlock, vm_amap::am_lock, vm_anon::an_lock)
 * i:	vm_page::interlock
 *        => flags set and cleared only with o&i held can
 *           safely be tested for with only o held.
 * o,i:	o|i for read, o&i for write (depends on context - if could be loaned)
 *	  => see uvm_loan.c
 * w:	wired page queue or uvm_pglistalloc:
 *	  => wired page queue: o&i to change, stable from wire to unwire
 *		XXX What about concurrent or nested wire?
 *	  => uvm_pglistalloc: owned by caller
 * ?:	locked by pmap or assumed page owner's lock
 * p:	locked by pagedaemon policy module (pdpolicy)
 * c:	cpu private
 * s:	stable, does not change
 *
 * UVM and pmap(9) may use uvm_page_owner_locked_p() to assert whether the
 * page owner's lock is acquired.
 *
 * A page can have one of four identities:
 *
 * o free
 *   => pageq.list is entry on global free page queue
 *   => uanon is unused (or (void *)0xdeadbeef for DEBUG)
 *   => uobject is unused (or (void *)0xdeadbeef for DEBUG)
 *   => PG_FREE is set in flags
 * o owned by a uvm_object
 *   => pageq.queue is entry on wired page queue, if any
 *   => uanon is NULL or the vm_anon to which it has been O->A loaned
 *   => uobject is owner
 * o owned by a vm_anon
 *   => pageq is unused (XXX correct?)
 *   => uanon is owner
 *   => uobject is NULL
 *   => PG_ANON is set in flags
 * o allocated by uvm_pglistalloc
 *   => pageq.queue is entry on resulting pglist, owned by caller
 *   => uanon is unused
 *   => uobject is unused
 *
 * The following transitions are allowed:
 *
 * - uvm_pagealloc: free -> owned by a uvm_object/vm_anon
 * - uvm_pagefree: owned by a uvm_object/vm_anon -> free
 * - uvm_pglistalloc: free -> allocated by uvm_pglistalloc
 * - uvm_pglistfree: allocated by uvm_pglistalloc -> free
 *
 * On the ordering of fields:
 *
 * The fields most heavily used by the page allocator and uvmpdpol are
 * clustered together at the start of the structure, so that while under
 * global lock it's more likely that only one cache line for each page need
 * be touched.
 */

struct vm_page {
	union {
		TAILQ_ENTRY(vm_page) queue;	/* w: wired page queue
						 * or uvm_pglistalloc output */
		LIST_ENTRY(vm_page) list;	/* f: global free page queue */
	} pageq;
	TAILQ_ENTRY(vm_page)	pdqueue;	/* p: pagedaemon queue */
	kmutex_t		interlock;	/* s: lock on identity */
	uint32_t		pqflags;	/* i: pagedaemon flags */
	uint32_t		flags;		/* o: object flags */
	paddr_t			phys_addr;	/* o: physical address of pg */
	uint32_t		loan_count;	/* o,i: num. active loans */
	uint32_t		wire_count;	/* o,i: wired down map refs */
	struct vm_anon		*uanon;		/* o,i: anon */
	struct uvm_object	*uobject;	/* o,i: object */
	voff_t			offset;		/* o: offset into object */

#ifdef __HAVE_VM_PAGE_MD
	struct vm_page_md	mdpage;		/* ?: pmap-specific data */
#endif

#if defined(UVM_PAGE_TRKOWN)
	/* debugging fields to track page ownership */
	pid_t			owner;		/* proc that set PG_BUSY */
	lwpid_t			lowner;		/* lwp that set PG_BUSY */
	const char		*owner_tag;	/* why it was set busy */
#endif
};

/*
 * Overview of UVM page flags.
 *
 * Locking notes:
 *
 * PG_, struct vm_page::flags	=> locked by owner
 * PG_AOBJ			=> additionally locked by vm_page::interlock
 * PG_ANON			=> additionally locked by vm_page::interlock
 * PG_FREE			=> additionally locked by uvm_fpageqlock
 *				   for uvm_pglistalloc()
 *
 * Flag descriptions:
 *
 * PG_CLEAN:
 *	Page is known clean.
 *	The contents of the page is consistent with its backing store.
 *
 * PG_DIRTY:
 *	Page is known dirty.
 *	To avoid losing data, the contents of the page should be written
 *	back to the backing store before freeing the page.
 *
 * PG_BUSY:
 *	Page is long-term locked, usually because of I/O (transfer from the
 *	page memory to the backing store) is in progress.  LWP attempting
 *	to access the page shall set PG_WANTED and wait.
 *
 * PG_WANTED:
 *	Indicates that the page, which is currently PG_BUSY, is wanted by
 *	some other LWP.  The page owner (i.e. LWP which set PG_BUSY) is
 *	responsible to clear both flags and wake up any waiters once it has
 *	released the long-term lock (PG_BUSY).
 *
 * PG_PAGEOUT:
 *	Indicates that the page is being paged-out in preparation for
 *	being freed.
 *
 * PG_RELEASED:
 *	Indicates that the page, which is currently PG_BUSY, should be freed
 *	after the release of long-term lock.  It is responsibility of the
 *	owning LWP (i.e. which set PG_BUSY) to do it.
 *
 * PG_FAKE:
 *	Page has been allocated, but not yet initialised.  The flag is used
 *	to avoid overwriting of valid data, e.g. to prevent read from the
 *	backing store when in-core data is newer.
 *
 * PG_RDONLY:
 *	Indicates that the page must be mapped read-only.
 *
 * PG_ZERO:
 *	Indicates that the page has been pre-zeroed.  This flag is only
 *	set when the page is not in the queues and is cleared when the
 *	page is placed on the free list.
 *
 * PG_MARKER:
 *	Dummy marker page, generally used for list traversal.
 */

/*
 * if you want to renumber PG_CLEAN and PG_DIRTY, check __CTASSERTs in
 * uvm_page_status.c first.
 */

#define	PG_CLEAN	0x00000001	/* page is known clean */
#define	PG_DIRTY	0x00000002	/* page is known dirty */
#define	PG_BUSY		0x00000004	/* page is locked */
#define	PG_WANTED	0x00000008	/* someone is waiting for page */
#define	PG_PAGEOUT	0x00000010	/* page to be freed for pagedaemon */
#define	PG_RELEASED	0x00000020	/* page to be freed when unbusied */
#define	PG_FAKE		0x00000040	/* page is not yet initialized */
#define	PG_RDONLY	0x00000080	/* page must be mapped read-only */
#define	PG_ZERO		0x00000100	/* page is pre-zero'd */
#define	PG_TABLED	0x00000200	/* page is tabled in object */
#define	PG_AOBJ		0x00000400	/* page is part of an anonymous
					   uvm_object */
#define	PG_ANON		0x00000800	/* page is part of an anon, rather
					   than an uvm_object */
#define	PG_FILE		0x00001000	/* file backed (non-anonymous) */
#define	PG_READAHEAD	0x00002000	/* read-ahead but not "hit" yet */
#define	PG_FREE		0x00004000	/* page is on free list */
#define	PG_MARKER	0x00008000	/* dummy marker page */
#define	PG_PAGER1	0x00010000	/* pager-specific flag */

#define	PG_STAT		(PG_ANON|PG_AOBJ|PG_FILE)
#define	PG_SWAPBACKED	(PG_ANON|PG_AOBJ)

#define	UVM_PGFLAGBITS \
	"\20\1CLEAN\2DIRTY\3BUSY\4WANTED" \
	"\5PAGEOUT\6RELEASED\7FAKE\10RDONLY" \
	"\11ZERO\12TABLED\13AOBJ\14ANON" \
	"\15FILE\16READAHEAD\17FREE\20MARKER" \
	"\21PAGER1"

/*
 * uvmpdpol state flags.
 *
 * => may only be changed with pg->interlock held.
 * => changing them is the responsibility of uvmpdpol ..
 * => .. but uvm_page needs to know about them in order to purge updates.
 * => PQ_PRIVATE is private to the individual uvmpdpol implementation.
 */

#define	PQ_INTENT_A		0x00000000	/* intend activation */
#define	PQ_INTENT_I		0x00000001	/* intend deactivation */
#define	PQ_INTENT_E		0x00000002	/* intend enqueue */
#define	PQ_INTENT_D		0x00000003	/* intend dequeue */
#define	PQ_INTENT_MASK		0x00000003	/* mask of intended state */
#define	PQ_INTENT_SET		0x00000004	/* not realized yet */
#define	PQ_INTENT_QUEUED	0x00000008	/* queued for processing */
#define	PQ_PRIVATE		0xfffffff0

/*
 * physical memory layout structure
 *
 * MD vmparam.h must #define:
 *   VM_PHYSEG_MAX = max number of physical memory segments we support
 *		   (if this is "1" then we revert to a "contig" case)
 *   VM_PHYSSEG_STRAT: memory sort/search options (for VM_PHYSEG_MAX > 1)
 * 	- VM_PSTRAT_RANDOM:   linear search (random order)
 *	- VM_PSTRAT_BSEARCH:  binary search (sorted by address)
 *	- VM_PSTRAT_BIGFIRST: linear search (sorted by largest segment first)
 *      - others?
 *   XXXCDC: eventually we should purge all left-over global variables...
 */
#define VM_PSTRAT_RANDOM	1
#define VM_PSTRAT_BSEARCH	2
#define VM_PSTRAT_BIGFIRST	3

#ifdef _KERNEL

/*
 * globals
 */

extern bool vm_page_zero_enable;

/*
 * prototypes: the following prototypes define the interface to pages
 */

void uvm_page_init(vaddr_t *, vaddr_t *);
#if defined(UVM_PAGE_TRKOWN)
void uvm_page_own(struct vm_page *, const char *);
#endif
#if !defined(PMAP_STEAL_MEMORY)
bool uvm_page_physget(paddr_t *);
#endif
void uvm_page_recolor(int);
void uvm_page_rebucket(void);
void uvm_pageidlezero(void);

void uvm_pageactivate(struct vm_page *);
vaddr_t uvm_pageboot_alloc(vsize_t);
void uvm_pagecopy(struct vm_page *, struct vm_page *);
void uvm_pagedeactivate(struct vm_page *);
void uvm_pagedequeue(struct vm_page *);
void uvm_pageenqueue(struct vm_page *);
void uvm_pagefree(struct vm_page *);
void uvm_pagelock(struct vm_page *);
void uvm_pagelock2(struct vm_page *, struct vm_page *);
void uvm_pageunlock(struct vm_page *);
void uvm_pageunlock2(struct vm_page *, struct vm_page *);
void uvm_page_unbusy(struct vm_page **, int);
struct vm_page *uvm_pagelookup(struct uvm_object *, voff_t);
void uvm_pageunwire(struct vm_page *);
void uvm_pagewire(struct vm_page *);
void uvm_pagezero(struct vm_page *);
bool uvm_pageismanaged(paddr_t);
bool uvm_page_owner_locked_p(struct vm_page *);
void uvm_pgfl_lock(void);
void uvm_pgfl_unlock(void);
unsigned int uvm_pagegetdirty(struct vm_page *);
void uvm_pagemarkdirty(struct vm_page *, unsigned int);
bool uvm_pagecheckdirty(struct vm_page *, bool);
bool uvm_pagereadonly_p(struct vm_page *);
bool uvm_page_locked_p(struct vm_page *);

int uvm_page_lookup_freelist(struct vm_page *);

struct vm_page *uvm_phys_to_vm_page(paddr_t);
paddr_t uvm_vm_page_to_phys(const struct vm_page *);

#if defined(PMAP_DIRECT)
extern bool ubc_direct;
int uvm_direct_process(struct vm_page **, u_int, voff_t, vsize_t,
	    int (*)(void *, size_t, void *), void *);
#endif

/*
 * page dirtiness status for uvm_pagegetdirty and uvm_pagemarkdirty
 *
 * UNKNOWN means that we need to consult pmap to know if the page is
 * dirty or not.
 * basically, UVM_PAGE_STATUS_CLEAN implies that the page has no writable
 * mapping.
 *
 * if you want to renumber these, check __CTASSERTs in
 * uvm_page_status.c first.
 */

#define	UVM_PAGE_STATUS_UNKNOWN	0
#define	UVM_PAGE_STATUS_CLEAN	1
#define	UVM_PAGE_STATUS_DIRTY	2
#define	UVM_PAGE_NUM_STATUS	3

/*
 * macros
 */

#define VM_PAGE_TO_PHYS(entry)	uvm_vm_page_to_phys(entry)

#ifdef __HAVE_VM_PAGE_MD
#define	VM_PAGE_TO_MD(pg)	(&(pg)->mdpage)
#endif

/*
 * Compute the page color for a given page.
 */
#define	VM_PGCOLOR(pg) \
	(atop(VM_PAGE_TO_PHYS((pg))) & uvmexp.colormask)
#define	PHYS_TO_VM_PAGE(pa)	uvm_phys_to_vm_page(pa)

/*
 * VM_PAGE_IS_FREE() can't tell if the page is on global free list, or a
 * per-CPU cache.  If you need to be certain, pause caching.
 */
#define VM_PAGE_IS_FREE(entry)  ((entry)->flags & PG_FREE)

/*
 * Use the lower 10 bits of pg->phys_addr to cache some some locators for
 * the page.  This implies that the smallest possible page size is 1kB, and
 * that nobody should use pg->phys_addr directly (use VM_PAGE_TO_PHYS()).
 * 
 * - 5 bits for the freelist index, because uvm_page_lookup_freelist()
 *   traverses an rbtree and therefore features prominently in traces
 *   captured during performance test.  It would probably be more useful to
 *   cache physseg index here because freelist can be inferred from physseg,
 *   but it requires changes to allocation for UVM_HOTPLUG, so for now we'll
 *   go with freelist.
 *
 * - 5 bits for "bucket", a way for us to categorise pages further as
 *   needed (e.g. NUMA node).
 *
 * None of this is set in stone; it can be adjusted as needed.
 */

#define	UVM_PHYSADDR_FREELIST	__BITS(0,4)
#define	UVM_PHYSADDR_BUCKET	__BITS(5,9)

static inline unsigned
uvm_page_get_freelist(struct vm_page *pg)
{
	unsigned fl = __SHIFTOUT(pg->phys_addr, UVM_PHYSADDR_FREELIST);
	KASSERT(fl == (unsigned)uvm_page_lookup_freelist(pg));
	return fl;
}

static inline unsigned
uvm_page_get_bucket(struct vm_page *pg)
{
	return __SHIFTOUT(pg->phys_addr, UVM_PHYSADDR_BUCKET);
}

static inline void
uvm_page_set_freelist(struct vm_page *pg, unsigned fl)
{
	KASSERT(fl < 32);
	pg->phys_addr &= ~UVM_PHYSADDR_FREELIST;
	pg->phys_addr |= __SHIFTIN(fl, UVM_PHYSADDR_FREELIST);
}

static inline void
uvm_page_set_bucket(struct vm_page *pg, unsigned b)
{
	KASSERT(b < 32);
	pg->phys_addr &= ~UVM_PHYSADDR_BUCKET;
	pg->phys_addr |= __SHIFTIN(b, UVM_PHYSADDR_BUCKET);
}

#ifdef DEBUG
void uvm_pagezerocheck(struct vm_page *);
#endif /* DEBUG */

#endif /* _KERNEL */

#endif /* _UVM_UVM_PAGE_H_ */
