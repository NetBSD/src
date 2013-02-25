/*	$NetBSD: ktrace.c,v 1.45.8.1 2013/02/25 00:30:36 tls Exp $	*/

/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ktrace.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: ktrace.c,v 1.45.8.1 2013/02/25 00:30:36 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "ktrace.h"

#ifdef KTRUSS
#include "setemul.h"
#endif

static int rpid(char *);
__dead static void usage(void);
static int do_ktrace(const char *, int, int, int, int, int);
__dead static void no_ktrace(int);
static void fclear(int fd, int flag);

#ifdef KTRUSS
extern int timestamp, decimal, fancy, tail, maxdata;
#endif

int
main(int argc, char *argv[])
{
	enum { NOTSET, CLEAR, CLEARALL } clear;
	int block, append, ch, fd, trset, ops, pid, pidset, synclog, trpoints;
	int vers;
	const char *outfile;
#ifdef KTRUSS
	const char *infile;
	const char *emul_name = "netbsd";
#endif

	clear = NOTSET;
	append = ops = pidset = trset = synclog = 0;
	trpoints = 0;
	block = 1;
	vers = 2;
	pid = 0;	/* Appease GCC */

#ifdef KTRUSS
# define OPTIONS "aCce:df:g:ilm:no:p:RTt:v:"
	outfile = infile = NULL;
#else
# define OPTIONS "aCcdf:g:ip:st:v:"
	outfile = DEF_TRACEFILE;
#endif

	while ((ch = getopt(argc, argv, OPTIONS)) != -1)
		switch (ch) {
		case 'a':
			append = 1;
			break;
		case 'C':
			clear = CLEARALL;
			pidset = 1;
			break;
		case 'c':
			clear = CLEAR;
			pidset = 1;
			break;
		case 'd':
			ops |= KTRFLAG_DESCEND;
			break;
#ifdef KTRUSS
		case 'e':
			emul_name = strdup(optarg); /* it's safer to copy it */
			break;
		case 'f':
			infile = optarg;
			break;
#else
		case 'f':
			outfile = optarg;
			break;
#endif
		case 'g':
			pid = -rpid(optarg);
			pidset = 1;
			break;
		case 'i':
			trpoints |= KTRFAC_INHERIT;
			break;
#ifdef KTRUSS
		case 'l':
			tail = 1;
			break;
		case 'm':
			maxdata = atoi(optarg);
			break;
		case 'o':
			outfile = optarg;
			break;
#endif
		case 'n':
			block = 0;
			break;
		case 'p':
			pid = rpid(optarg);
			pidset = 1;
			break;
#ifdef KTRUSS
		case 'R':
			timestamp = 2;	/* relative timestamp */
			break;
#else
		case 's':
			synclog = 1;
			break;
#endif
#ifdef KTRUSS
		case 'T':
			timestamp = 1;
			break;
#endif
		case 't':
			trset = 1;
			trpoints = getpoints(trpoints, optarg);
			if (trpoints < 0) {
				warnx("unknown facility in %s", optarg);
				usage();
			}
			break;
		case 'v':
			vers = atoi(optarg);
			break;
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (!trset)
		trpoints |= clear == NOTSET ? DEF_POINTS : ALL_POINTS;

	if ((pidset && *argv) || (!pidset && !*argv)) {
#ifdef KTRUSS
		if (!infile)
#endif
			usage();
	}

#ifdef KTRUSS
	if (clear == CLEAR && outfile == NULL && pid == 0)
		usage();

	if (infile) {
		dumpfile(infile, 0, trpoints);
		exit(0);
	}

	setemul(emul_name, 0, 0);
#endif

	/*
	 * For cleaner traces, initialize malloc now rather
	 * than in a traced subprocess.
	 */
	free(malloc(1));

	(void)signal(SIGSYS, no_ktrace);
	if (clear != NOTSET) {
		if (clear == CLEARALL) {
			ops = KTROP_CLEAR | KTRFLAG_DESCEND;
			trpoints = ALL_POINTS;
			pid = 1;
		} else
			ops |= pid ? KTROP_CLEAR : KTROP_CLEARFILE;

		(void)do_ktrace(outfile, vers, ops, trpoints, pid, block);
		exit(0);
	}

	if (outfile && strcmp(outfile, "-")) {
		if ((fd = open(outfile, O_CREAT | O_WRONLY |
		    (append ? 0 : O_TRUNC) | (synclog ? 0 : O_SYNC),
		    DEFFILEMODE)) < 0)
			err(EXIT_FAILURE, "%s", outfile);
		(void)close(fd);
	}

	if (*argv) {
#ifdef KTRUSS
		if (do_ktrace(outfile, vers, ops, trpoints, getpid(), block) == 1) {
			execvp(argv[0], &argv[0]);
			err(EXIT_FAILURE, "exec of '%s' failed", argv[0]);
		}
#else
		(void)do_ktrace(outfile, vers, ops, trpoints, getpid(), block);
		execvp(argv[0], &argv[0]);
		err(EXIT_FAILURE, "exec of '%s' failed", argv[0]);
#endif
	} else
		(void)do_ktrace(outfile, vers, ops, trpoints, pid, block);
	return 0;
}

static int
rpid(char *p)
{
	static int first;

	if (first++) {
		warnx("only one -g or -p flag is permitted.");
		usage();
	}
	if (!*p) {
		warnx("illegal process id.");
		usage();
	}
	return (atoi(p));
}

static void
fclear(int fd, int flag)
{
	int oflag = fcntl(fd, F_GETFL, 0);

	if (oflag == -1)
		err(EXIT_FAILURE, "Cannot get file flags");
	if (fcntl(fd, F_SETFL, oflag & ~flag) == -1)
		err(EXIT_FAILURE, "Cannot set file flags");
}

static void
usage(void)
{

#define	TRPOINTS "[AaceilmnSsuvw+-]"
#ifdef KTRUSS
	(void)fprintf(stderr, "usage:\t%s "
	    "[-aCcdilnRT] [-e emulation] [-f infile] [-g pgrp] "
	    "[-m maxdata]\n\t    "
	    "[-o outfile] [-p pid] [-t " TRPOINTS "]\n", getprogname());
	(void)fprintf(stderr, "\t%s "
	    "[-adinRT] [-e emulation] [-m maxdata] [-o outfile]\n\t    "
	    "[-t " TRPOINTS "] [-v vers] command\n",
	    getprogname());
#else
	(void)fprintf(stderr, "usage:\t%s "
	    "[-aCcdins] [-f trfile] [-g pgrp] [-p pid] [-t " TRPOINTS "]\n",
	    getprogname());
	(void)fprintf(stderr, "\t%s "
	    "[-adis] [-f trfile] [-t " TRPOINTS "] command\n",
	    getprogname());
#endif
	exit(1);
}

static const char *ktracefile = NULL;
static void
/*ARGSUSED*/
no_ktrace(int sig)
{

	if (ktracefile)
		(void)unlink(ktracefile);
	(void)errx(EXIT_FAILURE,
	    "ktrace(2) system call not supported in the running"
	    " kernel; re-compile kernel with `options KTRACE'");
}

static int
do_ktrace(const char *tracefile, int vers, int ops, int trpoints, int pid,
    int block)
{
	int ret;
	ops |= vers << KTRFAC_VER_SHIFT;

	if (KTROP(ops) == KTROP_SET &&
	    (!tracefile || strcmp(tracefile, "-") == 0)) {
		int pi[2], dofork;

		if (pipe2(pi, O_CLOEXEC) == -1)
			err(EXIT_FAILURE, "pipe(2)");

		dofork = (pid == getpid());

		if (dofork) {
#ifdef KTRUSS
			/*
			 * Create a child process and trace it.
			 */
			pid = fork();
			if (pid == -1)
				err(EXIT_FAILURE, "fork");
			else if (pid == 0) {
				pid = getpid();
				goto trace_and_exec;
			}
#else
			int fpid;

			/*
			 * Create a dumper process and we will be
			 * traced.
			 */
			fpid = fork();
			if (fpid == -1)
				err(EXIT_FAILURE, "fork");
			else if (fpid != 0)
				goto trace_and_exec;
#endif
			(void)close(pi[1]);
		} else {
			ret = fktrace(pi[1], ops, trpoints, pid);
			if (ret == -1)
				err(EXIT_FAILURE, "fd %d, pid %d",
				    pi[1], pid);
			if (block)
				fclear(pi[1], O_NONBLOCK);
		}
#ifdef KTRUSS
		dumpfile(NULL, pi[0], trpoints);
		waitpid(pid, NULL, 0);
#else
		{
			char	buf[BUFSIZ];
			int	n;

			while ((n =
			    read(pi[0], buf, sizeof(buf))) > 0)
				if (write(STDOUT_FILENO, buf, (size_t)n) == -1)
					warn("write failed");
		}
		if (dofork)
			_exit(0);
#endif
		return 0;

trace_and_exec:
		(void)close(pi[0]);
		ret = fktrace(pi[1], ops, trpoints, pid);
		if (ret == -1)
			err(EXIT_FAILURE, "fd %d, pid %d", pi[1], pid);
		if (block)
			fclear(pi[1], O_NONBLOCK);
	} else {
		ret = ktrace(ktracefile = tracefile, ops, trpoints, pid);
		if (ret == -1)
			err(EXIT_FAILURE, "file %s, pid %d",
			    tracefile != NULL ? tracefile : "NULL", pid);
	}
	return 1;
}
