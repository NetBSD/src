/*	$NetBSD: uchar.h,v 1.1 2024/08/15 13:14:44 riastradh Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

/*
 * C11, 7.28: Unicode utilities <uchar.h>
 *
 *	`1. The header <uchar.h> declares types and functions for
 *	    manipulating Unicode characters.'
 */

#ifndef	_UCHAR_H
#define	_UCHAR_H

#include <sys/ansi.h>

/*
 *	`2. The types declared are mbstate_t (described in 7.30.1) and
 *	    size_t (described in 7.19);
 *
 *	    	char16_t
 *
 *	    which is an unsigned integer type used for 16-bit
 *	    characters and is the same type as uint_least16_t
 *	    (described in 7.20.1.2); and
 *
 *		char32_t
 *
 *	    which is an unsigned integer type used for 32-bit
 *	    characters and is the same type as uint_least32_t (also
 *	    described in 7.20.1.2).'
 */

#ifdef _BSD_MBSTATE_T_
typedef _BSD_MBSTATE_T_	mbstate_t;
#undef _BSD_MBSTATE_T_
#endif

#ifdef _BSD_SIZE_T_
typedef _BSD_SIZE_T_	size_t;
#undef _BSD_SIZE_T_
#endif

#if !defined(__cplusplus) || __cplusplus < 201103L
typedef __UINT_LEAST16_TYPE__	char16_t;
typedef __UINT_LEAST32_TYPE__	char32_t;
#endif

__BEGIN_DECLS

size_t	mbrtoc16(char16_t *__restrict, const char *__restrict, size_t,
	    mbstate_t *__restrict);
size_t	c16rtomb(char *__restrict, char16_t, mbstate_t *__restrict);
size_t	mbrtoc32(char32_t *__restrict, const char *__restrict, size_t,
	    mbstate_t *__restrict);
size_t	c32rtomb(char *__restrict, char32_t, mbstate_t *__restrict);

__END_DECLS

#endif	/* _UCHAR_H */
