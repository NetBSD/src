/*	$NetBSD: mmu.c,v 1.3 2002/02/28 01:56:59 uch Exp $	*/

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
#include <sh3/pte.h>
#include <sh3/cache.h>
#include <sh3/mmu.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

/* Start MMU. call after exception vector is setuped. */
void (*__sh_mmu_start)(void);

/* TLB functions */
void (*__sh_tlb_invalidate_addr)(int, vaddr_t);
void (*__sh_tlb_invalidate_asid)(int);
void (*__sh_tlb_invalidate_all)(void);
void (*__sh_tlb_reset)(void);

/* TTB access */
u_int32_t (*__sh_mmu_ttb_read)(void);
void (*__sh_mmu_ttb_write)(u_int32_t);

/* PTEL, PTEA access */
void (*__sh_mmu_pte_setup)(vaddr_t, u_int32_t);

/* Page table method (software) */
vaddr_t sh3_mmu_pt_p1addr(vaddr_t);
vaddr_t sh4_mmu_pt_p2addr(vaddr_t);
vaddr_t (*__sh_mmu_pt_kaddr)(vaddr_t);
u_int32_t (*__sh_mmu_pd_area)(u_int32_t);


void
sh_mmu_init()
{

	/* 
	 * Assing function hook. but if only defined SH3 or SH4, it is called
	 * directly. see sh3/mmu.h
	 */
	if (CPU_IS_SH3) {
		__sh_mmu_start = sh3_mmu_start;
		__sh_tlb_invalidate_addr = sh3_tlb_invalidate_addr;
		__sh_tlb_invalidate_asid = sh3_tlb_invalidate_asid;
		__sh_tlb_invalidate_all = sh3_tlb_invalidate_all;
		__sh_tlb_reset = sh3_tlb_reset;
		__sh_mmu_pt_kaddr = sh3_mmu_pt_p1addr;
		__sh_mmu_pte_setup = sh3_mmu_pte_setup;
		__sh_mmu_ttb_read = sh3_mmu_ttb_read;
		__sh_mmu_ttb_write = sh3_mmu_ttb_write;
		__sh_mmu_pd_area = __sh3_mmu_pd_area;
	}

	if (CPU_IS_SH4) {
		__sh_mmu_start = sh4_mmu_start;
		__sh_tlb_invalidate_addr = sh4_tlb_invalidate_addr;
		__sh_tlb_invalidate_asid = sh4_tlb_invalidate_asid;
		__sh_tlb_invalidate_all = sh4_tlb_invalidate_all;
		__sh_tlb_reset = sh4_tlb_reset;
		__sh_mmu_pt_kaddr = sh4_mmu_pt_p2addr;
		__sh_mmu_pte_setup = sh4_mmu_pte_setup;
		__sh_mmu_ttb_read = sh4_mmu_ttb_read;
		__sh_mmu_ttb_write = sh4_mmu_ttb_write;
		__sh_mmu_pd_area = __sh4_mmu_pd_area;
	}

}

void
sh_mmu_information()
{
	u_int32_t r;

	if (CPU_IS_SH3) {
		printf("4-way set-associative 128 TLB entries\n");
		r = _reg_read_4(SH3_MMUCR);
		printf("%s mode, %s virtual storage mode\n",
		    r & SH3_MMUCR_IX
		    ? "ASID+VPN" : "VPN",
		    r & SH3_MMUCR_SV ? "single" : "multiple");
	}

	if (CPU_IS_SH4) {
		printf("full-associative 4 ITLB, 64 UTLB entries\n");
		r = _reg_read_4(SH4_MMUCR);
		printf("%s virtual storage mode, SQ access: kernel%s, ",
		    r & SH3_MMUCR_SV ? "single" : "multiple",
		    r & SH4_MMUCR_SQMD ? "" : "/user");
		printf("wired %d\n", (r & SH4_MMUCR_URB_MASK) >>
		    SH4_MMUCR_URB_SHIFT);
	}
}

