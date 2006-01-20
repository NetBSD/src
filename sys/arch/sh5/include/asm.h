/*	$NetBSD: asm.h,v 1.12 2006/01/20 22:02:40 christos Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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

#ifdef __NO_LEADING_UNDERSCORES__
# define _C_LABEL(x)	x
#else
# if __STDC__
#  define _C_LABEL(x)	_ ## x
# else
#  define _C_LABEL(x)	_/**/x
# endif
#endif

#define _ASM_LABEL(x)	x

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# define _ALIGN_TEXT .align 5
#endif

/* The text sections for 32 and 64 bit code are different ... */
#ifndef _LP64
#define	_TEXT_SECTION	.section .text..SHmedia32,"ax"
#else
#define	_TEXT_SECTION	.text
#endif

/*
 * Building block for C-callable entry points
 */
#define	_ENTRY(x)							\
	_TEXT_SECTION							;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:

/*
 * Building block for Assembly-callable entry points
 */
#define	_ASENTRY(x)							\
	_TEXT_SECTION							;\
	_ALIGN_TEXT							;\
	.globl x							;\
	.type x,@function						;\
	x:

/*
 * When profiling, the following instructions are included in
 * the function prologue...
 */
#ifdef GPROF
#ifdef PIC
#define	_PROF_PROLOGUE	pta/l	__mcount@PLT, tr0; blink tr0, r0
#else
#define	_PROF_PROLOGUE	pta/l	__mcount, tr0; blink tr0, r0
#endif
#else
#define	_PROF_PROLOGUE
#endif

/*
 * C-callable entry point, with profiling enabled.
 */
#define	ENTRY(y)		_ENTRY(_C_LABEL(y))			;\
				_PROF_PROLOGUE

/*
 * C-callable entry point, without profiling.
 */
#define	ENTRY_NOPROFILE(y)	_ENTRY(_C_LABEL(y))

/*
 * Alternative name for a C-callable entry point.
 */
#define	ALTENTRY(y)		_ENTRY(_C_LABEL(y))

/*
 * Profiled entry point, callable only from assembly code
 */
#define	ASENTRY(y)		_ASENTRY(_ASM_LABEL(y))			;\
				_PROF_PROLOGUE

/*
 * Non-profiled entry point, callable only from assembly code
 */
#define	ASENTRY_NOPROFILE(y)	_ASENTRY(_ASM_LABEL(y))


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
/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl _C_LABEL(alias);						\
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
 * Ditto, but used for loading a relative address
 */
#define	LEAR(sym, label, reg)						\
	movi	(((datalabel sym - (label - .)) >> 16) & 65535), reg;	\
	shori	((datalabel sym - (label - .)) & 65535), reg

/*
 * Load the effective address of a function into a register.
 */
#define	LEAF(sym, reg)		LDC32(sym, reg)

/*
 * Ditto, but used for loading a relative address
 */
#define	LEARF(fn, label, reg)					\
	movi	(((fn - (label - .)) >> 16) & 65535), reg;	\
	shori	((fn - (label - .)) & 65535), reg

/*
 * Instruction to load a pointer
 */
#define	LDPTR			ld.l
#define	LDXPTR			ldx.l

/*
 * Instruction to store a pointer
 */
#define	STPTR			st.l
#define	STXPTR			stx.l

/*
 * Set up/Clear down a C-like stack frame on entry/exit to an assembly function
 */
#define	LINK_FRAME(sz)			\
	addi.l	r15, -(8 + (sz)), r15	;\
	st.l	r15, (sz), r14		;\
	st.l	r15, (sz) + 4, r18	;\
	add.l	r15, r63, r14

#define	UNLINK_FRAME(sz)		\
	add.l	r14, r63, r15		;\
	ld.l	r15, (sz), r14		;\
	ld.l	r15, (sz) + 4, r18	;\
	addi.l	r15, (sz) + 8, r15

/*
 * For stack frames sized between 512 and 32767 bytes, inclusive.
 */
#define	LINK_BIG_FRAME(sz)		\
	addi.l	r15, -8, r15		;\
	movi	sz, r0			;\
	st.l	r15, 0, r14		;\
	st.l	r15, 4, r18		;\
	sub.l	r15, r0, r15		;\
	add.l	r15, r63, r14

