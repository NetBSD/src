/*	$NetBSD: asm.h,v 1.1 2002/07/05 13:31:56 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _SH5_ASM_H
#define	_SH5_ASM_H

#define	PIC_PROLOGUE
#define	PIC_EPILOGUE
#define	PIC_PLT(x)	x
#define	PIC_GOT(x)	x
#define	PIC_GOTOFF(x)	x

#define _C_LABEL(x)	_/**/x
#define	_ASM_LABEL(x)	_/**/x

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# define _ALIGN_TEXT .align 3
#endif

#ifndef _IPL64
#define	_ENTRY(x)							\
	.section .text..SHmedia32,"ax"					;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:

#define	_ASENTRY(x)							\
	.section .text..SHmedia32,"ax"					;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:
#else
#define	_ENTRY(x)							\
	.text								;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:

#define	_ASENTRY(x)							\
	.text								;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:
#endif

#define	_PROF_PROLOGUE

#define	ENTRY(y)	_ENTRY(_C_LABEL(y))				;\
	_PROF_PROLOGUE
#define	ASENTRY(y)	_ASENTRY(_ASM_LABEL(y))				;\
	_PROF_PROLOGUE

#define	ENTRY_NOPROFILE(y)	_ENTRY(_C_LABEL(y))
#define	ASENTRY_NOPROFILE(y)	_ENTRY(_ASM_LABEL(y))

#define	ALTENTRY(name)	.globl _C_LABEL(name)				;\
	.type _C_LABEL(name),@function					;\
	_C_LABEL(name):

/*
 * Global variables
 */
#define	GLOBAL(x)	.globl	_C_LABEL(x) ;\
		_C_LABEL(x):

#define	ASGLOBAL(x)	.globl	_ASM_LABEL(x) ;\
		_ASM_LABEL(x):

/*
 * ... and local variables.
 */
#define	LOCAL(x)	_C_LABEL(x):
#define	ASLOCAL(x)	_ASM_LABEL(x):

/*
 * Items in the BSS segment.
 */
#define	BSS(name, size)		.comm	_C_LABEL(name),size
#define	ASBSS(name, size)	.comm	_ASM_LABEL(name,size

#define	ASMSTR		.asciz

#define	RCSID(x)	.text; .asciz x

#define	WEAK_ALIAS(alias,sym)						\
	.weak _C_LABEL(alias);						\
	_C_LABEL(alias) = _C_LABEL(sym)

#ifdef __STDC__
#define	__STRING(x)			#x
#define	WARN_REFERENCES(sym, msg)
#else
#define	__STRING(x)			"x"
#define	WARN_REFERENCES(sym, msg)
#endif /* __STDC__ */

/*
 * Helper macroes for dealing with ILP32/LP64 issues in assembly code
 */

/*
 * Load a 32-bit constant `v' into register `reg', with sign-extension
 * to 64-bits.
 *
 * Use the first if the constant has bit-31 clear, otherwise use the 2nd.
 */
#define	LDC32(v, reg)		\
	movi	(((v) >> 16) & 65535), reg	;\
	shori	((v) & 65535), reg

#define	LDSC32(v, reg)		\
	movi	(((v) >> 16) & 65535) - 65536, reg	;\
	shori	((v) & 65535), reg

/*
 * Ditto, without sign-extension
 */
#define	LDUC32(v, reg)		\
	movi	0, reg				;\
	shori	((v) >> 16) & 65535, reg	;\
	shori	(v) & 65535, reg

/*
 * Load a 64-bit constance `v' into register `reg'.
 */
#define	LDC64(v, reg)		\
	movi	(((v) >> 48) & 65535), reg	;\
	shori	(((v) >> 32) & 65535), reg	;\
	shori	(((v) >> 16) & 65535), reg	;\
	shori	((v) & 65535), reg

#define	LDSC64(v, reg)		\
	movi	(((v) >> 48) & 65535) - 65536, reg	;\
	shori	(((v) >> 32) & 65535), reg	;\
	shori	(((v) >> 16) & 65535), reg	;\
	shori	((v) & 65535), reg

#ifndef _LP64
/*
 * Load the effective address of a data object into a register
 */
#define	LEA(sym, reg)		LDC32(datalabel (sym), reg)

/*
 * Load the effective address of a function into a register.
 */
#define	LEAF(sym, reg)		LDC32(sym, reg)

/*
 * Instruction to load a pointer
 */
#define	LDPTR			ld.l

/*
 * Instruction to store a pointer
 */
#define	STPTR			st.l

/*
 * Set up/Clear down a C-like stack frame on entry/exit to an assembly function
 */
#define	LINK_FRAME(sz)		\
	addi.l	r15, -8, r15	;\
	st.l	r15, 4, r18	;\
	st.l	r15, 0, r14	;\
	addi.l	r15, -(sz), r15	;\
	add.l	r15, r63, r14

#define	UNLINK_FRAME(sz)	\
	addi.l	r14, sz, r14	;\
	add.l	r14, r63, r15	;\
	ld.l	r15, 0, r14	;\
	ld.l	r15, 4, r18	;\
	addi.l	r15, 8, r15

#else

/*
 * 64-bit versions of the above macroes....
 */

#define	LEA(sym, reg)		LDC64(datalabel (sym), reg)
#define	LEAF(sym, reg)		LDC64(sym, reg)
#define	LDPTR			ld.q
#define	STPTR			st.q
#define	LINK_FRAME(sz)		\
	addi	r15, -16, r15	;\
	st.q	r15, 8, r18	;\
	st.q	r15, 0, r14	;\
	addi	r15, -(sz), r15	;\
	add	r15, r63, r14

#define	UNLINK_FRAME(sz)	\
	addi	r14, sz, r14	;\
	add	r14, r63, r15	;\
	ld.q	r15, 0, r14	;\
	ld.q	r15, 8, r18	;\
	addi	r15, 16, r15
#endif


/*
 * Used in assembly code to convert a pteg hash into the offset from
 * the base of the pmap_pteg_table.
 *
 * When SH5_NEFF_BITS = 32, sizeof(pteg_t) == 96
 * When SH5_NEFF_BITS > 32, sizeof(pteg_t) == 192
 *
 * XXX: should really be in <sh5/pte.h>
 */
#if SH5_NEFF_BITS == 32
#define	HASH_TO_PTEG_IDX(r,t)	\
	shlli	r, 5, t		/* t = r * 32 */	;\
	shlli	r, 6, r		/* r = r * 64 */	;\
	add	r, t, r		/* r = r + t  */
#define	LDPTE	ld.l
#define	STPTE	st.l
#else
#define	HASH_TO_PTEG_IDX(r,t)	\
	shlli	r, 6, t		/* t = r * 64  */	;\
	shlli	r, 7, r		/* r = r * 128 */	;\
	add	r, t, r		/* r = r + t   */
#define	LDPTE	ld.q
#define	STPTE	st.q
#endif

#endif /* _SH5_ASM_H */
