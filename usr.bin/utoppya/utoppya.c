/*	$NetBSD: utoppya.c,v 1.3.8.1 2009/05/13 19:20:10 jym Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sysexits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

#include <dev/usb/utoppy.h>

#define	GLOBAL
#include "progressbar.h"

#define	_PATH_DEV_UTOPPY	"/dev/utoppy0"

/*
 * This looks weird for a reason. The toppy protocol allows for data to be
 * transferred in 65535-byte chunks only. Anything more than this has to be
 * split within the driver. The following value leaves enough space for the
 * packet header plus some alignmnent slop.
 */
#define	TOPPY_IO_SIZE	0xffec

static int toppy_fd;

static void cmd_df(int, char **);
static void cmd_ls(int, char **);
static void cmd_rm(int, char **);
static void cmd_mkdir(int, char **);
static void cmd_rename(int, char **);
static void cmd_get(int, char **);
static void cmd_put(int, char **);

static struct toppy_command {
	const char *tc_cmd;
	void (*tc_handler)(int, char **);
} toppy_commands[] = {
	{"df",		cmd_df},
	{"ls",		cmd_ls},
	{"get",		cmd_get},
	{"mkdir",	cmd_mkdir},
	{"put",		cmd_put},
	{"rename",	cmd_rename},
	{"rm",		cmd_rm},
	{NULL,		NULL}
};

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-f <path>] <cmd> ...\n",
	    getprogname());

	exit(EX_USAGE);
}

