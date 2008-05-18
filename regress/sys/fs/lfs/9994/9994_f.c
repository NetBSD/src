/*	$NetBSD: 9994_f.c,v 1.2.30.1 2008/05/18 12:30:47 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro <iwamoto@sat.t.u-tokyo.ac.jp> and Konrad E. Schroder
 * <perseant@hhhh.org>.
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

/*
 * Regression test based on test program submutted with NetBSD PR #9994.
 * Two files are opened, one in the current directory and another in a
 * control directory (in another filesystem).  Random read, write, and
 * truncate operations are performed on both files.
 *
 * An error return indicates that an operation failed on one of the files
 * (control *or* test, either one).
 *
 * A zero return indicates success; the two files should be identical.
 * (This program does not test to make sure that they are identical.)
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define CONTROL_ERROR 1
#define TEST_ERROR    2

int main(int argc, char **argv)
{
	int bsize, quiet, usepid;
	char *hoge, *buf;
	char prop[] = "/\b-\b\\\b|\b";
	int testfd, controlfd;
	int c, i, j, k, n, p;
	char *ctldir;
	unsigned long rseed;
	const char *prog = getprogname();

	bsize = 8192;
	n = 10000;
	ctldir = "/var/tmp";
	quiet = 0;
	usepid = 0;
	rseed = time(0);

	while ((c = getopt(argc, argv, "b:c:n:pqs:")) != -1) {
		switch(c) {
		    case 'b':
			    bsize = atoi(optarg);
			    break;
		    case 'c':
			    ctldir = optarg;
			    break;
		    case 'n':
			    n = atoi(optarg);
			    break;
		    case 'p':
			    ++usepid;
			    break;
		    case 'q':
			    ++quiet;
			    break;
		    case 's':
			    rseed = strtoul(optarg, NULL, 0);
			    break;
		    default:
			    errx(1, "usage: %s [-b bsize] [-c control-dir] [-n count] [-pq] [-s randseed]", prog);
			    /* NOTREACHED */
		}
	}

	srandom(rseed);
	hoge = (char *)malloc(bsize);
	buf = (char *)malloc(bsize);

	for(i = 0; i < bsize; i++)
		hoge[i] = random() & 0xff;

	/* Set up test and control file descriptors */

	if (usepid)
		sprintf(buf, "test.%d", getpid());
	else
		sprintf(buf, "test");
	unlink(buf);
	testfd = open(buf, O_RDWR | O_CREAT, 0644);
	if (testfd < 0) {
		perror("open");
		exit(TEST_ERROR);
	}

	if (usepid)
		sprintf(buf, "%s/control.%d", ctldir, getpid());
	else
		sprintf(buf, "%s/control", ctldir);
	unlink(buf);
	controlfd = open(buf, O_RDWR | O_CREAT, 0644);
	if (controlfd < 0) {
		perror("open");
		exit(CONTROL_ERROR);
	}

	for(i = 0, p = 0; i < n; i++) {
		if (!quiet) {
			/* Draw propeller */
			p = (p + 1) % 4;
			write(1, prop + 2 * p, 2);
		}

		j = random() % 3;
		k = random() & 0x3fffff;
		if (j != 2) {
			if (lseek(testfd, k, SEEK_SET) < 0) {
				perror("read");
				exit(TEST_ERROR);
			}
			if (lseek(controlfd, k, SEEK_SET) < 0) {
				perror("read");
				exit(CONTROL_ERROR);
			}
		}

		switch(j) {
		    case 0:
			    if (read(testfd, buf, bsize) < 0) {
				    perror("read");
				    exit(TEST_ERROR);
			    }
			    if (read(controlfd, buf, bsize) < 0) {
				    perror("read");
				    exit(CONTROL_ERROR);
			    }
			    break;
		    case 1:
			    if (write(testfd, hoge, bsize) < 0) {
				    perror("write");
				    exit(TEST_ERROR);
			    }
			    if (write(controlfd, hoge, bsize) < 0) {
				    perror("write");
				    exit(CONTROL_ERROR);
			    }
			    break;
		    case 2:
			    if (ftruncate(testfd, k) < 0) {
				    perror("write");
				    exit(TEST_ERROR);
			    }
			    if (ftruncate(controlfd, k) < 0) {
				    perror("write");
				    exit(CONTROL_ERROR);
			    }
			    break;
		}
		j = random() % (100 * 1000);
		usleep(j);
	}
	exit(0);
}

