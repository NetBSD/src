/*	$NetBSD: start.S,v 1.2 2000/10/12 05:34:29 onoe Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
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

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.text
	.align	2
	.set	noreorder

	.globl	_start
_start:
	b	1f
	nop
	.word	0x19900106		/* version? */
	.word	0

	/* disklabel here */
	.space	0x1f0

1:	bgez	a3, 2f			/* load address */
	lui	v0, 0xa000		/* 0xa0000000 for non-APbus */
	ori	v0, 0x1000		/* 0xa0001000 for APbus */
2:	la	v1, _start		/* relocate myself */
	la	t1, _end
3:	lw	t0, 0(v0)
	addu	v1, 4
	sw	t0, -4(v1)
	bne	v1, t1, 3b
	addu	v0, 4
	bgez	a3, 4f
	nop
	lw	t0, 16(sp)
	sw	t0, _sip
4:	li	sp, 0xa0001000
	la	v0, bootxx
	j	v0
	nop
