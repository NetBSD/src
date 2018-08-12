/*	$NetBSD: stdio.c,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#include <config.h>

#include <io.h>
#include <errno.h>

#include <isc/stdio.h>
#include <isc/util.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "errno2result.h"

isc_result_t
isc_stdio_open(const char *filename, const char *mode, FILE **fp) {
	FILE *f;

	f = fopen(filename, mode);
	if (f == NULL)
		return (isc__errno2result(errno));
	*fp = f;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_stdio_close(FILE *f) {
	int r;

	r = fclose(f);
	if (r == 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

isc_result_t
isc_stdio_seek(FILE *f, off_t offset, int whence) {
	int r;

#ifndef _WIN64
	r = fseek(f, offset, whence);
#else
	r = _fseeki64(f, offset, whence);
#endif
	if (r == 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

isc_result_t
isc_stdio_tell(FILE *f, off_t *offsetp) {
#ifndef _WIN64
	long r;
#else
	__int64 r;
#endif

	REQUIRE(offsetp != NULL);

#ifndef _WIN64
	r = ftell(f);
#else
	r = _ftelli64(f);
#endif
	if (r >= 0) {
		*offsetp = r;
		return (ISC_R_SUCCESS);
	} else
		return (isc__errno2result(errno));
}

isc_result_t
isc_stdio_read(void *ptr, size_t size, size_t nmemb, FILE *f, size_t *nret) {
	isc_result_t result = ISC_R_SUCCESS;
	size_t r;

	clearerr(f);
	r = fread(ptr, size, nmemb, f);
	if (r != nmemb) {
		if (feof(f))
			result = ISC_R_EOF;
		else
			result = isc__errno2result(errno);
	}
	if (nret != NULL)
		*nret = r;
	return (result);
}

isc_result_t
isc_stdio_write(const void *ptr, size_t size, size_t nmemb, FILE *f,
	       size_t *nret)
{
	isc_result_t result = ISC_R_SUCCESS;
	size_t r;

	clearerr(f);
	r = fwrite(ptr, size, nmemb, f);
	if (r != nmemb)
		result = isc__errno2result(errno);
	if (nret != NULL)
		*nret = r;
	return (result);
}

isc_result_t
isc_stdio_flush(FILE *f) {
	int r;

	r = fflush(f);
	if (r == 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

isc_result_t
isc_stdio_sync(FILE *f) {
	struct _stat buf;
	int r;

	if (_fstat(_fileno(f), &buf) != 0)
		return (isc__errno2result(errno));

	/*
	 * Only call _commit() on regular files.
	 */
	if ((buf.st_mode & S_IFMT) != S_IFREG)
		return (ISC_R_SUCCESS);

	r = _commit(_fileno(f));
	if (r == 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

