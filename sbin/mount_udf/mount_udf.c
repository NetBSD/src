/* $NetBSD: mount_udf.c,v 1.11 2007/12/15 19:44:46 perry Exp $ */

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
__RCSID("$NetBSD: mount_udf.c,v 1.11 2007/12/15 19:44:46 perry Exp $");
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
#include <fs/udf/udf_mount.h>
#include <mntopts.h>
#include <fattr.h>


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
int		mount_udf(int argc, char **argv);
static void	usage(void) __dead;


/* code */

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-g gid] [-o options] [-s session] "
	    "[-t gmtoff] [-u uid] special node\n", getprogname());
	exit(EXIT_FAILURE);
}


/* copied from mount_msdos; is it necessary still? */
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
	time_t	 now;
	uid_t	 anon_uid, nobody_uid;
	gid_t	 anon_gid, nobody_gid;
	char	*dev, *dir;
	int	 ch, mntflags, set_gmtoff;
	uint32_t sector_size;
	mntoptparse_t mp;

	/* set program name for error messages */
	setprogname(argv[0]);

	/* initialise */
	(void)memset(&args, 0, sizeof(args));

	set_gmtoff = mntflags = 0;
	sector_size = 0;

	/* get nobody */
	nobody_uid = anon_uid = a_uid("nobody");
	nobody_gid = anon_gid = a_gid("nobody");

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
			anon_gid = a_gid(optarg);
			break;
		case 'u' :
			/* convert username or numeric equiv. */
			anon_uid = a_uid(optarg);
			break;
		case 'o' :
			/* process generic mount options */
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(EXIT_FAILURE, "getmntopts");
			freemntopts(mp);
			break;
		case 's' :
			args.sessionnr = a_num(optarg, "session number");
			break;
		case 't' :
			args.gmtoff = a_num(optarg, "gmtoff");
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
		args.gmtoff = tm->tm_gmtoff;
	}

	/* get device and directory specifier */
	dev = argv[optind];
	dir = argv[optind + 1];

	args.version = UDFMNT_VERSION;
	args.fspec = dev;
	args.anon_uid    = anon_uid;
	args.anon_gid    = anon_gid;
	args.nobody_uid  = nobody_uid;
	args.nobody_gid  = nobody_gid;
	args.sector_size = sector_size;		/* invalid */

	/* mount it! :) */
	if (mount(MOUNT_UDF, dir, mntflags, &args, sizeof args) == -1)
		err(EXIT_FAILURE, "Cannot mount %s on %s", dev, dir);

	if (mntflags & MNT_GETARGS) {
		char buf[1024];

		(void)snprintb(buf, sizeof(buf), UDFMNT_BITS,
		    (uint64_t)args.udfmflags);
		(void)printf("gmtoffset=%d, sessionnr=%d, flags=%s\n",
		    args.gmtoff, args.sessionnr, buf);
	}

	return EXIT_SUCCESS;
}
