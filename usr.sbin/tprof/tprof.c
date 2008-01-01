/*	$NetBSD: tprof.c,v 1.1 2008/01/01 21:33:26 yamt Exp $	*/

/*-
 * Copyright (c)2008 YAMAMOTO Takashi,
 * All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: tprof.c,v 1.1 2008/01/01 21:33:26 yamt Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/wait.h>

#include <dev/tprof/tprof_ioctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	_PATH_TPROF	"/dev/tprof"

int devfd;
int outfd;

static void
usage(void)
{

	fprintf(stderr, "%s [-c] [-o outfile] command ...\n", getprogname());

	exit(EXIT_FAILURE);
}

static void *
process_samples(void *dummy)
{

	for (;;) {
		char buf[4096];
		const char *cp;
		ssize_t ssz;

		ssz = read(devfd, buf, sizeof(buf));
		if (ssz == -1) {
			err(EXIT_FAILURE, "read");
		}
		if (ssz == 0) {
			break;
		}
		cp = buf;
		while (ssz) {
			ssize_t wsz;

			wsz = write(outfd, cp, ssz);
			if (wsz == -1) {
				err(EXIT_FAILURE, "write");
			}
			ssz -= wsz;
			cp += wsz;
		}
	}
	return NULL;
}

int
main(int argc, char *argv[])
{
	struct tprof_param param;
	struct tprof_stat ts;
	const char *outfile = "tprof.out";
	bool cflag = false;
	pid_t pid;
	pthread_t pt;
	int error;
	int ret;
	int ch;
	int version;

	while ((ch = getopt(argc, argv, "co:")) != -1) {
		switch (ch) {
		case 'c':
			cflag = true;
			break;
		case 'o':
			outfile = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0) {
		usage();
	}

	if (cflag) {
		outfd = STDOUT_FILENO;
	} else {
		outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (outfd == -1) {
			err(EXIT_FAILURE, "%s", optarg);
		}
	}

	devfd = open(_PATH_TPROF, O_RDWR);
	if (devfd == -1) {
		err(EXIT_SUCCESS, "%s", _PATH_TPROF);
	}

	ret = ioctl(devfd, TPROF_IOC_GETVERSION, &version);
	if (ret == -1) {
		err(EXIT_SUCCESS, "TPROF_IOC_GETVERSION");
	}
	if (version != TPROF_VERSION) {
		errx(EXIT_SUCCESS, "version mismatch: version=%d, expected=%d",
		    version, TPROF_VERSION);
	}

	memset(&param, 0, sizeof(param));
	ret = ioctl(devfd, TPROF_IOC_START, &param);
	if (ret == -1) {
		err(EXIT_SUCCESS, "TPROF_IOC_START");
	}

	pid = fork();
	switch (pid) {
	case -1:
		err(EXIT_FAILURE, "fork");
	case 0:
		close(devfd);
		execvp(argv[0], argv);
		_Exit(EXIT_FAILURE);
	}

	signal(SIGINT, SIG_IGN);

	error = pthread_create(&pt, NULL, process_samples, NULL);
	if (error != 0) {
		errx(1, "pthread_create: %s", strerror(error));
	}

	for (;;) {
		int status;

		pid = wait4(-1, &status, 0, NULL);
		if (pid == -1) {
			if (errno == ECHILD) {
				break;
			}
			err(EXIT_FAILURE, "wait4");
		}
		if (pid != 0 && WIFEXITED(status)) {
			break;
		}
	}

	ret = ioctl(devfd, TPROF_IOC_STOP, NULL);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_STOP");
	}

	pthread_join(pt, NULL);

	ret = ioctl(devfd, TPROF_IOC_GETSTAT, &ts);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_GETSTAT");
	}

	fprintf(stderr, "\n%s statistics:\n", getprogname());
	fprintf(stderr, "\tsample %" PRIu64 "\n", ts.ts_sample);
	fprintf(stderr, "\toverflow %" PRIu64 "\n", ts.ts_overflow);
	fprintf(stderr, "\tbuf %" PRIu64 "\n", ts.ts_buf);
	fprintf(stderr, "\temptybuf %" PRIu64 "\n", ts.ts_emptybuf);
	fprintf(stderr, "\tdropbuf %" PRIu64 "\n", ts.ts_dropbuf);
	fprintf(stderr, "\tdropbuf_sample %" PRIu64 "\n", ts.ts_dropbuf_sample);

	exit(EXIT_SUCCESS);
}
