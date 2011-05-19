/*
 * loadfont - load ascii font (for NetBSD/X680x0)
 * from: amiga/stand/loadkmap/loadkmap.c
 * Copyright 1993 by Masaru Oki
 *
 *	$NetBSD: loadfont.c,v 1.8 2011/05/19 03:22:27 christos Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: loadfont.c,v 1.8 2011/05/19 03:22:27 christos Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#define ITEKANJI 1 /* XXX */
#include <machine/iteioctl.h>

void load_font(const char *);

int
main(int argc, char *argv[])
{

	if (argc != 2) {
		fprintf(stderr, "Usage: %s fontfile\n", argv[0]);
		exit (1);
	}

	load_font(argv[1]);
	exit(0);
}

void
load_font(const char *file)
{
	unsigned char buf[4096];
	int fd;

	if ((fd = open(file, O_RDONLY)) >= 0) {
		if (read (fd, buf, sizeof(buf)) == sizeof (buf)) {
			if (ioctl(0, ITELOADFONT, buf) == 0)
				return;
			else
				perror("ITELOADFONT");
		} else {
			perror("read font");
		}

		close(fd);
	} else {
		perror("open font");
	}
}
