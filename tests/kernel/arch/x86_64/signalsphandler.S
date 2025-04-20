/*	$NetBSD: signalsphandler.S,v 1.1 2025/04/20 22:31:01 riastradh Exp $	*/

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

RCSID("$NetBSD: signalsphandler.S,v 1.1 2025/04/20 22:31:01 riastradh Exp $")

/*
 * signalsphandler(signo@rdi)
 *
 *	Signal handler.  Store the stack pointer on entry at the global
 *	variable signalsp and return.
 */
ENTRY(signalsphandler)
	/*
	 * `The end of the input argument area shall be aligned on a
	 *  [16-byte] boundary.  In other words, the value of (%rsp + 8)
	 *  is always a multiple of 16 when control is transferred to
	 *  the function entry point.'
	 *
	 * To make it convenient for t_signal_and_sp.c, we subtract 8
	 * from %rsp in order to get something congruent to zero modulo
	 * the stack alignemnt.
	 */
	movq	%rsp,_C_LABEL(signalsp)(%rip)
	addq	$-8,_C_LABEL(signalsp)(%rip)
	ret
END(signalsphandler)
