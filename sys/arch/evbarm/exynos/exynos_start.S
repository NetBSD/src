/*	$NetBSD: exynos_start.S,v 1.5.2.1 2018/07/28 04:37:31 pgoyette Exp $	*/

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
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"
#include "opt_arm_debug.h"
#include "opt_fdt.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arch/arm/samsung/exynos_reg.h>
#include <arch/arm/samsung/exynos5_reg.h>

#include <evbarm/exynos/platform.h>

RCSID("$NetBSD: exynos_start.S,v 1.5.2.1 2018/07/28 04:37:31 pgoyette Exp $")

#define XPUTC(n)
#define XPUTC2(n)

#define INIT_MEMSIZE	128

#define	TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET + INIT_MEMSIZE * L1_S_SIZE - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(arm_fdt_cpu_hatch)

/*
 * Kernel start routine for Exynos 5422 boards running on uboot firmware
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
#ifdef KERNEL_BASES_EQUAL
	.text
#else
	.section .start,"ax",%progbits
#endif

	.global	_C_LABEL(exynos_start)
_C_LABEL(exynos_start):
#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif

	/*
	 * Save any arguments passed to us.  If .start is not at
	 * 0x80000000 but .text is, we can't directly use the address that
	 * the linker gave us.  In that case convert the virtual address to the
	 * physical address by using KERNEL_BASE_VOFFSET.
	 */
	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	stmia	r4, {r0-r3}			// Save the arguments

#ifdef FDT
	/*
	 * ARM boot protocol has FDT address in r2
	 */
	movw	r4, #:lower16:fdt_addr_r
	movt	r4, #:upper16:fdt_addr_r
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	str	r2, [r4]
#endif

	/* Add DTB PA (1MB) from r2 to MMU init table */
	movw	r3, #:lower16:(L1_S_SIZE - 1)		/* align DTB PA to 1M */
	movt	r3, #:upper16:(L1_S_SIZE - 1)
	bic	r0, r2, r3
	orr	r0, r0, #1				/* 1MB mapping */
	bic	r1, r2, r3
	movw	r3, #:lower16:(L1_S_PROTO_armv7|L1_S_APv7_KRW|L1_S_CACHEABLE)
	movt	r3, #:upper16:(L1_S_PROTO_armv7|L1_S_APv7_KRW|L1_S_CACHEABLE)
	orr	r1, r1, r3
	adrl	r3, .Lmmu_init_table_dtb		/* table entry addr */
	stmia	r3, {r0-r1}				/* patch table entry */

	/*
	 * For easy and early SoC / PoP dependency, retrieve the IDs
	 */
	movw	r6, #:lower16:EXYNOS_CORE_PBASE
	movt	r6, #:upper16:EXYNOS_CORE_PBASE

	ldr	r0, [r6, #EXYNOS_PROD_ID_OFFSET]	// load soc_id

	movw	r4, #:lower16:exynos_soc_id
	movt	r4, #:upper16:exynos_soc_id
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	str	r0, [r4]				// save soc_id
	mov	r5, r0					// save soc_id

	/*
	 * Turn on the SMP bit
	 */
	bl	cortex_init

	XPUTC(#'C')

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	movw	r1, #:lower16:mmu_init_table
	movt	r1, #:upper16:mmu_init_table
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
	 * so setup the lr to be in .text.  Cache the address for exynos_putc
	 * before we go.
	 */
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	b	arm_cpuinit
	.pushsection .text, "ax", %progbits
1:
#endif
	XPUTC2(#'Z')

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

#include <arm/cortex/a9_mpsubr.S>

	.align	0
mmu_init_table:
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

#if KERNEL_BASE_VOFFSET !=0
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

	/* Map EXYNOS AUDIOBASE */
	MMU_INIT(EXYNOS5_AUDIOCORE_VBASE, EXYNOS5_AUDIOCORE_VBASE,
	        EXYNOS5_AUDIOCORE_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map EXYNOS AUDIOBASE */
	MMU_INIT(EXYNOS5_AUDIOCORE_PBASE, EXYNOS5_AUDIOCORE_VBASE,
	        EXYNOS5_AUDIOCORE_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map sysram for MP startup */
	MMU_INIT(EXYNOS5_SYSRAM_VBASE, EXYNOS5_SYSRAM_PBASE,
	        EXYNOS5_SYSRAM_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map sysram for MP startup */
	MMU_INIT(EXYNOS5_SYSRAM_PBASE, EXYNOS5_SYSRAM_PBASE,
	        EXYNOS5_SYSRAM_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map DTB location in SDRAM, patched in later */
.Lmmu_init_table_dtb:
	MMU_INIT(0, 0, 0, 0)


	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(exynos_start)
