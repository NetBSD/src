/*	$NetBSD: uvm_page.h,v 1.67 2010/11/14 15:06:34 uebayasi Exp $	*/

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

/*
 * uvm_page.h
 */

/*
 *	Resident memory system definitions.
 */

/*
 *	Management of resident (logical) pages.
 *
 *	A small structure is kept for each resident
 *	page, indexed by page number.  Each structure
 *	is an element of several lists:
 *
 *		A red-black tree rooted with the containing
 *		object is used to quickly perform object+
 *		offset lookups
 *
 *		A list of all pages for a given object,
 *		so they can be quickly deactivated at
 *		time of deallocation.
 *
 *		An ordered list of pages due for pageout.
 *
 *	In addition, the structure contains the object
 *	and offset to which this page belongs (for pageout),
 *	and sundry status bits.
 *
 *	Fields in this structure are locked either by the lock on the
 *	object that the page belongs to (O) or by the lock on the page
 *	queues (P) [or both].
 */

/*
 * locking note: the mach version of this data structure had bit
 * fields for the flags, and the bit fields were divided into two
 * items (depending on who locked what).  some time, in BSD, the bit
 * fields were dumped and all the flags were lumped into one short.
 * that is fine for a single threaded uniprocessor OS, but bad if you
 * want to actual make use of locking.  so, we've separated things
 * back out again.
 *
 * note the page structure has no lock of its own.
 */

#include <uvm/uvm_extern.h>
#include <uvm/uvm_pglist.h>

#include <sys/rbtree.h>

struct vm_page {
	struct rb_node		rb_node;	/* tree of pages in obj (O) */

	union {
		TAILQ_ENTRY(vm_page) queue;
		LIST_ENTRY(vm_page) list;
	} pageq;				/* queue info for FIFO
						 * queue or free list (P) */
	union {
		TAILQ_ENTRY(vm_page) queue;
		LIST_ENTRY(vm_page) list;
	} listq;				/* pages in same object (O)*/

	struct vm_anon		*uanon;		/* anon (O,P) */
	struct uvm_object	*uobject;	/* object (O,P) */
	voff_t			offset;		/* offset into object (O,P) */
	uint16_t		flags;		/* object flags [O] */
	uint16_t		loan_count;	/* number of active loans
						 * to read: [O or P]
						 * to modify: [O _and_ P] */
	uint16_t		wire_count;	/* wired down map refs [P] */
	uint16_t		pqflags;	/* page queue flags [P] */
	paddr_t			phys_addr;	/* physical address of page */

#ifdef __HAVE_VM_PAGE_MD
	struct vm_page_md	mdpage;		/* pmap-specific data */
#endif

#if defined(UVM_PAGE_TRKOWN)
	/* debugging fields to track page ownership */
	pid_t			owner;		/* proc that set PG_BUSY */
	lwpid_t			lowner;		/* lwp that set PG_BUSY */
	const char		*owner_tag;	/* why it was set busy */
#endif
};

/*
 * These are the flags defined for vm_page.
 */

/*
 * locking rules:
 *   PG_ ==> locked by object lock
 *   PQ_ ==> lock by page queue lock
 *   PQ_FREE is locked by free queue lock and is mutex with all other PQs
 *
 * PG_ZERO is used to indicate that a page has been pre-zero'd.  This flag
 * is only set when the page is on no queues, and is cleared when the page
 * is placed on the free list.
 */

#define	PG_BUSY		0x0001		/* page is locked */
#define	PG_WANTED	0x0002		/* someone is waiting for page */
#define	PG_TABLED	0x0004		/* page is in VP table  */
#define	PG_CLEAN	0x0008		/* page has not been modified */
#define	PG_PAGEOUT	0x0010		/* page to be freed for pagedaemon */
#define PG_RELEASED	0x0020		/* page to be freed when unbusied */
#define	PG_FAKE		0x0040		/* page is not yet initialized */
#define	PG_RDONLY	0x0080		/* page must be mapped read-only */
#define	PG_ZERO		0x0100		/* page is pre-zero'd */
#define	PG_MARKER	0x0200		/* dummy marker page */

#define PG_PAGER1	0x1000		/* pager-specific flag */

#define	UVM_PGFLAGBITS \
	"\20\1BUSY\2WANTED\3TABLED\4CLEAN\5PAGEOUT\6RELEASED\7FAKE\10RDONLY" \
	"\11ZERO\12MARKER\15PAGER1"

#define PQ_FREE		0x0001		/* page is on free list */
#define PQ_ANON		0x0002		/* page is part of an anon, rather
					   than an uvm_object */
