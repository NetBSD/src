/*	$NetBSD: stddef.h,v 1.2 2025/04/01 00:33:55 riastradh Exp $	*/

/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)stddef.h	8.1 (Berkeley) 6/2/93
 */

/*
 * C99, 7.17: Common definitions <stddef.h>
 * C11, 7.19: Common definitoins <stddef.h>
 * C23, 7.21: Common definitions <stddef.h>
 */

#ifndef _SYS_STDDEF_H_
#define _SYS_STDDEF_H_

/*
 * C23	`2. The macro
 *
 *		__STDC_VERSION_STDDEF_H__
 *
 *	    is an integer constant expression with a value equivalent
 *	    to 202311L.'
 */
#if defined(_NETBSD_SOURCE) || defined(_ISOC23_SOURCE) || \
    (__STDC_VERSION__ - 0) >= 202311L
#define	__STDC_VERSION_STDDEF_H__	202311L
#endif

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/ansi.h>

/*
 * C23	`3. The types are
 *
 *		ptrdiff_t
 *
 *	    which is the signed integer type of the result of
 *	    subtracting two pointers;
 *
 *		size_t
 *
 *	    which is the unsigned integer type of the result of the
 *	    sizeof operator;
 *
 *		max_align_t
 *
 *	    which is an object type whose alignment is the greatest
 *	    fundamental alignment;
 *
 *		wchar_t
 *
 *	    which is an integer type whose range of values can
 *	    represent distinct codes for all members of the largest
 *	    extended chracter set specified among the supported
 *	    locales; [...] and
 *
 *		nullptr_t
 *
 *	    which is the type of the nullptr predefined constant, see
 *	    below.'
 */
#ifdef	_BSD_PTRDIFF_T_
typedef	_BSD_PTRDIFF_T_	ptrdiff_t;
#undef	_BSD_PTRDIFF_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#if (__STDC_VERSION__ - 0) >= 201112L || (__cplusplus - 0) >= 201103L
typedef union {
	void *_v;
	long double _ld;
	long long int _ll;
} max_align_t;
#endif

#if defined(_BSD_WCHAR_T_) && !defined(__cplusplus)
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

#if (__STDC_VERSION__ - 0) >= 202311L
typedef typeof_unqual(nullptr)	nullptr_t;
#endif

/*
 * C23	`4. The macros are
 *
 *		NULL
 *
 *	    which expands to an implementation-defined null pointer
 *	    constant;
 *
 *		unreachable()
 *
 *	    which expands to a void expression that invokes undefined
 *	    behavior if it is reached during execution; and
 *
 *		offsetof(type, member-designator)
 *
 *	    which expands to an integer constant expression that has
 *	    type size_t, the value of which is the offset in bytes, to
 *	    the subobject (designated by member-designator), from the
 *	    beginning of any object of type type.'
 */

#include <sys/null.h>

#if (__STDC_VERSION__ - 0) >= 202311L
#define	unreachable()	__unreachable() /* sys/cdefs.h */
#endif

#if __GNUC_PREREQ__(4, 0)
#define	offsetof(type, member)	__builtin_offsetof(type, member)
#elif !defined(__cplusplus)
#define	offsetof(type, member)	((size_t)(unsigned long)(&((type *)0)->member))
#else
#if !__GNUC_PREREQ__(3, 4)
#define __offsetof__(a) a
#endif
#define	offsetof(type, member) __offsetof__((reinterpret_cast<size_t> \
    (&reinterpret_cast<const volatile char &>(static_cast<type *>(0)->member))))
#endif

#endif /* _SYS_STDDEF_H_ */
