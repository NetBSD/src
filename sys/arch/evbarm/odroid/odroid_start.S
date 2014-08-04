/*	$NetBSD: odroid_start.S,v 1.3 2014/08/04 18:14:43 reinoud Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#include "opt_exynos.h"
#include "opt_sscom.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"
#include "opt_arm_debug.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arch/arm/samsung/exynos_reg.h>
#include <arch/arm/samsung/exynos4_reg.h>
#include <arch/arm/samsung/exynos5_reg.h>

#include <evbarm/odroid/platform.h>

RCSID("$NetBSD: odroid_start.S,v 1.3 2014/08/04 18:14:43 reinoud Exp $")


#if defined(VERBOSE_INIT_ARM)

#define	XPUTC(n)	mov r0, n; bl xputc
#define	XPUTC2(n)	mov r0, n; blx r11
#ifdef __ARMEB__
#define COM_BSWAP
#endif
#else
#define	XPUTC(n)
#define	XPUTC2(n)
#endif

#define INIT_MEMSIZE	128

#define	TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET + INIT_MEMSIZE * 0x100000 - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(exynos_cpu_hatch)

/*
 * Kernel start routine for ODROID boards running on uboot firmware
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(odroid_start)
_C_LABEL(odroid_start):
#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif

	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid	if, #PSR_SVC32_MODE

	/*
	 * Save any arguments passed to us.  But since .start is not at
	 * 0x80000000 * but .text is, we can't directly use the address that
	 * the linker gave us directly.  Convert the virtual address to the
	 * physical address by using KERNEL_BASE_VOFFSET.
	 */
	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	stmia	r4, {r0-r3}			// Save the arguments

	/*
	 * Rescue passed "bootargs" env variable. This is not trivial
	 * since we can be booted using either `go' or trough `bootm'.
	 *
	 * 'go' passes R0 = argc, R1 = argv
	 * 'bootm' passes R0 = uboot_bootinfo, R3 = bootargs
	 */

	movw	r4, #:lower16:bootargs
	movt	r4, #:upper16:bootargs
	sub	r4, r4, #KERNEL_BASE_VOFFSET

	cmp	r0, #0
	beq	1f
	cmp	r0, #MAX_BOOT_STRING
	bge	1f

	/* `go' method */
	cmp	r0, #1					// extra argument?
	beq	3f
	ldr	r5, [r1, #4]				// load argv[1]
2:
	ldrb	r0, [r5], #1
	strb	r0, [r4], #1
	teq	r0, #0
	bne	2b

	b	3f
1:
	/* `bootm' method */
	mov	r6, r0					// save binfo pointer

	cmp	r3, #0
	beq	1f
2:
	ldrb	r0, [r3], #1
	strb	r0, [r4], #1
	teq	r0, #0
	bne	2b

1:
	cmp	r6, #0					// binfo passed?
	beq	3f

	add	r6, r6, #0x250				// to eth addr

	movw	r4, #:lower16:uboot_enaddr
	movt	r4, #:upper16:uboot_enaddr
	mov	r2, #6
2:
	ldrb	r0, [r6], #1
	strb	r0, [r4], #1
	subs	r2, r2, #1
	bne	2b
	
3:

	/*
	 * For easy and early SoC / PoP dependency, retrieve the IDs
	 */
#if 1
	mov	r6, #EXYNOS_CORE_PBASE
#else
	movw	r6, #:lower16:EXYNOS_CORE_PBASE
	movt	r6, #:upper16:EXYNOS_CORE_PBASE
#endif

	ldr	r0, [r6, #EXYNOS_PROD_ID_OFFSET]	// load soc_id

	movw	r4, #:lower16:exynos_soc_id
	movt	r4, #:upper16:exynos_soc_id
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	str	r0, [r4]				// save soc_id
	mov	r5, r0					// save soc_id

#ifdef EXYNOS4
	ldr	r0, [r6, #EXYNOS_PACKAGE_ID_OFFSET]		// load pop_id

	movw	r4, #:lower16:exynos_pop_id
	movt	r4, #:upper16:exynos_pop_id
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	str	r0, [r4]				// save pop_id
#endif

	/* Pick uart address and initial MMU table for the SoC */
	mov	r2, #0
	ubfx	r1, r5, #24, #8
	cmp	r1, #0xe5
	movteq	r2, #:upper16:(EXYNOS_CORE_PBASE + EXYNOS5_UART2_OFFSET)
	cmp	r1, #0xe4
	movteq	r2, #:upper16:(EXYNOS_CORE_PBASE + EXYNOS4_UART1_OFFSET)
	mcr	p15, 0, r2, c13, c0, 3		// TPIDRURO set (uart address)

	/*
	 * Turn on the SMP bit
	 */
	bl	cortex_init

	XPUTC(#67)

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	adr	r1, .Lmmu_init_table
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	bl	arm_boot_l1pt_init

	XPUTC(#68)

	/*
	 * Turn on the MMU, Caches, etc.
	 */
#ifdef VERBOSE_INIT_ARM
	adr	r11, xputc
#endif
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	b	arm_cpuinit

	.pushsection .text, "ax", %progbits
	.align	0
1:
	XPUTC2(#90)

#if defined(MULTIPROCESSOR)
#endif /* MULTIPROCESSOR */

	XPUTC2(#13)
	XPUTC2(#10)

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	b	start

	.popsection				// back to .start

	/* NOTREACHED */

#if defined(VERBOSE_INIT_ARM)
	.align 0
	.global xputc
	.type	xputc,%function

#define TIMO		0x25000
xputc:
	mov	r2, #TIMO
	mrc	p15, 0, r3, c13, c0, 3		// TPIDRURO get (uart address)
1:
	ldr	r1, [r3, #SSCOM_UTRSTAT]
#ifdef __ARMEB__
	rev	r1, r1
#endif
	tst	r1, #UTRSTAT_TXEMPTY
	bne	2f
	subs	r2, r2, #1
	bne	1b
2:
#ifdef __ARMEB__
	rev	r0, r0
#endif
	str	r0, [r3, #SSCOM_UTXH]

	mov	r2, #TIMO
3:
	ldr	r1, [r3, #SSCOM_UTRSTAT]
#ifdef __ARMEB__
	rev	r1, r1
#endif
	tst	r1, #UTRSTAT_TXSHIFTER_EMPTY
	bne	4f
	subs	r2, r2, #1
	bne	3b
4:
	bx	lr
#endif

#include <arm/cortex/a9_mpsubr.S>

#if EXYNOS_CORE_SIZE < EXYNOS4_CORE_SIZE
#error EXYNOS_CORE_SIZE smaller than EXYNOS4_CORE_SIZE
#endif
#if EXYNOS_CORE_SIZE < EXYNOS5_CORE_SIZE
#error EXYNOS_CORE_SIZE smaller than EXYNOS5_CORE_SIZE
#endif

	.align	0
.Lmmu_init_table:
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

#if KERNEL_BASE_VOFFSET
	/* Map physical addresses of kernel 1:1 PA:VA write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE - KERNEL_BASE_VOFFSET,
		KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)
#endif

	/* Map EXYNOS CORE (so console will work) */
	MMU_INIT(EXYNOS_CORE_VBASE, EXYNOS_CORE_PBASE,
		EXYNOS_CORE_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map EXYNOS CORE (so console will work) */
	MMU_INIT(EXYNOS_CORE_PBASE, EXYNOS_CORE_PBASE,
		EXYNOS_CORE_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(odroid_start)
