/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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

#include "opt_com.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"
#include "opt_arm_debug.h"
#include "opt_fdt.h"

#include <sys/cdefs.h>
#include <arm/asm.h>
#include <arm/armreg.h>

#include "assym.h"

RCSID("$NetBSD: rpi2_start.S,v 1.8 2018/08/03 15:46:41 skrll Exp $")

#if defined(KERNEL_BASES_EQUAL)
#define CALL(f)		bl	_C_LABEL(f)
#else
#define	CALL(f)	\
	movw	fp, #:lower16:_C_LABEL(f); \
	movt	fp, #:upper16:_C_LABEL(f); \
	sub	fp, fp, #KERNEL_BASE_VOFFSET; \
	blx	fp
#endif

#if defined(VERBOSE_INIT_ARM) && defined(EARLYCONS)
#define	PLATFORM_EARLY_PUTCHAR	___CONCAT(EARLYCONS, _platform_early_putchar)
#define	XPUTC(n)	push {r1-r12, lr}; mov r0, n; CALL(PLATFORM_EARLY_PUTCHAR); pop {r1-r12, lr}
#if KERNEL_BASE_VOFFSET == 0
#define	XPUTC2(n)	mov r0, n; CALL(PLATFORM_EARLY_PUTCHAR)
#else
#define XPUTC2(n)	mov r0, n; blx r11
#endif
#else
#define	XPUTC(n)
#define	XPUTC2(n)
#endif

#define INIT_MEMSIZE	(0x40000000 / L1_S_SIZE)
#define	TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET + 128 * L1_S_SIZE - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(arm_fdt_cpu_hatch)
/*
 * Kernel start routine for RPI2 board.
 * At this point, this code has been loaded into SDRAM
 * and the MMU maybe on or maybe off.
 */
#ifdef KERNEL_BASES_EQUAL
	.text
#else
	.section .start,"ax",%progbits
#endif

	.global	_C_LABEL(rpi_start)
_C_LABEL(rpi_start):

#if defined(VERBOSE_INIT_ARM) && defined(EARLYCONS)
	/* stack for calling bcm283*_platform_early_putchar() */
	movw	sp, #:lower16:bootstk
	movt	sp, #:upper16:bootstk
#if !defined(KERNEL_BASES_EQUAL)
	sub	sp, sp, #KERNEL_BASE_VOFFSET
#endif
#endif /* VERBOSE_INIT_ARM */

#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif
	/*
	 * Save any arguments passed to us.
	 */
	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
#if KERNEL_BASE_VOFFSET != 0
	/*
	 * But since .start is at 0x40000000 and .text is at 0x8000000, we
	 * can't directly use the address that the linker gave us directly.
	 * We have to adjust the address the linker gave us to get the to
	 * the physical address.
	 */
	sub	r4, r4, #KERNEL_BASE_VOFFSET
#endif

	stmia	r4, {r0-r3}		// Save the arguments

#ifdef FDT
	/*
	 * ARM boot protocol has FDT address in r2
	 */
	movw	r4, #:lower16:fdt_addr_r
	movt	r4, #:upper16:fdt_addr_r
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	str	r2, [r4]
#endif

	mov	r2, #KERNEL_BASE_VOFFSET
	movw	r4, #:lower16:kern_vtopdiff
	movt	r4, #:upper16:kern_vtopdiff
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	str	r2, [r4]

	/*
	 * Setup the CPU
	 */
	bl	cortex_init

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	movw	r1, #:lower16:.Lmmu_init_table
	movt	r1, #:upper16:.Lmmu_init_table
	bl	arm_boot_l1pt_init
	XPUTC(#'D')

	/*
	 * Turn on the MMU, Caches, etc.  Return to new enabled address space.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
#if KERNEL_BASE_VOFFSET == 0
	bl	arm_cpuinit
#else
	/*
	 * After the MMU is on, we can execute in the normal .text segment
	 * so setup the lr to be in .text.  Cache the address for
	 * bcm283x_platform_early_putchar before we go.
	 */
#if defined(VERBOSE_INIT_ARM) && defined(EARLYCONS)
	movw	r11, #:lower16:PLATFORM_EARLY_PUTCHAR
	movt	r11, #:upper16:PLATFORM_EARLY_PUTCHAR
#endif
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	b	arm_cpuinit

	.pushsection .text,"ax",%progbits
1:
#endif
	XPUTC2(#'Z')

#if defined(MULTIPROCESSOR)
	// Now spin up the second processors into the same state we are now.
	XPUTC2(#'M')
	XPUTC2(#'P')		// 'P'
	XPUTC2(#'<')		// '<'
	// Make sure the cache is flushed out to RAM for the other CPUs
	bl	_C_LABEL(armv7_dcache_wbinv_all)

	XPUTC2(#'>')		// '>'
#endif /* MULTIPROCESSOR */
	XPUTC2(#13)
	XPUTC2(#10)

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	b	start

	/* NOTREACHED */

#ifndef KERNEL_BASES_EQUAL
	.popsection
#endif

#if defined(VERBOSE_INIT_ARM) && defined(EARLYCONS)
	.pushsection .bss
	.align	3
	.space	256
bootstk:
	.popsection
#endif

#include <arm/cortex/a9_mpsubr.S>

.Lmmu_init_table:
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

#if KERNEL_BASE_VOFFSET != 0
	/* Map memory 1:1 VA to PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE - KERNEL_BASE_VOFFSET,
		KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)
#endif

	/* Map the 16MB of peripherals */
	MMU_INIT(BCM2836_PERIPHERALS_VBASE, BCM2836_PERIPHERALS_BASE,
	    (BCM2835_PERIPHERALS_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map the 16MB of peripherals */
	MMU_INIT(BCM2836_PERIPHERALS_BASE, BCM2836_PERIPHERALS_BASE,
	    (BCM2835_PERIPHERALS_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	MMU_INIT(BCM2836_ARM_LOCAL_VBASE, BCM2836_ARM_LOCAL_BASE,
	    (BCM2836_ARM_LOCAL_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(rpi_start))

