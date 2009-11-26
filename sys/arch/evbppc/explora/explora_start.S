/*	$NetBSD: explora_start.S,v 1.6 2009/11/26 00:19:16 matt Exp $	*/

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

#define _NOREGNAMES

#include <machine/param.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/asm.h>

#include <powerpc/spr.h>
#include <powerpc/ibm4xx/dcr403cgx.h>
#include <powerpc/ibm4xx/pmap.h>

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

	li	0,0
	mtmsr	0
	mttcr	0
	mtdccr	0
	mticcr	0
	sync
	isync

/* Clear caches and invalidate tlbs */
	li	7,256
	mtctr	7
	li	6,0
1:
	dccci	0,6
	addi	6,6,16
	bdnz	1b

	li	7,512
	mtctr	7
	li	6,0
1:
	iccci	0,6
	addi	6,6,16
	bdnz	1b

	tlbia
	sync
	isync

/* Get current address -- NOT the same as . */

	bl	_next
_next:
	mflr	3
	subi	3,3,_next-__start
	lis	4,STAGE1_BASE@h
	ori	4,4,STAGE1_BASE@l
	li	5,stage1size

1:
	lbz	1,0(3)
	mr	0,5
	cmpwi	0,0
	stb	1,0(4)
	addi	3,3,1
	addi	4,4,1
	addi	5,5,-1
	bgt	1b

/* Jump into the staging area so we can remap the dram. */

	lis	0,stage1reloc@h
	ori	0,0,stage1reloc@l
	mtlr	0
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

	REMAP(4, 1, 2)
	REMAP(5, 1, 2)
	REMAP(6, 1, 2)
	REMAP(7, 1, 2)

#undef REMAP

/* Initial setup. */

	ba	stage2

stage2:

#ifdef PPC_4XX_NOCACHE
	li	0,0
#else
	lis	0,0xfffc
#endif
	mtdccr	0
	mticcr	0
	sync
	isync

/* get start of bss */
	lis	7,_C_LABEL(edata)-4@h
	ori	7,7,_C_LABEL(edata)-4@l
/* get end of kernel */
	lis	4,_C_LABEL(end)@h
	ori	4,4,_C_LABEL(end)@l
/* clear bss */
	li	3,0
1:
	stwu	3,4(7)
	cmpw	7,4
	bne+	1b

	INIT_CPUINFO(4,1,9,0)

	lis	3,__start@h
	ori	3,3,__start@l

/* Run the remaining setup in C. */
	bl	_C_LABEL(bootstrap)

	bl	_C_LABEL(main)

	/* NOTREACHED */
2:	nop
	b	2b

stage1size = .-__start

#include <powerpc/ibm4xx/4xx_locore.S>
