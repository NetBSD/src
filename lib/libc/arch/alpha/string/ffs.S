/*	$NetBSD: ffs.S,v 1.1 1996/07/15 03:13:19 cgd Exp $	*/

/*
 * Copyright (c) 1995 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "DEFS.h"

LEAF(ffs, 1)
	addl	a0, 0, a0
	beq	a0, Lallzero

	ldil	v0, 1

	and	a0, 0xff, t0
	bne	t0, Ldo8

	/*
	 * If lower 16 bits empty, add 16 to result and use upper 16.
	 */
	zapnot	a0, 0x03, t1
	bne	t1, Ldo16
	sra	a0, 16, a0
	addl	v0, 16, v0

Ldo16:
	/*
	 * If lower 8 bits empty, add 8 to result and use upper 8.
	 */
	and	a0, 0xff, t2
	bne	t2, Ldo8
	sra	a0, 8, a0
	addl	v0, 8, v0

Ldo8:
	and	a0, 0x0f, t3		/* lower 4 of 8 empty? */
	and	a0, 0x33, t4		/* lower 2 of each 4 empty? */
	and	a0, 0x55, t5		/* lower 1 of each 2 empty? */

	/* If lower 4 bits empty, add 4 to result. */
	bne	t3, Ldo4
	addl	v0, 4, v0

Ldo4:	/* If lower 2 bits of each 4 empty, add 2 to result. */
	bne	t4, Ldo2
	addl	v0, 2, v0

Ldo2:	/* If lower bit of each 2 empty, add 1 to result. */
	bne	t5, Ldone
	addl	v0, 1, v0

Ldone:
	RET

Lallzero:
	bis	zero, zero, v0
	RET
END(ffs)
