/*	$NetBSD: dir.c,v 1.5 2022/09/23 12:15:27 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "dir.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dlz_minimal.h"

void
dir_init(dir_t *dir) {
	dir->entry.name[0] = '\0';
	dir->entry.length = 0;

	dir->handle = NULL;
}

isc_result_t
dir_open(dir_t *dir, const char *dirname) {
	char *p;
	isc_result_t result = ISC_R_SUCCESS;

	if (strlen(dirname) + 3 > sizeof(dir->dirname)) {
		return (ISC_R_NOSPACE);
	}
	strcpy(dir->dirname, dirname);

	p = dir->dirname + strlen(dir->dirname);
	if (dir->dirname < p && *(p - 1) != '/') {
		*p++ = '/';
	}
	*p++ = '*';
	*p = '\0';

	dir->handle = opendir(dirname);
	if (dir->handle == NULL) {
		switch (errno) {
		case ENOTDIR:
		case ELOOP:
		case EINVAL:
		case ENAMETOOLONG:
		case EBADF:
			result = ISC_R_INVALIDFILE;
			break;
		case ENOENT:
			result = ISC_R_FILENOTFOUND;
			break;
		case EACCES:
		case EPERM:
			result = ISC_R_NOPERM;
			break;
		case ENOMEM:
			result = ISC_R_NOMEMORY;
			break;
		default:
			result = ISC_R_UNEXPECTED;
			break;
		}
	}

	return (result);
}

/*!
 * \brief Return previously retrieved file or get next one.
 *
 * Unix's dirent has
 * separate open and read functions, but the Win32 and DOS interfaces open
 * the dir stream and reads the first file in one operation.
 */
isc_result_t
dir_read(dir_t *dir) {
	struct dirent *entry;

	entry = readdir(dir->handle);
	if (entry == NULL) {
		return (ISC_R_NOMORE);
	}

	if (sizeof(dir->entry.name) <= strlen(entry->d_name)) {
		return (ISC_R_UNEXPECTED);
	}

	strcpy(dir->entry.name, entry->d_name);

	dir->entry.length = strlen(entry->d_name);
	return (ISC_R_SUCCESS);
}

/*!
 * \brief Close directory stream.
 */
void
dir_close(dir_t *dir) {
	(void)closedir(dir->handle);
	dir->handle = NULL;
}

/*!
 * \brief Reposition directory stream at start.
 */
isc_result_t
dir_reset(dir_t *dir) {
	rewinddir(dir->handle);

	return (ISC_R_SUCCESS);
}
