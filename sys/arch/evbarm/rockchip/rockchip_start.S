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

#include "opt_rockchip.h"
#include "opt_com.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"
#include "opt_arm_debug.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/rockchip/rockchip_reg.h>
#include <evbarm/rockchip/platform.h>  

#include <arm/cortex/scu_reg.h>

RCSID("$NetBSD: rockchip_start.S,v 1.4.18.2 2017/12/03 11:36:06 jdolecek Exp $")

#if defined(VERBOSE_INIT_ARM)
#define	XPUTC(n)	mov r0, n; bl xputc
#if KERNEL_BASE_VOFFSET == 0
#define	XPUTC2(n)	mov r0, n; bl xputc
#else
#define XPUTC2(n)	mov r0, n; blx r11
#endif
#ifdef __ARMEB__
#define COM_BSWAP
#endif
#define COM_MULT	4
#define XPUTC_COM	1
#else
#define	XPUTC(n)
#define	XPUTC2(n)
#endif

#define INIT_MEMSIZE	128
#define	TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET + INIT_MEMSIZE * L1_S_SIZE - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(a9tmr_init_cpu_clock)

/*
 * Kernel start routine for CUBIE (Allwinner) boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU maybe on or maybe off.
 */
#ifdef KERNEL_BASES_EQUAL
	.text
#else
	.section .start,"ax",%progbits
#endif

	.global	_C_LABEL(rockchip_start)
_C_LABEL(rockchip_start):
#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif
	mov	r9, #0

	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid	if, #PSR_SVC32_MODE

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

	/*
	 * Turn on the SMP bit
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
	XPUTC(#68)

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
	 * so setup the lr to be in .text.  Cache the address for xputc
	 * before we go.
	 */
#if defined(VERBOSE_INIT_ARM)
	adr	r11, xputc		@ for XPUTC2
#endif
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	b	arm_cpuinit
	.pushsection .text,"ax",%progbits
