/*	$NetBSD: start.S,v 1.1.2.1 1999/12/27 18:33:04 wrstuden Exp $	*/

/*-
 * Copyright (C) 1999 Izumi Tsutsui. All rights reserved.
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

#include <m68k/asm.h>

	.text
	.align	4

ASENTRY_NOPROFILE(start)
	jra	1f	
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0


	/* disklabel here */
	.space	0x1f0

1:
	lea	0x3e0000,%sp	|
	movl	%d7,%sp@-	| %d7: bootargs
	movl	%d6,%sp@-	| %d6: bootdev
	movl	%d5,%sp@-	| %d5: kernel
	movl	%d4,%sp@-	| %d4: physmem

	jbsr	_C_LABEL(bootxx)

	jra	_ASM_LABEL(halt)
