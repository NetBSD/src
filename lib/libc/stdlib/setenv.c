/*	$NetBSD: setenv.c,v 1.19.2.1 2001/08/08 16:27:44 nathanw Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)setenv.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: setenv.c,v 1.19.2.1 2001/08/08 16:27:44 nathanw Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "local.h"
#include "reentrant.h"

#ifdef __weak_alias
__weak_alias(setenv,_setenv)
__weak_alias(unsetenv,_unsetenv)
#endif

#ifdef _REENTRANT
extern rwlock_t __environ_lock;
#endif

extern char **environ;

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
int
setenv(name, value, rewrite)
	const char *name;
	const char *value;
	int rewrite;
{
	static int alloced;			/* if allocated space before */
	char *c;
	const char *cc;
	int l_value, offset;

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(value != NULL);

	if (*value == '=')			/* no `=' in value */
		++value;
	l_value = strlen(value);
	rwlock_wrlock(&__environ_lock);
	/* find if already exists */
	if ((c = __findenv(name, &offset)) != NULL) {
		if (!rewrite) {
			rwlock_unlock(&__environ_lock);
			return (0);
		}
		if (strlen(c) >= l_value) {	/* old larger; copy over */
			while ((*c++ = *value++) != '\0');
			rwlock_unlock(&__environ_lock);
			return (0);
		}
	} else {					/* create new slot */
		int cnt;
		char **p;

		for (p = environ, cnt = 0; *p; ++p, ++cnt);
		if (alloced) {			/* just increase size */
			environ = realloc(environ,
			    (size_t)(sizeof(char *) * (cnt + 2)));
			if (!environ) {
				rwlock_unlock(&__environ_lock);
				return (-1);
			}
		}
		else {				/* get new space */
			alloced = 1;		/* copy old entries into it */
			p = malloc((size_t)(sizeof(char *) * (cnt + 2)));
			if (!p) {
				rwlock_unlock(&__environ_lock);
				return (-1);
			}
			memcpy(p, environ, cnt * sizeof(char *));
			environ = p;
		}
		environ[cnt + 1] = NULL;
		offset = cnt;
	}
	for (cc = name; *cc && *cc != '='; ++cc)/* no `=' in name */
		continue;
	if (!(environ[offset] =			/* name + `=' + value */
	    malloc((size_t)((int)(cc - name) + l_value + 2)))) {
		rwlock_unlock(&__environ_lock);
		return (-1);
	}
	for (c = environ[offset]; (*c = *name++) && *c != '='; ++c);
	for (*c++ = '='; (*c++ = *value++) != '\0'; );
	rwlock_unlock(&__environ_lock);
	return (0);
}

/*
 * unsetenv(name) --
 *	Delete environmental variable "name".
 */
void
unsetenv(name)
	const char *name;
{
	char **p;
	int offset;

	_DIAGASSERT(name != NULL);

	rwlock_wrlock(&__environ_lock);
	while (__findenv(name, &offset))	/* if set multiple times */
		for (p = &environ[offset];; ++p)
			if (!(*p = *(p + 1)))
				break;
	rwlock_unlock(&__environ_lock);
}
