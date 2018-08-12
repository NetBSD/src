/*	$NetBSD: stdio.c,v 1.1.1.1 2018/08/12 12:08:25 christos Exp $	*/

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

#include <errno.h>
#include <unistd.h>

#include <isc/stdio.h>
#include <isc/stat.h>
#include <isc/util.h>

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

#ifdef HAVE_FSEEKO
	r = fseeko(f, offset, whence);
#else
	r = fseek(f, offset, whence);
#endif
	if (r == 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

isc_result_t
isc_stdio_tell(FILE *f, off_t *offsetp) {
	off_t r;

	REQUIRE(offsetp != NULL);

#ifdef HAVE_FTELLO
	r = ftello(f);
#else
	r = ftell(f);
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

/*
 * OpenBSD has deprecated ENOTSUP in favor of EOPNOTSUPP.
 */
#if defined(EOPNOTSUPP) && !defined(ENOTSUP)
#define ENOTSUP EOPNOTSUPP
#endif

isc_result_t
isc_stdio_sync(FILE *f) {
	struct stat buf;
	int r;

	if (fstat(fileno(f), &buf) != 0)
		return (isc__errno2result(errno));

	/*
	 * Only call fsync() on regular files.
	 */
	if ((buf.st_mode & S_IFMT) != S_IFREG)
		return (ISC_R_SUCCESS);

	r = fsync(fileno(f));
	if (r == 0)
		return (ISC_R_SUCCESS);
	else
		return (isc__errno2result(errno));
}

