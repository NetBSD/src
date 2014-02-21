/*	$NetBSD: a9_mpsubr.S,v 1.13 2014/02/21 22:22:48 matt Exp $	*/
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

#if defined(CPU_CORTEXA8)
#undef CPU_CONTROL_SWP_ENABLE		// not present on A8
#define CPU_CONTROL_SWP_ENABLE		0
#endif
#ifdef __ARMEL__
#define CPU_CONTROL_EX_BEND_SET		0
#else
#define CPU_CONTROL_EX_BEND_SET		CPU_CONTROL_EX_BEND
#endif
#ifdef ARM32_DISABLE_ALIGNMENT_FAULTS
#define CPU_CONTROL_AFLT_ENABLE_CLR	CPU_CONTROL_AFLT_ENABLE
#define CPU_CONTROL_AFLT_ENABLE_SET	0
#else
#deifne CPU_CONTROL_AFLT_ENABLE_CLR	0
#define CPU_CONTROL_AFLT_ENABLE_SET	CPU_CONTROL_AFLT_ENABLE
#endif

#define CPU_CONTROL_SET \
	(CPU_CONTROL_MMU_ENABLE		|	\
	 CPU_CONTROL_AFLT_ENABLE_SET	|	\
	 CPU_CONTROL_DC_ENABLE		|	\
	 CPU_CONTROL_SWP_ENABLE		|	\
	 CPU_CONTROL_BPRD_ENABLE	|	\
	 CPU_CONTROL_IC_ENABLE		|	\
	 CPU_CONTROL_EX_BEND_SET	|	\
	 CPU_CONTROL_UNAL_ENABLE)

#define CPU_CONTROL_CLR \
	(CPU_CONTROL_AFLT_ENABLE_CLR)

arm_cpuinit:
	/*
	 * In theory, because the MMU is off, we shouldn't need all of this,
	 * but let's not take any chances and do a typical sequence to set
	 * the Translation Table Base.
	 */
	mov	ip, lr
	mov	r10, r0
	mov	r1, #0

	mcr     p15, 0, r1, c7, c5, 0	// invalidate I cache

	mrc	p15, 0, r2, c1, c0, 0	// read SCTRL
	movw	r1, #(CPU_CONTROL_DC_ENABLE|CPU_CONTROL_IC_ENABLE)
	bic	r2, r2, r1		// clear I+D cache enable

