/*	$NetBSD: errno2result.h,v 1.6 2022/09/23 12:15:34 christos Exp $	*/

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

#ifndef UNIX_ERRNO2RESULT_H
#define UNIX_ERRNO2RESULT_H 1

/* XXXDCL this should be moved to lib/isc/include/isc/errno2result.h. */

#include <errno.h> /* Provides errno. */
#include <stdbool.h>

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

#define isc__errno2result(posixerrno) \
	isc__errno2resultx(posixerrno, true, __FILE__, __LINE__)

isc_result_t
isc__errno2resultx(int posixerrno, bool dolog, const char *file, int line);

ISC_LANG_ENDDECLS

#endif /* UNIX_ERRNO2RESULT_H */
