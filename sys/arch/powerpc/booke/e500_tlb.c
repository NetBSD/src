/*	$NetBSD: e500_tlb.c,v 1.11.2.2 2014/08/20 00:03:19 tls Exp $	*/
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

#define	__PMAP_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: e500_tlb.c,v 1.11.2.2 2014/08/20 00:03:19 tls Exp $");

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/e500var.h>
#include <powerpc/booke/pmap.h>

struct e500_tlb {
	vaddr_t tlb_va;
	uint32_t tlb_pte;
	uint32_t tlb_asid;
	vsize_t tlb_size;
};

struct e500_hwtlb {
	uint32_t hwtlb_mas0;
	uint32_t hwtlb_mas1;
	uint32_t hwtlb_mas2;
	uint32_t hwtlb_mas3;
};

struct e500_xtlb {
	struct e500_tlb e_tlb;
	struct e500_hwtlb e_hwtlb;
	u_long e_refcnt;
};

static struct e500_tlb1 {
	uint32_t tlb1_maxsize;
	uint32_t tlb1_minsize;
	u_int tlb1_numentries;
	u_int tlb1_numfree;
	u_int tlb1_freelist[32];
	struct e500_xtlb tlb1_entries[32];
} e500_tlb1;

static inline register_t mftlb0cfg(void) __pure;
static inline register_t mftlb1cfg(void) __pure;

static inline register_t
mftlb0cfg(void)
{
	register_t tlb0cfg;
	__asm("mfspr %0, %1" : "=r"(tlb0cfg) : "n"(SPR_TLB0CFG));
	return tlb0cfg;
}

static inline register_t
mftlb1cfg(void)
{
	register_t tlb1cfg;
	__asm("mfspr %0, %1" : "=r"(tlb1cfg) : "n"(SPR_TLB1CFG));
	return tlb1cfg;
}

static struct e500_tlb
hwtlb_to_tlb(const struct e500_hwtlb hwtlb)
{
	struct e500_tlb tlb;
	register_t prot_mask;
	u_int prot_shift;

	tlb.tlb_va = MAS2_EPN & hwtlb.hwtlb_mas2;
	tlb.tlb_size = 1024 << (2 * MASX_TSIZE_GET(hwtlb.hwtlb_mas1));
	tlb.tlb_asid = MASX_TID_GET(hwtlb.hwtlb_mas1);
	tlb.tlb_pte = (hwtlb.hwtlb_mas2 & MAS2_WIMGE)
	    | (hwtlb.hwtlb_mas3 & MAS3_RPN);
	if (hwtlb.hwtlb_mas1 & MAS1_TS) {
		prot_mask = MAS3_UX|MAS3_UW|MAS3_UR;
		prot_shift = PTE_RWX_SHIFT - 1;
	} else {
		prot_mask = MAS3_SX|MAS3_SW|MAS3_SR;
		prot_shift = PTE_RWX_SHIFT;
	}
	tlb.tlb_pte |= (prot_mask & hwtlb.hwtlb_mas3) << prot_shift;
	return tlb;
}

static inline struct e500_hwtlb
hwtlb_read(uint32_t mas0, u_int slot)
{
	struct e500_hwtlb hwtlb;
	register_t tlbcfg;

	if (__predict_true(mas0 == MAS0_TLBSEL_TLB0)) {
		tlbcfg = mftlb0cfg();
	} else if (mas0 == MAS0_TLBSEL_TLB1) {
		tlbcfg = mftlb1cfg();
	} else {
		panic("%s:%d: unexpected MAS0 %#" PRIx32,
		    __func__, __LINE__, mas0);
	}

	/*
	 * ESEL is the way we want to look up.
	 * If tlbassoc is the same as tlbentries (like in TLB1) then the TLB is
	 * fully associative, the entire slot is placed into ESEL.  If tlbassoc 
	 * is less than the number of tlb entries, the slot is split in two
	 * fields.  Since the TLB is M rows by N ways, the lowers bits are for
	 * row (MAS2[EPN]) and the upper for the way (MAS1[ESEL]). 
	 */
	const u_int tlbassoc = TLBCFG_ASSOC(tlbcfg);
	const u_int tlbentries = TLBCFG_NENTRY(tlbcfg);
	const u_int esel_shift =
	    __builtin_clz(tlbassoc) - __builtin_clz(tlbentries);

	/*
	 * Disable interrupts since we don't want anyone else mucking with
	 * the MMU Assist registers
	 */
	const register_t msr = wrtee(0);
	const register_t saved_mas0 = mfspr(SPR_MAS0);
	mtspr(SPR_MAS0, mas0 | MAS0_ESEL_MAKE(slot >> esel_shift));

	if (__predict_true(tlbassoc > tlbentries))
		mtspr(SPR_MAS2, slot << PAGE_SHIFT);

	/*
	 * Now select the entry and grab its contents.
	 */
	__asm volatile("tlbre");
	
	hwtlb.hwtlb_mas0 = mfspr(SPR_MAS0);
	hwtlb.hwtlb_mas1 = mfspr(SPR_MAS1);
	hwtlb.hwtlb_mas2 = mfspr(SPR_MAS2);
	hwtlb.hwtlb_mas3 = mfspr(SPR_MAS3);

	mtspr(SPR_MAS0, saved_mas0);
	wrtee(msr);	/* restore interrupts */

	return hwtlb;
}

