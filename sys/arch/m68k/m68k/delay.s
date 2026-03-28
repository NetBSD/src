/*	$NetBSD: delay.s,v 1.1 2026/03/28 22:19:33 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 */

#include <machine/asm.h>

#include "assym.h"

	.file	"delay.s"
	.text

/*
 * delay(unsigned int N)
 *
 * Delay for at least N microseconds.
 * This routine depends on the variable:  delay_divisor
 * which should be set based on the CPU clock rate
 * (either a known pre-computed value for a given machine
 * type or calibrated against a known clock source).
 */
ENTRY_NOPROFILE(delay)
	movl	4(%sp),%d0		| %d0 = arg = usecs
#ifdef DIAGNOSTIC
	cmpl	#DELAY_MAXVAL,%d0	| exceeds max value?
	bgt	3f			| yes, go panic.
#endif
	moveq	#DELAY_MAGSHIFT,%d1	| magnification factor
	lsll	%d1,%d0

	| %d1 = delay_divisor;
	movl	_C_LABEL(delay_divisor),%d1

	jra	2f			/* Jump into the loop! */

	/*
	 * Align the branch target of the loop to a half-line (8-byte)
	 * boundary to minimize cache effects.  This guarantees both
	 * that there will be no prefetch stalls due to cache line burst
	 * operations and that the loop will run from a single cache
	 * half-line.
	 */
	.align	8
2:	subl	%d1,%d0
	bgt	2b
	rts
#ifdef DIAGNOSTIC
3:
	pea	Ldelaypanicmsg
	jbsr	_C_LABEL(panic)
	rts
Ldelaypanicmsg:
	.asciz	"delay exceeds max value"
	.even
#endif
