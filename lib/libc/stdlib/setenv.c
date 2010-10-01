/*	$NetBSD: setenv.c,v 1.39 2010/10/01 20:11:32 christos Exp $	*/

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
static char sccsid[] = "@(#)setenv.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: setenv.c,v 1.39 2010/10/01 20:11:32 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"

#ifdef __weak_alias
__weak_alias(setenv,_setenv)
#endif

/*
 * setenv --
 *	Set the value of the environmental variable "name" to be
 *	"value".  If rewrite is set, replace any current value.
 */
int
setenv(const char *name, const char *value, int rewrite)
{
	char *c;
	const char *cc;
	size_t l_value, size;
	int offset;

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(value != NULL);

	if (rwlock_wrlock(&__environ_lock) != 0)
		return -1;

	/* find if already exists */
	c = __findenv(name, &offset);

	if (__allocenv(offset) == -1)
		goto bad;

	if (*value == '=')			/* no `=' in value */
		++value;
	l_value = strlen(value);

	if (c != NULL) {
		if (!rewrite)
			goto good;
		if (strlen(c) >= l_value)	/* old is enough; copy over */
			goto copy;
	}
	for (cc = name; *cc && *cc != '='; ++cc)	/* no `=' in name */
		continue;
	size = cc - name;
	/* name + `=' + value */
	if ((c = malloc(size + l_value + 2)) == NULL)
		goto bad;

	if (environ[offset] == __environ_malloced[offset])
		free(__environ_malloced[offset]);

	environ[offset] = c;
	__environ_malloced[offset] = c;
	(void)memcpy(c, name, size);
	c += size;
	*c++ = '=';

copy:
	(void)memcpy(c, value, l_value + 1);
good:
	rwlock_unlock(&__environ_lock);
	return 0;
bad:
	rwlock_unlock(&__environ_lock);
	return -1;
}
