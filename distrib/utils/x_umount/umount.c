/*	$NetBSD: umount.c,v 1.2 2003/08/07 09:28:02 agc Exp $	*/

/*-
 * Copyright (c) 1980, 1989, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)umount.c	8.8 (Berkeley) 5/8/95";
#else
__RCSID("$NetBSD: umount.c,v 1.2 2003/08/07 09:28:02 agc Exp $");
#endif
#endif /* not lint */

/*
 * Scaled-down version of umount(8) that supports unmounting
 * local filesystems only.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/time.h>

#include <err.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { MNTANY, MNTON, MNTFROM } mntwhat;

int	fflag, raw;

char	*getmntname __P((char *, mntwhat, char **));
int	 main __P((int, char *[]));
int	 umountfs __P((char *, char **));
void	 usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, errs;
	char **typelist = NULL;

	/* Start disks transferring immediately. */
	sync();

	while ((ch = getopt(argc, argv, "fR")) != -1)
		switch (ch) {
		case 'f':
			fflag = MNT_FORCE;
			break;
		case 'R':
			raw = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	for (errs = 0; *argv != NULL; ++argv)
		if (umountfs(*argv, typelist) != 0)
			errs = 1;
	exit(errs);
}

int
umountfs(name, typelist)
	char *name;
	char **typelist;
{
	struct stat sb;
	char *type, *delimp, *mntpt, rname[MAXPATHLEN];
	mntwhat what;

	delimp = NULL;

	if (raw) {
		mntpt = name;
	} else {

		if (realpath(name, rname) == NULL) {
			warn("%s", rname);
			return (1);
		}

		what = MNTANY;
		mntpt = name = rname;

		if (stat(name, &sb) == 0) {
			if (S_ISBLK(sb.st_mode))
				what = MNTON;
			else if (S_ISDIR(sb.st_mode))
				what = MNTFROM;
		}

		switch (what) {
		case MNTON:
			if ((mntpt = getmntname(name, MNTON, &type)) == NULL) {
				warnx("%s: not currently mounted", name);
				return (1);
			}
			break;
		case MNTFROM:
			if ((name = getmntname(mntpt, MNTFROM, &type)) == NULL) {
				warnx("%s: not currently mounted", mntpt);
				return (1);
			}
			break;
		default:
			if ((name = getmntname(mntpt, MNTFROM, &type)) == NULL) {
				name = rname;
				if ((mntpt = getmntname(name, MNTON, &type)) == NULL) {
					warnx("%s: not currently mounted", name);
					return (1);
				}
			}
		}
	}

	if (unmount(mntpt, fflag) < 0) {
		warn("%s", mntpt);
		return (1);
	}

	return (0);
}

char *
getmntname(name, what, type)
	char *name;
	mntwhat what;
	char **type;
{
	static struct statfs *mntbuf;
	static int mntsize;
	int i;

	if (mntbuf == NULL &&
	    (mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0) {
		warn("getmntinfo");
		return (NULL);
	}
	for (i = mntsize - 1; i >= 0; i--) {
		if ((what == MNTON) && !strcmp(mntbuf[i].f_mntfromname, name)) {
			if (type)
				*type = mntbuf[i].f_fstypename;
			return (mntbuf[i].f_mntonname);
		}
		if ((what == MNTFROM) && !strcmp(mntbuf[i].f_mntonname, name)) {
			if (type)
				*type = mntbuf[i].f_fstypename;
			return (mntbuf[i].f_mntfromname);
		}
	}
	return (NULL);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: %s\n",
	    "umount [-fvR] special | node");
	exit(1);
}
