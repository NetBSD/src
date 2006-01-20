/*	$NetBSD: asm.h,v 1.3 2006/01/20 22:02:40 christos Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _PDP10_ASM_H_
#define _PDP10_ASM_H_

#ifdef __ELF__
# define _C_LABEL(x)	x
#else
# ifdef __STDC__
#  define _C_LABEL(x)	_ ## x
# else
#  define _C_LABEL(x)	_/**/x
# endif
#endif

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
# ifdef __ELF__
#  define _ALIGN_TEXT .align 4
# else
#  define _ALIGN_TEXT .align 2
# endif
#endif

#define	_ENTRY(x) \
	.text ; .globl x ; __CONCAT(x,:)

#ifdef GPROF
# ifdef __ELF__
#  define _PROF_PROLOGUE	\
	.data; 1:; .long 0; .text; moval 1b,r0; jsb _ASM_LABEL(__mcount)
# else 
#  define _PROF_PROLOGUE	\
	.data; 1:; .long 0; .text; moval 1b,r0; jsb _ASM_LABEL(mcount)
# endif
#else
# define _PROF_PROLOGUE
#endif

#define ENTRY(x)		_ENTRY(_C_LABEL(x)); _PROF_PROLOGUE
#define NENTRY(x, regs)		_ENTRY(_C_LABEL(x))
#define ASENTRY(x, regs)	_ENTRY(_ASM_LABEL(x)); _PROF_PROLOGUE

#define ALTENTRY(x)	.globl _C_LABEL(x) ; _C_LABEL(x):
#define RCSID(x)	.text ; .asciz x

#ifdef __ELF__
#define	WEAK_ALIAS(alias,sym) .weak alias ;  alias = sym
#endif
/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;					\
	.stabs __STRING(_C_LABEL(sym)) ## ,1,0,0,0
#else
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(_C_LABEL(sym)),1,0,0,0
#endif /* __STDC__ */

#endif /* !_PDP10_ASM_H_ */
