/*	$NetBSD: main.c,v 1.26 2010/08/06 09:14:40 dholland Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: main.c,v 1.26 2010/08/06 09:14:40 dholland Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <pwd.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "extern.h"
#include "pathnames.h"
#include "restart.h"

static void
initialize(void)
{
	int fd;
	const char *name;
	struct passwd *pw;
	char *s;

	gid = getgid();
	egid = getegid();
	setegid(gid);

	fd = open("/dev/null", O_RDONLY);
	if (fd < 3)
		exit(1);
	close(fd);

	if (chdir(_PATH_SAILDIR) < 0) {
		err(1, "%s", _PATH_SAILDIR);
	}

	srandom((u_long)time(NULL));

	name = getenv("SAILNAME");
	if (name != NULL && *name != '\0') {
		strlcpy(myname, name, sizeof(myname));
	} else {
		pw = getpwuid(getuid());
		if (pw != NULL) {
			strlcpy(myname, pw->pw_gecos, sizeof(myname));
			/* trim to just the realname */
			s = strchr(myname, ',');
			if (s != NULL) {
				*s = '\0';
			}
			/* use just the first name */
			s = strchr(myname, ' ');
			if (s != NULL) {
				*s = '\0';
			}
			/* should really do &-expansion properly */
			if (!strcmp(myname, "&")) {
				strlcpy(myname, pw->pw_name, sizeof(myname));
				myname[0] = toupper((unsigned char)myname[0]);
			}
		}
	}
	if (*myname == '\0') {
		strlcpy(myname, "Anonymous", sizeof(myname));
	}
}

int
main(int argc, char **argv)
{
	char *p;
	int a, i;

	initialize();

	if ((p = strrchr(*argv, '/')) != NULL)
		p++;
	else
		p = *argv;

	if (strcmp(p, "driver") == 0 || strcmp(p, "saildriver") == 0)
		mode = MODE_DRIVER;
	else if (strcmp(p, "sail.log") == 0)
		mode = MODE_LOGGER;
	else
		mode = MODE_PLAYER;

	while ((a = getopt(argc, argv, "dsxlb")) != -1)
		switch (a) {
		case 'd':
			mode = MODE_DRIVER;
			break;
		case 's':
			mode = MODE_LOGGER;
			break;
		case 'x':
			randomize = true;
			break;
		case 'l':
			longfmt = true;
			break;
		case 'b':
			nobells = true;
			break;
		default:
			errx(1, "Usage: %s [-bdlsx] [scenario-number]", p);
		}

	argc -= optind;
	argv += optind;

	if (*argv)
		game = atoi(*argv);
	else
		game = -1;

	if ((i = setjmp(restart)) != 0)
		mode = i;

	switch (mode) {
	case MODE_PLAYER:
		initscreen();
		startup();
		cleanupscreen();
		return 0;
	case MODE_DRIVER:
		return dr_main();
	case MODE_LOGGER:
		return lo_main();
	default:
		warnx("Unknown mode %d", mode);
		abort();
	}
	/*NOTREACHED*/
}
