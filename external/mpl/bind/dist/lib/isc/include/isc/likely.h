/*	$NetBSD: likely.h,v 1.5 2022/09/23 12:15:33 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_LIKELY_H
#define ISC_LIKELY_H 1

/*%
 * Performance
 */
#ifdef CPPCHECK
#define ISC_LIKELY(x)	(x)
#define ISC_UNLIKELY(x) (x)
#else /* ifdef CPPCHECK */
#ifdef HAVE_BUILTIN_EXPECT
#define ISC_LIKELY(x)	__builtin_expect(!!(x), 1)
#define ISC_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else /* ifdef HAVE_BUILTIN_EXPECT */
#define ISC_LIKELY(x)	(x)
#define ISC_UNLIKELY(x) (x)
#endif /* ifdef HAVE_BUILTIN_EXPECT */
#endif /* ifdef CPPCHECK */

#endif /* ISC_LIKELY_H */
