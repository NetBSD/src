/*	$NetBSD: pte.h,v 1.6.30.2 2016/10/05 20:55:34 skrll Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_PTE_H_
#define _POWERPC_BOOKE_PTE_H_

#ifndef _LOCORE
#ifndef __BSD_PT_ENTRY_T
#define __BSD_PT_ENTRY_T	__uint32_t
typedef __BSD_PT_ENTRY_T	pt_entry_t;
#define PRIxPTE			PRIx32
#endif
#endif

#include <powerpc/booke/spr.h>

/*
 * The PTE format is software and must be translated into the various portions
 * X W R are separted by single bits so that they can map to the MAS2 bits
 * UX/UW/UR or SX/SW/SR by a mask and a shift.
 */
#define	PTE_IO		(PTE_I|PTE_G|PTE_xW|PTE_xR)
#define	PTE_DEFAULT	(PTE_M|PTE_xX|PTE_xW|PTE_xR)
#define	PTE_MAS3_MASK	(MAS3_RPN|MAS3_U2|MAS3_U0)
#define	PTE_MAS2_MASK	(MAS2_WIMGE)
#define	PTE_RPN_MASK	MAS3_RPN		/* MAS3[RPN] */
#define	PTE_RWX_MASK	(PTE_xX|PTE_xW|PTE_xR)
#define	PTE_WIRED	(MAS3_U0 << 2)		/* page is wired (PTE only) */
#define	PTE_xX		(MAS3_U0 << 1)		/* MAS2[UX] | MAS2[SX] */
#define	PTE_UNSYNCED	MAS3_U0			/* page needs isync */
#define	PTE_xW		MAS3_U1			/* MAS2[UW] | MAS2[SW] */
#define	PTE_UNMODIFIED	MAS3_U2			/* page is unmodified */
#define	PTE_xR		MAS3_U3			/* MAS2[UR] | MAS2[SR] */
#define PTE_RWX_SHIFT	6
#define	PTE_UNUSED	0x00000020
#define	PTE_WIMGE_MASK	MAS2_WIMGE
#define	PTE_WIG		(PTE_W|PTE_I|PTE_G)
#define	PTE_W		MAS2_W			/* Write-through */
#define	PTE_I		MAS2_I			/* cache-Inhibited */
#define	PTE_M		MAS2_M			/* Memory coherence */
#define	PTE_G		MAS2_G			/* Guarded */
#define	PTE_E		MAS2_E			/* [Little] Endian */

#ifndef _LOCORE
#ifdef _KERNEL

static inline uint32_t
pte_value(pt_entry_t pt_entry)
{
	return pt_entry;
}

static inline bool
pte_cached_p(pt_entry_t pt_entry)
{
	return (pt_entry & PTE_I) == 0;
}

static inline bool
pte_modified_p(pt_entry_t pt_entry)
{
	return (pt_entry & (PTE_UNMODIFIED|PTE_xW)) == PTE_xW;
}

static inline bool
pte_valid_p(pt_entry_t pt_entry)
{
	return pt_entry != 0;
}

static inline bool
pte_zero_p(pt_entry_t pt_entry)
{
	return pt_entry == 0;
}

static inline bool
pte_exec_p(pt_entry_t pt_entry)
{
	return (pt_entry & PTE_xX) != 0;
}

static inline bool
pte_readonly_p(pt_entry_t pt_entry)
{
	return (pt_entry & PTE_xW) == 0;
}

static inline bool
pte_deferred_exec_p(pt_entry_t pt_entry)
{
	//return (pt_entry & (PTE_xX|PTE_UNSYNCED)) == (PTE_xX|PTE_UNSYNCED);
	return (pt_entry & PTE_UNSYNCED) == PTE_UNSYNCED;
}

static inline bool
pte_wired_p(pt_entry_t pt_entry)
{
	return (pt_entry & PTE_WIRED) != 0;
}

static inline pt_entry_t
pte_nv_entry(bool kernel)
{
	return 0;
}

static inline paddr_t
pte_to_paddr(pt_entry_t pt_entry)
{
	return (paddr_t)(pt_entry & PTE_RPN_MASK);
}

static inline pt_entry_t
pte_ionocached_bits(void)
{
	return PTE_I|PTE_G;
}

static inline pt_entry_t
pte_iocached_bits(void)
{
	return PTE_G;
}

static inline pt_entry_t
pte_nocached_bits(void)
{
	return PTE_M|PTE_I;
}

static inline pt_entry_t
pte_cached_bits(void)
{
	return PTE_M;
}

static inline pt_entry_t
pte_cached_change(pt_entry_t pt_entry, bool cached)
{
	return (pt_entry & ~PTE_I) | (cached ? 0 : PTE_I);
}

static inline pt_entry_t
pte_wire_entry(pt_entry_t pt_entry)
{
	return pt_entry | PTE_WIRED;
}

static inline pt_entry_t
pte_unwire_entry(pt_entry_t pt_entry)
{
	return pt_entry & ~PTE_WIRED;
}

static inline pt_entry_t
pte_prot_nowrite(pt_entry_t pt_entry)
{
	return pt_entry & ~(PTE_xW|PTE_UNMODIFIED);
}

static inline pt_entry_t
pte_prot_downgrade(pt_entry_t pt_entry, vm_prot_t newprot)
{
	pt_entry &= ~(PTE_xW|PTE_UNMODIFIED);
	if ((newprot & VM_PROT_EXECUTE) == 0)
		pt_entry &= ~(PTE_xX|PTE_UNSYNCED);
	return pt_entry;
}

static inline pt_entry_t
pte_prot_bits(struct vm_page_md *mdpg, vm_prot_t prot)
{
	KASSERT(prot & VM_PROT_READ);
	pt_entry_t pt_entry = PTE_xR;
	if (prot & VM_PROT_EXECUTE) {
#if 0
		pt_entry |= PTE_xX;
		if (mdpg != NULL && !VM_PAGEMD_EXECPAGE_P(mdpg))
			pt_entry |= PTE_UNSYNCED;
#elif 1
		if (mdpg != NULL && !VM_PAGEMD_EXECPAGE_P(mdpg))
			pt_entry |= PTE_UNSYNCED;
		else
			pt_entry |= PTE_xX;
#else
		pt_entry |= PTE_UNSYNCED;
#endif
	}
	if (prot & VM_PROT_WRITE) {
		pt_entry |= PTE_xW;
		if (mdpg != NULL && !VM_PAGEMD_MODIFIED_P(mdpg))
			pt_entry |= PTE_UNMODIFIED;
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
	pt_entry_t pt_entry = (pt_entry_t) pa & PTE_RPN_MASK;

	pt_entry |= pte_flag_bits(mdpg, flags);
	pt_entry |= pte_prot_bits(mdpg, prot);

	return pt_entry;
}

static inline pt_entry_t
pte_make_kenter_pa(paddr_t pa, struct vm_page_md *mdpg, vm_prot_t prot,
	int flags)
{
	pt_entry_t pt_entry = (pt_entry_t) pa & PTE_RPN_MASK;

	pt_entry |= PTE_WIRED;
	pt_entry |= pte_flag_bits(mdpg, flags);
	pt_entry |= pte_prot_bits(NULL, prot); /* pretend unmanaged */

	return pt_entry;
}

#endif /* _KERNEL */
#endif /* !_LOCORE */

#endif /* !_POWERPC_BOOKE_PTE_H_ */
