/*	$NetBSD: loadkmap.c,v 1.10 2011/05/19 21:26:39 tsutsui Exp $	*/
/*
 * loadkmap - load keyboard map (for NetBSD/X680x0)
 * from: amiga/stand/loadkmap/loadkmap.c
 * Copyright 1994 by Masaru Oki
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: loadkmap.c,v 1.10 2011/05/19 21:26:39 tsutsui Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <machine/kbdmap.h>
#include <machine/iteioctl.h>

void load_kmap(const char *);

int
main(int argc, char *argv[])
{

	if (argc != 2) {
		fprintf(stderr, "Usage: %s kmapfile\n", argv[0]);
		exit (1);
	}

	load_kmap(argv[1]);
	exit(0);
}

void
load_kmap(const char *file)
{
	unsigned char buf[sizeof(struct kbdmap)];
	int fd;

	if ((fd = open(file, 0)) >= 0) {
		if (read(fd, buf, sizeof(buf)) == sizeof(buf)) {
			if (ioctl(0, ITEIOCSKMAP, buf) == 0)
				return;
			else
				perror("ITEIOCSKMAP");
		} else {
			perror("read kbdmap");
		}

		close (fd);
	} else {
	    perror("open kbdmap");
	}
}