static inline void
hwtlb_write(const struct e500_hwtlb hwtlb, bool needs_sync)
{
	const register_t msr = wrtee(0);
	const uint32_t saved_mas0 = mfspr(SPR_MAS0);

	/*
	 * Need to always write MAS0 and MAS1
	 */
	mtspr(SPR_MAS0, hwtlb.hwtlb_mas0);
	mtspr(SPR_MAS1, hwtlb.hwtlb_mas1);

	/*
	 * Only write the VPN/WIMGE if this is in TLB0 or if a valid mapping.
	 */
	if ((hwtlb.hwtlb_mas0 & MAS0_TLBSEL) == MAS0_TLBSEL_TLB0
	    || (hwtlb.hwtlb_mas1 & MAS1_V)) {
		mtspr(SPR_MAS2, hwtlb.hwtlb_mas2);
	}
	/*
	 * Only need to write the RPN/prot if we are dealing with a valid
	 * mapping.
	 */
	if (hwtlb.hwtlb_mas1 & MAS1_V) {
		mtspr(SPR_MAS3, hwtlb.hwtlb_mas3);
		//mtspr(SPR_MAS7, 0);
	}
	
#if 0
	printf("%s->[%x,%x,%x,%x]\n",
	    __func__, 
	    hwtlb.hwtlb_mas0, hwtlb.hwtlb_mas1,
	    hwtlb.hwtlb_mas2, hwtlb.hwtlb_mas3);
#endif
	__asm volatile("tlbwe");
	if (needs_sync) {
		__asm volatile("tlbsync\n\tisync\n\tsync");
	}

	mtspr(SPR_MAS0, saved_mas0);
	wrtee(msr);
}

static struct e500_hwtlb
tlb_to_hwtlb(const struct e500_tlb tlb)
{
	struct e500_hwtlb hwtlb;

	KASSERT(trunc_page(tlb.tlb_va) == tlb.tlb_va);
	KASSERT(tlb.tlb_size != 0);
	KASSERT((tlb.tlb_size & (tlb.tlb_size - 1)) == 0);
	const uint32_t prot_mask = tlb.tlb_pte & PTE_RWX_MASK;
	if (__predict_true(tlb.tlb_size == PAGE_SIZE)) {
		hwtlb.hwtlb_mas0 = 0;
		hwtlb.hwtlb_mas1 = MAS1_V | MASX_TSIZE_MAKE(1);
		/*
		 * A non-zero ASID means this is a user page so mark it as
		 * being in the user's address space.
		 */
		if (tlb.tlb_asid) {
			hwtlb.hwtlb_mas1 |= MAS1_TS
			    | MASX_TID_MAKE(tlb.tlb_asid);
			hwtlb.hwtlb_mas3 = (prot_mask >> (PTE_RWX_SHIFT - 1))
			    | ((prot_mask & ~PTE_xX) >> PTE_RWX_SHIFT);
			KASSERT(prot_mask & PTE_xR);
			KASSERT(hwtlb.hwtlb_mas3 & MAS3_UR);
			CTASSERT(MAS3_UR == (PTE_xR >> (PTE_RWX_SHIFT - 1)));
			CTASSERT(MAS3_SR == (PTE_xR >> PTE_RWX_SHIFT));
		} else {
			hwtlb.hwtlb_mas3 = prot_mask >> PTE_RWX_SHIFT;
		}
		if (tlb.tlb_pte & PTE_UNMODIFIED)
			hwtlb.hwtlb_mas3 &= ~(MAS3_UW|MAS3_SW);
		if (tlb.tlb_pte & PTE_UNSYNCED)
			hwtlb.hwtlb_mas3 &= ~(MAS3_UX|MAS3_SX);
	} else {
		KASSERT(tlb.tlb_asid == 0);
		KASSERT((tlb.tlb_size & 0xaaaaa7ff) == 0);
		u_int cntlz = __builtin_clz(tlb.tlb_size);
		KASSERT(cntlz & 1);
		KASSERT(cntlz <= 19);
		hwtlb.hwtlb_mas0 = MAS0_TLBSEL_TLB1;
		/*
		 * TSIZE is defined (4^TSIZE) Kbytes except a TSIZE of 0 is not
		 * allowed.  So 1K would be 0x00000400 giving 21 leading zero
		 * bits.  Subtracting the leading number of zero bits from 21
		 * and dividing by 2 gives us the number that the MMU wants.
		 */
		hwtlb.hwtlb_mas1 = MASX_TSIZE_MAKE(((31 - 10) - cntlz) / 2)
		    | MAS1_IPROT | MAS1_V;
		hwtlb.hwtlb_mas3 = prot_mask >> PTE_RWX_SHIFT;
	}
	/* We are done with MAS1, on to MAS2 ... */
	hwtlb.hwtlb_mas2 = tlb.tlb_va | (tlb.tlb_pte & PTE_WIMGE_MASK);
	hwtlb.hwtlb_mas3 |= tlb.tlb_pte & PTE_RPN_MASK;

	return hwtlb;
}

void *
e500_tlb1_fetch(size_t slot)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;

	return &tlb1->tlb1_entries[slot].e_hwtlb;
}

void
e500_tlb1_sync(void)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	for (u_int slot = 1; slot < tlb1->tlb1_numentries; slot++) {
		const struct e500_hwtlb * const new_hwtlb =
		    &tlb1->tlb1_entries[slot].e_hwtlb;
		const struct e500_hwtlb old_hwtlb =
		    hwtlb_read(MAS0_TLBSEL_TLB1, slot);
#define CHANGED(n,o,f)	((n)->f != (o).f)
		bool mas1_changed_p = CHANGED(new_hwtlb, old_hwtlb, hwtlb_mas1);
		bool mas2_changed_p = CHANGED(new_hwtlb, old_hwtlb, hwtlb_mas2);
		bool mas3_changed_p = CHANGED(new_hwtlb, old_hwtlb, hwtlb_mas3);
#undef CHANGED
		bool new_valid_p = (new_hwtlb->hwtlb_mas1 & MAS1_V) != 0;
		bool old_valid_p = (old_hwtlb.hwtlb_mas1 & MAS1_V) != 0;
		if ((new_valid_p || old_valid_p)
		    && (mas1_changed_p
			|| (new_valid_p
			    && (mas2_changed_p || mas3_changed_p))))
			hwtlb_write(*new_hwtlb, true);
	}
}

static int
e500_alloc_tlb1_entry(void)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;

	if (tlb1->tlb1_numfree == 0)
		return -1;
	const u_int slot = tlb1->tlb1_freelist[--tlb1->tlb1_numfree];
	KASSERT((tlb1->tlb1_entries[slot].e_hwtlb.hwtlb_mas1 & MAS1_V) == 0);
	tlb1->tlb1_entries[slot].e_hwtlb.hwtlb_mas0 = 
	    MAS0_TLBSEL_TLB1 | __SHIFTIN(slot, MAS0_ESEL);
	return (int)slot;
}