#ifdef __ARMEB__
	/*
	 * SCTRL.EE determines the endianness of translation table lookups.
	 * So we need to make sure it's set before starting to use the new
	 * translation tables (which are big endian).
	 */
	orr	r2, r2, #CPU_CONTROL_EX_BEND
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	pli	[pc, #32]		/* preload the next few cachelines */
	pli	[pc, #64]
	pli	[pc, #96]
	pli	[pc, #128]
#endif

	mcr	p15, 0, r2, c1, c0, 0	/* write SCTRL */

	XPUTC(#70)
	dsb				/* Drain the write buffers. */
1:
	XPUTC(#71)
	mrc	p15, 0, r1, c0, c0, 5	/* get MPIDR */
	cmp	r1, #0
	orrlt	r10, r10, #0x5b		/* MP, cachable (Normal WB) */
	orrge	r10, r10, #0x1b		/* Non-MP, cacheable, normal WB */
	mcr	p15, 0, r10, c2, c0, 0	/* Set Translation Table Base */

	XPUTC(#72)
	mov	r1, #0
	mcr	p15, 0, r1, c2, c0, 2	/* Set Translation Table Control */

	XPUTC(#73)
	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0	/* Invalidate TLBs */

	/* Set the Domain Access register.  Very important! */
	XPUTC(#74)
	mov     r1, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r1, c3, c0, 0

	/*
	 * Enable the MMU, etc.
	 */
	XPUTC(#75)
	mrc	p15, 0, r0, c1, c0, 0

	movw	r3, #:lower16:CPU_CONTROL_SET
#if (CPU_CONTROL_SET & 0xffff0000)
	movt	r3, #:upper16:CPU_CONTROL_SET
#endif
	orr	r0, r0, r3
#if defined(CPU_CONTROL_CLR) && (CPU_CONTROL_CLR != 0)
	bic	r0, r0, #CPU_CONTROL_CLR
#endif
	pli	1f
	
	dsb
	@ turn mmu on!
	mov	r0, r0			/* fetch instruction cacheline */
1:	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	mrc	p15, 0, r0, c0, c0, 0	/* Read an arbitrary value. */
	mov	r0, r0			/* Stall until read completes. */
1:	XPUTC(#76)

	bx	ip			/* return */

/*
 * Coprocessor register initialization values
 */

	.p2align 2

	/* bits to set in the Control Register */

#if defined(VERBOSE_INIT_ARM) && XPUTC_COM
#define TIMO		0x25000
#ifndef COM_MULT
#define COM_MULT	1
#endif
xputc:
#ifdef MULTIPROCESSOR
	adr	r3, xputc
	movw	r2, #:lower16:comlock
	movt	r2, #:upper16:comlock
	bfi	r3, r2, #0, #28
	mov	r2, #1
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
#ifdef CONADDR
	movw	r3, #:lower16:CONADDR
	movt	r3, #:upper16:CONADDR
#elif defined(CONSADDR)
	movw	r3, #:lower16:CONSADDR
	movt	r3, #:upper16:CONSADDR
#endif
1:
#if COM_MULT == 1
	ldrb	r1, [r3, #(COM_LSR*COM_MULT)]
#else
#if COM_MULT == 2
	ldrh	r1, [r3, #(COM_LSR*COM_MULT)]
#elif COM_MULT == 4
	ldr	r1, [r3, #(COM_LSR*COM_MULT)]
#endif
#ifdef COM_BSWAP
	lsr	r1, r1, #(COM_MULT-1)*8
#endif
#endif
	tst	r1, #LSR_TXRDY
	bne	2f
	subs	r2, r2, #1
	bne	1b
2:
#if COM_MULT == 1
	strb	r0, [r3, #COM_DATA]
#else
#ifdef COM_BSWAP
	lsl	r0, r0, #(COM_MULT-1)*8
#endif
#if COM_MULT == 2
	strh	r0, [r3, #COM_DATA]
#else
	str	r0, [r3, #COM_DATA]
#endif
#endif

	mov	r2, #TIMO
3:	
#if COM_MULT == 1
	ldrb	r1, [r3, #(COM_LSR*COM_MULT)]
#else
#if COM_MULT == 2
	ldrh	r1, [r3, #(COM_LSR*COM_MULT)]
#elif COM_MULT == 4
	ldr	r1, [r3, #(COM_LSR*COM_MULT)]
#endif
#ifdef COM_BSWAP
	lsr	r1, r1, #(COM_MULT-1)*8
#endif
#endif
	tst	r1, #LSR_TSRE
	bne	4f
	subs	r2, r2, #1
	bne	3b
4:
#ifdef MULTIPROCESSOR
	adr	r3, xputc
	movw	r2, #:lower16:comlock
	movt	r2, #:upper16:comlock
	bfi	r3, r2, #0, #28
	mov	r0, #0
	str	r0, [r3]
	dsb
#endif
	bx	lr

#ifdef MULTIPROCESSOR
	.pushsection .data
comlock:
	.p2align 4
	.word	0		@ not in bss
	.p2align 4

	.popsection
#endif /* MULTIPROCESSOR */
#endif /* VERBOSE_INIT_ARM */

cortex_init:
	mov	r10, lr				@ save lr

	cpsid	if, #PSR_SVC32_MODE

	XPUTC(#64)
	adr	ip, cortex_init
	movw	r0, #:lower16:_C_LABEL(armv7_icache_inv_all)
	movt	r0, #:upper16:_C_LABEL(armv7_icache_inv_all)
	bfi	ip, r0, #0, #28
	blx	ip				@ toss i-cache

#ifdef CPU_CORTEXA9
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
#endif

	/*
	 * Step 1b, invalidate the data cache
	 */
	XPUTC(#66)
	adr	ip, cortex_init
	movw	r0, #:lower16:_C_LABEL(armv7_dcache_wbinv_all)
	movt	r0, #:upper16:_C_LABEL(armv7_dcache_wbinv_all)
	bfi	ip, r0, #0, #28
	blx	ip				@ writeback & toss d-cache
	XPUTC(#67)

#ifdef CPU_CORTEXA9
	/*
	 * Step 2, disable the data cache
	 */
	mrc	p15, 0, r2, c1, c0, 0		@ get system ctl register (save)
	bic	r1, r2, #CPU_CONTROL_DC_ENABLE	@ clear data cache enable
	mcr	p15, 0, r1, c1, c0, 0		@ set system ctl register
	isb
	XPUTC(#49)

	/*
	 * Step 3, enable the SCU (and set SMP mode)
	 */
	mrc	p15, 4, r3, c15, c0, 0		@ read cbar
	ldr	r1, [r3, #SCU_CTL]		@ read scu control
	orr	r1, r1, #SCU_CTL_SCU_ENA	@ set scu enable flag
	str	r1, [r3, #SCU_CTL]		@ write scu control
	dsb
	isb
	XPUTC(#50)

	/*
	 * Step 4a, enable the data cache
	 */
	orr	r2, r2, #CPU_CONTROL_DC_ENABLE	@ set data cache enable
	mcr	p15, 0, r2, c1, c0, 0		@ reenable caches
	isb
	XPUTC(#51)
#endif

#ifdef MULTIPROCESSOR
	/*
	 * Step 4b, set ACTLR.SMP=1 (and on A9, ACTRL.FX=1)
	 */
	mrc	p15, 0, r0, c1, c0, 1		@ read aux ctl
	orr	r0, r0, #CORTEXA9_AUXCTL_SMP	@ enable SMP
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	isb
#ifdef CPU_CORTEXA9
	orr	r0, r0, #CORTEXA9_AUXCTL_FW	@ enable cache/tlb/coherency
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	isb
#endif
	XPUTC(#52)
#endif /* MULTIPROCESSOR */

	bx	r10
ASEND(cortex_init)

/*
 * Secondary processors come here after exiting the SKU ROM.
 * Running native endian until we have SMP enabled.  Since no data
 * is accessed, that shouldn't be a problem.
 */
cortex_mpstart:
	cpsid	if, #PSR_SVC32_MODE		@ make sure we are in SVC mode
        mrs	r0, cpsr			@ fetch CPSR value
        msr	spsr_sxc, r0			@ set SPSR[23:8] to known value

#ifndef MULTIPROCESSOR
	/*
	 * If not MULTIPROCESSOR, drop CPU into power saving state.
	 */
3:	wfe
	b	3b
#else
	/*
	 * Step 1, invalidate the caches
	 */
	adr	ip, cortex_mpstart
	movw	r0, #:lower16:_C_LABEL(armv7_icache_inv_all)
	movt	r0, #:upper16:_C_LABEL(armv7_icache_inv_all)
	bfi	ip, r0, #0, #28
	blx	ip				@ toss i-cache
	adr	ip, cortex_mpstart
	movw	ip, #:lower16:_C_LABEL(armv7_dcache_inv_all)
	movt	ip, #:upper16:_C_LABEL(armv7_dcache_inv_all)
	bfi	ip, r0, #0, #28
	blx	ip				@ toss d-cache

#if defined(CPU_CORTEXA9)
	/*
	 * Step 2, wait for the SCU to be enabled
	 */
	mrc	p15, 4, r3, c15, c0, 0		@ read cbar
1:	ldr	r0, [r3, #SCU_CTL]		@ read scu control
	tst	r0, #SCU_CTL_SCU_ENA		@ enable bit set yet?
	bne	1b				@ try again
#endif

	/*
	 * Step 3, set ACTLR.SMP=1 (and ACTRL.FX=1)
	 */
	mrc	p15, 0, r0, c1, c0, 1		@ read aux ctl
	orr	r0, #CORTEXA9_AUXCTL_SMP	@ enable SMP
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	mov	r0, r0
#if defined(CPU_CORTEXA9)
	orr	r0, #CORTEXA9_AUXCTL_FW		@ enable cache/tlb/coherency
	mcr	p15, 0, r0, c1, c0, 1		@ write aux ctl
	mov	r0, r0
#endif

	/*
	 * We should be in SMP mode now.
	 */
	mrc	p15, 0, r4, c0, c0, 5		@ get MPIDR
	and	r4, r4, #7			@ get our cpu numder

#ifdef __ARMEB__
	setend	be				@ switch to BE now
#endif

#if defined(VERBOSE_INIT_ARM)
	add	r0, r4, #48
	bl	xputc
#endif

	/*
	 * To access things are not in .start, we need to replace the upper
	 * 4 bits of the address with where we are current executing.
	 */
	adr	r10, cortex_mpstart
	lsr	r10, r10, #28

	movw	r0, #:lower16:_C_LABEL(arm_cpu_hatched)
	movt	r0, #:upper16:_C_LABEL(arm_cpu_hatched)
	bfi	r0, r10, #28, #4		// replace top 4 bits
	add	r0, r0, r10
	mov	r5, #1
	lsl	r5, r5, r4
	/*
	 * We inline the atomic_or_32 call since we might be in a different
	 * area of memory.
	 */
2:	ldrex	r1, [r0]
	orr	r1, r1, r5
	strex	r2, r1, [r0]
	cmp	r2, #0
	bne	2b

	XPUTC(#97)

	/* Now we will wait for someone tell this cpu to start running */
	movw	r0, #:lower16:_C_LABEL(arm_cpu_mbox)
	movt	r0, #:upper16:_C_LABEL(arm_cpu_mbox)
	bfi	r0, r10, #28, #4
	add	r0, r0, r10
3:	dmb
	ldr	r2, [r0]
	tst	r2, r5
	wfeeq
	beq	3b

	XPUTC(#98)
	movw	r0, #:lower16:_C_LABEL(arm_cpu_marker)
	movt	r0, #:upper16:_C_LABEL(arm_cpu_marker)
	bfi	r0, r10, #28, #4
	str	pc, [r0]

	movw	r0, #:lower16:_C_LABEL(kernel_l1pt)
	movt	r0, #:upper16:_C_LABEL(kernel_l1pt)
	bfi	r0, r10, #28, #4		/* get address of l1pt pvaddr */
	ldr	r0, [r0, #PV_PA]		/* Now get the phys addr */
	/*
	 * After we turn on the MMU, we will no longer in .start so setup
	 * return to rest of MP startup code in .text.
	 */
	movw	lr, #:lower16:cortex_mpcontinuation
	movt	lr, #:upper16:cortex_mpcontinuation
	b	arm_cpuinit
#endif /* MULTIPROCESSOR */
ASEND(cortex_mpstart)

#ifdef MULTIPROCESSOR
	.pushsection .text
cortex_mpcontinuation:
	/* MMU, L1, are now on. */

	movw	r0, #:lower16:_C_LABEL(arm_cpu_marker)
	movt	r0, #:upper16:_C_LABEL(arm_cpu_marker)
	str	pc, [r0]

	movw	r0, #:lower16:cpu_info
	movt	r0, #:upper16:cpu_info		/* get pointer to cpu_infos */
	ldr	r5, [r0, r4, lsl #2]		/* load our cpu_info */
	ldr	r6, [r5, #CI_IDLELWP]		/* get the idlelwp */
	ldr	r7, [r6, #L_PCB]		/* now get its pcb */
	ldr	sp, [r7, #PCB_KSP]		/* finally, we can load our SP */
#ifdef TPIDRPRW_IS_CURCPU
	mcr	p15, 0, r5, c13, c0, 4		/* squirrel away curcpu() */
#elif defined(TPIDRPRW_IS_CURLWP)
	mcr	p15, 0, r6, c13, c0, 4		/* squirrel away curlwp() */
#else
#error either TPIDRPRW_IS_CURCPU or TPIDRPRW_IS_CURLWP must be defined
#endif
	str	r6, [r5, #CI_CURLWP]		/* and note we are running on it */

	str	pc, [r0]			// r0 still have arm_cpu_marker

	mov	r0, r5				// pass cpu_info
	mov	r1, r4				// pass cpu_id
	movw	r2, #:lower16:MD_CPU_HATCH	// pass md_cpu_hatch
	movt	r2, #:upper16:MD_CPU_HATCH	// pass md_cpu_hatch
	bl	_C_LABEL(cpu_hatch)
	b	_C_LABEL(idle_loop)
ASEND(cortex_mpcontinuation)
	/* NOT REACHED */
	.popsection
#endif /* MULTIPROCESSOR */
