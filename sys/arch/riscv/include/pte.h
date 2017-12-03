/* $NetBSD: pte.h,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $ */
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _RISCV_PTE_H_
#define _RISCV_PTE_H_

//
// RV32 page table entry (4 GB VA space)
//   [31..22] = PPN[1]
//   [21..12] = PPN[0]
//   [11.. 9] = software
//
// RV64 page table entry (4 TB VA space)
//   [64..43] = 0
//   [42..33] = PPN[2]
//   [32..23] = PPN[1]
//   [22..13] = PPN[0]
//   [12.. 9] = software
//
// Common to both:
//   [8] = SX
//   [7] = SW
//   [6] = SR
//   [5] = UX
//   [4] = UW
//   [3] = UR
//   [2] = G
//   [1] = T
//   [0] = V
//

#define NPTEPG		(1 + __BITS(9, 0))	// PTEs per Page
#define NSEGPG		NPTEPG
#define NPDEPG		NPTEPG
#ifdef _LP64
#define PTE_PPN		__BITS(63, 13)	// Physical Page Number
#define	PTE_PPN0	__BITS(42, 33)	// 1K 8-byte SDEs / PAGE
#define	PTE_PPN1	__BITS(32, 23)	// 1K 8-byte PDEs / PAGE
#define	PTE_PPN2	__BITS(22, 13)	// 1K 8-byte PTEs / PAGE
typedef __uint64_t pt_entry_t;
typedef __uint64_t pd_entry_t;
#define atomic_cas_pte	atomic_cas_64
#define atomic_cas_pde	atomic_cas_64
#else
#define PTE_PPN		__BITS(31, 12)	// Physical Page Number
#define	PTE_PPN0	__BITS(31, 22)	// 1K 4-byte PDEs / PAGE
#define	PTE_PPN1	__BITS(21, 12)	// 1K 4-byte PTEs / PAGE
typedef __uint32_t pt_entry_t;
typedef __uint32_t pd_entry_t;
#define atomic_cas_pte	atomic_cas_32
#define atomic_cas_pde	atomic_cas_32
#endif

// These only mean something to NetBSD
#define	PTE_NX		__BIT(11)	// Unexecuted
#define	PTE_NW		__BIT(10)	// Unmodified
#define	PTE_WIRED	__BIT(9)	// Do Not Delete

// These are hardware defined bits
#define	PTE_SX		__BIT(8)	// Supervisor eXecute
#define	PTE_SW		__BIT(7)	// Supervisor Write 
#define	PTE_SR		__BIT(6)	// Supervisor Read
#define	PTE_UX		__BIT(5)	// User eXecute
#define	PTE_UW		__BIT(4)	// User Write
#define	PTE_UR		__BIT(3)	// User Read
#define	PTE_G		__BIT(2)	// Global
#define	PTE_T		__BIT(1)	// "Transit" (non-leaf)
#define	PTE_V		__BIT(0)	// Valid

static inline bool
pte_valid_p(pt_entry_t pte)
{
	return (pte & PTE_V) != 0;
}

static inline bool
pte_wired_p(pt_entry_t pte)
{
	return (pte & PTE_WIRED) != 0;
}

static inline bool
pte_modified_p(pt_entry_t pte)
{
	return (pte & PTE_NW) == 0 && (pte & (PTE_UW|PTE_SW)) != 0;
}

static inline bool
pte_cached_p(pt_entry_t pte)
{
	return true;
}

static inline bool
pte_deferred_exec_p(pt_entry_t pte)
{
	return (pte & PTE_NX) != 0;
}

static inline pt_entry_t
pte_wire_entry(pt_entry_t pte)
{
	return pte | PTE_WIRED;
}
        
static inline pt_entry_t   
pte_unwire_entry(pt_entry_t pte)
{
	return pte & ~PTE_WIRED;
}

static inline paddr_t
pte_to_paddr(pt_entry_t pte)
{
	return pte & ~PAGE_MASK;
}

static inline pt_entry_t
pte_nv_entry(bool kernel_p)
{
	return kernel_p ? PTE_G : 0;
}

