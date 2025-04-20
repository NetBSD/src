/*	$NetBSD: execsp.S,v 1.1 2025/04/20 22:32:49 riastradh Exp $	*/

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

RCSID("$NetBSD: execsp.S,v 1.1 2025/04/20 22:32:49 riastradh Exp $")

	.import _GLOBAL_OFFSET_TABLE_

/*
 * void execsp_start(struct ps_strings *ps_strings@arg0,
 *     void (*cleanup@arg1)(void), void *obj_main@arg2)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
LEAF_ENTRY(execsp_start)
	/*
	 * Set up the data pointer (r19) and linkage table register
	 * (r27) like the real startup routine so we can get at the
	 * global symbols startsp and __start.
	 *
	 * XXX Not really sure why we need to set up r27, since we only
	 * use r19 here and the real startup routine, __start, will set
	 * up both r19 and r27 anyway.  But this crashes with SIGSEGV
	 * shortly after startup if we don't set up r27, and gdb
	 * crashes on my attempts to single-step, so I'll just leave
	 * the initialization of r27 here for now until someone is
	 * motivated by the potential for a single-instruction
	 * micro-optimization in this test program to find out why r27
	 * is needed too.
	 */
	bl	L$lpc, %r27
	 depi	0, 31, 2, %r27
L$lpc:	addil	L'_GLOBAL_OFFSET_TABLE_ - ($PIC_pcrel$0 - 8), %r27
	ldo	R'_GLOBAL_OFFSET_TABLE_ - ($PIC_pcrel$0 - 12)(%r1), %r27
	copy	%r27, %r19

	addil	LT%_C_LABEL(startsp), %r19	/* r20 := &startsp */
	ldw	RT%_C_LABEL(startsp)(%r1), %r20

	/* PIC_TAILCALL(__start), if we had it */
	addil	LT%_C_LABEL(__start), %r19	/* r1 := __start */
	ldw	RT%_C_LABEL(__start)(%r1), %r1
	bv	%r0(%r1)			/* jump to __start */
	 stw	%sp, 0(%r20)			/* startsp := sp */
EXIT(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
LEAF_ENTRY(execsp_ctor)
	addil	LT%_C_LABEL(ctorsp), %r19	/* r1 := &ctorsp */
	ldw	RT%_C_LABEL(ctorsp)(%r1), %r1
	bv	%r0(%rp)			/* return */
	 stw	%sp, 0(%r1)			/* ctorsp := sp */
EXIT(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.section .ctors,"aw",@progbits
	.p2align 2
	.word	_C_LABEL(execsp_ctor)

/*
 * int main(int argc@arg0, char **argv@arg1, ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
LEAF_ENTRY(main)
	addil	LT%_C_LABEL(mainsp), %r19	/* r1 := &mainsp */
	ldw	RT%_C_LABEL(mainsp)(%r1), %r1
	stw	%sp, 0(%r1)			/* ctorsp := sp */
	bv	%r0(%rp)			/* return... */
	 copy	%r0, %ret0			/* ...zero */
EXIT(main)

/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
LEAF_ENTRY(execsp_dtor)
	addil	LT%_C_LABEL(dtorsp), %r19	/* r20 := &dtorsp */
	ldw	RT%_C_LABEL(dtorsp)(%r1), %r20

	/* PIC_TAILCALL(__start), if we had it */
	addil	LT%_C_LABEL(execsp_main), %r19	/* r1 := execsp_main */
	ldw	RT%_C_LABEL(execsp_main)(%r1), %r1
	bv	%r0(%r1)			/* jump to execsp_main */
	 stw	%sp, 0(%r20)			/* startsp := sp */
EXIT(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.section .dtors,"aw",@progbits
	.p2align 2
	.word	_C_LABEL(execsp_dtor)
