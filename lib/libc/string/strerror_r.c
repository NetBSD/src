/*	$NetBSD: strerror_r.c,v 1.5.6.1 2024/10/13 16:08:53 martin Exp $	*/

/*
 * Copyright (c) 1988 Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: strerror_r.c,v 1.5.6.1 2024/10/13 16:08:53 martin Exp $");

#include "namespace.h"
#include <assert.h>
#include <atomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>	/* for sys_nerr on FreeBSD */
#ifdef NLS
#include <limits.h>
#include <nl_types.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include "setlocale_local.h"
#endif

#include "extern.h"

#define	UPREFIX	"Unknown error: %d"

__weak_alias(strerror_r, _strerror_r)

#ifdef NLS
static void
load_errlist(locale_t loc)
{
	const char **errlist;
	char *errlist_prefix;
	int i;
	nl_catd catd;
	catd = catopen_l("libc", NL_CAT_LOCALE, loc);

	if (loc->cache->errlist_prefix == NULL) {
		errlist_prefix = strdup(catgets(catd, 1, 0xffff, UPREFIX));
		if (errlist_prefix == NULL)
			goto cleanup2;

		membar_release();
		if (atomic_cas_ptr(__UNCONST(&loc->cache->errlist_prefix),
				   NULL, errlist_prefix) != NULL)
			free(errlist_prefix);
	}

	if (loc->cache->errlist)
		goto cleanup2;

	errlist = calloc(sys_nerr, sizeof(*errlist));
	if (errlist == NULL)
		goto cleanup2;
	for (i = 0; i < sys_nerr; ++i) {
		errlist[i] = strdup(catgets(catd, 1, i, sys_errlist[i]));
		if (errlist[i] == NULL)
			goto cleanup;
	}
	membar_release();
	if (atomic_cas_ptr(__UNCONST(&loc->cache->errlist), NULL, errlist) != NULL)
		goto cleanup;
	goto cleanup2;

  cleanup:
	for (i = 0; i < sys_nerr; ++i)
		free(__UNCONST(errlist[i]));
	free(errlist);
  cleanup2:
	catclose(catd);
}
#endif

int
_strerror_lr(int num, char *buf, size_t buflen, locale_t loc)
{
	unsigned int errnum = num;
	int retval = 0;
	size_t slen;
	int saved_errno = errno;
#ifdef NLS
	const char * const *errlist;
	const char *errlist_prefix;
#endif

	_DIAGASSERT(buf != NULL);

	if (errnum < (unsigned int) sys_nerr) {
#ifdef NLS
		errlist = *loc->cache->errlistp;
		membar_datadep_consumer();
		if (errlist == NULL) {
			load_errlist(loc);
			errlist = *loc->cache->errlistp;
			membar_datadep_consumer();
			if (errlist == NULL)
				errlist = *LC_C_LOCALE->cache->errlistp;
		}
		slen = strlcpy(buf, errlist[errnum], buflen);
#else
		slen = strlcpy(buf, sys_errlist[errnum], buflen);
#endif
	} else {
#ifdef NLS
		errlist_prefix = loc->cache->errlist_prefix;
		membar_datadep_consumer();
		if (errlist_prefix == NULL) {
			load_errlist(loc);
			errlist_prefix = loc->cache->errlist_prefix;
			membar_datadep_consumer();
			if (errlist_prefix == NULL)
				errlist_prefix = LC_C_LOCALE->cache->errlist_prefix;
		}
		slen = snprintf_l(buf, buflen, loc, errlist_prefix, num);
#else
		slen = snprintf(buf, buflen, UPREFIX, num);
#endif
		retval = EINVAL;
	}

	if (slen >= buflen)
		retval = ERANGE;

	errno = saved_errno;

	return retval;
}

int
strerror_r(int num, char *buf, size_t buflen)
{
#ifdef NLS
	return _strerror_lr(num, buf, buflen, _current_locale());
#else
	return _strerror_lr(num, buf, buflen, NULL);
#endif
}
