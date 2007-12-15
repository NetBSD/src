/*	$NetBSD: pmap.c,v 1.40 2007/12/15 00:39:20 perry Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

/*	$OpenBSD: pmap.c,v 1.46 2001/07/25 13:25:31 art Exp $	*/

/*
 * Copyright (c) 1998-2001 Michael Shalayeff
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
 *	This product includes software developed by Michael Shalayeff.
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
 */
/*
 * Copyright 1996 1995 by Open Software Foundation, Inc.
 *              All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Mach Operating System
 * Copyright (c) 1990,1991,1992,1993,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).
 * Copyright (c) 1991,1987 Carnegie Mellon University.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that all advertising materials mentioning features or use of
 * this software display the following acknowledgement: ``This product
 * includes software developed by the Computer Systems Laboratory at
 * the University of Utah.''
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * Carnegie Mellon requests users of this software to return to
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 *
 * 	Utah $Hdr: pmap.c 1.49 94/12/15$
 *	Author: Mike Hibler, Bob Wheeler, University of Utah CSL, 10/90
 */
/*
 *	Manages physical address maps for hppa.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information to
 *	when physical maps must be made correct.
 *
 */
/*
 * CAVEATS:
 *
 *	Needs more work for MP support
 *	page maps are stored as linear linked lists, some
 *		improvement may be achieved should we use smth else
 *	protection id (pid) allocation should be done in a pid_t fashion
 *		(maybe just use the pid itself)
 *	some ppl say, block tlb entries should be maintained somewhere in uvm
 *		and be ready for reloads in the fault handler.
 *	usage of inline grows the code size by 100%, but hopefully
 *		makes it faster as well, since the functions are actually
 *		very small.
 *		retail:  8.1k -> 15.1K
 *		debug:  12.2k -> 22.1K
 *
 * References:
 * 1. PA7100LC ERS, Hewlett-Packard, March 30 1999, Public version 1.0
 * 2. PA7300LC ERS, Hewlett-Packard, March 18 1996, Version 1.0
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.40 2007/12/15 00:39:20 perry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/cpufunc.h>

#include <hppa/hppa/hpt.h>
#include <hppa/hppa/machdep.h>

#define static	/**/
#define	inline /* */

#ifdef PMAPDEBUG
#define	PDB_INIT	0x00000002
#define	PDB_ENTER	0x00000004
#define	PDB_REMOVE	0x00000008
#define	PDB_KENTER	0x00000010
#define	PDB_PMAP	0x00000020
#define	PDB_CACHE	0x00000040
#define	PDB_BITS	0x00000080
#define	PDB_EXTRACT	0x00000100
#define	PDB_PROTECT	0x00000200
#define	PDB_PV_ALLOC	0x00000400
#define	PDB_PV_ENTER	0x00000800
#define	PDB_PV_REMOVE	0x00001000
#define	PDB_PV_FIND_VA	0x00002000
#define	PDB_WIRING	0x00004000
#define	PDB_ZERO	0x00008000
#define	PDB_STEAL	0x00010000
#define	PDB_COPY	0x00020000
int pmapdebug = 0
#if 1
	| PDB_ENTER
	| PDB_REMOVE
	| PDB_KENTER
	| PDB_BITS
	| PDB_PROTECT
	| PDB_EXTRACT
	| PDB_WIRING
	| PDB_ZERO
	| PDB_STEAL
	| PDB_COPY
#endif
	;
#define PMAP_PRINTF_MASK(m,v,x) do {	\
  if ((pmapdebug & (m)) == (v)) {	\
    printf("%s", __func__);		\
    printf x;				\
  }					\
} while(/* CONSTCOND */ 0)
#else
#define PMAP_PRINTF_MASK(m,v,x) do { } while(/* CONSTCOND */ 0)
#endif
#define PMAP_PRINTF(v,x) PMAP_PRINTF_MASK(v,v,x)

vaddr_t	virtual_steal, virtual_start, virtual_end;

/* These two virtual pages are available for copying and zeroing. */
static vaddr_t tmp_vpages[2];

/* Free list of PV entries. */
static struct pv_entry *pv_free_list;

/* This is an array of struct pv_head, one per physical page. */
static struct pv_head *pv_head_tbl;

/* 
 * This is a bitmap of page-is-aliased bits.
 * The magic 5 is log2(sizeof(u_int) * 8), and the magic 31 is 2^5 - 1.
 */
static u_int *page_aliased_bitmap;
#define _PAGE_ALIASED_WORD(pa) page_aliased_bitmap[((pa) >> PGSHIFT) >> 5]
#define _PAGE_ALIASED_BIT(pa) (1 << (((pa) >> PGSHIFT) & 31))
#define PAGE_IS_ALIASED(pa) (_PAGE_ALIASED_WORD(pa) & _PAGE_ALIASED_BIT(pa))

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;
bool		pmap_initialized = false;

TAILQ_HEAD(, pmap)	pmap_freelist;	/* list of free pmaps */
u_int pmap_nfree;
struct simplelock pmap_freelock;	/* and lock */

struct simplelock pmap_lock;	/* XXX this is all broken */
struct simplelock sid_pid_lock;	/* pids */

u_int	pid_counter;

#ifdef PMAPDEBUG
void pmap_hptdump(void);
#endif

u_int	kern_prot[8], user_prot[8];

vaddr_t	hpt_base;
vsize_t	hpt_mask;

/*
 * Page 3-6 of the "PA-RISC 1.1 Architecture and Instruction Set 
 * Reference Manual" (HP part number 09740-90039) defines equivalent
 * and non-equivalent virtual addresses in the cache.
 *
 * This macro evaluates to true iff the two space/virtual address
 * combinations are non-equivalent aliases, and therefore will find
 * two different locations in the cache.
 *
 * NB: currently, the CPU-specific desidhash() functions disable the
 * use of the space in all cache hashing functions.  This means that
 * this macro definition is stricter than it has to be (because it
 * takes space into account), but one day cache space hashing should 
 * be re-enabled.  Cache space hashing should yield better performance 
 * through better utilization of the cache, assuming that most aliasing 
 * is the read-only kind, which we do allow in the cache.
 */
#define NON_EQUIVALENT_ALIAS(sp1, va1, sp2, va2) \
  (((((va1) ^ (va2)) & ~HPPA_PGAMASK) != 0) || \
   ((((sp1) ^ (sp2)) & ~HPPA_SPAMASK) != 0))

/* Prototypes. */
void __pmap_pv_update(paddr_t, struct pv_entry *, u_int, u_int);
static inline void pmap_pv_remove(struct pv_entry *);
static inline void pmap_pv_add(vaddr_t, vaddr_t);
static inline struct pv_entry *pmap_pv_alloc(void);
static inline void pmap_pv_free(struct pv_entry *);
static inline struct hpt_entry *pmap_hpt_hash(pa_space_t, vaddr_t);
static inline int pmap_table_find_pa(paddr_t);
static inline struct pv_entry *pmap_pv_find_pa(paddr_t);
static inline struct pv_entry *pmap_pv_find_va(pa_space_t, vaddr_t);
static int pmap_pv_check_alias(paddr_t);
static inline void _pmap_pv_update(paddr_t, struct pv_entry *, u_int, u_int);
static inline struct pv_entry *pmap_pv_enter(pmap_t, pa_space_t, vaddr_t,
    paddr_t, u_int);
static void pmap_pinit(pmap_t);
static inline bool pmap_clear_bit(paddr_t, u_int);
static inline bool pmap_test_bit(paddr_t, u_int);

/*
 * Given a directly-mapped region, this makes pv_entries out of it and
 * adds them to the free list.
 */
