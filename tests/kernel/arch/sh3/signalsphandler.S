/*	$NetBSD: signalsphandler.S,v 1.2 2025/04/26 23:49:55 uwe Exp $	*/

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

#include "asm.h"
RCSID("$NetBSD: signalsphandler.S,v 1.2 2025/04/26 23:49:55 uwe Exp $")


/*
 * void signalsphandler(int signo)
 *
 *	Signal handler.  Store the stack pointer on entry at the global
 *	variable signalsp and return.
 */
ENTRY(signalsphandler)
	mov	sp, r1
#ifdef __PIC__
	// PR kern/59327: don't touch stack as SP may be misaligned
	// and as SuperH is a strict alignment architecture, we will
	// get SIGBUS if we try to save registers on the stack
	mov	r12, r2
#endif
	PIC_PROLOGUE_NOSAVE(.L_GOT)

	MOVL_VAR(.L_signalsp, r0)
	mov.l	r1, @r0

#ifdef __PIC__
	mov	r2, r12
#endif
	rts
	 nop

	.p2align 2
.L_GOT:		PIC_GOT_DATUM
.L_signalsp:	VAR_DATUM(signalsp)
	SET_ENTRY_SIZE(signalsphandler)
