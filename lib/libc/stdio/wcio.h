/* $NetBSD: wcio.h,v 1.1 2001/12/07 12:02:07 yamt Exp $ */

/*-
 * Copyright (c)2001 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Citrus$
 */

#ifndef _WCIO_H_
#define _WCIO_H_

#include <wchar.h> /* for mbstate_t and wchar_t */

/* minimal requirement of SUSv2 */
#define WCIO_UNGETWC_BUFSIZE 1

struct wchar_io_data {
	mbstate_t wcio_mbstate_in;
	mbstate_t wcio_mbstate_out;

	wchar_t wcio_ungetwc_buf[WCIO_UNGETWC_BUFSIZE];
	size_t wcio_ungetwc_inbuf;

	int wcio_mode; /* orientation */
};

#if 0
/*
 * XXX we use dynamically allocated storage
 * instead of changing sizeof(FILE) for now.
 */
/* #define _FP_WCIO_HACK */
#endif

#define _SET_ORIENTATION(fp, mode) \
do {\
	struct wchar_io_data *_wcio = WCIO_GET(fp);\
	if (_wcio && _wcio->wcio_mode == 0)\
		_wcio->wcio_mode = (mode);\
} while (/*CONSTCOND*/0)

/*
 * WCIO_FREE should be called by fclose
 */
#ifndef _FP_WCIO_HACK
#define WCIO_GET(fp) (&(_EXT(fp)->_wcio))
#define WCIO_FREE(fp) \
do {\
	_EXT(fp)->_wcio.wcio_mode = 0;\
	WCIO_FREEUB(fp);\
} while (/*CONSTCOND*/0)
#define WCIO_FREEUB(fp) \
do {\
	_EXT(fp)->_wcio.wcio_ungetwc_inbuf = 0;\
} while (/*CONSTCOND*/0)
#else /* _FP_WCIO_HACK */
#define WCIO_GET(fp) _wcio_get(fp)
#define WCIO_FREE(fp) _wcio_free(fp)
#if 0 /* unused */
#define WCIO_FREEUB(fp) _wcio_clear_ungetwc(fp)
#endif
struct wchar_io_data *_wcio_get(FILE *);
void _wcio_free(FILE *fp);
#if 0 /* unused */
void _wcio_clear_ungetwc(FILE *fp)
#endif
#endif /* _FP_WCIO_HACK */

#endif /*_WCIO_H_*/
