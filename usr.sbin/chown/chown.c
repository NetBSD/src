/*	$NetBSD: chown.c,v 1.19.6.1 1999/12/27 18:37:35 wrstuden Exp $	*/

/*
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)chown.c	8.8 (Berkeley) 4/4/94";
#else
__RCSID("$NetBSD: chown.c,v 1.19.6.1 1999/12/27 18:37:35 wrstuden Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <locale.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void	a_gid __P((const char *));
static void	a_uid __P((const char *));
static id_t	id __P((const char *, const char *));
	int	main __P((int, char **));
static void	usage __P((void));

static uid_t uid;
static gid_t gid;
static int Rflag, ischown, fflag;
static char *myname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FTS *ftsp;
	FTSENT *p;
	int Hflag, Lflag, ch, fts_options, hflag, rval;
	char *cp;
	int (*change_owner) __P((const char *, uid_t, gid_t));
	
	(void)setlocale(LC_ALL, "");

	myname = (cp = strrchr(*argv, '/')) ? cp + 1 : *argv;
	ischown = (myname[2] == 'o');
	
	Hflag = Lflag = hflag = 0;
	while ((ch = getopt(argc, argv, "HLPRfh")) != -1)
		switch (ch) {
		case 'H':
			Hflag = 1;
			Lflag = 0;
			break;
		case 'L':
			Lflag = 1;
			Hflag = 0;
			break;
		case 'P':
			Hflag = Lflag = 0;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'h':
			/*
			 * In System V the -h option causes chown/chgrp to
			 * change the owner/group of the symbolic link.
			 * 4.4BSD's symbolic links didn't have owners/groups,
			 * so it was an undocumented noop.
			 * In NetBSD 1.3, lchown(2) is introduced.
			 */
			hflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc < 2)
		usage();

	fts_options = FTS_PHYSICAL;
	if (Rflag) {
		if (Hflag)
			fts_options |= FTS_COMFOLLOW;
		if (Lflag) {
			if (hflag)
				errx(EXIT_FAILURE, "the -L and -h options may not be specified together.");
			fts_options &= ~FTS_PHYSICAL;
			fts_options |= FTS_LOGICAL;
		}
	}
	if (hflag)
		change_owner = lchown;
	else
		change_owner = chown;

	uid = (uid_t)-1;
	gid = (gid_t)-1;
	if (ischown) {
#ifdef SUPPORT_DOT
		if ((cp = strchr(*argv, '.')) != NULL) {
			*cp++ = '\0';
			a_gid(cp);
		} else
#endif
		if ((cp = strchr(*argv, ':')) != NULL) {
			*cp++ = '\0';
			a_gid(cp);
		} 
		a_uid(*argv);
	} else 
		a_gid(*argv);

	if ((ftsp = fts_open(++argv, fts_options, NULL)) == NULL)
		err(EXIT_FAILURE, NULL);

	for (rval = EXIT_SUCCESS; (p = fts_read(ftsp)) != NULL;) {
		switch (p->fts_info) {
		case FTS_D:
			if (!Rflag)		/* Change it at FTS_DP. */
				fts_set(ftsp, p, FTS_SKIP);
			continue;
		case FTS_DNR:			/* Warn, chown, continue. */
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			rval = EXIT_FAILURE;
			break;
		case FTS_ERR:			/* Warn, continue. */
		case FTS_NS:
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			rval = EXIT_FAILURE;
			continue;
		case FTS_SL:			/* Ignore. */
		case FTS_SLNONE:
			/*
			 * The only symlinks that end up here are ones that
			 * don't point to anything and ones that we found
			 * doing a physical walk.
			 */
			if (!hflag)
				continue;
			/* else */
			/* FALLTHROUGH */
		default:
			break;
		}

		if ((*change_owner)(p->fts_accpath, uid, gid) && !fflag) {
			warn("%s", p->fts_path);
			rval = EXIT_FAILURE;
		}
	}
	if (errno)
		err(EXIT_FAILURE, "fts_read");
	exit(rval);
	/* NOTREACHED */
}

static void
a_gid(s)
	const char *s;
{
	struct group *gr;

	if (*s == '\0')			/* Argument was "uid[:.]". */
		return;
	gid = ((gr = getgrnam(s)) == NULL) ? id(s, "group") : gr->gr_gid;
}

static void
a_uid(s)
	const char *s;
{
	struct passwd *pw;

	if (*s == '\0')			/* Argument was "[:.]gid". */
		return;
	uid = ((pw = getpwnam(s)) == NULL) ? id(s, "user") : pw->pw_uid;
}

static id_t
id(name, type)
	const char *name, *type;
{
	id_t val;
	char *ep;

	errno = 0;
	val = (id_t)strtoul(name, &ep, 10);
	if (errno)
		err(EXIT_FAILURE, "%s", name);
	if (*ep != '\0')
		errx(EXIT_FAILURE, "%s: invalid %s name", name, type);
	return (val);
}

static void
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-R [-H | -L | -P]] [-fh] %s file ...\n",
	    myname, ischown ? "[owner][:group]" : "group");
	exit(EXIT_FAILURE);
}
