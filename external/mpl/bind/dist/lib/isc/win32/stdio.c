/*	$NetBSD: stdio.c,v 1.5 2022/09/23 12:15:35 christos Exp $	*/

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
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <isc/stdio.h>
#include <isc/util.h>

#include "errno2result.h"

isc_result_t
isc_stdio_open(const char *filename, const char *mode, FILE **fp) {
	FILE *f;

	f = fopen(filename, mode);
	if (f == NULL) {
		return (isc__errno2result(errno));
	}
	*fp = f;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_stdio_close(FILE *f) {
	int r;

	r = fclose(f);
	if (r == 0) {
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}

isc_result_t
isc_stdio_seek(FILE *f, off_t offset, int whence) {
	int r;

#ifndef _WIN64
	r = fseek(f, offset, whence);
#else  /* ifndef _WIN64 */
	r = _fseeki64(f, offset, whence);
#endif /* ifndef _WIN64 */
	if (r == 0) {
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}

isc_result_t
isc_stdio_tell(FILE *f, off_t *offsetp) {
#ifndef _WIN64
	long r;
#else  /* ifndef _WIN64 */
	__int64 r;
#endif /* ifndef _WIN64 */

	REQUIRE(offsetp != NULL);

#ifndef _WIN64
	r = ftell(f);
#else  /* ifndef _WIN64 */
	r = _ftelli64(f);
#endif /* ifndef _WIN64 */
	if (r >= 0) {
		*offsetp = r;
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}

isc_result_t
isc_stdio_read(void *ptr, size_t size, size_t nmemb, FILE *f, size_t *nret) {
	isc_result_t result = ISC_R_SUCCESS;
	size_t r;

	clearerr(f);
	r = fread(ptr, size, nmemb, f);
	if (r != nmemb) {
		if (feof(f)) {
			result = ISC_R_EOF;
		} else {
			result = isc__errno2result(errno);
		}
	}
	if (nret != NULL) {
		*nret = r;
	}
	return (result);
}

isc_result_t
isc_stdio_write(const void *ptr, size_t size, size_t nmemb, FILE *f,
		size_t *nret) {
	isc_result_t result = ISC_R_SUCCESS;
	size_t r;

	clearerr(f);
	r = fwrite(ptr, size, nmemb, f);
	if (r != nmemb) {
		result = isc__errno2result(errno);
	}
	if (nret != NULL) {
		*nret = r;
	}
	return (result);
}

isc_result_t
isc_stdio_flush(FILE *f) {
	int r;

	r = fflush(f);
	if (r == 0) {
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}

isc_result_t
isc_stdio_sync(FILE *f) {
	struct _stat buf;
	int r;

	if (_fstat(_fileno(f), &buf) != 0) {
		return (isc__errno2result(errno));
	}

	/*
	 * Only call _commit() on regular files.
	 */
	if ((buf.st_mode & S_IFMT) != S_IFREG) {
		return (ISC_R_SUCCESS);
	}

	r = _commit(_fileno(f));
	if (r == 0) {
		return (ISC_R_SUCCESS);
	} else {
		return (isc__errno2result(errno));
	}
}
