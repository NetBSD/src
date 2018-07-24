/*	$NetBSD: tprof.c,v 1.13 2018/07/24 09:50:37 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__RCSID("$NetBSD: tprof.c,v 1.13 2018/07/24 09:50:37 maxv Exp $");
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
#include "tprof.h"

#define	_PATH_TPROF	"/dev/tprof"

int devfd;
int outfd;

static void tprof_list(int, char **);
static void tprof_monitor(int, char **) __dead;

static struct cmdtab {
	const char *label;
	bool takesargs;
	bool argsoptional;
	void (*func)(int, char **);
} const tprof_cmdtab[] = {
	{ "list",	false, false, tprof_list },
	{ "monitor",	true,  false, tprof_monitor },
	{ "analyze",	true,  true,  tprof_analyze },
	{ NULL,		false, false, NULL },
};

__dead static void
usage(void)
{

	fprintf(stderr, "%s op [arguments]\n", getprogname());
	fprintf(stderr, "\n");
	fprintf(stderr, "\tlist\n");
	fprintf(stderr, "\t\tList the available events.\n");
	fprintf(stderr, "\tmonitor -e name:option [-o outfile] command\n");
	fprintf(stderr, "\t\tMonitor the event 'name' with option 'option'\n"
	    "\t\tcounted during the execution of 'command'.\n");
	fprintf(stderr, "\tanalyze [-CkLPs] [-p pid] file\n");
	fprintf(stderr, "\t\tAnalyze the samples of the file 'file'.\n");

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

static void
tprof_list(int argc, char **argv)
{
	tprof_event_list();
}

static void
tprof_monitor(int argc, char **argv)
{
	const char *outfile = "tprof.out";
	struct tprof_param param;
	struct tprof_stat ts;
	pid_t pid;
	pthread_t pt;
	int ret, ch;
	char *tokens[2];

	memset(&param, 0, sizeof(param));

	while ((ch = getopt(argc, argv, "o:e:")) != -1) {
		switch (ch) {
		case 'o':
			outfile = optarg;
			break;
		case 'e':
			tokens[0] = strtok(optarg, ":");
			tokens[1] = strtok(NULL, ":");
			if (tokens[1] == NULL)
				usage();
			tprof_event_lookup(tokens[0], &param);
			if (strchr(tokens[1], 'u'))
				param.p_flags |= TPROF_PARAM_USER;
			if (strchr(tokens[1], 'k'))
				param.p_flags |= TPROF_PARAM_KERN;
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

	if (param.p_flags == 0) {
		usage();
	}

	outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (outfd == -1) {
		err(EXIT_FAILURE, "%s", outfile);
	}

	ret = ioctl(devfd, TPROF_IOC_START, &param);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_START");
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

	ret = pthread_create(&pt, NULL, process_samples, NULL);
	if (ret != 0) {
		errx(1, "pthread_create: %s", strerror(ret));
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

int
main(int argc, char *argv[])
{
	struct tprof_info info;
	const struct cmdtab *ct;
	int ret;

	setprogname(argv[0]);
	argv += 1, argc -= 1;

	devfd = open(_PATH_TPROF, O_RDWR);
	if (devfd == -1) {
		err(EXIT_FAILURE, "%s", _PATH_TPROF);
	}

	ret = ioctl(devfd, TPROF_IOC_GETINFO, &info);
	if (ret == -1) {
		err(EXIT_FAILURE, "TPROF_IOC_GETINFO");
	}
	if (info.ti_version != TPROF_VERSION) {
		errx(EXIT_FAILURE, "version mismatch: version=%d, expected=%d",
		    info.ti_version, TPROF_VERSION);
	}
	if (tprof_event_init(info.ti_ident) == -1) {
		errx(EXIT_FAILURE, "cpu not supported");
	}

	if (argc == 0)
		usage();

	for (ct = tprof_cmdtab; ct->label != NULL; ct++) {
		if (strcmp(argv[0], ct->label) == 0) {
			if (!ct->argsoptional &&
			    ((ct->takesargs == 0) ^ (argv[1] == NULL)))
			{
				usage();
			}
			(*ct->func)(argc, argv);
			break;
		}
	}
	if (ct->label == NULL) {
		usage();
	}
}
