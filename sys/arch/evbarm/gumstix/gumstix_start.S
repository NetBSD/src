/*	$NetBSD: gumstix_start.S,v 1.1 2006/10/16 16:40:14 kiyohara Exp $ */
/*
 * Copyright (C) 2005, 2006 WIDE Project and SOUM Corporation.
 * All rights reserved.
 *
 * Written by Takashi Kiyohara and Susumu Miki for WIDE Project and SOUM
 * Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the name of SOUM Corporation
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT and SOUM CORPORATION ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT AND SOUM CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pte.h>
#include <arm/arm32/pmap.h>		/* for PMAP_DOMAIN_KERNEL */

#ifndef SDRAM_START
#define SDRAM_START	0xa0000000
#endif

/*
 * CPWAIT -- Canonical method to wait for CP15 update.
 * NOTE: Clobbers the specified temp reg.
 * copied from arm/arm/cpufunc_asm_xscale.S
 * XXX: better be in a common header file.
 */
#define	CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define	CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0	/* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */
	
/*
 * Kernel start routine for GUMSTIX
 * this code is excuted at the very first after the kernel is loaded
 * by U-Boot.
 */
	.text

	.global	_C_LABEL(gumstix_start)
_C_LABEL(gumstix_start):
	/* gumstix's loader is u-boot.  it's running on RAM */
	/*
	 *  Kernel is loaded in SDRAM (0xa0200000..), and is expected to run
	 *  in VA 0xc0200000..  
	 */
	/* save u-boot's args */
	adr	r0, u_boot_args
	nop
	nop
	nop
	stmia	r0!, {r3, r4, r5, r6}
	nop
	nop
	nop

	/* build page table from scratch */
	ldr	r0, Lstartup_pagetable		/* pagetable */
	adr	r4, mmu_init_table
	b	3f

2:
	str	r3, [r0, r2]
	add	r2, r2, #4
	add	r3, r3, #(L1_S_SIZE)
	adds	r1, r1, #-1
	bhi	2b
3:	
	ldmia	r4!, {r1,r2,r3}   /* # of sections, PA|attr, VA */
	cmp	r1, #0
	bne	2b	

	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

	/* Set the Domain Access register.  Very important! */
        mov     r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #CPU_CONTROL_SYST_ENABLE
	orr	r0, r0, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r0, c1, c0, 0
	CPWAIT(r0)

	/* Jump to kernel code in TRUE VA */
	adr	r0, Lstart
	ldr	pc, [r0]

Lstart:
	.word	start

#define MMU_INIT(va,pa,n_sec,attr) \
	.word	n_sec					    ; \
	.word	4*((va)>>L1_S_SHIFT)			    ; \
	.word	(pa)|(attr)				    ;

#ifndef STARTUP_PAGETABLE_ADDR
#define STARTUP_PAGETABLE_ADDR 0xa0000000	/* aligned 16kByte */
#endif
Lstartup_pagetable:	.word	STARTUP_PAGETABLE_ADDR
		.globl _C_LABEL(u_boot_args)
u_boot_args:
			.space 16			/* r3, r4, r5, r6 */

mmu_init_table:	
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000,
	    1<<(32-L1_S_SHIFT), L1_TYPE_S|L1_S_AP(AP_KRW))
	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(SDRAM_START, SDRAM_START, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
	/* map VA 0xc0000000..0xc3ffffff to PA 0xa0000000..0xa3ffffff */
	MMU_INIT(0xc0000000, SDRAM_START, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))

	.word 0	/* end of table */
