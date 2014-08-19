/* $NetBSD: pte_coldfire.h,v 1.2.8.2 2014/08/20 00:03:10 tls Exp $ */
/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _M68K_PTE_COLDFIRE_H_
#define _M68K_PTE_COLDFIRE_H_

#ifdef __ASSEMBLY__
#error use assym.h instead
#endif

#ifndef __BSD_PT_ENTRY_T
#define __BSD_PT_ENTRY_T	__uint32_t
typedef __BSD_PT_ENTRY_T	pt_entry_t;
#endif

#define	MMUTR_VA	__BITS(31,10)	// Virtual Address
#define	MMUTR_ID	__BITS(9,2)	// ASID
#define	MMUTR_SG	__BIT(1)	// Shared Global
#define	MMUTR_V		__BIT(0)	// Valid

#define MMUDR_PA	__BITS(31,10)	// Physical Address
#define	MMUDR_SZ	__BITS(9,8)	// Entry Size
#define	MMUDR_SZ_1MB	0
#define	MMUDR_SZ_4KB	1
#define	MMUDR_SZ_8KB	2
#define	MMUDR_SZ_16MB	3
#define	MMUDR_CM	__BITS(7,6)	// Cache Mode
#define MMUDR_CM_WT	0		// Write-Through
#define MMUDR_CM_WB	1		// Write-Back (Copy-Back)
#define MMUDR_CM_NC	2		// Non-cacheable
#define MMUDR_CM_NCP	2		// Non-cacheable Precise
#define MMUDR_CM_NCI	3		// Non-cacheable Imprecise
#define MMUDR_SP	__BIT(5)	// Supervisor Protect
#define MMUDR_R		__BIT(4)	// Read Access
#define MMUDR_W		__BIT(3)	// Write Access
#define MMUDR_X		__BIT(2)	// Execute Access
#define MMUDR_LK	__BIT(1)	// Lock Entry
#define MMUDR_MBZ0	__BIT(0)	// Must be zero

/*
 * The PTE basically the contents of MMUDR[31:2] | MMUAR[0].
 * We overload the meaning of MMUDR_LK for indicating wired.
 * It will be cleared before writing to the TLB.
 */

#ifdef _KERNEL

static inline bool
pte_cached_p(pt_entry_t pt_entry)
{
	return (pt_entry & MMUDR_CM_NC) != MMUDR_CM_NC;
}

static inline bool
pte_modified_p(pt_entry_t pt_entry)
{
	return (pt_entry & MMUDR_W) == MMUDR_W;
}

static inline bool
pte_valid_p(pt_entry_t pt_entry)
{
	return (pt_entry & MMUAR_V) == MMUAR_V;
}

static inline bool
pte_exec_p(pt_entry_t pt_entry)
{
	return (pt_entry & MMUDR_X) == MMUDR_X;
}

static inline bool
pte_deferred_exec_p(pt_entry_t pt_entry)
{
	return !pte_exec_p(pt_entry);
}

static inline bool
pte_wired_p(pt_entry_t pt_entry)
{
	return (pt_entry & MMUDR_LK) == MMUDR_LK;
}

static inline pt_entry_t
pte_nv_entry(bool kernel)
{
	return 0;
}

static inline paddr_t
pte_to_paddr(pt_entry_t pt_entry)
{
	return (paddr_t)(pt_entry & MMUDR_PA);
}

static inline pt_entry_t
pte_ionocached_bits(void)
{
	return MMUDR_CM_NCP;
}

static inline pt_entry_t
pte_iocached_bits(void)
{
	return MMUDR_CM_NCP;
}

static inline pt_entry_t
pte_nocached_bits(void)
{
	return MMUDR_CM_NCP;
}

static inline pt_entry_t
pte_cached_bits(void)
{
	return MMUDR_CM_WB;
}

static inline pt_entry_t
pte_cached_change(pt_entry_t pt_entry, bool cached)
{
	return (pt_entry & ~MMUDR_CM) | (cached ? MMUDR_CM_WB : MMUDR_CM_NCP);
}

static inline pt_entry_t
pte_wire_entry(pt_entry_t pt_entry)
{
	return pt_entry | MMUDR_LK;
}

static inline pt_entry_t
pte_unwire_entry(pt_entry_t pt_entry)
{
	return pt_entry & ~MMUDR_LK;
}

static inline pt_entry_t
pte_prot_nowrite(pt_entry_t pt_entry)
{
	return pt_entry & ~MMUDR_W;
}

static inline pt_entry_t
pte_prot_downgrade(pt_entry_t pt_entry, vm_prot_t newprot)
{
	pt_entry &= ~MMUDR_W;
	if ((newprot & VM_PROT_EXECUTE) == 0)
		pt_entry &= ~MMUDR_X;
	return pt_entry;
}

static inline pt_entry_t
pte_prot_bits(struct vm_page_md *mdpg, vm_prot_t prot)
{
	KASSERT(prot & VM_PROT_READ);
	pt_entry_t pt_entry = MMUDR_R;
	if (prot & VM_PROT_EXECUTE) {
		/* Only allow exec for managed pages */
		if (mdpg != NULL && VM_PAGEMD_EXECPAGE_P(mdpg))
			pt_entry |= MMUDR_X;
	}
	if (prot & VM_PROT_WRITE) {
		if (mdpg == NULL || VM_PAGEMD_MODIFIED_P(mdpg))
			pt_entry |= MMUDR_W;
	}
	return pt_entry;
}

static inline pt_entry_t
pte_flag_bits(struct vm_page_md *mdpg, int flags)
{
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
}

static inline pt_entry_t
pte_make_enter(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
	int flags, bool kernel)
{
	pt_entry_t pt_entry = (pt_entry_t) pa & MMUDR_PA;

	pt_entry |= pte_flag_bits(mdpg, flags);
	pt_entry |= pte_prot_bits(mdpg, prot);

	return pt_entry;
}

static inline pt_entry_t
pte_make_kenter_pa(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
	int flags)
{
	pt_entry_t pt_entry = (pt_entry_t) pa & MMUDR_PA;

	pt_entry |= MMUDR_LK;
	pt_entry |= pte_flag_bits(mdpg, flags);
	pt_entry |= pte_prot_bits(NULL, prot); /* pretend unmanaged */

	return pt_entry;
}
#endif /* _KERNEL_ */

#endif /* _M68K_PTE_COLDFIRE_H_ */
