/*	$NetBSD$	*/

#include <sys/cdefs.h>
__RCSID("$NetBSD$");

#include <sys/param.h>
#include <sys/mount.h>
#include <miscfs/fullfs/full.h>

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <mntopts.h>

#include "mountprog.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

int	mount_full(int argc, char **argv);
__dead static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_full(argc, argv);
}
#endif

int
mount_full(int argc, char *argv[])
{
	struct full_args args;
	int ch, mntflags;
	char target[MAXPATHLEN], canon_dir[MAXPATHLEN];
	mntoptparse_t mp;

	mntflags = 0;
	while ((ch = getopt(argc, argv, "o:")) != -1)
		switch(ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(EXIT_FAILURE, "getmntopts");
			freemntopts(mp);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	pathadj(argv[0], target);
	pathadj(argv[1], canon_dir);

	if (strcmp(target, canon_dir) == 0)
		errx(EXIT_FAILURE, "%s (%s) and %s (%s) are identical paths",
		    argv[0], target, argv[1], canon_dir);

	args.la.target = target;

	if (mount(MOUNT_FULL, canon_dir, mntflags, &args, sizeof args) == -1)
		err(EXIT_FAILURE, "%s on %s", target, canon_dir);
	exit(0);
}

static void
usage(void)
{
	(void)fprintf(stderr,
		"usage: mount_full [-o options] target_fs mount_point\n");
	exit(EXIT_FAILURE);
}
