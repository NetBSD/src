/*
 * Who wrote this???
 */

#ifndef lint
static char rcsid[] = "$Id: mount_msdos.c,v 1.4 1993/09/07 15:40:33 ws Exp $";
#endif /* not lint */

#include <stdio.h>
#include <sys/types.h>
#include <sys/mount.h>

char *progname;

void
usage ()
{
	fprintf (stderr, "usage: %s bdev dir\n", progname);
	exit (1);
}
		
int
main (argc, argv)
int argc;
char **argv;
{
	char *dev;
	char *dir;
	struct msdosfs_args args;
	int c;
	extern char *optarg;
	extern int optind;
	int opts;

	progname = argv[0];

	opts = 0;

	while ((c = getopt (argc, argv, "F:")) != EOF) {
		switch (c) {
		case 'F':
			opts |= atoi (optarg);
			break;
		default:
			usage ();
		}
	}

	if (optind + 2 != argc)
		usage ();

	dev = argv[optind];
	dir = argv[optind + 1];

	args.fspec = dev;

	if (mount (MOUNT_MSDOS, dir, opts, &args) < 0) {
		perror ("mount");
		exit (1);
	}

	exit (0);
}
