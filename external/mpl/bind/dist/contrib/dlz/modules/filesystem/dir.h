/*	$NetBSD: dir.h,v 1.4 2022/09/23 12:15:27 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <dirent.h>
#include <sys/types.h>

#include <dlz_minimal.h>

#define DIR_NAMEMAX 256
#define DIR_PATHMAX 1024

typedef struct direntry {
	char name[DIR_NAMEMAX];
	unsigned int length;
} direntry_t;

typedef struct dir {
	char dirname[DIR_PATHMAX];
	direntry_t entry;
	DIR *handle;
} dir_t;

void
dir_init(dir_t *dir);

isc_result_t
dir_open(dir_t *dir, const char *dirname);

isc_result_t
dir_read(dir_t *dir);

isc_result_t
dir_reset(dir_t *dir);

void
dir_close(dir_t *dir);
