/*	$NetBSD: startdaemon.c,v 1.13 2003/08/07 11:25:25 agc Exp $	*/

/*
 * Copyright (c) 1983, 1993, 1994
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
#ifndef lint
#if 0
static char sccsid[] = "@(#)startdaemon.c	8.2 (Berkeley) 4/17/94";
#else
__RCSID("$NetBSD: startdaemon.c,v 1.13 2003/08/07 11:25:25 agc Exp $");
#endif
#endif /* not lint */


#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include "lp.h"
#include "pathnames.h"

extern uid_t	uid, euid;

/*
 * Tell the printer daemon that there are new files in the spool directory.
 */

int
startdaemon(char *printer)
{
	struct sockaddr_un un;
	int s;
	size_t n;
	char buf[BUFSIZ];

	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s < 0) {
		warn("socket");
		return(0);
	}
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_LOCAL;
	strncpy(un.sun_path, _PATH_SOCKETNAME, sizeof(un.sun_path) - 1);
#ifndef SUN_LEN
#define SUN_LEN(unp) (strlen((unp)->sun_path) + 2)
#endif
	seteuid(euid);
	if (connect(s, (const struct sockaddr *)&un,
	    (int)SUN_LEN(&un)) < 0) {
		seteuid(uid);
		warn("connect");
		(void)close(s);
		return(0);
	}
	seteuid(uid);
	n = snprintf(buf, sizeof(buf), "\1%s\n", printer);
	if (write(s, buf, n) != n) {
		warn("write");
		(void)close(s);
		return(0);
	}
	if (read(s, buf, 1) == 1) {
		if (buf[0] == '\0') {		/* everything is OK */
			(void)close(s);
			return(1);
		}
		putchar(buf[0]);
	}
	while ((n = read(s, buf, sizeof(buf))) > 0)
		fwrite(buf, 1, n, stdout);
	(void)close(s);
	return(0);
}
