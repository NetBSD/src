#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

int main(int argc, char **argv)
{
	int fd, e, sno;
	char cmd[BUFSIZ], s[BUFSIZ];
	FILE *pp;

	if (argv[1] == NULL)
		errx(1, "usage: %s <fs-root>\n", argv[0]);

	fd = open(argv[1], 0, 0);
	if (fd < 0)
		err(1, argv[1]);

	fcntl(fd, LFCNWRAPGO, NULL);
	close(fd);

	return 0;
}
