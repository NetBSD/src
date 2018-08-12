/*	$NetBSD: stdio.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


#ifndef ISC_STDIO_H
#define ISC_STDIO_H 1

/*! \file isc/stdio.h */

/*%
 * These functions are wrappers around the corresponding stdio functions.
 *
 * They return a detailed error code in the form of an an isc_result_t.  ANSI C
 * does not guarantee that stdio functions set errno, hence these functions
 * must use platform dependent methods (e.g., the POSIX errno) to construct the
 * error code.
 */

#include <stdio.h>

#include <isc/lang.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

/*% Open */
isc_result_t
isc_stdio_open(const char *filename, const char *mode, FILE **fp);

/*% Close */
isc_result_t
isc_stdio_close(FILE *f);

/*% Seek */
isc_result_t
isc_stdio_seek(FILE *f, off_t offset, int whence);

/*% Tell */
isc_result_t
isc_stdio_tell(FILE *f, off_t *offsetp);

/*% Read */
isc_result_t
isc_stdio_read(void *ptr, size_t size, size_t nmemb, FILE *f,
	       size_t *nret);

/*% Write */
isc_result_t
isc_stdio_write(const void *ptr, size_t size, size_t nmemb, FILE *f,
		size_t *nret);

/*% Flush */
isc_result_t
isc_stdio_flush(FILE *f);

isc_result_t
isc_stdio_sync(FILE *f);
/*%<
 * Invoke fsync() on the file descriptor underlying an stdio stream, or an
 * equivalent system-dependent operation.  Note that this function has no
 * direct counterpart in the stdio library.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_STDIO_H */
