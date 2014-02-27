/* $NetBSD: crti.S,v 1.2 2014/02/27 16:47:48 matt Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <powerpc/asm.h>

RCSID("$NetBSD: crti.S,v 1.2 2014/02/27 16:47:48 matt Exp $")

#include "sysident.S"

	.globl	_init
	.type	_init,@function
	.section ".init", "ax", @progbits
	.align	P2SZREG
#ifdef _LP64
	.globl	._init
	.pushsection ".opd", "aw"
	.align	3
_init:	.quad	._init,.TOC.@tocbase,0
	.size	_init, (3*SZREG)
	.popsection
._init:
#else
_init:
#endif
	mflr	%r0
#ifdef _LP64
	stlongu	%r1, -(6*SZREG)(%r1)
	stlong	%r0, (2*SZREG)(%r1)
#else
	stw	%r0, (1*SZREG)(%r1)
	stwu	%r1, -(4*SZREG)(%r1)
#endif

	.globl _fini
	.type	_fini,@function
	.section ".fini", "ax", @progbits
	.align	P2SZREG
#ifdef _LP64
	.globl	._fini
	.pushsection ".opd", "aw"
	.align	3
_fini:	.quad	._fini,.TOC.@tocbase,0
	.size	_fini, (3*SZREG)
	.popsection
._fini:
#else
_fini:
#endif
	mflr	%r0
#ifdef _LP64
	stlongu	%r1, -(6*SZREG)(%r1)
	stlong	%r0, (2*SZREG)(%r1)
#else
	stw	%r0, (1*SZREG)(%r1)
	stwu	%r1, -(4*SZREG)(%r1)
#endif