static inline void
pmap_pv_add(vaddr_t pv_start, vaddr_t pv_end)
{
	struct pv_entry *pv;
	int s;

	/* Align pv_start, then add the new pv_entries. */
	pv_start = (pv_start + sizeof(*pv) - 1) & ~(sizeof(*pv) - 1);
	pv = (struct pv_entry *) pv_start;
	s = splvm();
	while ((vaddr_t)(pv + 1) <= pv_end) {
		pv->pv_next = pv_free_list;
		pv_free_list = pv;
		pv++;
	}
	splx(s);
	
	PMAP_PRINTF(PDB_INIT, (": %ld pv_entries @ %x allocated\n",
		    (pv - (struct pv_entry *) pv_start), (u_int)pv_start));
}

/*
 * This allocates and returns a new struct pv_entry.
 *
 * If we run out of preallocated struct pv_entries, we have to forcibly
 * free one.   malloc() isn't an option, because a) we'll probably end 
 * up back here anyways when malloc() maps what it's trying to return to 
 * us, and b) even if malloc() did succeed, the TLB fault handlers run 
 * in physical mode and thus require that all pv_entries be directly
 * mapped, a quality unlikely for malloc()-returned memory.
 */
static inline struct pv_entry *
pmap_pv_alloc(void)
{
	struct pv_entry *pv, *pv_fallback;
	u_int hpt_index_first, hpt_index, hpt_size;
	struct hpt_entry *hpt;

	pv = pv_free_list;
	if (pv == NULL) {
		/*
		 * We need to find a struct pv_entry to forcibly
		 * free.  It cannot be wired.  We prefer to free 
		 * mappings that aren't marked as referenced.  We 
		 * search the HPT for an entry to free, starting 
		 * at a semirandom HPT index determined by the 
		 * current value of the interval timer.
		 */
		hpt_size = hpt_mask / sizeof(*hpt);
		mfctl(CR_ITMR, hpt_index_first);
		hpt_index = hpt_index_first = hpt_index_first & hpt_size;
		pv_fallback = NULL;
		do {
			hpt = ((struct hpt_entry *) hpt_base) + hpt_index;
			for (pv = hpt->hpt_entry; 
			     pv != NULL; 
			     pv = pv->pv_hash) {
				if (!(pv->pv_tlbprot & TLB_WIRED)) {
					if (!(pv->pv_tlbprot & TLB_REF))
						break;
					pv_fallback = pv;
				}
			}
			if (pv != NULL)
				break;
			if (pv_fallback != NULL) {
				pv = pv_fallback;
				break;
			}
			hpt_index = (hpt_index + 1) & hpt_size;
		} while (hpt_index != hpt_index_first);

		/* Remove the mapping. */
		if (pv != NULL) {
			KASSERT(pv->pv_pmap->pmap_stats.resident_count > 0);
			pv->pv_pmap->pmap_stats.resident_count--;
			pmap_pv_remove(pv);
			pv = pv_free_list;
		}

		if (pv == NULL)
			panic("out of pv_entries");
		
	}
	pv_free_list = pv->pv_next;
	pv->pv_next = NULL;

	PMAP_PRINTF(PDB_PV_ALLOC, ("() = %p\n", pv));
	return pv;
}

/*
 * Given a struct pv_entry allocated by pmap_pv_alloc, this frees it.
 */
static inline void
pmap_pv_free(struct pv_entry *pv)
{
	PMAP_PRINTF(PDB_PV_ALLOC, ("(%p)\n", pv));

	pv->pv_next = pv_free_list;
	pv_free_list = pv;
}

/*
 * Given a VA, this hashes it into an HPT index.
 *
 * This hash function is the one used by the hardware TLB filler on 
 * the 7100LC, to index the hardware page table (HPT), which is sort 
 * of a cache of TLB entries.
 * 
 * On other CPUs, locore.S has a software TLB filler that does exactly 
 * the same thing, right down to using this same hash function.
 *
 * This HPT is also used as a general VA->PA mapping store, with
 * struct pv_entry chains hanging off of the HPT entries.
 */
static inline struct hpt_entry *
pmap_hpt_hash(pa_space_t sp, vaddr_t va)
{
	struct hpt_entry *hpt;
	__asm volatile (
		"extru	%2, 23, 20, %%r22\n\t"	/* r22 = (va >> 8) */
		"zdep	%1, 22, 16, %%r23\n\t"	/* r23 = (sp << 9) */
		"dep	%%r0, 31, 4, %%r22\n\t"	/* r22 &= ~0xf */
		"xor	%%r22,%%r23, %%r23\n\t"	/* r23 ^= r22 */
		"mfctl	%%cr24, %%r22\n\t"	/* r22 = sizeof(HPT)-1 */
		"and	%%r22,%%r23, %%r23\n\t"	/* r23 &= r22 */
		"mfctl	%%cr25, %%r22\n\t"	/* r22 = addr of HPT table */
		"or	%%r23, %%r22, %0"	/* %0 = HPT entry */
		: "=r" (hpt) : "r" (sp), "r" (va) : "r22", "r23");
	return hpt;
}

/*
 * Given a PA, returns the table offset for it.
 */
static inline int
pmap_table_find_pa(paddr_t pa)
{
	int off;

	off = atop(pa);
	return (off < totalphysmem) ? off : -1;
}

/*
 * Given a PA, returns the first mapping for it.
 */
static inline struct pv_entry *
pmap_pv_find_pa(paddr_t pa)
{
	int table_off;

	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	return pv_head_tbl[table_off].pv_head_pvs;
}

/*
 * Given a VA, this finds any mapping for it.
 */
static inline struct pv_entry *
pmap_pv_find_va(pa_space_t space, vaddr_t va)
{
	struct pv_entry *pv = pmap_hpt_hash(space, va)->hpt_entry;

	while(pv && (pv->pv_va != va || pv->pv_space != space))
		pv = pv->pv_hash;

	PMAP_PRINTF(PDB_PV_FIND_VA, ("(0x%x:%p) = %p\n",
					  space, (void *)va, pv));
	return pv;
}

/*
 * Given a page's PA, checks for non-equivalent aliasing, 
 * and stores and returns the result.
 */
static int
pmap_pv_check_alias(paddr_t pa)
{
	struct pv_entry *pv_outer, *pv;
	pa_space_t space;
	vaddr_t va;
	int aliased;
	u_int *aliased_word, aliased_bit;

	/* By default we find no aliasing. */
	aliased = false;

	/*
	 * We should never get called on I/O pages.
	 */
	KASSERT(pa < HPPA_IOSPACE);

	/*
	 * Make an outer loop over the mappings, checking
	 * each following inner mapping for non-equivalent 
	 * aliasing.  If the non-equivalent alias relation 
	 * is deemed transitive, this outer loop only needs 
	 * one iteration.
	 */
	for (pv_outer = pmap_pv_find_pa(pa);
	     pv_outer != NULL;
	     pv_outer = pv_outer->pv_next) {

		/* Load this outer mapping's space and address. */
		space = pv_outer->pv_space;
		va = pv_outer->pv_va;

		/* Do the inner loop. */
		for (pv = pv_outer->pv_next;
		     pv != NULL;
		     pv = pv->pv_next) {
			if (NON_EQUIVALENT_ALIAS(space, va,
				pv->pv_space, pv->pv_va)) {
				aliased = true;
				break;
			}
		}

#ifndef NON_EQUIVALENT_ALIAS_TRANSITIVE
		if (aliased)
#endif /* !NON_EQUIVALENT_ALIAS_TRANSITIVE */
			break;
	}

	/* Store and return the result. */
	aliased_word = &_PAGE_ALIASED_WORD(pa);
	aliased_bit = _PAGE_ALIASED_BIT(pa);
	*aliased_word = (*aliased_word & ~aliased_bit) |
		(aliased ? aliased_bit : 0);
	return aliased;
}

/*
 * Given a VA->PA mapping and tlbprot bits to clear and set,
 * this flushes the mapping from the TLB and cache, and changes
 * the protection accordingly.  This is used when a mapping is
 * changing.
 */
