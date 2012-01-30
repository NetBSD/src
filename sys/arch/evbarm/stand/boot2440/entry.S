/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

#define _LOCORE
#define _KERNEL
	
#include <machine/asm.h>
#include <arm/armreg.h>
#include <arm/arm32/pte.h>
#include <arm/arm32/pmap.h>		/* for PMAP_DOMAIN_KERNEL */	

#include <arm/s3c2xx0/s3c2440reg.h>	/* for S3C2440_SDRAM_START */

#ifndef	SDRAM_START
#define	SDRAM_START	S3C2440_SDRAM_START
#endif
	
#define LED1 (1<<5)
#define LED2 (1<<6)
#define LED3 (1<<7)
#define LED4 (1<<8)
	
	.text
	.global _start
_start:
	/* Get arguments from boot-loader (stored in r0 and r1) */
	adr	r2, bootloader_args
	stmia	r2, {r0, r1}

	/* Disable interrupt */
	mrs	r0, cpsr
	orr	r0, r0, #I32_bit
	msr	cpsr, r0

        /* Turn off all LEDS except LED2 */
        mov     r1, #S3C2440_GPIO_BASE
        add     r1, r1, #0x14
        ldr     r3, [r1]
        orr     r3, r3, #LED1 /* LEDS are active-low, so we set their bit to turn them off */
        bic     r3, r3, #LED2
        orr     r3, r3, #LED3
        orr     r3, r3, #LED4
        str     r3, [r1]

        /* Setup BANK6/7 memory map */
        mov     r1, #S3C2440_MEMCTL_BASE
        ldr     r2, [r1, #MEMCTL_BANKSIZE]
        bic     r2, r2, #0x7 /* Clear the three lowest bits (BK67MAP) */
        add     r2, r2, #0x1 /* Set BK67MAP to b001 = 64MB/64MB */
        str     r2, [r1, #MEMCTL_BANKSIZE]

	/* Disable MMU for a while */
	mrc	p15, 0, r2, c1, c0, 0
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r2, c1, c0, 0

	nop
	nop
	nop
	
	ldr	r0, LpageTable      /* pagetable */
	adr	r4, mmu_init_table
	b	2f
1:
	str	r3, [r0, r2]
	add	r2, r2, #4
	add	r3, r3, #(L1_S_SIZE)
	adds	r1, r1, #-1
	bhi	1b
2:	
	ldmia	r4!, {r1,r2,r3}   /* # of sections, PA|attr, VA */
	cmp	r1, #0
	bne	1b	

	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

	/* Set the Domain Access register.  Very important! */
	mov	r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r0, c1, c0, 0
	
	nop
	nop
	nop

	/* Prepare stack */
	adr	r1, Lcrtsetup
	ldmia	r1, {r1, r2, sp}
	sub     r2, r2, r1              /* get zero init data */
        mov     r3, #0
.L1:
        str     r3, [r1], #0x0004       /* zero the bss */
        subs    r2, r2, #4
        bgt     .L1

	
	adr	r2, bootloader_args
	ldmia	r2, {r0, r1}

	/* Jump to kernel code in TRUE VA */
	ldr	pc, Lstart

Lstart:
	.word	main
	
	.macro clock_data  hdivn, pdivn, mdiv, pdiv, sdiv
	.word	(\hdivn)<<1 | \pdivn
	.word	(\mdiv)<<PLLCON_MDIV_SHIFT | (\pdiv)<<PLLCON_PDIV_SHIFT | (\sdiv)<<PLLCON_SDIV_SHIFT
	.endm

#define MMU_INIT(va,pa,n_sec,attr) \
	.word	n_sec					    ; \
	.word	4*((va)>>L1_S_SHIFT)			    ; \
	.word	(pa)|(attr)				    ;
	
mmu_init_table:	
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000, 1<<(32-L1_S_SHIFT), L1_TYPE_S|L1_S_AP(AP_KRW))
	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(SDRAM_START, SDRAM_START, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))
	/* map VA 0xc0000000..0xc3ffffff to PA 0x30000000..0x33ffffff */
	MMU_INIT(0xc0000000, SDRAM_START, 64, L1_TYPE_S|L1_S_C|L1_S_AP(AP_KRW))

	.word 0	/* end of table */

LpageTable:
	.word	0x30000000
Lcrtsetup:
	.word	_edata	/* Start of BSS */
	.word	_end	/* End of BSS */
	.word	0x30A00000 /* Place stack-bottom at load-point of libsa bootloader */

	.global _C_LABEL(bootloader_args)
_C_LABEL(bootloader_args):
	.space	8 /* Two registers */
