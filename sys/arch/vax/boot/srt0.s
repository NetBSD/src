/*	$NetBSD: srt0.s,v 1.2 1995/03/29 21:24:14 ragge Exp $ */
/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
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

 /* All bugs are subject to removal without further notice */


/*
 * Auto-moving startup code for standalone programs. Can be loaded
 * (almost) anywhere in memory but moves itself to the position
 * it is linked for. Must be started at first position, recommended
 * is phys addr 0 (boot loads programs at 0, but starts them at the
 * position set in a.out header.
 */

nisse:	.set	nisse,0		# pass -e nisse to ld gives OK start addr
	.globl	nisse

start:	.globl	start
	nop;nop;		# If we get called by calls, or something
	movl	$start, sp	# Probably safe place for stack
	jsb	1f
1:	movl	(sp)+,r0
	bicl2	$0x1ff,r0

	subl3	sp,$_end,r4	# Get size to copy, size < 64K
	movc3	r4,(r0),(sp)	# Copy all

	pushl	$cont		# new run address
	rsb			# Doit

cont:	movl    $start, sp
	calls	$0,_main	# Were here!
	halt			# no return

        .globl _hoppabort
_hoppabort: .word 0x0
        movl    4(ap),r6
        movl    8(ap),r11
        movl    0xc(ap),r10
        calls   $0,(r6)

