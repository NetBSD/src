/*	$NetBSD: asm.h,v 1.2 2000/06/23 12:18:46 kleink Exp $	*/

/*
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
 *	from: @(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _ARM32_ASM_H_
#define _ARM32_ASM_H_

#define _BEGIN_ENTRY_NP	.text; .align 0
#define _END_ENTRY_NP

#ifdef GPROF
# define _BEGIN_ENTRY	_BEGIN_ENTRY_NP
# define _END_ENTRY	_END_ENTRY_NP
#else
# define _BEGIN_ENTRY	_BEGIN_ENTRY_NP
# define _END_ENTRY	_END_ENTRY_NP
#endif

#ifdef __ELF__
#  define _C_FUNC(x)	x
#  define _C_LABEL(x)	x
#else
# ifdef __STDC__
#  define _C_FUNC(x)	_ ## x
#  define _C_LABEL(x)	_ ## x
# else
#  define _C_FUNC(x)	_/**/x
#  define _C_LABEL(x)	_/**/x
# endif
#endif

#ifdef __STDC__
# define __CONCAT(x,y)	x ## y
# define __STRING(x)	#x
#else
# define __CONCAT(x,y)	x/**/y
# define __STRING(x)	"x"
#endif
#define	_ASM_FUNC(x)	x

/*
 * gas/arm uses @ as a single comment character and thus cannot be used here
 * Instead it recognised the # instead of an @ symbols in .type directives
 * We define a couple of macros so that assembly code will not be dependant
 * on one or the other.
 */
#define _ASM_TYPE_FUNCTION	#function
#define _ASM_TYPE_OBJECT	#object
#define _ENTRY(x)	.globl x; .type x,_ASM_TYPE_FUNCTION; x:

#define	ENTRY(y)	_BEGIN_ENTRY; _ENTRY(_C_FUNC(y)); _END_ENTRY
#define	TWOENTRY(y,z)	_BEGIN_ENTRY; _ENTRY(_C_FUNC(y)); _ENTRY(_C_FUNC(z)); \
			_END_ENTRY
#define	ASENTRY(y)	_BEGIN_ENTRY; _ENTRY(_ASM_FUNC(y)); _END_ENTRY

#define	ENTRY_NP(y)	_BEGIN_ENTRY_NP; _ENTRY(_C_FUNC(y)); _END_ENTRY_NP
#define	ASENTRY_NP(y)	_BEGIN_ENTRY_NP; _ENTRY(_ASM_FUNC(y)); _END_ENTRY_NP

#define	ASMSTR		.asciz

#ifdef __ELF__
#define RCSID(x)	.section ".ident"; .asciz x
#else
#define RCSID(x)	.text; .asciz x
#endif

#ifdef __ELF__
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;							\
	alias = sym
#endif

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;					\
	.stabs __STRING(_C_LABEL(sym)) ## ,1,0,0,0
#elif defined(__ELF__)
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(sym),1,0,0,0
#else
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;						\
	.stabs __STRING(_/**/sym),1,0,0,0
#endif /* __STDC__ */

#endif /* !_ARM_ASM_H_ */
