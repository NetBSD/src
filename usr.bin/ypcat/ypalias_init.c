/*	$NetBSD: ypalias_init.c,v 1.1 2009/06/20 19:27:26 christos Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Code contributed by Greywolf <greywolf@starwolf.com>
 * 28 August 2003.  All rights released to The NetBSD Foundation.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypalias_init.c,v 1.1 2009/06/20 19:27:26 christos Exp $");
#endif

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <errno.h>

#include "ypalias_init.h"

#ifndef _PATH_YPNICKNAMES
#define _PATH_YPNICKNAMES   "/var/yp/nicknames"
#endif

const struct ypalias def_ypaliases[] = {
	{ "passwd", "passwd.byname" },
	{ "group", "group.byname" },
	{ "networks", "networks.byaddr" },
	{ "hosts", "hosts.byaddr" },
	{ "protocols", "protocols.bynumber" },
	{ "services", "services.byname" },
	{ "aliases", "mail.aliases" },
	{ "ethers", "ethers.byname" },
	{ NULL, NULL },
};

const struct ypalias *
ypalias_init(void)
{
	FILE *fp;
	char *cp, *line;
	struct ypalias *ypa, *nypa;
	size_t nypalias = 50;
	size_t i, len, lineno;

	if ((fp = fopen(_PATH_YPNICKNAMES, "r")) == NULL)
		return &def_ypaliases[0];

	if ((ypa = calloc(sizeof(*ypa), nypalias)) == NULL)
		goto out;

	lineno = 1;
	for (i = 0; (line = fparseln(fp, &len, &lineno, NULL,
	    FPARSELN_UNESCALL));) {
		cp = line;
		/* Ignore malformed lines */
		if ((ypa[i].alias = strsep(&line, " \t\n")) == NULL ||
		    (ypa[i].name = strsep(&line, " \t\n")) == NULL ||
		    ypa[i].alias != cp) {
			warnx("%s, %zu: syntax error, ignored",
			    _PATH_YPNICKNAMES, lineno);
			free(cp);
		} else
			i++;
		if (i == nypalias) {
			nypalias <<= 1;
			nypa = realloc(ypa, sizeof(*ypa) * nypalias);
			if (nypa == NULL)
				goto out;
			ypa = nypa;
		}
	}
	ypa[i].alias = ypa[i].name = NULL;
	i++;

	(void)fclose(fp);
	if ((nypa = realloc(ypa, sizeof(*ypa) * i)) != NULL)
		return nypa;
out:
	warn("Cannot alllocate alias space, returning default list");
	if (ypa) {
		do
			free(__UNCONST(ypa[--i].alias));
		while (i != 0);
		free(ypa);
	}
	(void)fclose(fp);
	return def_ypaliases;
	return ypa;
}