static inline void
_pmap_pv_update(paddr_t pa, struct pv_entry *pv, u_int tlbprot_clear,
    u_int tlbprot_set)
{
	struct pv_entry *ppv;
	int no_rw_alias;

	/*
	 * We should never get called on I/O pages.
	 */
	KASSERT(pa < HPPA_IOSPACE);

	/*
	 * If the TLB protection of this mapping is changing,
	 * check for a change in the no read-write alias state 
	 * of the page.
	 */
	KASSERT((tlbprot_clear & TLB_AR_MASK) == 0 ||
		(tlbprot_clear & TLB_AR_MASK) == TLB_AR_MASK);
	if (tlbprot_clear & TLB_AR_MASK) {

		/*
		 * Assume that no read-write aliasing 
		 * exists.  It does exist if this page is
		 * aliased and any mapping is writable.
		 */
		no_rw_alias = TLB_NO_RW_ALIAS;
		if (PAGE_IS_ALIASED(pa)) {
			for (ppv = pmap_pv_find_pa(pa);
			     ppv != NULL;
			     ppv = ppv->pv_next) {
				if (TLB_AR_WRITABLE(ppv == pv ? 
						    tlbprot_set : 
						    ppv->pv_tlbprot)) {
					no_rw_alias = 0;
					break;
				}
			}
		}

		/* Note if the no read-write alias state has changed. */
		if ((pv->pv_tlbprot & TLB_NO_RW_ALIAS) ^ no_rw_alias) {
			tlbprot_clear |= TLB_NO_RW_ALIAS;
			tlbprot_set |= no_rw_alias;
		}
	}

	/*
	 * Now call our asm helper function.  At the very least, 
	 * this will flush out the requested mapping and change
	 * its protection.  If the changes touch any of TLB_REF,
	 * TLB_DIRTY, and TLB_NO_RW_ALIAS, all mappings of the
	 * page will be flushed and changed.
	 */
	__pmap_pv_update(pa, pv, tlbprot_clear, tlbprot_set);
}
#define pmap_pv_update(pv, tc, ts) \
	_pmap_pv_update(tlbptob((pv)->pv_tlbpage), pv, tc, ts)

/*
 * Given a pmap, a VA, a PA, and a TLB protection, this enters
 * a new mapping and returns the new struct pv_entry.
 */
static inline struct pv_entry *
pmap_pv_enter(pmap_t pmap, pa_space_t space, vaddr_t va, paddr_t pa,
    u_int tlbprot)
{
	struct hpt_entry *hpt = pmap_hpt_hash(space, va);
	int table_off;
	struct pv_head *hpv;
	struct pv_entry *pv, *pv_other;

#ifdef DIAGNOSTIC
	/* Make sure this VA isn't already entered. */
	for (pv = hpt->hpt_entry; pv != NULL; pv = pv->pv_hash)
		if (pv->pv_va == va && pv->pv_space == space)
			panic("pmap_pv_enter: VA already entered");
#endif /* DIAGNOSTIC */

	/*
	 * Allocate a new pv_entry, fill it, and link it into the HPT.
	 */
	pv = pmap_pv_alloc();
	pv->pv_va = va;
	pv->pv_pmap = pmap;
	pv->pv_space = space;
	pv->pv_tlbprot = tlbprot;
	pv->pv_tlbpage = tlbbtop(pa);
	pv->pv_hpt = hpt;
	pv->pv_hash = hpt->hpt_entry;
	hpt->hpt_entry = pv;

	/*
	 * If this mapping is for I/O space, mark the mapping
	 * uncacheable.  (This is fine even on CPUs that don't 
	 * support the U-bit; these CPUs don't cache references 
	 * to I/O space.)  Also mark this mapping as having
	 * no read/write aliasing, and we're done - we don't
	 * keep PA->VA lists for I/O space.
	 */
	if (pa >= HPPA_IOSPACE) {
		KASSERT(tlbprot & TLB_UNMANAGED);
		pv->pv_tlbprot |= TLB_UNCACHEABLE | TLB_NO_RW_ALIAS;
		return pv;
	}

	/* Get the head of the PA->VA translation list. */
	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	hpv = pv_head_tbl + table_off;

#ifdef DIAGNOSTIC
	/* Make sure this VA isn't already entered. */
	for (pv_other = hpv->pv_head_pvs;
	     pv_other != NULL;
	     pv_other = pv_other->pv_next)
		if (pmap == pv_other->pv_pmap && va == pv_other->pv_va)
			panic("pmap_pv_enter: VA already in pv_tab");
#endif /* DIAGNOSTIC */

	/*
	 * Link this mapping into the PA->VA list.
	 */
	pv_other = hpv->pv_head_pvs;
	pv->pv_next = pv_other;
	hpv->pv_head_pvs = pv;

	/*
	 * If there are no other mappings of this page, this
	 * mapping has no read/write aliasing.  Otherwise, give 
	 * this mapping the same TLB_NO_RW_ALIAS status as the 
	 * other mapping (all mappings of the same page must 
	 * always be marked the same).
	 */
	pv->pv_tlbprot |= (pv_other == NULL ?
			   TLB_NO_RW_ALIAS :
			   (pv_other->pv_tlbprot & TLB_NO_RW_ALIAS));

	/* Check for read-write aliasing. */
	if (!PAGE_IS_ALIASED(pa))
		pmap_pv_check_alias(pa);
	_pmap_pv_update(pa, pv, TLB_AR_MASK, tlbprot & TLB_AR_MASK);

	return pv;
}

/*
 * Given a particular VA->PA mapping, this removes it.
 */
static inline void
pmap_pv_remove(struct pv_entry *pv)
{
	paddr_t pa = tlbptob(pv->pv_tlbpage);
	int table_off;
	struct pv_head *hpv;
	struct pv_entry **_pv;

	PMAP_PRINTF(PDB_PV_REMOVE, ("(%p)\n", pv));

	/* Unlink this pv_entry from the HPT. */
	_pv = &pv->pv_hpt->hpt_entry;
	while (*_pv != pv) {
		KASSERT(*_pv != NULL);
		_pv = &(*_pv)->pv_hash;
	}
	*_pv = pv->pv_hash;

	/*
	 * If this mapping is for I/O space, simply flush the 
	 * old mapping, free it, and we're done.
	 */
	if (pa >= HPPA_IOSPACE) {
		__pmap_pv_update(pa, pv, 0, 0);
		pmap_pv_free(pv);
		return;
	}

	/* Get the head of the PA->VA translation list. */
	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	hpv = pv_head_tbl + table_off;

	/* Unlink this pv_entry from the PA->VA translation list. */
	_pv = &hpv->pv_head_pvs;
	while (*_pv != pv) {
		KASSERT(*_pv != NULL);
		_pv = &(*_pv)->pv_next;
	}
	*_pv = pv->pv_next;

	/*
	 * Check for read-write aliasing.  This will also flush
	 * the old mapping.  
	 */
	if (PAGE_IS_ALIASED(pa))
		pmap_pv_check_alias(pa);
	_pmap_pv_update(pa, pv, TLB_AR_MASK, TLB_AR_KR);

	/* Free this mapping. */
	pmap_pv_free(pv);
}

/*
 * Bootstrap the system enough to run with virtual memory.
 * Map the kernel's code, data and bss, and allocate the system page table.
 * Called with mapping OFF.
 *
 * Parameters:
 * vstart	PA of first available physical page
 * vend		PA of last available physical page
 */
