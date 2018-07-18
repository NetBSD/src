/*	$NetBSD: fdt_start.S,v 1.3 2018/07/18 06:50:48 ryo Exp $	*/

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

#include "opt_arm_debug.h"	/* VERBOSE_INIT_ARM */

#include <sys/cdefs.h>

#include <aarch64/asm.h>

RCSID("$NetBSD: fdt_start.S,v 1.3 2018/07/18 06:50:48 ryo Exp $")

#define PRINT(string)	\
	bl xprint;.asciz string;.align 2

/* load far effective address (pc relative) */
.macro	ADDR, reg, addr
	adrp	\reg, \addr
	add	\reg, \reg, #:lo12:\addr
.endm

/*
 * Kernel start routine for aarch64 boards.
 */
	.global start
start:
	adr	x9, start
	ldr	x10, =start

	sub	x10, x10, x9

	/* address of kern_vtopdiff (relative) */
	ADDR	x8, kern_vtopdiff
	str	x10, [x8]	/* kern_vtopdiff = start(virt) - start(phys) */

	/*
	 * store uboot arguments to uboot_args[4]
	 */

	/* address of uboot_args (relative) */
	ADDR	x8, uboot_args
	str	x0, [x8, #(8*0)]
	str	x1, [x8, #(8*1)]
	str	x2, [x8, #(8*2)]
	str	x3, [x8, #(8*3)]

	/*
	 * ARM64 boot protocol has FDT address in x0 *
	 */
	ADDR	x8, fdt_addr_r
	str	x0, [x8]


#ifdef VERBOSE_INIT_ARM
	PRINT("boot NetBSD/evbarm (aarch64)\r\n")
#endif

	b	aarch64_start		/* aarch64_start() @ aarch64/locore.S */

ENTRY_NP(uartputc)
#ifdef EARLYCONS
	b	___CONCAT(EARLYCONS, _platform_early_putchar)
#endif
	ret
