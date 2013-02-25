/*	$NetBSD: a9_mpsubr.S,v 1.3.2.1 2013/02/25 00:28:26 tls Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include <arm/cortex/scu_reg.h>
#include "assym.h"


/* We'll modify va and pa at run time so we can use relocatable addresses. */
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	va					    ; \
	.word	pa					    ; \
	.word	n_sec					    ; \
	.word	attr					    ;

/*
 * Set up a preliminary mapping in the MMU to allow us to run
 * at KERNEL_BASE with caches on.
 */
arm_boot_l1pt_init:
	mov	ip, r1			@ save mmu table addr
	/* Build page table from scratch */
	mov	r1, r0			/* Start address to clear memory. */
	/* Zero the entire table so all virtual addresses are invalid. */
	mov	r2, #L1_TABLE_SIZE	/* in bytes */
	mov	r3, #0
	mov	r4, r3
	mov	r5, r3
	mov	r6, r3
	mov	r7, r3
	mov	r8, r3
	mov	r10, r3
	mov	r11, r3
1:	stmia	r1!, {r3-r8,r10-r11}
	stmia	r1!, {r3-r8,r10-r11}
	stmia	r1!, {r3-r8,r10-r11}
	stmia	r1!, {r3-r8,r10-r11}
	subs	r2, r2, #(4 * 4 * 8)	/* bytes per loop */
	bne	1b

	/* Now create our entries per the mmu_init_table. */
	l1table	.req r0
	va	.req r1
	pa	.req r2
	n_sec	.req r3
	attr	.req r4
	itable	.req r5

	mov	itable, ip		@ reclaim table address
	b	3f