void
sh_tlb_set_asid(int asid)
{
	
	_reg_write_4(SH_(PTEH), 0);
}

#ifdef SH3
u_int32_t
__sh3_mmu_pd_area(u_int32_t a)
{

	return (a);
}
#endif /* SH3 */
#ifdef SH4
u_int32_t
__sh4_mmu_pd_area(u_int32_t a)
{

	return (SH3_P1SEG_TO_P2SEG(a));
}
#endif /* SH4 */

/*
 * TTB
 */
#ifdef SH3
u_int32_t
sh3_mmu_ttb_read()
{

	return (_reg_read_4(SH3_TTB));
}

void
sh3_mmu_ttb_write(u_int32_t r)
{

	_reg_write_4(SH3_TTB, r);
	sh_tlb_invalidate_all();
}
#endif /* SH3 */
#ifdef SH4
u_int32_t
sh4_mmu_ttb_read()
{

	return (_reg_read_4(SH4_TTB));
}

void
sh4_mmu_ttb_write(u_int32_t r)
{

	_reg_write_4(SH4_TTB, r);
	sh_tlb_invalidate_all();
}
#endif /* SH4 */

/*
 * Load PTEL, PTEA utility
 */
#ifdef SH3
void
sh3_mmu_pte_setup(vaddr_t va, u_int32_t pte)
{

	_reg_write_4(SH3_PTEL, pte & PG_HW_BITS);
}
#endif /* SH3 */
#ifdef SH4
void
sh4_mmu_pte_setup(vaddr_t va, u_int32_t pte)
{
	u_int32_t ptel;

	ptel = pte & PG_HW_BITS;

	if (pte & _PG_PCMCIA) {
		_reg_write_4(SH4_PTEA,
		    (pte >> _PG_PCMCIA_SHIFT) & SH4_PTEA_SA_MASK);
		_reg_write_4(SH4_PTEL, ptel & ~PG_N);
	} else {
		if (va >= SH3_P1SEG_BASE)
			ptel |= PG_WT;	/* P3SEG is always write-through */

		_reg_write_4(SH4_PTEA, 0);
		_reg_write_4(SH4_PTEL, ptel);
	}
}
#endif /* SH4 */

/*
 * Page table utility. will be obsoleted.
 */
#ifdef SH3
/*
 * returns P1 address of U0, P0, P1 address.
 */
vaddr_t
sh3_mmu_pt_p1addr(vaddr_t va)	/* va = U0, P0, P1 */
{
	u_int32_t *pd, *pde, pte;
	vaddr_t p1addr;

	/* P1SEG */
	if ((va & 0xc0000000) == 0x80000000)
		return (va);

	/* P0/U0SEG */
	pd = (u_int32_t *)_reg_read_4(SH3_TTB);
	pde = (u_int32_t *)(pd[va >> PDSHIFT] & PG_FRAME);
	pte = pde[(va & PT_MASK) >> PGSHIFT];
	p1addr = (pte & PG_FRAME) | (va & PGOFSET);

	return (p1addr);
}
#endif /* SH3 */
#ifdef SH4
/*
 * returns P2 address of U0, P0, P1 address.
 */
vaddr_t
sh4_mmu_pt_p2addr(vaddr_t va)	/* va = U0, P0, P1 */
{
	u_int32_t *pd, *pde, pte;
	vaddr_t p1addr;

	sh_dcache_wbinv_all();

	/* P1SEG */
	if ((va & 0xc0000000) == 0x80000000)
		return SH3_P1SEG_TO_P2SEG(va);

	/* P0/U0SEG */
	pd = (u_int32_t *)SH3_P1SEG_TO_P2SEG(_reg_read_4(SH4_TTB));
	pde = (u_int32_t *)SH3_P1SEG_TO_P2SEG((pd[va >> PDSHIFT] & PG_FRAME));
	pte = pde[(va & PT_MASK) >> PGSHIFT];
	p1addr = (pte & PG_FRAME) | (va & PGOFSET);

	return SH3_P1SEG_TO_P2SEG(p1addr);
}
#endif
