/*	$NetBSD: magic.h,v 1.6 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

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
static __inline int
checkmagic(const void *_p, unsigned int _m)
{
	const isc__magic_t *s = _p;
	return s->magic == _m;
}

#define ISC_MAGIC_VALID(a, b)       \
	(ISC_LIKELY((a) != NULL) && ISC_LIKELY(checkmagic(a, b))) \

#define ISC_MAGIC(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

#endif /* ISC_MAGIC_H */
