/*	$NetBSD: mknod.c,v 1.26 2003/05/08 11:34:19 dsl Exp $	*/

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
__RCSID("$NetBSD: mknod.c,v 1.26 2003/05/08 11:34:19 dsl Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <ctype.h>

#include "pack_dev.h"

static int gid_name(const char *, gid_t *);

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
	int	 r_flag = 0;		/* force: delete existing entry */
	void	*modes = 0;
	uid_t	 uid = -1;
	gid_t	 gid = -1;
	int	 rval;

	dev = 0;
	fifo = hasformat = 0;
	pack = pack_native;

	while ((ch = getopt(argc, argv, "rRF:g:m:u:")) != -1) {
		switch (ch) {

		case 'r':
			r_flag = 1;
			break;

		case 'R':
			r_flag = 2;
			break;

		case 'F':
			pack = pack_find(optarg);
			if (pack == NULL)
				errx(1, "invalid format: %s", optarg);
			hasformat++;
			break;

		case 'g':
			if (optarg[0] == '#') {
				gid = strtol(optarg + 1, &p, 10);
				if (*p == 0)
					break;
			}
			if (gid_name(optarg, &gid) == 0)
				break;
			gid = strtol(optarg, &p, 10);
			if (*p == 0)
				break;
			errx(1, "%s: invalid group name", optarg);

		case 'm':
			modes = setmode(optarg);
			if (modes == NULL)
				errx(1, "invalid mode: %s", optarg);
			break;

		case 'u':
			if (optarg[0] == '#') {
				uid = strtol(optarg + 1, &p, 10);
				if (*p == 0)
					break;
			}
			if (uid_from_user(optarg, &uid) == 0)
				break;
			uid = strtol(optarg, &p, 10);
			if (*p == 0)
				break;
			errx(1, "%s: invalid user name", optarg);

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

	umask(mode = umask(0));
	mode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) & ~mode;

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
		mode |= S_IFIFO;
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
		errno = 0;
		numbers[n] = strtoul(argv[n], &p, 0);
		if (*p == 0 && errno == 0)
			continue;
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

	if (modes != NULL)
		mode = getmode(modes, mode);
#if 0
	printf("name: %s\nmode: %05o\ndev:  %08x\nuid: %5d\ngid: %5d\n",
		name, mode, dev, uid, gid);
#endif
	umask(0);
	rval = fifo ? mkfifo(name, mode) : mknod(name, mode, dev);
	if (rval < 0 && errno == EEXIST && r_flag) {
		struct stat sb;
		if (lstat(name, &sb) != 0 || (!fifo && sb.st_rdev != dev))
			sb.st_mode = 0;

		if ((sb.st_mode & S_IFMT) == (mode & S_IFMT)) {
			if (r_flag == 1)
				/* Ignore permissions and user/group */
				return 0;
			if (sb.st_mode != mode)
				rval = chmod(name, mode);
			else
				rval = 0;
		} else {
			unlink(name);
			rval = fifo ? mkfifo(name, mode)
				    : mknod(name, mode, dev);
		}
	}
	if (rval < 0)
		err(1, "%s", name);
	if ((uid != -1 || gid != -1) && chown(name, uid, gid) == -1)
		/* XXX Should we unlink the files here? */
		warn("%s: uid/gid not changed", name);

	return 0;
}

static void
usage(void)
{
	const char *progname = getprogname();

	(void)fprintf(stderr,
	    "Usage: %s [-rR] [-F format] [-m mode] [-u user] [-g group]\n",
	    progname);
	(void)fprintf(stderr,
	    "                   [ name [b | c] major minor\n"
	    "                   | name [b | c] major unit subunit\n"
	    "                   | name [b | c] number\n"
	    "                   | name p ]\n");
	exit(1);
}

static int
gid_name(const char *name, gid_t *gid)
{
	struct group *g;

	g = getgrnam(name);
	if (!g)
		return -1;
	*gid = g->gr_gid;
	return 0;
}
