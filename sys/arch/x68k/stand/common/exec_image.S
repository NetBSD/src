/*	$NetBSD: exec_image.S,v 1.2.2.2 2001/10/01 12:43:08 fvdl Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto.
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

#include <machine/asm.h>

#ifndef LASTADDR
#define LASTADDR	(0x00ed0008)
#endif

	.text
	.even

ENTRY_NOPROFILE(exec_image)
||	void exec_image(loaded,executed,entry,size,bootdev,boothowto)
|| 	unsigned int loaded;
||	unsigned int executed;
||	unsigned int entry;
||	int size;
||	int bootdev;
||	int boothowto;
	addl		#4,%sp			| throw away the return address
	moveml		%sp@+,%a4-%a6
	moveml		%sp@+,%d5-%d7

/* copy the trampoline to the last physical page. */
	moval		LASTADDR,%sp		| tmpstack from end of physmem
	lea		%sp@(-4096),%a3		| use last phys page as tramp

	movl		#(end_trampoline-trampoline),%sp@-
	pea		%pc@(trampoline)
	pea		%a3@
	jbsr		_C_LABEL(memcpy)	| memcpy() is still alive
	lea		%sp@(12),%sp

	jmp		%a3@			| jump to tramp

ASENTRY_NOPROFILE(trampoline)
||	%a4:	loaded
||	%a5:	executed
||	%a6:	entry
||	%d5:	size
||	%d6:	bootdev
||	%d7:	boothowto
	movl		%d5,%sp@-		| push last arg (esym)
	movel		LASTADDR,%sp@-		| second arg (physsize)
	pea		%a5@			| first arg (firstpa)

	cmpl		%a4,%a5			| if (l == x)
	beq		L1			|	copy not required

/* this may overwrite the trap vectors; disable interrupt. */
	oriw		#0x2700,%sr

L0:	movb		%a4@+,%a5@+		| copy kern to x
	subql		#1,%d5
	bne		L0

L1:
	movq		#0,%d0			| reg arg (unused)
	movq		#0,%d1			| reg arg (unused)
	movq		#0,%d2			| reg arg (unused)
	movq		#0,%d3			| reg arg (unused)
	movq		#0,%d4			| reg arg (unused)
	movq		#0,%d5			| reg arg (unused)
						| bootdev(%d6) is already set
						| boothowto(%d7) is already set
	moval		%a6,%a0			| entry
	moval		%d0,%a1			| reg arg (unused)
	moval		%d0,%a2			| reg arg (unused)
	moval		%d0,%a3			| reg arg (unused)
	moval		%d0,%a4			| reg arg (unused)
	moval		%d0,%a5			| reg arg (unused)
	moval		%d0,%a6			| reg arg (unused)
	movec		%a1,%vbr		| clear vbr

	jsr		%a0@			| here we go!

	/* NOTREACHED */
end_trampoline:
