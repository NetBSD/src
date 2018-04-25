/*	$NetBSD: asm.h,v 1.27 2018/04/25 09:23:00 ragge Exp $ */
/*
 * Copyright (c) 1982, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)DEFS.h	8.1 (Berkeley) 6/4/93
 */

#ifndef _VAX_ASM_H_
#define _VAX_ASM_H_

#define R0	0x001
#define R1	0x002
#define R2	0x004
#define R3	0x008
#define R4	0x010
#define R5	0x020
#define R6	0x040
#define R7 	0x080
#define R8	0x100
#define R9	0x200
#define R10	0x400
#define R11	0x800

#define _C_LABEL(x)	x

#define	_ASM_LABEL(x)	x

#ifdef __STDC__
# define __CONCAT(x,y)	x ## y
# define __STRING(x)	#x
#else
# define __CONCAT(x,y)	x/**/y
# define __STRING(x)	"x"
#endif

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# define _ALIGN_TEXT .p2align 2
#endif

#define	_ENTRY(x, regs) \
	.text; _ALIGN_TEXT; .globl x; .type x@function; x: .word regs

#ifdef GPROF
# define _PROF_PROLOGUE	\
	.data; 1:; .long 0; .text; moval 1b,%r0; jsb _ASM_LABEL(__mcount)
#else
# define _PROF_PROLOGUE
#endif

#define ENTRY(x, regs)		_ENTRY(_C_LABEL(x), regs); _PROF_PROLOGUE
#define NENTRY(x, regs)		_ENTRY(_C_LABEL(x), regs)
#define ASENTRY(x, regs)	_ENTRY(_ASM_LABEL(x), regs); _PROF_PROLOGUE
#define END(x)			.size _C_LABEL(x),.-_C_LABEL(x)

#define ALTENTRY(x)		.globl _C_LABEL(x); _C_LABEL(x):
#define RCSID(name)		.pushsection ".ident"; .asciz name; .popsection

#ifdef NO_KERNEL_RCSIDS
#define __KERNEL_RCSID(_n, _s)  /* nothing */
#else
#define __KERNEL_RCSID(_n, _s)  RCSID(_s)
#endif

#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.pushsection .gnu.warning. ## sym;				\
	.ascii msg;							\
	.popsection
#else
#define	WARN_REFERENCES(sym,msg)					\
	.pushsection .gnu.warning./**/sym;				\
	.ascii msg;							\
	.popsection
#endif /* __STDC__ */

.macro	polyf arg:req degree:req tbladdr:req
	movf	\arg, %r1
	movzwl	\degree, %r2
	movab	\tbladdr, %r3

	movf	(%r3)+, %r0
1:
	mulf2	%r1, %r0		/* result *= arg */
	addf2	(%r3)+, %r0		/* result += c[n] */
	sobgtr	%r2, 1b
	clrf	%r1			/* r1 is 0 on finish */
.endm

.macro	polyd arg:req degree:req tbladdr:req
	movd	\arg, %r4
	movzwl	\degree, %r2
	movab	\tbladdr, %r3

	movd	(%r3)+,	%r0
1:
	muld2	%r4, %r0		/* result *= arg */
	addd2	(%r3)+, %r0		/* result += c[n] */
	sobgtr	%r2, 1b
	clrq	%r4			/* r4, r5 are 0 on finish */
.endm

#endif /* !_VAX_ASM_H_ */
