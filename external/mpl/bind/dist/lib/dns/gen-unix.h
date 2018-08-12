/*	$NetBSD: gen-unix.h,v 1.2 2018/08/12 13:02:35 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

#include <sys/types.h>          /* Required on some systems for dirent.h. */

#include <dirent.h>
#include <unistd.h>		/* XXXDCL Required for ?. */

#include <isc/boolean.h>
#include <isc/lang.h>

#ifdef NEED_OPTARG
extern char *optarg;
#endif

#define isc_commandline_parse		getopt
#define isc_commandline_argument 	optarg

typedef struct {
	DIR *handle;
	char *filename;
} isc_dir_t;

ISC_LANG_BEGINDECLS

static isc_boolean_t
start_directory(const char *path, isc_dir_t *dir) {
	dir->handle = opendir(path);

	if (dir->handle != NULL)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);

}

static isc_boolean_t
next_file(isc_dir_t *dir) {
	struct dirent *dirent;

	dir->filename = NULL;

	if (dir->handle != NULL) {
		dirent = readdir(dir->handle);
		if (dirent != NULL)
			dir->filename = dirent->d_name;
	}

	if (dir->filename != NULL)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);
}

static void
end_directory(isc_dir_t *dir) {
	if (dir->handle != NULL)
		(void)closedir(dir->handle);

	dir->handle = NULL;
}

ISC_LANG_ENDDECLS

#endif /* DNS_GEN_UNIX_H */
