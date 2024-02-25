/*	$NetBSD: glob.h,v 1.2.2.2 2024/02/25 15:47:20 martin Exp $	*/

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

#include <isc/lang.h>
#include <isc/result.h>

#if HAVE_GLOB_H
#include <glob.h>
#else
#include <stddef.h>

#include <isc/mem.h>

typedef struct {
	size_t	   gl_pathc;
	char	 **gl_pathv;
	isc_mem_t *mctx;
	void	  *reserved;
} glob_t;

#endif

ISC_LANG_BEGINDECLS

isc_result_t
isc_glob(const char *pattern, glob_t *pglob);

void
isc_globfree(glob_t *pglob);

ISC_LANG_ENDDECLS
