/*	$NetBSD: pmap.c,v 1.1 2002/06/05 01:04:20 fredette Exp $	*/

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
 *	PAGE_SIZE must equal NBPG
 *	Needs more work for MP support
 *	page maps are stored as linear linked lists, some
 *		improvement may be achieved should we use smth else
 *	protection id (pid) allocation should be done in a pid_t fashion
 *		(maybe just use the pid itself)
 *	some ppl say, block tlb entries should be maintained somewhere in uvm
 *		and be ready for reloads in the fault handler.
 *	usage of __inline grows the code size by 100%, but hopefully
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
#include <machine/pdc.h>
#include <machine/iomod.h>

#include <hppa/hppa/hpt.h>
#include <hppa/hppa/machdep.h>

#define static	/**/
#define	__inline /* */

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
    printf("%s", __FUNCTION__);		\
    printf x;				\
  }					\
} while(/* CONSTCOND */ 0)
#else
#define PMAP_PRINTF_MASK(m,v,x) do { } while(/* CONSTCOND */ 0)
#endif
#define PMAP_PRINTF(v,x) PMAP_PRINTF_MASK(v,v,x)

vaddr_t	virtual_steal, virtual_stolen, virtual_start, virtual_end;

/* These two virtual pages are available for copying and zeroing. */
static vaddr_t tmp_vpages[2];

/* Free list of PV entries. */
static struct pv_entry *pv_free_list;

/*
 * These tables have one entry per physical page.  While
 * memory may be sparse, these tables are always dense;
 * we navigate them with the help of vm_physseg_find.
 */

/* This table is heads of struct pv_entry lists. */
static struct pv_entry **pv_head_tbl;

/*
 * This table is modified/referenced information.  Here,
 * TLB_REF and TLB_DIRTY are shifted right by PV_SHIFT
 * so they fit in the u_char.
 */
static u_char *pv_flags_tbl;
#define PV_SHIFT	24

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;
boolean_t	pmap_initialized = FALSE;

TAILQ_HEAD(, pmap)	pmap_freelist;	/* list of free pmaps */
u_int pmap_nfree;
struct simplelock pmap_freelock;	/* and lock */

struct simplelock pmap_lock;	/* XXX this is all broken */
struct simplelock sid_pid_lock;	/* pids */

u_int	pages_per_vm_page;
u_int	pid_counter;

#ifdef PMAPDEBUG
void pmap_hptdump __P((void));
#endif

u_int	kern_prot[8], user_prot[8];

vaddr_t	hpt_base;
vsize_t	hpt_mask;

#define	pmap_sid(pmap, va) \
	(((va & 0xc0000000) != 0xc0000000)? pmap->pmap_space : HPPA_SID_KERNEL)

/*
 * XXX For now, we assume that two VAs mapped to the same PA, for 
 * any kind of accesses, are always bad (non-equivalent) aliases.  
 * See page 3-6 of the "PA-RISC 1.1 Architecture and Instruction 
 * Set Reference Manual" (HP part number 09740-90039) for more on 
 * aliasing.
 */
#define BADALIAS(sp1, va1, pr1, sp2, va2, pr2) (TRUE)

/*
 * Given a directly-mapped region, this makes pv_entries out of it and
 * adds them to the free list.
 */
static __inline void pmap_pv_add __P((vaddr_t, vaddr_t));
static __inline void
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
	
	PMAP_PRINTF(PDB_INIT, (": %d pv_entries @ %x allocated\n",
		    (pv - (struct pv_entry *) pv_start), (u_int)pv_start));
}

/*
 * This allocates and returns a new struct pv_entry.
 *
 * If we run out of preallocated struct pv_entries, we have to panic.  
 * malloc() isn't an option, because a) we'll probably end up back 
 * here anyways when malloc() maps what it's trying to return to us, 
 * and b) even if malloc() did succeed, the TLB fault handlers run in 
 * physical mode and thus require that all pv_entries be directly
 * mapped, a quality unlikely for malloc()-returned memory.
 */
static __inline struct pv_entry *pmap_pv_alloc __P((void));
static __inline struct pv_entry *
pmap_pv_alloc(void)
{
	struct pv_entry *pv;

	pv = pv_free_list;
	if (pv == NULL)
		panic("out of pv_entries");
	pv_free_list = pv->pv_next;
	pv->pv_next = NULL;

	PMAP_PRINTF(PDB_PV_ALLOC, ("() = %p\n", pv));
	return pv;
}

/*
 * Given a struct pv_entry allocated by pmap_pv_alloc, this frees it.
 */
