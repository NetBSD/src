/*	$NetBSD: dir.h,v 1.2.2.2 2024/02/25 15:47:20 martin Exp $	*/

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

/*! \file */

#include <dirent.h>
#include <limits.h>

#include <isc/lang.h>
#include <isc/result.h>

#include <sys/types.h> /* Required on some systems. */

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

/*% Directory Entry */
typedef struct isc_direntry {
	char	     name[NAME_MAX];
	unsigned int length;
} isc_direntry_t;

/*% Directory */
typedef struct isc_dir {
	unsigned int   magic;
	char	       dirname[PATH_MAX];
	isc_direntry_t entry;
	DIR	      *handle;
} isc_dir_t;

ISC_LANG_BEGINDECLS

void
isc_dir_init(isc_dir_t *dir);

isc_result_t
isc_dir_open(isc_dir_t *dir, const char *dirname);

isc_result_t
isc_dir_read(isc_dir_t *dir);

isc_result_t
isc_dir_reset(isc_dir_t *dir);

void
isc_dir_close(isc_dir_t *dir);

isc_result_t
isc_dir_chdir(const char *dirname);

isc_result_t
isc_dir_chroot(const char *dirname);

isc_result_t
isc_dir_createunique(char *templet);
/*!<
 * Use a templet (such as from isc_file_mktemplate()) to create a uniquely
 * named, empty directory.  The templet string is modified in place.
 * If result == ISC_R_SUCCESS, it is the name of the directory that was
 * created.
 */

ISC_LANG_ENDDECLS