int
main(int argc, char *argv[])
{
	struct toppy_command *tc;
	const char *devpath;
	int ch;

	setprogname(argv[0]);
	devpath = _PATH_DEV_UTOPPY;

	while ((ch = getopt(argc, argv, "f:")) != -1) {
		switch (ch) {
		case 'f':
			devpath = optarg;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	for (tc = toppy_commands; tc->tc_cmd != NULL; tc++)
		if (strcasecmp(argv[0], tc->tc_cmd) == 0)
			break;

	if (tc->tc_cmd == NULL)
		errx(EX_USAGE, "'%s' is not a valid command", argv[0]);

	if ((toppy_fd = open(devpath, O_RDWR)) < 0)
		err(EX_OSERR, "open(%s)", devpath);

	(*tc->tc_handler)(argc, argv);

	close(toppy_fd);

	return (0);
}

static int
find_toppy_dirent(const char *path, struct utoppy_dirent *udp)
{
	struct utoppy_dirent ud;
	char *d, *b, dir[FILENAME_MAX];
	ssize_t l;

	strncpy(dir, path, sizeof(dir));
	b = basename(dir);
	d = dirname(dir);
	if (strcmp(b, "/") == 0 || strcmp(b, ".") == 0 || strcmp(d, ".") == 0)
		errx(EX_USAGE, "'%s' is not a valid Toppy pathname", path);

	if (ioctl(toppy_fd, UTOPPYIOREADDIR, &d) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOREADDIR, %s)", d);

	if (udp == NULL)
		udp = &ud;

	while ((l = read(toppy_fd, udp, sizeof(*udp))) == sizeof(*udp)) {
		if (strcmp(b, udp->ud_path) == 0)
			break;
	}

	if (l < 0)
		err(EX_OSERR, "read(TOPPYDIR, %s)", d);

	if (l == 0)
		return (0);

	while (read(toppy_fd, &ud, sizeof(ud)) > 0)
		;

	return (1);
}

static void
cmd_df(int argc, char **argv)
{
	struct utoppy_stats us;

	if (ioctl(toppy_fd, UTOPPYIOSTATS, &us) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOSTATS)");

	printf("Hard Disk Size: %" PRId64 " MB\n", us.us_hdd_size / (1024 * 1024));
	printf("Hard Disk Free: %" PRId64 " MB\n", us.us_hdd_free / (1024 * 1024));
}

static void
cmd_ls(int argc, char **argv)
{
	struct utoppy_dirent ud;
	struct tm *tm;
	char *dir, *ext, dirbuf[2], ex, ft, tmbuf[32];
	ssize_t l;

	if (argc == 1) {
		dirbuf[0] = '/';
		dirbuf[1] = '\0';
		dir = dirbuf;
	} else
	if (argc == 2)
		dir = argv[1];
	else
		errx(EX_USAGE, "usage: ls [toppy-pathname]");

	if (ioctl(toppy_fd, UTOPPYIOREADDIR, &dir) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOREADDIR, %s)", dir);

	while ((l = read(toppy_fd, &ud, sizeof(ud))) == sizeof(ud)) {
		switch (ud.ud_type) {
		default:
			ft = '?';
			break;

		case UTOPPY_DIRENT_DIRECTORY:
			ft = 'd';
			break;

		case UTOPPY_DIRENT_FILE:
			ft = '-';
			break;
		}

		if ((ext = strrchr(ud.ud_path, '.')) != NULL &&
		    strcasecmp(ext, ".tap") == 0)
			ex = 'x';
		else
			ex = '-';

		tm = localtime(&ud.ud_mtime);
		strftime(tmbuf, sizeof(tmbuf), "%b %e %G %R", tm);

		printf("%crw%c %11lld %s %s\n", ft, ex, (long long)ud.ud_size,
		    tmbuf, ud.ud_path);
	}

	if (l < 0)
		err(EX_OSERR, "read(utoppy_dirent)");
}

static void
cmd_rm(int argc, char **argv)
{
	char *path;

	if (argc != 2)
		errx(EX_USAGE, "usage: rm <toppy-pathname>");

	path = argv[1];

	if (ioctl(toppy_fd, UTOPPYIODELETE, &path) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIODELETE, %s)", path);
}

static void
cmd_mkdir(int argc, char **argv)
{
	char *path;

	if (argc != 2)
		errx(EX_USAGE, "usage: mkdir <toppy-pathname>");

	path = argv[1];

	if (find_toppy_dirent(path, NULL))
		errx(EX_DATAERR, "'%s' already exists", path);

	if (ioctl(toppy_fd, UTOPPYIOMKDIR, &path) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOMKDIR, %s)", path);
}

static void
cmd_rename(int argc, char **argv)
{
	struct utoppy_dirent ud;
	struct utoppy_rename ur;
	char *oldpath, *newpath, *o, *n;

	if (argc != 3)
		errx(EX_USAGE, "usage: rename <from> <to>");

	o = oldpath = argv[1];
	n = newpath = argv[2];

	for (o = oldpath; *o != '\0'; o++)
		if (*o == '\\')
			*o = '/';
	for (n = newpath; *n != '\0'; n++)
		if (*n == '\\')
			*n = '/';

	for (o = oldpath; *o && *o == '\\'; o++)
		;
	for (n = newpath; *n && *n == '\\'; n++)
		;

	if (strcmp(n, o) == 0)
		errx(EX_DATAERR, "'%s' and '%s' refer to the same file\n",
		    oldpath, newpath);

	if (find_toppy_dirent(oldpath, &ud) == 0)
		errx(EX_DATAERR, "'%s' does not exist on the Toppy", oldpath);

	if (ud.ud_type != UTOPPY_DIRENT_FILE)
		errx(EX_DATAERR, "%s: not a regular file", oldpath);

	if (find_toppy_dirent(newpath, &ud))
		errx(EX_DATAERR, "'%s' already exists", newpath);

	ur.ur_old_path = o;
	ur.ur_new_path = n;

	if (ioctl(toppy_fd, UTOPPYIORENAME, &ur) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIORENAME, %s, %s)", oldpath,
		    newpath);
}


static void
init_progress(FILE *to, char *f, off_t fsize, off_t restart)
{
	struct ttysize ts;

	if (ioctl(fileno(to), TIOCGSIZE, &ts) == -1)
		ttywidth = 80;
	else
		ttywidth = ts.ts_cols;

	ttyout = to;
	progress = 1;
	bytes = 0;
	filesize = fsize;
	restart_point = restart;
	prefix = f;
}

static void
cmd_get(int argc, char **argv)
{
	struct utoppy_readfile ur;
	struct utoppy_dirent ud;
	struct stat st;
	char *dst, dstbuf[FILENAME_MAX];
	uint8_t *buf;
	ssize_t l;
	size_t rv;
	int ch, turbo_mode = 0, reget = 0, progbar = 0;
	FILE *ofp, *to;

	optind = 1;
	optreset = 1;

	while ((ch = getopt(argc, argv, "prt")) != -1) {
		switch (ch) {
		case 'p':
			progbar = 1;
			break;
		case 'r':
			reget = 1;
			break;
		case 't':
			turbo_mode = 1;
			break;
		default:
 get_usage:
			errx(EX_USAGE, "usage: get [-prt] <toppy-pathname> "
			    "[file | directory]");
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 1)
		dst = basename(argv[0]);
	else
	if (argc == 2) {
		dst = argv[1];
		if (stat(dst, &st) == 0 && S_ISDIR(st.st_mode)) {
			snprintf(dstbuf, sizeof(dstbuf), "%s/%s", dst,
			    basename(argv[0]));
			dst = dstbuf;
		}
	} else
		goto get_usage;

	ur.ur_path = argv[0];
	ur.ur_offset = 0;

	if ((buf = malloc(TOPPY_IO_SIZE)) == NULL)
		err(EX_OSERR, "malloc(TOPPY_IO_SIZE)");

	if (strcmp(dst, "-") == 0) {
		ofp = stdout;
		to = stderr;
		if (reget)
			warnx("Ignoring -r option in combination with stdout");
	} else {
		to = stdout;

		if (reget) {
			if (stat(dst, &st) < 0) {
				if (errno != ENOENT)
					err(EX_OSERR, "stat(%s)", dst);
			} else
			if (!S_ISREG(st.st_mode))
				errx(EX_DATAERR, "-r only works with regular "
				    "files");
			else
				ur.ur_offset = st.st_size;
		}

		if ((ofp = fopen(dst, reget ? "a" : "w")) == NULL)
			err(EX_OSERR, "fopen(%s)", dst);
	}

	if (progbar) {
		if (find_toppy_dirent(ur.ur_path, &ud) == 0)
			ud.ud_size = 0;
		init_progress(to, dst, ud.ud_size, ur.ur_offset);
	}

	if (ioctl(toppy_fd, UTOPPYIOTURBO, &turbo_mode) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOTURBO, %d)", turbo_mode);

	if (ioctl(toppy_fd, UTOPPYIOREADFILE, &ur) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOREADFILE, %s)", ur.ur_path);

	if (progbar)
		progressmeter(-1);

	for (;;) {
		while ((l = read(toppy_fd, buf, TOPPY_IO_SIZE)) < 0 &&
		    errno == EINTR)
			;

		if (l <= 0)
			break;

		rv = fwrite(buf, 1, l, ofp);

		if (rv != (size_t)l) {
			if (ofp != stdout)
				fclose(ofp);
			progressmeter(1);
			err(EX_OSERR, "fwrite(%s)", dst);
		}
		bytes += l;
	}

	if (progbar)
		progressmeter(1);

	if (ofp != stdout)
		fclose(ofp);

	if (l < 0)
		err(EX_OSERR, "read(TOPPY: ur.ur_path)");

	free(buf);
}

static void
cmd_put(int argc, char **argv)
{
	struct utoppy_writefile uw;
	struct utoppy_dirent ud;
	struct stat st;
	char dstbuf[FILENAME_MAX];
	char *src;
	void *buf;
	ssize_t rv;
	size_t l;
	int ch, turbo_mode = 0, reput = 0, progbar = 0;
	FILE *ifp;

	optind = 1;
	optreset = 1;

	while ((ch = getopt(argc, argv, "prt")) != -1) {
		switch (ch) {
		case 'p':
			progbar = 1;
			break;
		case 'r':
			reput = 1;
			break;
		case 't':
			turbo_mode = 1;
			break;
		default:
 put_usage:
			errx(EX_USAGE, "usage: put [-prt] <local-pathname> "
			    "<toppy-pathname>");
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		goto put_usage;

	src = argv[0];
	uw.uw_path = argv[1];

	if (stat(src, &st) < 0)
		err(EX_OSERR, "%s", src);

	if (!S_ISREG(st.st_mode))
		errx(EX_DATAERR, "'%s' is not a regular file", src);

	uw.uw_size = st.st_size;
	uw.uw_mtime = st.st_mtime;
	uw.uw_offset = 0;

	if (find_toppy_dirent(uw.uw_path, &ud)) {
		if (ud.ud_type == UTOPPY_DIRENT_DIRECTORY) {
			snprintf(dstbuf, sizeof(dstbuf), "%s/%s", uw.uw_path,
			    basename(src));
			uw.uw_path = dstbuf;
		} else
		if (ud.ud_type != UTOPPY_DIRENT_FILE)
			errx(EX_DATAERR, "'%s' is not a regular file.",
			    uw.uw_path);
		else
		if (reput) {
			if (ud.ud_size > uw.uw_size)
				errx(EX_DATAERR, "'%s' is already larger than "
				    "'%s'", uw.uw_path, src);

			uw.uw_size -= ud.ud_size;
			uw.uw_offset = ud.ud_size;
		}
	}

	if ((buf = malloc(TOPPY_IO_SIZE)) == NULL)
		err(EX_OSERR, "malloc(TOPPY_IO_SIZE)");

	if ((ifp = fopen(src, "r")) == NULL)
		err(EX_OSERR, "fopen(%s)", src);

	if (ioctl(toppy_fd, UTOPPYIOTURBO, &turbo_mode) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOTURBO, %d)", turbo_mode);

	if (ioctl(toppy_fd, UTOPPYIOWRITEFILE, &uw) < 0)
		err(EX_OSERR, "ioctl(UTOPPYIOWRITEFILE, %s)", uw.uw_path);

	if (progbar)
		init_progress(stdout, src, st.st_size, uw.uw_offset);

	if (progbar)
		progressmeter(-1);

	while ((l = fread(buf, 1, TOPPY_IO_SIZE, ifp)) > 0) {
		rv = write(toppy_fd, buf, l);
		if ((size_t)rv != l) {
			fclose(ifp);
			if (progbar)
				progressmeter(1);
			err(EX_OSERR, "write(TOPPY: %s)", uw.uw_path);
		}
		bytes += l;
	}

	if (progbar)
		progressmeter(1);

	if (ferror(ifp))
		err(EX_OSERR, "fread(%s)", src);

	fclose(ifp);
	free(buf);
}