static void
e500_free_tlb1_entry(struct e500_xtlb *xtlb, u_int slot, bool needs_sync)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	KASSERT(slot < tlb1->tlb1_numentries);
	KASSERT(&tlb1->tlb1_entries[slot] == xtlb);

	KASSERT(xtlb->e_hwtlb.hwtlb_mas0 == (MAS0_TLBSEL_TLB1|__SHIFTIN(slot, MAS0_ESEL)));
	xtlb->e_hwtlb.hwtlb_mas1 &= ~(MAS1_V|MAS1_IPROT);
	hwtlb_write(xtlb->e_hwtlb, needs_sync);

	const register_t msr = wrtee(0);
	tlb1->tlb1_freelist[tlb1->tlb1_numfree++] = slot;
	wrtee(msr);
}

static tlb_asid_t
e500_tlb_get_asid(void)
{
	return mfspr(SPR_PID0);
}

static void
e500_tlb_set_asid(tlb_asid_t asid)
{
	mtspr(SPR_PID0, asid);
}

static void
e500_tlb_invalidate_all(void)
{
	/*
	 * This does a flash invalidate of all entries in TLB0.
	 * We don't touch TLB1 since we don't expect those to be volatile.
	 */
#if 1
	__asm volatile("tlbivax\t0, %0" :: "b"(4));	/* INV_ALL */
	__asm volatile("tlbsync\n\tisync\n\tsync");
#else
	mtspr(SPR_MMUCSR0, MMUCSR0_TLB0_FL);
	while (mfspr(SPR_MMUCSR0) != 0)
		;
#endif
}

static void
e500_tlb_invalidate_globals(void)
{
	const size_t tlbassoc = TLBCFG_ASSOC(mftlb0cfg());
	const size_t tlbentries = TLBCFG_NENTRY(mftlb0cfg());
	const size_t max_epn = (tlbentries / tlbassoc) << PAGE_SHIFT;
	const vaddr_t kstack_lo = (uintptr_t)curlwp->l_addr;
	const vaddr_t kstack_hi = kstack_lo + USPACE - 1;
	const vaddr_t epn_kstack_lo = kstack_lo & (max_epn - 1);
	const vaddr_t epn_kstack_hi = kstack_hi & (max_epn - 1);

	const register_t msr = wrtee(0);
	for (size_t assoc = 0; assoc < tlbassoc; assoc++) {
		mtspr(SPR_MAS0, MAS0_ESEL_MAKE(assoc) | MAS0_TLBSEL_TLB0);
		for (size_t epn = 0; epn < max_epn; epn += PAGE_SIZE) {
			mtspr(SPR_MAS2, epn);
			__asm volatile("tlbre");
			uint32_t mas1 = mfspr(SPR_MAS1);

			/*
			 * Make sure this is a valid kernel entry first.
			 */
			if ((mas1 & (MAS1_V|MAS1_TID|MAS1_TS)) != MAS1_V)
				continue;

			/*
			 * We have a valid kernel TLB entry.  But if it matches
			 * the stack we are currently running on, it would
			 * unwise to invalidate it.  First see if the epn
			 * overlaps the stack.  If it does then get the
			 * VA and see if it really is part of the stack.
			 */
			if (epn_kstack_lo < epn_kstack_hi
			    ? (epn_kstack_lo <= epn && epn <= epn_kstack_hi)
			    : (epn <= epn_kstack_hi || epn_kstack_lo <= epn)) {
				const uint32_t mas2_epn =
				    mfspr(SPR_MAS2) & MAS2_EPN;
				if (kstack_lo <= mas2_epn
				    && mas2_epn <= kstack_hi)
					continue;
			}
			mtspr(SPR_MAS1, mas1 ^ MAS1_V);
			__asm volatile("tlbwe");
		}
	}
	__asm volatile("isync\n\tsync");
	wrtee(msr);
}

static void
e500_tlb_invalidate_asids(tlb_asid_t asid_lo, tlb_asid_t asid_hi)
{
	const size_t tlbassoc = TLBCFG_ASSOC(mftlb0cfg());
	const size_t tlbentries = TLBCFG_NENTRY(mftlb0cfg());
	const size_t max_epn = (tlbentries / tlbassoc) << PAGE_SHIFT;

	asid_lo = __SHIFTIN(asid_lo, MAS1_TID);
	asid_hi = __SHIFTIN(asid_hi, MAS1_TID);

	const register_t msr = wrtee(0);
	for (size_t assoc = 0; assoc < tlbassoc; assoc++) {
		mtspr(SPR_MAS0, MAS0_ESEL_MAKE(assoc) | MAS0_TLBSEL_TLB0);
		for (size_t epn = 0; epn < max_epn; epn += PAGE_SIZE) {
			mtspr(SPR_MAS2, epn);
			__asm volatile("tlbre");
			const uint32_t mas1 = mfspr(SPR_MAS1);
			/*
			 * If this is a valid entry for AS space 1 and
			 * its asid matches the constraints of the caller,
			 * clear its valid bit.
			 */
			if ((mas1 & (MAS1_V|MAS1_TS)) == (MAS1_V|MAS1_TS)
			    && asid_lo <= (mas1 & MAS1_TID)
			    && (mas1 & MAS1_TID) <= asid_hi) {
				mtspr(SPR_MAS1, mas1 ^ MAS1_V);
#if 0
				printf("%s[%zu,%zu]->[%x]\n",
				    __func__, assoc, epn, mas1);
#endif
				__asm volatile("tlbwe");
			}
		}
	}
	__asm volatile("isync\n\tsync");
	wrtee(msr);
}