1:
#endif
	XPUTC2(#90)

#if defined(MULTIPROCESSOR)
	// Now spin up the second processors into the same state we are now.
	XPUTC2(#77)
	XPUTC2(#80)
	XPUTC2(#60)
	// Make sure the cache is flushed out to RAM for the other CPUs
	bl	_C_LABEL(armv7_dcache_wbinv_all)
	bl	rockchip_mpinit
	XPUTC2(#62)
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

#include <arm/cortex/a9_mpsubr.S>

#define PMU_PWRDN_REG	0x0008
#define PMU_PWRDN_SCU	__BIT(4)

#if defined(MULTIPROCESSOR)
#ifndef KERNEL_BASES_EQUAL
	.pushsection .text,"ax",%progbits
#endif
rockchip_mptramp:
	ldr	pc, 1f
.global rockchip_mpstart_vec
rockchip_mpstart_vec:
1:	.space	4

rockchip_mpinit:
	mov	r4, lr
	/* r5: SCU, r6: PMU, r7: SRAM */
	movw	r5, #:lower16:(ROCKCHIP_CORE0_BASE+ROCKCHIP_SCU_OFFSET)
	movt	r5, #:upper16:(ROCKCHIP_CORE0_BASE+ROCKCHIP_SCU_OFFSET)
	movw	r6, #:lower16:(ROCKCHIP_CORE1_BASE+ROCKCHIP_PMU_OFFSET)
	movt	r6, #:upper16:(ROCKCHIP_CORE1_BASE+ROCKCHIP_PMU_OFFSET)
	movw	r7, #:lower16:(ROCKCHIP_CORE0_BASE+ROCKCHIP_SRAM_OFFSET)
	movt	r7, #:upper16:(ROCKCHIP_CORE0_BASE+ROCKCHIP_SRAM_OFFSET)

	/* Set where the other CPU(s) are going to execute */
	XPUTC2(#118)
	movw	r1, #:lower16:rockchip_mpstart
	movt	r1, #:upper16:rockchip_mpstart
	ldr	r0, =rockchip_mpstart_vec
	str	r1, [r0]
	ldr	r0, =rockchip_mptramp
	mov	r2, #0
1:	ldr	r1, [r0, r2]
	str	r1, [r7, r2]
	add	r2, r2, #4
	cmp	r2, #32
	blt	1b
	dsb

	/* Invalid SCU cache tags */
	XPUTC2(#45)
	movw	r1, #0xffff
	movt	r1, #0
	str	r1, [r5, #SCU_INV_ALL_REG]

	/* Get CPU count */
	ldr	r1, [r5, #SCU_CFG]
	and	r2, r1, #SCU_CFG_CPUMAX
	add	r2, r2, #1

	/* Convert to CPU1..N mask */
	mov	r7, #0
	lsl	r7, r2, #1
	sub	r7, r7, #1
	and	r7, r7, #~1

	/* Power down secondary CPUs */
	XPUTC2(#46)
	ldr	r1, [r6, #PMU_PWRDN_REG]
	orr	r1, r1, r7
	str	r1, [r6, #PMU_PWRDN_REG]
	dsb

	/* Power up SCU */
	XPUTC2(#46)
	ldr	r1, [r6, #PMU_PWRDN_REG]
	and	r1, r1, #~PMU_PWRDN_SCU
	str	r1, [r6, #PMU_PWRDN_REG]
	dsb

	/* Enable SCU */
	XPUTC2(#46)
	ldr	r1, [r5, #SCU_CTL]
	orr	r1, r1, #SCU_CTL_SCU_ENA
	str	r1, [r5, #SCU_CTL]
	dsb

	/* Power up secondary CPUs */
	XPUTC2(#33)
	ldr	r1, [r6, #PMU_PWRDN_REG]
	and	r1, r1, r7
	str	r1, [r6, #PMU_PWRDN_REG]
	dsb

	XPUTC2(#49)
	XPUTC2(#50)
	XPUTC2(#51)

	//
	// Wait up a second for CPU1 to hatch. 
	//
	movw	r2, #:lower16:arm_cpu_hatched
	movt	r2, #:upper16:arm_cpu_hatched
	mov	r1, #0x10000000
1:	dmb
	ldr	r0, [r2]
	cmp	r0, r7
	beq	.hatched
	subs	r1, r1, #1
	bne	1b

.hatched:
	bx	r4

ASEND(rockchip_mpinit)
#ifndef KERNEL_BASES_EQUAL
	.popsection
#endif

rockchip_mpstart:
	/* invalidate cache */
	movw	ip, #:lower16:_C_LABEL(armv7_dcache_inv_all)
	movt	ip, #:upper16:_C_LABEL(armv7_dcache_inv_all)
#ifndef KERNEL_BASES_EQUAL
	sub	ip, ip, #KERNEL_BASE_VOFFSET
#endif
	blx	ip
	b	_C_LABEL(cortex_mpstart)

#endif /* MULTIPROCESSOR */

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

	/* Map ROCKCHIP CORE0 */
	MMU_INIT(ROCKCHIP_CORE0_VBASE, ROCKCHIP_CORE0_BASE,
		(ROCKCHIP_CORE0_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map ROCKCHIP CORE0 */
	MMU_INIT(ROCKCHIP_CORE0_BASE, ROCKCHIP_CORE0_BASE,
		(ROCKCHIP_CORE0_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map ROCKCHIP CORE1 (so console will work) */
	MMU_INIT(ROCKCHIP_CORE1_VBASE, ROCKCHIP_CORE1_BASE,
		(ROCKCHIP_CORE1_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map ROCKCHIP CORE1 (so console will work) */
	MMU_INIT(ROCKCHIP_CORE1_BASE, ROCKCHIP_CORE1_BASE,
		(ROCKCHIP_CORE1_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)
	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(rockchip_start))
