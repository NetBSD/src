/*	$NetBSD: netwalker_start.S,v 1.6 2019/05/18 08:49:24 skrll Exp $	*/

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

/*-
 * Copyright (c) 2009 SHIMIZU Ryo <ryo@nerv.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2002, 2003, 2010  Genetec Corporation.  All rights reserved.
 * Written by Kenichi Hashimoto and Hiroyuki Bessho for Genetec Corporation.
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

#include "opt_imx.h"
#include "opt_com.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_arm_debug.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/imx/imx51reg.h>
#include <arm/imx/imxuartreg.h>
#include <evbarm/netwalker/netwalker_reg.h>

RCSID("$NetBSD: netwalker_start.S,v 1.6 2019/05/18 08:49:24 skrll Exp $")

#if defined(VERBOSE_INIT_ARM)
#define DEBUG_STARTUP
#define	XPUTC(n) 	mov r0, n; bl _C_LABEL(debugputc)
#else
#define	XPUTC(n)
#endif

#ifndef SDRAM_START
#define SDRAM_START CSD0DDR_BASE
#endif
#define KERNEL_TEXT_ADDR	(SDRAM_START+0x00100000)

#define INIT_MEMSIZE	128
#define TEMP_L1_TABLE	(SDRAM_START + INIT_MEMSIZE * 0x100000 - L1_TABLE_SIZE)

#ifdef	DEBUG_STARTUP
#define	CHECKPOINT(n)	CHECKPOINT2(n,r0,r1)
#define	CHECKPOINT2(n,ra,rb)		\
	mov ra,#0x30+(n); 		\
	ldr	rb, =UART1_BASE;	\
	str	ra, [rb, #IMX_UTXD]
#else
#define	CHECKPOINT(n)		/* nothing to do */
#define	CHECKPOINT2(n,ra,rb)	/* nothing to do */
#endif

	.section .start,"ax",%progbits

	.text

	.global _C_LABEL(netwalker_start)
_C_LABEL(netwalker_start):
	CHECKPOINT(0)

