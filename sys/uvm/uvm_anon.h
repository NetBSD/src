/*	$NetBSD: uvm_anon.h,v 1.5 1998/02/10 02:34:25 perry Exp $	*/

/*
 * XXXCDC: "ROUGH DRAFT" QUALITY UVM PRE-RELEASE FILE!
 *	   >>>USE AT YOUR OWN RISK, WORK IS NOT FINISHED<<<
 */
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
 * from: Id: uvm_anon.h,v 1.1.2.4 1998/01/04 22:43:39 chuck Exp
 */

#ifndef _UVM_UVM_ANON_H_
#define _UVM_UVM_ANON_H_

/*
 * uvm_anon.h
 */

/*
 * anonymous memory management
 *
 * anonymous virtual memory is short term virtual memory that goes away
 * when the processes referencing it go away.    an anonymous page of
 * virtual memory is described by the following data structure:
 */

struct vm_anon {
	int an_ref;			/* reference count [an_lock] */
#if NCPU > 1
	simple_lock_data_t an_lock;	/* lock for an_ref */
#endif
	union {
		struct vm_anon *an_nxt;	/* if on free list [afreelock] */
		struct vm_page *an_page;/* if in RAM [pageqlock] */
	} u;
	int an_swslot;		/* drum swap slot # (if != 0) [pageqlock] */
};

/*
 * a pool of vm_anon data structures is allocated and put on a global
 * free list at boot time.  vm_anon's on the free list use "an_nxt" as
 * a pointer to the next item on the free list.  for active vm_anon's
 * the data can be in one of the following state: [1] in a vm_page
 * with no backing store allocated yet, [2] in a vm_page with backing
 * store allocated, or [3] paged out to backing store (no vm_page).
 *
 * for pageout in case [2]: if the page has been modified then we must
 * flush it out to backing store, otherwise we can just dump the
 * vm_page.
 */

/*
 * anonymous virtual memory pages (vm_anon's) live in anonymous memory
 * maps.   anonymous memory maps can be shared between processes.   
 * different subsets of an anonymous memory map can be referenced by
 * processes (see below).    an anonymous map is described by the following
 * data structure:
 */

#define VM_AMAP_PPREF		/* use a per-page reference count for split
				   vm_map_entry_t's. */

struct vm_amap {
#if NCPU > 1
	simple_lock_data_t am_l; /* simple lock [locks all vm_amap fields] */
#endif
	int am_ref;		/* reference count */
	int am_flags;		/* flags */
	int am_maxslot;		/* max # of slots allocated */
	int am_nslot;		/* # of slots currently in map ( <= maxslot) */
	int am_nused;		/* # of slots currently in use */
	int *am_slots;		/* contig array of active slots */
	int *am_bckptr;		/* back pointer array to am_slots */
	struct vm_anon **am_anon; /* array of anonymous pages */
#ifdef VM_AMAP_PPREF
	int *am_ppref;		/* per page reference count (if !NULL) */
#endif
};

#define AMAP_SHARED	0x1	/* am_flags: shared amap */
#define AMAP_REFALL	0x2	/* flag to amap_ref() */

/*
 * note that am_slots, am_bckptr, and am_anon are arrays.   this allows
 * fast lookup of pages based on their virual address at the expense of
 * some extra memory.    [XXX: for memory starved systems provide alternate
 * functions?]
 *
 * 4 slot/page example, with slots 1 and 3 in use:
 * ("D/C" == don't care what the value is)
 *
 *            0     1      2     3
 * am_anon:   NULL, anon0, NULL, anon1		(actual pointers to anons)
 * am_bckptr: D/C,  1,     D/C,  0		(points to am_slots entry)
 *
 * am_slots:  3, 1, D/C, D/C    		(says slots 3 and 1 are in use)
 * 
 * note that am_bckptr is D/C if the slot in am_anon is set to NULL.
 * to find the entry in am_slots for an anon, look at am_bckptr[slot],
 * thus the entry for slot 3 in am_slots[] is at am_slots[am_bckptr[3]].
 * in general, if am_anon[X] is non-NULL, then the following must be
 * true: am_slots[am_bckptr[X]] == X
 *
 * note that am_slots is always contig-packed.
 */
  
/*
 * processes reference anonymous virtual memory maps with an anonymous 
 * reference structure:
 */

struct vm_aref {
	int ar_slotoff;			/* slot offset into amap we start */
	struct vm_amap *ar_amap;	/* pointer to amap */
};

/*
 * the offset field indicates which part of the amap we are referencing.
 * locked by vm_map lock.
 */


#endif /* _UVM_UVM_ANON_H_ */
