/*	$NetBSD: bitypes.h,v 1.1.1.2 2005/12/21 19:57:31 christos Exp $	*/

/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

	/*
	 * Basic integral types.  Omit the typedef if
	 * not possible for a machine/compiler combination.
	 */
	typedef /*signed*/ char            int8_t, int8m_t;
	typedef unsigned char            u_int8_t, u_int8m_t;
	typedef short                     int16_t, int16m_t;
	typedef unsigned short          u_int16_t, u_int16m_t;
	typedef int                       int32_t, int32m_t;
	typedef unsigned int            u_int32_t, u_int32m_t;

	typedef long			  int64_t, int64m_t;
	typedef unsigned long		u_int64_t, u_int64m_t;

	typedef u_int8_t uint8_t;
	typedef u_int16_t uint16_t;
	typedef u_int32_t uint32_t;
	typedef u_int64_t uint64_t;

#endif	/* __BIT_TYPES_DEFINED__ */
