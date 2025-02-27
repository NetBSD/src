/*	$NetBSD: h_execregs.S,v 1.1 2025/02/27 00:55:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#define	_LOCORE

#include <sys/syscall.h>

#include <machine/asm.h>

#include "execregs.h"

#define REGSIZE 4
#define BUFSIZE (NEXECREGS * REGSIZE)
#define SLOT(n)	(n)*REGSIZE(%sp)

ENTRY(execregs_start, 0)
	/* store registers to a buffer on stack */
	subl2	$BUFSIZE,%sp		/* space for NEXECREGS registers */
	movl	%fp,SLOT(0)		/* order matches execregs.h */
	movl	%ap,SLOT(1)
	/* sp: stack pointer */
	movl	%r0,SLOT(2)
	movl	%r1,SLOT(3)
	movl	%r2,SLOT(4)
	movl	%r3,SLOT(5)
	movl	%r4,SLOT(6)
	movl	%r5,SLOT(7)
	/* r6: initial stack pointer */
	movl	%r7,SLOT(8)
	movl	%r8,SLOT(9)
	/* r9: ps_strings */
	movl	%r10,SLOT(10)
	movl	%r11,SLOT(11)

	/* call write(STDOUT_FILENO, regs, sizeof(regs)) */
	pushl	$BUFSIZE		/* arg2 := sizeof(regs) */
	pushal	4(%sp)			/* arg1 := regs */
	pushl	$1			/* arg0 := STDOUT_FILENO */
	pushl	$3			/* number of arguments */
	movl	%sp,%ap			/* argument pointer */
	chmk	$SYS_write

	bcs	2f			/* bail if write failed */
	cmpl	$BUFSIZE,%r0		/* bail if wrote wrong # of bytes */
	bneq	2f

	/* call exit(0) */
	pushl	$0			/* arg0 := 0 */
1:	pushl	$1			/* number of arguments */
	movl	%sp,%ap			/* argument pointer */
	chmk	$SYS_exit
	.word	0xffff			/* paranoia -- illegal opcode */

2:	/* call exit(127) */
	pushl	$127			/* arg0 := 127 */
	jmp	1b
END(execregs_start)

/* main stub to simplify linking */
ENTRY(main, 0)
	.word	0xffff			/* illegal opcode */
END(main)
