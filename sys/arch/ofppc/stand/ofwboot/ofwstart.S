/* $NetBSD: ofwstart.S,v 1.3.62.1 2014/09/22 11:13:17 martin Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#ifdef XCOFF_GLUE
	.text
	.globl	_entry
_entry:
	.long	_start,0,0
#endif /* XCOFF_GLUE */

	.text
	.globl	_start
	/* This is the magical note section that IBM machines require
	 * to boot.  This should be a note, but gnu ld will not let us
	 * position it at the start of the file.
	 */
	# length of name
	.long	8
	# note descriptor size
	.long	24
	# note type (IEEE 1275)
	.long	0x1275
	# name of owner
	.asciz  "PowerPC"
	.balign 4

	# note descriptor
	# real mode (-1) or virtual mode (0)
	.long	-1
	# real-base
	.long	-1
	# real-size
	.long	-1
	# virt-base
	.long	-1
	# virt-size
	.long	-1
	# load-base
	.long	0x4000

	/* note for dealing with IBM LPARs */
	# length of name
	.long 24
	# note descriptor size
	.long 28
	# note type
	.long 0x12759999
	# name of owner
	.asciz "IBM,RPA-Client-Config"
	.balign 4

	# lpar affinity
	.long 0
	# minimum size in megabytes
	.long 64
	# minimum percentage size
	.long 0
	# max pft size ( 2^48 bytes )
	.long 48
	# splpar
	.long 1
	# min load ?
	.long -1
	# new mem def ?
	.long 0
	
_start:
	sync
	isync
	lis	%r1,stack+0x20000@ha
	addi	%r1,%r1,stack+0x20000@l

	mfmsr	%r8
	li	%r0,0
	mtmsr	%r0
	isync

	mtibatu	0,%r0
	mtibatu	1,%r0
	mtibatu	2,%r0
	mtibatu	3,%r0
	mtdbatu	0,%r0
	mtdbatu	1,%r0
	mtdbatu	2,%r0
	mtdbatu	3,%r0

	li	%r9,0x12 	/* BATL(0, BAT_M, BAT_PP_RW) */
	mtibatl	0,%r9
	mtdbatl	0,%r9
	li	%r9,0x1ffe	/* BATU(0, BAT_BL_256M, BAT_Vs) */
	mtibatu	0,%r9
	mtdbatu	0,%r9
	isync

	mtmsr	%r8
	isync

	/*
	 * Make sure that .bss is zeroed
	 */

	li	%r0,0
	lis	%r8,_edata@ha
	addi	%r8,%r8,_edata@l
	lis	%r9,_end@ha
	addi	%r9,%r9,_end@l

5:	cmpw	0,%r8,%r9
	bge	6f
	stw	%r0,0(%r8)
	addi	%r8,%r8,4
	b	5b

6:	b	startup