static u_int
e500_tlb_record_asids(u_long *bitmap)
{
	const size_t tlbassoc = TLBCFG_ASSOC(mftlb0cfg());
	const size_t tlbentries = TLBCFG_NENTRY(mftlb0cfg());
	const size_t max_epn = (tlbentries / tlbassoc) << PAGE_SHIFT;
	const size_t nbits = 8 * sizeof(bitmap[0]);
	u_int found = 0;

	const register_t msr = wrtee(0);
	for (size_t assoc = 0; assoc < tlbassoc; assoc++) {
		mtspr(SPR_MAS0, MAS0_ESEL_MAKE(assoc) | MAS0_TLBSEL_TLB0);
		for (size_t epn = 0; epn < max_epn; epn += PAGE_SIZE) {
			mtspr(SPR_MAS2, epn);
			__asm volatile("tlbre");
			const uint32_t mas1 = mfspr(SPR_MAS1);
			/*
			 * If this is a valid entry for AS space 1 and
			 * its asid matches the constraints of the caller,
			 * clear its valid bit.
			 */
			if ((mas1 & (MAS1_V|MAS1_TS)) == (MAS1_V|MAS1_TS)) {
				const uint32_t asid = MASX_TID_GET(mas1);
				const u_int i = asid / nbits;
				const u_long mask = 1UL << (asid & (nbits - 1));
				if ((bitmap[i] & mask) == 0) {
					bitmap[i] |= mask;
					found++;
				}
			}
		}
	}
	wrtee(msr);

	return found;
}

static void
e500_tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
{
	KASSERT((va & PAGE_MASK) == 0);
	/*
	 * Bits 60 & 61 have meaning
	 */
	if (asid == KERNEL_PID) {
		/*
		 * For data accesses, the context-synchronizing instruction
		 * before tlbwe or tlbivax ensures that all memory accesses
		 * due to preceding instructions have completed to a point
		 * at which they have reported all exceptions they will cause.
		 */
		__asm volatile("isync");
	}
	__asm volatile("tlbivax\t0, %0" :: "b"(va));
	__asm volatile("tlbsync");
	__asm volatile("tlbsync");	/* Why? */
	if (asid == KERNEL_PID) {
		/*
		 * The context-synchronizing instruction after tlbwe or tlbivax
		 * ensures that subsequent accesses (data and instruction) use
		 * the updated value in any TLB entries affected.
		 */
		__asm volatile("isync\n\tsync");
	}
}

static bool
e500_tlb_update_addr(vaddr_t va, tlb_asid_t asid, pt_entry_t pte, bool insert)
{
	struct e500_hwtlb hwtlb = tlb_to_hwtlb(
	    (struct e500_tlb){ .tlb_va = va, .tlb_asid = asid,
		.tlb_size = PAGE_SIZE, .tlb_pte = pte,});

	register_t msr = wrtee(0);
	mtspr(SPR_MAS6, asid ? __SHIFTIN(asid, MAS6_SPID0) | MAS6_SAS : 0);
	__asm volatile("tlbsx 0, %0" :: "b"(va));
	register_t mas1 = mfspr(SPR_MAS1);
	if ((mas1 & MAS1_V) == 0) {
		if (!insert) {
			wrtee(msr);
#if 0
			printf("%s(%#lx,%#x,%#x,%x)<no update>\n",
			    __func__, va, asid, pte, insert);
#endif
			return false;
		}
		mtspr(SPR_MAS1, hwtlb.hwtlb_mas1);
	}
	mtspr(SPR_MAS2, hwtlb.hwtlb_mas2);
	mtspr(SPR_MAS3, hwtlb.hwtlb_mas3);
	//mtspr(SPR_MAS7, 0);
	__asm volatile("tlbwe");
	if (asid == KERNEL_PID)
		__asm volatile("isync\n\tsync");
	wrtee(msr);
#if 0
	if (asid)
	printf("%s(%#lx,%#x,%#x,%x)->[%x,%x,%x]\n",
	    __func__, va, asid, pte, insert,
	    hwtlb.hwtlb_mas1, hwtlb.hwtlb_mas2, hwtlb.hwtlb_mas3);
#endif
	return (mas1 & MAS1_V) != 0;
}

static void
e500_tlb_write_entry(size_t index, const struct tlbmask *tlb)
{
}

static void
e500_tlb_read_entry(size_t index, struct tlbmask *tlb)
{
}

static void
e500_tlb_dump(void (*pr)(const char *, ...))
{
	const size_t tlbassoc = TLBCFG_ASSOC(mftlb0cfg());
	const size_t tlbentries = TLBCFG_NENTRY(mftlb0cfg());
	const size_t max_epn = (tlbentries / tlbassoc) << PAGE_SHIFT;
	const uint32_t saved_mas0 = mfspr(SPR_MAS0);
	size_t valid = 0;

	if (pr == NULL)
		pr = printf;

	const register_t msr = wrtee(0);
	for (size_t assoc = 0; assoc < tlbassoc; assoc++) {
		struct e500_hwtlb hwtlb;
		hwtlb.hwtlb_mas0 = MAS0_ESEL_MAKE(assoc) | MAS0_TLBSEL_TLB0;
		mtspr(SPR_MAS0, hwtlb.hwtlb_mas0);
		for (size_t epn = 0; epn < max_epn; epn += PAGE_SIZE) {
			mtspr(SPR_MAS2, epn);
			__asm volatile("tlbre");
			hwtlb.hwtlb_mas1 = mfspr(SPR_MAS1);
			/*
			 * If this is a valid entry for AS space 1 and
			 * its asid matches the constraints of the caller,
			 * clear its valid bit.
			 */
			if (hwtlb.hwtlb_mas1 & MAS1_V) {
				hwtlb.hwtlb_mas2 = mfspr(SPR_MAS2);
				hwtlb.hwtlb_mas3 = mfspr(SPR_MAS3);
				struct e500_tlb tlb = hwtlb_to_tlb(hwtlb);
				(*pr)("[%zu,%zu]->[%x,%x,%x]",
				    assoc, atop(epn),
				    hwtlb.hwtlb_mas1, 
				    hwtlb.hwtlb_mas2, 
				    hwtlb.hwtlb_mas3);
				(*pr)(": VA=%#lx size=4KB asid=%u pte=%x",
				    tlb.tlb_va, tlb.tlb_asid, tlb.tlb_pte);
				(*pr)(" (RPN=%#x,%s%s%s%s%s,%s%s%s%s%s)\n",
				    tlb.tlb_pte & PTE_RPN_MASK,
				    tlb.tlb_pte & PTE_xR ? "R" : "",
				    tlb.tlb_pte & PTE_xW ? "W" : "",
				    tlb.tlb_pte & PTE_UNMODIFIED ? "*" : "",
				    tlb.tlb_pte & PTE_xX ? "X" : "",
				    tlb.tlb_pte & PTE_UNSYNCED ? "*" : "",
				    tlb.tlb_pte & PTE_W ? "W" : "",
				    tlb.tlb_pte & PTE_I ? "I" : "",
				    tlb.tlb_pte & PTE_M ? "M" : "",
				    tlb.tlb_pte & PTE_G ? "G" : "",
				    tlb.tlb_pte & PTE_E ? "E" : "");
				valid++;
			}
		}
	}
	mtspr(SPR_MAS0, saved_mas0);
	wrtee(msr);
	(*pr)("%s: %zu valid entries\n", __func__, valid);
}

