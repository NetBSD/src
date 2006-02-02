/* $NetBSD: mount_udf.c,v 1.3 2006/02/02 16:25:46 xtraeme Exp $ */

/*
 * Copyright (c) 2006 Reinoud Zandijk
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
__RCSID("$NetBSD: mount_udf.c,v 1.3 2006/02/02 16:25:46 xtraeme Exp $");
#endif /* not lint */


/* main includes; strip me! */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <ctype.h>
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#include <assert.h>
#include <pwd.h>
#include <errno.h>
#include <grp.h>


/* mount specific options */
#include <fs/udf/udf_mount.h>
#include <mntopts.h>
#include <fattr.h>


/* options to pass on to the `mount' call */
static const struct mntopt mopts[] = {
	MOPT_STDOPTS,		/* `normal' options	*/
	MOPT_ASYNC,		/* default		*/
	MOPT_UPDATE,		/* not yet supported	*/
	MOPT_GETARGS,		/* printing		*/
	{ NULL }
};


/* prototypes */
int		mount_udf(int argc, char **argv);
static void	usage(void);


/* code */

static void
usage()
{
	(void)fprintf(stderr, "usage: %s [-g gid] [-o options] [-s session] "
			"[-t gmtoff] [-u uid] special node\n", getprogname());
	exit(EXIT_FAILURE);
}


/* copied from mount_msdos; is it nessisary still? */
#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_udf(argc, argv);
}
#endif


/* main routine */
int
mount_udf(int argc, char **argv)
{
	struct udf_args args;
	struct tm *tm;
	struct passwd *passwd;
	struct group  *group;
	time_t	 now;
	uid_t	 anon_uid, nobody_uid;
	gid_t	 anon_gid, nobody_gid;
	char	*dev, *dir;
	int	 ch, mntflags, set_gmtoff;
	uint32_t sector_size;

	/* set program name for error messages */
	setprogname(argv[0]);

	/* initialise */
	memset(&args, 0, sizeof(args));
	args.version = UDFMNT_VERSION;

	set_gmtoff = mntflags = 0;
	sector_size = 0;

	/* get nobody */
	anon_uid = anon_gid = (uid_t)0;	/* shutup gcc  */
	passwd = getpwnam("nobody");
	group  = getgrnam("nobody");

	if (passwd && group) {
		anon_uid = passwd->pw_uid;
		anon_gid = group->gr_gid;
	} else
		errx(EXIT_FAILURE, "mount_udf: failed to get nobody:nobody\n");

	nobody_uid = anon_uid;
	nobody_gid = anon_gid;

	/* NEVER EVER allow nobody_uid:nobody_gid to be 0:0 */
	assert(nobody_uid != 0);
	assert(nobody_gid != 0);

	while ((ch = getopt(argc, argv, "cg:o:s:t:u:")) != -1) {
		switch (ch) {
#ifdef notyet
		case 'c' :
			args.udfmflags |= UDFMNT_CLOSESESSION;
			break;
#endif
		case 'g' :
			/* convert groupname or numeric equiv. */
			group = getgrnam(optarg);
			if (group == NULL)
				group  = getgrgid((gid_t) atoi(optarg));
			if (group == NULL)
				usage();

			anon_gid = group->gr_gid;
			break;
		case 'u' :
			/* convert username or numeric equiv. */
			passwd = getpwnam(optarg);
			if (passwd == NULL)
				passwd = getpwuid((uid_t) atoi(optarg));
			if (passwd == NULL)
				usage();

			anon_uid = passwd->pw_uid;
			break;
		case 'o' :
			/* process generic mount options */
			getmntopts(optarg, mopts, &mntflags, 0);
			break;
		case 's' :
			args.sessionnr = atoi(optarg);
			break;
		case 't' :
			args.gmtoff = atoi(optarg);
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
		time(&now);
		tm = localtime(&now);
		args.gmtoff = tm->tm_gmtoff;
	}

	/* get device and directory specifier */
	dev = argv[optind];
	dir = argv[optind + 1];
	args.fspec = dev;
	args.anon_uid    = anon_uid;
	args.anon_gid    = anon_gid;
	args.nobody_uid  = nobody_uid;
	args.nobody_gid  = nobody_gid;
	args.sector_size = sector_size;		/* invalid */

	/* mount it! :) */
	if (mount(MOUNT_UDF, dir, mntflags, &args) < 0)
		err(1, "%s on %s", dev, dir);

	if (mntflags & MNT_GETARGS) {
		char buf[1024];

		snprintb(buf, sizeof(buf), UDFMNT_BITS, args.udfmflags);
		printf("gmtoffset=%d, sessionnr=%d, flags=%s\n",
			args.gmtoff, args.sessionnr, buf);
	}

	exit(EXIT_SUCCESS);
}

