/*	$NetBSD: start.S,v 1.13 2022/08/23 05:31:12 ryo Exp $	*/

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_arm_debug.h"	/* VERBOSE_INIT_ARM and EARLYCONS */

#include <sys/cdefs.h>

#include <aarch64/asm.h>
#include "assym.h"

RCSID("$NetBSD: start.S,v 1.13 2022/08/23 05:31:12 ryo Exp $")

/*
 * Padding at start of kernel image to make room for 64-byte header
 * (non-ELF booting)
 */
.header:
	.space	64, 0x0

/*
 * Kernel start routine for aarch64 boards.
 */
	.global start
start:
	/* DON'T CLOBBER X0-X3 REGISTERS. THEY ARE UBOOT ARGUMENTS */

	/*
	 * Relocate to L2_SIZE(2Mbyte) align if necessary
	 *
	 * x8 = currently loaded address
	 * x9 = (x8 + L2_SIZE - 1) & -L2_SIZE = new (aligned) loaded address
	 */
	adrl	x8, .header
	mov	x9, #(L2_SIZE-1)
	add	x9, x9, x8
	and	x9, x9, #-L2_SIZE
	cmp	x8, x9
	b.eq	9f

	/* x10 = size = (_edata - __kernel_text) */
	adrl	x10, _edata
	adrl	x11, __kernel_text
	sub	x10, x10, x11

	/* do memmove(x9, x8, x10) */
	add	x8, x8, x10
	add	x13, x9, x10
1:
	ldp	x11, x12, [x8, #-16]!
	stp	x11, x12, [x13, #-16]!
	cmp	x13, x9
	b.hi	1b

	/* jump to new (aligned) loaded address */
	add	x9, x9, #(start - .header)	/* skip header */
	br	x9
9:


	/*
	 * Zero the BSS
	 */
	adrl	x8, __bss_start__
	adrl	x9, __bss_end__
	/* while (x8 < x9) *(uint128_t *)x8++ = 0; */
	b	2f
1:	stp	xzr, xzr, [x8], #16
2:	cmp	x8, x9
	b.lo	1b


	mrs	x8, CurrentEL
	lsr	x8, x8, #2
	cmp	x8, #0x2
	b.lo	1f

	mrs	x8, sctlr_el2
#ifdef __AARCH64EB__
	orr	x8, x8, #SCTLR_EE	/* set: Big Endian */
#else
	bic	x8, x8, #SCTLR_EE	/* clear: Little Endian */
#endif
	msr	sctlr_el2, x8
	isb

1:
	mrs	x8, sctlr_el1
#ifdef __AARCH64EB__
	orr	x8, x8, #(SCTLR_EE | SCTLR_E0E)	/* set: Big Endian */
#else
	bic	x8, x8, #(SCTLR_EE | SCTLR_E0E)	/* clear: Little Endian */
#endif
	msr	sctlr_el1, x8
	isb


	adr	x9, start
	ldr	x10, =start

	sub	x10, x10, x9

	/* address of kern_vtopdiff (relative) */
	adrl	x8, kern_vtopdiff
	str	x10, [x8]	/* kern_vtopdiff = start(virt) - start(phys) */

	/*
	 * store uboot arguments to uboot_args[4]
	 */

	/* address of uboot_args (relative) */
	adrl	x8, uboot_args
	str	x0, [x8, #(8*0)]
	str	x1, [x8, #(8*1)]
	str	x2, [x8, #(8*2)]
	str	x3, [x8, #(8*3)]

	/*
	 * ARM64 boot protocol has FDT address in x0 *
	 */
	adrl	x8, fdt_addr_r
	str	x0, [x8]

	adrl	x8, pmapboot_pagebase
	ldr	x9, =ARM_BOOTSTRAP_LxPT
	sub	x9, x9, x10
	str	x9, [x8]

	b	aarch64_start		/* aarch64_start() @ aarch64/locore.S */

ENTRY_NP(uartputc)
#ifdef EARLYCONS
	b	___CONCAT(EARLYCONS, _platform_early_putchar)
#endif
	ret
