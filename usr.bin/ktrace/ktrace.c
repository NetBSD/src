/*	$NetBSD: ktrace.c,v 1.10 1999/01/11 22:40:01 kleink Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ktrace.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: ktrace.c,v 1.10 1999/01/11 22:40:01 kleink Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ktrace.h"

int	main __P((int, char **));
int	rpid __P((char *));
void	usage __P((void));
int	do_ktrace __P((char *, int, int,int));
void	no_ktrace __P((int));

#ifdef KTRUSS
extern int timestamp, decimal, fancy, tail, maxdata;
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	enum { NOTSET, CLEAR, CLEARALL } clear;
	int append, ch, fd, inherit, ops, pid, pidset, trpoints;
	char *infile, *outfile;

	clear = NOTSET;
	append = ops = pidset = inherit = 0;
	trpoints = DEF_POINTS;
	pid = 0;	/* Appease GCC */

#ifdef KTRUSS
# define OPTIONS "aCce:df:g:ilm:o:p:RTt:"
	outfile = infile = NULL;
#else
# define OPTIONS "aCcdf:g:ip:t:"
	outfile = infile = DEF_TRACEFILE;
#endif

	while ((ch = getopt(argc,argv, OPTIONS)) != -1)
		switch((char)ch) {
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
			setemul(optarg);
			break;
		case 'f':
			infile = optarg;
			break;
#else
		case 'f':
			infile = outfile = optarg;
			break;
#endif
		case 'g':
			pid = -rpid(optarg);
			pidset = 1;
			break;
		case 'i':
			inherit = 1;
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
		case 'p':
			pid = rpid(optarg);
			pidset = 1;
			break;
#ifdef KTRUSS
		case 'R':
			timestamp = 2;	/* relative timestamp */
			break;
		case 'T':
			timestamp = 1;
			break;
#endif
		case 't':
			trpoints = getpoints(optarg);
			if (trpoints < 0) {
				warnx("unknown facility in %s", optarg);
				usage();
			}
			break;
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (!infile && ((pidset && *argv) || (!pidset && !*argv)))
		usage();

#ifdef KTRUSS
	if (infile) {
		dumpfile(infile, 0, trpoints);
		exit(0);
	}
#endif
	if (inherit)
		trpoints |= KTRFAC_INHERIT;

	(void)signal(SIGSYS, no_ktrace);
	if (clear != NOTSET) {
		if (clear == CLEARALL) {
			ops = KTROP_CLEAR | KTRFLAG_DESCEND;
			trpoints = ALL_POINTS;
			pid = 1;
		} else
			ops |= pid ? KTROP_CLEAR : KTROP_CLEARFILE;

		(void)do_ktrace(outfile, ops, trpoints, pid);
		exit(0);
	}

	if (outfile && strcmp(outfile, "-")) {
		if ((fd = open(outfile, O_CREAT | O_WRONLY |
		    (append ? 0 : O_TRUNC), DEFFILEMODE)) < 0)
			err(1, outfile);
		(void)close(fd);
	}

	if (*argv) { 
		(void)do_ktrace(outfile, ops, trpoints, getpid());
		execvp(argv[0], &argv[0]);
		err(1, "exec of '%s' failed", argv[0]);
	} else
		(void)do_ktrace(outfile, ops, trpoints, pid);
	exit(0);
}

int
rpid(p)
	char *p;
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
	return(atoi(p));
}

void
usage()
{
	extern char *__progname;
#ifdef KTRUSS
# define LONG_OPTION "[-e emulation] [-m maxdata] [-o outfile] "
# define SHRT_OPTION "RT"
#else
# define LONG_OPTION ""
# define SHRT_OPTION ""
#endif
	(void)fprintf(stderr,
"Usage:\t%s [-aCcid%s] %s[-f trfile] [-g pgid] [-p pid] [-t [cenisw+]]\n\t%s [-aCcid%s] %s[-f trfile] [-t [cenisw+]] command\n",
	__progname, SHRT_OPTION, LONG_OPTION,
	__progname, SHRT_OPTION, LONG_OPTION);
	exit(1);
}

void
no_ktrace(sig)
        int sig;
{
        (void)fprintf(stderr,
"error:\tktrace() system call not supported in the running kernel\n\tre-compile kernel with 'options KTRACE'\n");
        exit(1);
}

int
do_ktrace(tracefile, ops, trpoints, pid)
	char *tracefile;
	int ops;
	int trpoints;
	int pid;
{
	int ret;

	if (!tracefile || strcmp(tracefile, "-") == 0) {
		int pi[2], nofork, fpid;

		if (pipe(pi) < 0)
			err(1, "pipe(2)");
		fcntl(pi[0], F_SETFD, FD_CLOEXEC|fcntl(pi[0], F_GETFD, 0));
		fcntl(pi[1], F_SETFD, FD_CLOEXEC|fcntl(pi[1], F_GETFD, 0));

		nofork = (pid != getpid());

		if (!nofork)
			fpid = fork();
		else
			fpid = 0;	/* XXX: Gcc */
		if (nofork || !fpid) {
			if (nofork)
				ret = fktrace(pi[1], ops, trpoints, pid);
			else
				close(pi[1]);
#ifdef KTRUSS
			dumpfile(NULL, pi[0], trpoints);
#else
			{
				char	buf[512];
				int	n, cnt = 0;

				while ((n = read(pi[0], buf, sizeof(buf)))>0) {
					write(1, buf, n);
					cnt += n;
				}
			}
#endif
			return 0;
		}
		close(pi[0]);
		ret = fktrace(pi[1], ops, trpoints, pid);
	} else
		ret = ktrace(tracefile, ops, trpoints, pid);
	if (ret < 0)
		err(1, tracefile);
	return ret;
}
