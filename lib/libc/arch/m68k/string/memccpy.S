/*	$NetBSD: memccpy.S,v 1.2 2013/07/16 20:49:42 matt Exp $	*/

/*
 * Copyright (C) 1999 Scott Reynolds.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: memccpy.S,v 1.2 2013/07/16 20:49:42 matt Exp $")
#endif /* LIBC_SCCS and not lint */

ENTRY(memccpy)
	movl	16(%sp),%d0		| count
	jeq	Lmcbail			| nothing to do

	movl	4(%sp),%a0		| a0 = toaddr
	subql	#1,%d0			|   prepare count for DBcc loop
	movl	8(%sp),%a1		| a1 = fromaddr
	movl	12(%sp),%d1		| d1 = terminator
	jeq	Lmcloop			|   handle ASCII NUL specially

	movl	%d2,-(%sp)		| save scratch register
Lmcnzloop:
	movb	(%a1)+,%d2		| move a byte
	movb	%d2,(%a0)+
	cmpb	%d2,%d1			| found the terminator?
	dbeq	%d0,Lmcnzloop		| if not, keep transferring,
	jeq	Lmcnzdone		|   otherwise done
	clrw	%d0			| check to see if more to do
	subql	#1,%d0
	jcc	Lmcnzloop		| yes, keep going...

	movl	(%sp)+,%d2		| restore scratch register
	movql	#0,%d0			| no terminator found, return NULL
#ifdef __SVR4_ABI__
	movl	%d0,%a0			| XXX maybe use lea to avoid stall?
#endif
	rts

Lmcloop:
	movb	(%a1)+,(%a0)+		| move a byte; was it NUL?
	dbeq	%d0,Lmcloop		| if not, keep transferring,
	jeq	Lmcdone			|   otherwise done
	clrw	%d0			| check to see if more to do
	subql	#1,%d0
	jcc	Lmcloop			| yes, keep going...
					| Note: %d0 is now -1!
	movql	#0,%d0			| no terminator found, return NULL
Lmcbail:
#ifdef __SVR4_ABI__
	movl	%d0,%a0			| XXX maybe use lea to avoid stall?
#endif
	rts

Lmcnzdone:
	movl	(%sp)+,%d2		| restore scratch register
Lmcdone:
	movl	%a0,%d0
	rts
