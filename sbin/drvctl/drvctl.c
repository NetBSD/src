/* $NetBSD: drvctl.c,v 1.3 2005/01/20 15:56:30 xtraeme Exp $ */

/*
 * Copyright (c) 2004
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/drvctlio.h>

#define OPTS "dra:"

static void usage(void);

static void
usage(void)
{

	fprintf(stderr, "Usage: %s -r [-a attribute] busdevice [locator ...]\n"
	    "       %s -d device\n", getprogname(), getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	int c, mode;
	char *attr = 0;
	extern char *optarg;
	extern int optind;
	int fd, res;
	struct devdetachargs daa;
	struct devrescanargs raa;
	int *locs, i;

	while ((c = getopt(argc, argv, OPTS)) != -1) {
		switch (c) {
		case 'd':
		case 'r':
			mode = c;
			break;
		case 'a':
			attr = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	fd = open(DRVCTLDEV, O_RDWR, 0);
	if (fd < 0)
		err(2, "open %s", DRVCTLDEV);

	if (mode == 'd') {
		strlcpy(daa.devname, argv[0], sizeof(daa.devname));

		res = ioctl(fd, DRVDETACHDEV, &daa);
		if (res)
			err(3, "DRVDETACHDEV");
	} else if (mode == 'r') {
		memset(&raa, 0, sizeof(raa));
		strlcpy(raa.busname, argv[0], sizeof(raa.busname));
		if (attr)
			strlcpy(raa.ifattr, attr, sizeof(raa.ifattr));
		if (argc > 1) {
			locs = malloc((argc - 1) * sizeof(int));
			if (!locs)
				err(5, "malloc int[%d]", argc - 1);
			for (i = 0; i < argc - 1; i++)
				locs[i] = atoi(argv[i + 1]);
			raa.numlocators = argc - 1;
			raa.locators = locs;
		}

		res = ioctl(fd, DRVRESCANBUS, &raa);
		if (res)
			err(3, "DRVRESCANBUS");
	} else
		errx(4, "unknown command");

	return (0);
}
