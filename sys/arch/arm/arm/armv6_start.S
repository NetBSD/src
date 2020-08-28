/*	$NetBSD: armv6_start.S,v 1.27 2020/08/28 13:36:52 skrll Exp $	*/

/*-
 * Copyright (c) 2012, 2017, 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Nick Hudson.
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

#include "opt_arm_debug.h"
#include "opt_console.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_fdt.h"
#include "opt_kasan.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#if defined(CONSADDR) && defined(CONADDR)
#error Only one of CONSADDR and CONADDR should be defined
#endif

#if defined(CONSADDR)
#define START_CONSADDR	CONSADDR
#endif
#if defined(CONADDR)
#define START_CONSADDR	CONADDR
#endif

#if defined(VERBOSE_INIT_ARM)
#define XPUTC(n)	mov r0, n; bl uartputc
#define VPRINTF(string)	bl generic_vprint; .asciz string; .align 2
#define VPRINTX(regno)	mov r0, regno; bl generic_printx
#else
#define XPUTC(c)	/* nothing */
#define VPRINTF(string)	/* nothing */
#define VPRINTX(regno)	/* nothing */
#endif

#if defined(FDT)
#define MD_CPU_HATCH	arm_fdt_cpu_hatch
#endif

/*
 * A generic kernel start routine.
 *
 * At this point, this code has been loaded into SDRAM and the MMU should be off
 * with data caches disabled.
 *
 * linux image type should be used in uboot images to ensure this is the case.
 */

	// Use callee saved registers
	R_L1TABLE	.req r4
	R_VA		.req r5
	R_PA		.req r6
	R_NSEC		.req r7
	R_ATTR		.req r8
	R_DEVATTR	.req r9

	R_TMP1		.req r8
	R_TMP2		.req r9
	R_VTOPDIFF	.req r10
	R_FDTADDR	.req r11
	R_INDEX		.req r11

	.text

ENTRY_NP(generic_start)

	// ARMv7 only?!?
#if defined(__ARMEB__)
	setend	be			/* force big endian */
#endif

	/* disable IRQs/FIQs. */
	cpsid	if

	adr	R_TMP1, generic_start
	ldr	R_VTOPDIFF, =generic_start
	sub	R_VTOPDIFF, R_VTOPDIFF, R_TMP1

	ldr	R_TMP1, =start_stacks_top
	sub	sp, R_TMP1, R_VTOPDIFF

	mov	r4, r0
	mov	r5, r1
	mov	r6, r2
	mov	r7, r3

	// We can now call functions

	VPRINTF("\n\rpc    : ")
	VPRINTX(pc)

	VPRINTF("\n\roff   : ")
	VPRINTX(R_VTOPDIFF)

	VPRINTF("\n\rsp    : ")
	VPRINTX(sp)

	ldr	R_TMP1, =(L1_S_SIZE - 1)
	ands	R_TMP2, R_VTOPDIFF, R_TMP1
	bne	arm_bad_vtopdiff

#if defined(FDTBASE)
	/*
	 * ARM boot protocol has FDT address in r2 which is now in r6
	 */
	VPRINTF("\n\rfdt   : ")
	mov	R_FDTADDR, r6		// Save fdt_addr_r for mapping later

	VPRINTX(r6)
#endif

#if defined(VERBOSE_INIT_ARM)
	VPRINTF("\n\rmidr  : ")
	mrc	p15, 0, r0, c0, c0, 0	// MIDR
	VPRINTX(r0)
	VPRINTF("\n\rrevidr: ")
	mrc	p15, 0, r0, c0, c0, 6	// REVIDR
	VPRINTX(r0)
	VPRINTF("\n\rmpidr : ")
	mrc	p15, 0, r0, c0, c0, 5	// MPIDR
	VPRINTX(r0)
	VPRINTF("\n\rttb0  : ")
	mrc	p15, 0, r0, c2, c0, 0	// TTBR0 read
	VPRINTX(r0)
	VPRINTF("\n\rttb1  : ")
	mrc	p15, 0, r0, c2, c0, 1	// TTBR1 read
	VPRINTX(r0)
	VPRINTF("\n\rttcr  : ")
	mrc	p15, 0, r0, c2, c0, 2	// TTBCR read
	VPRINTX(r0)
	VPRINTF("\n\r")
#endif

#if defined(_ARM_ARCH_7)
	b	generic_startv7
#elif defined(_ARM_ARCH_6)
	b	generic_startv6
