/*	$NetBSD: armadaxp_start.S,v 1.2.2.4 2017/12/03 11:36:02 jdolecek Exp $	*/
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
#include <evbarm/marvell/marvellreg.h>
#include <evbarm/marvell/marvellvar.h>
#include "assym.h"

RCSID("$NetBSD: armadaxp_start.S,v 1.2.2.4 2017/12/03 11:36:02 jdolecek Exp $")

#ifdef KERNEL_BASES_EQUAL
#error KERNEL_BASE_VIRT should not equal KERNEL_BASE_PHYS
#endif

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

	.section .start,"ax",%progbits

	.global	_C_LABEL(armadaxp_start)
_C_LABEL(armadaxp_start):
	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid	if, #PSR_SVC32_MODE

	/* Disable MMU for a while */
	mrc     p15, 0, r2, c1, c0, 0
	movw	r1, #(CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE |\
	    CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_IC_ENABLE |\
	    CPU_CONTROL_BPRD_ENABLE)
	bic	r2, r2, r1
	mcr	p15, 0, r2, c1, c0, 0
	dsb
	isb

	/* Save U-Boot arguments */
	adr	r4, uboot_regs_pa
	stmia	r4!, {r0, r1, r2, r3}

	/* build page table from scratch */
	movw	r0, #:lower16:STARTUP_PAGETABLE_ADDR
	movt	r0, #:upper16:STARTUP_PAGETABLE_ADDR
	adr	r4, mmu_init_table
	b	3f

2:	str	r3, [r0, r2]
	add	r2, r2, #4
	add	r3, r3, #(L1_S_SIZE)
	adds	r1, r1, #-1
	bhi	2b
3:
	ldmia	r4!, {r1,r2,r3}   /* # of sections, VA, PA|attr */
	cmp	r1, #0
	bne	2b

	mcr	p15, 0, r0, c2, c0, 0	// Set TTBR0
#ifdef ARM_MMU_EXTENDED
	mcr	p15, 0, r0, c2, c0, 1	// Set TTBR1
	mov	r0, #TTBCR_S_N_1
#else
	mov	r0, #0
#endif
	mcr	p15, 0, r0, c2, c0, 2	// TTBCR write 

	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

	mcr	p15, 0, r0, c13, c0, 1	// CONTEXTIDR write: Set ASID to 0

	/* Set the Domain Access register.  Very important! */
	mov	r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0	// DACR write

#define CPU_CONTROL_SET (CPU_CONTROL_XP_ENABLE | CPU_CONTROL_IC_ENABLE \
	    | CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE)

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	movw	r1, #:lower16:CPU_CONTROL_SET
#if (CPU_CONTROL_SET & 0xffff) != 0
	movt	r1, #:upper16:CPU_CONTROL_SET
#endif
	orr	r0, r0, r1
	mcr	p15, 0, r0, c1, c0, 0
	isb
	dsb

	/* Jump to kernel code in TRUE VA */
	movw	ip, #:lower16:start
	movt	ip, #:upper16:start
	bx	ip

	/* NOTREACHED */

	.global _C_LABEL(uboot_regs_pa)
uboot_regs_pa:
	.space	16 /* r0, r1, r2, r3 */

#define MMU_INIT(va,pa,n_sec,attr) \
	.word	n_sec					; \
	.word	4*((va)>>L1_S_SHIFT)			; \
	.word	(pa)|(attr)				;

mmu_init_table:
	/* fill all table VA==PA */
	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(MEMSTART, MEMSTART, 64, L1_TYPE_S|L1_S_C|L1_S_AP_KRW)

	/* map VA 0x80000000..0x83ffffff to PA */
	MMU_INIT(KERNEL_BASE_EXT, MEMSTART, 64, L1_TYPE_S|L1_S_C|L1_S_AP_KRW)

	/* 
	 * In case of early start debugging it might be useful to map
	 * SoC registers (for UART access).
	 */
	MMU_INIT(MARVELL_INTERREGS_PBASE, MARVELL_INTERREGS_PBASE, 1,
	    L1_TYPE_S|L1_S_PROTO|L1_S_AP_KRW)
	MMU_INIT(MARVELL_INTERREGS_VBASE, MARVELL_INTERREGS_PBASE, 1,
	    L1_TYPE_S|L1_S_PROTO|L1_S_AP_KRW)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)
