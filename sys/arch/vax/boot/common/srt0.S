/*	$NetBSD: srt0.S,v 1.5.2.1 2018/03/24 18:08:06 snj Exp $ */
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

/*
 * Auto-moving startup code for standalone programs. Can be loaded
 * (almost) anywhere in memory but moves itself to the position
 * it is linked for. Must be started at first position, recommended
 * is phys addr 0 (boot loads programs at 0, but starts them at the
 * position set in a.out header.
 */

	.globl	nisse		# pass -e nisse to ld gives OK start addr
	.set	nisse,0

ALTENTRY(start)
	nop;nop;
	movl	$_C_LABEL(start), %sp	# Probably safe place for stack
	pushr	$0x1fff		# save for later usage

	subl3	$_C_LABEL(start), $_C_LABEL(edata), %r0
	movab	_C_LABEL(start), %r1 # get where we are
	movl	$_C_LABEL(start), %r3 # get where we want to be
	cmpl	%r1,%r3		# are we where we want to be?
	beql	relocated	# already relocated, skip copy
1:	movb	(%r1)+,(%r3)+	# copy
	sobgtr	%r0,1b

	subl3	$_C_LABEL(edata), $_C_LABEL(end), %r2
	movc5	$0,(%r3),$0,%r2,(%r3) # Zero bss

	movpsl	-(%sp)
	pushl	$relocated
	rei
relocated:	                # now relocation is done !!!
	movl	%sp,_C_LABEL(bootregs)	# *bootregs
	calls	$0, _C_LABEL(Xmain)	# Were here!
	halt			# no return

ENTRY(machdep_start, 0)
	calls	$0,_C_LABEL(niclose)	# Evil hack to shutdown DEBNA.
	mtpr	$0x1f,$0x12	# Block all interrupts
	mtpr	$0,$0x18	# stop real time interrupt clock
	movl	4(%ap), %r6
	movl	20(%ap), %r9	# end of symbol table
	pushl	8(%ap)		# number of symbols
	pushl	16(%ap)		# start of symbols
	movab	_C_LABEL(bootrpb),%r10	# get RPB address
	pushl	%r10		# argument for new boot
	ashl	$9,76(%r10),%r8	# memory size (COMPAT)
	movl	$3,%r11		# ask boot (COMPAT)
	clrl	%r10		# no boot dev (COMPAT)

	calls	$3,(%r6)
	halt
