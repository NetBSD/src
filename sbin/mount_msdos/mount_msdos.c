/* $NetBSD: mount_msdos.c,v 1.27 2003/03/22 11:15:51 jdolecek Exp $ */

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
__RCSID("$NetBSD: mount_msdos.c,v 1.27 2003/03/22 11:15:51 jdolecek Exp $");
#endif /* not lint */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <msdosfs/msdosfsmount.h>
#include <ctype.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <mntopts.h>
#include <fattr.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_SYNC,
	MOPT_UPDATE,
	{ NULL }
};

int	main __P((int, char *[]));
int	mount_msdos __P((int argc, char **argv));
static void	usage __P((void));

#ifndef MOUNT_NOMAIN
int
main(argc, argv)
	int argc;
	char **argv;
{
	return mount_msdos(argc, argv);
}
#endif

int
mount_msdos(argc, argv)
	int argc;
	char **argv;
{
	struct msdosfs_args args;
	struct stat sb;
	int c, mntflags, set_gid, set_uid, set_mask;
	char *dev, *dir, ndir[MAXPATHLEN+1];

	mntflags = set_gid = set_uid = set_mask = 0;
	(void)memset(&args, '\0', sizeof(args));

	while ((c = getopt(argc, argv, "Gsl9u:g:m:o:")) != -1) {
		switch (c) {
		case 'G':
			args.flags |= MSDOSFSMNT_GEMDOSFS;
			break;
		case 's':
			args.flags |= MSDOSFSMNT_SHORTNAME;
			break;
		case 'l':
			args.flags |= MSDOSFSMNT_LONGNAME;
			break;
		case '9':
			args.flags |= MSDOSFSMNT_NOWIN95;
			break;
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
			getmntopts(optarg, mopts, &mntflags, 0);
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
	if (dir[0] != '/') {
		warnx("\"%s\" is a relative path.", dir);
		if (getcwd(ndir, sizeof(ndir)) == NULL)
			err(1, "getcwd");
		strncat(ndir, "/", sizeof(ndir) - strlen(ndir) - 1);
		strncat(ndir, dir, sizeof(ndir) - strlen(ndir) - 1);
		dir = ndir;
		warnx("using \"%s\" instead.", dir);
	}

	args.fspec = dev;
	args.export.ex_root = -2;	/* unchecked anyway on DOS fs */
	if (mntflags & MNT_RDONLY)
		args.export.ex_flags = MNT_EXRDONLY;
	else
		args.export.ex_flags = 0;
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

	if (mount(MOUNT_MSDOS, dir, mntflags, &args) < 0)
		err(1, "%s on %s", dev, dir);

	if (mntflags & MNT_GETARGS) {
		char buf[1024];
		(void)snprintb(buf, sizeof(buf), MSDOSFSMNT_BITS, args.flags);
		printf("uid=%d, gid=%d, mask=%d, flags=%s\n", args.uid,
		    args.gid, args.mask, buf);
	}

	exit (0);
}

static void
usage()
{

	fprintf(stderr, "usage: mount_msdos [-o options] [-u user] [-g group] [-m mask] bdev dir\n");
	exit(1);
}
