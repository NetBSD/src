/*	$NetBSD: str.S,v 1.5.122.1 2017/12/03 11:36:47 jdolecek Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
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

/*
 * Small versions of the most common functions not using any
 * emulated instructions.
 */

#include "asm.h"

/*
 * atoi() used in devopen.
 */
ENTRY(atoi, 0)
	movl	4(%ap),%r1
	clrl	%r0

2:	movzbl	(%r1)+,%r2
	cmpb	%r2,$48
	blss	1f
	cmpb	%r2,$57
	bgtr	1f
	subl2	$48,%r2
	mull2	$10,%r0
	addl2	%r2,%r0
	brb	2b
1:	ret

/*
 * strchr() small and easy.
 */
ENTRY(strchr, 0)
	movq	4(%ap),%r0
1:	cmpb	(%r0), 8(%ap)
	beql	2f
	tstb	(%r0)+
	bneq	1b
	clrl	%r0
2:	ret

/*
 * cmpc3 is emulated on MVII.
 */
ENTRY(memcmp, 0)
	brb	10f
ENTRY(bcmp, 0)
10:	movl	4(%ap), %r2
	movl	8(%ap), %r1
	movl	12(%ap), %r0
2:	cmpb	(%r2)+, (%r1)+
	bneq	1f
	sobgtr	%r0, 2b
3:	ret
1:	bgtru	5f
	movl	$-1, %r0
	brb	3b
5:	movl    $1, %r0
	ret

/*
 * movc can't do length in excess of 64K, so we shall not use them.
 */
ENTRY(bzero,0)
	movl	4(%ap),%r0
	movl	8(%ap),%r1
1:	clrb	(%r0)+
	sobgtr	%r1,1b
	ret

/*
 * memcpy and bcopy are the same, except for argument order. Silly stuff.
 */
ENTRY(memcpy,0)
	movl	8(%ap),%r0
	movl	4(%ap),%r1
	brb	1f
ENTRY(bcopy,0)
	movl	4(%ap),%r0
	movl	8(%ap),%r1
1:	movl	12(%ap),%r2
	cmpl	%r0,%r1
	bgtru	3f
	addl2	%r2,%r0
	addl2	%r2,%r1
2:	movb	-(%r0),-(%r1)
	sobgtr	%r2,2b
	ret
3:	movb	(%r0)+,(%r1)+
	sobgtr	%r2,3b
	ret

ENTRY(memset,0)
	movl	4(%ap),%r0
	movl	12(%ap),%r1
1:	movb	8(%ap),(%r0)+
	sobgtr	%r1,1b
	ret

ENTRY(strlen, 0)
	movl	4(%ap), %r0
1:	tstb	(%r0)+
	bneq	1b
	decl	%r0
	subl2	4(%ap), %r0
	ret

ENTRY(strncmp, 0)
	movl	12(%ap), %r3
	bneq	5f
	brb	4f

ENTRY(strcmp, 0)
	movl	$250, %r3	# max string len to compare
5:	movl	4(%ap), %r2
	movl	8(%ap), %r1
	movl	$1, %r0

2:	cmpb	(%r2),(%r1)+
	bneq	1f		# something differ
	tstb	(%r2)+
	beql	4f		# continue, strings unequal
	decl	%r3		# max string len encountered?
	bneq	2b

4:	clrl	%r0		# We are done, strings equal.
	ret

1:	bgtru	3f
	mnegl	%r0, %r0
3:	ret

ENTRY(strncpy, 0)
	movl	4(%ap), %r1
	movl	8(%ap), %r2
	movl	12(%ap), %r3
	bleq	2f

1:	movb	(%r2)+, (%r1)+
	beql	2f
	sobgtr	%r3,1b
	ret
2:	decl	%r1
3:	clrb	(%r1)+
	sobgtr	%r3,3b
	ret

ENTRY(strcat, 0)
	pushl	4(%ap)
	calls	$1,_C_LABEL(strlen)
	addl2	4(%ap),%r0
	movl	8(%ap),%r1
1:	movb	(%r1)+,(%r0)+
	bneq	1b
	ret

ENTRY(setjmp, 0)
	movl	4(%ap), %r0
	movl	8(%fp), (%r0)
	movl	12(%fp), 4(%r0)
	movl	16(%fp), 8(%r0)
	addl3	%fp,$28,12(%r0)
	clrl	%r0
	ret

ENTRY(longjmp, 0)
	movl	4(%ap), %r1
	movl	8(%ap), %r0
	movl	(%r1), %ap
	movl	4(%r1), %fp
	movl	12(%r1), %sp
	jmp	*8(%r1)

