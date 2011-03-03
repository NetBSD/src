/*	$NetBSD: entry.S,v 1.1 2011/03/03 05:59:37 kiyohara Exp $	*/
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

#if defined(SH3) && defined(SH4)
#error "mmeye port don't support SH3,SH4 common kernel."	
#elif defined(SH3)
#define RAM_BEGIN	0x8c000000	/* Area3 */
#define RAM_SIZE	0x01000000	/* 16M */
#elif defined(SH4)
#define RAM_BEGIN	0x88000000	/* Area2 */
#define RAM_SIZE	0x04000000	/* 64M */
#endif

#define	INIT_STACK	(RAM_BEGIN + RAM_SIZE - 0x00001000)

NENTRY(start)
	/* Set SP to initial position */
	mov.l	XLtmpstk, r15

	__INTR_MASK(r0, r1)

	/* Set Register Bank to Bank 0 */
	mov.l	SR_init, r0
	ldc	r0, sr

	xor	r0, r0
	MOV	(MMUCR, r2)	
	mov.l	r0, @r2		/* MMU OFF */

	bra	start1
	nop
	.align	2
SR_init:	.long	0x500000F0
REG_SYMBOL(MMUCR)	
start1:

#if defined(SH4)
	/* Copy boot image from _LOADADDR to ___start */
	mov.l	_LOADADDR, r1
	mov.l	___end, r0
	mov.l	___start, r3
	sub	r3, r0
	add	#4, r0		/* size of bytes to be copied */
	shlr2	r0		/* number of long word */
1:
	mov.l	@r1+, r4
	mov.l	r4, @r3
	add	#4, r3
	dt	r0		/* decrement and Test */
	bf	1b
	/* boot image copy end */
#endif

	mov.l	XLmain, r0
	jmp	@r0		/* jump to main() */
	nop

		.align	2

_LOADADDR:	.long	0x88010000	/* loaded here by 1st loader. */
___start:	.long	start
___end:		.long	_end
XLtmpstk:	.long	INIT_STACK
XLmain:		.long	_C_LABEL(main)
