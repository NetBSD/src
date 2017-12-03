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

#include "opt_allwinner.h"
#include "opt_com.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"
#include "opt_arm_debug.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/allwinner/awin_reg.h>
#include <evbarm/awin/platform.h>

RCSID("$NetBSD: awin_start.S,v 1.14.4.2 2017/12/03 11:36:02 jdolecek Exp $")

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

#define	MD_CPU_HATCH	_C_LABEL(awin_cpu_hatch)

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

	.global	_C_LABEL(awin_start)
_C_LABEL(awin_start):
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
	XPUTC2(#'Z')

#if defined(MULTIPROCESSOR)
	// Now spin up the second processors into the same state we are now.
	XPUTC2(#'M')
	XPUTC2(#'P')
	XPUTC2(#'<')
	// Make sure the cache is flushed out to RAM for the other CPUs
	bl	_C_LABEL(armv7_dcache_wbinv_all)

#if defined(ALLWINNER_A20) || defined(ALLWINNER_A31)
	// Read SoC ID
	movw	r5, #:lower16:(AWIN_CORE_PBASE+AWIN_SRAM_OFFSET)
	movt	r5, #:upper16:(AWIN_CORE_PBASE+AWIN_SRAM_OFFSET)
	ldr	r1, [r5, #AWIN_SRAM_VER_REG]
#ifdef __ARMEB__
	# it's bit 15
	orr	r1, r1, #(AWIN_SRAM_VER_R_EN << 8)
#else
	orr	r1, r1, #AWIN_SRAM_VER_R_EN
#endif
	str	r1, [r5, #AWIN_SRAM_VER_REG]
	dsb
	ldr	r1, [r5, #AWIN_SRAM_VER_REG]
#ifdef __ARMEB__
	rev	r1, r1
#endif
	lsr	r1, r1, #16

	/* Set up a small stack in case gtmr_bootdelay() wants it */
	movw	sp, #:lower16:awin_initstkbase
	movt	sp, #:upper16:awin_initstkbase

	// MP init based on SoC ID
#if defined(ALLWINNER_A20)
# if defined(ALLWINNER_A31)
	movw	r0, #AWIN_SRAM_VER_KEY_A20
	cmp	r1, r0
	bleq	a20_mpinit
# else
	bl	a20_mpinit
# endif
#endif
#if defined(ALLWINNER_A31)
# if defined(ALLWINNER_A20)
	movw	r0, #AWIN_SRAM_VER_KEY_A31
	cmp	r1, r0
	bleq	a31_mpinit
# else
	bl	a31_mpinit
# endif
#endif
#elif defined(ALLWINNER_A80)
	bl	a80_mpinit
#endif

	XPUTC2(#'>')
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

	.pushsection .bss
	.align	8
awin_initstk:
	.space	32
awin_initstkbase:
	.popsection

#include <arm/cortex/a9_mpsubr.S>

#if defined(MULTIPROCESSOR)
#ifndef KERNEL_BASES_EQUAL
	.pushsection .text,"ax",%progbits
#endif
a20_mpinit:
	mov	r4, lr			// because we call gtmr_bootdelay
	movw	r5, #:lower16:(AWIN_CORE_PBASE+AWIN_CPUCFG_OFFSET)
	movt	r5, #:upper16:(AWIN_CORE_PBASE+AWIN_CPUCFG_OFFSET)

	XPUTC2(#'A')
	XPUTC2(#'2')
	XPUTC2(#'0')

#ifdef __ARMEB__
	setend	le			// everything here is little-endian
#endif

	/* Set where the other CPU(s) are going to execute */
	movw	r1, #:lower16:cortex_mpstart
	movt	r1, #:upper16:cortex_mpstart
	str	r1, [r5, #AWIN_CPUCFG_PRIVATE_REG]
	dsb

	/* Assert CPU core reset */
	mov	r1, #0
	str	r1, [r5, #AWIN_CPUCFG_CPU1_RST_CTRL_REG]
	dsb

	/* Ensure CPU1 reset also invalidates its L1 caches */
	ldr	r1, [r5, #AWIN_CPUCFG_GENCTRL_REG]
	bic	r1, r1, #(1 << 1)
	str	r1, [r5, #AWIN_CPUCFG_GENCTRL_REG]
	dsb

	/* Hold DBGPWRDUP signal low */
	ldr	r1, [r5, #AWIN_CPUCFG_DBGCTRL1_REG]
	bic	r1, r1, #(1 << 1)
	str	r1, [r5, #AWIN_CPUCFG_DBGCTRL1_REG]
	dsb

	/* Ramp up power to CPU1 */
	movw	r1, #0xff
1:	str	r1, [r5, #AWIN_CPUCFG_CPU1_PWRCLAMP_REG]
	dsb
	lsrs	r1, r1, #1
	bne	1b

	/* We need to wait (at least) 10ms */
	mov	r0, #0x3b000			// 10.06ms
	bl	_C_LABEL(gtmr_bootdelay)	// endian-neutral

	/* Clear power-off gating */
	ldr	r1, [r5, #AWIN_CPUCFG_CPU1_PWROFF_REG]
	bic	r1, r1, #(1 << 1)
	str	r1, [r5, #AWIN_CPUCFG_CPU1_PWROFF_REG]
	dsb

	/* Bring CPU1 out of reset */
	ldr	r1, [r5, #AWIN_CPUCFG_CPU1_RST_CTRL_REG]
	orr	r1, r1, #(AWIN_CPUCFG_CPU_RST_CTRL_CORE_RESET|AWIN_CPUCFG_CPU_RST_CTRL_RESET)
	str	r1, [r5, #AWIN_CPUCFG_CPU1_RST_CTRL_REG]
	dsb

	/* Reassert DBGPWRDUP signal */
	ldr	r1, [r5, #AWIN_CPUCFG_DBGCTRL1_REG]
	orr	r1, r1, #(1 << 1)
	str	r1, [r5, #AWIN_CPUCFG_DBGCTRL1_REG]
	dsb

#ifdef __ARMEB__
	setend	be				// we're done with little endian
#endif

	//
	// Wait up a second for CPU1 to hatch.
	//
	movw	r6, #:lower16:arm_cpu_hatched
	movt	r6, #:upper16:arm_cpu_hatched
	mov	r5, #200			// 200 x 5ms

1:	dmb					// memory barrier
	ldr	r0, [r6]			// load hatched
	tst	r0, #2				// our bit set yet?
	bxne	r4				//   yes, return
	subs	r5, r5, #1			// decrement count
	bxeq	r4				//   0? return
	mov	r0, #0x1d800			// 5.03ms
	bl	_C_LABEL(gtmr_bootdelay)
	b	1b
ASEND(a20_mpinit)
#ifndef KERNEL_BASES_EQUAL
	.popsection
#endif

#ifndef KERNEL_BASES_EQUAL
	.pushsection .text,"ax",%progbits
#endif
a31_mpinit:
	mov	r4, lr			// because we call gtmr_bootdelay
	movw	r5, #:lower16:(AWIN_CORE_PBASE+AWIN_A31_CPUCFG_OFFSET)
	movt	r5, #:upper16:(AWIN_CORE_PBASE+AWIN_A31_CPUCFG_OFFSET)
	movw	r6, #:lower16:(AWIN_CORE_PBASE+AWIN_A31_PRCM_OFFSET)
	movt	r6, #:upper16:(AWIN_CORE_PBASE+AWIN_A31_PRCM_OFFSET)

	XPUTC2(#'A')
	XPUTC2(#'3')
	XPUTC2(#'1')

#ifdef __ARMEB__
	setend	le			// everything here is little-endian
#endif

	mov	r10, #1			// CPU number

a31_mpinit_cpu:

	add	r1, r10, #'0'
	XPUTC2(r1)

	/* Set where the other CPU(s) are going to execute */
	movw	r1, #:lower16:cortex_mpstart
	movt	r1, #:upper16:cortex_mpstart
	str	r1, [r5, #AWIN_CPUCFG_PRIVATE_REG]
	dsb

	/* Assert CPU core reset */
	mov	r1, #0
	mov	r2, #0x40
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A31_CPUCFG_CPU0_RST_CTRL_REG
	str	r1, [r5, r7]
	dsb

	/* Ensure CPUX reset also invalidates its L1 caches */
	ldr	r1, [r5, #AWIN_CPUCFG_GENCTRL_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	bic	r1, r1, r0
	str	r1, [r5, #AWIN_CPUCFG_GENCTRL_REG]
	dsb

	/* Release power clamp */
	mov	r1, #0xe7
	mov	r2, #0x4
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A31_PRCM_CPUX_PWR_CLAMP_REG
	str	r1, [r6, r7]
	dsb

	mov	r2, #0x40
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A31_CPUCFG_CPU0_PWR_CLAMP_STATUS_REG
1:
	ldr	r1, [r5, r7]
	cmp	r1, #0xe7
	bne	1b

	/* We need to wait (at least) 10ms */
	mov	r0, #0x3b000			// 10.06ms
	bl	_C_LABEL(gtmr_bootdelay)	// endian-neutral

	/* Restore power clamp */
	mov	r1, #0
	mov	r2, #0x4
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A31_PRCM_CPUX_PWR_CLAMP_REG
	str	r1, [r6, r7]
	dsb

	mov	r2, #0x40
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A31_CPUCFG_CPU0_PWR_CLAMP_STATUS_REG
1:
	ldr	r1, [r5, r7]
	cmp	r1, #0
	bne	1b

	/* We need to wait (at least) 10ms */
	mov	r0, #0x3b000			// 10.06ms
	bl	_C_LABEL(gtmr_bootdelay)	// endian-neutral

	/* Clear power-off gating */
	ldr	r1, [r6, #AWIN_A31_PRCM_PWROFF_GATING_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	bic	r1, r1, r0
	str	r1, [r6, #AWIN_A31_PRCM_PWROFF_GATING_REG]
	dsb

	/* We need to wait (at least) 10ms */
	mov	r0, #0x3b000			// 10.06ms
	bl	_C_LABEL(gtmr_bootdelay)	// endian-neutral

	/* Bring CPUX out of reset */
	mov	r1, #(AWIN_A31_CPUCFG_RST_CTRL_CPU_RESET|AWIN_A31_CPUCFG_RST_CTRL_CORE_RESET)
	mov	r2, #0x40
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A31_CPUCFG_CPU0_RST_CTRL_REG
	str	r1, [r5, r7]
	dsb

	/* If there is another CPU, start it */
	add	r10, r10, #1
	cmp	r10, #3
	ble	a31_mpinit_cpu

#ifdef __ARMEB__
	setend	be				// we're done with little endian
#endif

	//
	// Wait up a second for CPU1-3 to hatch.
	//
	movw	r6, #:lower16:arm_cpu_hatched
	movt	r6, #:upper16:arm_cpu_hatched
	mov	r5, #200			// 200 x 5ms

1:	dmb					// memory barrier
	ldr	r0, [r6]			// load hatched
	cmp	r0, #0xe			// our bits set yet?
	bxge	r4				//   yes, return
	subs	r5, r5, #1			// decrement count
	bxeq	r4				//   0? return
	mov	r0, #0x1d800			// 5.03ms
	bl	_C_LABEL(gtmr_bootdelay)
	b	1b

ASEND(a31_mpinit)
#ifndef KERNEL_BASES_EQUAL
	.popsection
#endif

#if defined(ALLWINNER_A80)
#ifndef KERNEL_BASES_EQUAL
	.pushsection .text,"ax",%progbits
#endif
a80_mpinit:
	mov	r4, lr			// because we call gtmr_bootdelay
	movw	r5, #:lower16:(AWIN_A80_RCPUCFG_PBASE)
	movt	r5, #:upper16:(AWIN_A80_RCPUCFG_PBASE)
	movw	r6, #:lower16:(AWIN_A80_RCPUS_PBASE+AWIN_A80_RPRCM_OFFSET)
	movt	r6, #:upper16:(AWIN_A80_RCPUS_PBASE+AWIN_A80_RPRCM_OFFSET)

	XPUTC2(#'A')
	XPUTC2(#'8')
	XPUTC2(#'0')

#ifdef __ARMEB__
	setend	le			// everything here is little-endian
#endif

	mov	r10, #1			// CPU number

a80_mpinit_cpu:

	add	r1, r10, #'0'
	XPUTC2(r1)

	/* Set where the other CPU(s) are going to execute */
	movw	r1, #:lower16:cortex_mpstart
	movt	r1, #:upper16:cortex_mpstart
	str	r1, [r6, #AWIN_A80_RPRCM_PRIVATE_REG]
	dsb

	/* Assert CPU power on reset */
	ldr	r1, [r6, #AWIN_A80_RPRCM_CLUSTER0_RST_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	bic	r1, r1, r0
	str	r1, [r6, #AWIN_A80_RPRCM_CLUSTER0_RST_REG]

	/* Assert CPU core reset */
	ldr	r1, [r5, #AWIN_A80_RCPUCFG_CLUSTER0_RST_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	bic	r1, r1, r0
	str	r1, [r5, #AWIN_A80_RCPUCFG_CLUSTER0_RST_REG]

	/* Release power clamp */
	mov	r1, #0x00
	mov	r2, #0x4
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A80_RPRCM_CLUSTER0_PRW_CLAMP_REG
	str	r1, [r6, r7]
	dsb

	mov	r2, #0x40
	mul	r7, r10, r2
	add	r7, r7, #AWIN_A80_RPRCM_CLUSTER0_PRW_CLAMP_STATUS_REG
1:
	ldr	r1, [r5, r7]
	cmp	r1, #0x00
	bne	1b

	/* We need to wait (at least) 10ms */
	mov	r0, #0x3b000			// 10.06ms
	bl	_C_LABEL(gtmr_bootdelay)	// endian-neutral

	/* Clear power-off gating */
	ldr	r1, [r6, #AWIN_A80_RPRCM_CLUSTER0_PWR_GATING_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	bic	r1, r1, r0
	str	r1, [r6, #AWIN_A80_RPRCM_CLUSTER0_PWR_GATING_REG]
	dsb

	/* We need to wait (at least) 10ms */
	mov	r0, #0x3b000			// 10.06ms
	bl	_C_LABEL(gtmr_bootdelay)	// endian-neutral

	/* Bring core out of reset */
	ldr	r1, [r5, #AWIN_A80_RCPUCFG_CLUSTER0_RST_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	orr	r1, r1, r0
	str	r1, [r5, #AWIN_A80_RCPUCFG_CLUSTER0_RST_REG]

	/* Bring cpu power-on out of reset */
	ldr	r1, [r6, #AWIN_A80_RPRCM_CLUSTER0_RST_REG]
	mov	r0, #1
	lsl	r0, r0, r10
	orr	r1, r1, r0
	str	r1, [r6, #AWIN_A80_RPRCM_CLUSTER0_RST_REG]
	dsb

	/* If there is another CPU, start it */
	add	r10, r10, #1
	cmp	r10, #3
	ble	a80_mpinit_cpu

#ifdef __ARMEB__
	setend	be				// we're done with little endian
#endif

	//
	// Wait up a second for CPU1-3 to hatch.
	//
	movw	r6, #:lower16:arm_cpu_hatched
	movt	r6, #:upper16:arm_cpu_hatched
	mov	r5, #200			// 200 x 5ms

1:	dmb					// memory barrier
	ldr	r0, [r6]			// load hatched
	cmp	r0, #0xe			// our bits set yet?
	bxge	r4				//   yes, return
	subs	r5, r5, #1			// decrement count
	bxeq	r4				//   0? return
	mov	r0, #0x1d800			// 5.03ms
	bl	_C_LABEL(gtmr_bootdelay)
	b	1b

ASEND(a80_mpinit)
#ifndef KERNEL_BASES_EQUAL
	.popsection
#endif
#endif /* ALLWINNER_A80 */

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

	/* Map AWIN CORE (so console will work) */
	MMU_INIT(AWIN_CORE_VBASE, AWIN_CORE_PBASE,
		(AWIN_CORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map AWIN CORE (so console will work) */
	MMU_INIT(AWIN_CORE_PBASE, AWIN_CORE_PBASE,
		(AWIN_CORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

#if defined(ALLWINNER_A80)
	/* Map AWIN CORE2 */
	MMU_INIT(AWIN_A80_CORE2_VBASE, AWIN_A80_CORE2_PBASE,
		(AWIN_A80_CORE2_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map AWIN CORE2 */
	MMU_INIT(AWIN_A80_CORE2_PBASE, AWIN_A80_CORE2_PBASE,
		(AWIN_A80_CORE2_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map AWIN RCPUS (for PIO L-N, PRCM) */
	MMU_INIT(AWIN_A80_RCPUS_VBASE, AWIN_A80_RCPUS_PBASE,
		(AWIN_A80_RCPUS_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map AWIN RCPUS (for PIO L-N, PRCM) */
	MMU_INIT(AWIN_A80_RCPUS_PBASE, AWIN_A80_RCPUS_PBASE,
		(AWIN_A80_RCPUS_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map AWIN RCPUCFG */
	MMU_INIT(AWIN_A80_RCPUCFG_VBASE, AWIN_A80_RCPUCFG_PBASE,
		(AWIN_A80_RCPUCFG_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map AWIN RCPUCFG */
	MMU_INIT(AWIN_A80_RCPUCFG_PBASE, AWIN_A80_RCPUCFG_PBASE,
		(AWIN_A80_RCPUCFG_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)
#endif

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(awin_start))
