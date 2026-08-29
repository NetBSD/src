/*	$NetBSD: execsp.S,v 1.1 2026/08/29 13:40:17 riastradh Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

RCSID("$NetBSD: execsp.S,v 1.1 2026/08/29 13:40:17 riastradh Exp $")

/*
 * void execsp_start(void *obj_main@a0, void (*cleanup@a1)(void),
 *     struct ps_strings *ps_strings@a2)
 *
 *	ELF entry point.  Saves the stack pointer in startsp and defers
 *	to the usual csu __start routine.
 */
ENTRY(execsp_start)
	movl	%sp,%d0
#ifdef __PIC__
	GOT_SETUP(%a3)
	movl	_C_LABEL(startsp)@GOT:w(%a3),%a3
	movl	%d0,(%a3)
#else
	movl	%d0,_C_LABEL(startsp)
#endif
	jbsr	PIC_PLT(_C_LABEL(__start))
END(execsp_start)

/*
 * void execsp_ctor(void)
 *
 *	ELF constructor.  Saves the stack pointer in ctorsp and
 *	returns.
 */
ENTRY(execsp_ctor)
	movl	%sp,%d0
#ifdef __PIC__
	GOT_SETUP(%a0)
	movl	_C_LABEL(ctorsp)@GOT:w(%a0),%a0
	movl	%d0,(%a0)
#else
	movl	%d0,_C_LABEL(ctorsp)
#endif
	rts
END(execsp_ctor)

	/* Make execsp_ctor a constructor. */
	.pushsection .ctors,"aw",@progbits
	.p2align 2
	.long	execsp_ctor
	.popsection

/*
 * int main(int argc@sp[4,8), char **argv@sp[8,12), ...)
 *
 *	Main function.  Saves the stack pointer in mainsp and returns
 *	zero.  We will call execsp_main in execsp_dtor once dtorsp has
 *	been initialized.
 */
ENTRY(main)
	movl	%sp,%d0
#ifdef __PIC__
	GOT_SETUP(%a0)
	movl	_C_LABEL(mainsp)@GOT:w(%a0),%a0
	movl	%d0,(%a0)
#else
	movl	%d0,_C_LABEL(mainsp)
#endif
	clrl	%d0
	rts
END(main)

/*
 * void execsp_dtor(void)
 *
 *	ELF destructor.  Saves the stack pointer in dtorsp and defers
 *	to the C execsp_main in h_execsp.c to report the stack pointers
 *	back to the t_signal_and_sp parent.
 */
ENTRY(execsp_dtor)
	movl	%sp,%d0
#ifdef __PIC__
	GOT_SETUP(%a0)
	movl	_C_LABEL(dtorsp)@GOT:w(%a0),%a0
	movl	%d0,(%a0)
#else
	movl	%d0,_C_LABEL(dtorsp)
#endif
	jra	PIC_PLT(_C_LABEL(execsp_main))
END(execsp_dtor)

	/* Make execsp_ctor a destructor. */
	.pushsection .dtors,"aw",@progbits
	.p2align 2
	.long	execsp_dtor
	.popsection
