/*	$NetBSD: resumecontext.S,v 1.3 2003/01/26 16:22:01 fvdl Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/asm.h>

#if defined(LIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: resumecontext.S,v 1.3 2003/01/26 16:22:01 fvdl Exp $")
#endif /* LIBC_SCCS && !lint */

/*
 * This assembly-language implementation differs from the (obvious)
 * C-language implementation only in not clobbering the previous
 * function's return address (us), which is the point of the exercise.
 */

NENTRY(_resumecontext)	/* profiling prologue would clobber TOS */
	subl	$(4 + 512),%esp
	movl	%esp,%ecx
	PIC_PROLOGUE
	pushl	%ecx
#ifdef PIC
	call	PIC_PLT(_C_LABEL(_getcontext))
#else
	call	_C_LABEL(_getcontext)
#endif
	addl	$4,%esp
	PIC_EPILOGUE
	movl	4(%esp),%ecx
	PIC_PROLOGUE
	pushl	%ecx
#ifdef PIC
	call	PIC_PLT(_C_LABEL(setcontext))
#else
	call	_C_LABEL(setcontext)
#endif
	/* NOTREACHED */
