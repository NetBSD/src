/*	$NetBSD: textdomain.c,v 1.1.1.1 2000/10/31 10:45:04 itojun Exp $	*/

/*-
 * Copyright (c) 2000 Citrus Project,
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: textdomain.c,v 1.1.1.1 2000/10/31 10:45:04 itojun Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/param.h>

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include "libintl_local.h"
#include "pathnames.h"

static char dpath[PATH_MAX];
static char dname[PATH_MAX];	/*XXX*/
const char *__domainpath = _PATH_TEXTDOMAIN;
const char *__domainname;

/*
 * set the default domainname for dcngettext() and friends.
 */
char *
textdomain(domainname)
	const char *domainname;
{

	return bindtextdomain(domainname, _PATH_TEXTDOMAIN);
}

char *
bindtextdomain(domainname, dirname)
	const char *domainname;
	const char *dirname;
{

	if (strlen(dirname) + 1 > sizeof(dpath))
		return NULL;
	/* disallow relative path */
	if (dirname[0] != '/')
		return NULL;

	if (strlen(domainname) + 1 > sizeof(dname))
		return NULL;

	strlcpy(dpath, dirname, sizeof(dpath));
	__domainpath = dpath;
	strlcpy(dname, domainname, sizeof(dname));
	__domainname = dname;

	/* LINTED const cast */
	return (char *)domainname;
}

char *
bind_textdomain_codeset(domainname, codeset)
	const char *domainname;
	const char *codeset;
{

	/* yet to be done - always fail */
	return NULL;
}
