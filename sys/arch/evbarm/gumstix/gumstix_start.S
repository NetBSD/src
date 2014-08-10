/*	$NetBSD: gumstix_start.S,v 1.12.2.1 2014/08/10 06:53:55 tls Exp $ */
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

#include "opt_cputypes.h"
#include "opt_gumstix.h"

#include <machine/asm.h>
#include <arm/armreg.h>
#include "assym.h"

RCSID("$NetBSD: gumstix_start.S,v 1.12.2.1 2014/08/10 06:53:55 tls Exp $")

/*
 * CPWAIT -- Canonical method to wait for CP15 update.
 * NOTE: Clobbers the specified temp reg.
 * copied from arm/arm/cpufunc_asm_xscale.S
 * XXX: better be in a common header file.
 */
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
#define	CPWAIT_BRANCH							  \
	sub	pc, pc, #4
#else
#define	CPWAIT_BRANCH
#endif

#define	CPWAIT(tmp)							  \
	mrc	p15, 0, tmp, c2, c0, 0	/* arbitrary read of CP15 */	; \
	mov	tmp, tmp		/* wait for it to complete */	; \
	CPWAIT_BRANCH			/* branch to next insn */

/*
 * Kernel start routine for GUMSTIX
 * this code is excuted at the very first after the kernel is loaded
 * by U-Boot.
 */
	.text

	.global	_C_LABEL(gumstix_start)
_C_LABEL(gumstix_start):
	/*
	 * gumstix's loader is U-boot.  it's running on RAM
	 */

	/* Our page table might be cached.  Disable D-cache beforehand. */
	mrc	p15, 0, r4, c1, c0, 0
	bic	r4, r4, #CPU_CONTROL_DC_ENABLE
	mcr	p15, 0, r4, c1, c0, 0

	/*
	 *  Kernel is loaded in SDRAM (0xa0200000..), and is expected to run
	 *  in VA 0xc0200000.. (GUMSTIX)
	 *  VA == PA if OVERO.
	 */

	/* save u-boot's args */
	adr	r4, u_boot_args
	nop
	nop
	nop
	stmia	r4!, {r0, r1, r2, r3}
	nop
	nop
	nop

	/* Calculate RAM size */
	adr	r4, ram_size
#if defined(GUMSTIX)
	ldr	r0, [r4]

	mrc	p15, 0, r1, c0, c0, 0
	and	r1, r1, #CPU_ID_XSCALE_COREGEN_MASK
	cmp	r1, #0x4000
	bne	3f			/* goto 3f, if basix or connex */
0:
	/* check memory size, if verdex or verdex-pro */
	add	r3, r4, r0
	ldr	r1, [r3]
	cmp	r0, r1
	beq	2f
1:
	add	r0, r0, r0		/* r0 <<= 1 */
	str	r0, [r4]
	b	0b
2:
	mvn	r1, r1			/* r1 ^= 0xffffffff */
	str	r1, [r3]
	ldr	r2, [r4]
	cmp	r1, r2
	beq	3f
	str	r0, [r3]		/* restore */
	b	1b
3:
#elif defined(OVERO)
	mov	r1, #0x7f000000		/* mask */
	orr	r1, r1, #0x00e00000	/* mask */
	mov	r3, #0x6d000000		/* OMAP34xx SDRC */
	add	r3, r3, #0x0080		/* CS0 MCFG */
	ldr	r2, [r3]
	and	r0, r1, r2, lsl #13
	add	r3, r3, #0x0030		/* CS1 MCFG */
	ldr	r2, [r3]
	and	r2, r1, r2, lsl #13
	add	r0, r0, r2
#endif
	str	r0, [r4]

	/* Build page table from scratch */
	ldr	r0, Lstartup_pagetable	/* pagetable */
	adr	r4, mmu_init_table
	b	5f