void
pmap_bootstrap(vaddr_t *vstart, vaddr_t *vend)
{
	vaddr_t addr, va;
	vsize_t size;
	vaddr_t pv_region;
	struct hpt_entry *hptp;
#define BTLB_SET_SIZE 16
	vaddr_t btlb_entry_start[BTLB_SET_SIZE];
	vsize_t btlb_entry_size[BTLB_SET_SIZE];
	int btlb_entry_vm_prot[BTLB_SET_SIZE];
	int btlb_i, btlb_j;
	vsize_t btlb_entry_min, btlb_entry_max, btlb_entry_got;
	paddr_t ksro, kero, ksrw, kerw;
	paddr_t phys_start, phys_end;

	/* Provided by the linker script */
	extern int kernel_text, __data_start, __rodata_end;

	PMAP_PRINTF(PDB_INIT, (": phys addresses %p - %p\n",
	    (void *)*vstart, (void *)*vend));

	uvm_setpagesize();

	kern_prot[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_NONE]    =TLB_AR_NA;
	kern_prot[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_NONE]    =TLB_AR_KR;
	kern_prot[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE]    =TLB_AR_KRW;
	kern_prot[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE]    =TLB_AR_KRW;
	kern_prot[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_EXECUTE] =TLB_AR_KRX;
	kern_prot[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_EXECUTE] =TLB_AR_KRX;
	kern_prot[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE] =TLB_AR_KRWX;
	kern_prot[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE] =TLB_AR_KRWX;

	user_prot[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_NONE]    =TLB_AR_NA;
	user_prot[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_NONE]    =TLB_AR_UR;
	user_prot[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE]    =TLB_AR_URW;
	user_prot[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE]    =TLB_AR_URW;
	user_prot[VM_PROT_NONE | VM_PROT_NONE  | VM_PROT_EXECUTE] =TLB_AR_URX;
	user_prot[VM_PROT_READ | VM_PROT_NONE  | VM_PROT_EXECUTE] =TLB_AR_URX;
	user_prot[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE] =TLB_AR_URWX;
	user_prot[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE] =TLB_AR_URWX;

	/*
	 * Initialize kernel pmap
	 */
	kernel_pmap = &kernel_pmap_store;
#if	NCPUS > 1
	lock_init(&pmap_lock, false, ETAP_VM_PMAP_SYS, ETAP_VM_PMAP_SYS_I);
#endif	/* NCPUS > 1 */
	simple_lock_init(&kernel_pmap->pmap_lock);
	simple_lock_init(&pmap_freelock);
	simple_lock_init(&sid_pid_lock);

	kernel_pmap->pmap_refcnt = 1;
	kernel_pmap->pmap_space = HPPA_SID_KERNEL;
	kernel_pmap->pmap_pid = HPPA_PID_KERNEL;

	ksro = (paddr_t) &kernel_text;
	kero = ksrw = (paddr_t) &__data_start;

	/*
	 * Allocate various tables and structures.
	 */
	addr = round_page(*vstart);
	virtual_end = *vend;

	/*
	 * Figure out how big the HPT must be, and align
	 * addr to what will be its beginning.  We don't
	 * waste the pages skipped for the alignment; 
	 * they become struct pv_entry pages.
	 */ 
	pv_region = addr;
	mfctl(CR_HPTMASK, size);
	addr = (addr + size) & ~(size);
	pv_free_list = NULL;
	pmap_pv_add(pv_region, addr);

	/* Allocate the HPT */
	for (hptp = (struct hpt_entry *)addr;
	     ((u_int)hptp - addr) <= size; hptp++) {
		hptp->hpt_valid   = 0;
		hptp->hpt_vpn     = 0;
		hptp->hpt_space   = -1;
		hptp->hpt_tlbpage = 0;
		hptp->hpt_tlbprot = 0;
		hptp->hpt_entry   = NULL;
	}
	PMAP_PRINTF(PDB_INIT, (": hpt_table 0x%lx @ %p\n", size + 1,
	    (void *)addr));
	/*
	 * load cr25 with the address of the HPT table
	 * NB: It sez CR_VTOP, but we (and the TLB handlers) know better ...
	 */
	mtctl(addr, CR_VTOP);
	hpt_base = addr;
	hpt_mask = size;
	lwp0.l_md.md_regs->tf_hptm = size;
	lwp0.l_md.md_regs->tf_vtop = addr;
	addr += size + 1;

	/* Allocate the struct pv_head array. */
	addr = ALIGN(addr);
	pv_head_tbl = (struct pv_head *) addr;
	memset(pv_head_tbl, 0, sizeof(*pv_head_tbl) * totalphysmem);
	addr = (vaddr_t) (pv_head_tbl + totalphysmem);

	PMAP_PRINTF(PDB_INIT, (": pv_head array 0x%lx @ %p\n",
	    sizeof(*pv_head_tbl) * totalphysmem, pv_head_tbl));

	/* Allocate the page aliased bitmap. */
	addr = ALIGN(addr);
	page_aliased_bitmap = (u_int *) addr;
	addr = (vaddr_t) (&_PAGE_ALIASED_WORD(totalphysmem) + 1);
	memset(page_aliased_bitmap, 0, addr - (vaddr_t) page_aliased_bitmap);

	PMAP_PRINTF(PDB_INIT, (": page_aliased_bitmap 0x%lx @ %p\n",
	    addr - (vaddr_t) page_aliased_bitmap, page_aliased_bitmap));

	/*
	 * Allocate the largest struct pv_entry region.   The
	 * 6 is a magic constant, chosen to allow on average
	 * all physical pages to have 6 simultaneous mappings 
	 * without having to reclaim any struct pv_entry.
	 */
	pv_region = addr;
	addr += sizeof(struct pv_entry) * totalphysmem * 6;
	pmap_pv_add(pv_region, addr);

	/*
	 * Allocate the steal region.  Because pmap_steal_memory
	 * must panic whenever an allocation cannot be fulfilled,
	 * we have to guess at the maximum amount of space that
	 * might be stolen.  Overestimating is not really a problem,
	 * as it only leads to lost virtual space, not lost physical
	 * pages.
	 */
	addr = round_page(addr);
	virtual_steal = addr;
	addr += totalphysmem * sizeof(struct vm_page);
	memset((void *) virtual_steal, 0, addr - virtual_steal);
	
	/*
	 * We now have a rough idea of where managed kernel virtual
	 * space will begin, and we can start mapping everything
	 * before that.
	 */
	addr = round_page(addr);
	*vstart = addr;
	
	/*
	 * In general, the virtual space below the kernel text is
	 * left unmapped, to allow detection of NULL dereferences.
	 * However, these tmp_vpages are two virtual pages right 
	 * before the kernel text that can be mapped for page copying 
	 * and zeroing.
	 */
	tmp_vpages[1] = trunc_page((vaddr_t) &kernel_text) - PAGE_SIZE;
	tmp_vpages[0] = tmp_vpages[1] - PAGE_SIZE;

	/*
	 * The kernel text, data, and bss must be direct-mapped,
	 * because the kernel often runs in physical mode, and 
	 * anyways the loader loaded the kernel into physical 
	 * memory exactly where it was linked.
	 *
	 * All memory already allocated after bss, either by
	 * our caller or by this function itself, must also be
	 * direct-mapped, because it's completely unmanaged 
	 * and was allocated in physical mode.
	 *
	 * BTLB entries are used to do this direct mapping.
	 * BTLB entries have a minimum and maximum possible size,
	 * and MD code gives us these sizes in units of pages.
	 */
	btlb_entry_min = (vsize_t) hppa_btlb_size_min * PAGE_SIZE;
	btlb_entry_max = (vsize_t) hppa_btlb_size_max * PAGE_SIZE;

	/*
	 * We begin by making BTLB entries for the kernel text.
	 * To keep things simple, we insist that the kernel text
	 * be aligned to the minimum BTLB entry size.
	 */
	if (((vaddr_t) &kernel_text) & (btlb_entry_min - 1))
		panic("kernel text not aligned to BTLB minimum size");

	/*
	 * To try to conserve BTLB entries, take a hint from how 
	 * the kernel was linked: take the kernel text start as 
	 * our effective minimum BTLB entry size, assuming that
	 * the data segment was also aligned to that size.
	 *
	 * In practice, linking the kernel at 2MB, and aligning
	 * the data segment to a 2MB boundary, should control well
	 * how much of the BTLB the pmap uses.  However, this code
	 * should not rely on this 2MB magic number, nor should
	 * it rely on the data segment being aligned at all.  This
	 * is to allow (smaller) kernels (linked lower) to work fine.
	 */
	btlb_entry_min = (vaddr_t) &kernel_text;

	/*
	 * Now make BTLB entries to direct-map the kernel text
	 * read- and execute-only as much as possible.  Note that
	 * if the data segment isn't nicely aligned, the last
	 * BTLB entry for the kernel text may also cover some of
	 * the data segment, meaning it will have to allow writing.
	 */
	addr = ksro;

	PMAP_PRINTF(PDB_INIT, (": mapping text and rodata @ %p - %p\n",
	    (void *)addr, (void *)&__rodata_end));

	btlb_j = 0;
	while (addr < (vaddr_t) &__rodata_end) {

		/* Set up the next BTLB entry. */
		KASSERT(btlb_j < BTLB_SET_SIZE);
		btlb_entry_start[btlb_j] = addr;
		btlb_entry_size[btlb_j] = btlb_entry_min;
		btlb_entry_vm_prot[btlb_j] = VM_PROT_READ | VM_PROT_EXECUTE;
		if (addr + btlb_entry_min > kero)
			btlb_entry_vm_prot[btlb_j] |= VM_PROT_WRITE;

		/* Coalesce BTLB entries whenever possible. */
		while (btlb_j > 0 &&
		    btlb_entry_vm_prot[btlb_j] == 
			btlb_entry_vm_prot[btlb_j - 1] &&
		    btlb_entry_size[btlb_j] ==
			btlb_entry_size[btlb_j - 1] &&
		    !(btlb_entry_start[btlb_j - 1] &
			((btlb_entry_size[btlb_j - 1] << 1) - 1)) &&
		    (btlb_entry_size[btlb_j - 1] << 1) <=
			btlb_entry_max)
			btlb_entry_size[--btlb_j] <<= 1;

		/* Move on. */
		addr = btlb_entry_start[btlb_j] + btlb_entry_size[btlb_j];
		btlb_j++;
	} 

	/*
	 * Now make BTLB entries to direct-map the kernel data,
	 * bss, and all of the preallocated space read-write.  
	 * 
	 * Note that, unlike above, we're not concerned with 
	 * making these BTLB entries such that they finish as 
	 * close as possible to the end of the space we need 
	 * them to map.  Instead, to minimize the number of BTLB 
	 * entries we need, we make them as large as possible.
	 * The only thing this wastes is kernel virtual space,
	 * which is plentiful.
	 */

	PMAP_PRINTF(PDB_INIT, (": mapping data, bss, etc @ %p - %p\n",
	    (void *)addr, (void *)*vstart));

	while (addr < *vstart) {

		/* Make the next BTLB entry. */
		KASSERT(btlb_j < BTLB_SET_SIZE);
		size = btlb_entry_min;
		while ((addr + size) < *vstart &&
			(size << 1) < btlb_entry_max &&
		    !(addr & ((size << 1) - 1)))
			size <<= 1;
		btlb_entry_start[btlb_j] = addr;
		btlb_entry_size[btlb_j] = size;
		btlb_entry_vm_prot[btlb_j] = VM_PROT_READ | VM_PROT_WRITE;

		/* Move on. */
		addr = btlb_entry_start[btlb_j] + btlb_entry_size[btlb_j];
		btlb_j++;
	} 

	/* Now insert all of the BTLB entries. */
	for (btlb_i = 0; btlb_i < btlb_j; btlb_i++) {
		btlb_entry_got = btlb_entry_size[btlb_i];
		if (hppa_btlb_insert(kernel_pmap->pmap_space, 
				btlb_entry_start[btlb_i],
				btlb_entry_start[btlb_i],
				&btlb_entry_got,
				kernel_pmap->pmap_pid |
				pmap_prot(kernel_pmap, 
					btlb_entry_vm_prot[btlb_i])) < 0)
			panic("pmap_bootstrap: cannot insert BTLB entry");
		if (btlb_entry_got != btlb_entry_size[btlb_i])
			panic("pmap_bootstrap: BTLB entry mapped wrong amount");
	}

	/*
	 * We now know the exact beginning of managed kernel
	 * virtual space.
	 */
	*vstart = btlb_entry_start[btlb_j - 1] + btlb_entry_size[btlb_j - 1];
	virtual_start = *vstart;
	kerw = *vstart;

	/*
	 * Finally, load physical pages into UVM.  There are
	 * three segments of pages.
	 */
	physmem = 0;
	 
	/* The first segment runs from [resvmem..kernel_text). */
	phys_start = resvmem;
	phys_end = atop(&kernel_text);

	PMAP_PRINTF(PDB_INIT, (": phys segment 0x%05x 0x%05x\n",
	    (u_int)phys_start, (u_int)phys_end));
	if (phys_end > phys_start) {
		uvm_page_physload(phys_start, phys_end,
			phys_start, phys_end, VM_FREELIST_DEFAULT);
		physmem += phys_end - phys_start;
	}

	/* The second segment runs from [__rodata_end..__data_start). */
	phys_start = atop(&__rodata_end);
	phys_end = atop(&__data_start);

	PMAP_PRINTF(PDB_INIT, (": phys segment 0x%05x 0x%05x\n",
	    (u_int)phys_start, (u_int)phys_end));
	if (phys_end > phys_start) {
		uvm_page_physload(phys_start, phys_end,
			phys_start, phys_end, VM_FREELIST_DEFAULT);
		physmem += phys_end - phys_start;
	}

	/* The third segment runs from [virtual_steal..totalphysmem). */
	phys_start = atop(virtual_steal);
	phys_end = totalphysmem;

	PMAP_PRINTF(PDB_INIT, (": phys segment 0x%05x 0x%05x\n",
	    (u_int)phys_start, (u_int)phys_end));
	if (phys_end > phys_start) {
		uvm_page_physload(phys_start, phys_end,
			phys_start, phys_end, VM_FREELIST_DEFAULT);
		physmem += phys_end - phys_start;
	}

	for (va = ksro; va < kero; va += PAGE_SIZE)
		pmap_kenter_pa(va, va, UVM_PROT_RX);
	for (va = ksrw; va < kerw; va += PAGE_SIZE)
		pmap_kenter_pa(va, va, UVM_PROT_RW);

}

