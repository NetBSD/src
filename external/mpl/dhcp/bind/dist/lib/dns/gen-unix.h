/*	$NetBSD: gen-unix.h,v 1.1.2.2 2024/02/24 13:06:58 martin Exp $	*/

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

/*! \file
 * \brief
 * This file is responsible for defining two operations that are not
 * directly portable between Unix-like systems and Windows NT, option
 * parsing and directory scanning.  It is here because it was decided
 * that the "gen" build utility was not to depend on libisc.a, so
 * the functions declared in isc/commandline.h and isc/dir.h could not
 * be used.
 *
 * The commandline stuff is really just a wrapper around getopt().
 * The dir stuff was shrunk to fit the needs of gen.c.
 */

#ifndef DNS_GEN_UNIX_H
#define DNS_GEN_UNIX_H 1

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h> /* Required on some systems for dirent.h. */
#include <unistd.h>    /* XXXDCL Required for ?. */

#include <isc/lang.h>

#ifdef NEED_OPTARG
extern char *optarg;
#endif /* ifdef NEED_OPTARG */

#define isc_commandline_parse	 getopt
#define isc_commandline_argument optarg

typedef struct {
	DIR *handle;
	char *filename;
} isc_dir_t;

ISC_LANG_BEGINDECLS

static bool
start_directory(const char *path, isc_dir_t *dir) {
	dir->handle = opendir(path);

	if (dir->handle != NULL) {
		return (true);
	} else {
		return (false);
	}
}

static bool
next_file(isc_dir_t *dir) {
	struct dirent *dirent;

	dir->filename = NULL;

	if (dir->handle != NULL) {
		errno = 0;
		dirent = readdir(dir->handle);
		if (dirent != NULL) {
			dir->filename = dirent->d_name;
		} else {
			if (errno != 0) {
				exit(1);
			}
		}
	}

	if (dir->filename != NULL) {
		return (true);
	} else {
		return (false);
	}
}

static void
end_directory(isc_dir_t *dir) {
	if (dir->handle != NULL) {
		(void)closedir(dir->handle);
	}

	dir->handle = NULL;
}

ISC_LANG_ENDDECLS

#endif /* DNS_GEN_UNIX_H */
