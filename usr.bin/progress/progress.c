/*	$NetBSD: progress.c,v 1.7 2003/02/12 00:58:34 ross Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Hawkinson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
__RCSID("$NetBSD: progress.c,v 1.7 2003/02/12 00:58:34 ross Exp $");
#endif				/* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/ftp.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#define GLOBAL			/* force GLOBAL decls in progressbar.h to be
				 * declared */
#include "progressbar.h"

static void usage(void);
int main(int, char *[]);

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-z] [-f file] [-l length] cmd [args...]\n",
	    getprogname());
	exit(1);
}


int
main(int argc, char *argv[])
{
	static char fb_buf[BUFSIZ];
	char *infile = NULL;
	pid_t pid = 0, gzippid = 0;
	int ch, fd, outpipe[2], waitstat;
	int lflag = 0, zflag = 0;
	ssize_t nr, nw, off;
	struct stat statb;
	struct ttysize ts;

	setprogname(argv[0]);

	/* defaults: Read from stdin, 0 filesize (no completion estimate) */
	fd = STDIN_FILENO;
	filesize = 0;

	while ((ch = getopt(argc, argv, "f:l:z")) != -1)
		switch (ch) {
		case 'f':
			infile = optarg;
			break;
		case 'l':
			lflag++;
			filesize = strtoull(optarg, NULL, 0);
			break;
		case 'z':
			zflag++;
			break;
		default:
		case '?':
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	if (infile && (fd = open(infile, O_RDONLY, 0)) < 0)
		err(1, "%s", infile);

	/* stat() to get the filesize unless overridden, or -z */
	if (!zflag && !lflag && (fstat(fd, &statb) == 0))
		filesize = statb.st_size;

	/* gzip -l the file if we have the name and -z is given */
	if (zflag && !lflag && infile != NULL) {
		FILE *gzipsizepipe;
		char buf[256], *cmd;
		long size;

		/*
		 * Read second word of last line of gzip -l output. Looks like:
		 * % gzip -l ../etc.tgz 
		 *   compressed uncompressed  ratio uncompressed_name
		 * 	 119737       696320  82.8% ../etc.tar
		 */

		asprintf(&cmd, "gzip -l %s", infile);
		if ((gzipsizepipe = popen(cmd, "r")) == NULL)
			err(1, "reading compressed file length");
		for (; fgets(buf, 256, gzipsizepipe) != NULL;)
		    continue;
		sscanf(buf, "%*d %ld", &size);
		filesize = size;
		if (pclose(gzipsizepipe) < 0)
			err(1, "closing compressed file length pipe");
		free(cmd);
	}
	/* Pipe input through gzip -dc if -z is given */
	if (zflag) {
		int gzippipe[2];

		if (pipe(gzippipe) < 0)
			err(1, "gzip pipe");
		gzippid = fork();
		if (gzippid < 0)
			err(1, "fork for gzip");

		if (gzippid) {
			/* parent */
			dup2(gzippipe[0], fd);
			close(gzippipe[0]);
			close(gzippipe[1]);
		} else {
			dup2(gzippipe[1], STDOUT_FILENO);
			dup2(fd, STDIN_FILENO);
			close(gzippipe[0]);
			close(gzippipe[1]);
			if (execlp("gzip", "gzip", "-dc", NULL))
				err(1, "exec()ing gzip");
		}
	}

	/* Initialize progressbar.c's global state */
	bytes = 0;
	progress = 1;
	ttyout = stdout;

	if (ioctl(fileno(ttyout), TIOCGSIZE, &ts) == -1) {
		ttywidth = 80;
	} else
		ttywidth = ts.ts_cols;

	if (pipe(outpipe) < 0)
		err(1, "output pipe");
	pid = fork();
	if (pid < 0)
		err(1, "fork for output pipe");

	if (pid == 0) {
		/* child */
		dup2(outpipe[0], STDIN_FILENO);
		close(outpipe[0]);
		close(outpipe[1]);
		execvp(argv[0], argv);
		err(1, "could not exec %s", argv[0]);
	}
	close(outpipe[0]);

	progressmeter(-1);
	while ((nr = read(fd, fb_buf, BUFSIZ)) > 0)
		for (off = 0; nr; nr -= nw, off += nw, bytes += nw)
			if ((nw = write(outpipe[1], fb_buf + off,
			    (size_t) nr)) < 0)
				err(1, "writing %u bytes to output pipe",
							(unsigned) nr);
	close(outpipe[1]);

	while (pid || gzippid) {
		int deadpid;

		deadpid = wait(&waitstat);

		if (deadpid == pid)
			pid = 0;
		else if (deadpid == gzippid)
			gzippid = 0;
		else if (deadpid != -1)
			continue;
		else if (errno == EINTR)
			continue;
		else break;
	}

	progressmeter(1);
	return 0;
}
