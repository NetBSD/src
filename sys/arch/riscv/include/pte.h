/* $NetBSD: pte.h,v 1.1.22.3 2020/04/13 08:04:05 martin Exp $ */

/*
 * Copyright (c) 2014, 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas (of 3am Software Foundry) and Maxime Villard.
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

#ifdef _LP64	/* Sv39 */
#define PTE_PPN		__BITS(53, 10)
#define	PTE_PPN0	__BITS(18, 10)
#define	PTE_PPN1	__BITS(27, 19)
#define	PTE_PPN2	__BITS(53, 28)
typedef __uint64_t pt_entry_t;
typedef __uint64_t pd_entry_t;
#define atomic_cas_pte	atomic_cas_64
#else		/* Sv32 */
#define PTE_PPN		__BITS(31, 10)
#define	PTE_PPN0	__BITS(19, 10)
#define	PTE_PPN1	__BITS(31, 20)
typedef __uint32_t pt_entry_t;
typedef __uint32_t pd_entry_t;
#define atomic_cas_pte	atomic_cas_32
#endif

#define PTE_PPN_SHIFT	10

#define NPTEPG		(PAGE_SIZE / sizeof(pt_entry_t))
#define NSEGPG		NPTEPG
#define NPDEPG		NPTEPG

/* Software PTE bits. */
#define	PTE_WIRED	__BIT(8)

/* Hardware PTE bits. */
#define	PTE_D		__BIT(7)
#define	PTE_A		__BIT(6)
#define	PTE_G		__BIT(5)
#define	PTE_U		__BIT(4)
#define	PTE_X		__BIT(3)
#define	PTE_W		__BIT(2)
#define	PTE_R		__BIT(1)
#define	PTE_V		__BIT(0)

#define PA_TO_PTE(pa)	(((pa) >> PAGE_SHIFT) << PTE_PPN_SHIFT)
#define PTE_TO_PA(pte)	(((pte) >> PTE_PPN_SHIFT) << PAGE_SHIFT)

#define	L2_SHIFT	30
#define	L1_SHIFT	21
#define	L0_SHIFT	12

#define	L2_SIZE 	(1 << L2_SHIFT)
#define	L1_SIZE 	(1 << L1_SHIFT)
#define	L0_SIZE 	(1 << L0_SHIFT)

#define	L2_OFFSET 	(L2_SIZE - 1)
#define	L1_OFFSET 	(L1_SIZE - 1)
#define	L0_OFFSET 	(L0_SIZE - 1)

#define	Ln_ENTRIES	(1 << 9)
#define	Ln_ADDR_MASK	(Ln_ENTRIES - 1)

#define pl2_i(va)	(((va) >> L2_SHIFT) & Ln_ADDR_MASK)
#define pl1_i(va)	(((va) >> L1_SHIFT) & Ln_ADDR_MASK)
#define pl0_i(va)	(((va) >> L0_SHIFT) & Ln_ADDR_MASK)

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
	pt_entry_t pte;

	KASSERT(prot & VM_PROT_READ);

	pte = PTE_R;
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
	pt_entry_t pte = (pt_entry_t)PA_TO_PTE(pa);

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
	pt_entry_t pte = (pt_entry_t)PA_TO_PTE(pa);

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

static inline pt_entry_t
pte_value(pt_entry_t pte)
{
	return pte;
}

#endif /* _RISCV_PTE_H_ */
