/*	$NetBSD: temp.c,v 1.22 2010/01/12 14:45:31 christos Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
static char sccsid[] = "@(#)temp.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: temp.c,v 1.22 2010/01/12 14:45:31 christos Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include <util.h>
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * Give names to all the temporary files that we will need.
 */

PUBLIC void
tinit(void)
{
	char pathbuf[MAXPATHLEN];
	const char *cp;
	char *p;

	/*
	 * It's okay to call savestr in here because main will
	 * do a spreserve() after us.
	 */
	if ((cp = getenv("TMPDIR")) == NULL || *cp == '\0')
		cp = _PATH_TMP;

	tmpdir = savestr(cp);

	/* Remove trailing slashes. */
	p = tmpdir + strlen(tmpdir) - 1;
	while (p > tmpdir && *p == '/') {
		*p = '\0';
		p--;
	}

	if (myname != NULL) {
		if (getuserid(myname) < 0)
			errx(EXIT_FAILURE, "`%s' is not a user of this system", myname);
	}
	else {
		if ((cp = username()) == NULL) {
			myname = savestr("nobody");
			if (mailmode == mm_receiving)
				errx(EXIT_FAILURE, "who am I receiving for?");
		} else
			myname = savestr(cp);
	}
	if ((cp = getenv("HOME")) == NULL)
		cp = ".";
	homedir = savestr(cp);

	if (getcwd(pathbuf, sizeof(pathbuf)) != NULL)
		origdir = savestr(pathbuf);
	else {
		warn("getcwd");
		origdir = savestr(".");
	}

	if (debug)
		(void)printf("user = %s, homedir = %s, origdir = %s\n",
		    myname, homedir, origdir);
}
