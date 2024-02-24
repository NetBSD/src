/*	$NetBSD: formatcheck.h,v 1.1.2.2 2024/02/24 13:07:24 martin Exp $	*/

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

#ifndef ISC_FORMATCHECK_H
#define ISC_FORMATCHECK_H 1

/*! \file isc/formatcheck.h */

/*%
 * ISC_FORMAT_PRINTF().
 *
 * \li fmt is the location of the format string parameter.
 * \li args is the location of the first argument (or 0 for no argument
 * checking).
 *
 * Note:
 * \li The first parameter is 1, not 0.
 */
#ifdef __GNUC__
#define ISC_FORMAT_PRINTF(fmt, args) \
	__attribute__((__format__(__printf__, fmt, args)))
#else /* ifdef __GNUC__ */
#define ISC_FORMAT_PRINTF(fmt, args)
#endif /* ifdef __GNUC__ */

#endif /* ISC_FORMATCHECK_H */