#define	UNLINK_BIG_FRAME(sz)		\
	movi	sz, r0			;\
	add.l	r14, r0, r15		;\
	ld.l	r15, 0, r14		;\
	ld.l	r15, 4, r18		;\
	addi.l	r15, 8, r15

#else

/*
 * 64-bit versions of the above macroes....
 */

#define	LEA(sym, reg)		LDC64(datalabel (sym), reg)
#define	LEAR(sym, label, reg)						\
	movi	(((datalabel sym - (label - .)) >> 48) & 65535), reg;	\
	movi	(((datalabel sym - (label - .)) >> 32) & 65535), reg;	\
	movi	(((datalabel sym - (label - .)) >> 16) & 65535), reg;	\
	shori	((datalabel sym - (label - .)) & 65535), reg
#define	LEAF(sym, reg)		LDC64(sym, reg)
#define	LEARF(fn, label, reg)					\
	movi	(((fn - (label - .)) >> 48) & 65535), reg;	\
	movi	(((fn - (label - .)) >> 32) & 65535), reg;	\
	movi	(((fn - (label - .)) >> 16) & 65535), reg;	\
	shori	((fn - (label - .)) & 65535), reg
#define	LDPTR			ld.q
#define	LDXPTR			ldx.q
#define	STPTR			st.q
#define	STXPTR			stx.q
#define	LINK_FRAME(sz)			\
	addi	r15, -(16 + (sz)), r15	;\
	st.q	r15, (sz), r14		;\
	st.q	r15, (sz) + 8, r18	;\
	add	r15, r63, r14

#define	UNLINK_FRAME(sz)		\
	add	r14, r63, r15		;\
	ld.q	r15, (sz), r14		;\
	ld.q	r15, (sz) + 8, r18	;\
	addi	r15, (sz) + 16, r15

#define	LINK_BIG_FRAME(sz)		\
	addi	r15, -16, r15		;\
	movi	sz, r0			;\
	st.q	r15, 0, r14		;\
	st.q	r15, 8, r18		;\
	sub	r15, r0, r15		;\
	add	r15, r63, r14

#define	UNLINK_BIG_FRAME(sz)		\
	movi	sz, r0			;\
	add	r14, r0, r15		;\
	ld.q	r15, 0, r14		;\
	ld.q	r15, 8, r18		;\
	addi	r15, 16, r15
#endif

#ifndef PIC
#define	PIC_PROLOGUE
#define	PIC_EPILOGUE
#define	PIC_GET_GOT(tr)
#define	PIC_LEAF(x, r)					\
	LEAF(x, r)
#define	PIC_LEA(x, r)					\
	LEA(x, r)
#define	PIC_PTAL(x, tmp, tr)				\
	pta/l	x, tr
#define	PIC_PTAU(x, tmp, tr)				\
	pta/u	x, tr
#else
#define	PIC_GET_GOT(tr)					\
	LEAR(_GLOBAL_OFFSET_TABLE_, 666f, r12)		;\
666:	ptrel/u	r12, tr					;\
	gettr	tr, r12
#define	PIC_PROLOGUE					\
	addi	r15, -8, r15				;\
	st.q	r15, 0, r12				;\
	PIC_GET_GOT(tr0)
#define	PIC_EPILOGUE					\
	ld.q	r15, 0, r12				;\
	addi	r15, 8, r15
#define	PIC_LEAF(x, r)					\
	LEAF(x@GOTPLT, r)				;\
	LDXPTR	r12, r, r
#define	PIC_LEA(x, r)					\
	LEA(x@GOT, r)					;\
	LDXPTR	r12, r, r
#define	PIC_PTAL(x, tmp, tr)				\
	PIC_LEAF(x, tmp)				;\
	ptabs/l	tmp, tr
#define	PIC_PTAU(x, tmp, tr)				\
	PIC_LEAF(x, tmp)				;\
	ptabs/u	tmp, tr
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
#ifdef _KERNEL
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
#endif	/* _KERNEL */

#endif /* _SH5_ASM_H */