4:
	str	r3, [r0, r2, lsl #2]
	add	r2, r2, #1
	add	r3, r3, #(L1_S_SIZE)
	adds	r1, r1, #-1
	bhi	4b
5:
	ldmia	r4!, {r1, r2, r3}	/* # of sections, PA|attr, VA */
	lsr	r2, r2, #L1_S_SHIFT
	cmp	r1, #0
	bne	4b

	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */

#if defined(CPU_CORTEXA8)
	mcr	p15, 0, r0, c2, c0, 1	/* Set TTB1 */
	mov	r0, #TTBCR_S_N_1
	mcr	p15, 0, r0, c2, c0, 2	/* Set TTBCR */
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */
#endif

	/*
	 * Set the Domain Access register.  Very important!
	 * startup_pagetable puts to domain 0 now.
	 */
#define KERNEL_DOMAIN(x)	((x) << (PMAP_DOMAIN_KERNEL << 1))
	mov	r0, #(KERNEL_DOMAIN(DOMAIN_CLIENT) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU and etc. */
	mrc	p15, 0, r0, c1, c0, 0
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
	orr	r0, r0, #CPU_CONTROL_SYST_ENABLE
#endif
#if defined(CPU_CORTEXA8)
	/* Disable L2 cache beforehand. */
	mrc	p15, 0, r1, c1, c0, 1
	bic	r1, r1, #0x2		/* clear L2EN */
	mcr	p15, 0, r1, c1, c0, 1

	orr	r0, r0, #CPU_CONTROL_AFLT_ENABLE | CPU_CONTROL_DC_ENABLE
	orr	r0, r0, #CPU_CONTROL_BPRD_ENABLE | CPU_CONTROL_IC_ENABLE
#endif
	orr	r0, r0, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r0, c1, c0, 0
	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	CPWAIT(r0)

	/* Jump to kernel code in TRUE VA */
	ldr	r0, Lstart
	bx	r0

Lstart:
	.word	start

#ifndef STARTUP_PAGETABLE_ADDR
#if defined(GUMSTIX)
#define STARTUP_PAGETABLE_ADDR 0xa0000000	/* aligned 16kByte */
#elif defined(OVERO)
#define STARTUP_PAGETABLE_ADDR 0x80000000	/* aligned 16kByte */
#endif
#endif
Lstartup_pagetable:
	.word	STARTUP_PAGETABLE_ADDR

	.globl	_C_LABEL(u_boot_args)
u_boot_args:
	.space	16			/* r0, r1, r2, r3 */

	.globl	_C_LABEL(ram_size)
ram_size:
	.word	0x04000000		/* 64Mbyte */


#define MMU_INIT(va, pa, n_sec, attr)	  \
	.word	n_sec			; \
	.word	(va)			; \
	.word	(pa) | (attr)		;

mmu_init_table:
#if defined(GUMSTIX)
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000,
	    1 << (32 - L1_S_SHIFT), L1_S_PROTO | L1_S_AP_KRW)

#define SDRAM_START	0xa0000000

	/* map SDRAM VA==PA, write-back cacheable (first 64M only)*/
	MMU_INIT(SDRAM_START, SDRAM_START,
	    64, L1_S_PROTO | L1_S_C | L1_S_AP_KRW)

	/* map VA 0xc0000000..0xc3ffffff to PA 0xa0000000..0xa3ffffff */
	MMU_INIT(0xc0000000, SDRAM_START,
	    64, L1_S_PROTO | L1_S_C | L1_S_AP_KRW)

#elif defined(OVERO)
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000,
	    1 << (32 - L1_S_SHIFT), L1_S_PROTO | L1_S_APv7_KRW)

#define SDRAM_START	0x80000000

	/* Map VA to PA, write-back cacheable (first 64M only) */
	MMU_INIT(KERNEL_BASE & 0xffffffff, SDRAM_START,
	    64, L1_S_PROTO | L1_S_B | L1_S_C | L1_S_APv7_KRW)
#endif

	MMU_INIT(0, 0, 0, 0)		/* end of table */