static __inline void pmap_pv_free __P((struct pv_entry *));
static __inline void
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
static __inline struct hpt_entry *pmap_hpt_hash __P((pa_space_t, vaddr_t));
static __inline struct hpt_entry *
pmap_hpt_hash(pa_space_t sp, vaddr_t va)
{
	struct hpt_entry *hpt;
	__asm __volatile (
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
 * Given an HPT entry and a VA->PA mapping, this flushes the
 * mapping from the cache and TLB, possibly invalidates the HPT 
 * entry, and optionally frobs protection bits in the mapping.  
 * This is used when a mapping is changing.
 *
 * Invalidating the HPT entry prevents the hardware or software HPT 
 * walker from using stale information for the mapping, if the mapping
 * happens to be the one cached in the HPT entry.
 */
static __inline void _pmap_pv_update __P((struct hpt_entry *, struct pv_entry *, u_int, u_int));
static __inline void
_pmap_pv_update(struct hpt_entry *hpt, struct pv_entry *pv,
		u_int tlbprot_clear, u_int tlbprot_set)
{
	int s;

	/* We're paranoid and turn off all interrupts. */
	s = splhigh();

	/*
	 * If TLB_REF and/or TLB_DIRTY aren't set on the mapping 
	 * already, clear them after the mapping is flushed, in 
	 * case the flushing causes them to be set.
	 */
	tlbprot_clear |= (~pv->pv_tlbprot) & (TLB_REF | TLB_DIRTY);

	/* We have to flush the icache first, since fic may use the DTLB. */
	ficache(pv->pv_space, pv->pv_va, NBPG);
	pitlb(pv->pv_space, pv->pv_va);

	fdcache(pv->pv_space, pv->pv_va, NBPG);
	pdtlb(pv->pv_space, pv->pv_va);

	/* Possibly invalidate the HPT entry. */
	if (hptbtop(pv->pv_va) == hpt->hpt_vpn &&
	    pv->pv_space == hpt->hpt_space) {
		hpt->hpt_space = -1;
		hpt->hpt_valid = 0;
	}

	/* Frob bits in the protection. */
	pv->pv_tlbprot = (pv->pv_tlbprot & ~tlbprot_clear) | tlbprot_set;

	splx(s);
}
#define pmap_pv_update(pv, bc, bs) \
  _pmap_pv_update(pmap_hpt_hash((pv)->pv_space, (pv)->pv_va), pv, bc, bs)

/*
 * Given a PA, returns the table offset for it.
 */
static __inline int pmap_table_find_pa __P((paddr_t));
static __inline int
pmap_table_find_pa(paddr_t pa)
{
	int bank, off;

	if (pa < virtual_start)
		off = atop(pa);
	else if ((bank = vm_physseg_find(atop(pa), &off)) != -1) {
		off += vm_physmem[bank].pmseg.pmap_table_off;
	} else
		return -1;
	return off;
}

/*
 * Given a PA, returns the first mapping for it.
 */
static __inline struct pv_entry *pmap_pv_find_pa __P((paddr_t));
static __inline struct pv_entry *
pmap_pv_find_pa(paddr_t pa)
{
	int table_off;

	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	return pv_head_tbl[table_off];
}

/*
 * Given a VA, this finds any mapping for it.
 */
static __inline struct pv_entry *pmap_pv_find_va __P((pa_space_t, vaddr_t));
static __inline struct pv_entry *
pmap_pv_find_va(pa_space_t space, vaddr_t va)
{
	struct pv_entry *pv = pmap_hpt_hash(space, va)->hpt_entry;

	while(pv && (pv->pv_va != va || pv->pv_space != space))
		pv = pv->pv_hash;

	PMAP_PRINTF(PDB_PV_FIND_VA, ("(0x%x:%p) = %p\n",
					  space, (caddr_t)va, pv));
	return pv;
}

/*
 * Given a pmap, a VA, a PA, and a TLB protection, this enters
 * a new mapping and returns the new struct pv_entry.
 */
static __inline struct pv_entry *pmap_pv_enter __P((pmap_t, pa_space_t, vaddr_t, paddr_t, u_int));
static __inline struct pv_entry *
pmap_pv_enter(pmap_t pmap, pa_space_t space, vaddr_t va,
	      paddr_t pa, u_int tlbprot)
{
	struct hpt_entry *hpt = pmap_hpt_hash(space, va);
	int table_off;
	struct pv_entry **hpv, *pv;
	u_int unmanaged;

	/*
	 * If we're called with a NULL pmap, this mapping is
	 * unmanaged, but it does belong to the kernel.
	 */
	if (pmap == NULL) {
		unmanaged = HPPA_PV_UNMANAGED;
		pmap = pmap_kernel();
	} else
		unmanaged = 0;

	KASSERT(!pmap_initialized ||
		(space == HPPA_SID_KERNEL ?
		 ((pa == 0 && va == 0) ||
		  (va == tmp_vpages[0]) ||
		  (va == tmp_vpages[1]) ||
		  (pa >= virtual_start && va >= virtual_start)) :
		 (pa >= virtual_start)));
#ifdef DIAGNOSTIC
	/* Make sure this VA isn't already entered. */
	for (pv = hpt->hpt_entry; pv != NULL; pv = pv->pv_hash)
		if (pv->pv_va == va && pv->pv_space == space)
			panic("pmap_pv_enter: VA already entered");
#endif /* DIAGNOSTIC */

	/* Get the head of the PA->VA translation list. */
	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	hpv = pv_head_tbl + table_off;
	PMAP_PRINTF(PDB_PV_ENTER, (": hpv %p -> pv %p\n", hpv, *hpv));

	/*
	 * All mappings must be entered in the PA->VA translation
	 * list, because we always have to watch out for cache
	 * aliasing.  However, PAs in I/O space must be mapped
	 * uncacheable, so in that case we can skip the alias check.
	 */
	if ((pa & HPPA_IOSPACE) == HPPA_IOSPACE)
		tlbprot |= TLB_UNCACHEABLE;
	else {
		for (pv = *hpv; pv != NULL; pv = pv->pv_next) {
#ifdef DIAGNOSTIC
			if (pmap == pv->pv_pmap && va == pv->pv_va)
				panic("pmap_pv_enter: VA already in pv_tab");
#endif /* DIAGNOSTIC */

			/*
			 * If the older mapping is not a non-equivalent
			 * alias, just continue.
			 */
			if (!BADALIAS(pv->pv_space, pv->pv_va, pv->pv_tlbprot,
				      space, va, tlbprot))
				continue;

			/*
			 * We have found a non-equivalent alias.
			 * The new mapping must be uncacheable.
			 */
			tlbprot |= TLB_UNCACHEABLE;

			/*
			 * If the older mapping is not uncacheable,
			 * change it to uncacheable.
			 */
			if (!(pv->pv_tlbprot & TLB_UNCACHEABLE)) {
				PMAP_PRINTF(PDB_CACHE, (": new alias to %p\n",
						     pv));
				pmap_pv_update(pv, 0, TLB_UNCACHEABLE);
			} else {
				PMAP_PRINTF(PDB_CACHE, (": old alias to %p\n",
						     pv));
			}
		}
	}

	/* Allocate a new pv_entry, fill it, and link it in. */
	pv = pmap_pv_alloc();
	pv->pv_va = va;
	pv->pv_pmap = pmap;
	pv->pv_space = space;
	pv->pv_tlbprot = tlbprot;
	pv->pv_tlbpage = tlbbtop(pa);
	pv->pv_flags = unmanaged;
	pv->pv_next = *hpv;
	*hpv = pv;
	pv->pv_hash = hpt->hpt_entry;
	hpt->hpt_entry = pv;

	return pv;
}

/*
 * Given a particular VA->PA mapping, this removes it.
 */
static __inline void pmap_pv_remove __P((struct pv_entry *));
static __inline void
pmap_pv_remove(struct pv_entry *pv)
{
	struct hpt_entry *hpt = pmap_hpt_hash(pv->pv_space, pv->pv_va);
	struct pv_entry **_pv = &hpt->hpt_entry;
	paddr_t pa = tlbptob(pv->pv_tlbpage);
	int table_off;
	struct pv_entry **hpv, *ppv, *pv_aliased;
	int alias_count;

	PMAP_PRINTF(PDB_PV_REMOVE, ("(%p)\n", pv));

	/* Get the head of the PA->VA translation list. */
	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	hpv = pv_head_tbl + table_off;
	KASSERT(*hpv != NULL);

	/* Invalidate this entry in the HPT and unlink it. */
	while(*_pv && *_pv != pv)
		_pv = &(*_pv)->pv_hash;
	_pmap_pv_update(hpt, pv, 0, 0);
	KASSERT(*_pv == pv);
	*_pv = pv->pv_hash;
	pv->pv_hash = NULL;

	/* Do the referenced/modified accounting. */
	if (!(pv->pv_flags & HPPA_PV_UNMANAGED))
		pv_flags_tbl[table_off] |=
			((pv->pv_tlbprot & (TLB_REF | TLB_DIRTY)) >> PV_SHIFT);

	/*
	 * Walk the list of entries mapping this PA, looking for
	 * this entry and unlinking it and freeing it when we
	 * find it.
	 *
	 * Also track the number of uncacheable mappings that 
	 * remain, and the last one seen.
	 */
	alias_count = 0;
	pv_aliased = NULL;
	_pv = hpv;
	while ((ppv = *_pv) != NULL) {

		/* If we have found the mapping, unlink it. */
		if (ppv == pv) {
			*_pv = pv->pv_next;
			pmap_pv_free(pv);
			pv = NULL;
			continue;
		}

		/* Check if this is an uncacheable mapping. */
		if (ppv->pv_tlbprot & TLB_UNCACHEABLE) {
			alias_count++;
			pv_aliased = ppv;
		}

		/* Advance. */
		_pv = &ppv->pv_next;
	}
	KASSERT(pv == NULL);

	/*
	 * If there is only a single remaining uncacheable mapping 
	 * of this page remaining, we must have removed what was its 
	 * only non-equivalent alias, so it can now be uncached.
	 *
	 * NB: This is the only way we make uncacheable mappings 
	 * cacheable again.  It gives correct operation, but it 
	 * might be weak.  If the non-equivalent alias relation # 
	 * is ever not transitive (A # B and B # C but A ~# C) 
	 * and B is removed, we will still leave A and C uncacheable.
	 */
	if (alias_count == 1 && (pa & HPPA_IOSPACE) != HPPA_IOSPACE)
		pmap_pv_update(pv_aliased, TLB_UNCACHEABLE, 0);
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *	Called with mapping OFF.
 *
 *	Parameters:
 *	vstart	PA of first available physical page
 *	vend	PA of last available physical page
 */
void
pmap_bootstrap(vstart, vend)
	vaddr_t *vstart;
	vaddr_t *vend;
{
	vaddr_t addr;
	vsize_t size;
	vaddr_t pv_region;
	struct hpt_entry *hptp;
#define BTLB_SET_SIZE 16
	vaddr_t btlb_entry_start[BTLB_SET_SIZE];
	vsize_t btlb_entry_size[BTLB_SET_SIZE];
	int btlb_entry_vm_prot[BTLB_SET_SIZE];
	int btlb_i, btlb_j;
	vsize_t btlb_entry_min, btlb_entry_got;
	extern int kernel_text, etext;
	PMAP_PRINTF(PDB_INIT, ("(%p, %p)\n", vstart, vend));

	uvm_setpagesize();

	pages_per_vm_page = PAGE_SIZE / NBPG;
	/* XXX for now */
	if (pages_per_vm_page != 1)
		panic("HPPA page != VM page");

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
	lock_init(&pmap_lock, FALSE, ETAP_VM_PMAP_SYS, ETAP_VM_PMAP_SYS_I);
#endif	/* NCPUS > 1 */
	simple_lock_init(&kernel_pmap->pmap_lock);
	simple_lock_init(&pmap_freelock);
	simple_lock_init(&sid_pid_lock);

	kernel_pmap->pmap_refcnt = 1;
	kernel_pmap->pmap_space = HPPA_SID_KERNEL;
	kernel_pmap->pmap_pid = HPPA_PID_KERNEL;

	/*
	 * Allocate various tables and structures.
	 */
	addr = hppa_round_page(*vstart);
	virtual_end = *vend;
	pv_region = addr;

	/*
	 * Figure out how big the HPT must be, and align
	 * addr to what will be its beginning.  We don't
	 * waste the pages skipped for the alignment; 
	 * they become struct pv_entry pages.
	 */ 
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
#ifdef PMAPDEBUG
	if (pmapdebug & PDB_INIT)
		printf("hpt_table: 0x%lx @ %p\n", size + 1, (caddr_t)addr);
#endif
	/* load cr25 with the address of the HPT table
	   NB: It sez CR_VTOP, but we (and the TLB handlers) know better ... */
	mtctl(addr, CR_VTOP);
	hpt_base = addr;
	hpt_mask = size;
	proc0.p_md.md_regs->tf_hptm = size;
	proc0.p_md.md_regs->tf_vtop = addr;
	addr += size + 1;

	/*
	 * we know that btlb_insert() will round it up to the next
	 * power of two at least anyway
	 */
	for (physmem = 1; physmem < btoc(addr); physmem *= 2);

	/* map the kernel space, which will give us virtual_start */
	*vstart = hppa_round_page(addr + (totalphysmem - physmem) *
				  (sizeof(struct pv_entry) * maxproc / 8 +
				   sizeof(struct vm_page)));
	/* XXX PCXS needs two separate inserts in separate btlbs */

	/*
	 * We want to offer kernel NULL pointer dereference 
	 * detection, and write protection for the kernel text.  
	 * To keep things simple, we use BTLB entries to directly 
	 * map the kernel text, data, bss, and anything else we've
	 * allocated (directly-mapped) space for already.  The
	 * region from 0 to the start of the kernel text is unmapped.
	 *
	 * Note that a BTLB entry must be some power-of-two pages
	 * in size, and must be aligned on that size.  This is
	 * why we insist that the kernel text start no earlier than
	 * 0x80000 (the minimum BTLB entry eize), why we need 
	 * multiple entries to do the job, and why 100% of the 
	 * kernel text isn't necessarily protected (one BTLB
	 * entry may need to cover both text and data).
	 */

	/* XXX fredette - we should get this from machdep.c. */
	btlb_entry_min = (vsize_t) &kernel_text;

	/*
	 * The address of the start of the kernel text must
	 * be some multiple of the minimum BTLB entry size.
	 */
	btlb_entry_start[0] = (vaddr_t) &kernel_text;
	if (btlb_entry_start[0] & (btlb_entry_min - 1))
		panic("kernel text start incompatible with BTLB minimum");
	btlb_entry_size[0] = btlb_entry_min;
	btlb_entry_vm_prot[0] = VM_PROT_READ | VM_PROT_EXECUTE;
	if (btlb_entry_start[0] + btlb_entry_size[0] > (vaddr_t) &etext)
		btlb_entry_vm_prot[0] |= VM_PROT_WRITE;
	btlb_j = 1;

	/*
	 * All BTLB entries allow reading.  Any BTLB entry
	 * that maps kernel text also allows execution.  Any 
	 * BTLB entry that maps data+bss also allows writing.
	 */
	do {

		/* Set up the next BTLB entry. */
		KASSERT(btlb_j < BTLB_SET_SIZE);
		btlb_entry_start[btlb_j] = 
			btlb_entry_start[btlb_j - 1] + 
			btlb_entry_size[btlb_j - 1];
		btlb_entry_size[btlb_j] = btlb_entry_min;
		btlb_entry_vm_prot[btlb_j] = VM_PROT_READ;
		if (btlb_entry_start[btlb_j] < (vaddr_t) &etext)
			btlb_entry_vm_prot[btlb_j] |= VM_PROT_EXECUTE;
		if ((btlb_entry_start[btlb_j] + btlb_entry_size[btlb_j]) > 
			(vaddr_t) &etext)
			btlb_entry_vm_prot[btlb_j] |= VM_PROT_WRITE;

		/* As we can, aggregate BTLB entries. */
		while (btlb_j > 0 &&
			btlb_entry_vm_prot[btlb_j] == 
			btlb_entry_vm_prot[btlb_j - 1] &&
			btlb_entry_size[btlb_j] ==
			btlb_entry_size[btlb_j - 1] &&
			!(btlb_entry_start[btlb_j - 1] &
			  ((btlb_entry_size[btlb_j - 1] << 1) - 1)))
			btlb_entry_size[--btlb_j] <<= 1;

		/* Move on. */
		btlb_j++;
	} while ((btlb_entry_start[btlb_j - 1] + btlb_entry_size[btlb_j - 1]) < 			*vstart);
		
	/* Now insert all of the BTLB entries. */
	for (btlb_i = 0; btlb_i < btlb_j; btlb_i++) {
		btlb_entry_got = btlb_entry_size[btlb_i];
		if (btlb_insert(kernel_pmap->pmap_space, 
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

	*vstart = btlb_entry_start[btlb_j - 1] + btlb_entry_size[btlb_j - 1];
	virtual_start = *vstart;

	/*
	 * NOTE: we no longer trash the BTLB w/ unused entries,
	 * lazy map only needed pieces (see bus_mem_add_mapping() for refs).
	 */

	/*
	 * Allocate the struct pv_entry heads table and the flags table
	 * for the physical pages.
	 */
	size = hppa_round_page(totalphysmem *
			(sizeof(*pv_head_tbl) +
			 sizeof(struct pv_entry) +
			 sizeof(*pv_flags_tbl)));
	bzero ((caddr_t)addr, size);
#ifdef PMAPDEBUG
	if (pmapdebug & PDB_INIT)
		printf("pv_array: 0x%x @ %p\n", (u_int)size, (caddr_t)addr);
#endif

	virtual_steal = addr + size;
	virtual_steal = round_page(virtual_steal);
	uvm_page_physload(atop(virtual_steal), totalphysmem,
		atop(virtual_steal), totalphysmem, VM_FREELIST_DEFAULT);
	/* we have only one initial phys memory segment */
	pv_head_tbl = (struct pv_entry **)addr;
	pv_region = (vaddr_t)(pv_head_tbl + totalphysmem);
	pv_flags_tbl = (u_char *)(pv_region + 
		totalphysmem * sizeof(struct pv_entry));
	pmap_pv_add(pv_region, (vaddr_t) pv_flags_tbl);
	vm_physmem[0].pmseg.pmap_table_off = atop(virtual_start);

	/* here will be a hole due to the kernel memory alignment
	   and we use it for pmap_steal_memory */

	/*
	 * The tmp_vpages are two virtual pages that can be
	 * mapped for the copying and zeroing operations.
	 * We use two pages immediately before the kernel text.
	 */
	tmp_vpages[1] = hppa_trunc_page((vaddr_t) &kernel_text) - PAGE_SIZE;
	tmp_vpages[0] = tmp_vpages[1] - PAGE_SIZE;
}

/*
 * pmap_steal_memory(size, startp, endp)
 *	steals memory block of size `size' from directly mapped
 *	segment (mapped behind the scenes).
 *	directly mapped cannot be grown dynamically once allocated.
 */
vaddr_t
pmap_steal_memory(size, startp, endp)
	vsize_t size;
	vaddr_t *startp;
	vaddr_t *endp;
{
	vaddr_t va;

	PMAP_PRINTF(PDB_STEAL, ("(%lx, %p, %p)\n", size, startp, endp));

	/* Remind the caller of the start and end of virtual space. */
	if (startp)
		*startp = virtual_start;
	if (endp)
		*endp = virtual_end;

	/* Round the allocation up to a page. */
	size = hppa_round_page(size);

	/* We must panic if we cannot steal the memory. */
	if (size > virtual_start - virtual_steal)
		panic("pmap_steal_memory: out of memory");

	/* Steal the memory. */
	va = virtual_steal;
	virtual_steal += size;
	PMAP_PRINTF(PDB_STEAL, (": steal %ld bytes (%x+%x,%x)\n",
		    size, (u_int)va, (u_int)size, (u_int)virtual_start));

	/*
	 * We are an unusual pmap in that we really, really
	 * want to steal the rest of the directly-mapped
	 * segment for struct pv_entries, after UVM has done
	 * all of its stealing.  The tricky part is detecting
	 * when UVM is doing the last of its stealing.
	 * For now, we key off of uvmexp.ncolors being set
	 * to 1, which it is by uvm_page_init before it
	 * does the final stealing.
	 */
	if (uvmexp.ncolors == 1) {
		virtual_stolen = virtual_steal;
		virtual_steal = virtual_start;
		vm_physmem[0].avail_start = atop(virtual_start);
		vm_physmem[0].start = vm_physmem[0].avail_start;
	}

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
pmap_init()
{
	extern void gateway_page __P((void));
#ifdef notyet
	vaddr_t va;
#endif

	/* allocate the rest of the steal area for pv_entries */
	pmap_pv_add(virtual_stolen, virtual_start);

#ifdef notyet
	/*
	 * Enter direct mappings for many of the physical
	 * pages below the tmp_vpages, so we can put them
	 * to good use as pv_entry pages and not waste them.
	 */
	/*
	 * XXX this is a poorly named constant.  It should
	 * probably get a better name or maybe its value
	 * should be VM_MIN_KERNEL_ADDRESS in vmparam.h.
	 * Note that it is the old kernel text start 
	 * address.  Pages below this are assumed to belong
	 * to the firmware.
	 */
#define LOW_MEM_PAGES 0x12000
	for (va = LOW_MEM_PAGES; 
	     (va + NBPG) <= tmp_vpages[0];
	     va += NBPG)
		pmap_kenter_pa(va, va, VM_PROT_ALL);
	pmap_pv_add(LOW_MEM_PAGES, va);
#endif /* notyet */

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
		      (paddr_t)&gateway_page, TLB_GATE_PROT);

	pmap_initialized = TRUE;
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
static void pmap_pinit __P((pmap_t));
static void
pmap_pinit(pmap)
	pmap_t pmap;
{
	register u_int pid;
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
pmap_create()
{
	register pmap_t pmap;
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
		MALLOC(pmap, struct pmap *, sizeof(*pmap), M_VMMAP, M_NOWAIT);
		if (pmap == NULL)
			return NULL;
		bzero(pmap, sizeof(*pmap));
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
pmap_destroy(pmap)
	pmap_t pmap;
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
 * pmap_activate(proc)
 *	Activates the vmspace for the given process.  This
 *	isn't necessarily the current process.
 */
void
pmap_activate(struct proc *p)
{
	pmap_t pmap = p->p_vmspace->vm_map.pmap;
	pa_space_t space = pmap->pmap_space;
	struct trapframe *tf = p->p_md.md_regs;

	/* space is cached for the copy{in,out}'s pleasure */
	p->p_addr->u_pcb.pcb_space = space;

	/* Load all of the user's space registers. */
	tf->tf_sr0 = tf->tf_sr1 = tf->tf_sr2 = tf->tf_sr3 =
	tf->tf_sr4 = tf->tf_sr5 = tf->tf_sr6 = space;
	tf->tf_iisq_head = tf->tf_iisq_tail = space;

	/*
	 * Load the protection registers.  NB that
	 * if p *is* the current process, we set pidr2
	 * to the new space immediately, so any copyins
	 * or copyouts that happen before we return to
	 * userspace work.
	 */
	tf->tf_pidr1 = tf->tf_pidr2 = pmap->pmap_pid;
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
pmap_enter(pmap, va, pa, prot, flags)
	pmap_t pmap;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	int flags;
{
	register struct pv_entry *pv;
	u_int tlbpage, tlbprot;
	pa_space_t space;
	boolean_t waswired;
	boolean_t wired = (flags & PMAP_WIRED) != 0;
	int s;

	/* Get a handle on the mapping we want to enter. */
	space = pmap_sid(pmap, va);
	va = hppa_trunc_page(va);
	pa = hppa_trunc_page(pa);
	tlbpage = tlbbtop(pa);
	tlbprot = pmap_prot(pmap, prot) | pmap->pmap_pid;
	if (wired)
		tlbprot |= TLB_WIRED;
	if (flags & VM_PROT_WRITE)
		tlbprot |= TLB_DIRTY;

#ifdef PMAPDEBUG
	if (!pmap_initialized || (pmapdebug & PDB_ENTER))
		PMAP_PRINTF(0, ("(%p, %p, %p, %x, %swired)\n", 
				pmap, (caddr_t)va, (caddr_t)pa,
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
		waswired = FALSE;
	} else {
		KASSERT((pv->pv_flags & HPPA_PV_UNMANAGED) == 0);
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
pmap_remove(pmap, sva, eva)
	register pmap_t pmap;
	register vaddr_t sva;
	register vaddr_t eva;
{
	register struct pv_entry *pv;
	register pa_space_t space;
	int s;

	PMAP_PRINTF(PDB_REMOVE, ("(%p, %p, %p)\n", 
				 pmap, (caddr_t)sva, (caddr_t)eva));

	sva = hppa_trunc_page(sva);
	space = pmap_sid(pmap, sva);

	s = splvm();

	while (sva < eva) {
		pv = pmap_pv_find_va(space, sva);
		if (pv) {
			KASSERT((pv->pv_flags & HPPA_PV_UNMANAGED) == 0);
			KASSERT(pmap->pmap_stats.resident_count > 0);
			pmap->pmap_stats.resident_count--;
			if (pv->pv_tlbprot & TLB_WIRED) {
				KASSERT(pmap->pmap_stats.wired_count > 0);
				pmap->pmap_stats.wired_count--;
			}
			pmap_pv_remove(pv);
			PMAP_PRINTF(PDB_REMOVE, (": removed %p for 0x%x:%p\n",
						 pv, space, (caddr_t)sva));
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
pmap_page_protect(pg, prot)
	struct vm_page *pg;
	vm_prot_t prot;
{
	register struct pv_entry *pv, *pv_next;
	register pmap_t pmap;
	register u_int tlbprot;
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int s;

	PMAP_PRINTF(PDB_PROTECT, ("(%p, %x)\n", (caddr_t)pa, prot));

	switch (prot) {
	case VM_PROT_ALL:
		return;
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		s = splvm();
		for (pv = pmap_pv_find_pa(pa); pv; pv = pv->pv_next) {
			/* Ignore unmanaged mappings. */
			if (pv->pv_flags & HPPA_PV_UNMANAGED)
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
			if (pv->pv_flags & HPPA_PV_UNMANAGED)
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
pmap_protect(pmap, sva, eva, prot)
	pmap_t pmap;
	vaddr_t sva;
	vaddr_t eva;
	vm_prot_t prot;
{
	register struct pv_entry *pv;
	u_int tlbprot;
	pa_space_t space;
	int s;

	PMAP_PRINTF(PDB_PROTECT, ("(%p, %p, %p, %x)\n", 
				 pmap, (caddr_t)sva, (caddr_t)eva, prot));

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return;

	sva = hppa_trunc_page(sva);
	space = pmap_sid(pmap, sva);
	tlbprot = pmap_prot(pmap, prot);

	s = splvm();
	for(; sva < eva; sva += PAGE_SIZE) {
		if((pv = pmap_pv_find_va(space, sva))) {
			KASSERT((pv->pv_flags & HPPA_PV_UNMANAGED) == 0);
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
pmap_unwire(pmap, va)
	pmap_t	pmap;
	vaddr_t	va;
{
	struct pv_entry *pv;
	int s;

	va = hppa_trunc_page(va);
	PMAP_PRINTF(PDB_WIRING, ("(%p, %p)\n", pmap, (caddr_t)va));

	simple_lock(&pmap->pmap_lock);

	s = splvm();
	if ((pv = pmap_pv_find_va(pmap_sid(pmap, va), va)) == NULL)
		panic("pmap_unwire: can't find mapping entry");

	KASSERT((pv->pv_flags & HPPA_PV_UNMANAGED) == 0);
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
 *	storage pointed to by pap and returns TRUE if the
 *	virtual address is mapped. returns FALSE in not mapped.
 */
boolean_t
pmap_extract(pmap, va, pap)
	pmap_t pmap;
	vaddr_t va;
	paddr_t *pap;
{
	struct pv_entry *pv;
	vaddr_t off;
	int s;

	off = va;
	off -= (va = hppa_trunc_page(va));

	s = splvm();
	if ((pv = pmap_pv_find_va(pmap_sid(pmap, va), va))) {
		if (pap != NULL)
			*pap = tlbptob(pv->pv_tlbpage) + off;
		PMAP_PRINTF(PDB_EXTRACT, ("(%p, %p) = %p\n",
				pmap, (caddr_t)va, 
				(caddr_t)(tlbptob(pv->pv_tlbpage) + off)));
	} else {
		PMAP_PRINTF(PDB_EXTRACT, ("(%p, %p) unmapped\n",
					 pmap, (caddr_t)va));
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
pmap_zero_page(pa)
	register paddr_t pa;
{
	struct pv_entry *pv;
	int s;

	PMAP_PRINTF(PDB_ZERO, ("(%p)\n", (caddr_t)pa));

	KASSERT(!pmap_initialized || pa >= virtual_start);

	s = splvm(); /* XXX are we already that high? */

	/* Map the physical page. */
	pv = pmap_pv_enter(NULL, HPPA_SID_KERNEL, tmp_vpages[1], pa, 
			TLB_AR_KRW | TLB_DIRTY);

	/* Zero it. */
	memset((caddr_t)tmp_vpages[1], 0, PAGE_SIZE);

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
pmap_copy_page(spa, dpa)
	paddr_t spa;
	paddr_t dpa;
{
	struct pv_entry *spv, *dpv;
	int s;

	PMAP_PRINTF(PDB_COPY, ("(%p, %p)\n", (caddr_t)spa, (caddr_t)dpa));

	KASSERT(!pmap_initialized || (spa >= virtual_start && dpa >= virtual_start));

	s = splvm(); /* XXX are we already that high? */

	/* Map the two pages. */
	spv = pmap_pv_enter(NULL, HPPA_SID_KERNEL, tmp_vpages[0], spa, 
			TLB_AR_KR);
	dpv = pmap_pv_enter(NULL, HPPA_SID_KERNEL, tmp_vpages[1], dpa, 
			TLB_AR_KRW | TLB_DIRTY);

	/* Do the copy. */
	memcpy((caddr_t)tmp_vpages[1], (const caddr_t)tmp_vpages[0], PAGE_SIZE);

	/* Unmap the pages. */
	pmap_pv_remove(spv);
	pmap_pv_remove(dpv);

	splx(s);
}

/*
 * Given a PA and a bit, this tests and clears that bit in 
 * the modref information for the PA.
 */
static __inline boolean_t pmap_clear_bit __P((paddr_t, u_int));
static __inline boolean_t
pmap_clear_bit(paddr_t pa, u_int bit)
{
	int table_off;
	u_char flags;
	struct pv_entry *pv;
	boolean_t ret;
	int s;

	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	s = splvm();
	flags = pv_flags_tbl[table_off];
	ret = (flags & (bit >> PV_SHIFT)) != 0;
	if (ret)
		pv_flags_tbl[table_off] = flags & ~(bit >> PV_SHIFT);
	for (pv = pv_head_tbl[table_off];
	     pv != NULL;
	     pv = pv->pv_next) {
		if (!(pv->pv_flags & HPPA_PV_UNMANAGED) &&
		    (pv->pv_tlbprot & bit)) {
			pmap_pv_update(pv, bit, 0);
			ret = TRUE;
		}
	}
	splx(s);
	return ret;
}

/*
 * Given a PA and a bit, this tests that bit in the modref
 * information for the PA.
 */
static __inline boolean_t pmap_test_bit __P((paddr_t, u_int));
static __inline boolean_t
pmap_test_bit(paddr_t pa, u_int bit)
{
	int table_off;
	struct pv_entry *pv;
	boolean_t ret;
	int s;

	table_off = pmap_table_find_pa(pa);
	KASSERT(table_off >= 0);
	s = splvm();
	ret = (pv_flags_tbl[table_off] & (bit >> PV_SHIFT)) != 0;
	if (!ret) {
		for (pv = pv_head_tbl[table_off]; 
		     pv != NULL; 
		     pv = pv->pv_next) {
			if (!(pv->pv_flags & HPPA_PV_UNMANAGED) &&
			    (pv->pv_tlbprot & bit)) {
				pv_flags_tbl[table_off] |= (bit >> PV_SHIFT);
				ret = TRUE;
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
 *	machine independant page starting at the given
 *	physical address.  phys must be aligned on a machine
 *	independant page boundary.
 */
boolean_t
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	register paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t ret = pmap_clear_bit(pa, TLB_DIRTY);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (caddr_t)pa, ret));
	return ret;
}

/*
 * pmap_is_modified(pa)
 *	returns TRUE if the given physical page has been modified
 *	since the last call to pmap_clear_modify().
 */
boolean_t
pmap_is_modified(pg)
	struct vm_page *pg;
{
	register paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t ret = pmap_test_bit(pa, TLB_DIRTY);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (caddr_t)pa, ret));
	return ret;
}

/*
 * pmap_clear_reference(pa)
 *	clears the hardware referenced bit in the given machine
 *	independant physical page.
 *
 *	Currently, we treat a TLB miss as a reference; i.e. to clear
 *	the reference bit we flush all mappings for pa from the TLBs.
 */
boolean_t
pmap_clear_reference(pg)
	struct vm_page *pg;
{
	register paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t ret = pmap_clear_bit(pa, TLB_REF);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (caddr_t)pa, ret));
	return ret;
}

/*
 * pmap_is_referenced(pa)
 *	returns TRUE if the given physical page has been referenced
 *	since the last call to pmap_clear_reference().
 */
boolean_t
pmap_is_referenced(pg)
	struct vm_page *pg;
{
	register paddr_t pa = VM_PAGE_TO_PHYS(pg);
	boolean_t ret = pmap_test_bit(pa, TLB_REF);
	PMAP_PRINTF(PDB_BITS, ("(%p) = %d\n", (caddr_t)pa, ret));
	return ret;
}

void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
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
				 (caddr_t)va, (caddr_t)pa, prot));
	va = hppa_trunc_page(va);
	s = splvm();
	KASSERT(pmap_pv_find_va(HPPA_SID_KERNEL, va) == NULL);
	pmap_pv_enter(NULL, HPPA_SID_KERNEL, va, pa, 
		      pmap_prot(pmap_kernel(), prot) |
		      TLB_WIRED |
		      ((prot & VM_PROT_WRITE) ? TLB_DIRTY : 0));
	splx(s);
#ifdef PMAPDEBUG
	pmapdebug = opmapdebug;
#endif /* PMAPDEBUG */
}

void
pmap_kremove(va, size)
	vaddr_t va;
	vsize_t size;
{
	register struct pv_entry *pv;
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
				 (caddr_t)va, (u_int)size));

	size += va;
	va = hppa_trunc_page(va);
	size -= va;
	s = splvm();
	for (size = hppa_round_page(size); size;
	    size -= PAGE_SIZE, va += PAGE_SIZE) {
		pv = pmap_pv_find_va(HPPA_SID_KERNEL, va);
		if (pv) {
			KASSERT((pv->pv_flags & HPPA_PV_UNMANAGED) != 0);
			pmap_pv_remove(pv);
		} else {
			PMAP_PRINTF(PDB_REMOVE, (": no pv for %p\n",
						 (caddr_t)va));
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
	
	sva = hppa_trunc_page(sva);
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
pmap_hptdump()
{
	register struct hpt_entry *hpt, *ehpt;
	register struct pv_entry *pv;

	mfctl(CR_HPTMASK, ehpt);
	mfctl(CR_VTOP, hpt);
	ehpt = (struct hpt_entry *)((int)hpt + (int)ehpt + 1);
	db_printf("HPT dump %p-%p:\n", hpt, ehpt);
	for (; hpt < ehpt; hpt++)
		if (hpt->hpt_valid || hpt->hpt_entry) {
			db_printf("hpt@%p: %x{%sv=%x:%x},%b,%x\n",
			    hpt, *(int *)hpt, (hpt->hpt_valid?"ok,":""),
			    hpt->hpt_space, hpt->hpt_vpn << 9,
			    hpt->hpt_tlbprot, TLB_BITS,
			    tlbptob(hpt->hpt_tlbpage));
			for (pv = hpt->hpt_entry; pv; pv = pv->pv_hash)
				db_printf("    pv={%p,%x:%x,%b,%x}->%p\n",
				    pv->pv_pmap, pv->pv_space, pv->pv_va,
				    pv->pv_tlbprot, TLB_BITS,
				    tlbptob(pv->pv_tlbpage), pv->pv_hash);
		}
}
#endif
