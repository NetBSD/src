#ifndef lint
static char rcsid[] = "$Header: /cvsroot/src/sbin/mount_isofs/Attic/mount_isofs.c,v 1.5.2.2 1993/07/19 13:31:40 cgd Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#define ISOFS
#include <sys/mount.h>

void
usage ()
{
	fprintf (stderr, "usage: mount_iso bdev dir\n");
	exit (1);
}
		
int
main (argc, argv)
int argc;
char **argv;
{
	char *dev;
	char *dir;
	struct ufs_args args;
	int c;
	int opts;

	opts = MNT_RDONLY;

	argc--;
	argv++;
	while (argc > 2) {
		if (!strcmp("-F", argv[0])) {
			argc--; argv++;
			opts |= atoi(argv[0]);
			argc--; argv++;
		} else if (!strcmp(argv[0], "-norrip")) {
			opts |= ISOFSMNT_NORRIP;
			argc--; argv++;
		} else
			usage();
	}

	dev = argv[0];
	dir = argv[1];

	args.fspec = dev;
	args.exflags = MNT_EXRDONLY | opts;
	args.exroot = 0;

	if (mount (MOUNT_ISOFS, dir, MNT_RDONLY, &args) < 0) {
		perror ("mount");
		exit (1);
	}

	exit (0);
}