/*
 * pmap_steal_memory(size, startp, endp)
 *	steals memory block of size `size' from directly mapped
 *	segment (mapped behind the scenes).
 *	directly mapped cannot be grown dynamically once allocated.
 */
vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *startp, vaddr_t *endp)
{
	vaddr_t va;
	int lcv;

	PMAP_PRINTF(PDB_STEAL, ("(%lx, %p, %p)\n", size, startp, endp));

	/* Remind the caller of the start and end of virtual space. */
	if (startp)
		*startp = virtual_start;
	if (endp)
		*endp = virtual_end;

	/* Round the allocation up to a page. */
	size = round_page(size);

	/* We must panic if we cannot steal the memory. */
	if (size > virtual_start - virtual_steal)
		panic("pmap_steal_memory: out of memory");

	/* Steal the memory. */
	va = virtual_steal;
	virtual_steal += size;
	PMAP_PRINTF(PDB_STEAL, (": steal %ld bytes @%x\n", size, (u_int)va));
	for (lcv = 0; lcv < vm_nphysseg ; lcv++)
		if (vm_physmem[lcv].start == atop(va)) {
			vm_physmem[lcv].start = atop(virtual_steal);
			vm_physmem[lcv].avail_start = atop(virtual_steal);
			break;
		}
	if (lcv == vm_nphysseg)
		panic("pmap_steal_memory inconsistency");

	return va;
}

/* 
 * How much virtual space does this kernel have?
 * (After mapping kernel text, data, etc.)
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	*vstartp = virtual_start;
	*vendp = virtual_end;
}

/*
 * Finishes the initialization of the pmap module.
 * This procedure is called from vm_mem_init() in vm/vm_init.c
 * to initialize any remaining data structures that the pmap module
 * needs to map virtual memory (VM is already ON).
 */