static inline pt_entry_t
pte_prot_nowrite(pt_entry_t pte)
{
	return pte & ~(PTE_NW|PTE_SW|PTE_UW);
}

static inline pt_entry_t
pte_prot_downgrade(pt_entry_t pte, vm_prot_t newprot)
{
	pte &= ~(PTE_NW|PTE_SW|PTE_UW);
	if ((newprot & VM_PROT_EXECUTE) == 0)
		pte &= ~(PTE_NX|PTE_SX|PTE_UX);
	return pte;
}

static inline pt_entry_t
pte_prot_bits(struct vm_page_md *mdpg, vm_prot_t prot, bool kernel_p)
{
	KASSERT(prot & VM_PROT_READ);
	pt_entry_t pt_entry = PTE_SR | (kernel_p ? 0 : PTE_UR);
	if (prot & VM_PROT_EXECUTE) {
		if (mdpg != NULL && !VM_PAGEMD_EXECPAGE_P(mdpg))
			pt_entry |= PTE_NX;
		else
			pt_entry |= kernel_p ? PTE_SX : PTE_UX;
	}
	if (prot & VM_PROT_WRITE) {
		if (mdpg != NULL && !VM_PAGEMD_MODIFIED_P(mdpg))
			pt_entry |= PTE_NW;
		else
			pt_entry |= PTE_SW | (kernel_p ? 0 : PTE_UW);
	}
	return pt_entry;
}

static inline pt_entry_t
pte_flag_bits(struct vm_page_md *mdpg, int flags, bool kernel_p)
{
#if 0
	if (__predict_false(flags & PMAP_NOCACHE)) {
		if (__predict_true(mdpg != NULL)) {
			return pte_nocached_bits();
		} else {
			return pte_ionocached_bits();
		}
	} else {
		if (__predict_false(mdpg != NULL)) {
			return pte_cached_bits();
		} else {
			return pte_iocached_bits();
		}
	}
#else
	return 0;
#endif
}

static inline pt_entry_t
pte_make_enter(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
	int flags, bool kernel_p)
{
	pt_entry_t pte = (pt_entry_t) pa & ~PAGE_MASK;

	pte |= pte_flag_bits(mdpg, flags, kernel_p);
	pte |= pte_prot_bits(mdpg, prot, kernel_p);

	if (mdpg == NULL && VM_PAGEMD_REFERENCED_P(mdpg))
		pte |= PTE_V;

	return pte;
}

static inline pt_entry_t
pte_make_kenter_pa(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
	int flags)
{
	pt_entry_t pte = (pt_entry_t) pa & ~PAGE_MASK;

	pte |= PTE_WIRED | PTE_V;
	pte |= pte_flag_bits(NULL, flags, true);
	pte |= pte_prot_bits(NULL, prot, true); /* pretend unmanaged */

	return pte;
}

static inline void
pte_set(pt_entry_t *ptep, pt_entry_t pte)
{
	*ptep = pte;
}

static inline pd_entry_t
pte_invalid_pde(void)
{
	return 0;
}

static inline pd_entry_t
pte_pde_pdetab(paddr_t pa)
{
	return PTE_V | PTE_G | PTE_T | pa;
}

static inline pd_entry_t
pte_pde_ptpage(paddr_t pa)
{
	return PTE_V | PTE_G | PTE_T | pa;
}

static inline bool
pte_pde_valid_p(pd_entry_t pde)
{
	return (pde & (PTE_V|PTE_T)) == (PTE_V|PTE_T);
}

static inline paddr_t
pte_pde_to_paddr(pd_entry_t pde)
{
	return pde & ~PAGE_MASK;
}

static inline pd_entry_t
pte_pde_cas(pd_entry_t *pdep, pd_entry_t opde, pt_entry_t npde)
{
#ifdef MULTIPROCESSOR
#ifdef _LP64
	return atomic_cas_64(pdep, opde, npde);
#else
	return atomic_cas_32(pdep, opde, npde);
#endif
#else
	*pdep = npde;
	return 0;
#endif
}
#endif /* _RISCV_PTE_H_ */
