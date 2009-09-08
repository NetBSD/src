/*	$NetBSD: img2cgd.c,v 1.1 2009/09/08 21:48:25 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

/*
 * We really should use disklabel.  However, for the time being,
 * use a endian independent magic number at offset == 0 and a
 * 64bit size at offset == 8.
 */
#define MYMAGIC 0x11000a00000a0011LL
#define MAGOFF	0
#define SIZEOFF	8

#define SKIPLABEL 8192
#define IMG_MINSIZE (128*1024) /* label/mbr/etc search looks here and there */

static void
usage(void)
{

	fprintf(stderr, "usage: %s read|write cgd_image file\n", getprogname());
	exit(1);
}

typedef ssize_t (*readfn)(int, void *, size_t);
typedef ssize_t (*writefn)(int, const void *, size_t);

#define BLOCKSIZE 512
#define BLKROUND(a) (((a)+(BLOCKSIZE-1)) & ~(BLOCKSIZE-1))

static void
doxfer(int fd_from, int fd_to, off_t nbytes, readfn rfn, writefn wfn,
	int roundwrite)
{
	char buf[8192];
	ssize_t n;

	assert(sizeof(buf) % BLOCKSIZE == 0);
	if (roundwrite)
		nbytes = BLKROUND(nbytes);

	memset(buf, 0, sizeof(buf));
	while (nbytes) {
		n = rfn(fd_from, buf, sizeof(buf));
		if (n == -1)
			err(1, "read");
		if (n == 0)
			break;
		n = MIN(n, nbytes);
		if (roundwrite)
			n = BLKROUND(n);
		nbytes -= n;
		if (wfn(fd_to, buf, n) == -1)
			err(1, "write");
	}
}

#define RFLAGS (O_RDONLY)
#define WFLAGS (O_WRONLY | O_CREAT | O_TRUNC)
int
main(int argc, char *argv[])
{
	const char *the_argv[10];
	const char *cgd_file, *img_file;
	struct stat sb_cgd, sb_file;
	off_t nbytes;
	int error;
	int fd, fd_r;
	int readmode;

	setprogname(argv[0]);

	if (argc != 4)
		usage();

	if (strcmp(argv[1], "read") == 0)
		readmode = 1;
	else if (strcmp(argv[1], "write") == 0)
		readmode = 0;
	else
		usage();

	cgd_file = argv[2];
	img_file = argv[3];

	if (stat(img_file, &sb_file) == -1) {
		if (!readmode)
			err(1, "cannot open file image %s", img_file);
	} else {
		if (!S_ISREG(sb_file.st_mode))
			errx(1, "%s is not a regular file", img_file);
	}

	if (stat(cgd_file, &sb_cgd) == -1) {
		if (readmode)
			err(1, "cannot open cgd image %s", cgd_file);
	} else {
		if (!S_ISREG(sb_cgd.st_mode))
			errx(1, "%s is not a regular file", cgd_file);
	}

	/*
	 * Create a file big enough to hold the file we are encrypting.
	 * This is because cgd works on a device internally and does
	 * not know how to enlarge a device (surprisingly ...).
	 */
	if (!readmode) {
		uint64_t tmpval;

		fd = open(cgd_file, WFLAGS, 0755);
		if (fd == -1)
			err(1, "fd");
		ftruncate(fd,
		    MAX(IMG_MINSIZE, BLKROUND(sb_file.st_size)) + SKIPLABEL);

		/* write magic info */
		tmpval = MYMAGIC;
		if (pwrite(fd, &tmpval, 8, MAGOFF) != 8)
			err(1, "magic write failed");
		tmpval = htole64(sb_file.st_size);
		if (pwrite(fd, &tmpval, 8, SIZEOFF) != 8)
			err(1, "size write failed");

		close(fd);

		nbytes = sb_file.st_size;
	} else {
		uint64_t tmpval;

		fd = open(cgd_file, RFLAGS);
		if (fd == -1)
			err(1, "image open failed");

		if (pread(fd, &tmpval, 8, MAGOFF) != 8)
			err(1, "magic read failed");
		if (tmpval != MYMAGIC)
			errx(1, "%s is not a valid image", img_file);
		if (pread(fd, &tmpval, 8, SIZEOFF) != 8)
			errx(1, "size read failed");
		close(fd);

		nbytes = le64toh(tmpval);
	}

	rump_init();
	if ((error = rump_etfs_register("/cryptfile", cgd_file,
	    RUMP_ETFS_BLK)) != 0) {
		printf("etfs: %d\n", error);
		exit(1);
	}

	the_argv[0] = "cgdconfig";
	the_argv[1] = "cgd0";
	the_argv[2] = "/cryptfile";
	the_argv[3] = "./cgd.conf";
	the_argv[4] = NULL;
	error = cgdconfig(4, the_argv);
	if (error) {
		fprintf(stderr, "cgdconfig failed: %d (%s)\n",
		    error, strerror(error));
		exit(1);
	}

	fd = open(img_file, readmode ? WFLAGS : RFLAGS, 0755);
	if (fd == -1)
		err(1, "fd");
	fd_r = rump_sys_open("/dev/rcgd0d", O_RDWR, 0755);
	if (fd_r == -1)
		err(1, "fd_r");
	if (rump_sys_lseek(fd_r, SKIPLABEL, SEEK_SET) == -1)
		err(1, "rump lseek");

	if (readmode) {
		doxfer(fd_r, fd, nbytes, rump_sys_read, write, 0);
	} else {
		doxfer(fd, fd_r, sb_file.st_size, read, rump_sys_write, 1);
	}

	return 0;
}
