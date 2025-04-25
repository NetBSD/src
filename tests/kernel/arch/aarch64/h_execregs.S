/*	$NetBSD: h_execregs.S,v 1.2 2025/04/25 12:58:40 riastradh Exp $	*/

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

ENTRY(execregs_start)
	/* create a stack frame with NEXECREGS*8 bytes, aligned to 16-byte */
	stp	fp, lr, [sp, #-(16 + ((NEXECREGS*8 + 15)/16)*16)]!

	/* store registers to buffer on stack */
	stp	x0, x1, [sp, #16]	/* order matches execregs.h */
	/* x2: ps_strings */
	stp	x3, x4, [sp, #(16 + 1*2*8)]
	stp	x5, x6, [sp, #(16 + 2*2*8)]
	stp	x7, x8, [sp, #(16 + 3*2*8)]
	stp	x9, x10, [sp, #(16 + 4*2*8)]
	stp	x11, x12, [sp, #(16 + 5*2*8)]
	stp	x13, x14, [sp, #(16 + 6*2*8)]
	stp	x15, x16, [sp, #(16 + 7*2*8)]
	stp	x17, x18, [sp, #(16 + 8*2*8)]
	stp	x19, x20, [sp, #(16 + 9*2*8)]
	stp	x21, x22, [sp, #(16 + 10*2*8)]
	stp	x23, x24, [sp, #(16 + 11*2*8)]
	stp	x25, x26, [sp, #(16 + 12*2*8)]
	stp	x27, x28, [sp, #(16 + 13*2*8)]
	stp	x29, x30, [sp, #(16 + 14*2*8)]
	mrs	x0, tpidr_el0
	str	x0, [sp, #(16 + 15*2*8)]

	/* call write(STDOUT_FILENO, regs, sizeof(regs)) */
	mov	x0, #1			/* arg0 := STDOUT_FILENO */
	add	x1, sp, #16		/* arg1 := regs */
	mov	x2, #(NEXECREGS*8)	/* arg2 := sizeof(regs) */
	svc	#SYS_write

	b.cs	2f			/* bail if write failed */
	cmp	x0, #(NEXECREGS*8)	/* bail if wrote wrong # of bytes */
	b.ne	2f

	/* call exit(0) */
	mov	x0, #0			/* arg0 := 0 */
1:	svc	#SYS_exit
	brk	#0xffff			/* paranoia */

2:	/* call exit(127) */
	mov	x0, #127		/* arg0 := 127 */
	b	1b
END(execregs_start)

/* main stub to simplify linking */
ENTRY(main)
	brk	#0xffff
END(main)
