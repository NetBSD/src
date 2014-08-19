/*	$NetBSD: pmap_pvt.h,v 1.15.44.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SUN3X_PMAPPVT_H
#define _SUN3X_PMAPPVT_H

#include "opt_pmap_debug.h"

/*************************** TMGR STRUCTURES ***************************
 * The sun3x 'tmgr' structures contain MMU tables and additional       *
 * information about their current usage and availability.             *
 ***********************************************************************/
typedef struct a_tmgr_struct a_tmgr_t;
typedef struct b_tmgr_struct b_tmgr_t;
typedef struct c_tmgr_struct c_tmgr_t;

/* A level A table manager contains a pointer to an MMU table of long
 * format table descriptors (an 'A' table), a pointer to the pmap
 * currently using the table, and the number of wired and active entries
 * it contains.
 */
struct a_tmgr_struct {
	pmap_t		at_parent; /* pmap currently using this table    */
	mmu_long_dte_t	*at_dtbl;  /* the MMU table being managed        */
	uint8_t         at_wcnt;   /* no. of wired entries in this table */
	uint8_t         at_ecnt;   /* no. of valid entries in this table */
	uint16_t	at_dum1;   /* structure padding                  */
	TAILQ_ENTRY(a_tmgr_struct) at_link;  /* list linker              */
};

/* A level B table manager contains a pointer to an MMU table of
 * short format table descriptors (a 'B' table), a pointer to the level
 * A table manager currently using it, the index of this B table
 * within that parent A table, and the number of wired and active entries
 * it currently contains.
 */
struct b_tmgr_struct {
	a_tmgr_t	*bt_parent; /* Parent 'A' table manager         */
	mmu_short_dte_t *bt_dtbl;   /* the MMU table being managed      */
	uint8_t		bt_pidx;    /* this table's index in the parent */
	uint8_t		bt_wcnt;    /* no. of wired entries in table    */
	uint8_t		bt_ecnt;    /* no. of valid entries in table    */
	uint8_t		bt_dum1;    /* structure padding                */
    	TAILQ_ENTRY(b_tmgr_struct) bt_link; /* list linker              */
};

/* A level 'C' table manager consists of pointer to an MMU table of short
 * format page descriptors (a 'C' table), a pointer to the level B table
 * manager currently using it, and the number of wired and active pages
 * it currently contains.
 *
 * Additionally, the table manager contains a pointer to the pmap
 * that is currently using it and the starting virtual address of the
 * range that the MMU table manages.  These two items can be obtained
 * through the traversal of other table manager structures, but having
 * them close at hand helps speed up operations in the PV system.
 */
struct c_tmgr_struct {
	b_tmgr_t	*ct_parent; /* Parent 'B' table manager         */
	mmu_short_pte_t	*ct_dtbl;   /* the MMU table being managed      */
	uint8_t		ct_pidx;    /* this table's index in the parent */
	uint8_t		ct_wcnt;    /* no. of wired entries in table    */
	uint8_t		ct_ecnt;    /* no. of valid entries in table    */
	uint8_t		ct_dum1;    /* structure padding                */
	TAILQ_ENTRY(c_tmgr_struct) ct_link; /* list linker              */
#define	MMU_SHORT_PTE_WIRED	MMU_SHORT_PTE_UN1
#define MMU_PTE_WIRED		((*pte)->attr.raw & MMU_SHORT_PTE_WIRED)
	pmap_t		ct_pmap;    /* pmap currently using this table  */
	vaddr_t		ct_va;      /* starting va that this table maps */
};

/* The Mach VM code requires that the pmap module be able to apply
 * several different operations on all page descriptors that map to a
 * given physical address.  A few of these are:
 *  + invalidate all mappings to a page.
 *  + change the type of protection on all mappings to a page.
 *  + determine if a physical page has been written to
 *  + determine if a physical page has been accessed (read from)
 *  + clear such information
 * The collection of structures and tables which we used to make this
 * possible is known as the 'Physical to Virtual' or 'PV' system.
 *
 * Every physical page of memory managed by the virtual memory system
 * will have a structure which describes whether or not it has been
 * modified or referenced, and contains a list of page descriptors that
 * are currently mapped to it (if any).  This array of structures is
 * known as the 'PV' list.
 *
 ** Old PV Element structure
 * To keep a list of page descriptors currently using the page, another
 * structure had to be invented.  Its sole purpose is to be a link in
 * a chain of such structures.  No other information is contained within
 * the structure however!  The other piece of information it holds is
 * hidden within its address.  By maintaining a one-to-one correspondence
 * of page descriptors in the system and such structures, this address can
 * readily be translated into its associated page descriptor by using a
 * simple macro.  This bizzare structure is simply known as a 'PV
 * Element', or 'pve' for short.
 *
 ** New PV Element structure
 * To keep a list of page descriptors currently using the page, another
 * structure had to be invented.  Its sole purpose is to indicate the index
 * of the next PTE currently referencing the page.  By maintaining a one-to-
 * one correspondence of page descriptors in the system and such structures,
 * this same index is also the index of the next PV element, which describes
 * the index of yet another page mapped to the same address and so on.  The
 * special index 'PVE_EOL' is used to represent the end of the list.
 */
struct pv_struct {
	u_short	pv_idx;		/* Index of PTE using this page */
	u_short	pv_flags;	/* Physical page status flags */
#define PV_FLAGS_USED	MMU_SHORT_PTE_USED
#define PV_FLAGS_MDFY	MMU_SHORT_PTE_M
};
typedef struct pv_struct pv_t;

struct pv_elem_struct {
	u_short	pve_next;
#define	PVE_EOL	0xffff		/* End-of-list marker */
};
typedef struct pv_elem_struct pv_elem_t;

/* Physical memory on the 3/80 is not contiguous.  The ROM Monitor
 * provides us with a linked list of memory segments describing each
 * segment with its base address and its size.
 */
struct pmap_physmem_struct {
	paddr_t		pmem_start;  /* Starting physical address      */
	paddr_t		pmem_end;    /* First byte outside of range    */
	int             pmem_pvbase; /* Offset within the pv list      */
	struct pmap_physmem_struct *pmem_next; /* Next block of memory */
};

/* These are defined in pmap.c */
extern struct pmap_physmem_struct avail_mem[];

#endif /* _SUN3X_PMAPPVT_H */
