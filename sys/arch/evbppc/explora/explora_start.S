/*	$NetBSD: explora_start.S,v 1.13 2022/06/04 22:32:20 rin Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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

/*
 * Initial state:
 *
 * iccr = 0x00008001 0x80000000-0x87ffffff 0xf80000000-0xffffffff
 * dccr = 0x00008001 0x80000000-0x87ffffff 0xf80000000-0xffffffff
 * dcwr = 0x00000000
 * msr  = 0x00001000 ME=machine check enable
 *
 */

#include "assym.h"

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/dcr403cgx.h>

#include "opt_ddb.h"
#include "opt_ppcparam.h"

/*
 * Initially the dram starts at 0x01000000. This is way too high.
 * We relocate dram to 0x00000000. We use the video ram at 0xf0000000
 * as a temporary staging area.
 */

#define STAGE1_BASE	0xf0000000

	.text
	.globl	__start
__start:
	b	1f
	nop
	nop
	.long	0
	.ascii	"XncdPPC\0"
	.long	0
	.long	0

1:
	/* Disable exceptions, caches, invalidate all TLB's. */

	li	%r0,0
	mtmsr	%r0
	mttcr	%r0
	mtdccr	%r0
	mticcr	%r0
	sync
	isync

/* Clear caches and invalidate tlbs */
	li	%r7,256
	mtctr	%r7
	li	%r6,0
1:
	dccci	%r0,%r6
	addi	%r6,%r6,16
	bdnz	1b

	li	%r7,512
	mtctr	%r7
	li	%r6,0
1:
	iccci	%r0,%r6
	addi	%r6,%r6,16
	bdnz	1b

	tlbia
	sync
	isync

/* Get current address -- NOT the same as . */

	bl	_next
_next:
	mflr	%r3
	subi	%r3,%r3,_next-__start
	lis	%r4,STAGE1_BASE@h
	ori	%r4,%r4,STAGE1_BASE@l
	li	%r5,stage1size

1:
	lbz	%r1,0(%r3)
	mr	%r0,%r5
	cmpwi	%r0,0
	stb	%r1,0(%r4)
	addi	%r3,%r3,1
	addi	%r4,%r4,1
	addi	%r5,%r5,-1
	bgt	1b

/* Jump into the staging area so we can remap the dram. */

	lis	%r0,stage1reloc@h
	ori	%r0,%r0,stage1reloc@l
	mtlr	%r0
	blr

stage1reloc = .-__start+STAGE1_BASE

/* Remap the dram from 0x01000000 to 0x00000000. */

#define REMAP(r, tmp1, tmp2) \
	mfbr##r	tmp1 ; \
	lis	tmp2,0xff ; \
	ori	tmp2,tmp2,0xffff ; \
	cmplw	tmp1,tmp2 ; \
	ble	1f ; \
	addis	tmp1,tmp1,0xf000 ; \
	mtbr##r	tmp1 ; \
1:

	REMAP(4, %r1, %r2)
	REMAP(5, %r1, %r2)
	REMAP(6, %r1, %r2)
	REMAP(7, %r1, %r2)

#undef REMAP

/* Initial setup. */

	ba	stage2

stage2:

#ifdef PPC_4XX_NOCACHE
	li	%r0,0
#else
	lis	%r0,0xfffc
#endif
	mtdccr	%r0
	mticcr	%r0
	sync
	isync

/* get start of bss */
	lis	%r7,_C_LABEL(edata)-4@h
	ori	%r7,%r7,_C_LABEL(edata)-4@l
/* get end of kernel */
	lis	%r4,_C_LABEL(end)@h
	ori	%r4,%r4,_C_LABEL(end)@l
/* clear bss */
	li	%r3,0
1:
	stwu	%r3,4(%r7)
	cmpw	%r7,%r4
	bne+	1b

/* Set kernel MMU context. */
	li	%r0,KERNEL_PID
	mtpid	%r0
	sync

	INIT_CPUINFO(%r4,%r1,%r9,%r0)

	lis	%r3,__start@h
	ori	%r3,%r3,__start@l

/* Run the remaining setup in C. */
	bl	_C_LABEL(initppc)

	bl	_C_LABEL(main)

	/* NOTREACHED */
2:	nop
	b	2b

stage1size = .-__start

#include <powerpc/ibm4xx/4xx_locore.S>
