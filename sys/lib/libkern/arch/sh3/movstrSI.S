/*	$NetBSD: movstrSI.S,v 1.1 2000/06/09 04:42:24 msaitoh Exp $	*/

/*
 * Copyright (c) 2000 SHIMIZU Ryo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

	.globl	___movstrSI64, ___movstrSI60, ___movstrSI56, ___movstrSI52
	.globl	___movstrSI48, ___movstrSI44, ___movstrSI40, ___movstrSI36
	.globl	___movstrSI32, ___movstrSI28, ___movstrSI24, ___movstrSI20
	.globl	___movstrSI16, ___movstrSI12, ___movstrSI8,  ___movstrSI4

	.text
	.align	2

___movstrSI64:
	mov.l	@(60,r5),r0
	mov.l	r0,@(60,r4)
___movstrSI60:
	mov.l	@(56,r5),r0
	mov.l	r0,@(56,r4)
___movstrSI56:
	mov.l	@(52,r5),r0
	mov.l	r0,@(52,r4)
___movstrSI52:
	mov.l	@(48,r5),r0
	mov.l	r0,@(48,r4)
___movstrSI48:
	mov.l	@(44,r5),r0
	mov.l	r0,@(44,r4)
___movstrSI44:
	mov.l	@(40,r5),r0
	mov.l	r0,@(40,r4)
___movstrSI40:
	mov.l	@(36,r5),r0
	mov.l	r0,@(36,r4)
___movstrSI36:
	mov.l	@(32,r5),r0
	mov.l	r0,@(32,r4)
___movstrSI32:
	mov.l	@(28,r5),r0
	mov.l	r0,@(28,r4)
___movstrSI28:
	mov.l	@(24,r5),r0
	mov.l	r0,@(24,r4)
___movstrSI24:
	mov.l	@(20,r5),r0
	mov.l	r0,@(20,r4)
___movstrSI20:
	mov.l	@(16,r5),r0
	mov.l	r0,@(16,r4)
___movstrSI16:
	mov.l	@(12,r5),r0
	mov.l	r0,@(12,r4)
___movstrSI12:
	mov.l	@(8,r5),r0
	mov.l	r0,@(8,r4)
___movstrSI8:
	mov.l	@(4,r5),r0
	mov.l	r0,@(4,r4)
___movstrSI4:
	mov.l	@r5,r0
	rts
	mov.l	r0,@r4


