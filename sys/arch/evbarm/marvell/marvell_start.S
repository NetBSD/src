/*	$NetBSD: marvell_start.S,v 1.7 2014/08/30 13:24:44 kiyohara Exp $ */
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
#include "opt_mvsoc.h"
#include <machine/asm.h>
#include <arm/armreg.h>
#include <evbarm/marvell/marvellreg.h>
#include "assym.h"

RCSID("$NetBSD: marvell_start.S,v 1.7 2014/08/30 13:24:44 kiyohara Exp $")

#ifndef SDRAM_START
#define SDRAM_START	0x00000000
#endif

#define SHEEVA	1
#define PJ4B	2

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
 * Kernel start routine for Marvell boards
 * this code is excuted at the very first after the kernel is loaded
 * by U-Boot.
 */
	.text

	.global	_C_LABEL(marvell_start)
_C_LABEL(marvell_start):
	/* The Loader for Marvell board is u-boot.  it's running on RAM */
	/*
	 *  Kernel is loaded in SDRAM (0x00200000..), and is expected to run
	 *  in VA 0xc0200000..
	 */

	/* Check cores */
	mrc	p15, 0, r4, c0, c0, 0
	and	r4, r4, #CPU_ID_CPU_MASK
	adr	r5, cores_start
	adr	r6, cores_end
0:
	cmp	r5, r6
	beq	1f
	ldmia	r5!, {r7, r8}
	cmp	r4, r7
	bne	0b

	cmp	r8, #SHEEVA
	bne	1f

sheeva_l2_disable:
	/* Make sure L2 is disabled */
	mrc	p15, 1, r5, c15, c1, 0	@ Get Marvell Extra Features Register
	bic	r5, r5, #0x00400000	@ disable L2 cache
	mcr	p15, 1, r5, c15, c1, 0

#ifdef SHEEVA_L2_CACHE_WT
	/* L2 WT Mode */
	ldr	r5, =0xf1020128		/* CPU L2 Configuration Register */
	ldr	r6, [r5]
	bic	r6, r6, #0x10		/* Force Write Through */
	str	r6, [r5]
#endif

1:

	/* save u-boot's args */
	adr	r4, u_boot_args
	nop
	nop
	nop
	stmia	r4!, {r0, r1, r2, r3}
	nop
	nop
	nop

#if defined(MVSOC_FIXUP_DEVID) && MVSOC_FIXUP_DEVID > 0
	adr	r6, marvell_interregs_pbase
	ldr	r7, [r6]
	add	r7, r7, #0x40000
	ldr	r6, [r7]
	bic	r6, r6, 0xff000000
	bic	r6, r6, 0x00ff0000
	/*
	 * Some SoC returns ugly DeviceID.  Fixup it.
	 */
	adr	r5, devid
	ldr	r5, [r5]
	orr	r6, r6, r5, lsl #16
	str	r6, [r7]
	b	1f
devid:
	.word	MVSOC_FIXUP_DEVID
marvell_interregs_pbase:
	.word	MARVELL_INTERREGS_PBASE
#endif
1:

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
	ldmia	r4!, {r1, r2, r3}	/* # of sections, VA, PA|attr */
	cmp	r1, #0
	bne	2b

	mcr	p15, 0, r0, c2, c0, 0	/* Set TTB */
	mcr	p15, 0, r0, c8, c7, 0	/* Flush TLB */
	mov	r0, #0
	cmp	r8, #PJ4B
	mcreq	p15, 0, r0, c13, c0, 1	/* Set ASID to 0 */
	mcr	p15, 0, r0, c7, c6, 0	/* Invalidate D cache */
	mcr	p15, 0, r0, c7, c10, 4	/* Drain write-buffer */

	/* Ensure safe Translation Table. */

	/* Set the Domain Access register.  Very important! */
	mov	r0, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r0, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r0, c1, c0, 0
	cmp	r8, #PJ4B
	orreq	r0, r0, #CPU_CONTROL_XP_ENABLE
	biceq	r0, r0, #(CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE | CPU_CONTROL_WBUF_ENABLE)
	biceq	r0, r0, #(CPU_CONTROL_IC_ENABLE)
	biceq	r0, r0, #(CPU_CONTROL_BPRD_ENABLE)
	orr	r0, r0, #CPU_CONTROL_SYST_ENABLE
	orr	r0, r0, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r0, c1, c0, 0
	CPWAIT(r0)

	/* Jump to kernel code in TRUE VA */
	adr	r0, Lstart
	ldr	pc, [r0]

Lstart:
	.word	start

#ifndef STARTUP_PAGETABLE_ADDR
#define STARTUP_PAGETABLE_ADDR 0x00004000	/* aligned 16kByte */
#endif
Lstartup_pagetable:
	.word	STARTUP_PAGETABLE_ADDR

	.globl	_C_LABEL(u_boot_args)
u_boot_args:
	.space	16			/* r0, r1, r2, r3 */

cores_start:
	.word	CPU_ID_MV88SV131,	SHEEVA
	.word	CPU_ID_MV88FR571_VD,	SHEEVA	/* Is it Sheeva? */
	.word	CPU_ID_MV88SV581X_V6,	PJ4B
	.word	CPU_ID_MV88SV581X_V7,	PJ4B
	.word	CPU_ID_MV88SV584X_V7,	PJ4B
	.word	CPU_ID_ARM_88SV581X_V6,	PJ4B
	.word	CPU_ID_ARM_88SV581X_V7,	PJ4B
	.word	0,			0
cores_end:

#define MMU_INIT(va,pa,n_sec,attr) \
	.word	n_sec					    ; \
	.word	4 * ((va) >> L1_S_SHIFT)		    ; \
	.word	(pa) | (attr)				    ;

mmu_init_table:
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000,
	    1 << (32 - L1_S_SHIFT), L1_TYPE_S | L1_S_AP_KRW)

	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(SDRAM_START, SDRAM_START,
	    128, L1_TYPE_S | L1_S_C | L1_S_AP_KRW)

	/* map VA 0xc0000000..0xc7ffffff to PA 0x00000000..0x07ffffff */
	MMU_INIT(0xc0000000, SDRAM_START,
	    128, L1_TYPE_S | L1_S_C | L1_S_AP_KRW)

	.word	0			/* end of table */
