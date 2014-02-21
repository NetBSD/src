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

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/allwinner/awin_reg.h>
#include <evbarm/cubie/platform.h>  

RCSID("$NetBSD: cubie_start.S,v 1.6 2014/02/21 22:22:48 matt Exp $")

#if defined(VERBOSE_INIT_ARM)
#define	XPUTC(n)	mov r0, n; bl xputc
#ifdef __ARMEB__
#define COM_BSWAP
#endif
#define COM_MULT	4
#define XPUTC_COM	1
#else
#define	XPUTC(n)
#endif

#define INIT_MEMSIZE	128
#define	TEMP_L1_TABLE	(AWIN_SDRAM_PBASE + INIT_MEMSIZE * 0x100000 - L1_TABLE_SIZE)

#define	MD_CPU_HATCH	_C_LABEL(awin_cpu_hatch)

/*
 * Kernel start routine for BEAGLEBOARD boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(cubie_start)
_C_LABEL(cubie_start):
#ifdef __ARMEB__
	setend	be			/* force big endian */
#endif

	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid	if, #PSR_SVC32_MODE

	/*
	 * Save any arguments passed to us.  But since .start is at 0x40000000
	 * and .text is at 0x8000000, we can't directly use the address that
	 * the linker gave us directly.  We have to replace the upper 4 bits
	 * of the address the linker gave us and replace it with the upper 4
	 * bits of our pc.  Or replace the lower 28 bits of our PC with the
	 * lower 28 bits of what the linker gave us.
	 */
	adr	r4, _C_LABEL(cubie_start)
	movw	r5, #:lower16:uboot_args
	movt	r5, #:upper16:uboot_args
	bfi	r4, r5, #0, #28

	stmia	r4, {r0-r3}		// Save the arguments
	/*
	 * Turn on the SMP bit
	 */
	bl	cortex_init
	XPUTC(#67)

#if defined(MULTIPROCESSOR) && 0
	movw	r0, #:lower16:(AWIN_CORE_PBASE+AWIN_CPUCFG_OFFSET)
	movt	r0, #:upper16:(AWIN_CORE_PBASE+AWIN_CPUCFG_OFFSET)

	/* Set where the other CPU(s) are going to execute */
	adr	r1, cortex_mpstart
	str	r1, [r0, #AWIN_CPUCFG_PRIVATE_REG]

	/* Bring CPU1 out of reset */
	ldr	r1, [r0, #AWIN_CPUCFG_CPU1_RST_CTRL_REG]
	orr	r1, r1, #(AWIN_CPUCFG_CPU_RST_CTRL_CORE_RESET|AWIN_CPUCFG_CPU_RST_CTRL_RESET)
	str	r1, [r0, #AWIN_CPUCFG_CPU1_RST_CTRL_REG]
#endif /* MULTIPROCESSOR */

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	adr	r1, .Lmmu_init_table
	bl	arm_boot_l1pt_init

	XPUTC(#68)

	/*
	 * Turn on the MMU, Caches, etc.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	bl	arm_cpuinit

	XPUTC(#90)
	XPUTC(#13)
	XPUTC(#10)

	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	movw	ip, #:lower16:start
	movt	ip, #:upper16:start
	bx	ip		/* Jump to start (flushes pipeline). */

	/* NOTREACHED */

#include <arm/cortex/a9_mpsubr.S>

.Lmmu_init_table:
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, AWIN_SDRAM_PBASE,
		(INIT_MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW | L1_S_B | L1_S_C | L1_S_V6_S)

	/* Map memory 1:1 VA to PA, write-back cacheable, shareable */
	MMU_INIT(AWIN_SDRAM_PBASE, AWIN_SDRAM_PBASE,
		(INIT_MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW | L1_S_B | L1_S_C | L1_S_V6_S)

	/* Map AWIN CORE (so console will work) */
	MMU_INIT(AWIN_CORE_VBASE, AWIN_CORE_PBASE,
		(AWIN_CORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)

	/* Map AWIN CORE (so console will work) */
	MMU_INIT(AWIN_CORE_PBASE, AWIN_CORE_PBASE,
		(AWIN_CORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(cubie_start))
