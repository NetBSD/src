/*	$NetBSD: pw_scan.c,v 1.10.10.1 2002/03/08 21:35:14 nathanw Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994, 1995
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

#if HAVE_CONFIG_H
#include "config.h"
#include "compat_pwd.h"
#else
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: pw_scan.c,v 1.10.10.1 2002/03/08 21:35:14 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(_LIBC)
#include "namespace.h"
#endif
#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _LIBC
#include "pw_private.h"
#endif
#endif /* !HAVE_CONFIG_H */

int
#ifdef _LIBC
__pw_scan(bp, pw, flags)
#else
pw_scan(bp, pw, flags)
#endif
	char *bp;
	struct passwd *pw;
	int *flags;
{
	unsigned long id;
	int root, inflags;
	char *ep;
	const char *p, *sh;

	_DIAGASSERT(bp != NULL);
	_DIAGASSERT(pw != NULL);

	inflags = 0;
	if (flags != (int *)NULL) {
		inflags = *flags;
		*flags = 0;
	}

	if (!(pw->pw_name = strsep(&bp, ":")))		/* login */
		goto fmt;
	root = !strcmp(pw->pw_name, "root");

	if (!(pw->pw_passwd = strsep(&bp, ":")))	/* passwd */
		goto fmt;

	if (!(p = strsep(&bp, ":")))			/* uid */
		goto fmt;
	id = strtoul(p, &ep, 10);
	if (root && id) {
		if (!(inflags & _PASSWORD_NOWARN))
			warnx("root uid should be 0");
		return (0);
	}
	if (id > UID_MAX || *ep != '\0') {
		if (!(inflags & _PASSWORD_NOWARN))
			warnx("invalid uid '%s'", p);
		return (0);
	}
	pw->pw_uid = (uid_t)id;
	if ((*p == '\0') && (flags != (int *)NULL))
		*flags |= _PASSWORD_NOUID;

	if (!(p = strsep(&bp, ":")))			/* gid */
		goto fmt;
	id = strtoul(p, &ep, 10);
	if (id > GID_MAX || *ep != '\0') {
		if (!(inflags & _PASSWORD_NOWARN))
			warnx("invalid gid '%s'", p);
		return (0);
	}
	pw->pw_gid = (gid_t)id;
	if ((*p == '\0') && (flags != (int *)NULL))
		*flags |= _PASSWORD_NOGID;

	if (inflags & _PASSWORD_OLDFMT) {
		pw->pw_class = "";
		pw->pw_change = 0;
		pw->pw_expire = 0;
		*flags |= (_PASSWORD_NOCHG | _PASSWORD_NOEXP);
	} else {
		pw->pw_class = strsep(&bp, ":");	/* class */
		if (!(p = strsep(&bp, ":")))		/* change */
			goto fmt;
		pw->pw_change = atol(p);
		if ((*p == '\0') && (flags != (int *)NULL))
			*flags |= _PASSWORD_NOCHG;
		if (!(p = strsep(&bp, ":")))		/* expire */
			goto fmt;
		pw->pw_expire = atol(p);
		if ((*p == '\0') && (flags != (int *)NULL))
			*flags |= _PASSWORD_NOEXP;
	}
	pw->pw_gecos = strsep(&bp, ":");		/* gecos */
	pw->pw_dir = strsep(&bp, ":");			/* directory */
	if (!(pw->pw_shell = strsep(&bp, ":")))		/* shell */
		goto fmt;

#if !HAVE_CONFIG_H
	p = pw->pw_shell;
	if (root && *p)					/* empty == /bin/sh */
		for (setusershell();;) {
			if (!(sh = getusershell())) {
				if (!(inflags & _PASSWORD_NOWARN))
					warnx("warning, unknown root shell");
				break;
			}
			if (!strcmp(p, sh))
				break;	
		}
#endif

	if ((p = strsep(&bp, ":")) != NULL) {			/* too many */
fmt:		
		if (!(inflags & _PASSWORD_NOWARN))
			warnx("corrupted entry");
		return (0);
	}

	return (1);
}