static void
e500_tlb_walk(void *ctx, bool (*func)(void *, vaddr_t, uint32_t, uint32_t))
{
	const size_t tlbassoc = TLBCFG_ASSOC(mftlb0cfg());
	const size_t tlbentries = TLBCFG_NENTRY(mftlb0cfg());
	const size_t max_epn = (tlbentries / tlbassoc) << PAGE_SHIFT;
	const uint32_t saved_mas0 = mfspr(SPR_MAS0);

	const register_t msr = wrtee(0);
	for (size_t assoc = 0; assoc < tlbassoc; assoc++) {
		struct e500_hwtlb hwtlb;
		hwtlb.hwtlb_mas0 = MAS0_ESEL_MAKE(assoc) | MAS0_TLBSEL_TLB0;
		mtspr(SPR_MAS0, hwtlb.hwtlb_mas0);
		for (size_t epn = 0; epn < max_epn; epn += PAGE_SIZE) {
			mtspr(SPR_MAS2, epn);
			__asm volatile("tlbre");
			hwtlb.hwtlb_mas1 = mfspr(SPR_MAS1);
			if (hwtlb.hwtlb_mas1 & MAS1_V) {
				hwtlb.hwtlb_mas2 = mfspr(SPR_MAS2);
				hwtlb.hwtlb_mas3 = mfspr(SPR_MAS3);
				struct e500_tlb tlb = hwtlb_to_tlb(hwtlb);
				if (!(*func)(ctx, tlb.tlb_va, tlb.tlb_asid,
				    tlb.tlb_pte))
					break;
			}
		}
	}
	mtspr(SPR_MAS0, saved_mas0);
	wrtee(msr);
}

static struct e500_xtlb *
e500_tlb_lookup_xtlb_pa(vaddr_t pa, u_int *slotp)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	struct e500_xtlb *xtlb = tlb1->tlb1_entries;

	/*
	 * See if we have a TLB entry for the pa.
	 */
	for (u_int i = 0; i < tlb1->tlb1_numentries; i++, xtlb++) {
		psize_t mask = ~(xtlb->e_tlb.tlb_size - 1);
		if ((xtlb->e_hwtlb.hwtlb_mas1 & MAS1_V)
		    && ((pa ^ xtlb->e_tlb.tlb_pte) & mask) == 0) {
			if (slotp != NULL)
				*slotp = i;
			return xtlb;
		}
	}

	return NULL;
}

struct e500_xtlb *
e500_tlb_lookup_xtlb(vaddr_t va, u_int *slotp)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	struct e500_xtlb *xtlb = tlb1->tlb1_entries;

	/*
	 * See if we have a TLB entry for the va.
	 */
	for (u_int i = 0; i < tlb1->tlb1_numentries; i++, xtlb++) {
		vsize_t mask = ~(xtlb->e_tlb.tlb_size - 1);
		if ((xtlb->e_hwtlb.hwtlb_mas1 & MAS1_V)
		    && ((va ^ xtlb->e_tlb.tlb_va) & mask) == 0) {
			if (slotp != NULL)
				*slotp = i;
			return xtlb;
		}
	}

	return NULL;
}

static struct e500_xtlb *
e500_tlb_lookup_xtlb2(vaddr_t va, vsize_t len)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	struct e500_xtlb *xtlb = tlb1->tlb1_entries;

	/*
	 * See if we have a TLB entry for the pa.
	 */
	for (u_int i = 0; i < tlb1->tlb1_numentries; i++, xtlb++) {
		vsize_t mask = ~(xtlb->e_tlb.tlb_size - 1);
		if ((xtlb->e_hwtlb.hwtlb_mas1 & MAS1_V)
		    && ((va ^ xtlb->e_tlb.tlb_va) & mask) == 0
		    && (((va + len - 1) ^ va) & mask) == 0) {
			return xtlb;
		}
	}

	return NULL;
}

static void *
e500_tlb_mapiodev(paddr_t pa, psize_t len, bool prefetchable)
{
	struct e500_xtlb * const xtlb = e500_tlb_lookup_xtlb_pa(pa, NULL);

	/*
	 * See if we have a TLB entry for the pa.  If completely falls within
	 * mark the reference and return the pa.  But only if the tlb entry
	 * is not cacheable.
	 */
	if (xtlb
	    && (prefetchable
		|| (xtlb->e_tlb.tlb_pte & PTE_WIG) == (PTE_I|PTE_G))) {
		xtlb->e_refcnt++;
		return (void *) (xtlb->e_tlb.tlb_va
		    + pa - (xtlb->e_tlb.tlb_pte & PTE_RPN_MASK));
	}
	return NULL;
}

static void
e500_tlb_unmapiodev(vaddr_t va, vsize_t len)
{
	if (va < VM_MIN_KERNEL_ADDRESS || VM_MAX_KERNEL_ADDRESS <= va) {
		struct e500_xtlb * const xtlb = e500_tlb_lookup_xtlb(va, NULL);
		if (xtlb)
			xtlb->e_refcnt--;
	}
}

