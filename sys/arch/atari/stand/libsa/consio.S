/*	$NetBSD: consio.S,v 1.1 2001/09/08 11:21:02 thomas Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
 *        This product includes software developed by Waldi Ravens.
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

#include <machine/asm.h>

ENTRY_NOPROFILE(getchar)	| int getchar (void);
	movml	%d2/%a2,%sp@-
	movw	#2,%sp@-	| BIOS device 2 => console (VT52)
	movw	#2,%sp@-	| BIOS opcode 2 => Bconin
	trap	#13		| => char in d0.b
	addql	#4,%sp
	andl	#0xff,%d0
	movml	%sp@+,%d2/%a2
	rts

ENTRY_NOPROFILE(putchar)	| void putchar (int c);
	movl	%sp@(4),%d0
	cmpw	#10,%d0		| linefeed?
	bnes	0f
	movq	#13,%d0		| yes, put CR and LF.
	bsrs	0f
	movq	#10,%d0
0:	movml	%d2/%a2,%sp@-
	movw	%d0,%sp@-	| character
	movw	#2,%sp@-	| BIOS device 2 => console (VT52)
	movw	#3,%sp@-	| BIOS opcode 3 => Bconout
	trap	#13		| => none
	addql	#6,%sp
	movml	%sp@+,%d2/%a2
	rts
