/*	$NetBSD: offset.h,v 1.5 2015/09/12 19:03:11 joerg Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
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

/* Id: offset.h,v 1.17 2008/12/01 23:47:45 tbox Exp  */

#ifndef ISC_OFFSET_H
#define ISC_OFFSET_H 1

/*! \file
 * \brief
 * File offsets are operating-system dependent.
 */
#include <limits.h>             /* Required for CHAR_BIT. */
#include <sys/types.h>
#include <stddef.h>		/* For Linux Standard Base. */

typedef off_t isc_offset_t;

#define ISC_OFFSET_MAXIMUM \
    (sizeof(off_t) == sizeof(char) ? INT_MAX : \
     (sizeof(off_t) == sizeof(short) ? SHRT_MAX : \
      (sizeof(off_t) == sizeof(int) ? INT_MAX : \
       (sizeof(off_t) == sizeof(long) ? LONG_MAX : \
        (sizeof(off_t) == sizeof(long long) ? LLONG_MAX : INTMAX_MAX)))))


#endif /* ISC_OFFSET_H */