#ifdef	DEBUG_STARTUP
	ldr	sp,=SDRAM_START+0x4000

	bl	newline
	ldr	r0, =0xdeadb01f
	bl	_C_LABEL(debugprintx)
	bl	newline

	mov	r0, pc
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c0, c0, 3	/* read TLB type register */
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c1, c0, 0	/* read control register */
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c2, c0, 0	/* read TTB0 */
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c2, c0, 1	/* read TTB1 */
	bl	_C_LABEL(debugprintx)
	bl	newline

	ldr	r0, =0xbabeface
	bl	_C_LABEL(debugprintx)
	bl	newline

	/* dump some of UART1 registers to know clock frequency */
	ldr	r4,=UART1_BASE
	ldr	r0,[r4,#IMX_UBMR]
	bl	_C_LABEL(debugprintx)
	bl	newline
	ldr	r0,[r4,#IMX_UBIR]
	bl	_C_LABEL(debugprintx)
	bl	newline
	ldr	r0,[r4,#IMX_UFCR]
	bl	_C_LABEL(debugprintx)
	bl	newline
#endif	/* DEBUG_STARTUP */

	/* Are we running on right place ? */
	ldr	r2, =KERNEL_TEXT_ADDR
	adr	r0, _C_LABEL(netwalker_start)
	cmp	r0, r2
	beq	relocated

	/*
	 * move me to RAM
	 */
	ldr	r1, .Lcopy_size
	add	r1, r1, #3
	mov	r1, r1, LSR #2
	mov	r4, r2

	bhs	 5f

	/* src < dest. copy from top */
	add r0,r0,r1,LSL #2
	add r2,r2,r1,LSL #2

3:	ldr r3,[r0,#-4]!
	str r3,[r2,#-4]!
	subs r1,r1,#1
	bhi 3b
	b 7f

	/* src >= dest. copy from bottom */
5:	ldr r3,[r0],#4
	str r3,[r2],#4
	subs r1,r1,#1
	bhi 5b

7:


	/*
	 * Okay, we are finished relocating the text segment.  Now
	 * we need to leap to the next instruction.
	 */
	ldr	r0, .Lrelocate_address
	ldr	r1, .Lrelocate_offset
	add	pc, r0, r1

.Lrelocate_offset:	.word relocated-_C_LABEL(netwalker_start)

relocated:
	CHECKPOINT(1)

	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid   if, #PSR_SVC32_MODE

	bl	cortex_init

	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	adr	r1, .Lmmu_init_table
	bl	arm_boot_l1pt_init

	CHECKPOINT(2)
	/*
	 * Turn on the MMU, Caches, etc.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	bl	arm_cpuinit
	CHECKPOINT(3)

	movw	ip, #:lower16:start
	movt	ip, #:upper16:start
	bx	ip		/* Jump to start (flushes pipeline). */

	/* NOTREACHED */

/*
 * Calculate size of kernel to copy.  Don't bother to copy bss,
 * although I guess the CPU could use the warmup exercise ...
 */
.Lcopy_size:
        .word	_edata - _C_LABEL(netwalker_start)

.Lrelocate_address:
	.word	KERNEL_BASE_phys

#include "opt_console.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include <arm/cortex/scu_reg.h>
#include "assym.h"

// Macro to call routines in .text
#if defined(KERNEL_BASES_EQUAL)
#define CALL(f)		bl	_C_LABEL(f)
#else
#define	CALL(f)	\
	movw	fp, #:lower16:_C_LABEL(f); \
	movt	fp, #:upper16:_C_LABEL(f); \
	sub	fp, fp, #KERNEL_BASE_VOFFSET; \
	blx	fp
#endif


// We'll modify va and pa at run time so we can use relocatable addresses.
#define MMU_INIT(va,pa,n_sec,attr) \
	.word	((va) & 0xffffffff)|(n_sec)		    ; \
	.word	((pa) & 0xffffffff)|(attr)		    ; \

// Set up a preliminary mapping in the MMU to allow us to run at KERNEL_BASE
// with caches on.  If we are MULTIPROCESSOR, save the TTB address.
//
arm_boot_l1pt_init:

	mov	ip, r1			// save mmu table addr
	// Build page table from scratch
	mov	r1, r0			// Start address to clear memory.
	// Zero the entire table so all virtual addresses are invalid.
	add	r2, r1, #L1_TABLE_SIZE	// Ending address
	mov	r4, #0
	mov	r5, #0
	mov	r6, #0
	mov	r7, #0
1:	stmia	r1!, {r4-r7}		// 16 bytes at a time
	cmp	r1, r2
	blt	1b

	// Now create our entries per the mmu_init_table.
	l1table	.req r0
	va	.req r1
	pa	.req r2
	n_sec	.req r3
	attr	.req r4
	itable	.req r5

	mov	attr, #0
	mrc	p15, 0, r3, c0, c0, 5	// MPIDR read
	cmp	r3, #0			// not zero?
	movne	attr, #L1_S_V6_S	//    yes, shareable attribute
	mov	itable, ip		// reclaim table address
	b	3f

2:	str	pa, [l1table, va, lsl #2]
	add	va, va, #1
	add	pa, pa, #(L1_S_SIZE)
	subs	n_sec, n_sec, #1
	bhi	2b

3:	ldmia	itable!, {va, pa}
	// Convert va to l1 offset:	va = 4 * (va >> L1_S_SHIFT)
	ubfx	n_sec, va, #0, #L1_S_SHIFT
	lsr	va, va, #L1_S_SHIFT

	// Do we need add sharing for this?
	tst	pa, #(L1_S_C|L1_S_B)	// is this entry cacheable?
	orrne	pa, pa, attr		// add sharing

4:	cmp	n_sec, #0
	bne	2b
	bx	lr			// return

	.unreq	va
	.unreq	pa
	.unreq	n_sec
	.unreq	attr
	.unreq	itable
	.unreq	l1table

//
// Coprocessor register initialization values
//
#undef CPU_CONTROL_SWP_ENABLE		// not present on A8
#define CPU_CONTROL_SWP_ENABLE		0
#ifdef __ARMEL__
#define CPU_CONTROL_EX_BEND_SET		0
#else
#define CPU_CONTROL_EX_BEND_SET		CPU_CONTROL_EX_BEND
#endif
#ifdef ARM32_DISABLE_ALIGNMENT_FAULTS
#define CPU_CONTROL_AFLT_ENABLE_CLR	CPU_CONTROL_AFLT_ENABLE
#define CPU_CONTROL_AFLT_ENABLE_SET	0
#else
#define CPU_CONTROL_AFLT_ENABLE_CLR	0
#define CPU_CONTROL_AFLT_ENABLE_SET	CPU_CONTROL_AFLT_ENABLE
#endif

// bits to set in the Control Register
//
#define CPU_CONTROL_SET \
	(CPU_CONTROL_MMU_ENABLE		|	\
	 CPU_CONTROL_AFLT_ENABLE_SET	|	\
	 CPU_CONTROL_DC_ENABLE		|	\
	 CPU_CONTROL_SWP_ENABLE		|	\
	 CPU_CONTROL_BPRD_ENABLE	|	\
	 CPU_CONTROL_IC_ENABLE		|	\
	 CPU_CONTROL_EX_BEND_SET	|	\
	 CPU_CONTROL_UNAL_ENABLE)

// bits to clear in the Control Register
//
#define CPU_CONTROL_CLR \
	(CPU_CONTROL_AFLT_ENABLE_CLR	|	\
	 CPU_CONTROL_TR_ENABLE)

arm_cpuinit:
	// Because the MMU may already be on do a typical sequence to set
	// the Translation Table Base(s).
	mov	ip, lr
	mov	r10, r0			// save TTBR
	mov	r1, #0

	mcr     p15, 0, r1, c7, c5, 0	// invalidate I cache

	mrc	p15, 0, r2, c1, c0, 0	// SCTLR read
	movw	r1, #(CPU_CONTROL_DC_ENABLE|CPU_CONTROL_IC_ENABLE)
	bic	r2, r2, r1		// clear I+D cache enable

#ifdef __ARMEB__
	// SCTLR.EE determines the endianness of translation table lookups.
	// So we need to make sure it's set before starting to use the new
	// translation tables (which are big endian).
	//
	orr	r2, r2, #CPU_CONTROL_EX_BEND
	bic	r2, r2, #CPU_CONTROL_MMU_ENABLE
	pli	[pc, #32]		// preload the next few cachelines
	pli	[pc, #64]
	pli	[pc, #96]
	pli	[pc, #128]
#endif

	mcr	p15, 0, r2, c1, c0, 0	// SCTLR write

	XPUTC(#'F')
	dsb				// Drain the write buffers.

	XPUTC(#'G')
	mrc	p15, 0, r1, c0, c0, 5	// MPIDR read
	cmp	r1, #0
	orrlt	r10, r10, #TTBR_MPATTR	// MP, cachable (Normal WB)
	orrge	r10, r10, #TTBR_UPATTR	// Non-MP, cacheable, normal WB
	XPUTC(#'0')
	mcr	p15, 0, r10, c2, c0, 0	// TTBR0 write
#if defined(ARM_MMU_EXTENDED)
	// When using split TTBRs, we need to set both since the physical
	// addresses we were/are using might be in either.
	XPUTC(#'1')
	mcr	p15, 0, r10, c2, c0, 1	// TTBR1 write
#endif

	XPUTC(#'H')
#if defined(ARM_MMU_EXTENDED)
	XPUTC(#'1')
	mov	r1, #TTBCR_S_N_1	// make sure TTBCR_S_N is 1
#else
	XPUTC(#'0')
	mov	r1, #0			// make sure TTBCR is 0
#endif
	mcr	p15, 0, r1, c2, c0, 2	// TTBCR write

	isb

	XPUTC(#'I')
	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0	// TLBIALL (just this core)
	dsb
	isb

	XPUTC(#'J')
	mov	r1, #0			// get KERNEL_PID
	mcr	p15, 0, r1, c13, c0, 1	// CONTEXTIDR write

	// Set the Domain Access register.  Very important!
	XPUTC(#'K')
	mov     r1, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r1, c3, c0, 0	// DACR write

	//
	// Enable the MMU, etc.
	//
	XPUTC(#'L')
	mrc	p15, 0, r1, c1, c0, 0	// SCTLR read

	movw	r3, #:lower16:CPU_CONTROL_SET
#if (CPU_CONTROL_SET & 0xffff0000)
	movt	r3, #:upper16:CPU_CONTROL_SET
#endif
	orr	r0, r1, r3
	bic	r0, r0, #CPU_CONTROL_CLR
	//cmp	r0, r1			// any changes to SCTLR?
	//bxeq	ip			//    no, then return.

	pli	1f
	dsb

	// turn mmu on!
	//
	mov	r0, r0			// fetch instruction cacheline
1:	mcr	p15, 0, r0, c1, c0, 0	// SCTLR write

	// Ensure that the coprocessor has finished turning on the MMU.
	//
	mrc	p15, 0, r0, c0, c0, 0	// Read an arbitrary value.
	mov	r0, r0			// Stall until read completes.
	XPUTC(#'M')

	bx	ip			// return

	.p2align 2

#if defined(VERBOSE_INIT_ARM) && defined(XPUTC_COM)
#define TIMO		0x25000
#ifndef COM_MULT
#define COM_MULT	1
#endif
xputc:
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
	bx	lr
#endif /* VERBOSE_INIT_ARM */

//
// Perform the initialization of the Cortex core required by NetBSD.
//
//
cortex_init:
	mov	r10, lr				// save lr
	mov	r9, sp				// save sp


1:
	cpsid	if, #PSR_SVC32_MODE		// SVC32 with no interrupts

2:
	mov	r0, #0
	msr	spsr_sxc, r0			// set SPSR[23:8] to known value

	mov	sp, r9				// restore sp

#if 0
	mrc	p14, 0, r0, c0, c0, 0		// MIDR read
	ufbx	r0, r0, #4, #4			// extract cortex part.
	mov	r5, #1
	lsl	r5, r5, r0
#endif

	XPUTC(#'@')

	mrc	p15, 0, r4, c1, c0, 0		// SCTLR read

	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		// toss i-cache


	//
	// Step 1b, invalidate the data cache
	//
	XPUTC(#'B')
	CALL(armv7_dcache_wbinv_all)
	XPUTC(#'C')

	//
	// Check to see if we are really MP before enabling SMP mode
	//
	mrc	p15, 0, r1, c0, c0, 5		// MPIDR get
	ubfx	r1, r1, #30, #2			// get MP bits
	cmp	r1, #2				// is it MP?
	bxne	r10				//    no, return

	//
	// Step 2, disable the data cache
	//
	mrc	p15, 0, r2, c1, c0, 0		// SCTLR read
	bic	r2, r2, #CPU_CONTROL_DC_ENABLE	// clear data cache enable
	mcr	p15, 0, r2, c1, c0, 0		// SCTLR write
	isb
	XPUTC(#'1')

	//
	// Step 4b, restore SCTLR (enable the data cache)
	//
	orr	r4, r4, #CPU_CONTROL_IC_ENABLE	// enable icache
	orr	r4, r4, #CPU_CONTROL_DC_ENABLE	// enable dcache
	mcr	p15, 0, r4, c1, c0, 0		// SCTLR write
	isb
	XPUTC(#'-')

	bx	r10
ASEND(cortex_init)


	.global	cortex_mpstart
	.type	cortex_mpstart,%object

#ifdef VERBOSE_INIT_ARM
	.pushsection .bss
	/* temporary stack for secondary CPUs (for XPUTC) */
#define BOOT_STACKSIZE	256
	.align	3
	.space	BOOT_STACKSIZE * (MAXCPUS - 1)
bootstk_cpus:
	.popsection
#endif

cortex_mpstart:
	//
	// If not MULTIPROCESSOR, drop CPU into power saving state.
	//
3:	wfi
	b	3b
ASEND(cortex_mpstart)


.Lmmu_init_table:
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000, 1 << (32 - L1_S_SHIFT),
		L1_S_PROTO_armv7 | L1_S_APv7_KRW)

	/* Map memory 1:1 VA to PA, write-back cacheable, shareable */
	MMU_INIT(SDRAM_START, SDRAM_START, 512,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, SDRAM_START, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

	/*
	 * In case of early start debugging it might be useful to map
	 * SoC registers (for UART access).
	 */
	MMU_INIT(NETWALKER_IO_VBASE0, NETWALKER_IO_PBASE0, 4,
		L1_S_PROTO | L1_S_APv7_KRW | L1_S_V6_XN)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

#ifdef	DEBUG_STARTUP
.Luart0adr:
	.word	UART1_BASE

	.global _C_LABEL(debugprintx)
_C_LABEL(debugprintx):
	stmfd	sp!, {r0, r3, lr}

	mov	r3, r0

	mov	r0, #'0'
	bl	debugputc
	mov	r0, #'x'
	bl	debugputc

	bl	print_r3

	ldmfd	sp!, {r0, r3, pc}

	.global	_C_LABEL(debugprint)
_C_LABEL(debugprint):
	stmfd	sp!, {r0, r1, lr}
	mov	r1, r0
1:
	ldrb	r0, [r1], #1
	cmp	r0, #0
	beq	9f
	bl	debugputc
	b	1b
9:
	ldmfd	sp!, {r0, r1, pc}

print_r3:
	stmfd	sp!, {r0, r3-r6, lr}
	mov	r4, #28
	mov	r5, #0xf
1:
	and	r6, r5, r3, ROR r4
	cmp	r6, #10
	addlt	r0, r6, #'0'
	addge	r0, r6, #('a' - 0x0a)
	bl	debugputc
	subs	r4, r4, #4
	bge	1b
	ldmfd	sp!, {r0, r3-r6, pc}

	.global	_C_LABEL(debugputc)
_C_LABEL(debugputc):
	stmfd	sp!, {r0, r1, r2, lr}

	ldr	r1, .Luart0adr

1:	/* wait */
	ldr	r2, [r1, #0x98]
	tst	r2, #0x4000
	beq	1b

	/* output */
	strb	r0, [r1, #0x40]

	ldmfd	sp!, {r0, r1, r2, pc}

newline:
	mov	r1, lr
	mov	r0, #0x0d
	bl	_C_LABEL(debugputc)
	mov	lr, r1
	mov	r0, #0x0a
	b	_C_LABEL(debugputc)

#endif	/* DEBUG_STARTUP */

END(_C_LABEL(netwalker_start))
