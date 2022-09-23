/*	$NetBSD: errno.h,v 1.4 2022/09/23 12:15:33 christos Exp $	*/

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

#ifndef ISC_ERRNO_H
#define ISC_ERRNO_H 1

/*! \file isc/file.h */

#include <isc/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
isc_errno_toresult(int err);
/*!<
 * \brief Convert a POSIX errno value to an ISC result code.
 */
ISC_LANG_ENDDECLS

#endif /* ISC_ERRNO_H */
