/* $NetBSD: findcc.c,v 1.10 2021/08/20 06:36:10 rillig Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1999 The NetBSD Foundation, Inc.\
 All rights reserved.");
__RCSID("$NetBSD: findcc.c,v 1.10 2021/08/20 06:36:10 rillig Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "findcc.h"

char *
findcc(const char *progname)
{
	char *cc;
	const char *path, *dir;
	char buffer[MAXPATHLEN];
	size_t progname_len, dir_len;

	progname_len = strcspn(progname, " ");

	if (memchr(progname, '/', progname_len) != NULL) {
		if ((cc = strndup(progname, progname_len)) == NULL)
			return NULL;
		if (access(cc, X_OK) == 0)
			return cc;
		free(cc);
		return NULL;
	}

	if ((path = getenv("PATH")) == NULL)
		return NULL;

	for (dir = path; *dir != '\0'; ) {
		dir_len = strcspn(dir, ":");

		if ((size_t)snprintf(buffer, sizeof(buffer), "%.*s/%.*s",
			(int)dir_len, dir, (int)progname_len, progname)
		    < sizeof(buffer)
		    && access(buffer, X_OK) == 0)
			return strdup(buffer);

		dir += dir_len;
		if (*dir == ':')
			dir++;
	}

	return NULL;
}