void
pmap_init(void)
{
	extern void gateway_page(void);

	TAILQ_INIT(&pmap_freelist);
	pid_counter = HPPA_PID_KERNEL + 2;

	/*
	 * map SysCall gateways page once for everybody
	 * NB: we'll have to remap the phys memory
	 *     if we have any at SYSCALLGATE address (;
	 *
	 * no spls since no interrupts
	 */
	pmap_pv_enter(pmap_kernel(), HPPA_SID_KERNEL, SYSCALLGATE,
		      (paddr_t)&gateway_page, 
		      TLB_GATE_PROT | TLB_UNMANAGED | TLB_WIRED);

	pmap_initialized = true;
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
static void
pmap_pinit(pmap_t pmap)
{
	u_int pid;
	int s;

	PMAP_PRINTF(PDB_PMAP, ("(%p), pid=%x\n", pmap, pmap->pmap_pid));

	if (!(pid = pmap->pmap_pid)) {

		/*
		 * Allocate space and protection IDs for the pmap.
		 * If all are allocated, there is nothing we can do.
		 */
		s = splvm();
		if (pid_counter < HPPA_MAX_PID) {
			pid = pid_counter;
			pid_counter += 2;
		} else
			pid = 0;
		splx(s);

		if (pid == 0)
			panic ("no more pmap ids\n");

		simple_lock_init(&pmap->pmap_lock);
	}

	s = splvm();
	pmap->pmap_pid = pid;
	pmap->pmap_space = (pmap->pmap_pid >> 1) - 1;
	pmap->pmap_refcnt = 1;
	pmap->pmap_stats.resident_count = 0;
	pmap->pmap_stats.wired_count = 0;
	splx(s);
}

/*
 * pmap_create()
 *
 * Create and return a physical map.
 * the map is an actual physical map, and may be referenced by the hardware.
 */
pmap_t
pmap_create(void)
{
	pmap_t pmap;
	int s;

	PMAP_PRINTF(PDB_PMAP, ("()"));

	/*
	 * If there is a pmap in the pmap free list, reuse it.
	 */
	s = splvm();
	if (pmap_nfree) {
		pmap = pmap_freelist.tqh_first;
		TAILQ_REMOVE(&pmap_freelist, pmap, pmap_list);
		pmap_nfree--;
		splx(s);
	} else {
		splx(s);
		MALLOC(pmap, struct pmap *, sizeof(*pmap), M_VMPMAP, M_NOWAIT);
		if (pmap == NULL)
			return NULL;
		memset(pmap, 0, sizeof(*pmap));
	}

	pmap_pinit(pmap);

	return(pmap);
}

/*
 * pmap_destroy(pmap)
 *	Gives up a reference to the specified pmap.  When the reference count
 *	reaches zero the pmap structure is added to the pmap free list.
 *	Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pmap_t pmap)
{
	int ref_count;
	int s;

	PMAP_PRINTF(PDB_PMAP, ("(%p)\n", pmap));

	s = splvm();

	ref_count = --pmap->pmap_refcnt;

	if (ref_count < 0)
		panic("pmap_destroy(): ref_count < 0");
	if (!ref_count) {
		KASSERT(pmap->pmap_stats.resident_count == 0);
		KASSERT(pmap->pmap_stats.wired_count == 0);

		/*
		 * Add the pmap to the pmap free list
		 * We cannot free() disposed pmaps because of
		 * PID shortage of 2^16
		 */
		TAILQ_INSERT_HEAD(&pmap_freelist, pmap, pmap_list);
		pmap_nfree++;
	}
	splx(s);
}

/*
 * pmap_activate(lwp)
 *	Activates the vmspace for the given LWP.  This
 *	isn't necessarily the current process.
 */
void
pmap_activate(struct lwp *l)
{

	struct proc *p = l->l_proc;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	pa_space_t space = pmap->pmap_space;
	struct pcb *pcb = &l->l_addr->u_pcb;

	/* space is cached for the copy{in,out}'s pleasure */
	pcb->pcb_space = space;

	if (p == curproc)
		mtctl(pmap->pmap_pid, CR_PIDR2);
}

/*
 * pmap_enter(pmap, va, pa, prot, flags)
 *	Create a translation for the virtual address (va) to the physical
 *	address (pa) in the pmap with the protection requested. If the
 *	translation is wired then we can not allow a page fault to occur
 *	for this mapping.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	struct pv_entry *pv;
	u_int tlbpage, tlbprot;
	pa_space_t space;
	bool waswired;
	bool wired = (flags & PMAP_WIRED) != 0;
	int s;

	/* Get a handle on the mapping we want to enter. */
	space = pmap_sid(pmap, va);
	va = trunc_page(va);
	pa = trunc_page(pa);
	tlbpage = tlbbtop(pa);
	tlbprot = pmap_prot(pmap, prot) | pmap->pmap_pid;
	if (wired)
		tlbprot |= TLB_WIRED;
	if (flags & VM_PROT_ALL) {
		tlbprot |= TLB_REF;
		if (flags & VM_PROT_WRITE)
			tlbprot |= TLB_DIRTY;
	}

#ifdef PMAPDEBUG
	if (!pmap_initialized || (pmapdebug & PDB_ENTER))
		PMAP_PRINTF(0, ("(%p, %p, %p, %x, %swired)\n", 
				pmap, (void *)va, (void *)pa,
				prot, wired? "" : "un"));
#endif

	s = splvm();

	if (!(pv = pmap_pv_find_va(space, va))) {
		/*
		 * Mapping for this virtual address doesn't exist.
		 * Enter a new mapping.
		 */
		pv = pmap_pv_enter(pmap, space, va, pa, tlbprot);
		pmap->pmap_stats.resident_count++;
		waswired = false;
	} else {
		KASSERT((pv->pv_tlbprot & TLB_UNMANAGED) == 0);
		waswired = pv->pv_tlbprot & TLB_WIRED;

		/* see if we are remapping the page to another PA */
		if (pv->pv_tlbpage != tlbpage) {
			PMAP_PRINTF(PDB_ENTER, (": moving pa %x -> %x\n",
						pv->pv_tlbpage, tlbpage));
			/* update tlbprot to avoid extra subsequent fault */
			pmap_pv_remove(pv);
			pv = pmap_pv_enter(pmap, space, va, pa, tlbprot);
		} else {
			/* We are just changing the protection.  */
#ifdef PMAPDEBUG
			if (pmapdebug & PDB_ENTER) {
				char buffer1[64];
				char buffer2[64];
				bitmask_snprintf(pv->pv_tlbprot, TLB_BITS, 
						 buffer1, sizeof(buffer1));
				bitmask_snprintf(tlbprot, TLB_BITS, 
						 buffer2, sizeof(buffer2));
				printf("pmap_enter: changing %s->%s\n",
				    buffer1, buffer2);
			}
#endif
			pmap_pv_update(pv, TLB_AR_MASK|TLB_PID_MASK|TLB_WIRED, 
				       tlbprot);
		}
	}

	/*
	 * Adjust statistics
	 */
	if (wired && !waswired) {
		pmap->pmap_stats.wired_count++;
	} else if (!wired && waswired) {
		pmap->pmap_stats.wired_count--;
	}
	splx(s);

	return (0);
}

/*
 * pmap_remove(pmap, sva, eva)
 *	unmaps all virtual addresses v in the virtual address
 *	range determined by [sva, eva) and pmap.
 *	sva and eva must be on machine independent page boundaries and
 *	sva must be less than or equal to eva.
 */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct pv_entry *pv;
	pa_space_t space;
	int s;

	PMAP_PRINTF(PDB_REMOVE, ("(%p, %p, %p)\n", 
				 pmap, (void *)sva, (void *)eva));

	sva = trunc_page(sva);
	space = pmap_sid(pmap, sva);

	s = splvm();

	while (sva < eva) {
		pv = pmap_pv_find_va(space, sva);
		if (pv) {
			KASSERT((pv->pv_tlbprot & TLB_UNMANAGED) == 0);
			KASSERT(pmap->pmap_stats.resident_count > 0);
			pmap->pmap_stats.resident_count--;
			if (pv->pv_tlbprot & TLB_WIRED) {
				KASSERT(pmap->pmap_stats.wired_count > 0);
				pmap->pmap_stats.wired_count--;
			}
			pmap_pv_remove(pv);
			PMAP_PRINTF(PDB_REMOVE, (": removed %p for 0x%x:%p\n",
						 pv, space, (void *)sva));
		}
		sva += PAGE_SIZE;
	}

	splx(s);
}