static int
e500_tlb_ioreserve(vaddr_t va, vsize_t len, pt_entry_t pte)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	struct e500_xtlb *xtlb;

	KASSERT(len & 0x55555000);
	KASSERT((len & ~0x55555000) == 0);
	KASSERT(len >= PAGE_SIZE);
	KASSERT((len & (len - 1)) == 0);
	KASSERT((va & (len - 1)) == 0);
	KASSERT(((pte & PTE_RPN_MASK) & (len - 1)) == 0);

	if ((xtlb = e500_tlb_lookup_xtlb2(va, len)) != NULL) {
		psize_t mask = ~(xtlb->e_tlb.tlb_size - 1);
		KASSERT(len <= xtlb->e_tlb.tlb_size);
		KASSERT((pte & mask) == (xtlb->e_tlb.tlb_pte & mask));
		xtlb->e_refcnt++;
		return 0;
	}

	const int slot = e500_alloc_tlb1_entry();
	if (slot < 0)
		return ENOMEM;

	xtlb = &tlb1->tlb1_entries[slot]; 
	xtlb->e_tlb.tlb_va = va;
	xtlb->e_tlb.tlb_size = len;
	xtlb->e_tlb.tlb_pte = pte;
	xtlb->e_tlb.tlb_asid = KERNEL_PID;

	xtlb->e_hwtlb = tlb_to_hwtlb(xtlb->e_tlb);
	xtlb->e_hwtlb.hwtlb_mas0 |= __SHIFTIN(slot, MAS0_ESEL);
	hwtlb_write(xtlb->e_hwtlb, true);
	return 0;
}

static int
e500_tlb_iorelease(vaddr_t va)
{
	u_int slot;
	struct e500_xtlb * const xtlb = e500_tlb_lookup_xtlb(va, &slot);

	if (xtlb == NULL)
		return ENOENT;

	if (xtlb->e_refcnt)
		return EBUSY;

	e500_free_tlb1_entry(xtlb, slot, true);

	return 0;
}

static u_int
e500_tlbmemmap(paddr_t memstart, psize_t memsize, struct e500_tlb1 *tlb1)
{
	u_int slotmask = 0;
	u_int slots = 0, nextslot = 0;
	KASSERT(tlb1->tlb1_numfree > 1);
	KASSERT(((memstart + memsize - 1) & -memsize) == memstart);
	for (paddr_t lastaddr = memstart; 0 < memsize; ) {
		u_int cnt = __builtin_clz(memsize);
		psize_t size = min(1UL << (31 - (cnt | 1)), tlb1->tlb1_maxsize);
		slots += memsize / size;
		if (slots > 4)
			panic("%s: %d: can't map memory (%#lx) into TLB1: %s",
			    __func__, __LINE__, memsize, "too fragmented");
		if (slots > tlb1->tlb1_numfree - 1)
			panic("%s: %d: can't map memory (%#lx) into TLB1: %s",
			    __func__, __LINE__, memsize,
			    "insufficent TLB entries");
		for (; nextslot < slots; nextslot++) {
			const u_int freeslot = e500_alloc_tlb1_entry();
			struct e500_xtlb * const xtlb =
			    &tlb1->tlb1_entries[freeslot];
			xtlb->e_tlb.tlb_asid = KERNEL_PID;
			xtlb->e_tlb.tlb_size = size;
			xtlb->e_tlb.tlb_va = lastaddr;
			xtlb->e_tlb.tlb_pte = lastaddr
			    | PTE_M | PTE_xX | PTE_xW | PTE_xR;
			lastaddr += size;
			memsize -= size;
			slotmask |= 1 << (31 - freeslot); /* clz friendly */
		}
	}

	return nextslot;
}
static const struct tlb_md_ops e500_tlb_ops = {
	.md_tlb_get_asid = e500_tlb_get_asid,
	.md_tlb_set_asid = e500_tlb_set_asid,
	.md_tlb_invalidate_all = e500_tlb_invalidate_all,
	.md_tlb_invalidate_globals = e500_tlb_invalidate_globals,
	.md_tlb_invalidate_asids = e500_tlb_invalidate_asids,
	.md_tlb_invalidate_addr = e500_tlb_invalidate_addr,
	.md_tlb_update_addr = e500_tlb_update_addr,
	.md_tlb_record_asids = e500_tlb_record_asids,
	.md_tlb_write_entry = e500_tlb_write_entry,
	.md_tlb_read_entry = e500_tlb_read_entry,
	.md_tlb_dump = e500_tlb_dump,
	.md_tlb_walk = e500_tlb_walk,
};

static const struct tlb_md_io_ops e500_tlb_io_ops = {
	.md_tlb_mapiodev = e500_tlb_mapiodev,
	.md_tlb_unmapiodev = e500_tlb_unmapiodev,
	.md_tlb_ioreserve = e500_tlb_ioreserve,
	.md_tlb_iorelease = e500_tlb_iorelease,
};

