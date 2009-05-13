/*	$NetBSD: bitypes.h,v 1.1.1.1.2.2 2009/05/13 18:52:21 jym Exp $	*/

/*
 * Copyright (C) 2008  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: bitypes.h,v 1.2 2008/11/14 02:54:35 tbox Exp */

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

	/*
	 * Basic integral types.  Omit the typedef if
	 * not possible for a machine/compiler combination.
	 */
	typedef /*signed*/ char            int8_t;
	typedef unsigned char            u_int8_t;
	typedef short                     int16_t;
	typedef unsigned short          u_int16_t;
	typedef int                       int32_t;
	typedef unsigned int            u_int32_t;

# if 0	/* don't fight with these unless you need them */
	typedef long long                 int64_t;
	typedef unsigned long long      u_int64_t;
# endif

#endif	/* __BIT_TYPES_DEFINED__ */
