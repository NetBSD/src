/*	$NetBSD: extattrctl.c,v 1.1.28.1 2009/05/13 19:20:22 jym Exp $	*/

/*-
 * Copyright (c) 1999-2002 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed by Robert Watson for the TrustedBSD Project.
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
 * $FreeBSD: src/usr.sbin/extattrctl/extattrctl.c,v 1.19 2002/04/19 01:42:55 rwatson Exp $
 */

/*
 * Developed by the TrustedBSD Project.
 * Support for file system extended attribute.
 */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/extattr.h>
#include <sys/param.h>
#include <sys/mount.h>

#include <ufs/ufs/extattr.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <machine/bswap.h>
#include <machine/endian.h>

static int needswap;

static uint32_t
rw32(uint32_t v)
{
	if (needswap)
		return (bswap32(v));
	return (v);
}

static void
usage(void)
{

	fprintf(stderr,
	    "usage:\n"
	    "  %s start path\n"
	    "  %s stop path\n"
	    "  %s initattr [-f] [-p path] attrsize attrfile\n"
	    "  %s showattr attrfile\n"
	    "  %s enable path attrnamespace attrname attrfile\n"
	    "  %s disable path attrnamespace attrname\n",
	    getprogname(), getprogname(), getprogname(),
	    getprogname(), getprogname(), getprogname());
	exit(1);
}

static uint64_t
num_inodes_by_path(const char *path)
{
	struct statvfs buf;

	if (statvfs(path, &buf) == -1) {
		warn("statvfs(%s)", path);
		return (-1);
	}

	return (buf.f_files);
}

static const char zero_buf[8192];

static int
initattr(int argc, char *argv[])
{
	struct ufs_extattr_fileheader uef;
	char *fs_path = NULL;
	int ch, i, error, flags;
	ssize_t wlen;
	size_t easize;

	flags = O_CREAT | O_WRONLY | O_TRUNC | O_EXCL;
	optind = 0;
	while ((ch = getopt(argc, argv, "fp:r:w:")) != -1) {
		switch (ch) {
		case 'f':
			flags &= ~O_EXCL;
			break;
		case 'p':
			fs_path = optarg;
			break;
		case 'B':
#if BYTE_ORDER == LITTLE_ENDIAN
			if (strcmp(optarg, "le") == 0)
				needswap = 0;
			else if (strcmp(optarg, "be") == 0)
				needswap = 1;
			else
				usage();
#else
			if (strcmp(optarg, "be") == 0)
				needswap = 0;
			else if (strcmp(optarg, "le") == 0)
				needswap = 1;
			else
				usage();
#endif
			break;
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	error = 0;
	if ((i = open(argv[1], flags, 0600)) == -1) {
		warn("open(%s)", argv[1]);
		return (-1);
	}
	uef.uef_magic = rw32(UFS_EXTATTR_MAGIC);
	uef.uef_version = rw32(UFS_EXTATTR_VERSION);
	uef.uef_size = rw32(atoi(argv[0]));
	if (write(i, &uef, sizeof(uef)) != sizeof(uef)) {
		warn("unable to write arribute file header");
		error = -1;
	} else if (fs_path != NULL) {
		easize = (sizeof(uef) + uef.uef_size) *
		    num_inodes_by_path(fs_path);
		while (easize > 0) {
			size_t x = (easize > sizeof(zero_buf)) ?
			    sizeof(zero_buf) : easize;
			wlen = write(i, zero_buf, x);
			if ((size_t)wlen != x) {
				warn("unable to write attribute file");
				error = -1;
				break;
			}
			easize -= wlen;
		}
	}
	if (error == -1) {
		unlink(argv[1]);
		return (-1);
	}

	return (0);
}

static int
showattr(int argc, char *argv[])
{
	struct ufs_extattr_fileheader uef;
	int i, fd;
	const char *bo;

	if (argc != 1)
		usage();

	fd = open(argv[0], O_RDONLY);
	if (fd == -1) {
		warn("open(%s)", argv[0]);
		return (-1);
	}

	i = read(fd, &uef, sizeof(uef));
	if (i != sizeof(uef)) {
		warn("unable to read attribute file header");
		return (-1);
	}

	if (rw32(uef.uef_magic) != UFS_EXTATTR_MAGIC) {
		needswap = 1;
		if (rw32(uef.uef_magic) != UFS_EXTATTR_MAGIC) {
			fprintf(stderr, "%s: bad magic\n", argv[0]);
			return (-1);
		}
	}

#if BYTE_ORDER == LITTLE_ENDIAN
	bo = needswap ? "big-endian" : "little-endian";
#else
	bo = needswap ? "little-endian" : "big-endian";
#endif

	printf("%s: version %u, size %u, byte-order: %s\n",
	    argv[0], rw32(uef.uef_version), rw32(uef.uef_size), bo);

	return (0);
}

int
main(int argc, char *argv[])
{
	int error = 0, attrnamespace;

	if (argc < 2)
		usage();

	if (!strcmp(argv[1], "start")) {
		if (argc != 3)
			usage();
		error = extattrctl(argv[2], UFS_EXTATTR_CMD_START, NULL, 0,
		    NULL);
		if (error)
			err(1, "start");
	} else if (!strcmp(argv[1], "stop")) {
		if (argc != 3)
			usage();
		error = extattrctl(argv[2], UFS_EXTATTR_CMD_STOP, NULL, 0,
		   NULL);
		if (error)
			err(1, "stop");
	} else if (!strcmp(argv[1], "enable")) {
		if (argc != 6)
			usage();
		error = extattr_string_to_namespace(argv[3], &attrnamespace);
		if (error)
			errx(1, "bad namespace: %s", argv[3]);
		error = extattrctl(argv[2], UFS_EXTATTR_CMD_ENABLE, argv[5],
		    attrnamespace, argv[4]);
		if (error)
			err(1, "enable");
	} else if (!strcmp(argv[1], "disable")) {
		if (argc != 5)
			usage();
		error = extattr_string_to_namespace(argv[3], &attrnamespace);
		if (error)
			errx(1, "bad namespace: %s", argv[3]);
		error = extattrctl(argv[2], UFS_EXTATTR_CMD_DISABLE, NULL,
		    attrnamespace, argv[4]);
		if (error)
			err(1, "disable");
	} else if (!strcmp(argv[1], "initattr")) {
		argc -= 2;
		argv += 2;
		error = initattr(argc, argv);
		if (error)
			return (1);
	} else if (!strcmp(argv[1], "showattr")) {
		argc -= 2;
		argv += 2;
		error = showattr(argc, argv);
		if (error)
			return (1);
	} else
		usage();

	return (0);
}
