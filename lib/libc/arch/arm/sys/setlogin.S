/*	$NetBSD: setlogin.S,v 1.1.2.1 2001/10/08 20:17:11 nathanw Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)setlogin.s	5.2 (Berkeley) 4/12/91
 */

#include <machine/asm.h>
#include "SYS.h"

	.globl	_C_LABEL(__logname_valid)	/* in getlogin() */

SYSCALL(setlogin)
#ifdef PIC
	/* Setup the GOT */
	ldr	r0, got
	add	r0, pc, r0
.L1:
	ldr	r1, Llogname
	ldr	r1, [r0, r1]
#else
	ldr	r1, Llogname
#endif
	mov	r0, #0x00000000
	str	r0, [r1]
#ifdef __APCS_26__
	movs	r15, r14
#else
	mov	r15, r14
#endif

#ifdef PIC
	.align	0
got:
	.word	_C_LABEL(_GLOBAL_OFFSET_TABLE_) + (. - (.L1+4))
#endif

Llogname:
	.word	PIC_SYM(_C_LABEL(__logname_valid), GOT)
