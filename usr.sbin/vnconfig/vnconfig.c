/*
 * Copyright (c) 1993 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vnconfig.c 1.1 93/12/15$
 *
 *	@(#)vnconfig.c	8.1 (Berkeley) 12/15/93
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <dev/vnioctl.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VN_CONFIG	1
#define VN_UNCONFIG	2

int verbose = 0;

char *rawdevice __P((char *));
void usage __P((void));

main(argc, argv)
	int argc;
	char **argv;
{
	int ch, rv, action = VN_CONFIG;

	while ((ch = getopt(argc, argv, "cuv")) != -1) {
		switch (ch) {
		case 'c':
			action = VN_CONFIG;
			break;
		case 'u':
			action = VN_UNCONFIG;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
		case '?':
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (action == VN_CONFIG && argc == 2)
		rv = config(argv[0], argv[1], action);
	else if (action == VN_UNCONFIG && argc == 1)
		rv = config(argv[0], NULL, action);
	else
		usage();
	exit(rv);
}

config(dev, file, action)
	char *dev;
	char *file;
	int action;
{
	struct vn_ioctl vnio;
	FILE *f;
	char *rdev;
	int rv;

	rdev = rawdevice(dev);
	f = fopen(rdev, "rw");
	if (f == NULL) {
		warn(rdev);
		return (1);
	}
	vnio.vn_file = file;

	/*
	 * Clear (un-configure) the device
	 */
	if (action == VN_UNCONFIG) {
		rv = ioctl(fileno(f), VNIOCCLR, &vnio);
		if (rv) {
			if (errno == ENODEV) {
				if (verbose)
					printf("%s: not configured\n", dev);
				rv = 0;
			} else
				warn("VNIOCCLR");
		} else if (verbose)
			printf("%s: cleared\n", dev);
	}
	/*
	 * Configure the device
	 */
	if (action == VN_CONFIG) {
		rv = ioctl(fileno(f), VNIOCSET, &vnio);
		if (rv)
			warn("VNIOCSET");
		else if (verbose)
			printf("%s: %d bytes on %s\n", dev, vnio.vn_size, file);
	}
done:
	fclose(f);
	fflush(stdout);
	return (rv < 0);
}

char *
rawdevice(dev)
	char *dev;
{
	register char *rawbuf, *dp, *ep;
	struct stat sb;
	int len;

	len = strlen(dev);
	rawbuf = malloc(len + 2);
	strcpy(rawbuf, dev);
	if (stat(rawbuf, &sb) != 0 || !S_ISCHR(sb.st_mode)) {
		dp = rindex(rawbuf, '/');
		if (dp) {
			for (ep = &rawbuf[len]; ep > dp; --ep)
				*(ep+1) = *ep;
			*++ep = 'r';
		}
	}
	return (rawbuf);
}

void
usage()
{

	(void)fprintf(stderr, "usage: vnconfig -c [-v] special-file regular-file\n");
	(void)fprintf(stderr, "       vnconfig -u [-v] special-file\n");
	exit(1);
}