#else
#error Unsupported CPU
#endif
arm_bad_vtopdiff:
	VPRINTF("\n\rpanic: vtop not L1_FRAME aligned (")
	VPRINTX(R_VTOPDIFF)
	VPRINTF(")\n\r")
1:	b	 1b
ASEND(generic_start)

/*
 * Save the u-boot arguments (including FDT address) and the virtual to physical
 * offset.
 *
 * Uses the following callee saved registers:
 *
 * 	r8 (R_TMP1), r9 (R_TMP2)
 */
generic_savevars:
	mov	R_TMP1, lr
	/*
	 * Store virtual to physical address difference
	 */
	ldr	R_TMP2, =kern_vtopdiff
	sub	R_TMP2, R_TMP2, R_VTOPDIFF
	str	R_VTOPDIFF, [R_TMP2]

	/*
	 * store uboot arguments to uboot_args[4]
	 */
	ldr	R_TMP2, =uboot_args
	sub	R_TMP2, R_VTOPDIFF

	VPRINTF("\n\ruboot : ")
	VPRINTX(R_TMP2)
	str	r4, [R_TMP2, #(4*0)]
	str	r5, [R_TMP2, #(4*1)]
	str	r6, [R_TMP2, #(4*2)]
	str	r7, [R_TMP2, #(4*3)]

#if defined(FDTBASE)
	/*
	 * ARM boot protocol has FDT address in r2 which is now in r6
	 */
	VPRINTF("\n\rfdt   : ")
	ldr	R_TMP2, =fdt_addr_r
	sub	R_TMP2, R_VTOPDIFF
	str	r6, [R_TMP2]

	VPRINTX(r6)
#endif

	RETr(R_TMP1)

	.ltorg

	/*
	 * Allocate some memory after the kernel image for stacks and
	 * bootstrap L1PT
	 */
	.section "_init_memory", "aw", %nobits
	.p2align INIT_ARM_STACK_SHIFT
	.global start_stacks_bottom
	.global start_stacks_top
start_stacks_bottom:
	.space	INIT_ARM_TOTAL_STACK
start_stacks_top:

	.section "_init_memory", "aw", %nobits
	.p2align  14 /* 16KiB aligned */
	.global ARM_BOOTSTRAP_LxPT
ARM_BOOTSTRAP_LxPT:
TEMP_L1_TABLE:
	.space  L1_TABLE_SIZE
TEMP_L1_TABLE_END:

	.text
	.align  2

arm_build_translation_table:
	push	{r0, lr}
	/*
	 * Initialise the l1pt for identity mapping of the kernel with caches
	 * off. This will be updated further with additional mappings to form
	 * the bootstrap table.
	 */
	ldr	R_L1TABLE, =TEMP_L1_TABLE
	sub	R_L1TABLE, R_VTOPDIFF

	// PA of kernel rounded down to nearest L1_S boundary
	adr	R_PA, generic_start
	ldr	r0, =(L1_S_SIZE - 1)
	bic	R_PA, R_PA, r0

	// attribute to map kernel
	ldr     R_ATTR, =(L1_S_PROTO | L1_S_AP_KRW | L1_S_NOCACHE)
	bl	arm_boot_l1pt_init

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE_VIRT (determined from generic_start) with caches off.
	 */
	ldr	R_L1TABLE, =TEMP_L1_TABLE
	sub	R_L1TABLE, R_VTOPDIFF

	// Calculate kernel size plus 1M to grow into
	ldr	r0, =generic_start	// kernel start VA
	ldr	r1, =TEMP_L1_TABLE_END	// kernel end VA
	add	r1, #L1_S_SIZE		// Make sure we have 1M to grow into

	ldr	r2, =(L1_S_SIZE - 1)

	bic	r0, r2			// trunc kernel start to 1M boundary
	add	r1, r2
	bic	r1, r2			// round kernel end to 1M boundary

	sub	r1, r0			// kernel size plus 1M to grow into
	mov	R_NSEC, r1, lsr #(L1_S_SHIFT)

	ldr	R_VA, =generic_start	// VA of kernel
	bic	R_VA, r2		// ...rounded down to L1_S boundary
	adr	R_PA, generic_start	// PA of kernel
	bic	R_PA, r2		// ...rounded down to L1_S boundary

	// attribute to map kernel
	ldr     R_ATTR, =(L1_S_PROTO | L1_S_AP_KRW | L1_S_NOCACHE)
	bl	arm_boot_l1pt_entry

#if defined(FDTBASE)
	VPRINTF("DTB")

	/*
	 * Add DTB identity mapping (1MB) from originaly r2 (but saved in
	 * R_FDTADDR)
	 */
	ldr	r0, =(L1_S_SIZE - 1)		/* align DTB PA to 1M */
	bic	R_VA, R_FDTADDR, r0
	mov	R_PA, R_VA
	mov	R_NSEC, #1			/* 1MB mapping */

	ldr	R_L1TABLE, =TEMP_L1_TABLE
	sub	R_L1TABLE, R_VTOPDIFF
	bl	arm_boot_l1pt_entry
#endif

#if defined(START_CONSADDR)
	/* If START_CONSADDR exists add its identity mapping (1MB) */
	VPRINTF("\n\rCONSADDR")
	ldr	r0, =(L1_S_SIZE - 1)		/* align DTB PA to 1M */
	ldr	R_VA, =START_CONSADDR
	bic	R_VA, R_VA, r0
	mov	R_PA, R_VA
	mov	R_NSEC, #1

	ldr     R_ATTR, =(L1_S_PROTO | L1_S_AP_KRW)
	orr	R_ATTR, R_DEVATTR

	ldr	R_L1TABLE, =TEMP_L1_TABLE
	sub	R_L1TABLE, R_VTOPDIFF
	bl	arm_boot_l1pt_entry
#endif

	XPUTC(#'M')

	pop	{r0, pc}

	.ltorg

/*
 * Set up a preliminary mapping in the MMU to allow us to run at
 * KERNEL_BASE_VIRT (determined from generic_start)
 *
 * On Entry
 *
 * R_L1TABLE	is the PA of the temporary L1PT
 * R_ATTR	is the attribute bits to set for each section mapping
 *
 * No R_VA/R_PA/R_NSEC needed here as we use 'generic_start' and
 * 'TEMP_L1_TABLE_END' to calculate the initial direct VA:PA mapping
 */

/* We push r0 to maintain stack alignment only */
arm_boot_l1pt_init:
	push	{r0, lr}

	// Start address to clear memory.
	mov	r0, R_L1TABLE

	// Zero the entire table so all virtual addresses are invalid.
	add	r1, r0, #L1_TABLE_SIZE	// Ending address
	mov	r2, #0
	mov	r3, #0
1:	stmia	r0!, {r2-r3}
	stmia	r0!, {r2-r3}		// 16 bytes per loop
	cmp	r0, r1
	blt	1b

	// Calculate the size of the kernel in L1_S_SIZE sections
	ldr	r0, =generic_start
	ldr	r1, =TEMP_L1_TABLE_END

	ldr	r2, =(L1_S_SIZE - 1)

	bic	r0, r2			// trunc kernel start to 1M boundary
	add	r1, r2
	bic	r1, r2			// round kernel end to 1M boundary

	sub	r1, r0			// kernel size plus 1M to grow into
	mov	R_NSEC, r1, lsr #(L1_S_SHIFT)

	// identity mapping for size of kernel
	adr	R_VA, generic_start
	bic	R_VA, r2
	mov	R_PA, R_VA

	pop	{r0, lr}
	/* Fallthrough */

// We push r0 to maintain stack alignment only
arm_boot_l1pt_entry:
	push	{r0, lr}

	VPRINTF("\n\r")
	VPRINTX(R_L1TABLE)

	VPRINTF(" va:")
	VPRINTX(R_VA)

	VPRINTF(" pa:")
	VPRINTX(R_PA)

	VPRINTF(" nsec:")
	VPRINTX(R_NSEC)

	VPRINTF(" attr:")
	VPRINTX(R_ATTR)

	VPRINTF("\n\r")
	lsr	R_VA, R_VA, #L1_S_SHIFT
	orr	R_PA, R_ATTR

2:
	VPRINTX(R_L1TABLE)
	XPUTC('[')
	VPRINTX(R_VA)
	XPUTC(']')
	XPUTC('=')
	VPRINTX(R_PA)

	ldr	r0, [R_L1TABLE, R_VA, lsl #2]
	cmp	r0, #0
	cmpne	r0, R_PA
	bne	arm_boot_overlap

	str	R_PA, [R_L1TABLE, R_VA, lsl #2]
	add	R_VA, R_VA, #1
	add	R_PA, R_PA, #(L1_S_SIZE)

	VPRINTF("\n\r")

	subs	R_NSEC, R_NSEC, #1
	bhi	2b

	pop	{r0, pc}

arm_boot_overlap:
	VPRINTF("\n\rpanic: overlapping mappings\n\r")
3:
	b	3b

#if defined(_ARM_ARCH_7)
generic_startv7:

	.arch		armv7a
	.arch_extension	sec
	.arch_extension	virt

	VPRINTF("v7    : ")

	bl	armv7_init
	bl	generic_savevars

	mov	R_DEVATTR, #L1_S_V6_XN
	bl	arm_build_translation_table

	/*
	 * Turn on the MMU.  Return to virtual address space.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	sub	r0, R_VTOPDIFF

	// Return to virtual addess after the call to armv7_mmuinit
	movw	lr, #:lower16:generic_vstartv7
	movt	lr, #:upper16:generic_vstartv7
	b	armv7_mmuinit

generic_vstartv7:
	// Stack to KVA address
	add	sp, sp, R_VTOPDIFF

	VPRINTF("virtual\n\r")

#if defined(KASAN)
	ldr	r0, =start_stacks_bottom
	bl	_C_LABEL(kasan_early_init)

	VPRINTF("kasan\n\r")
#endif

	/* r0 = &cpu_info_store[0] */
	movw	r0, #:lower16:cpu_info_store
	movt	r0, #:upper16:cpu_info_store

	mrc	p15, 0, r1, c0, c0, 0	// MIDR get
	str	r1, [r0, #CI_MIDR]
	mrc	p15, 0, r1, c0, c0, 5	// MPIDR get
	str	r1, [r0, #CI_MPIDR]

	bl	arm_cpu_topology_set

	VPRINTF("go\n\r")

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	b	start

	/* NOTREACHED */
	.ltorg
#elif defined(_ARM_ARCH_6)
generic_startv6:
	VPRINTF("v6    : ")

	bl	armv6_init
	bl	generic_savevars

#if defined(ARM_MMU_EXTENDED)
	mov	R_DEVATTR, #L1_S_V6_XN
#else
	mov	R_DEVATTR, #0
#endif
	bl	arm_build_translation_table

	XPUTC(#'E')
	/*
	 * Turn on the MMU.  Return to new enabled address space.
	 */
	ldr	r0, =TEMP_L1_TABLE
	sub	r0, R_VTOPDIFF

	ldr	lr, =generic_vstartv6
	b	armv6_mmuinit

generic_vstartv6:
	// Stack to KVA address
	add	sp, sp, R_VTOPDIFF

	VPRINTF("virtual\n\r")

#if defined(KASAN)
	ldr	r0, =start_stacks_bottom
	bl	_C_LABEL(kasan_early_init)

	VPRINTF("kasan\n\r")
#endif

	VPRINTF("go\n\r")

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	b	start

	/* NOTREACHED */
	.ltorg

#endif

#if defined(_ARM_ARCH_7)

//
// SCTLR register initialization values
//
#if defined(__ARMEL__)
#define CPU_CONTROL_EX_BEND_SET		0
#else
#define CPU_CONTROL_EX_BEND_SET		CPU_CONTROL_EX_BEND
#endif

#if defined(ARM32_DISABLE_ALIGNMENT_FAULTS)
#define CPU_CONTROL_AFLT_ENABLE_CLR	CPU_CONTROL_AFLT_ENABLE
#define CPU_CONTROL_AFLT_ENABLE_SET	0
#else
#define CPU_CONTROL_AFLT_ENABLE_CLR	0
#define CPU_CONTROL_AFLT_ENABLE_SET	CPU_CONTROL_AFLT_ENABLE
#endif

#if defined(ARM_MMU_EXTENDED)
#define CPU_CONTROL_XP_ENABLE_CLR	0
#define CPU_CONTROL_XP_ENABLE_SET	CPU_CONTROL_XP_ENABLE
#else
#define CPU_CONTROL_XP_ENABLE_CLR	CPU_CONTROL_XP_ENABLE
#define CPU_CONTROL_XP_ENABLE_SET	0
#endif

// bits to set in the Control Register
//
#define CPU_CONTROL_SET			( 	\
	CPU_CONTROL_MMU_ENABLE		|	\
	CPU_CONTROL_UNAL_ENABLE		|	\
	CPU_CONTROL_EX_BEND_SET		|	\
	CPU_CONTROL_AFLT_ENABLE_SET	|	\
	CPU_CONTROL_XP_ENABLE_SET	|	\
	0)

// bits to clear in the Control Register
//
#define CPU_CONTROL_CLR			(	\
	CPU_CONTROL_AFLT_ENABLE_CLR	|	\
	CPU_CONTROL_XP_ENABLE_CLR	|	\
	0)

/*
 * Perform the initialization of the an ARMv7 core required by NetBSD.
 *
 * Uses the following callee saved registers:
 *
 * 	r8 (R_TMP1), r9 (R_TMP2)
 */
armv7_init:

	.arch		armv7a
	.arch_extension	sec
	.arch_extension	virt

	mov	R_TMP1, lr
	mov	R_TMP2, sp

	/*
	 * Leave HYP mode and move into supervisor mode with IRQs/FIQs
	 * disabled.
	 */
	mrs	r0, cpsr
	and	r0, r0, #(PSR_MODE)	/* Mode is in the low 5 bits of CPSR */
	teq	r0, #(PSR_HYP32_MODE)	/* Hyp Mode? */
	bne	1f

	XPUTC('h')

	mov	sp, #0

	/* Set CNTVOFF to 0 */
	mov	r1, #0
	mcrr	p15, 4, r1, r1, c14

	/* Ensure that IRQ, and FIQ will be disabled after eret */
	mrs	r0, cpsr
	bic	r0, r0, #(PSR_MODE)
	orr	r0, r0, #(PSR_SVC32_MODE)
	orr	r0, r0, #(I32_bit | F32_bit)
	msr	spsr_cxsf, r0
	/* Exit hypervisor mode */
	adr	lr, 2f
	msr	elr_hyp, lr
	eret

1:
	cpsid	if, #PSR_SVC32_MODE		// SVC32 with no interrupts

2:
	mov	r0, #0
	msr	spsr_sxc, r0			// set SPSR[23:8] to known value

	mov	sp, R_TMP2

	XPUTC('A')

	mrc	p15, 0, r0, c1, c0, 0
	tst	r0, #CPU_CONTROL_DC_ENABLE
	blne	armv7_dcache_wbinv_all

	// TeX remap

#define ARMV7_SCTLR_CLEAR	( 	\
    CPU_CONTROL_IC_ENABLE |	\
    CPU_CONTROL_DC_ENABLE |	\
    CPU_CONTROL_MMU_ENABLE |	\
    CPU_CONTROL_BPRD_ENABLE |	\
    CPU_CONTROL_TR_ENABLE |	\
    0)

#define ARMV7_SCTLR_SET	( 	\
    CPU_CONTROL_UNAL_ENABLE |	\
    0)

	mrc	p15, 0, r0, c1, c0, 0
	movw	r1, #:lower16:ARMV7_SCTLR_CLEAR
	movt	r1, #:upper16:ARMV7_SCTLR_CLEAR
	movw	r2, #:lower16:ARMV7_SCTLR_SET
	movt	r2, #:upper16:ARMV7_SCTLR_SET

	mov	R_TMP2, r0			// save for printing
	bic	r0, r0, r1			// disable icache/dcache/mmu
	orr	r0, r0, r2			// enable unaligned access

	mcr	p15, 0, r0, c1, c0, 0		// SCTLR write
	dsb
	isb

	bl	armv7_dcache_inv_all
	mcr	p15, 0, r0, c7, c5,  0		/* ICIALLU */
	dsb
	isb

#if defined(VERBOSE_INIT_ARM)
	XPUTC(#'B')

	VPRINTF(" sctlr:")
	VPRINTX(R_TMP2)
	VPRINTF("/")
	mrc     p15, 0, r0, c1, c0, 0
	VPRINTX(r0)
	VPRINTF(" ")

	XPUTC(#'C')
#endif

	bx	R_TMP1				// return

	.ltorg

/*
 * Transitions the CPU to using the TTB passed in r0.
 *
 * Uses the following callee saved registers:
 *
 * Callee saved:
 * 	r4, r5
 */

armv7_mmuinit:
	// Because the MMU may already be on do a typical sequence to set
	// the Translation Table Base(s).
	mov	r4, lr
	mov	r5, r0			// save TTBR

	// XXXNH dsb - Why?
	XPUTC(#'F')
	dsb				// Drain the write buffers.

	XPUTC(#'G')
	mrc	p15, 0, r1, c0, c0, 5	// MPIDR read
	cmp	r1, #0
	orrlt	r5, r5, #TTBR_MPATTR	// MP, cachable (Normal WB)
	orrge	r5, r5, #TTBR_UPATTR	// Non-MP, cacheable, normal WB

	XPUTC(#'0')
	mcr	p15, 0, r5, c2, c0, 0	// TTBR0 write

#if defined(ARM_MMU_EXTENDED)
	// When using split TTBRs, we need to set both since the physical
	// addresses we were/are using might be in either.
	XPUTC(#'1')
	mcr	p15, 0, r5, c2, c0, 1	// TTBR1 write
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

	XPUTC(#'J')
	mov	r1, #0			// get KERNEL_PID
	mcr	p15, 0, r1, c13, c0, 1	// CONTEXTIDR write

	// XXXNH FreeBSD doesn't do this isb
	isb

	// Set the Domain Access register.  Very important!
	XPUTC(#'K')
	mov	r1, #DOMAIN_DEFAULT
	mcr	p15, 0, r1, c3, c0, 0	// DACR write

#if 0

/*
 * Set TEX remap registers
 *  - All is set to uncacheable memory
 */
	ldr	r0, =0xAAAAA
	mcr	CP15_PRRR(r0)
	mov	r0, #0
	mcr	CP15_NMRR(r0)
#endif

	XPUTC(#'I')
	mov	r1, #0
	mcr	p15, 0, r1, c8, c7, 0	// TLBIALL (just this core)
	dsb
	isb

	//
	// Enable the MMU, etc.
	//
	XPUTC(#'L')
	XPUTC(#'\n')
	XPUTC(#'\r')
	mrc	p15, 0, r1, c1, c0, 0	// SCTLR read

	movw	r3, #:lower16:CPU_CONTROL_SET
	movt	r3, #:upper16:CPU_CONTROL_SET
	movw	r2, #:lower16:CPU_CONTROL_CLR
	movt	r2, #:upper16:CPU_CONTROL_CLR
	orr	r0, r1, r3
	bic	r0, r0, r2

	mcr	p15, 0, r0, c1, c0, 0	/* SCTLR write */

	dsb
	isb

	mcr	p15, 0, r0, c8, c7, 0	/* TLBIALL - Flush TLB */
	mcr	p15, 0, r0, c7, c5, 6	/* BPIALL - Branch predictor invalidate all */
	dsb
	isb

	VPRINTF("MMU\n\r")
	bx	r4			// return

	.p2align 2

	.text

ENTRY_NP(cpu_mpstart)
#ifndef MULTIPROCESSOR
	//
	// If not MULTIPROCESSOR, drop CPU into power saving state.
	//
3:	wfi
	b	3b
#else
#if defined(__ARMEB__)
	setend	be				// switch to BE now
#endif

	/* disable IRQs/FIQs. */
	cpsid	if

	adr	R_TMP2, cpu_mpstart
	ldr	R_VTOPDIFF, =cpu_mpstart
	sub	R_VTOPDIFF, R_VTOPDIFF, R_TMP2

	mrc	p15, 0, r4, c0, c0, 5		// MPIDR get
	and	r4, #(MPIDR_AFF2|MPIDR_AFF1|MPIDR_AFF0)

	mov	r0, #0
	ldr	r1, =cpu_mpidr
	sub	r1, R_VTOPDIFF
1:
	ldr	r2, [r1, r0, lsl #2]		// r2 = cpu_mpidr[r0]
	cmp	r2, r4
	beq	2f				// found our mpidr

	add	r0, #1
	cmp	r0, #MAXCPUS
	bne	1b

	// Not found our mpidr in the list - use Aff0 for cpuindex
	and	r0, r4, #7
2:
	mov	R_INDEX, r0			// save cpu_index for later

	ldr	R_TMP1, =start_stacks_top
	sub	sp, R_TMP1, R_VTOPDIFF

	mov	r5, R_INDEX
	lsl	r5, #INIT_ARM_STACK_SHIFT
	sub	sp, sp, r5

#if defined(VERBOSE_INIT_ARM)
	VPRINTF("\n\rmidr  : ")
	mrc	p15, 0, r0, c0, c0, 0		// MIDR
	VPRINTX(r0)
	VPRINTF("\n\rrevidr: ")
	mrc	p15, 0, r0, c0, c0, 6		// REVIDR
	VPRINTX(r0)
	VPRINTF("\n\rmpidr : ")
	mrc	p15, 0, r0, c0, c0, 5		// MPIDR
	VPRINTX(r0)
#endif
	VPRINTF("\n\rindex : ")
	VPRINTX(R_INDEX)
	VPRINTF("\n\rsp    : ")
	VPRINTX(sp)
	XPUTC('\n')
	XPUTC('\r')

	// disables and clears caches
	bl	armv7_init

	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	sub	r0, R_VTOPDIFF

	movw	lr, #:lower16:armv7_mpcontinuation
	movt	lr, #:upper16:armv7_mpcontinuation
	b	armv7_mmuinit
ASEND(cpu_mpstart)

/*
 * Now running with real kernel VA via bootstrap tables
 */
armv7_mpcontinuation:
	// Stack to KVA address
	add	sp, sp, R_VTOPDIFF

	VPRINTF("virtual\n\r")

	// index into cpu_mpidr[] or cpu_number if not found
	mov	r0, R_INDEX
	bl	cpu_init_secondary_processor

	/*
	 * Wait for cpu_boot_secondary_processors
	 */

	/* r6 = &arm_cpu_mbox[0] */
	movw	r6, #:lower16:arm_cpu_mbox
	movt	r6, #:upper16:arm_cpu_mbox

	mov	r5, #1				// bitmask...
	lsl	r5, R_INDEX			// ... for our cpu

	/* wait for the mailbox start bit to become true */
1:	dmb					// data memory barrier
	ldr	r2, [r6]			// load mbox
	tst	r2, r5				// is our bit set?
	wfeeq					//    no, back to waiting
	beq	1b				//    no, and try again

	movw	r0, #:lower16:cpu_info
	movt	r0, #:upper16:cpu_info		// get pointer to cpu_info
	ldr	r5, [r0, R_INDEX, lsl #2]	// load our cpu_info
	ldr	r6, [r5, #CI_IDLELWP]		// get the idlelwp
	ldr	r7, [r6, #L_PCB]		// now get its pcb
	ldr	sp, [r7, #PCB_KSP]		// finally, we can load our SP
#if defined(TPIDRPRW_IS_CURCPU)
	mcr	p15, 0, r5, c13, c0, 4		// squirrel away curcpu()
#elif defined(TPIDRPRW_IS_CURLWP)
	mcr	p15, 0, r6, c13, c0, 4		// squirrel away curlwp()
#else
#error either TPIDRPRW_IS_CURCPU or TPIDRPRW_IS_CURLWP must be defined
#endif
	str	r6, [r5, #CI_CURLWP]		// and note we are running on it

	mov	r0, r5				// pass cpu_info
	mov	r1, R_INDEX			// pass cpu_index
	movw	r2, #:lower16:MD_CPU_HATCH	// pass md_cpu_hatch
	movt	r2, #:upper16:MD_CPU_HATCH	// pass md_cpu_hatch
	bl	_C_LABEL(cpu_hatch)
	b	_C_LABEL(idle_loop)		// never to return
ASEND(armv7_mpcontinuation)
#endif	// MULTIPROCESSOR

#elif defined(_ARM_ARCH_6)

ENTRY_NP(armv6_init)
/*
 * Workaround Erratum 411920
 *
 *	- value of arg 'reg' Should Be Zero
 */
#define Invalidate_I_cache(reg) \
	.p2align 5;								\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	mcr	p15, 0, reg, c7, c5, 0;	/* Invalidate Entire I cache */		\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;									\
	nop;

	mov	R_TMP1, lr
	mov	r0, #0			/* SBZ */
	Invalidate_I_cache(r0)

	mcr	p15, 0, r0, c7, c14, 0	/* Clean and Invalidate Entire Data Cache */

	ldr     r2, =(CPU_CONTROL_IC_ENABLE|CPU_CONTROL_DC_ENABLE)
					/* Disable I+D caches */
	mrc	p15, 0, r1, c1, c0, 0	/*  "       "   "     */
	mov	R_TMP2, r1
	bic	r1, r1, r2		/*  "       "   "     */
	mcr	p15, 0, r1, c1, c0, 0	/*  "       "   "     */

	mcr	p15, 0, r0, c7, c10, 4	/* Drain the write buffers. */

#if defined(VERBOSE_INIT_ARM)
	XPUTC(#'B')

	VPRINTF(" sctlr:")
	VPRINTX(R_TMP2)
	VPRINTF("/")
	mrc	p15, 0, r0, c1, c0, 0
	VPRINTX(r0)
	VPRINTF(" ")

	XPUTC(#'C')
#endif

	bx	R_TMP1

	.ltorg

armv6_mmuinit:
	mov	r4, lr
	mov	r5, r0

	XPUTC(#'0')
	mcr	p15, 0, r5, c2, c0, 0	// TTBR0 write

#if defined(ARM_MMU_EXTENDED)
	// When using split TTBRs, we need to set both since the physical
	// addresses we were/are using might be in either.
	XPUTC(#'1')
	mcr	p15, 0, r5, c2, c0, 1	/* TTBR1 write */
#endif

	XPUTC(#'H')
#if defined(ARM_MMU_EXTENDED)
	XPUTC(#'1')
	mov	r1, #TTBCR_S_N_1        /* make sure TTBCR_S_N is 1 */
#else
	XPUTC(#'0')
	mov	r1, #0			/* make sure TTBCR is 0 */
#endif
	mcr	p15, 0, r1, c2, c0, 2	/* TTBCR write */

	XPUTC(#'I')

	mov     r0, #0
	mcr	p15, 0, r0, c8, c7, 0	/* Invalidate TLBs */

	XPUTC(#'K')
	/* Set the Domain Access register.  Very important! */
	mov     r0, #DOMAIN_DEFAULT
	mcr	p15, 0, r0, c3, c0, 0

	/*
	 * Enable the MMU, etc.
	 */

#if defined(VERBOSE_INIT_ARM)
	VPRINTF(" sctlr:")
	mrc     p15, 0, r0, c1, c0, 0
	VPRINTX(r0)
	VPRINTF("/")
#endif

	mrc     p15, 0, r0, c1, c0, 0

	ldr     r1, Lcontrol_wax
	and	r0, r0, r1
	ldr     r1, Lcontrol_clr
	bic	r0, r0, r1
	ldr     r1, Lcontrol_set
	orr	r0, r0, r1

	mov	r6, r0
	VPRINTX(r6)
	VPRINTF(" ")

	.align 5
	/* turn mmu on! */
	mov	r0, r6
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Ensure that the coprocessor has finished turning on the MMU.
	 */
	mrc	p15, 0, r0, c0, c0, 0	/* Read an arbitrary value. */
	mov	r0, r0			/* Stall until read completes. */

	nop
	nop
	nop
	nop
	nop

	mov	pc, r4

	.ltorg

	/* bits to set in the Control Register */
Lcontrol_set:
#if defined(ARM_MMU_EXTENDED)
#define	CPU_CONTROL_EXTRA	CPU_CONTROL_XP_ENABLE
#else
#define	CPU_CONTROL_EXTRA	CPU_CONTROL_SYST_ENABLE
#endif
	.word CPU_CONTROL_MMU_ENABLE  | \
	      CPU_CONTROL_WBUF_ENABLE |    /* not defined in 1176 (SBO) */ \
	      CPU_CONTROL_32BP_ENABLE |    /* SBO */ \
	      CPU_CONTROL_32BD_ENABLE |    /* SBO */ \
	      CPU_CONTROL_LABT_ENABLE |    /* SBO */ \
		(1 << 16) | 	/* SBO - Global enable for data tcm */ \
		(1 << 18) |	/* SBO - Global enable for insn tcm */ \
	      CPU_CONTROL_UNAL_ENABLE | \
	      CPU_CONTROL_EXTRA

	/* bits to clear in the Control Register */
Lcontrol_clr:
	.word	0

	/* bits to "write as existing" in the Control Register */
Lcontrol_wax:
	.word	(3 << 30) | \
		(1 << 29) | \
		(1 << 28) | \
		(3 << 26) | \
		(3 << 19) | \
		(1 << 17) | \
	        (1 << 10)
#endif

ENTRY_NP(generic_vprint)
	push	{r4, lr}

	mov	r4, lr
	b	2f
1:
	bl	uartputc

2:
	ldrb	r0, [r4], #1
	cmp	r0, #0
	bne	1b

	add	lr, r4, #3
	bic	lr, #3

	pop	{r4}
	add	sp, sp, #4
	mov	pc, lr
ASEND(generic_vprint)

ENTRY_NP(generic_prints)
	push	{r4, lr}

	mov	r4, r0
1:
	ldrb	r0, [r4], #1
	cmp	r0, #0
	popeq	{r4, pc}

	bl	uartputc
	b	1b
ASEND(generic_prints)

ENTRY_NP(generic_printx)
	push	{r4, r5, r6, lr}
	mov	r5, r0

	mov	r0, #'0'
	bl	uartputc
	mov	r0, #'x'
	bl	uartputc

	// Word size in bits
	mov	r4, #32
1:
	sub	r4, r4, #4		// nibble shift
	lsr	r3, r5, r4		// extract ...
	and	r3, r3, #0xf		// ... nibble

	cmp	r3, #9
	add	r0, r3, #'0'
	addgt	r0, r3, #'a' - 10
	bl	uartputc

	cmp	r4, #0
	bne	1b
	pop	{r4, r5, r6, pc}
ASEND(generic_printx)
