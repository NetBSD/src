/*	$NetBSD: installboot.c,v 1.5.18.1 2017/12/03 11:35:49 jdolecek Exp $	*/

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../elf2bb/chksum.h"

/* XXX Must be kept in sync with bbstart.s! */
#define CMDLN_LOC 0x10
#define CMDLN_LEN 0x20

int main(int argc, char *argv[]);

int main(int argc, char *argv[]){

	char *line = NULL;
	char *progname;
	char *bootnam, *devnam;
	char *dline;
	int bootfd, devfd;
	int rc;	/* read,  write */
	int c;	/* getopt */
	int sumlen;
	u_int32_t sum2, sum16;
	
	u_int32_t block[128*16];

	progname = argv[0];
	while ((c = getopt(argc, argv, "l:")) != -1) {
		switch(c) {
		case 'l':
			line = optarg;
			break;
		default:
			errx(1,
			"usage: %s [-l newcommandline] bootblock device",
			progname);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 2) {
		errx(1, "usage: %s [-l newcommandline] bootblock device",
			progname);
		/* NOTREACHED */
	}

	bootnam = *argv++;
	devnam = *argv;
	
	bootfd = open(bootnam, O_RDONLY, 0);
	if (bootfd < 0) {
		err(1, "Can't open bootblock for reading");
		/* NOTREACHED */
	}

	devfd = open(devnam, O_CREAT|O_RDWR, 0666);
	if (devfd < 0) {
		err(1, "Can't open output device for writing");
		/* NOTREACHED */
	}

	rc = read(bootfd, block, sizeof(block));

	if (rc < sizeof(block)) {
		err(1, "Can't read bootblock");
		/* NOTREACHED */
	}

	/* XXX the choices should not be hardcoded */

	sum2  = chksum(block, 1024/4);
	sum16 = chksum(block, 8192/4);

	if (sum16 == 0xffffffff) {
		sumlen = 8192/4;
	} else if (sum2 == 0xffffffff) {
		sumlen = 1024/4;
	} else {
		errx(1, "%s: wrong checksum", bootnam);
		/* NOTREACHED */
	}

	if (sum2 == sum16) {
		warnx("eek - both sums are the same");
	}
	

	if (line) {
		dline = (char *)&(block[CMDLN_LOC/4]);
		/* XXX keep the default default line in sync with bbstart.s */
		if (strcmp(dline, "netbsd -ASn2") != 0) {
			errx(1, "Old bootblock version? Can't change command line.");
		}
		(void)strncpy(dline, line, CMDLN_LEN-1);

		block[1] = 0;
		block[1] = 0xffffffff - chksum(block, sumlen);
	}

	rc = write(devfd, block, sizeof(block));

	if (rc < sizeof(block)) {
		err(1, "Can't write bootblock");
		/* NOTREACHED */
	}

	exit(1);
	/* NOTREACHED */
}
