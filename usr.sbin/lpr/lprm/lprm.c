/*	$NetBSD: lprm.c,v 1.10 1999/12/07 14:54:48 mrg Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)lprm.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: lprm.c,v 1.10 1999/12/07 14:54:48 mrg Exp $");
#endif
#endif /* not lint */

/*
 * lprm - remove the current user's spool entry
 *
 * lprm [-] [[job #] [user] ...]
 *
 * Using information in the lock file, lprm will kill the
 * currently active daemon (if necessary), remove the associated files,
 * and startup a new daemon.  Privileged users may remove anyone's spool
 * entries, otherwise one can only remove their own.
 */

#include <sys/param.h>

#include <err.h>
#include <syslog.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lp.h"
#include "lp.local.h"

/*
 * Stuff for handling job specifications
 */
char	*person;		/* name of person doing lprm */
int	 requ[MAXREQUESTS];	/* job number of spool entries */
int	 requests;		/* # of spool requests */
char	*user[MAXUSERS];	/* users to process */
int	 users;			/* # of users in user array */
uid_t	 uid, euid;		/* real and effective user id's */

static char	luser[16];	/* buffer for person */

static void usage __P((void));
int main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *arg;
	struct passwd *p;

	uid = getuid();
	euid = geteuid();
	seteuid(uid);	/* be safe */
	name = argv[0];
	gethostname(host, sizeof(host));
	host[sizeof(host) - 1] = '\0';
	openlog("lpd", 0, LOG_LPR);
	if ((p = getpwuid(getuid())) == NULL)
		fatal("Who are you?");
	if (strlen(p->pw_name) >= sizeof(luser))
		fatal("Your name is too long");
	strncpy(luser, p->pw_name, sizeof(luser) - 1);
	luser[sizeof(luser) - 1] = '\0';
	person = luser;
	while (--argc) {
		if ((arg = *++argv)[0] == '-')
			switch (arg[1]) {
			case 'P':
				if (arg[2])
					printer = &arg[2];
				else if (argc > 1) {
					argc--;
					printer = *++argv;
				}
				break;
			case 'w':
				if (arg[2])
					wait_time = atoi(&arg[2]);
				else if (argc > 1) {
					argc--;
					wait_time = atoi(*++argv);
				}
				if (wait_time < 0)
					errx(1, "wait time must be postive: %s",
					    optarg);
				if (wait_time < 30)
				    warnx("warning: wait time less than 30 seconds");
				break;
			case '\0':
				if (!users) {
					users = -1;
					break;
				}
			default:
				usage();
			}
		else {
			if (users < 0)
				usage();
			if (isdigit(arg[0])) {
				if (requests >= MAXREQUESTS)
					fatal("Too many requests");
				requ[requests++] = atoi(arg);
			} else {
				if (users >= MAXUSERS)
					fatal("Too many users");
				user[users++] = arg;
			}
		}
	}
	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;

	rmjob();
	exit(0);
}

static void
usage()
{
	fprintf(stderr, "usage: lprm [-] [-Pprinter] [[job #] [user] ...]\n");
	exit(2);
}
