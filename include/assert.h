/*	$NetBSD: assert.h,v 1.11.2.1 2001/10/08 20:13:44 nathanw Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)assert.h	8.2 (Berkeley) 1/21/94
 */

/*
 * Unlike other ANSI header files, <assert.h> may usefully be included
 * multiple times, with and without NDEBUG defined.
 */

#include <sys/cdefs.h>

#undef assert
#undef _assert

#ifdef NDEBUG
# ifndef lint
#  define assert(e)	(__static_cast(void,0))
#  define _assert(e)	(__static_cast(void,0))
# else /* !lint */
#  define assert(e)
#  define _assert(e)
# endif /* lint */
#else /* !NDEBUG */
# define _assert(e)	assert(e)
# if __STDC__
#  define assert(e)							\
	((e) ? __static_cast(void,0) : __assert13(__FILE__, __LINE__,	\
	                                          __assert_function__, #e))
# else	/* PCC */
#  define assert(e)							\
	((e) ? __static_cast(void,0) : __assert13(__FILE__, __LINE__,	\
	                                          __assert_function__, "e"))
# endif /* !__STDC__ */
#endif /* NDEBUG */

#undef _DIAGASSERT
#if !defined(_DIAGNOSTIC)
# if !defined(lint)
#  define _DIAGASSERT(e) (__static_cast(void,0))
# else /* !lint */
#  define _DIAGASSERT(e)
# endif /* lint */
#else /* _DIAGNOSTIC */
# if __STDC__
#  define _DIAGASSERT(e)						\
	((e) ? __static_cast(void,0) : __diagassert13(__FILE__, __LINE__, \
	                                              __assert_function__, #e))
# else	/* !__STDC__ */
#  define _DIAGASSERT(e)	 					\
	((e) ? __static_cast(void,0) : __diagassert13(__FILE__, __LINE__, \
	                                              __assert_function__, "e"))
# endif
#endif /* _DIAGNOSTIC */


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define	__assert_function__	__func__
#elif __GNUC_PREREQ__(2, 6)
#define	__assert_function__	__PRETTY_FUNCTION__
#else
#define	__assert_function__	(__static_cast(const void *,0))
#endif

#ifndef __ASSERT_DECLARED
#define __ASSERT_DECLARED
__BEGIN_DECLS
void __assert __P((const char *, int, const char *));
void __assert13 __P((const char *, int, const char *, const char *));
void __diagassert __P((const char *, int, const char *));
void __diagassert13 __P((const char *, int, const char *, const char *));
__END_DECLS
#endif /* __ASSERT_DECLARED */
