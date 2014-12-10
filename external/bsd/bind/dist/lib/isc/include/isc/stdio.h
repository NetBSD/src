/*	$NetBSD: stdio.h,v 1.5 2014/12/10 04:38:00 christos Exp $	*/

/*
 * Copyright (C) 2004-2007, 2013  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: stdio.h,v 1.13 2007/06/19 23:47:18 tbox Exp  */

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
