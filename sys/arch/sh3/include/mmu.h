/*	$NetBSD: mmu.h,v 1.4 2002/04/28 17:10:35 uch Exp $	*/

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

#ifndef _SH3_MMU_H_
#define	_SH3_MMU_H_

/*
 * Initialize routines.
 *	sh_mmu_init		Assign function vector, and register addresses.
 *				Don't access hardware.
 *				Call as possible as first.
 *	sh_mmu_start		Reset TLB entry, set default ASID, and start to
 *				translate address.
 *				Call after exception vector was installed.
 *
 * TLB access ops.
 *	sh_tlb_invalidate_addr	invalidate TLB entris for given
 *				virtual addr with ASID.
 *	sh_tlb_invalidate_asid	invalidate TLB entries for given ASID.
 *	sh_tlb_invalidate_all	invalidate all non-wired TLB entries. //sana
 *	sh_tlb_reset		invalidate all TLB entries.
 *	sh_tlb_set_asid		set ASID to PTEH
 *
 * Page table acess ops. (for current NetBSD/sh3 implementation)
 *
 */

extern void sh_mmu_init(void);
extern void sh_mmu_information(void);

extern void (*__sh_mmu_start)(void);
extern void sh3_mmu_start(void);
extern void sh4_mmu_start(void);
#define	sh_mmu_start()			(*__sh_mmu_start)()

extern void sh_tlb_set_asid(int);

extern void (*__sh_tlb_invalidate_addr)(int, vaddr_t);
extern void (*__sh_tlb_invalidate_asid)(int);
extern void (*__sh_tlb_invalidate_all)(void);
extern void (*__sh_tlb_reset)(void);
extern void sh3_tlb_invalidate_addr(int, vaddr_t);
extern void sh3_tlb_invalidate_asid(int);
extern void sh3_tlb_invalidate_all(void);
extern void sh3_tlb_reset(void);
extern void sh4_tlb_invalidate_addr(int, vaddr_t);
extern void sh4_tlb_invalidate_asid(int);
extern void sh4_tlb_invalidate_all(void);
extern void sh4_tlb_reset(void);
#if defined(SH3) && defined(SH4)
#define	sh_tlb_invalidate_addr(a, va)	(*__sh_tlb_invalidate_addr)(a, va)
#define	sh_tlb_invalidate_asid(a)	(*__sh_tlb_invalidate_asid)(a)
#define	sh_tlb_invalidate_all()		(*__sh_tlb_invalidate_all)()
#define	sh_tlb_reset()			(*__sh_tlb_reset)()
#elif defined(SH3)
#define	sh_tlb_invalidate_addr(a, va)	sh3_tlb_invalidate_addr(a, va)
#define	sh_tlb_invalidate_asid(a)	sh3_tlb_invalidate_asid(a)
#define	sh_tlb_invalidate_all()		sh3_tlb_invalidate_all()
#define	sh_tlb_reset()			sh3_tlb_reset()
#elif defined(SH4)
#define	sh_tlb_invalidate_addr(a, va)	sh4_tlb_invalidate_addr(a, va)
#define	sh_tlb_invalidate_asid(a)	sh4_tlb_invalidate_asid(a)
#define	sh_tlb_invalidate_all()		sh4_tlb_invalidate_all()
#define	sh_tlb_reset()			sh4_tlb_reset()
#endif

/*
 * MMU and page table entry access ops.
 */
#if defined(SH3) && defined(SH4)
extern u_int32_t __sh_PTEH;
extern u_int32_t __sh_TTB;
extern u_int32_t __sh_TEA;
#define	SH_PTEH		(*(__volatile__ u_int32_t *)__sh_PTEH)
#define	SH_TTB		(*(__volatile__ u_int32_t *)__sh_TTB)
#define	SH_TEA		(*(__volatile__ u_int32_t *)__sh_TEA)

#elif defined(SH3)
#define	SH_PTEH		(*(__volatile__ u_int32_t *)SH3_PTEH)
#define	SH_TTB		(*(__volatile__ u_int32_t *)SH3_TTB)
#define	SH_TEA		(*(__volatile__ u_int32_t *)SH3_TEA)

#elif defined(SH4)
#define	SH_PTEH		(*(__volatile__ u_int32_t *)SH4_PTEH)
#define	SH_TTB		(*(__volatile__ u_int32_t *)SH4_TTB)
#define	SH_TEA		(*(__volatile__ u_int32_t *)SH4_TEA)
#endif

extern void (*__sh_mmu_pte_setup)(vaddr_t, u_int32_t);
extern void sh3_mmu_pte_setup(vaddr_t, u_int32_t);
extern void sh4_mmu_pte_setup(vaddr_t, u_int32_t);
#if defined(SH3) && defined(SH4)
#define	SH_MMU_PTE_SETUP(v, pte)	(*__sh_mmu_pte_setup)((v), (pte))
#elif defined(SH3)
#define	SH_MMU_PTE_SETUP(v, pte)	sh3_mmu_pte_setup((v), (pte))
#elif defined(SH4)
#define	SH_MMU_PTE_SETUP(v, pte)	sh4_mmu_pte_setup((v), (pte))
#endif

/*
 * SH3 port access pte from P1, SH4 port access it from P2.
 */
extern u_int32_t (*__sh_mmu_pd_area)(u_int32_t);
extern u_int32_t __sh3_mmu_pd_area(u_int32_t);
extern u_int32_t __sh4_mmu_pd_area(u_int32_t);
#if defined(SH3) && defined(SH4)
#define	SH_MMU_PD_AREA(x)		__sh_mmu_pd_area(x)
#elif defined(SH3)
#define	SH_MMU_PD_AREA(x)		(x)
#elif defined(SH4)
#define	SH_MMU_PD_AREA(x)		SH3_P1SEG_TO_P2SEG(x)
#endif

/*
 * TTB stores pte entry start address.
 */
extern u_int32_t (*__sh_mmu_ttb_read)(void);
extern void (*__sh_mmu_ttb_write)(u_int32_t);
extern u_int32_t sh3_mmu_ttb_read(void);
extern void sh3_mmu_ttb_write(u_int32_t);
extern u_int32_t sh4_mmu_ttb_read(void);
extern void sh4_mmu_ttb_write(u_int32_t);
#if defined(SH3) && defined(SH4)
#define	SH_MMU_TTB_READ()	(*__sh_mmu_ttb_read)()
#define	SH_MMU_TTB_WRITE(x)	(*__sh_mmu_ttb_write)(x)
#elif defined(SH3)
#define	SH_MMU_TTB_READ()	sh3_mmu_ttb_read()
#define	SH_MMU_TTB_WRITE(x)	sh3_mmu_ttb_write(x)
#elif defined(SH4)
#define	SH_MMU_TTB_READ()	sh4_mmu_ttb_read()
#define	SH_MMU_TTB_WRITE(x)	sh4_mmu_ttb_write(x)
#endif

/*
 * some macros for pte access.
 */
#define	SH_MMU_PD_TOP()		((u_long *)SH_MMU_PD_AREA(SH_MMU_TTB_READ()))
#define	SH_MMU_PDE(pd, i)	((u_long *)SH_MMU_PD_AREA((pd)[(i)]))

#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>
#endif /* !_SH3_MMU_H_ */
