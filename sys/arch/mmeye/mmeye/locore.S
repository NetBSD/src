/*	$NetBSD: locore.S,v 1.3 2002/03/17 14:05:12 uch Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1997
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */

#include "opt_memsize.h"	
#include "assym.h"

#if defined(SH3) && defined(SH4)	
#error "mmeye port don't support SH3,SH4 common kernel."	
#endif

#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/trapreg.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>	
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

#define INIT_STACK	0x8c3ff000

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

#ifdef ROMIMAGE
	/* Initialize BUS State Control Regs. */
	mov.l	_ROM_START, r3
	mov.l	_C_LABEL(ram_start), r4
	sub	r3, r4
	/* Set Bus State Controler */
	mov.l	XLInitializeBsc, r0
	sub	r4, r0
	jsr	@r0
	nop

	/* Move kernel image from ROM area to RAM area */
	mov.l	___end, r0
	mov.l	___start, r1
	mov.l	_KERNBASE, r2
	sub	r2, r0
	sub	r2, r1
	sub	r1, r0
	add	#4, r0		/* size of bytes to be copied */
	shlr2	r0		/* number of long word */
	mov.l	_ROM_START, r3
	add	r3, r1		/* src address */
	mov.l	___start, r3
	sub	r2, r3
	mov.l	_C_LABEL(ram_start), r4
	add	r4, r3		/* dest address */
1:
	mov.l	@r1+, r4
	mov.l	r4, @r3
	add	#4, r3
	dt	r0		/* decrement and Test */
	bf	1b
	/* kernel image copy end */

	mov.l	LXstart_in_RAM, r0
	jmp	@r0		/* jump to RAM area */
	nop

	.align	2
LXstart_in_RAM:
	.long	start_in_RAM
#else /* ROMIMAGE */
	/* Set Bus State Controler */
	mov.l	XLInitializeBsc, r0
	jsr	@r0
	nop
#endif /* ROMIMAGE */

start_in_RAM:
	mova	1f, r0
	mov	r0, r4
	mov.l	XLinitSH3, r0
	jsr	@r0		/* call initSH3() */
	nop

	.align	2
1:

	mov.l	XLmain, r0
	jsr	@r0		/* call main() */
	nop

		.align	2

XLInitializeBsc:.long	_C_LABEL(InitializeBsc)
___start:	.long	start
___etext:	.long	_etext
___end:		.long	_end
XLtmpstk:	.long	INIT_STACK
_KERNBASE:	.long	0x8c000000
_ROM_START:	.long	IOM_ROM_BEGIN
XLinitSH3:	.long	_C_LABEL(initSH3)
XLmain:		.long	_C_LABEL(main)

load_and_reset:
	mov.l	XL_start_address, r0
	mov	r0, r8
	mov.l	@r4+, r1	/* r1 = osimage size */
	mov.l	@r4+, r2	/* r2 = check sum */
	shlr2	r1		/* r1 = osimage size in dword */
1:
	mov.l	@r4+, r3
	mov.l	r3, @r0
	add	#4, r0
	dt	r1
	bf	1b

	jmp	@r8		/* jump to start address */
	nop

	.align	2
XL_start_address:
	.long	IOM_RAM_BEGIN + 0x00010000
load_and_reset_end:

ENTRY(XLoadAndReset)
	__INTR_MASK(r0, r1)
	/* copy trampoline code to RAM area top */
	mov.l	XL_load_and_reset, r0
	mov.l	XL_load_and_reset_end, r1
	mov.l	XL_load_trampoline_addr, r2
	mov	r2, r8
	sub	r0, r1		/* r1 = bytes to be copied */
1:	mov.b	@r0+, r3
	mov.b	r3, @r2
	add	#1, r2
	dt	r1
	bf	1b

	jmp	@r8		/* jump to trampoline code */
	nop

	.align	2
XL_load_trampoline_addr:
	.long	IOM_RAM_BEGIN + 0x00008000
XL_load_and_reset:
	.long	load_and_reset
XL_load_and_reset_end:
	.long	load_and_reset_end

ENTRY(Sh3Reset)
	mov.l	XL_reset_vector, r8
	jmp	@r8
	nop

	.align	2
XL_reset_vector:
	.long	0xa0000000

/*
 * void interrupt_exp(int, int, int, int, stuct trapframe)
 *    __attribute__((__noreturn__)):
 *	on entry, SR.BL = 1, SR_RB = 0, all regsiters are saved,
 *	stack is already setuped.	
 */
	.align	2
recurse:
	stc	sr, r4
	ldc	r5, spc	
	ldc	r4, ssr
	RECURSEENTRY
	bra	1f
	 nop
NENTRY(interrupt_exp)
1:
	MOV	(INTEVT, r0)
	mov.l	@r0,	r0
	mov.l	r0, @(TF_TRAPNO, r14)	/* trapframe->tf_trapno = INTEVT */
	__INTR_MASK(r0, r1)		/* mask all interrupt */
	__EXCEPTION_UNBLOCK(r0, r1)	/* enable exception for TLB handling */
	mov.l	_L.intrhandler, r0
	jsr	@r0
 	 mov	r14,	r6
	tst	r0,	r0	/* intrhandler() == 0, fast intr return */
	bt	2f

	mov.l	_L.check_ipending, r0
	jsr	@r0
	 nop
	tst	r0,	r0
	bf	1b		/* handle pending interrupt. */

	/* Check for ASTs on exit to user mode. */
	mov.l	_L.ast,	r0
	jsr	@r0
	 mov	r14,	r4
2:	
	EXCEPTION_RETURN
	/* NOTREACHED */
	.align	2
REG_SYMBOL(INTEVT)
_L.intrhandler:		.long	_C_LABEL(intrhandler)
_L.ast:			.long	_C_LABEL(ast)

NENTRY(Xspllower)
	sts.l	pr,	@-r15

restart:
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	_L.check_ipending, r0
	jsr	@r0
	 nop
	tst	r0,	r0
	bt	1f

	mov.l	_L.restart, r5 
	mov.l	_L.recurse, r0
	jmp	@r0
	 nop

1:
	__INTR_UNMASK(r0, r1)
	lds.l	@r15+,	pr
	rts
	 nop

	.align	2
_L.check_ipending:	.long	_C_LABEL(check_ipending)
_L.recurse:		.long	recurse
_L.restart:		.long	restart

	.data
	.align	2
	.globl	_C_LABEL(intrcnt), _C_LABEL(eintrcnt)
	.globl	_C_LABEL(intrnames), _C_LABEL(eintrnames)
_C_LABEL(intrcnt):
_C_LABEL(eintrcnt):
_C_LABEL(intrnames):
_C_LABEL(eintrnames):
