/*	$NetBSD: h_execregs.S,v 1.1 2025/04/27 02:24:07 uwe Exp $	*/

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
#include <machine/asm.h>
#include <machine/mcontext.h>
#include <sys/syscall.h>


// ELF entry point
ENTRY(execregs_start)
	// __gregset_t r;	// mcontext (but general regs only)
	mov.l	sp, @-sp
	mov.l	r0, @-sp
	mov.l	r1, @-sp
	mov.l	r2, @-sp
	mov.l	r3, @-sp
	mov.l	r4, @-sp
	mov.l	r5, @-sp
	mov.l	r6, @-sp
	mov.l	r7, @-sp
	mov.l	r8, @-sp
	mov.l	r9, @-sp
	mov.l	r10, @-sp
	mov.l	r11, @-sp
	mov.l	r12, @-sp
	mov.l	r13, @-sp
	mov.l	r14, @-sp
	sts.l	pr, @-sp
	sts.l	mach, @-sp
	sts.l	macl, @-sp
	mov	#0, r0
	mov.l	r0, @-sp	// _REG_SR is privileged
	mova	.Lend, r0	// _REG_PC
	mov.l	@r0, r1
	sub	r1, r0
	mov.l	r0, @-sp
	stc.l	gbr, @-sp

	// write(STDOUT_FILENO, &r, sizeof(__gregset_t))
	mov	#1, r4
	mov	sp, r5
	mov	#(_NGREG * 4), r6
	mov	#SYS_write, r0
	trapa	#0x80

	// _exit(0)
	mov	#0, r4
	mov	#SYS_exit, r0
	trapa	#0x80

	.p2align 2
.Lend:	.long	.Lend - execregs_start
	SET_ENTRY_SIZE(execregs_start)


// main stub to simplify linking
ENTRY(main)
	rts
	 mov	#0, r0
	SET_ENTRY_SIZE(main)
