/*	$NetBSD: textdomain.c,v 1.4 2000/11/03 14:29:23 itojun Exp $	*/

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
__RCSID("$NetBSD: textdomain.c,v 1.4 2000/11/03 14:29:23 itojun Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/param.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>
#include "libintl_local.h"
#include "pathnames.h"

struct domainbinding __binding = { NULL, DEFAULT_DOMAINNAME, _PATH_TEXTDOMAIN };

/*
 * set the default domainname for dcngettext() and friends.
 */
char *
textdomain(domainname)
	const char *domainname;
{

	/* NULL pointer gives the current setting */
	if (!domainname)
		return __binding.domainname;

	/* empty string sets the value back to the default */
	if (!*domainname) {
		strlcpy(__binding.domainname, DEFAULT_DOMAINNAME,
		    sizeof(__binding.domainname));
	} else {
		strlcpy(__binding.domainname, domainname,
		    sizeof(__binding.domainname));
	}
	return __binding.domainname;
}

char *
bindtextdomain(domainname, dirname)
	const char *domainname;
	const char *dirname;
{
	struct domainbinding *p;

	/* NULL pointer or empty string returns NULL with no operation */
	if (!domainname || !*domainname)
		return NULL;

	if (strlen(dirname) + 1 > sizeof(p->path))
		return NULL;
	/* disallow relative path */
	if (dirname[0] != '/')
		return NULL;

	if (strlen(domainname) + 1 > sizeof(p->domainname))
		return NULL;

	/* lookup binding for the domainname */
	for (p = __binding.next; p; p = p->next)
		if (strcmp(p->domainname, domainname) == 0)
			break;
	if (!p) {
		p = (struct domainbinding *)malloc(sizeof(*p));
		if (!p)
			return NULL;
		memset(p, 0, sizeof(*p));
		p->next = __binding.next;
		__binding.next = p;
	}

	strlcpy(p->path, dirname, sizeof(p->path));
	strlcpy(p->domainname, domainname, sizeof(p->domainname));

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
