/*	$NetBSD: mmu_sh4.c,v 1.1 2002/02/17 20:55:57 uch Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/cpufunc.h>
#include <sh3/mmu.h>
#include <sh3/mmu_sh4.h>

#define SH4_MMU_HAZARD	__asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;")

extern __inline__ void __sh4_itlb_invalidate_all(void);

void
__sh4_itlb_invalidate_all()
{

	_reg_write_4(SH4_ITLB_AA, 0);
	_reg_write_4(SH4_ITLB_AA | (1 << SH4_ITLB_E_SHIFT), 0);
	_reg_write_4(SH4_ITLB_AA | (2 << SH4_ITLB_E_SHIFT), 0);
	_reg_write_4(SH4_ITLB_AA | (3 << SH4_ITLB_E_SHIFT), 0);
}

void
sh4_mmu_start()
{
	
	/* Zero clear all TLB entry */
	_reg_write_4(SH4_MMUCR, 0);	/* zero wired entry */
	sh_tlb_invalidate_all();

	/* Set current ASID to 0 */
	sh_tlb_set_asid(0);

	/* Single virtual memory mode. User can't access store queue */
	_reg_write_4(SH4_MMUCR, SH4_MMUCR_AT | SH4_MMUCR_TI | SH4_MMUCR_SV |
	    SH4_MMUCR_SQMD);

	SH4_MMU_HAZARD;
}

void
sh4_tlb_invalidate_addr(int asid, vaddr_t va)
{
	u_int32_t pteh;
	va &= SH4_PTEH_VPN_MASK;

	/* Save current ASID */
	pteh = _reg_read_4(SH4_PTEH);
	/* Set ASID for associative write */
	_reg_write_4(SH4_PTEH, asid);

	/* Associative write(UTLB/ITLB). not required ITLB invalidate. */
	RUN_P2;	
	_reg_write_4(SH4_UTLB_AA | SH4_UTLB_A, va); /* Clear D, V */
	RUN_P1;
	/* Restore ASID */
	_reg_write_4(SH4_PTEH, pteh);
}
 
void
sh4_tlb_invalidate_asid(int asid)
{
	u_int32_t a;
	int e;
	
	/* Invalidate entry attribute to ASID */
	RUN_P2;
	for (e = 0; e < SH4_UTLB_ENTRY; e++) {
		a = SH4_UTLB_AA | (e << SH4_UTLB_E_SHIFT);
		if ((_reg_read_4(a) & SH4_UTLB_AA_ASID_MASK) == asid)
			_reg_write_4(a, 0);
	}

	__sh4_itlb_invalidate_all();
	RUN_P1;
}

void
sh4_tlb_invalidate_all()
{
	u_int32_t a;
	int e, eend;
	
	/* If non-wired entry limit is zero, clear all entry. */
	a = _reg_read_4(SH4_MMUCR) & SH4_MMUCR_URB_MASK;
	eend = a ? (a >> SH4_MMUCR_URB_SHIFT) : SH4_UTLB_ENTRY;
	
	RUN_P2;
	for (e = 0; e < eend; e++) {
		a = SH4_UTLB_AA | (e << SH4_UTLB_E_SHIFT);
		_reg_write_4(a, 0);
	}
	__sh4_itlb_invalidate_all();
	RUN_P1;
}

void
sh4_tlb_reset()
{
	/* 
	 * SH4 MMUCR reserved bit 
	 *   read:  unknown.
	 *   write: must be 0.
	 */
	_reg_write_4(SH4_MMUCR,
	    (_reg_read_4(SH4_MMUCR) & SH4_MMUCR_MASK) | SH4_MMUCR_TI);

	SH4_MMU_HAZARD;
}
