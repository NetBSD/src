/* $NetBSD: mount_nilfs.c,v 1.1.12.1 2014/08/20 00:02:26 tls Exp $ */

/*
 * Copyright (c) 2008, 2009 Reinoud Zandijk
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
 */


#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount_nilfs.c,v 1.1.12.1 2014/08/20 00:02:26 tls Exp $");
#endif /* not lint */


#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>


/* mount specific options */
#include <fs/nilfs/nilfs_mount.h>
#include <mntopts.h>

#include "mountprog.h"
#include "mount_nilfs.h"

/* options to pass on to the `mount' call */
static const struct mntopt mopts[] = {
	MOPT_STDOPTS,		/* `normal' options		*/
	MOPT_ASYNC,		/* default			*/
	MOPT_NOATIME,		/* dont update access times	*/
	MOPT_UPDATE,		/* not yet supported		*/
	MOPT_GETARGS,		/* printing			*/
	MOPT_NULL,
};


/* prototypes */
static void	usage(void) __dead;


/* code */

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-o options] [-c cpno] "
	    "[-t gmtoff] special node\n", getprogname());
	exit(EXIT_FAILURE);
}


#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	setprogname(argv[0]);
	return mount_nilfs(argc, argv);
}
#endif


/* main routine */
void
mount_nilfs_parseargs(int argc, char **argv,
	struct nilfs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	struct tm *tm;
	time_t	 now;
	int	 ch, set_gmtoff;
	mntoptparse_t mp;

	/* initialise */
	(void)memset(args, 0, sizeof(*args));

	set_gmtoff = *mntflags = 0;

	while ((ch = getopt(argc, argv, "c:o:t:")) != -1) {
		switch (ch) {
		case 'o' :
			/* process generic mount options */
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL)
				err(EXIT_FAILURE, "getmntopts");
			freemntopts(mp);
			break;
		case 'c' :
			args->cpno = a_num(optarg, "checkpoint number");
			break;
		case 't' :
			args->gmtoff = a_num(optarg, "gmtoff");
			set_gmtoff  = 1;
			break;
		default  :
			usage();
			/* NOTREACHED */
		}
	}

	if (optind + 2 != argc)
		usage();

	if (!set_gmtoff) {
		/* use user's time zone as default */
		(void)time(&now);
		tm = localtime(&now);
		args->gmtoff = tm->tm_gmtoff;
	}

	/* get device and directory specifier */
	pathadj(argv[optind], canon_dev);
	pathadj(argv[optind+1], canon_dir);

	args->version = NILFSMNT_VERSION;
	args->fspec = canon_dev;
}

int
mount_nilfs(int argc, char *argv[])
{
	struct nilfs_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	int mntflags;

	mount_nilfs_parseargs(argc, argv, &args, &mntflags, canon_dev, canon_dir);

	/* mount it! :) */
	if (mount(MOUNT_NILFS, canon_dir, mntflags, &args, sizeof args) == -1)
		err(EXIT_FAILURE, "Cannot mount %s on %s", canon_dev,canon_dir);

	if (mntflags & MNT_GETARGS) {
		char buf[1024];

		(void)snprintb(buf, sizeof(buf), NILFSMNT_BITS,
		    (uint64_t)args.nilfsmflags);
		(void)printf("gmtoffset = %d, cpno = %"PRIu64", flags = %s\n",
		    args.gmtoff, args.cpno, buf);
	}

	return EXIT_SUCCESS;
}
