/*	$NetBSD: srt0.s,v 1.4 2000/05/24 19:53:11 ragge Exp $ */
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

#include "../include/asm.h"

#define JSBENTRY(x)     .globl x ; .align 2 ; x :
/*
 * Auto-moving startup code for standalone programs. Can be loaded
 * (almost) anywhere in memory but moves itself to the position
 * it is linked for. Must be started at first position, recommended
 * is phys addr 0 (boot loads programs at 0, but starts them at the
 * position set in a.out header.
 */

nisse:	.set	nisse,0		# pass -e nisse to ld gives OK start addr
	.globl	nisse

JSBENTRY(_start)
	nop;nop;
	movl	$_start, sp	# Probably safe place for stack

	subl3	$_start, $_edata, r0
	movab	_start, r1
	movl	$_start, r3
	movc3	r0,(r1),(r3)	# Kopiera text + data
	subl3	$_edata, $_end, r2
	movc5	$0,(r3),$0,r2,(r3) # Nolla bss också.

	jsb	1f
1:	movl    $relocated, (sp)   # return-address on top of stack
	rsb                        # can be replaced with new address
relocated:	                   # now relocation is done !!!
	pushl	r11		# RPB is copied here.

	calls	$1, _Xmain	# Were here!
	halt			# no return

ENTRY(machdep_start, 0)
	mtpr	$0x1f,$0x12	# Block all interrupts
	mtpr	$0,$0x18	# stop real time interrupt clock
	movl	4(ap), r6
	movl	20(ap), r9	# end of symbol table
	movab	_bootrpb,r10	# get RPB address
	pushl	r10		# argument for new boot
	ashl	$9,76(r10),r8	# memory size (COMPAT)
	movl	$3,r11		# ask boot (COMPAT)
	clrl	r10		# no boot dev (COMPAT)

	calls	$1,(r6)
	halt