void
e500_tlb_init(vaddr_t endkernel, psize_t memsize)
{
	struct e500_tlb1 * const tlb1 = &e500_tlb1;

#if 0
	register_t mmucfg = mfspr(SPR_MMUCFG);
	register_t mas4 = mfspr(SPR_MAS4);
#endif

	const uint32_t tlb1cfg = mftlb1cfg();
	tlb1->tlb1_numentries = TLBCFG_NENTRY(tlb1cfg);
	KASSERT(tlb1->tlb1_numentries <= __arraycount(tlb1->tlb1_entries));
	/*
	 * Limit maxsize to 1G since 4G isn't really useful to us.
	 */
	tlb1->tlb1_minsize = 1024 << (2 * TLBCFG_MINSIZE(tlb1cfg));
	tlb1->tlb1_maxsize = 1024 << (2 * min(10, TLBCFG_MAXSIZE(tlb1cfg)));

#ifdef VERBOSE_INITPPC
	printf(" tlb1cfg=%#x numentries=%u minsize=%#xKB maxsize=%#xKB",
	    tlb1cfg, tlb1->tlb1_numentries, tlb1->tlb1_minsize >> 10,
	    tlb1->tlb1_maxsize >> 10);
#endif

	/*
	 * Let's see what's in TLB1 and we need to invalidate any entry that
	 * would fit within the kernel's mapped address space.
	 */
	psize_t memmapped = 0;
	for (u_int i = 0; i < tlb1->tlb1_numentries; i++) {
		struct e500_xtlb * const xtlb = &tlb1->tlb1_entries[i];

		xtlb->e_hwtlb = hwtlb_read(MAS0_TLBSEL_TLB1, i);

		if ((xtlb->e_hwtlb.hwtlb_mas1 & MAS1_V) == 0) {
			tlb1->tlb1_freelist[tlb1->tlb1_numfree++] = i;
#ifdef VERBOSE_INITPPC
			printf(" TLB1[%u]=<unused>", i);
#endif
			continue;
		}

		xtlb->e_tlb = hwtlb_to_tlb(xtlb->e_hwtlb);
#ifdef VERBOSE_INITPPC
		printf(" TLB1[%u]=<%#lx,%#lx,%#x,%#x>",
		    i, xtlb->e_tlb.tlb_va, xtlb->e_tlb.tlb_size,
		    xtlb->e_tlb.tlb_asid, xtlb->e_tlb.tlb_pte);
#endif
		if ((VM_MIN_KERNEL_ADDRESS <= xtlb->e_tlb.tlb_va
		    && xtlb->e_tlb.tlb_va < VM_MAX_KERNEL_ADDRESS)
		    || (xtlb->e_tlb.tlb_va < VM_MIN_KERNEL_ADDRESS
		        && VM_MIN_KERNEL_ADDRESS <
			   xtlb->e_tlb.tlb_va + xtlb->e_tlb.tlb_size)) {
#ifdef VERBOSE_INITPPC
			printf("free");
#endif
			e500_free_tlb1_entry(xtlb, i, false);
#ifdef VERBOSE_INITPPC
			printf("d");
#endif
			continue;
		}
		if ((xtlb->e_hwtlb.hwtlb_mas1 & MAS1_IPROT) == 0) {
			xtlb->e_hwtlb.hwtlb_mas1 |= MAS1_IPROT;
			hwtlb_write(xtlb->e_hwtlb, false);
#ifdef VERBOSE_INITPPC
			printf("+iprot");
#endif
		}
		if (xtlb->e_tlb.tlb_pte & PTE_I)
			continue;

		if (xtlb->e_tlb.tlb_va == 0
		    || xtlb->e_tlb.tlb_va + xtlb->e_tlb.tlb_size <= memsize) {
			memmapped += xtlb->e_tlb.tlb_size;
			/*
			 * Let make sure main memory is setup so it's memory
			 * coherent.  For some reason u-boot doesn't set it up
			 * that way.
			 */
			if ((xtlb->e_hwtlb.hwtlb_mas2 & MAS2_M) == 0) {
				xtlb->e_hwtlb.hwtlb_mas2 |= MAS2_M;
				hwtlb_write(xtlb->e_hwtlb, true);
			}
		}
	}

	cpu_md_ops.md_tlb_ops = &e500_tlb_ops;
	cpu_md_ops.md_tlb_io_ops = &e500_tlb_io_ops;

	if (__predict_false(memmapped < memsize)) {
		/*
		 * Let's see how many TLB entries are needed to map memory.
		 */
		u_int slotmask = e500_tlbmemmap(0, memsize, tlb1);

		/*
		 * To map main memory into the TLB, we need to flush any
		 * existing entries from the TLB that overlap the virtual
		 * address space needed to map physical memory.  That may
		 * include the entries for the pages currently used by the
		 * stack or that we are executing.  So to avoid problems, we
		 * are going to temporarily map the kernel and stack into AS 1,
		 * switch to it, and clear out the TLB entries from AS 0,
		 * install the new TLB entries to map memory, and then switch
		 * back to AS 0 and free the temp entry used for AS1.
		 */
		u_int b = __builtin_clz(endkernel);

		/*
		 * If the kernel doesn't end on a clean power of 2, we need
		 * to round the size up (by decrementing the number of leading
		 * zero bits).  If the size isn't a power of 4KB, decrement
		 * again to make it one.
		 */
		if (endkernel & (endkernel - 1))
			b--;
		if ((b & 1) == 0)
			b--;

		/*
		 * Create a TLB1 mapping for the kernel in AS1.
		 */
		const u_int kslot = e500_alloc_tlb1_entry();
		struct e500_xtlb * const kxtlb = &tlb1->tlb1_entries[kslot];
		kxtlb->e_tlb.tlb_va = 0;
		kxtlb->e_tlb.tlb_size = 1UL << (31 - b);
		kxtlb->e_tlb.tlb_pte = PTE_M|PTE_xR|PTE_xW|PTE_xX;
		kxtlb->e_tlb.tlb_asid = KERNEL_PID;

		kxtlb->e_hwtlb = tlb_to_hwtlb(kxtlb->e_tlb);
		kxtlb->e_hwtlb.hwtlb_mas0 |= __SHIFTIN(kslot, MAS0_ESEL);
		kxtlb->e_hwtlb.hwtlb_mas1 |= MAS1_TS;
		hwtlb_write(kxtlb->e_hwtlb, true);

		/*
		 * Now that we have a TLB mapping in AS1 for the kernel and its
		 * stack, we switch to AS1 to cleanup the TLB mappings for TLB0.
		 */
		const register_t saved_msr = mfmsr();
		mtmsr(saved_msr | PSL_DS | PSL_IS);
		__asm volatile("isync");

		/*
		 *** Invalidate all the TLB0 entries.
		 */
		e500_tlb_invalidate_all();

		/*
		 *** Now let's see if we have any entries in TLB1 that would
		 *** overlap the ones we are about to install.  If so, nuke 'em.
		 */
		for (u_int i = 0; i < tlb1->tlb1_numentries; i++) {
			struct e500_xtlb * const xtlb = &tlb1->tlb1_entries[i];
			struct e500_hwtlb * const hwtlb = &xtlb->e_hwtlb;
			if ((hwtlb->hwtlb_mas1 & (MAS1_V|MAS1_TS)) == MAS1_V
			    && (hwtlb->hwtlb_mas2 & MAS2_EPN) < memsize) {
				e500_free_tlb1_entry(xtlb, i, false);
			}
		}

		/*
		 *** Now we can add the TLB entries that will map physical
		 *** memory.  If bit 0 [MSB] in slotmask is set, then tlb
		 *** entry 0 contains a mapping for physical memory...
		 */
		struct e500_xtlb *entries = tlb1->tlb1_entries;
		while (slotmask != 0) {
			const u_int slot = __builtin_clz(slotmask);
			hwtlb_write(entries[slot].e_hwtlb, false);
			entries += slot + 1;
			slotmask <<= slot + 1;
		}

		/*
		 *** Synchronize the TLB and the instruction stream.
		 */
		__asm volatile("tlbsync");
		__asm volatile("isync");

		/*
		 *** Switch back to AS 0.
		 */
		mtmsr(saved_msr);
		__asm volatile("isync");

		/*
		 * Free the temporary TLB1 entry.
		 */
		e500_free_tlb1_entry(kxtlb, kslot, true);
	}

	/*
	 * Finally set the MAS4 defaults.
	 */
	mtspr(SPR_MAS4, MAS4_TSIZED_4KB | MAS4_MD);

	/*
	 * Invalidate all the TLB0 entries.
	 */
	e500_tlb_invalidate_all();
}

