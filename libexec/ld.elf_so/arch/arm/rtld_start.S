/*	$NetBSD: rtld_start.S,v 1.1 2001/07/15 03:08:33 matt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
	.align	0
	.globl	_rtld_start
	.type	_rtld_start,%function
_rtld_start:
	sub	sp, sp, #8		/* make room for obj_main & exit proc */
	mov	r4, r0			/* save ps_strings */

	mov	r0, sp			/* arg#1 = sp */
	bl	_rtld			/* call the shared loader */
	mov	r3, r0			/* save entry point */

	ldr	r2, [sp, #4]		/* r2 = cleanup */
	ldr	r1, [sp]		/* r1 = obj_main */
	add	sp, sp, #8		/* restore stack */
	mov	r0, r4			/* restore ps_strings */
	mov	pc, r3			/* jump to the entry point */

	.align	0
	.globl	_rtld_bind_start
	.type	_rtld_bind_start,%function
/*
 *	stack[0] = RA
 *	ip = &GOT[n+3]
 *	lr = &GOT[2]
 */
_rtld_bind_start:
	stmdb	sp!,{r0-r3,sl,fp}

	sub	r1, ip, lr		/* r1 = 4 * (n + 1) */
	sub	r1, r1, #4		/* r1 = 4 * n */
	add	r1, r1, r1		/* r1 = 8 * n */

	ldr	r0, [lr, #-4]		/* get obj ptr from GOT[1] */

	bl	_rtld_bind		/* Call the binder */

	str	r0, [ip]		/* save address in GOT */
	mov	ip, r0			/* save new address */

	ldmia	sp!,{r0-r3,sl,fp,lr}	/* restore the stack */
	mov	pc, ip			/* jump to the new address */

