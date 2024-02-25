/*	$NetBSD: glob.c,v 1.2.2.2 2024/02/25 15:47:16 martin Exp $	*/

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

#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <string.h>

#include <isc/errno.h>
#include <isc/glob.h>
#include <isc/print.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

isc_result_t
isc_glob(const char *pattern, glob_t *pglob) {
	REQUIRE(pattern != NULL);
	REQUIRE(*pattern != '\0');
	REQUIRE(pglob != NULL);

	int rc = glob(pattern, GLOB_ERR, NULL, pglob);

	switch (rc) {
	case 0:
		return (ISC_R_SUCCESS);

	case GLOB_NOMATCH:
		return (ISC_R_FILENOTFOUND);

	case GLOB_NOSPACE:
		return (ISC_R_NOMEMORY);

	default:
		return (errno != 0 ? isc_errno_toresult(errno) : ISC_R_IOERROR);
	}
}

void
isc_globfree(glob_t *pglob) {
	REQUIRE(pglob != NULL);
	globfree(pglob);
}
