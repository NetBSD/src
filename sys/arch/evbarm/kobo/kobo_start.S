/*	$NetBSD: kobo_start.S,v 1.1 2014/07/25 11:22:50 hkenken Exp $	*/

/*-
 * Copyright (c) 2009 SHIMIZU Ryo <ryo@nerv.org>
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

/*
 * Copyright (c) 2002, 2003, 2010  Genetec Corporation.  All rights reserved.
 * Written by Kenichi Hashimoto and Hiroyuki Bessho for Genetec Corporation.
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
 */

#include "opt_imx.h"
#include "opt_com.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_arm_debug.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include "assym.h"

#include <arm/imx/imx51reg.h>
#include <arm/imx/imxuartreg.h>
#include <evbarm/kobo/kobo_reg.h>

RCSID("$NetBSD: kobo_start.S,v 1.1 2014/07/25 11:22:50 hkenken Exp $")

#if defined(VERBOSE_INIT_ARM)
#define DEBUG_STARTUP
#define	XPUTC(n) 	mov r0, n; bl _C_LABEL(debugputc)
#else
#define	XPUTC(n)
#endif

#ifndef SDRAM_START
#define SDRAM_START CSD0DDR_BASE
#endif
#define KERNEL_TEXT_ADDR	(SDRAM_START+0x00100000)

#define INIT_MEMSIZE	128
#define TEMP_L1_TABLE	(SDRAM_START + INIT_MEMSIZE * 0x100000 - L1_TABLE_SIZE)

