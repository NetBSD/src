/*	$NetBSD: modsi3.S,v 1.1.2.2 2002/04/01 07:48:05 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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

#if defined(LIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: modsi3.S,v 1.1.2.2 2002/04/01 07:48:05 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

ENTRY(__modsi3)
| NB: this requires that __udivsi3 preserve %a0 and return
| the modulus in %d1:
	movel	%sp@+, %a0	| pop return address
	movel	%sp@(4), %d1	| load the divisor
	bpl	1f
	negl	%sp@(4)		| store abs(divisor)
1:	movel	%sp@, %d0	| load the dividend
	pea	%pc@(Lret)	| push our return address
	bpl	2f
	negl	%sp@(4)		| store abs(dividend)
	subql	#2, %sp@	| adjust return address
2:	jmp	_C_LABEL(__udivsi3)
	negl	%d1		| negate modulus
Lret:	movl	%d1, %d0	| move modulus into %d0
	jmp	%a0@
