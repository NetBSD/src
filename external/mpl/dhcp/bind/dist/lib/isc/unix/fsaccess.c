/*	$NetBSD: fsaccess.c,v 1.1.2.2 2024/02/24 13:07:30 martin Exp $	*/

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

#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "errno2result.h"

/*! \file
 * \brief
 * The OS-independent part of the API is in lib/isc.
 */
#include "../fsaccess.c"

isc_result_t
isc_fsaccess_set(const char *path, isc_fsaccess_t access) {
	struct stat statb;
	mode_t mode;
	bool is_dir = false;
	isc_fsaccess_t bits;
	isc_result_t result;

	if (stat(path, &statb) != 0) {
		return (isc__errno2result(errno));
	}

	if ((statb.st_mode & S_IFDIR) != 0) {
		is_dir = true;
	} else if ((statb.st_mode & S_IFREG) == 0) {
		return (ISC_R_INVALIDFILE);
	}

	result = check_bad_bits(access, is_dir);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Done with checking bad bits.  Set mode_t.
	 */
	mode = 0;

#define SET_AND_CLEAR1(modebit)     \
	if ((access & bits) != 0) { \
		mode |= modebit;    \
		access &= ~bits;    \
	}
#define SET_AND_CLEAR(user, group, other) \
	SET_AND_CLEAR1(user);             \
	bits <<= STEP;                    \
	SET_AND_CLEAR1(group);            \
	bits <<= STEP;                    \
	SET_AND_CLEAR1(other);

	bits = ISC_FSACCESS_READ | ISC_FSACCESS_LISTDIRECTORY;

	SET_AND_CLEAR(S_IRUSR, S_IRGRP, S_IROTH);

	bits = ISC_FSACCESS_WRITE | ISC_FSACCESS_CREATECHILD |
	       ISC_FSACCESS_DELETECHILD;

	SET_AND_CLEAR(S_IWUSR, S_IWGRP, S_IWOTH);

	bits = ISC_FSACCESS_EXECUTE | ISC_FSACCESS_ACCESSCHILD;

	SET_AND_CLEAR(S_IXUSR, S_IXGRP, S_IXOTH);

	INSIST(access == 0);

	if (chmod(path, mode) < 0) {
		return (isc__errno2result(errno));
	}

	return (ISC_R_SUCCESS);
}
