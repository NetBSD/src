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

#include <arm/asm.h>
#include <arm/armreg.h>

#include "assym.h"

RCSID("$NetBSD: rpi2_start.S,v 1.3.16.2 2017/12/03 11:36:06 jdolecek Exp $")

#if defined(VERBOSE_INIT_ARM)
#define	XPUTC(n)	mov r0, n; bl plputc
#if KERNEL_BASE_VOFFSET == 0
#define	XPUTC2(n)	mov r0, n; bl plputc
#else
#define XPUTC2(n)	mov r0, n; blx r11
#endif
#ifdef __ARMEB__
#define COM_BSWAP
#endif
#define COM_MULT	4
#else
#define	XPUTC(n)
#define	XPUTC2(n)
#endif

#define INIT_MEMSIZE	128
#define	TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET + INIT_MEMSIZE * L1_S_SIZE - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(bcm2836_cpu_hatch)

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

#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif
	/*
	 * Save any arguments passed to us.
	 */
	movw	r4, #:lower16:rpi_boot_regs
	movt	r4, #:upper16:rpi_boot_regs
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
#if !defined(KERNEL_BASES_EQUAL)
	sub	r8, r8, #KERNEL_BASE_VOFFSET
#endif
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
	 * so setup the lr to be in .text.  Cache the address for plputc
	 * before we go.
	 */
#if defined(VERBOSE_INIT_ARM)
	adr	r11, plputc		@ for XPUTC2
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

#include <arm/cortex/a9_mpsubr.S>

#if defined(VERBOSE_INIT_ARM)

#define	PL01XCOM_FR	0x18	/* Flag Register */
#define	PL01XCOM_DR	0x00	/* Data Register */
#define	PL01X_FR_TXFE	0x080	/* Transmit fifo empty */

#define COM_DATA	PL01XCOM_DR
#define	COM_LSR		PL01XCOM_FR
#define	LSR_TSRE	PL01X_FR_TXFE
#define COM_MULT	4

#define CONSADDR	0x3f201000

#define TIMO		0x25000
#ifndef COM_MULT
#define COM_MULT	1
#endif
	.global	_C_LABEL(plputc)
_C_LABEL(plputc):

	mov	r2, #TIMO
	movw	r3, #:lower16:CONSADDR
	movt	r3, #:upper16:CONSADDR
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
	tst	r1, #LSR_TSRE
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
	MMU_INIT(RPI_KERNEL_IO_VBASE, RPI_KERNEL_IO_PBASE,
	    (RPI_KERNEL_IO_VSIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* Map the 16MB of peripherals */
	MMU_INIT(RPI_KERNEL_IO_PBASE, RPI_KERNEL_IO_PBASE,
	    (RPI_KERNEL_IO_VSIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)


	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(rpi_start))

