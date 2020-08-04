/*	$NetBSD: mmu_sh4.c,v 1.21 2020/08/04 01:55:16 uwe Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mmu_sh4.c,v 1.21 2020/08/04 01:55:16 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/pte.h>	/* NetBSD/sh3 specific PTE */
#include <sh3/mmu.h>
#include <sh3/mmu_sh4.h>

static __noinline void do_tlb_assoc(uint32_t);
static __noinline void do_invalidate_asid(int);
static __noinline void do_invalidate_all(uint32_t);

#define	SH4_MMU_HAZARD	__asm volatile("nop;nop;nop;nop;nop;nop;nop;nop;")


void
sh4_mmu_start(void)
{
	uint32_t cr;

	/* Zero clear all TLB entry */
	_reg_write_4(SH4_MMUCR, 0);	/* zero wired entry */
	sh4_tlb_invalidate_all();

	/* Set current ASID to 0 */
	sh_tlb_set_asid(0);

	cr  = SH4_MMUCR_AT;	/* address translation enabled */
	cr |= SH4_MMUCR_TI;	/* TLB invalidate */
	cr |= SH4_MMUCR_SQMD;	/* store queues not accessible to user */

	/* resereve TLB entries for wired u-area (cf. sh4_switch_resume) */
	cr |= (SH4_UTLB_ENTRY - UPAGES) << SH4_MMUCR_URB_SHIFT;

	_reg_write_4(SH4_MMUCR, cr);
	SH4_MMU_HAZARD;
}


/*
 * Perform associative write to UTLB.  Must be called via its P2
 * address.  Note, the ASID match is against PTEH, not "va".  The
 * caller is responsible for saving/setting/restoring PTEH.
 */
static __noinline void
do_tlb_assoc(uint32_t va)
{

	_reg_write_4(SH4_UTLB_AA | SH4_UTLB_A, va);
	PAD_P1_SWITCH;
}


void
sh4_tlb_invalidate_addr(int asid, vaddr_t va)
{
	void (*tlb_assoc_p2)(uint32_t);
	uint32_t opteh;
	uint32_t sr;

	tlb_assoc_p2 = SH3_P2SEG_FUNC(do_tlb_assoc);

	va &= SH4_UTLB_AA_VPN_MASK;

	sr = _cpu_exception_suspend();
	opteh = _reg_read_4(SH4_PTEH); /* save current ASID */

	_reg_write_4(SH4_PTEH, asid); /* set ASID for associative write */
	(*tlb_assoc_p2)(va); /* invalidate { va, ASID } entry if exists */

	_reg_write_4(SH4_PTEH, opteh); /* restore ASID */
	_cpu_set_sr(sr);
}


static __noinline void
do_invalidate_asid(int asid)
{
	int e;

	for (e = 0; e < SH4_UTLB_ENTRY; ++e) {
		uint32_t addr = SH4_UTLB_AA | (e << SH4_UTLB_E_SHIFT);
		uint32_t aa = _reg_read_4(addr);
		if ((aa & SH4_UTLB_AA_ASID_MASK) == asid)
			_reg_write_4(addr, 0);
	}

	for (e = 0; e < SH4_ITLB_ENTRY; ++e) {
		uint32_t addr = SH4_ITLB_AA | (e << SH4_UTLB_E_SHIFT);
		uint32_t aa = _reg_read_4(addr);
		if ((aa & SH4_ITLB_AA_ASID_MASK) == asid)
			_reg_write_4(addr, 0);
	}

	PAD_P1_SWITCH;
}


void
sh4_tlb_invalidate_asid(int asid)
{
	void (*invalidate_asid_p2)(int);
	uint32_t sr;

	KDASSERT(asid != 0);

	invalidate_asid_p2 = SH3_P2SEG_FUNC(do_invalidate_asid);

	sr = _cpu_exception_suspend();
	(*invalidate_asid_p2)(asid);

	_cpu_set_sr(sr);
}


static __noinline void
do_invalidate_all(uint32_t limit)
{
	int e;

	for (e = 0; e < limit; ++e) {
		_reg_write_4(SH4_UTLB_AA  | (e << SH4_UTLB_E_SHIFT), 0);
		_reg_write_4(SH4_UTLB_DA1 | (e << SH4_UTLB_E_SHIFT), 0);
	}

	for (e = 0; e < SH4_ITLB_ENTRY; ++e) {
		_reg_write_4(SH4_ITLB_AA  | (e << SH4_ITLB_E_SHIFT), 0);
		_reg_write_4(SH4_ITLB_DA1 | (e << SH4_ITLB_E_SHIFT), 0);
	}

	PAD_P1_SWITCH;
}


void
sh4_tlb_invalidate_all(void)
{
	void (*invalidate_all_p2)(uint32_t);
	uint32_t limit;
	uint32_t sr;

	invalidate_all_p2 = SH3_P2SEG_FUNC(do_invalidate_all);

	/* do we resereve TLB entries for wired u-area? */
	limit = _reg_read_4(SH4_MMUCR) & SH4_MMUCR_URB_MASK;
	limit = limit ? (limit >> SH4_MMUCR_URB_SHIFT) : SH4_UTLB_ENTRY;

	sr = _cpu_exception_suspend();
	(*invalidate_all_p2)(limit);

	_cpu_set_sr(sr);
}


void
sh4_tlb_update(int asid, vaddr_t va, uint32_t pte)
{
	void (*tlb_assoc_p2)(uint32_t);
	uint32_t opteh, ptel;
	uint32_t sr;

	KDASSERT(asid < 0x100);
	KDASSERT(va != 0);
	KDASSERT((pte & ~PGOFSET) != 0);

	tlb_assoc_p2 = SH3_P2SEG_FUNC(do_tlb_assoc);

	sr = _cpu_exception_suspend();
	opteh = _reg_read_4(SH4_PTEH); /* save old ASID */

	/*
	 * Invalidate { va, ASID } entry if exists.  Only ASID is
	 * matched in PTEH, but set the va part too for ldtlb below.
	 */
	_reg_write_4(SH4_PTEH, (va & ~PGOFSET) | asid);
	(*tlb_assoc_p2)(va & SH4_UTLB_AA_VPN_MASK);

	/* Load new entry (PTEH is already set) */
	ptel = pte & PG_HW_BITS;
	_reg_write_4(SH4_PTEL, ptel);
	if (pte & _PG_PCMCIA) {
		_reg_write_4(SH4_PTEA,
		    (pte >> _PG_PCMCIA_SHIFT) & SH4_PTEA_SA_MASK);
	} else {
		_reg_write_4(SH4_PTEA, 0);
	}
	__asm volatile("ldtlb; nop");

	_reg_write_4(SH4_PTEH, opteh); /* restore old ASID */
	_cpu_set_sr(sr);
}
