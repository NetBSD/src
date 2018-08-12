/*	$NetBSD: dir.h,v 1.2 2018/08/12 13:02:39 christos Exp $	*/

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

#ifndef ISC_DIR_H
#define ISC_DIR_H 1

/*! \file */

#include <sys/types.h>		/* Required on some systems. */
#include <dirent.h>

#include <isc/lang.h>
#include <isc/result.h>

#define ISC_DIR_NAMEMAX 256
#define ISC_DIR_PATHMAX 1024

/*% Directory Entry */
typedef struct isc_direntry {
	/*!
	 * Ideally, this should be NAME_MAX, but AIX does not define it by
	 * default and dynamically allocating the space based on pathconf()
	 * complicates things undesirably, as does adding special conditionals
	 * just for AIX.  So a comfortably sized buffer is chosen instead.
	 */
	char 		name[ISC_DIR_NAMEMAX];
	unsigned int	length;
} isc_direntry_t;

/*% Directory */
typedef struct isc_dir {
	unsigned int	magic;
	/*!
	 * As with isc_direntry_t->name, making this "right" for all systems
	 * is slightly problematic because AIX does not define PATH_MAX.
	 */
	char		dirname[ISC_DIR_PATHMAX];
	isc_direntry_t	entry;
	DIR *		handle;
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

#endif /* ISC_DIR_H */