#ifdef	DEBUG_STARTUP
#define	CHECKPOINT(n)	CHECKPOINT2(n,r0,r1)
#define	CHECKPOINT2(n,ra,rb)		\
	mov	ra, #0x30+(n); 		\
	ldr	rb, =UART2_BASE;	\
	str	ra, [rb, #IMX_UTXD];
#else
#define	CHECKPOINT(n)		/* nothing to do */
#define	CHECKPOINT2(n,ra,rb)	/* nothing to do */
#endif

	.section .start,"ax",%progbits

	.text

	.global _C_LABEL(kobo_start)
_C_LABEL(kobo_start):
	CHECKPOINT(0)

#ifdef	DEBUG_STARTUP
	ldr	sp,=SDRAM_START+0x4000

	bl	newline
	ldr	r0, =0xdeadb01f
	bl	_C_LABEL(debugprintx)
	bl	newline

	mov	r0, pc
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c0, c0, 3	/* read TLB type register */
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c1, c0, 0	/* read control register */
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c2, c0, 0	/* read TTB0 */
	bl	_C_LABEL(debugprintx)
	bl	newline

	mrc	p15, 0, r0, c2, c0, 1	/* read TTB1 */
	bl	_C_LABEL(debugprintx)
	bl	newline

	ldr	r0, =0xbabeface
	bl	_C_LABEL(debugprintx)
	bl	newline

	/* dump some of UART2 registers to know clock frequency */
	ldr	r4,=UART2_BASE
	ldr	r0,[r4,#IMX_UBMR]
	bl	_C_LABEL(debugprintx)
	bl	newline
	ldr	r0,[r4,#IMX_UBIR]
	bl	_C_LABEL(debugprintx)
	bl	newline
	ldr	r0,[r4,#IMX_UFCR]
	bl	_C_LABEL(debugprintx)
	bl	newline
#endif	/* DEBUG_STARTUP */

	/* Are we running on right place ? */
	ldr	r2, =KERNEL_TEXT_ADDR
	adr	r0, _C_LABEL(kobo_start)
	cmp	r0, r2
	beq	relocated

	/*
	 * move me to RAM
	 */
	ldr	r1, .Lcopy_size
	add	r1, r1, #3
	mov	r1, r1, LSR #2
	mov	r4, r2

	bhs	5f

	/* src < dest. copy from top */
	add r0,r0,r1,LSL #2
	add r2,r2,r1,LSL #2

3:	ldr r3,[r0,#-4]!
	str r3,[r2,#-4]!
	subs r1,r1,#1
	bhi 3b
	b 7f

	/* src >= dest. copy from bottom */
5:	ldr r3,[r0],#4
	str r3,[r2],#4
	subs r1,r1,#1
	bhi 5b
7:
	/*
	 * Okay, we are finished relocating the text segment.  Now
	 * we need to leap to the next instruction.
	 */
	ldr	r0, .Lrelocate_address
	ldr	r1, .Lrelocate_offset
	add	pc, r0, r1

.Lrelocate_offset:	.word relocated-_C_LABEL(kobo_start)

relocated:
	CHECKPOINT(1)

	/* Move into supervisor mode and disable IRQs/FIQs. */
	cpsid   if, #PSR_SVC32_MODE

	bl	cortex_init

	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	adr	r1, .Lmmu_init_table
	bl	arm_boot_l1pt_init

	CHECKPOINT(2)

	/*
	 * Turn on the MMU, Caches, etc.
	 */
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	bl	arm_cpuinit

	CHECKPOINT(3)

	movw	ip, #:lower16:start
	movt	ip, #:upper16:start
	bx	ip		/* Jump to start (flushes pipeline). */

	/* NOTREACHED */

/*
 * Calculate size of kernel to copy.  Don't bother to copy bss,
 * although I guess the CPU could use the warmup exercise ...
 */
.Lcopy_size:
        .word	_edata - _C_LABEL(kobo_start)

.Lrelocate_address:
	.word	KERNEL_BASE_phys

#include <arm/cortex/a9_mpsubr.S>

.Lmmu_init_table:
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000, 1 << (32 - L1_S_SHIFT),
		L1_S_PROTO_armv7 | L1_S_APv7_KRW)

	/* Map memory 1:1 VA to PA, write-back cacheable, shareable */
	MMU_INIT(SDRAM_START, SDRAM_START, MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, SDRAM_START, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE, DDRSDRAM_BASE, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

	/* Map all 256KB of L4 WAKEUP (so console will work) */
	MMU_INIT(KOBO_IO_VBASE0, KOBO_IO_PBASE0, 4,
		L1_S_PROTO | L1_S_APv7_KRW | L1_S_V6_XN)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

#ifdef	DEBUG_STARTUP
.Luart0adr:
	.word	UART2_BASE

	.global _C_LABEL(debugprintx)
_C_LABEL(debugprintx):
	stmfd	sp!, {r0, r3, lr}

	mov	r3, r0

	mov	r0, #'0'
	bl	debugputc
	mov	r0, #'x'
	bl	debugputc

	bl	print_r3

	ldmfd	sp!, {r0, r3, pc}

	.global	_C_LABEL(debugprint)
_C_LABEL(debugprint):
	stmfd	sp!, {r0, r1, lr}
	mov	r1, r0
1:
	ldrb	r0, [r1], #1
	cmp	r0, #0
	beq	9f
	bl	debugputc
	b	1b
9:
	ldmfd	sp!, {r0, r1, pc}

print_r3:
	stmfd	sp!, {r0, r3-r6, lr}
	mov	r4, #28
	mov	r5, #0xf
1:
	and	r6, r5, r3, ROR r4
	cmp	r6, #10
	addlt	r0, r6, #'0'
	addge	r0, r6, #('a' - 0x0a)
	bl	debugputc
	subs	r4, r4, #4
	bge	1b
	ldmfd	sp!, {r0, r3-r6, pc}

	.global	_C_LABEL(debugputc)
_C_LABEL(debugputc):
	stmfd	sp!, {r0, r1, r2, lr}

	ldr	r1, .Luart0adr

1:	/* wait */
	ldr	r2, [r1, #0x98]
	tst	r2, #0x4000
	beq	1b

	/* output */
	strb	r0, [r1, #0x40]

	ldmfd	sp!, {r0, r1, r2, pc}

newline:
	mov	r1, lr
	mov	r0, #0x0d
	bl	_C_LABEL(debugputc)
	mov	lr, r1
	mov	r0, #0x0a
	b	_C_LABEL(debugputc)

#endif	/* DEBUG_STARTUP */

END(_C_LABEL(kobo_start))
