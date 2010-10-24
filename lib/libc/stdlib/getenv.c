/*	$NetBSD: getenv.c,v 1.26 2010/10/24 17:53:27 tron Exp $	*/

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getenv.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: getenv.c,v 1.26 2010/10/24 17:53:27 tron Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"

#ifdef _REENTRANT
rwlock_t __environ_lock = RWLOCK_INITIALIZER;
#endif
char **__environ_malloced;
static char **saveenv;
static size_t environ_malloced_len;

__weak_alias(getenv_r, _getenv_r)

/*
 * getenv --
 *	Returns ptr to value associated with name, if any, else NULL.
 *	XXX: we cannot use getenv_r to implement this, because getenv()
 *	cannot use a shared buffer, because if it did, subsequent calls
 *	to getenv would trash previous results.
 */
char *
getenv(const char *name)
{
	int offset;
	char *result;

	_DIAGASSERT(name != NULL);

	rwlock_rdlock(&__environ_lock);
	result = __findenv(name, &offset);
	rwlock_unlock(&__environ_lock);
	return result;
}

int
getenv_r(const char *name, char *buf, size_t len)
{
	int offset;
	char *result;
	int rv = -1;

	_DIAGASSERT(name != NULL);

	rwlock_rdlock(&__environ_lock);
	result = __findenv(name, &offset);
	if (result == NULL) {
		errno = ENOENT;
		goto out;
	}
	if (strlcpy(buf, result, len) >= len) {
		errno = ERANGE;
		goto out;
	}
	rv = 0;
out:
	rwlock_unlock(&__environ_lock);
	return rv;
}

int
__allocenv(int offset)
{
	char **p;
	size_t required_len, new_len;

	if (offset == -1 || saveenv != environ) {
		char **ptr;
		for (ptr = environ, offset = 0; *ptr != NULL; ptr++)
			offset++;
	}

	/* one for potentially new entry one for NULL */
	required_len = offset + 2;

	if (required_len <= environ_malloced_len && saveenv == environ)
		return 0;

	/* Make sure we at least double the size of the arrays. */
	new_len = (environ_malloced_len >= 16) ?
	    (environ_malloced_len << 1) : 16;
	while (new_len < required_len)
		new_len = new_len << 1;

	if (saveenv == environ) {		/* just increase size */
		if ((p = realloc(saveenv, new_len * sizeof(*p))) == NULL)
			return -1;
		(void)memset(&p[environ_malloced_len], 0,
		    (new_len - environ_malloced_len) * sizeof(*p));
		saveenv = p;
	} else {				/* get new space */
		free(saveenv);
		if ((saveenv = malloc(new_len * sizeof(*saveenv))) == NULL)
			return -1;
		(void)memcpy(saveenv, environ,
		    (required_len - 2) * sizeof(*saveenv));
		(void)memset(&saveenv[required_len - 2], 0,
		    (new_len - (required_len - 2)) * sizeof(*saveenv));
	}
	environ = saveenv;

	p = realloc(__environ_malloced, new_len * sizeof(*p));
	if (p == NULL)
		return -1;

	(void)memset(&p[environ_malloced_len], 0,
	    (new_len - environ_malloced_len) * sizeof(*p));
	environ_malloced_len = new_len;
	__environ_malloced = p;

	return 0;
}

/*
 * __findenv --
 *	Returns pointer to value associated with name, if any, else NULL.
 *	Sets offset to be the offset of the name/value combination in the
 *	environmental array, for use by setenv(3) and unsetenv(3).
 *	Explicitly removes '=' in argument name.
 *
 *	This routine *should* be a static; don't use it.
 */
char *
__findenv(const char *name, int *offset)
{
	size_t len;
	const char *np;
	char **p, *c;

	if (name == NULL || environ == NULL)
		return NULL;
	for (np = name; *np && *np != '='; ++np)
		continue;
	len = np - name;
	for (p = environ; (c = *p) != NULL; ++p)
		if (strncmp(c, name, len) == 0 && c[len] == '=') {
			*offset = (int)(p - environ);
			return c + len + 1;
		}
	*offset = (int)(p - environ);
	return NULL;
}
