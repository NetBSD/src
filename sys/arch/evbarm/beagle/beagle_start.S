/*
 * Machine dependent startup code for BEAGLEBOARD boards.
 * Based on omap_start.S
 *
 * Copyright (c) 2002, 2003  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 2007 Microsoft
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_omap.h"
#include "opt_com.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/omap/omap2_obioreg.h>
#include <evbarm/beagle/beagle.h>  

#ifdef CPU_CORTEXA9
#include <arm/cortex/a9tmr_reg.h>
#endif

RCSID("$NetBSD: beagle_start.S,v 1.18 2013/08/18 06:34:02 matt Exp $")

#if defined(VERBOSE_INIT_ARM)
#define	XPUTC(n)	mov r0, n; bl xputc
#define COM_MULT	4
#define XPUTC_COM	1
#else
#define	XPUTC(n)
#endif

#define INIT_MEMSIZE	128
#define	TEMP_L1_TABLE	(0x80000000 + INIT_MEMSIZE * 0x100000 - L1_TABLE_SIZE)

/*
 * Kernel start routine for BEAGLEBOARD boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(beagle_start)
_C_LABEL(beagle_start):
	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid	if, #PSR_SVC32_MODE

	/*
	 * Save any arguments passed to us.
	 */
	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
	stmia	r4, {r0-r3}

#ifdef CPU_CORTEXA9
	/*
	 * Turn on the SCU if we are on a Cortex-A9
	 */
	bl	a9_start
	XPUTC(#67)
#endif

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
	b	start		/* Jump to start (flushes pipeline). */
	nop
	nop
	nop
	nop
	
	/* NOTREACHED */

#include <arm/cortex/a9_mpsubr.S>

.Lmmu_init_table:
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, KERNEL_BASE,
		(INIT_MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW | L1_S_B | L1_S_C | L1_S_V6_S)

	/* Map first 1MB of L4 CORE (so console will work) */
	MMU_INIT(OMAP_L4_CORE_VBASE, OMAP_L4_CORE_BASE,
		(OMAP_L4_CORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)

#if OMAP_L4_CORE_BASE <= CONSADDR \
	&& CONSADDR < OMAP_L4_CORE_BASE + OMAP_L4_CORE_SIZE
	/* Map first 1MB of L4 CORE 1:1 (so console will work) */
	MMU_INIT(OMAP_L4_CORE_BASE, OMAP_L4_CORE_BASE,
		(OMAP_L4_CORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

	/* Map first 4MB of L4 PERIPHERAL (so console will work) */
	MMU_INIT(OMAP_L4_PERIPHERAL_VBASE, OMAP_L4_PERIPHERAL_BASE,
		(OMAP_L4_PERIPHERAL_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)

#if OMAP_L4_PERIPHERAL_BASE <= CONSADDR \
	&& CONSADDR < OMAP_L4_PERIPHERAL_BASE + OMAP_L4_PERIPHERAL_SIZE
	/* Map first 1MB of L4 PERIPHERAL 1:1 (so console will work) */
	MMU_INIT(OMAP_L4_PERIPHERAL_BASE, OMAP_L4_PERIPHERAL_BASE,
		(OMAP_L4_PERIPHERAL_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

#if defined(OMAP_L4_WAKEUP_BASE) && defined(OMAP_L4_WAKEUP_VBASE)
	/* Map all 4MB of L4 WAKEUP (so console will work) */
	MMU_INIT(OMAP_L4_WAKEUP_VBASE, OMAP_L4_WAKEUP_BASE,
		(OMAP_L4_WAKEUP_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

#ifdef OMAP_L4_FAST_BASE
	/* Map first 1MB of L4 FAST (so console will work) */
	MMU_INIT(OMAP_L4_FAST_VBASE, OMAP_L4_FAST_BASE,
		(OMAP_L4_FAST_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

#ifdef OMAP_EMIF1_BASE
	/* Map first 1MB of EMIF1 (so we can probe memory size) */
	MMU_INIT(OMAP_EMIF1_VBASE, OMAP_EMIF1_BASE,
		(OMAP_EMIF1_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

#ifdef OMAP_EMIF2_BASE
	/* Map first 1MB of EMIF2 (so we can probe memory size) */
	MMU_INIT(OMAP_EMIF2_VBASE, OMAP_EMIF2_BASE,
		(OMAP_EMIF2_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

#ifdef OMAP_SDRC_BASE
	/* Map 64KB SDRAM Controller (SDRC) */
	MMU_INIT(OMAP_SDRC_VBASE, OMAP_SDRC_BASE,
		(OMAP_SDRC_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO | L1_S_APv7_KRW)
#endif

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(beagle_start))