2:	str	pa, [l1table, va, lsl #2]
	add	va, va, #1
	add	pa, pa, #(L1_S_SIZE)
	subs	n_sec, n_sec, #1
	bhi	2b

3:	ldmia	itable!, {va,pa,n_sec,attr}
	/* Convert va to l1 offset:	va = 4 * (va >> L1_S_SHIFT)	*/
	lsr	va, va, #L1_S_SHIFT
	/* Convert pa to l1 entry:	pa = (pa & L1_S_FRAME) | attr	*/
#ifdef _ARM_ARCH_7
	bfc	pa, #0, #L1_S_SHIFT
#else
	lsr	pa, pa, #L1_S_SHIFT
	lsl	pa, pa, #L1_S_SHIFT
#endif
	orr	pa, pa, attr
	cmp	n_sec, #0
	bne	2b
	bx	lr			@ return

	.unreq	va
	.unreq	pa
	.unreq	n_sec
	.unreq	attr
	.unreq	itable
	.unreq	l1table

a9_cpuinit:
	/*
	 * In theory, because the MMU is off, we shouldn't need all of this,
	 * but let's not take any chances and do a typical sequence to set
	 * the Translation Table Base.
	 */
	mov	ip, lr
	mov	r10, r0

	mrc	p15, 0, r2, c1, c0, 0	/*  "       "   "     */
	bic	r2, r2, #CPU_CONTROL_DC_ENABLE	@ clear data cache enable
	bic	r2, r2, #CPU_CONTROL_IC_ENABLE	@ clear instruction cache enable
	mcr	p15, 0, r2, c1, c0, 0	/*  "       "   "     */

	XPUTC(#70)
	mov	r1, #0
	mcr	p15, 0, r1, c7, c10, 4	/* Drain the write buffers. */

	XPUTC(#71)
	mcr	p15, 0, r10, c2, c0, 0	/* Set Translation Table Base */

	XPUTC(#72)
	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0	/* Invalidate TLBs */

	/* Set the Domain Access register.  Very important! */
	XPUTC(#73)
	mov     r1, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r1, c3, c0, 0

	XPUTC(#74)
	mov	r0, #0			/* make sure TTBCR is 0 */
	mcr	p15, 0, r0, c1, c0, 2
	
	/*
	 * Enable the MMU, etc.
	 */
	XPUTC(#75)
	mrc	p15, 0, r0, c1, c0, 0

	ldr	r2, .Lcontrol_clr
	bic	r0, r0, r2
	ldr	r3, .Lcontrol_set
	orr	r0, r0, r3
	
	dsb
	.align 5
	@ turn mmu on!
	mov	r0, r0
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	mrc	p15, 0, r0, c0, c0, 0	/* Read an arbitrary value. */
	mov	r0, r0			/* Stall until read completes. */

	bx	ip			/* return */

/*
 * Coprocessor register initialization values
 */

	.p2align 2
	/* bits to clear in the Control Register */
.Lcontrol_clr:
	.word	0

	/* bits to set in the Control Register */
.Lcontrol_set:
	.word CPU_CONTROL_MMU_ENABLE	|			\
	      CPU_CONTROL_AFLT_ENABLE	|			\
	      CPU_CONTROL_DC_ENABLE	|			\
	      CPU_CONTROL_SYST_ENABLE	|			\
	      CPU_CONTROL_SWP_ENABLE	|			\
	      CPU_CONTROL_IC_ENABLE

#if defined(VERBOSE_INIT_ARM) && XPUTC_COM
#define	TIMO	0x25000
#ifndef COM_MULT
#define	COM_MULT	1
#endif
xputc:
#ifdef MULTIPROCESSOR
	mov	r2, #1
	ldr	r3, .Lcomlock
10:
	ldrex	r1, [r3]
	cmp	r1, #0
	bne	10b
	strex	r1, r2, [r3]
	cmp	r1, #0
	bne	10b
	dsb
#endif

	mov	r2, #TIMO
	ldr	r3, .Luart0
1:	ldrb	r1, [r3, #(COM_LSR*COM_MULT)]
	tst	r1, #LSR_TXRDY
	bne	2f
	subs	r2, r2, #1
	bne	1b
2:
	strb	r0, [r3, #COM_DATA]

	mov	r2, #TIMO
3:	ldrb	r1, [r3, #(COM_LSR*COM_MULT)]
	tst	r1, #LSR_TSRE
	bne	4f
	subs	r2, r2, #1
	bne	3b
4:
#ifdef MULTIPROCESSOR
	ldr	r3, .Lcomlock
	mov	r0, #0
	str	r0, [r3]
	dsb
#endif
	bx	lr

.Luart0:
	.word	CONADDR

#ifdef MULTIPROCESSOR
.Lcomlock:
	.word	comlock

	.pushsection .data
comlock:
	.p2align 2
	.word	0		@ not in bss

	.popsection
#endif /* MULTIPROCESSOR */
#endif /* VERBOSE_INIT_ARM */

#ifdef CPU_CORTEXA9
a9_start:
	mov	r10, lr				@ save lr

	cpsid	if, #PSR_SVC32_MODE

	XPUTC(#64)
	bl	_C_LABEL(armv7_icache_inv_all)	@ invalidate i-cache

	/*
	 * Step 1a, invalidate the all cache tags in all ways on the SCU.
	 */
	XPUTC(#65)
	mrc	p15, 4, r3, c15, c0, 0		@ read cbar
	ldr	r0, [r3, #SCU_CFG]		@ read scu config
	and	r0, r0, #7			@ get cpu max
	add	r0, r0, #2			@ adjust to cpu num
	mov	r1, #0xf			@ select all ways
	lsl	r1, r1, r0			@ shift into place
	str	r1, [r3, #SCU_INV_ALL_REG]	@ write scu invalidate all
	dsb
	isb

	/*
	 * Step 1b, invalidate the data cache
	 */
	XPUTC(#66)
	bl	_C_LABEL(armv7_dcache_wbinv_all)	@ writeback/invalidate d-cache

	/*
	 * Step 2, disable the data cache
	 */
	mrc	p15, 0, r2, c1, c0, 0		@ get system ctl register (save)
	bic	r1, r2, #CPU_CONTROL_DC_ENABLE	@ clear data cache enable
	mcr	p15, 0, r1, c1, c0, 0		@ set system ctl register
	isb

	/*
	 * Step 3, enable the SCU (and set SMP mode)
	 */
	ldr	r1, [r3, #SCU_CTL]		@ read scu control
	orr	r1, r1, #SCU_CTL_SCU_ENA	@ set scu enable flag
	str	r1, [r3, #SCU_CTL]		@ write scu control
	dsb
	isb

	/*
	 * Step 4a, enable the data cache
	 */
	mcr	p15, 0, r2, c1, c0, 0		@ reenable caches
	isb

	/*
	 * Step 4b, set ACTLR.SMP=1 (and ACTRL.FX=1)
	 */
	mrc	p15, 0, r0, c1, c0, 1		@ read aux ctl
	orr	r0, r0, #CORTEXA9_AUXCTL_SMP	@ enable SMP
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	isb
	orr	r0, r0, #CORTEXA9_AUXCTL_FW	@ enable cache/tlb/coherency
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	isb

	bx	r10
ASEND(a9_start)

/*
 * Secondary processors come here after exiting the SKU ROM.
 */
a9_mpstart:
#ifdef MULTIPROCESSOR
	/*
	 * Step 1, invalidate the caches
	 */
	bl	_C_LABEL(armv7_icache_inv_all)	@ toss i-cache
	bl	_C_LABEL(armv7_dcache_inv_all)	@ toss d-cache

	/*
	 * Step 2, wait for the SCU to be enabled
	 */
	mrc	p15, 4, r3, c15, c0, 0		@ read cbar
1:	ldr	r0, [r3, #SCU_CTL]		@ read scu control
	tst	r0, #SCU_CTL_SCU_ENA		@ enable bit set yet?
	bne	1b				@ try again

	/*
	 * Step 3, set ACTLR.SMP=1 (and ACTRL.FX=1)
	 */
	mrc	p15, 0, r0, c1, c0, 1		@ read aux ctl
	orr	r0, #CORTEXA9_AUXCTL_SMP	@ enable SMP
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	mov	r0, r0
	orr	r0, #CORTEXA9_AUXCTL_FW		@ enable cache/tlb/coherency
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	mov	r0, r0

	/*
	 * We should be in SMP mode now.
	 */
	mrc	p15, 0, r4, c0, c0, 5		@ get MPIDR
	and	r4, r4, #7			@ get our cpu numder

#if defined(VERBOSE_INIT_ARM)
	add	r0, r4, #48
	bl	xputc
#endif

	ldr	r0, .Lcpu_hatched		@ now show we've hatched
	mov	r5, #1
	lsl	r5, r5, r4
	mov	r1, r5
	bl	_C_LABEL(atomic_or_32)

	XPUTC(#97)
#endif

	cpsid	if, #PSR_SVC32_MODE		@ make sure we are in SVC mode

	/* Now we will wait for someone tell this cpu to start running */
#ifdef MULTIPROCESSOR
	ldr	r0, .Lcpu_mbox
#else
	cmp	r0, r0
#endif
2:
#ifdef MULTIPROCESSOR
	dmb
	ldr	r2, [r0]
	tst	r2, r5
#endif
	@wfeeq
	beq	2b

#ifdef MULTIPROCESSOR
3:	XPUTC(#98)
	ldr	r0, .Lcpu_marker
	str	pc, [r0]

	ldr	r0, .Lkernel_l1pt		/* get address of l1pt pvaddr */
	ldr	r0, [r0, #PV_PA]		/* Now get the phys addr */
	bl	cpu_init

	ldr	r0, .Lcpu_marker
	str	pc, [r0]

	/* MMU, L1, are now on. */

	ldr	r0, .Lcpu_info			/* get pointer to cpu_infos */
	ldr	r5, [r0, r4, lsl #2]		/* load our cpu_info */
	ldr	r6, [r5, #CI_IDLELWP]		/* get the idlelwp */
	ldr	r7, [r6, #L_PCB]		/* now get its pcb */
	ldr	sp, [r7, #PCB_SP]		/* finally, we can load our SP */
#ifdef TPIDRPRW_IS_CURCPU
	mcr	p15, 0, r5, c13, c0, 4		/* squirrel away curcpu() */
#elif defined(TPIDRPRW_IS_CURLWP)
	mcr	p15, 0, r6, c13, c0, 4		/* squirrel away curlwp() */
#else
#error either TPIDRPRW_IS_CURCPU or TPIDRPRW_IS_CURLWP must be defined
#endif
	str	r6, [r5, #CI_CURLWP]		/* and note we are running on it */

	ldr	r0, .Lcpu_marker
	str	pc, [r0]

	mov	r0, r5				/* pass cpu_info */
	mov	r1, r4				/* pass cpu_id */
	ldr	r2, .Lbcm53xx_cpu_hatch		/* pass md_cpu_hatch */
	bl	_C_LABEL(cpu_hatch)
	b	_C_LABEL(idle_loop)
ASEND(a9_mpstart)
	/* NOT REACHED */

.Lkernel_l1pt:
	.word	_C_LABEL(kernel_l1pt)
.Lcpu_info:
	.word	_C_LABEL(cpu_info)
.Lcpu_max:
	.word	_C_LABEL(arm_cpu_max)
.Lcpu_hatched:
	.word	_C_LABEL(arm_cpu_hatched)
.Lcpu_mbox:
	.word	_C_LABEL(arm_cpu_mbox)
.Lcpu_marker:
	.word	_C_LABEL(arm_cpu_marker)
.Lbcm53xx_cpu_hatch:
	.word	_C_LABEL(bcm53xx_cpu_hatch)
#endif /* MULTIPROCESSOR */
#endif /* CPU_CORTEXA9 */
