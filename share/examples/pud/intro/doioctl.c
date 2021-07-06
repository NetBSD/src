/*	$NetBSD: doioctl.c,v 1.2 2021/07/06 09:30:07 jmcneill Exp $	*/

#include <sys/types.h>
#include <sys/ioctl.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int
main(int argc, char *argv[])
{
	int fd, i;

	if (argc != 3 && argc != 4)
		errx(1, "args");

	fd = open(argv[1], O_RDWR);
	if (fd == -1)
		err(1, "open");

	i = atoi(argv[2]);

	if (argc == 3)
		if (ioctl(fd, INTROTOGGLE, &i) == -1)
			err(1, "ioctl");
	else
		if (ioctl(fd, INTROTOGGLE_R, &i) == -1)
			err(1, "ioctl");

	printf("i is now %d\n", i);
}
