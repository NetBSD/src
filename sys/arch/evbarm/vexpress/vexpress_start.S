/*	$NetBSD: vexpress_start.S,v 1.6 2018/03/03 13:46:33 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Sergio L. Pascual.
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
#include "opt_arm_debug.h"
#include "opt_fdt.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/vexpress/vexpress_platform.h>

RCSID("$NetBSD: vexpress_start.S,v 1.6 2018/03/03 13:46:33 skrll Exp $")

#ifdef VERBOSE_INIT_ARM
#define	XPUTC(n)	mov r0, n; bl xputc
#define	XPUTC2(n)	mov r0, n; blx r11
#else
#define	XPUTC(n)
#define	XPUTC2(n)
#endif

#define	INIT_MEMSIZE	64
#define	TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET + INIT_MEMSIZE * L1_S_SIZE - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(arm_fdt_cpu_hatch)

/*
 * Kernel start routine for Versatile Express boards running on uboot firmware
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(vexpress_start)
_C_LABEL(vexpress_start):
#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif
	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid	if, #PSR_SVC32_MODE

	/* Save any arguments passed to us. */
	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
	sub	r4, r4, #KERNEL_BASE_VOFFSET
	stmia	r4, {r0-r3}

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
	adr	r3, .Lmmu_init_table_dtb		/* table entry addr */
	stmia	r3, {r0-r1}				/* patch table entry */

	XPUTC('a')

	bl	cortex_init

	XPUTC('b')

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	adr     r1, .Lmmu_init_table
	movw    r0, #:lower16:TEMP_L1_TABLE
	movt    r0, #:upper16:TEMP_L1_TABLE
	bl      arm_boot_l1pt_init

	XPUTC('c')

	adr	r11, xputc
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	b	arm_cpuinit

	.pushsection .text, "ax", %progbits
	.align	0

1:
	XPUTC2('d')

	b	start

	.popsection

	.align 0
	.global xputc
	.type	xputc,%function
xputc:
	movw	r2, #0x0000
	movt	r2, #0x1c09
	str	r0, [r2]
	bx	lr

#include <arm/cortex/a9_mpsubr.S>

	.align  0
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

	/* Map VEXPRESS CORE (so console will work) */
	MMU_INIT(VEXPRESS_CORE_VBASE, VEXPRESS_CORE_PBASE,
		VEXPRESS_CORE_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map VEXPRESS CORE (so console will work) */
	MMU_INIT(VEXPRESS_CORE_PBASE, VEXPRESS_CORE_PBASE,
		VEXPRESS_CORE_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map VEXPRESS GIC */
	MMU_INIT(VEXPRESS_GIC_VBASE, VEXPRESS_GIC_PBASE,
		VEXPRESS_GIC_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map VEXPRESS GIC */
	MMU_INIT(VEXPRESS_GIC_PBASE, VEXPRESS_GIC_PBASE,
		VEXPRESS_GIC_SIZE / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map DTB location in SDRAM, patched in later */
.Lmmu_init_table_dtb:
	MMU_INIT(0, 0, 0, 0)

        /* end of table */
        MMU_INIT(0, 0, 0, 0)


END(vexpress_start)
