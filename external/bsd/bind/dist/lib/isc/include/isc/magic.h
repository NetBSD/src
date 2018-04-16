/*	$NetBSD: magic.h,v 1.5.14.1 2018/04/16 01:57:58 pgoyette Exp $	*/

/*
 * Copyright (C) 2004-2007, 2016, 2017  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/* Id: magic.h,v 1.18 2007/06/19 23:47:18 tbox Exp  */

#ifndef ISC_MAGIC_H
#define ISC_MAGIC_H 1

#include <isc/likely.h>

/*! \file isc/magic.h */

typedef struct {
	unsigned int magic;
} isc__magic_t;


/*%
 * To use this macro the magic number MUST be the first thing in the
 * structure, and MUST be of type "unsigned int".
 * The intent of this is to allow magic numbers to be checked even though
 * the object is otherwise opaque.
 */
#define ISC_MAGIC_VALID(a,b)	(ISC_LIKELY((a) != NULL) && \
				 ISC_LIKELY(((const isc__magic_t *)(a))->magic == (b)))

#define ISC_MAGIC(a, b, c, d)	((a) << 24 | (b) << 16 | (c) << 8 | (d))

#endif /* ISC_MAGIC_H */
