/*	$NetBSD: rtld_start.S,v 1.9 2002/09/12 17:18:41 mycroft Exp $	*/

/*-
 * Copyright (c) 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Paul Kranenburg, and by Charles M. Hannum.
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

	.section	".text"
	.align	4
.LLGETPC0:
	retl
	 add	%o7,%l7,%l7

	.section	".text"
	.align	4
	.global	_rtld_start
	.type	_rtld_start,@function
_rtld_start:
	mov	0, %fp			/* Erect a fence post for ourselves */
	mov	%g1, %l1		/* save ps_strings */
	sub	%sp, 8, %sp		/* Make room for return args */

	sethi	%hi(_GLOBAL_OFFSET_TABLE_-4), %l7
	call	.LLGETPC0
	 add	%l7, %lo(_GLOBAL_OFFSET_TABLE_+4), %l7
	ld	[%l7+_DYNAMIC], %o0
	ld	[%l7+_GLOBAL_OFFSET_TABLE_], %l0

	sub	%l7, %l0, %o1		/* relocbase */
	call	_rtld_relocate_nonplt_self
	 add	%o0, %o1, %o0		/* &_DYNAMIC */

	sub	%l7, %l0, %o1		/* relocbase */
	call	_rtld
	 add	%sp, 64, %o0		/* &argc - 8 */

	ld	[%sp + 64], %g3		/* arg: cleanup */
	ld	[%sp + 64 + 4], %g2	/* arg: obj */
	add	%sp, 8, %sp		/* restore stack pointer */

	jmp	%o0
	 mov	%l1, %g1		/* restore ps_strings */


	.section	".text"
	.align	4
	.global	_rtld_bind_start
	.type	_rtld_bind_start,@function
_rtld_bind_start:	# (obj, reloff)
	save	%sp, -96, %sp		/* setup standard stack frame */
	ld	[%i7 + 8], %o0		/* obj id is in second PLT slot */
	srl	%g1, 10, %o1		/* offset is in high 22 bits */
	call	_rtld_bind		/* Call _rtld_bind(obj, offset) */
	 sub	%o1, 12*4, %o1		/* first 4 `pltrel' entries missing! */

	mov	%o0, %g1		/* return value == function address */
	restore				/* get rid of our context */
	 jmp	%g1			/* and the jmpslot context, then go. */
	restore

