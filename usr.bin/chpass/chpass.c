/*	$NetBSD: chpass.c,v 1.13 1997/07/25 06:40:32 mikel Exp $	*/

/*-
 * Copyright (c) 1988, 1993, 1994
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)chpass.c	8.4 (Berkeley) 4/2/94";
#else 
static char rcsid[] = "$NetBSD: chpass.c,v 1.13 1997/07/25 06:40:32 mikel Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "chpass.h"
#include "pathnames.h"

extern	char *__progname;		/* from crt0.o */

char *tempname;
uid_t uid;
int use_yp;
int yflag;

void	(*Pw_error) __P((const char *, int, int));

#ifdef	YP
extern struct passwd *ypgetpwnam(), *ypgetpwuid();
extern	int _yp_check __P((char **));	/* buried deep inside libc */
#endif

void	baduser __P((void));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	enum { NEWSH, LOADENTRY, EDITENTRY } op;
	struct passwd *pw, lpw, old_pw;
	int ch, pfd, tfd, dfd;
	char *arg, *username = NULL, tempname[] = "/etc/pw.XXXXXX";

#ifdef __GNUC__
	pw = NULL;		/* XXX gcc -Wuninitialized */
	arg = NULL;
#endif
#ifdef	YP
	use_yp = _yp_check(NULL);
#endif

	op = EDITENTRY;
	while ((ch = getopt(argc, argv, "a:s:ly")) != -1)
		switch(ch) {
		case 'a':
			op = LOADENTRY;
			arg = optarg;
			break;
		case 's':
			op = NEWSH;
			arg = optarg;
			break;
		case 'l':
			use_yp = 0;
			break;
		case 'y':
#ifdef	YP
			if (!use_yp)
				errx(1, "YP not in use.");
			yflag = 1;
#else
			errx(1, "YP support not compiled in.");
#endif
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	uid = getuid();
	switch (argc) {
	case 0:
		/* nothing */
		break;

	case 1:
		username = argv[0];
		break;

	default:
		usage();
	}

#ifdef YP
	/*
	 * We need to determine if we _really_ want to use YP.
	 * If we defaulted to YP (i.e. were not given the -y flag),
	 * and the master is not running rpc.yppasswdd, we check
	 * to see if the user exists in the local passwd database.
	 * If so, we use it, otherwise we error out.
	 */
	if (use_yp && yflag == 0) {
		if (check_yppasswdd()) {
			/*
			 * We weren't able to contact rpc.yppasswdd.
			 * Check to see if we're in the local
			 * password database.  If we are, use it.
			 */
			if (username != NULL)
				pw = getpwnam(username);
			else
				pw = getpwuid(uid);
			if (pw != NULL)
				use_yp = 0;
			else {
				errx(1, "master YP server not running yppasswd daemon.\n\t%s\n",
				    "Can't change password.");
			}
		}
	}
#endif

#ifdef YP
	if (use_yp)
		Pw_error = yppw_error;
	else
#endif
		Pw_error = pw_error;

#ifdef	YP
	if (op == LOADENTRY && use_yp)
		errx(1, "cannot load entry using YP.\n\tUse the -l flag to load local.");
#endif

	if (op == EDITENTRY || op == NEWSH) {
		if (username != NULL) {
#ifdef YP
			if (use_yp)
				pw = ypgetpwnam(username);
			else
#endif /* YP */
				pw = getpwnam(username);
			if (pw == NULL)
				errx(1, "unknown user: %s", username);
			if (uid && uid != pw->pw_uid)
				baduser();
		} else {
#ifdef YP
			if (use_yp)
				pw = ypgetpwuid(uid);
			else
#endif /* YP */
				pw = getpwuid(uid);
			if (pw == NULL)
				errx(1, "unknown user: uid %u\n", uid);
		}
	}

	if (op == NEWSH) {
		/* protect p_shell -- it thinks NULL is /bin/sh */
		if (!arg[0])
			usage();
		if (p_shell(arg, pw, (ENTRY *)NULL))
				(*Pw_error)((char *)NULL, 0, 1);
	}

	if (op == LOADENTRY) {
		if (uid)
			baduser();
		pw = &lpw;
		if (!pw_scan(arg, pw, (int *)NULL))
			exit(1);
	}

	/* Make a copy for later verification */
	old_pw = *pw;
	old_pw.pw_gecos = strdup(old_pw.pw_gecos);

	/* Edit the user passwd information if requested. */
	if (op == EDITENTRY) {
		dfd = mkstemp(tempname);
		if (dfd < 0) {
				(*Pw_error)(tempname, 1, 1);
		}
		display(tempname, dfd, pw);
		edit(tempname, pw);
		(void)unlink(tempname);
	}

#ifdef	YP
	if (use_yp) {
		if (pw_yp(pw, uid))
			yppw_error((char *)NULL, 0, 1);
		else
			exit(0);
		/* Will not exit from this if. */
	}
#endif	/* YP */


	/*
	 * Get the passwd lock file and open the passwd file for
	 * reading.
	 */
	pw_init();
	tfd = pw_lock(0);
	if (tfd < 0) {
		warnx ("The passwd file is busy, waiting...");
		tfd = pw_lock(10);
		if (tfd < 0)
			errx(1, "The passwd file is still busy, "
			     "try again later.");
	}

	pfd = open(_PATH_MASTERPASSWD, O_RDONLY, 0);
	if (pfd < 0)
		pw_error(_PATH_MASTERPASSWD, 1, 1);

	/* Copy the passwd file to the lock file, updating pw. */
	pw_copy(pfd, tfd, pw, &old_pw);

	/* Now finish the passwd file update. */
	if (pw_mkdb() < 0)
		pw_error((char *)NULL, 0, 1);

	exit(0);
}

void
baduser()
{

	errx(1, "%s", strerror(EACCES));
}

void
usage()
{

#ifdef	YP
	(void)fprintf(stderr, "usage: chpass [-a list] [-s shell] [-l]%s [user]\n", use_yp?" [-y]":"");
#else
	(void)fprintf(stderr, "usage: chpass [-a list] [-s shell] [user]\n");
#endif
	exit(1);
}
