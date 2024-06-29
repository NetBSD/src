/* $NetBSD $ */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fs/exfatfs/exfatfs.h>
/*#include <fs/exfatfs/exfatfs_mount.h>*/
#include <err.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#define EXFATFSMNT_BITS "\177\20" /* XXX */

#include <mntopts.h>

#include "mountprog.h"
#include "mount_exfatfs.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_SYNC,
	MOPT_UPDATE,
	MOPT_GETARGS,
	MOPT_NULL,
};

static void	usage(void) __dead;

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	setprogname(argv[0]);
	return mount_exfatfs(argc, argv);
}
#endif

void
mount_exfatfs_parseargs(int argc, char **argv,
	struct exfatfs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	struct stat sb;
	int c, set_gid, set_uid, set_mask, set_dirmask, set_gmtoff;
	char *dev, *dir;
	time_t now;
	struct tm *tm;
	mntoptparse_t mp;

	*mntflags = set_gid = set_uid = set_mask = set_dirmask = set_gmtoff = 0;
	(void)memset(args, '\0', sizeof(*args));

	while ((c = getopt(argc, argv, "u:g:m:M:o:t:")) != -1) {
		switch (c) {
		case 'u':
			args->uid = a_uid(optarg);
			set_uid = 1;
			break;
		case 'g':
			args->gid = a_gid(optarg);
			set_gid = 1;
			break;
		case 'm':
			args->mask = a_mask(optarg);
			set_mask = 1;
			break;
		case 'M':
			args->dirmask = a_mask(optarg);
			set_dirmask = 1;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 't':
			args->gmtoff = atoi(optarg);
			set_gmtoff = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	}

	if (optind + 2 != argc)
		usage();

	if (set_mask && !set_dirmask) {
		args->dirmask = args->mask;
		set_dirmask = 1;
	} else if (set_dirmask && !set_mask) {
		args->mask = args->dirmask;
		set_mask = 1;
	}

	dev = argv[optind];
	dir = argv[optind + 1];

	pathadj(dev, canon_dev);
	pathadj(dir, canon_dir);

	args->fspec = canon_dev;
	if (!set_gid || !set_uid || !set_mask) {
		if (stat(dir, &sb) == -1)
			err(1, "stat %s", dir);

		if (!set_uid)
			args->uid = sb.st_uid;
		if (!set_gid)
			args->gid = sb.st_gid;
		if (!set_mask) {
			args->mask = args->dirmask =
				sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
		}
	}

	if (!set_gmtoff) {
		/* use user's time zone as default */
		time(&now);
		tm = localtime(&now);
		args->gmtoff = tm->tm_gmtoff;

	}

	args->version = EXFATFSMNT_VERSION;
}

int
mount_exfatfs(int argc, char **argv)
{
	struct exfatfs_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	int mntflags;

	mount_exfatfs_parseargs(argc, argv, &args, &mntflags,
	    canon_dev, canon_dir);

	if (mount(MOUNT_EXFATFS, canon_dir, mntflags, &args, sizeof args) == -1)
		err(1, "%s on %s", canon_dev, canon_dir);

	if (mntflags & MNT_GETARGS) {
		char buf[1024];
		(void)snprintb(buf, sizeof(buf), EXFATFSMNT_BITS, args.flags);
		printf("uid=%d, gid=%d, mask=0%o, dirmask=0%o, gmtoff=%d, flags=%s\n",
		    args.uid, args.gid, args.mask, args.dirmask,
		    args.gmtoff, buf);
	}

	exit (0);
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-9GlsU] [-g gid] [-M mask] [-m mask] "
	    "[-o options]\n\t[-t gmtoff] [-u uid] special mountpath\n",
	    getprogname());
	exit(1);
}
