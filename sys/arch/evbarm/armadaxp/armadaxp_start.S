/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

Developed by Semihalf

********************************************************************************
Marvell BSD License

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
            this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "opt_cputypes.h"

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pmap.h>

RCSID("$NetBSD: armadaxp_start.S,v 1.1 2013/05/29 19:55:56 rkujawa Exp $")

#define	CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define	CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0	/* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */

/*
 * We don't want to hard-code some basic things like RAM start etc.
 * Hence, it is important to set the following options to resanoable values
 * in std.armadaxp configuration file.
 */
#if !defined(STARTUP_PAGETABLE_ADDR)
#error STARTUP_PAGETABLE_ADDR not defined. Please define it in std.armadaxp
#elif !defined(MEMSTART)
#error MEMSTART not defined. Please define it in std.armadaxp
#endif

	.text

	.global	_C_LABEL(armadaxp_start)
_C_LABEL(armadaxp_start):
	/* Move into supervisor mode and disable IRQs/FIQs. */
	mrs	r0, cpsr
	bic	r0, r0, #PSR_MODE
	orr	r0, r0, #(I32_bit | F32_bit)
	msr	cpsr_c, r0

	adr	r7, Lunmapped
	bic	r7, r7, #0xf0000000
	orr	r7, r7, #MEMSTART
disable_mmu:
	/* Disable MMU for a while */
	mrc     p15, 0, r2, c1, c0, 0
	bic	r2, r2, #(CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE |\
	    CPU_CONTROL_WBUF_ENABLE)
	bic	r2, r2, #(CPU_CONTROL_IC_ENABLE)
	bic	r2, r2, #(CPU_CONTROL_BPRD_ENABLE)
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop
	mov	pc, r7
Lunmapped:
	/* build page table from scratch */
	ldr	r0, Lstartup_pagetable
	adr	r4, mmu_init_table
	b	3f

2:
	str	r3, [r0, r2]
	add	r2, r2, #4
	add	r3, r3, #(L1_S_SIZE)
	adds	r1, r1, #-1
	bhi	2b
3:
	ldmia	r4!, {r1,r2,r3}   /* # of sections, VA, PA|attr */
	cmp	r1, #0
	adrne	r5, 2b
	bicne	r5, r5, #0xf0000000
	orrne	r5, r5, #MEMSTART
	movne	pc, r5

	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

	mov	r0, #0
	mcr	p15, 0, r0, c13, c0, 1	/* Set ASID to 0 */

	/* Set the Domain Access register.  Very important! */
	mov	r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #CPU_CONTROL_XP_ENABLE
	orr	r0, r0, #(CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE)
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop
	CPWAIT(r0)

	/* Jump to kernel code in TRUE VA */
	adr	r0, Lstart
	ldr	pc, [r0]
Lstart:
	.word	start
	
	/* NOTREACHED */
Lstartup_pagetable:
	.word	STARTUP_PAGETABLE_ADDR

#define MMU_INIT(va,pa,n_sec,attr) \
	.word	n_sec					; \
	.word	4*((va)>>L1_S_SHIFT)			; \
	.word	(pa)|(attr)				;

mmu_init_table:
	/* fill all table VA==PA */
	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(0, 0, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
	/* map VA 0xc0000000..0xc3ffffff to PA */
	MMU_INIT(0xc0000000, 0, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
	/* 
	 * In case of early start debugging it might be useful to map
	 * SoC registers (for UART access).
	 */
	MMU_INIT(0xd0000000, 0xd0000000, 1, L1_TYPE_S|L1_S_PROTO|L1_S_AP(AP_KRW))
	/* end of table */
	MMU_INIT(0, 0, 0, 0)