/*
 *	pmap_page_protect(pa, prot)
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct pv_entry *pv, *pv_next;
	pmap_t pmap;
	u_int tlbprot;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int s;

	PMAP_PRINTF(PDB_PROTECT, ("(%p, %x)\n", (void *)pa, prot));

	switch (prot) {
	case VM_PROT_ALL:
		return;
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		s = splvm();
		for (pv = pmap_pv_find_pa(pa); pv; pv = pv->pv_next) {
			/* Ignore unmanaged mappings. */
			if (pv->pv_tlbprot & TLB_UNMANAGED)
				continue;
			/*
			 * Compare new protection with old to see if
			 * anything needs to be changed.
			 */
			tlbprot = pmap_prot(pv->pv_pmap, prot);
			if ((pv->pv_tlbprot & TLB_AR_MASK) != tlbprot) {
				pmap_pv_update(pv, TLB_AR_MASK, tlbprot);
			}
		}
		splx(s);
		break;
	default:
		s = splvm();
		for (pv = pmap_pv_find_pa(pa); pv != NULL; pv = pv_next) {
			pv_next = pv->pv_next;
			/* Ignore unmanaged mappings. */
			if (pv->pv_tlbprot & TLB_UNMANAGED)
				continue;
#ifdef PMAPDEBUG
			if (pmapdebug & PDB_PROTECT) {
				char buffer[64];
				bitmask_snprintf(pv->pv_tlbprot, TLB_BITS,
						 buffer, sizeof(buffer));
				printf("pv={%p,%x:%x,%s,%x}->%p\n",
				    pv->pv_pmap, pv->pv_space, pv->pv_va,
				    buffer,
				    tlbptob(pv->pv_tlbpage), pv->pv_hash);
			}
#endif
			pmap = pv->pv_pmap;
			if (pv->pv_tlbprot & TLB_WIRED) {
				KASSERT(pmap->pmap_stats.wired_count > 0);
				pmap->pmap_stats.wired_count--;
			}
			pmap_pv_remove(pv);
			KASSERT(pmap->pmap_stats.resident_count > 0);
			pmap->pmap_stats.resident_count--;
		}
		splx(s);
		break;
	}
}

/*
 * pmap_protect(pmap, s, e, prot)
 *	changes the protection on all virtual addresses v in the
 *	virtual address range determined by [s, e) and pmap to prot.
 *	s and e must be on machine independent page boundaries and
 *	s must be less than or equal to e.
 */
void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	struct pv_entry *pv;
	u_int tlbprot;
	pa_space_t space;
	int s;

	PMAP_PRINTF(PDB_PROTECT, ("(%p, %p, %p, %x)\n", 
				 pmap, (void *)sva, (void *)eva, prot));

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	sva = trunc_page(sva);
	space = pmap_sid(pmap, sva);
	tlbprot = pmap_prot(pmap, prot);

	s = splvm();
	for(; sva < eva; sva += PAGE_SIZE) {
		if ((pv = pmap_pv_find_va(space, sva))) {

			/*
			 * Compare new protection with old to see if
			 * anything needs to be changed.
			 */
			if ((pv->pv_tlbprot & TLB_AR_MASK) != tlbprot) {
				pmap_pv_update(pv, TLB_AR_MASK, tlbprot);
			}
		}
	}
	splx(s);
}

/*
 *	Routine:	pmap_unwire
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 *
 * Change the wiring for a given virtual page. This routine currently is
 * only used to unwire pages and hence the mapping entry will exist.
 */
void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	struct pv_entry *pv;
	int s;

	va = trunc_page(va);
	PMAP_PRINTF(PDB_WIRING, ("(%p, %p)\n", pmap, (void *)va));

	simple_lock(&pmap->pmap_lock);

	s = splvm();
	if ((pv = pmap_pv_find_va(pmap_sid(pmap, va), va)) == NULL)
		panic("pmap_unwire: can't find mapping entry");

	KASSERT((pv->pv_tlbprot & TLB_UNMANAGED) == 0);
	if (pv->pv_tlbprot & TLB_WIRED) {
		KASSERT(pmap->pmap_stats.wired_count > 0);
		pv->pv_tlbprot &= ~TLB_WIRED;
		pmap->pmap_stats.wired_count--;
	}
	splx(s);
	simple_unlock(&pmap->pmap_lock);
}

/*
 * pmap_extract(pmap, va, pap)
 *	fills in the physical address corrsponding to the
 *	virtual address specified by pmap and va into the
 *	storage pointed to by pap and returns true if the
 *	virtual address is mapped. returns false in not mapped.
 */
bool
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	struct pv_entry *pv;
	vaddr_t off;
	int s;

	off = va;
	off -= (va = trunc_page(va));

	s = splvm();
	if ((pv = pmap_pv_find_va(pmap_sid(pmap, va), va))) {
		if (pap != NULL)
			*pap = tlbptob(pv->pv_tlbpage) + off;
		PMAP_PRINTF(PDB_EXTRACT, ("(%p, %p) = %p\n",
				pmap, (void *)va, 
				(void *)(tlbptob(pv->pv_tlbpage) + off)));
	} else {
		PMAP_PRINTF(PDB_EXTRACT, ("(%p, %p) unmapped\n",
					 pmap, (void *)va));
	}
	splx(s);
	return (pv != NULL);
}

/*
 * pmap_zero_page(pa)
 *
 * Zeros the specified page.
 */
void
pmap_zero_page(paddr_t pa)
{
	struct pv_entry *pv;
	int s;

	PMAP_PRINTF(PDB_ZERO, ("(%p)\n", (void *)pa));

	s = splvm(); /* XXX are we already that high? */

	/* Map the physical page. */
	pv = pmap_pv_enter(pmap_kernel(), HPPA_SID_KERNEL, tmp_vpages[1], pa, 
			TLB_AR_KRW | TLB_UNMANAGED | TLB_WIRED);

	/* Zero it. */
	memset((void *)tmp_vpages[1], 0, PAGE_SIZE);

	/* Unmap the physical page. */
	pmap_pv_remove(pv);

	splx(s);
}

/*
 * pmap_copy_page(src, dst)
 *
 * pmap_copy_page copies the src page to the destination page. If a mapping
 * can be found for the source, we use that virtual address. Otherwise, a
 * slower physical page copy must be done. The destination is always a
 * physical address sivnce there is usually no mapping for it.
 */
void
pmap_copy_page(paddr_t spa, paddr_t dpa)
{
	struct pv_entry *spv, *dpv;
	int s;

	PMAP_PRINTF(PDB_COPY, ("(%p, %p)\n", (void *)spa, (void *)dpa));

	s = splvm(); /* XXX are we already that high? */

	/* Map the two pages. */
	spv = pmap_pv_enter(pmap_kernel(), HPPA_SID_KERNEL, tmp_vpages[0], spa, 
			TLB_AR_KR | TLB_UNMANAGED | TLB_WIRED);
	dpv = pmap_pv_enter(pmap_kernel(), HPPA_SID_KERNEL, tmp_vpages[1], dpa, 
			TLB_AR_KRW | TLB_UNMANAGED | TLB_WIRED);

	/* Do the copy. */
	memcpy((void *)tmp_vpages[1], (const void *)tmp_vpages[0], PAGE_SIZE);

	/* Unmap the pages. */
	pmap_pv_remove(spv);
	pmap_pv_remove(dpv);

	splx(s);
}

/*
 * Given a PA and a bit, this tests and clears that bit in 
 * the modref information for the PA.
 */
