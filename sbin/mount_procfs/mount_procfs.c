#include <stdio.h>
#include <sys/types.h>
#include <sys/mount.h>

void
usage ()
{
	fprintf (stderr, "usage: mount_pfs dir\n");
	exit (1);
}
		
int
main (argc, argv)
int argc;
char **argv;
{
	char *dev;
	char *dir;
	int c;
	extern char *optarg;
	extern int optind;
	int opts;

	opts = MNT_RDONLY;

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

	if (mount (MOUNT_PROCFS, dir, opts, (caddr_t)0) < 0) {
		perror ("mount");
		exit (1);
	}

	exit (0);
}

