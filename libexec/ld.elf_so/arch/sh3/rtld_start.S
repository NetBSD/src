/*	$NetBSD: rtld_start.S,v 1.2 2002/02/04 18:18:51 thorpej Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Marcus Comstedt.
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

	.text
	.align	2
	.globl	.rtld_start
	.type	.rtld_start,@function

.rtld_start:

	/* relocate GOT, so that we can use global function calls in rtld.c */

	mova	1f,r0
	mov.l	@r0,r1
	add	r1,r0		/* r0 = _GLOBAL_OFFSET_TABLE_ */

	mov.l	2f,r1
	add	r0,r1		/* r1 = GOT end */
	mov.l	3f,r2
	mov.l	@(r0,r2),r3	/* r3 = where the linker thinks GOT end is */
	mov	r1,r2
	sub	r3,r2		/* r2 = displacement */
4:
	mov.l	@r0+,r3		/* add displacement to all GOT entries */
	cmp/hs	r1,r0
	add	#-4,r0
	add	r2,r3
	mov.l	r3,@r0
	bf/s	4b
	add	#4,r0

	bra	4f		/* all done */
	nop	
		
	.align	2
1:	.long	_GLOBAL_OFFSET_TABLE_
2:	.long	_DYNAMIC@GOTOFF
3:	.long	_DYNAMIC@GOT

			
4:
	add	#-8,r15		/* Make room for return args */
	mov	r15,r4
	mov.l	1f,r0
	add	#(1f-3f),r0
	bsrf	r0		/* _rtld(sp) */
	mov.l	r9,@-r15	/* save ps_strings */
3:
	mov.l	@r15+,r9	/* restore ps_strings */
	
	mov.l	@r15+,r7	/* arg: cleanup */
	mov.l	@r15+,r8	/* arg: obj */

	mov.l	@r15,r4		/* restore argc */

	mov	r15,r5		/* restore argv */
	add	#4,r5

	mov	r4,r6		/* restore envp */
	shll2	r6
	add	r15,r6
	jmp	@r0
	add	#8,r6
	.align	2
1:	.long	_rtld-.
	.size	.rtld_start,.-.rtld_start
	
	.align	2
	.globl	_rtld_bind_start
	.type	_rtld_bind_start,@function
_rtld_bind_start:		/* r0 = flag, r1 = reloff, r2 = obj */
	mov.l	r3,@-r15	/* save registers */
	mov.l	r4,@-r15
	mov.l	r5,@-r15
	mov.l	r6,@-r15
	mov.l	r7,@-r15
	sts.l	mach,@-r15
	sts.l	macl,@-r15
	sts.l	pr,@-r15

	mov.l	2f,r0
	add	#(2f-4f),r0
	mov	r2,r4		/* copy of obj */
	bsrf	r0		/* call the binder */
	mov	r1,r5		/* copy of relobj */
4:	
	lds.l	@r15+,pr	/* restore registers */
	lds.l	@r15+,macl
	lds.l	@r15+,mach
	mov.l	@r15+,r7
	mov.l	@r15+,r6
	mov.l	@r15+,r5
	mov.l	@r15+,r4
	jmp	@r0
	mov.l	@r15+,r3
	.align	2
2:	.long	_rtld_bind-.
	.size	_rtld_bind_start,.-_rtld_bind_start

	.end