static inline bool
pmap_clear_bit(paddr_t pa, u_int tlbprot_bit)
{
	int table_off;
	struct pv_head *hpv;
	u_int pv_head_bit;
	bool ret;
	int s;

	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	hpv = pv_head_tbl + table_off;
	pv_head_bit = (tlbprot_bit == TLB_REF ? PV_HEAD_REF : PV_HEAD_DIRTY);
	s = splvm();
	_pmap_pv_update(pa, NULL, tlbprot_bit, 0);
	ret = hpv->pv_head_writable_dirty_ref & pv_head_bit;
	hpv->pv_head_writable_dirty_ref &= ~pv_head_bit;
	splx(s);
	return ret;
}

/*
 * Given a PA and a bit, this tests that bit in the modref
 * information for the PA.
 */
static inline bool
pmap_test_bit(paddr_t pa, u_int tlbprot_bit)
{
	int table_off;
	struct pv_head *hpv;
	u_int pv_head_bit;
	struct pv_entry *pv;
	bool ret;
	int s;

	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	hpv = pv_head_tbl + table_off;
	pv_head_bit = (tlbprot_bit == TLB_REF ? PV_HEAD_REF : PV_HEAD_DIRTY);
	s = splvm();
	ret = (hpv->pv_head_writable_dirty_ref & pv_head_bit) != 0;
	if (!ret) {
		for (pv = hpv->pv_head_pvs;
		     pv != NULL; 
		     pv = pv->pv_next) {
			if ((pv->pv_tlbprot & (TLB_UNMANAGED | tlbprot_bit)) ==
			    tlbprot_bit) {
				hpv->pv_head_writable_dirty_ref |= pv_head_bit;
				ret = true;
				break;
			}
		}
	}
	splx(s);
	return ret;
}

/*
 * pmap_clear_modify(pa)
 *	clears the hardware modified ("dirty") bit for one
 *	machine independent page starting at the given
 *	physical address.  phys must be aligned on a machine
 *	independent page boundary.
 */
bool
pmap_clear_modify(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	bool ret = pmap_clear_bit(pa, TLB_DIRTY);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (void *)pa, ret));
	return ret;
}

/*
 * pmap_is_modified(pa)
 *	returns true if the given physical page has been modified
 *	since the last call to pmap_clear_modify().
 */
bool
pmap_is_modified(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	bool ret = pmap_test_bit(pa, TLB_DIRTY);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (void *)pa, ret));
	return ret;
}

/*
 * pmap_clear_reference(pa)
 *	clears the hardware referenced bit in the given machine
 *	independent physical page.
 *
 *	Currently, we treat a TLB miss as a reference; i.e. to clear
 *	the reference bit we flush all mappings for pa from the TLBs.
 */
bool
pmap_clear_reference(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	bool ret = pmap_clear_bit(pa, TLB_REF);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (void *)pa, ret));
	return ret;
}

/*
 * pmap_is_referenced(pa)
 *	returns true if the given physical page has been referenced
 *	since the last call to pmap_clear_reference().
 */
bool
pmap_is_referenced(struct vm_page *pg)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	bool ret = pmap_test_bit(pa, TLB_REF);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (void *)pa, ret));
	return ret;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	u_int tlbprot;
	int s;
#ifdef PMAPDEBUG
	int opmapdebug = pmapdebug;

	/*
	 * If we're being told to map page zero, we can't
	 * call printf() at all, because doing so would 
	 * lead to an infinite recursion on this call.
	 * (printf requires page zero to be mapped).
	 */
	if (va == 0)
		pmapdebug = 0;
#endif /* PMAPDEBUG */

	PMAP_PRINTF(PDB_KENTER, ("(%p, %p, %x)\n", 
				 (void *)va, (void *)pa, prot));
	va = trunc_page(va);
	tlbprot = TLB_WIRED | TLB_UNMANAGED;
	tlbprot |= (prot & PMAP_NC) ? TLB_UNCACHEABLE : 0;
	tlbprot |= pmap_prot(pmap_kernel(), prot & VM_PROT_ALL);
	s = splvm();
	KASSERT(pmap_pv_find_va(HPPA_SID_KERNEL, va) == NULL);
	pmap_pv_enter(pmap_kernel(), HPPA_SID_KERNEL, va, pa, tlbprot);
	splx(s);

#ifdef PMAPDEBUG
	pmapdebug = opmapdebug;
#endif /* PMAPDEBUG */
}

void
pmap_kremove(vaddr_t va, vsize_t size)
{
	struct pv_entry *pv;
	int s;
#ifdef PMAPDEBUG
	int opmapdebug = pmapdebug;

	/*
	 * If we're being told to unmap page zero, we can't
	 * call printf() at all, because doing so would 
	 * lead to an infinite recursion on this call.
	 * (printf requires page zero to be mapped).
	 */
	if (va == 0)
		pmapdebug = 0;
#endif /* PMAPDEBUG */

	PMAP_PRINTF(PDB_KENTER, ("(%p, %x)\n", 
				 (void *)va, (u_int)size));

	size += va;
	va = trunc_page(va);
	size -= va;
	s = splvm();
	for (size = round_page(size); size;
	    size -= PAGE_SIZE, va += PAGE_SIZE) {
		pv = pmap_pv_find_va(HPPA_SID_KERNEL, va);
		if (pv) {
			KASSERT((pv->pv_tlbprot & TLB_UNMANAGED) != 0);
			pmap_pv_remove(pv);
		} else {
			PMAP_PRINTF(PDB_REMOVE, (": no pv for %p\n",
						 (void *)va));
		}
	}
	splx(s);
#ifdef PMAPDEBUG
	pmapdebug = opmapdebug;
#endif /* PMAPDEBUG */
}

/*
 * pmap_redzone(sva, eva, create)
 *	creates or removes a red zone in already mapped and wired memory, 
 *	from [sva, eva) in the kernel map.
 */
void
pmap_redzone(vaddr_t sva, vaddr_t eva, int create)
{
	vaddr_t va;
	struct pv_entry *pv;
	u_int tlbprot;
	int s;
	
	sva = trunc_page(sva);
	tlbprot = (create ? TLB_AR_NA : TLB_AR_KRW);
	s = splvm();
	for(va = sva; va < eva; va += PAGE_SIZE) {
		pv = pmap_pv_find_va(HPPA_SID_KERNEL, va);
		KASSERT(pv != NULL);
		/*
		 * Compare new protection with old to see if
		 * anything needs to be changed.
		 */
		if ((pv->pv_tlbprot & TLB_AR_MASK) != tlbprot)
			pmap_pv_update(pv, TLB_AR_MASK, tlbprot);
	}
	splx(s);
}

#if defined(PMAPDEBUG) && defined(DDB)
#include <ddb/db_output.h>
/*
 * prints whole va->pa (aka HPT or HVT)
 */
void
pmap_hptdump(void)
{
	struct hpt_entry *hpt, *ehpt;
	struct pv_entry *pv;

	mfctl(CR_HPTMASK, ehpt);
	mfctl(CR_VTOP, hpt);
	ehpt = (struct hpt_entry *)((int)hpt + (int)ehpt + 1);
	db_printf("HPT dump %p-%p:\n", hpt, ehpt);
	for (; hpt < ehpt; hpt++)
		if (hpt->hpt_valid || hpt->hpt_entry) {
			char buf[128];

			bitmask_snprintf(hpt->hpt_tlbprot, TLB_BITS, buf,
			    sizeof(buf));
			db_printf("hpt@%p: %x{%sv=%x:%x},%s,%x\n",
			    hpt, *(int *)hpt, (hpt->hpt_valid?"ok,":""),
			    hpt->hpt_space, hpt->hpt_vpn << 9,
			    buf, tlbptob(hpt->hpt_tlbpage));

			for (pv = hpt->hpt_entry; pv; pv = pv->pv_hash) {
				bitmask_snprintf(hpt->hpt_tlbprot, TLB_BITS, buf,
				    sizeof(buf));
				db_printf("    pv={%p,%x:%x,%s,%x}->%p\n",
				    pv->pv_pmap, pv->pv_space, pv->pv_va,
				    buf, tlbptob(pv->pv_tlbpage), pv->pv_hash);
			}
		}
}
#endif