void
e500_tlb_minimize(vaddr_t endkernel)
{
#ifdef PMAP_MINIMALTLB
	struct e500_tlb1 * const tlb1 = &e500_tlb1;
	extern uint32_t _fdata[];

	u_int slot;

	paddr_t boot_page = cpu_read_4(GUR_BPTR);
	if (boot_page & BPTR_EN) {
		/*
		 * shift it to an address
		 */
		boot_page = (boot_page & BPTR_BOOT_PAGE) << PAGE_SHIFT;
		pmap_kvptefill(boot_page, boot_page + NBPG,
		    PTE_M | PTE_xR | PTE_xW | PTE_xX);
	}


	KASSERT(endkernel - (uintptr_t)_fdata < 0x400000);
	KASSERT((uintptr_t)_fdata == 0x400000);

	struct e500_xtlb *xtlb = e500_tlb_lookup_xtlb(endkernel, &slot);

	KASSERT(xtlb == e500_tlb_lookup_xtlb2(0, endkernel));
	const u_int tmp_slot = e500_alloc_tlb1_entry();
	KASSERT(tmp_slot != (u_int) -1);

	struct e500_xtlb * const tmp_xtlb = &tlb1->tlb1_entries[tmp_slot];
	tmp_xtlb->e_tlb = xtlb->e_tlb;
	tmp_xtlb->e_hwtlb = tlb_to_hwtlb(tmp_xtlb->e_tlb);
	tmp_xtlb->e_hwtlb.hwtlb_mas1 |= MAS1_TS;
	KASSERT((tmp_xtlb->e_hwtlb.hwtlb_mas0 & MAS0_TLBSEL) == MAS0_TLBSEL_TLB1);
	tmp_xtlb->e_hwtlb.hwtlb_mas0 |= __SHIFTIN(tmp_slot, MAS0_ESEL);
	hwtlb_write(tmp_xtlb->e_hwtlb, true);

	const u_int text_slot = e500_alloc_tlb1_entry();
	KASSERT(text_slot != (u_int)-1);
	struct e500_xtlb * const text_xtlb = &tlb1->tlb1_entries[text_slot];
	text_xtlb->e_tlb.tlb_va = 0;
	text_xtlb->e_tlb.tlb_size = 0x400000;
	text_xtlb->e_tlb.tlb_pte = PTE_M | PTE_xR | PTE_xX | text_xtlb->e_tlb.tlb_va;
	text_xtlb->e_tlb.tlb_asid = 0;
	text_xtlb->e_hwtlb = tlb_to_hwtlb(text_xtlb->e_tlb);
	KASSERT((text_xtlb->e_hwtlb.hwtlb_mas0 & MAS0_TLBSEL) == MAS0_TLBSEL_TLB1);
	text_xtlb->e_hwtlb.hwtlb_mas0 |= __SHIFTIN(text_slot, MAS0_ESEL);

	const u_int data_slot = e500_alloc_tlb1_entry();
	KASSERT(data_slot != (u_int)-1);
	struct e500_xtlb * const data_xtlb = &tlb1->tlb1_entries[data_slot];
	data_xtlb->e_tlb.tlb_va = 0x400000;
	data_xtlb->e_tlb.tlb_size = 0x400000;
	data_xtlb->e_tlb.tlb_pte = PTE_M | PTE_xR | PTE_xW | data_xtlb->e_tlb.tlb_va;
	data_xtlb->e_tlb.tlb_asid = 0;
	data_xtlb->e_hwtlb = tlb_to_hwtlb(data_xtlb->e_tlb);
	KASSERT((data_xtlb->e_hwtlb.hwtlb_mas0 & MAS0_TLBSEL) == MAS0_TLBSEL_TLB1);
	data_xtlb->e_hwtlb.hwtlb_mas0 |= __SHIFTIN(data_slot, MAS0_ESEL);

	const register_t msr = mfmsr();
	const register_t ts_msr = (msr | PSL_DS | PSL_IS) & ~PSL_EE;

	__asm __volatile(
		"mtmsr	%[ts_msr]"	"\n\t"
		"sync"			"\n\t"
		"isync"
	    ::	[ts_msr] "r" (ts_msr));

#if 0
	hwtlb_write(text_xtlb->e_hwtlb, false);
	hwtlb_write(data_xtlb->e_hwtlb, false);
	e500_free_tlb1_entry(xtlb, slot, true);
#endif

	__asm __volatile(
		"mtmsr	%[msr]"		"\n\t"
		"sync"			"\n\t"
		"isync"
	    ::	[msr] "r" (msr));

	e500_free_tlb1_entry(tmp_xtlb, tmp_slot, true);
#endif	/* PMAP_MINIMALTLB */
}