#define PQ_AOBJ		0x0004		/* page is part of an anonymous
					   uvm_object */
#define PQ_SWAPBACKED	(PQ_ANON|PQ_AOBJ)
#define PQ_READAHEAD	0x0008	/* read-ahead but has not been "hit" yet */

#define PQ_PRIVATE1	0x0100
#define PQ_PRIVATE2	0x0200
#define PQ_PRIVATE3	0x0400
#define PQ_PRIVATE4	0x0800
#define PQ_PRIVATE5	0x1000
#define PQ_PRIVATE6	0x2000
#define PQ_PRIVATE7	0x4000
#define PQ_PRIVATE8	0x8000

#define	UVM_PQFLAGBITS \
	"\20\1FREE\2ANON\3AOBJ\4READAHEAD" \
	"\11PRIVATE1\12PRIVATE2\13PRIVATE3\14PRIVATE4" \
	"\15PRIVATE5\16PRIVATE6\17PRIVATE7\20PRIVATE8"

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

/*
 * vm_physseg: describes one segment of physical memory
 */
struct vm_physseg {
	paddr_t	start;			/* PF# of first page in segment */
	paddr_t	end;			/* (PF# of last page in segment) + 1 */
	paddr_t	avail_start;		/* PF# of first free page in segment */
	paddr_t	avail_end;		/* (PF# of last free page in segment) +1  */
	int	free_list;		/* which free list they belong on */
	struct	vm_page *pgs;		/* vm_page structures (from start) */
	struct	vm_page *lastpg;	/* vm_page structure for end */
	SIMPLEQ_ENTRY(vm_physseg) list;

#ifdef __HAVE_PMAP_PHYSSEG
	struct	pmap_physseg pmseg;	/* pmap specific (MD) data */
#endif
};

#ifdef _KERNEL

/*
 * globals
 */

extern bool vm_page_zero_enable;

/*
 * physical memory config is stored in vm_physmem.
 */

#define	VM_PHYSMEM_PTR(i)	(vm_physmem_ptrs[i])
#define	VM_PHYSMEM_PTR_SWAP(i, j) \
	do { VM_PHYSMEM_PTR(i) = VM_PHYSMEM_PTR(j); } while (0)

extern struct vm_physseg *vm_physmem_ptrs[VM_PHYSSEG_MAX];
extern int vm_nphysmem;

#define	vm_nphysseg	vm_nphysmem	/* XXX backward compat */

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
void uvm_pageidlezero(void);

void uvm_pageactivate(struct vm_page *);
vaddr_t uvm_pageboot_alloc(vsize_t);
void uvm_pagecopy(struct vm_page *, struct vm_page *);
void uvm_pagedeactivate(struct vm_page *);
void uvm_pagedequeue(struct vm_page *);
void uvm_pageenqueue(struct vm_page *);
void uvm_pagefree(struct vm_page *);
void uvm_page_unbusy(struct vm_page **, int);
struct vm_page *uvm_pagelookup(struct uvm_object *, voff_t);
void uvm_pageunwire(struct vm_page *);
void uvm_pagewait(struct vm_page *, int);
void uvm_pagewake(struct vm_page *);
void uvm_pagewire(struct vm_page *);
void uvm_pagezero(struct vm_page *);
bool uvm_pageismanaged(paddr_t);

int uvm_page_lookup_freelist(struct vm_page *);

int vm_physseg_find(paddr_t, int *);
struct vm_page *uvm_phys_to_vm_page(paddr_t);
paddr_t uvm_vm_page_to_phys(const struct vm_page *);

/*
 * macros
 */

#define UVM_PAGE_TREE_PENALTY	4	/* XXX: a guess */

#define VM_PAGE_TO_PHYS(entry)	uvm_vm_page_to_phys(entry)

#ifdef __HAVE_VM_PAGE_MD
#define	VM_PAGE_TO_MD(pg)	(&(pg)->mdpage)
#endif

/*
 * Compute the page color bucket for a given page.
 */
#define	VM_PGCOLOR_BUCKET(pg) \
	(atop(VM_PAGE_TO_PHYS((pg))) & uvmexp.colormask)

#define	PHYS_TO_VM_PAGE(pa)	uvm_phys_to_vm_page(pa)

#define VM_PAGE_IS_FREE(entry)  ((entry)->pqflags & PQ_FREE)
#define	VM_FREE_PAGE_TO_CPU(pg)	((struct uvm_cpu *)((uintptr_t)pg->offset))

#ifdef DEBUG
void uvm_pagezerocheck(struct vm_page *);
#endif /* DEBUG */

#endif /* _KERNEL */

#endif /* _UVM_UVM_PAGE_H_ */
