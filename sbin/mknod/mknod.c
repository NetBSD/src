/*	$NetBSD: mknod.c,v 1.24 2001/10/08 04:45:29 lukem Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1998 The NetBSD Foundation, Inc.  All rights reserved.\n");
__RCSID("$NetBSD: mknod.c,v 1.24 2001/10/08 04:45:29 lukem Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pack_dev.h"

	int	main(int, char *[]);
static	void	usage(void);


#define	MAXARGS	3		/* 3 for bsdos, 2 for rest */

int
main(int argc, char **argv)
{
	char	*name, *p;
	mode_t	 mode;
	dev_t	 dev;
	pack_t	*pack;
	u_long	 numbers[MAXARGS];
	int	 n, ch, fifo, hasformat;

	dev = 0;
	fifo = hasformat = 0;
	pack = pack_native;

	while ((ch = getopt(argc, argv, "F:")) != -1) {
		switch (ch) {
		case 'F':
			pack = pack_find(optarg);
			if (pack == NULL)
				errx(1, "invalid format: %s", optarg);
			hasformat++;
			break;

		default:
		case '?':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2 || argc > 10)
		usage();

	name = *argv;
	argc--;
	argv++;

	mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	if (argv[0][1] != '\0')
		goto badtype;
	switch (*argv[0]) {
	case 'c':
		mode |= S_IFCHR;
		break;

	case 'b':
		mode |= S_IFBLK;
		break;

	case 'p':
		if (hasformat)
			errx(1, "format is meaningless for fifos");
		fifo = 1;
		break;

	default:
 badtype:
		errx(1, "node type must be 'b', 'c' or 'p'.");
	}
	argc--;
	argv++;

	if (fifo) {
		if (argc != 0)
			usage();
	} else {
		if (argc < 1 || argc > MAXARGS)
			usage();
	}

	for (n = 0; n < argc; n++) {
		numbers[n] = strtoul(argv[n], &p, 0);
		if ((p && *p != '\0') ||
		    (numbers[n] == ULONG_MAX && errno == ERANGE))
			errx(1, "invalid number: %s", argv[n]);
	}

	switch (argc) {
	case 0:
		dev = 0;
		break;

	case 1:
		dev = numbers[0];
		break;

	default:
		dev = (*pack)(argc, numbers);
		break;
	}

#if 0
	printf("name: %s\nmode: %05o\ndev:  %08x\n", name, mode, dev);
#else
	if ((fifo ? mkfifo(name, mode) : mknod(name, mode, dev)) < 0)
		err(1, "%s", name);
#endif

	return(0);
}

static void
usage(void)
{
	const char *progname = getprogname();

	(void)fprintf(stderr,
	    "Usage: %s [-F format] name [b | c] major minor\n", progname);
	(void)fprintf(stderr,
	    "       %s [-F format] name [b | c] major unit subunit\n",
	    progname);
	(void)fprintf(stderr, "       %s name [b | c] number\n", progname);
	(void)fprintf(stderr, "       %s name p\n", progname);
	exit(1);
}
