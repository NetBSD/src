/*	$NetBSD: fstyp.c,v 1.1 2018/01/09 03:31:15 christos Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
 *
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: fstyp.c,v 1.1 2018/01/09 03:31:15 christos Exp $");

#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "fstyp.h"

#define	LABEL_LEN	256

typedef int (*fstyp_function)(FILE *, char *, size_t);
typedef int (*fsvtyp_function)(const char *, char *, size_t);

static struct {
	const char	*name;
	fstyp_function	function;
	bool		unmountable;
} fstypes[] = {
	{ "cd9660", &fstyp_cd9660, false },
	{ "ext2fs", &fstyp_ext2fs, false },
	{ "msdosfs", &fstyp_msdosfs, false },
	{ "ntfs", &fstyp_ntfs, false },
	{ "ufs", &fstyp_ufs, false },
#ifdef HAVE_ZFS
	{ "zfs", &fstyp_zfs, true },
#endif
	{ NULL, NULL, NULL }
};

static struct {
	const char	*name;
	fsvtyp_function	function;
	bool		unmountable;
} fsvtypes[] = {
	{ NULL, NULL, NULL }
};

void *
read_buf(FILE *fp, off_t off, size_t len)
{
	int error;
	size_t nread;
	void *buf;

	error = fseek(fp, off, SEEK_SET);
	if (error != 0) {
		warn("cannot seek to %jd", (uintmax_t)off);
		return NULL;
	}

	buf = malloc(len);
	if (buf == NULL) {
		warn("cannot malloc %zd bytes of memory", len);
		return NULL;
	}

	nread = fread(buf, len, 1, fp);
	if (nread != 1) {
		free(buf);
		if (feof(fp) == 0)
			warn("fread");
		return NULL;
	}

	return buf;
}

char *
checked_strdup(const char *s)
{
	char *c;

	c = strdup(s);
	if (c == NULL)
		err(EXIT_FAILURE, "strdup");
	return c;
}

void
rtrim(char *label, size_t size)
{
	for (size_t i = size; i > 0; i--) {
		size_t j = i - 1;
		if (label[j] == '\0')
			continue;
		else if (label[j] == ' ')
			label[j] = '\0';
		else
			break;
	}
}

__dead static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-l] [-s] [-u] special\n", getprogname());
	exit(EXIT_FAILURE);
}

static void
type_check(const char *path, FILE *fp)
{
	int error, fd;
	struct stat sb;
	struct disklabel dl;

	fd = fileno(fp);

	error = fstat(fd, &sb);
	if (error != 0)
		err(EXIT_FAILURE, "%s: fstat", path);

	if (S_ISREG(sb.st_mode))
		return;

	error = ioctl(fd, DIOCGDINFO, &dl);
	if (error != 0)
		errx(EXIT_FAILURE, "%s: not a disk", path);
}

int
main(int argc, char **argv)
{
	int ch, error, i, nbytes;
	bool ignore_type = false, show_label = false, show_unmountable = false;
	char label[LABEL_LEN + 1], strvised[LABEL_LEN * 4 + 1];
	char *path;
	const char *name = NULL;
	FILE *fp;
	fstyp_function fstyp_f;
	fsvtyp_function fsvtyp_f;

	while ((ch = getopt(argc, argv, "lsu")) != -1) {
		switch (ch) {
		case 'l':
			show_label = true;
			break;
		case 's':
			ignore_type = true;
			break;
		case 'u':
			show_unmountable = true;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();

	path = argv[0];

	fp = fopen(path, "r");
	if (fp == NULL)
		goto fsvtyp; /* DragonFly */

	if (ignore_type == false)
		type_check(path, fp);

	memset(label, '\0', sizeof(label));

	for (i = 0;; i++) {
		if (!show_unmountable && fstypes[i].unmountable)
			continue;
		fstyp_f = fstypes[i].function;
		if (fstyp_f == NULL)
			break;

		error = fstyp_f(fp, label, sizeof(label));
		if (error == 0) {
			name = fstypes[i].name;
			goto done;
		}
	}
fsvtyp:
	for (i = 0;; i++) {
		if (!show_unmountable && fsvtypes[i].unmountable)
			continue;
		fsvtyp_f = fsvtypes[i].function;
		if (fsvtyp_f == NULL)
			break;

		error = fsvtyp_f(path, label, sizeof(label));
		if (error == 0) {
			name = fsvtypes[i].name;
			goto done;
		}
	}

	err(EXIT_FAILURE, "%s: filesystem not recognized", path);
	/*NOTREACHED*/
done:
	if (show_label && label[0] != '\0') {
		/*
		 * XXX: I'd prefer VIS_HTTPSTYLE, but it unconditionally
		 *      encodes spaces.
		 */
		nbytes = strsnvis(strvised, sizeof(strvised), label,
		    VIS_GLOB | VIS_NL, "\"'$");
		if (nbytes == -1)
			err(EXIT_FAILURE, "strsnvis");

		printf("%s %s\n", name, strvised);
	} else {
		printf("%s\n", name);
	}

	return EXIT_SUCCESS;
}
