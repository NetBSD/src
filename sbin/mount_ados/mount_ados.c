/* $NetBSD: mount_ados.c,v 1.26 2007/07/14 15:57:24 dsl Exp $ */

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount_ados.c,v 1.26 2007/07/14 15:57:24 dsl Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <adosfs/adosfs.h>

#include <mntopts.h>
#include <errno.h>
#include <fattr.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

int	mount_ados(int argc, char **argv);
static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_ados(argc, argv);
}
#endif

int
mount_ados(int argc, char **argv)
{
	struct adosfs_args args;
	struct stat sb;
	mntoptparse_t mp;
	int c, mntflags, set_gid, set_uid, set_mask;
	char *dev, *dir, canon_dir[MAXPATHLEN], canon_dev[MAXPATHLEN];

	mntflags = set_gid = set_uid = set_mask = 0;
	(void)memset(&args, '\0', sizeof(args));

	while ((c = getopt(argc, argv, "u:g:m:o:")) != -1) {
		switch (c) {
		case 'u':
			args.uid = a_uid(optarg);
			set_uid = 1;
			break;
		case 'g':
			args.gid = a_gid(optarg);
			set_gid = 1;
			break;
		case 'm':
			args.mask = a_mask(optarg);
			set_mask = 1;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case '?':
		default:
			usage();
			break;
		}
	}

	if (optind + 2 != argc)
		usage();

	dev = argv[optind];
	dir = argv[optind + 1];

	if (realpath(dev, canon_dev) == NULL)        /* Check device path */
		err(1, "realpath %s", dev);
	if (strncmp(dev, canon_dev, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", dev);
		dev = canon_dev;
		warnx("using \"%s\" instead.", dev);
	}

	if (realpath(dir, canon_dir) == NULL)        /* Check mounton path */
		err(1, "realpath %s", dir);
	if (strncmp(dir, canon_dir, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", dir);
		dir = canon_dir;
		warnx("using \"%s\" instead.", dir);
	}

	args.fspec = dev;
	if (!set_gid || !set_uid || !set_mask) {
		if (stat(dir, &sb) == -1)
			err(1, "stat %s", dir);

		if (!set_uid)
			args.uid = sb.st_uid;
		if (!set_gid)
			args.gid = sb.st_gid;
		if (!set_mask)
			args.mask = sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
	}

	if (mount(MOUNT_ADOSFS, dir, mntflags, &args, sizeof args) >= 0)
		exit (0);

	if (errno != EROFS)
		err(1, "%s on %s", dev, dir);

	mntflags |= MNT_RDONLY;

	if (mount(MOUNT_ADOSFS, dir, mntflags, &args, sizeof args) == -1)
		err(1, "%s on %s", dev, dir);

	if (mntflags & MNT_GETARGS)
		printf("uid=%d, gid=%d, mask=0%o\n", args.uid, args.gid,
		    args.mask);

	exit (0);
}

static void
usage(void)
{

	fprintf(stderr, "usage: mount_ados [-o options] [-u user] [-g group] [-m mask] bdev dir\n");
	exit(1);
}
