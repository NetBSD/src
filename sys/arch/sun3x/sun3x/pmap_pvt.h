/*	$NetBSD: pmap_pvt.h,v 1.5 1997/03/21 22:46:16 gwr Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
	u_char          at_wcnt;   /* no. of wired entries in this table */
	u_char          at_ecnt;   /* no. of valid entries in this table */
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
	u_char		bt_pidx;    /* this table's index in the parent */
	u_char		bt_wcnt;    /* no. of wired entries in table    */
	u_char		bt_ecnt;    /* no. of valid entries in table    */
    	TAILQ_ENTRY(b_tmgr_struct) bt_link; /* list linker              */
};

/* A level 'C' table manager consists of pointer to an MMU table of short
 * format page descriptors (a 'C' table), a pointer to the level B table
 * manager currently using it, and the number of wired and active pages
 * it currently contains.
 */
struct c_tmgr_struct {
	b_tmgr_t	*ct_parent; /* Parent 'B' table manager         */
	mmu_short_pte_t	*ct_dtbl;   /* the MMU table being managed      */
	u_char		ct_pidx;    /* this table's index in the parent */
	u_char		ct_wcnt;    /* no. of wired entries in table    */
	u_char		ct_ecnt;    /* no. of valid entries in table    */
	TAILQ_ENTRY(c_tmgr_struct) ct_link; /* list linker              */
#define	MMU_SHORT_PTE_WIRED	MMU_SHORT_PTE_UN1
#define MMU_PTE_WIRED		((*pte)->attr.raw & MMU_SHORT_PTE_WIRED)
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
	vm_offset_t	pmem_start;  /* Starting physical address      */
	vm_offset_t	pmem_end;    /* First byte outside of range    */
	int             pmem_pvbase; /* Offset within the pv list      */
	struct pmap_physmem_struct *pmem_next; /* Next block of memory */
};

/* Internal function definitions. */
a_tmgr_t *get_a_table __P((void));
b_tmgr_t *get_b_table __P((void));
c_tmgr_t *get_c_table __P((void));
int    free_a_table __P((a_tmgr_t *, boolean_t));
int    free_b_table __P((b_tmgr_t *, boolean_t));
int    free_c_table __P((c_tmgr_t *, boolean_t));
void   pmap_bootstrap_aalign __P((int));
void   pmap_alloc_usermmu __P((void));
void   pmap_alloc_usertmgr __P((void));
void   pmap_alloc_pv __P((void));
void   pmap_init_a_tables __P((void));
void   pmap_init_b_tables __P((void));
void   pmap_init_c_tables __P((void));
void   pmap_init_pv __P((void));
void   pmap_clear_pv __P((vm_offset_t, int));
boolean_t pmap_remove_a __P((a_tmgr_t *, vm_offset_t, vm_offset_t));
boolean_t pmap_remove_b __P((b_tmgr_t *, vm_offset_t, vm_offset_t));
boolean_t pmap_remove_c __P((c_tmgr_t *, vm_offset_t, vm_offset_t));
void   pmap_remove_pte __P((mmu_short_pte_t *));
void   pmap_enter_kernel __P((vm_offset_t, vm_offset_t, vm_prot_t));
void   pmap_remove_kernel __P((vm_offset_t, vm_offset_t));
void   pmap_protect_kernel __P((vm_offset_t, vm_offset_t, vm_prot_t));
vm_offset_t pmap_extract_kernel __P((vm_offset_t));
vm_offset_t pmap_get_pteinfo __P((u_int, pmap_t *, c_tmgr_t **));
void   pmap_pinit __P((pmap_t));
int    pmap_dereference __P((pmap_t));
boolean_t is_managed __P((vm_offset_t));
boolean_t pmap_stroll __P((pmap_t, vm_offset_t, a_tmgr_t **, b_tmgr_t **,\
	c_tmgr_t **, mmu_short_pte_t **, int *, int *, int *));
void  pmap_bootstrap_copyprom __P((void));
void  pmap_takeover_mmu __P((void));
void  pmap_bootstrap_setprom __P((void));

/* Debugging function definitions */
void  pv_list __P((vm_offset_t, int));

/* These are defined in pmap.c */
extern struct pmap_physmem_struct avail_mem[];

#endif /* _SUN3X_MYPMAP_H */
