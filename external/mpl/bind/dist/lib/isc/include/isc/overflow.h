/*	$NetBSD: overflow.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#pragma once

#include <isc/util.h>

/*
 * It is awkward to support signed numbers as well, so keep it simple
 * (with a safety check).
 */
#define ISC_OVERFLOW_IS_UNSIGNED(a)                                      \
	({                                                               \
		STATIC_ASSERT((typeof(a))-1 > 0,                         \
			      "overflow checks require unsigned types"); \
		(a);                                                     \
	})

#define ISC_OVERFLOW_UINT_MAX(a) ISC_OVERFLOW_IS_UNSIGNED((typeof(a))-1)

#define ISC_OVERFLOW_UINT_MIN(a) ISC_OVERFLOW_IS_UNSIGNED(0)

/*
 * Return true on overflow, e.g.
 *
 *	bool overflow = ISC_OVERFLOW_MUL(count, sizeof(array[0]), &bytes);
 *	INSIST(!overflow);
 */

#if HAVE_BUILTIN_MUL_OVERFLOW
#define ISC_OVERFLOW_MUL(a, b, cp) __builtin_mul_overflow(a, b, cp)
#else
#define ISC_OVERFLOW_MUL(a, b, cp)                                           \
	((ISC_OVERFLOW_UINT_MAX(a) / (b) > (a)) ? (*(cp) = (a) * (b), false) \
						: true)
#endif

#if HAVE_BUILTIN_ADD_OVERFLOW
#define ISC_OVERFLOW_ADD(a, b, cp) __builtin_add_overflow(a, b, cp)
#else
#define ISC_OVERFLOW_ADD(a, b, cp)                                           \
	((ISC_OVERFLOW_UINT_MAX(a) - (b) > (a)) ? (*(cp) = (a) + (b), false) \
						: true)
#endif

#if HAVE_BUILTIN_SUB_OVERFLOW
#define ISC_OVERFLOW_SUB(a, b, cp) __builtin_sub_overflow(a, b, cp)
#else
#define ISC_OVERFLOW_SUB(a, b, cp)                                           \
	((ISC_OVERFLOW_UINT_MIN(a) + (b) < (a)) ? (*(cp) = (a) - (b), false) \
						: true)
#endif

#define ISC_CHECKED_MUL(a, b)                                      \
	({                                                         \
		typeof(a) _c;                                      \
		bool	  _overflow = ISC_OVERFLOW_MUL(a, b, &_c); \
		INSIST(!_overflow);                                \
		_c;                                                \
	})

#define ISC_CHECKED_ADD(a, b)                                      \
	({                                                         \
		typeof(a) _c;                                      \
		bool	  _overflow = ISC_OVERFLOW_ADD(a, b, &_c); \
		INSIST(!_overflow);                                \
		_c;                                                \
	})

#define ISC_CHECKED_SUB(a, b)                                     \
	({                                                        \
		typeof(a) _c;                                     \
		bool	  _overflow = ISC_OVERFLOW_SUB(a, b, cp); \
		INSIST(!_overflow);                               \
		_c;                                               \
	})

#define ISC_CHECKED_MUL_ADD(a, b, c)                              \
	({                                                        \
		size_t _d;                                        \
		bool   _overflow = ISC_OVERFLOW_MUL(a, b, &_d) || \
				 ISC_OVERFLOW_ADD(_d, c, &_d);    \
		INSIST(!_overflow);                               \
		_d;                                               \
	})
