/*	$NetBSD: putenv.c,v 1.13 2010/10/01 20:11:32 christos Exp $	*/

/*-
 * Copyright (c) 1988, 1993
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
static char sccsid[] = "@(#)putenv.c	8.2 (Berkeley) 3/27/94";
#else
__RCSID("$NetBSD: putenv.c,v 1.13 2010/10/01 20:11:32 christos Exp $");
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
__weak_alias(putenv,_putenv)
#endif

int
putenv(char *str)
{
	char *p, *equal;
	int offset;

	_DIAGASSERT(str != NULL);

	if ((equal = strchr(str, '=')) == NULL)
		return -1;

	if (rwlock_wrlock(&__environ_lock) != 0)
		return -1;

	*equal = '\0';
	p = __findenv(str, &offset);
	*equal = '=';

	if (__allocenv(offset) == -1)
		goto bad;

	if (p != NULL && environ[offset] == __environ_malloced[offset])
		free(__environ_malloced[offset]);

	environ[offset] = str;
	rwlock_unlock(&__environ_lock);
	return 0;
bad:
	rwlock_unlock(&__environ_lock);
	return -1;
}
