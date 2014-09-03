/* $NetBSD: asm.h,v 1.1 2014/09/03 19:34:26 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _OR1K_ASM_H_
#define _OR1K_ASM_H_

#define	__BIT(n)	(1 << (n))
#define __BITS(hi,lo)	((~((~0)<<((hi)+1)))&((~0)<<(lo)))

#define _C_LABEL(x)	x
#define	_ASM_LABEL(x)	x

#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x

#ifndef _ALIGN_TEXT
# define _ALIGN_TEXT .align 4
#endif

#ifndef _TEXT_SECTION
#define _TEXT_SECTION	.text
#endif
#define _ASM_TYPE_FUNCTION	@function
#define _ASM_TYPE_OBJECT	@object
#define _ENTRY(x) \
	_TEXT_SECTION; _ALIGN_TEXT; .globl x; .type x,_ASM_TYPE_FUNCTION; x:
#define	_END(x)		.size x,.-x

#ifdef GPROF
# define _PROF_PROLOGUE	\
	mov x9, x30; l.jal __mcount
#else
# define _PROF_PROLOGUE
#endif

#define	ENTRY(y)		_ENTRY(_C_LABEL(y)); _PROF_PROLOGUE
#define	ENTRY_NP(y)		_ENTRY(_C_LABEL(y))
#define	END(y)			_END(_C_LABEL(y))
#define	END(y)			_END(_C_LABEL(y))
#define	ASENTRY(y)		_ENTRY(_ASM_LABEL(y)); _PROF_PROLOGUE
#define	ASENTRY_NP(y)		_ENTRY(_ASM_LABEL(y))
#define	ASEND(y)		_END(_ASM_LABEL(y))

#define	ASMSTR		.asciz

#ifdef __PIC__
#define	PLT(x)			plt(x)
#define	PIC_GOTSETUP(r)					\
        l.jal	999f					;\
	l.nop						;\
999:	l.movhi	r,gotpchi(_GLOBAL_OFFSET_TABLE_+0)	;\
	l.ori	r,r,gotpclo(_GLOBAL_OFFSET_TABLE_+4)	;\
	l.add	r,r,r9
#else
#define	PLT(x)			x
#endif

#define __RCSID(x)	.pushsection ".ident"; .asciz x; .popsection
#define RCSID(x)	__RCSID(x)

#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym

/*
 * STRONG_ALIAS: create a strong alias.
 */
#define STRONG_ALIAS(alias,sym)						\
	.globl alias;							\
	alias = sym

#define	WARN_REFERENCES(sym,msg)					\
	.pushsection .gnu.warning. ## sym;				\
	.ascii msg;							\
	.popsection

#endif /* !_OR1K_ASM_H_ */
