/* $NetBSD: pte.h,v 1.13 2023/05/07 12:41:48 skrll Exp $ */

/*
 * Copyright (c) 2014, 2019, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas (of 3am Software Foundry), Maxime Villard, and
 * Nick Hudson.
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
#define	_RISCV_PTE_H_

#ifdef _LP64	/* Sv39 */
#define	PTE_PPN		__BITS(53, 10)
#define	PTE_PPN0	__BITS(18, 10)
#define	PTE_PPN1	__BITS(27, 19)
#define	PTE_PPN2	__BITS(53, 28)
typedef uint64_t pt_entry_t;
typedef uint64_t pd_entry_t;
#define	atomic_cas_pte	atomic_cas_64
#else		/* Sv32 */
#define	PTE_PPN		__BITS(31, 10)
#define	PTE_PPN0	__BITS(19, 10)
#define	PTE_PPN1	__BITS(31, 20)
typedef uint32_t pt_entry_t;
typedef uint32_t pd_entry_t;
#define	atomic_cas_pte	atomic_cas_32
#endif

#define	PTE_PPN_SHIFT	10

#define	NPTEPG		(PAGE_SIZE / sizeof(pt_entry_t))
#define	NSEGPG		NPTEPG
#define	NPDEPG		NPTEPG


/* HardWare PTE bits SV39 */
#define PTE_N		__BIT(63)	// Svnapot
#define PTE_PBMT	__BITS(62, 61)	// Svpbmt
#define PTE_reserved0	__BITS(60, 54)	//

/* Software PTE bits. */
#define	PTE_RSW		__BITS(9, 8)
#define	PTE_WIRED	__BIT(9)

/* Hardware PTE bits. */
// These are hardware defined bits
#define	PTE_D		__BIT(7)	// Dirty
#define	PTE_A		__BIT(6)	// Accessed
#define	PTE_G		__BIT(5)	// Global
#define	PTE_U		__BIT(4)	// User
#define	PTE_X		__BIT(3)	// eXecute
#define	PTE_W		__BIT(2)	// Write
#define	PTE_R		__BIT(1)	// Read
#define	PTE_V		__BIT(0)	// Valid

#define	PTE_HARDWIRED	(PTE_A | PTE_D)
#define	PTE_USER	(PTE_V | PTE_U)
#define	PTE_KERN	(PTE_V | PTE_G)
#define	PTE_RW		(PTE_R | PTE_W)
#define	PTE_RX		(PTE_R | PTE_X)
#define	PTE_RWX		(PTE_R | PTE_W | PTE_X)

#define	PTE_ISLEAF_P(pte) (((pte) & PTE_RWX) != 0)

#define	PA_TO_PTE(pa)	(((pa) >> PAGE_SHIFT) << PTE_PPN_SHIFT)
#define	PTE_TO_PA(pte)	(((pte) >> PTE_PPN_SHIFT) << PAGE_SHIFT)



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
	return (pte & PTE_D) != 0;
}

static inline bool
pte_cached_p(pt_entry_t pte)
{
	/* TODO: This seems wrong... */
	return true;
}

static inline bool
pte_deferred_exec_p(pt_entry_t pte)
{
	return false;
}

static inline pt_entry_t
pte_wire_entry(pt_entry_t pte)
{
	return pte | PTE_HARDWIRED | PTE_WIRED;
}

static inline pt_entry_t
pte_unwire_entry(pt_entry_t pte)
{
	return pte & ~(PTE_HARDWIRED | PTE_WIRED);
}

static inline paddr_t
pte_to_paddr(pt_entry_t pte)
{
	return PTE_TO_PA(pte);
}

static inline pt_entry_t
pte_nv_entry(bool kernel_p)
{
	return 0;
}

static inline pt_entry_t
pte_prot_nowrite(pt_entry_t pte)
{
	return pte & ~PTE_W;
}

static inline pt_entry_t
pte_prot_downgrade(pt_entry_t pte, vm_prot_t newprot)
{
	if ((newprot & VM_PROT_READ) == 0)
		pte &= ~PTE_R;
	if ((newprot & VM_PROT_WRITE) == 0)
		pte &= ~PTE_W;
	if ((newprot & VM_PROT_EXECUTE) == 0)
		pte &= ~PTE_X;
	return pte;
}

static inline pt_entry_t
pte_prot_bits(struct vm_page_md *mdpg, vm_prot_t prot, bool kernel_p)
{
	KASSERT(prot & VM_PROT_READ);
	pt_entry_t pte = PTE_R;

	if (prot & VM_PROT_EXECUTE) {
		pte |= PTE_X;
	}
	if (prot & VM_PROT_WRITE) {
		pte |= PTE_W;
	}

	return pte;
}

static inline pt_entry_t
pte_flag_bits(struct vm_page_md *mdpg, int flags, bool kernel_p)
{
	return 0;
}

static inline pt_entry_t
pte_make_enter(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
    int flags, bool kernel_p)
{
	pt_entry_t pte = (pt_entry_t)PA_TO_PTE(pa);

	pte |= kernel_p ? PTE_KERN : PTE_USER;
	pte |= pte_flag_bits(mdpg, flags, kernel_p);
	pte |= pte_prot_bits(mdpg, prot, kernel_p);

	if (mdpg != NULL) {

		if ((prot & VM_PROT_WRITE) != 0 &&
		    ((flags & VM_PROT_WRITE) != 0 || VM_PAGEMD_MODIFIED_P(mdpg))) {
			/*
			* This is a writable mapping, and the page's mod state
			* indicates it has already been modified.  No need for
			* modified emulation.
			*/
			pte |= PTE_A;
		} else if ((flags & VM_PROT_ALL) || VM_PAGEMD_REFERENCED_P(mdpg)) {
			/*
			* - The access type indicates that we don't need to do
			*   referenced emulation.
			* OR
			* - The physical page has already been referenced so no need
			*   to re-do referenced emulation here.
			*/
			pte |= PTE_A;
		}
	} else {
		pte |= PTE_A | PTE_D;
	}

	return pte;
}

static inline pt_entry_t
pte_make_kenter_pa(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
    int flags)
{
	pt_entry_t pte = (pt_entry_t)PA_TO_PTE(pa);

	pte |= PTE_KERN | PTE_HARDWIRED | PTE_WIRED;
	pte |= pte_flag_bits(NULL, flags, true);
	pte |= pte_prot_bits(NULL, prot, true);

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
pte_pde_pdetab(paddr_t pa, bool kernel_p)
{
	return PTE_V | (pa >> PAGE_SHIFT) << PTE_PPN_SHIFT;
}

static inline pd_entry_t
pte_pde_ptpage(paddr_t pa, bool kernel_p)
{
	return PTE_V | (pa >> PAGE_SHIFT) << PTE_PPN_SHIFT;
}

static inline bool
pte_pde_valid_p(pd_entry_t pde)
{
	return (pde & (PTE_X | PTE_W | PTE_R | PTE_V)) == PTE_V;
}

static inline paddr_t
pte_pde_to_paddr(pd_entry_t pde)
{
	return pte_to_paddr((pt_entry_t)pde);
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

static inline void
pte_pde_set(pd_entry_t *pdep, pd_entry_t npde)
{

	*pdep = npde;
}


static inline pt_entry_t
pte_value(pt_entry_t pte)
{
	return pte;
}

#endif /* _RISCV_PTE_H_ */
