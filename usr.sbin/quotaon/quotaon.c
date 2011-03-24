/*	$NetBSD: quotaon.c,v 1.26 2011/03/24 17:05:47 bouyer Exp $	*/

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
__RCSID("$NetBSD: quotaon.c,v 1.26 2011/03/24 17:05:47 bouyer Exp $");
#endif
#endif /* not lint */

/*
 * Turn quota on/off for a filesystem.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/mount.h>

#include <quota/quotaprop.h>
#include <ufs/ufs/quota1.h>
#include <sys/quota.h>


#include <err.h>
#include <fstab.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "quotautil.h"

static int	aflag;		/* all file systems */
static int	gflag;		/* operate on group quotas */
static int	uflag;		/* operate on user quotas */
static int	vflag;		/* verbose */

static void usage(void) __attribute__((__noreturn__));
static int quotaonoff(struct fstab *, int, int, const char *);
static int readonly(struct fstab *);

int
main(int argc, char *argv[])
{
	struct fstab *fs;
	char qfnp[MAXPATHLEN];
	long argnum, done = 0;
	int i, offmode = 0, errs = 0;
	int ch;

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
		gflag++;
		uflag++;
	}
	setfsent();
	while ((fs = getfsent()) != NULL) {
		if ((strcmp(fs->fs_vfstype, "ffs") &&
		     strcmp(fs->fs_vfstype, "lfs")) ||
		    strcmp(fs->fs_type, FSTAB_RW))
			continue;
		if (aflag) {
			if (gflag && hasquota(qfnp, sizeof(qfnp), fs, GRPQUOTA))
				errs += quotaonoff(fs, offmode, GRPQUOTA, qfnp);
			if (uflag && hasquota(qfnp, sizeof(qfnp), fs, USRQUOTA))
				errs += quotaonoff(fs, offmode, USRQUOTA, qfnp);
			continue;
		}
		if ((argnum = oneof(fs->fs_file, argv, argc)) >= 0 ||
		    (argnum = oneof(fs->fs_spec, argv, argc)) >= 0) {
			done |= 1U << argnum;
			if (gflag && hasquota(qfnp, sizeof(qfnp), fs, GRPQUOTA))
				errs += quotaonoff(fs, offmode, GRPQUOTA, qfnp);
			if (uflag && hasquota(qfnp, sizeof(qfnp), fs, USRQUOTA))
				errs += quotaonoff(fs, offmode, USRQUOTA, qfnp);
		}
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
quotaonoff( struct fstab *fs, int offmode, int type, const char *qfpathname)
{
	const char *mode = (offmode == 1) ? "off" : "on";
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	struct plistref pref;
	int8_t error8;

	dict = quota_prop_create();
	cmds = prop_array_create();
	datas = prop_array_create();

	if (strcmp(fs->fs_file, "/") && readonly(fs))
		return 1;

	if (dict == NULL || cmds == NULL || datas == NULL)
		errx(1, "can't allocate proplist");

	if (offmode) {
		if (!quota_prop_add_command(cmds, "quotaoff",
		    qfextension[type], datas))
			err(1, "prop_add_command");
	} else {
		data = prop_dictionary_create();
		if (data == NULL)
			errx(1, "can't allocate proplist");
		if (!prop_dictionary_set_cstring(data, "quotafile", 
		    qfpathname))
			err(1, "prop_dictionary_set(quotafile)");
		if (!prop_array_add_and_rel(datas, data))
			err(1, "prop_array_add(data)");
		if (!quota_prop_add_command(cmds, "quotaon",
		    qfextension[type], datas))
			err(1, "prop_add_command");
	}
	if (!prop_dictionary_set(dict, "commands", cmds))
		err(1, "prop_dictionary_set(command)");

	if (!prop_dictionary_send_syscall(dict, &pref))
		err(1, "prop_dictionary_send_syscall");
	prop_object_release(dict);

	if (quotactl(fs->fs_file, &pref) != 0) {
		warn("quotactl(%s)", fs->fs_file);
		return(1);
	}

	if ((errno = prop_dictionary_recv_syscall(&pref, &dict)) != 0)
		err(1, "prop_dictionary_recv_syscall");

	if ((errno = quota_get_cmds(dict, &cmds)) != 0)
		err(1, "quota_get_cmds");

	/* only one command, no need to iter */
	cmd = prop_array_get(cmds, 0);
	if (cmd == NULL)
		err(1, "prop_array_get(cmd)");

	if (!prop_dictionary_get_int8(cmd, "return", &error8))
		err(1, "prop_get(return)");

	if (error8) {
		errno = error8;
		warn("quota%s for %s", mode, fs->fs_file);
		return 1;
	}

	if (vflag) {
		printf("%s: %s quotas turned %s\n",
		    fs->fs_file, qfextension[type], mode);
	}
	return 0;
}

/*
 * Verify file system is mounted and not readonly.
 */
static int
readonly(struct fstab *fs)
{
	struct statvfs fsbuf;

	if (statvfs(fs->fs_file, &fsbuf) < 0 ||
	    strcmp(fsbuf.f_mntonname, fs->fs_file) ||
	    strcmp(fsbuf.f_mntfromname, fs->fs_spec)) {
		printf("%s: not mounted\n", fs->fs_file);
		return 1;
	}
	if (fsbuf.f_flag & MNT_RDONLY) {
		printf("%s: mounted read-only\n", fs->fs_file);
		return 1;
	}
	return 0;
}
