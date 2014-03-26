/*	$NetBSD: bcm53xx_start.S,v 1.10.2.3 2014/03/26 02:16:05 matt Exp $	*/
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

//#define GXEMUL

#include "opt_broadcom.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/cortex/a9tmr_reg.h>

#ifndef CONADDR
#define	CONADDR		0x18000300
#endif

RCSID("$NetBSD: bcm53xx_start.S,v 1.10.2.3 2014/03/26 02:16:05 matt Exp $")

#undef VERBOSE_INIT_ARM
#define VERBOSE_INIT_ARM

#if defined(VERBOSE_INIT_ARM)
#define	XPUTC(n)	mov r0, n; bl xputc
#define XPUTC_COM	1
#else
#define	XPUTC(n)
#endif

#define	MD_CPU_HATCH	bcm53xx_cpu_hatch

/*
 * Kernel start routine for BCM5301X boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.text

	.global	_C_LABEL(bcm53xx_start)
_C_LABEL(bcm53xx_start):
#ifdef __ARMEB__
	setend	be		/* make sure we are running big endian */
#endif
	/*
	 * Save any arguments u-boot passed us.
	 */
#ifdef KERNEL_BASES_EQUAL
	ldr	r4, .Luboot_args
#else
	adr	r4, bcm53xx_start
	ldr	r5, .Luboot_args
	bfi	r4, r5, #0, #28
#endif
	stmia	r4, {r0-r3}

	/*
	 * Let's turn on the CCA watchdog in case something goes horribly wrong.
	 */
	ldr	r0, .Lcca_wdog
	ldr	r1, .Lcca_wdog + 4
	str	r1, [r0]

	/*
	 * Cal the initial start code for the a9
	 */
	bl	a9_start

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	ldr	r0, .Ltemp_l1_table	/* The L1PT address - entered into TTB later */
	adr	r1, mmu_init_table
	bl	arm_boot_l1pt_init

	XPUTC(#68)

	/*
	 * Before we turn on the MMU, let's the other process out of the
	 * SKU ROM but setting the magic LUT address to our own mp_start
	 * routine. 
	 */
	ldr	r1, .Lsku_rom_lut
	adr	r2, a9_mpstart
	str	r2, [r1]
	sev				/* wake up the others */

	/*
	 * init the CPU TLB, Cache, MMU.
	 */
	XPUTC(#69)

	ldr	r0, .Ltemp_l1_table	/* The page table address */
	bl	arm_cpuinit

	XPUTC(#89)

	XPUTC(#90)

	/*
	 * Let's turn off the CCA watchdog since nothing went horribly wrong.
	 */
	ldr	r0, .Lcca_wdog
	mov	r1, #0
	str	r1, [r0]

	XPUTC(#33)
	XPUTC(#10)
	XPUTC(#13)
	/*
	 * Jump to start in locore.S, which in turn will call initarm and main.
	 */
	ldr	ip, .Lstart
	bx	ip
	nop
	nop
	nop
	nop

	/* NOTREACHED */
.Luboot_args:
	.word	uboot_args
.Lstart:
	.word	start

.Lsku_rom_lut:
	.word	0xffff0400

.Lcca_wdog:
	.word	0x18000080
	.word	0x0fffffff	/* maximum watchdog time out, about 10 seconds */

.Ltemp_l1_table:
	/* Put the temporary L1 translation table far enough away. */
	.word	31 * 0x100000 - L1_TABLE_SIZE

#include <arm/cortex/a9_mpsubr.S>

mmu_init_table:
	/* Add 128MB of VA==PA at 0x80000000 so we can keep the kernel going */
#ifdef BCM5301X
	MMU_INIT(KERNEL_BASE, 0x80000000, 128,
	    L1_S_PROTO | L1_S_APv7_KRW | L1_S_B | L1_S_C | L1_S_V6_S)
#elif defined(BCM563XX)
	MMU_INIT(KERNEL_BASE, 0x60000000, 128,
	    L1_S_PROTO | L1_S_APv7_KRW | L1_S_B | L1_S_C | L1_S_V6_S)
#endif

	MMU_INIT(0, 0x00000000,
	    (32 * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO | L1_S_APv7_KRW | L1_S_B | L1_S_C | L1_S_V6_S)

	/* Map the 2MB of primary peripherals */
	MMU_INIT(KERNEL_IO_IOREG_VBASE, BCM53XX_IOREG_PBASE,
	    (BCM53XX_IOREG_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO | L1_S_APv7_KRW)

	/* Map the 2MB of primary peripherals at PA:VA 1:1 */
	MMU_INIT(BCM53XX_IOREG_PBASE, BCM53XX_IOREG_PBASE,
	    (BCM53XX_IOREG_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO | L1_S_APv7_KRW)

	/* Map the 1MB of armcore peripherals */
	MMU_INIT(KERNEL_IO_ARMCORE_VBASE, BCM53XX_ARMCORE_PBASE,
	    (BCM53XX_ARMCORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO | L1_S_APv7_KRW)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)
