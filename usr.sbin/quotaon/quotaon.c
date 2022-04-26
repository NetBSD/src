/*	$NetBSD: quotaon.c,v 1.31 2022/04/26 15:39:00 hannken Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quotaon.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: quotaon.c,v 1.31 2022/04/26 15:39:00 hannken Exp $");
#endif
#endif /* not lint */

/*
 * Turn quota on/off for a filesystem.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/mount.h>

#include <quota.h>
#include <ufs/ufs/quota1.h>

#include <err.h>
#include <fstab.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>


static int	vflag;		/* verbose */

static void usage(void) __dead;
static int quotaonoff(struct fstab *, struct quotahandle *, int, int, int,
    const char *);
static int readonly(struct fstab *, const char *);
static int oneof(const char *target, char *list[], int cnt);

int
main(int argc, char *argv[])
{
	struct fstab *fs;
	struct quotahandle *qh;
	long argnum, done = 0;
	int i, offmode = 0, errs = 0;
	unsigned restrictions;
	int ch;

	int aflag = 0;		/* all file systems */
	int gflag = 0;		/* operate on group quotas */
	int uflag = 0;		/* operate on user quotas */
	int noguflag = 0;	/* operate on both (by default) */

	if (strcmp(getprogname(), "quotaoff") == 0)
		offmode++;
	else if (strcmp(getprogname(), "quotaon") != 0)
		errx(1, "Name must be quotaon or quotaoff");

	while ((ch = getopt(argc, argv, "avug")) != -1) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'g':
			gflag++;
			break;
		case 'u':
			uflag++;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc <= 0 && !aflag)
		usage();

	if (!gflag && !uflag) {
		noguflag = 1;
	}

	/*
	 * XXX at the moment quota_open also uses getfsent(), but it
	 * uses it only up front. To avoid conflicting with it, let it
	 * initialize first.
	 */
	qh = quota_open("/");
	if (qh != NULL) {
		quota_close(qh);
	}

	setfsent();
	while ((fs = getfsent()) != NULL) {
		char buf[MAXPATHLEN];
		const char *fsspec;
		if ((strcmp(fs->fs_vfstype, "ffs") &&
		     strcmp(fs->fs_vfstype, "lfs")) ||
		    strcmp(fs->fs_type, FSTAB_RW))
			continue;

		fsspec = getfsspecname(buf, sizeof(buf), fs->fs_spec);
		if (fsspec == NULL) {
			warn("%s", buf);
			continue;
		}
		if (!aflag) {
			if ((argnum = oneof(fs->fs_file, argv, argc)) < 0 &&
			    (argnum = oneof(fsspec, argv, argc)) < 0) {
				continue;
			}
			done |= 1U << argnum;
		}

		qh = quota_open(fs->fs_file);
		if (qh == NULL) {
			if (!aflag) {
				warn("quota_open");
				errs++;
			}
			continue;
		}

		restrictions = quota_getrestrictions(qh);
		if ((restrictions & QUOTA_RESTRICT_NEEDSQUOTACHECK) == 0) {
			/* Not a quota v1 volume, skip it */
			if (!aflag) {
				errno = EBUSY;
				warn("%s", fs->fs_file);
				errs++;
			}
			quota_close(qh);
			continue;
		}

		/*
		 * The idea here is to warn if someone explicitly
		 * tries to turn on group quotas and there are no
		 * group quotas, and likewise for user quotas, but not
		 * to warn if just doing the default thing and one of
		 * the quota types isn't configured.
		 */

		if (noguflag) {
			errs += quotaonoff(fs, qh, offmode, GRPQUOTA, 0, fsspec);
			errs += quotaonoff(fs, qh, offmode, USRQUOTA, 0, fsspec);
		}
		if (gflag) {
			errs += quotaonoff(fs, qh, offmode, GRPQUOTA, 1, fsspec);
		}
		if (uflag) {
			errs += quotaonoff(fs, qh, offmode, USRQUOTA, 1, fsspec);
		}
		quota_close(qh);
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1U << i)) == 0)
			warnx("%s not found in fstab", argv[i]);
	return errs;
}

static void
usage(void)
{
	const char *p = getprogname();
	(void) fprintf(stderr, "Usage: %s [-g] [-u] [-v] -a\n"
	    "\t%s [-g] [-u] [-v] filesys ...\n", p, p);
	exit(1);
}

static int
quotaonoff(struct fstab *fs, struct quotahandle *qh, int offmode, int idtype,
    int warn_on_enxio, const char *fsspec)
{
	const char *mode = (offmode == 1) ? "off" : "on";
	const char *type;

	if (strcmp(fs->fs_file, "/") && readonly(fs, fsspec)) {
		return 1;
	}

	if (offmode) {
		type = quota_idtype_getname(qh, idtype);
		if (quota_quotaoff(qh, idtype)) {
			if (warn_on_enxio || errno != ENXIO) {
				warn("quota%s for %s", mode, fs->fs_file);
			}
			return 1;
		}
	} else {
		if (quota_quotaon(qh, idtype)) {
			if (warn_on_enxio || errno != ENXIO) {
				warn("quota%s for %s", mode, fs->fs_file);
			}
			return 1;
		}
		type = quota_idtype_getname(qh, idtype);
	}

	if (vflag) {
		printf("%s: %s quotas turned %s\n",
		    fs->fs_file, type, mode);
	}
	return 0;
}

/*
 * Verify file system is mounted and not readonly.
 */
static int
readonly(struct fstab *fs, const char *fsspec)
{
	struct statvfs fsbuf;

	if (statvfs(fs->fs_file, &fsbuf) < 0 ||
	    strcmp(fsbuf.f_mntonname, fs->fs_file) ||
	    strcmp(fsbuf.f_mntfromname, fsspec)) {
		printf("%s: not mounted\n", fs->fs_file);
		return 1;
	}
	if (fsbuf.f_flag & MNT_RDONLY) {
		printf("%s: mounted read-only\n", fs->fs_file);
		return 1;
	}
	return 0;
}

/*
 * Check to see if target appears in list of size cnt.
 */
static int
oneof(const char *target, char *list[], int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		if (strcmp(target, list[i]) == 0)
			return i;
	return -1;
}
