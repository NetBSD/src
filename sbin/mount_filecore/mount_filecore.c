/* $NetBSD: mount_filecore.c,v 1.8 2003/08/07 10:04:28 agc Exp $ */

/*
 * Copyright (c) 1992, 1993, 1994 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is contains code contributed to the NetBSD project by 
 * Christopher G. Demetriou
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
 *
 *      mount_filecore.c	1.1	1998/6/26
 */

/*
 * Copyright (c) 1998 Andrew McMurry
 *
 * This code is contains code contributed to the NetBSD project by 
 * Christopher G. Demetriou
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
 *
 *      mount_filecore.c	1.1	1998/6/26
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\
        The Regents of the University of California.\n\
	Copyright (c) 1998 Andrew McMurry\n\
	All rights reserved.\n");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <ctype.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <filecorefs/filecore_mount.h>

#include <mntopts.h>
#include <fattr.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_UPDATE,
	MOPT_GETARGS,
	{ NULL }
};

int	main __P((int, char *[]));
int	mount_filecore __P((int argc, char **argv));
static void	usage __P((void));

#ifndef MOUNT_NOMAIN
int
main(argc, argv)
	int argc;
	char **argv;
{
	return mount_filecore(argc, argv);
}
#endif

int
mount_filecore(argc, argv)
	int argc;
	char **argv;
{
	struct filecore_args args;
	int ch, mntflags, opts, useuid;
	char *dev, *dir;

	mntflags = opts = 0;
	useuid = 1;
	args.uid = 0;
	args.gid = 0;
	
	while ((ch = getopt(argc, argv, "anRfu:g:o:")) != -1)
		switch (ch) {
		case 'a':
			args.flags |= FILECOREMNT_ALLACCESS;
			break;
		case 'n':
			args.flags |= FILECOREMNT_OWNACCESS;
			break;
		case 'R':
			args.flags |= FILECOREMNT_OWNREAD;
			break;
		case 'f':
			args.flags |= FILECOREMNT_FILETYPE;
			break;
                case 'u':
                        args.uid = a_uid(optarg);
			useuid = 0;
                        break;
                case 'g':
                        args.gid = a_gid(optarg);
			useuid = 0;
                        break;
		case 'o':
			getmntopts(optarg, mopts, &mntflags, 0);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	dev = argv[0];
	dir = argv[1];

#define DEFAULT_ROOTUID	-2
	/*
	 * FILECORE filesystems are not writable.
	 */
	mntflags |= MNT_RDONLY;
	if (useuid) args.flags |= FILECOREMNT_USEUID;
	args.export.ex_flags = MNT_EXRDONLY;
	args.fspec = dev;
	args.export.ex_root = DEFAULT_ROOTUID;
	args.flags = opts;

	if (mount(MOUNT_FILECORE, dir, mntflags, &args) < 0)
		err(1, "%s on %s", dev, dir);
	if (mntflags & MNT_GETARGS) {
		char buf[1024];
		(void)snprintb(buf, sizeof(buf), FILECOREMNT_BITS, args.flags);
		printf("uid=%d, gid=%d, flags=%s\n", args.uid, args.gid, buf);
	}
	exit(0);
}

static void
usage()
{
	(void)fprintf(stderr,
		"usage: mount_filecore [-o options] special node\n");
	exit(1);
}
