/* $NetBSD: mkdep.c,v 1.18 2003/11/10 17:56:38 dsl Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1999 The NetBSD Foundation, Inc.\n\
	All rights reserved.\n");
__RCSID("$NetBSD: mkdep.c,v 1.18 2003/11/10 17:56:38 dsl Exp $");
#endif /* not lint */

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "findcc.h"

#define DEFAULT_PATH		_PATH_DEFPATH
#define DEFAULT_FILENAME	".depend"


static inline void *
deconst(const void *p)
{
	return (const char *)p - (const char *)0 + (char *)0;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-acopq] [-f file] [-s suffix_list] flags file ...\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static int
run_cc(int argc, char **argv, const char **fname)
{
	const char *CC, *pathname, *tmpdir;
	static char tmpfilename[MAXPATHLEN];
	char **args;
	int tmpfd;
	pid_t pid, cpid;
	int status;

	if ((CC = getenv("CC")) == NULL)
		CC = DEFAULT_CC;
	if ((pathname = findcc(CC)) == NULL)
		if (!setenv("PATH", DEFAULT_PATH, 1))
			pathname = findcc(CC);
	if (pathname == NULL)
		err(EXIT_FAILURE, "%s: not found", CC);
	if ((args = malloc((argc + 3) * sizeof(char *))) == NULL)
		err(EXIT_FAILURE, "malloc");

	args[0] = deconst(CC);
	args[1] = deconst("-M");
	(void)memcpy(&args[2], argv, (argc + 1) * sizeof(char *));

	if ((tmpdir = getenv("TMPDIR")) == NULL)
		tmpdir = _PATH_TMP;
	(void)snprintf(tmpfilename, sizeof (tmpfilename), "%s/%s", tmpdir,
	    "mkdepXXXXXX");
	if ((tmpfd = mkstemp(tmpfilename)) < 0) {
		warn("unable to create temporary file %s", tmpfilename);
		exit(EXIT_FAILURE);
	}
	(void)unlink(tmpfilename);
	*fname = tmpfilename;

	switch (cpid = vfork()) {
	case 0:
		(void)dup2(tmpfd, STDOUT_FILENO);
		(void)close(tmpfd);

		(void)execv(pathname, args);
		_exit(EXIT_FAILURE);
		/* NOTREACHED */

	case -1:
		err(EXIT_FAILURE, "unable to fork");
	}

	while (((pid = wait(&status)) != cpid) && (pid >= 0))
		continue;

	if (status)
		errx(EXIT_FAILURE, "compile failed.");

	return tmpfd;
}

int
main(int argc, char **argv)
{
	int 	aflag, dflag, oflag, qflag;
	const char *filename;
	int	dependfile;
	char	*buf, *ptr, *line, *suf, *colon, *eol;
	int	ok_ind, ch;
	int	sz;
	int	fd;
	const char *fname;
	const char *suffixes = NULL, *s, *s1;

	setlocale(LC_ALL, "");
	setprogname(argv[0]);

	aflag = O_WRONLY | O_APPEND | O_CREAT | O_TRUNC;
	dflag = 0;
	oflag = 0;
	qflag = 0;
	filename = DEFAULT_FILENAME;
	dependfile = -1;

	opterr = 0;	/* stop getopt() bleating about errors. */
	ok_ind = 1;
	for (; (ch = getopt(argc, argv, "adf:opqs:")) != -1; ok_ind = optind) {
		switch (ch) {
		case 'a':	/* Append to output file */
			aflag &= ~O_TRUNC;
			continue;
		case 'd':	/* Process *.d files (don't run cc -M) */
			dflag = 1;
			opterr = 1;
			continue;
		case 'f':	/* Name of output file */
			filename = optarg;
			continue;
		case 'o':	/* Mark dependant files .OPTIONAL */
			oflag = 1;
			continue;
		case 'p':	/* Program mode (x.o: -> x:) */
			suffixes = "";
			continue;
		case 'q':	/* Quiet */
			qflag = 1;
			continue;
		case 's':	/* Suffix list */
			suffixes = optarg;
			continue;
		default:
			if (dflag)
				usage();
			/* Unknown arguments are passed to "${CC} -M" */
			break;
		}
		break;
	}

	argc -= ok_ind;
	argv += ok_ind;
	if (argc == 0 && !dflag)
		usage();

	dependfile = open(filename, aflag, 0666);
	if (dependfile == -1)
		err(EXIT_FAILURE, "unable to %s to file %s\n",
		    aflag & O_TRUNC ? "write" : "append", filename);

	for (; *argv != NULL; argv++) {
		if (dflag) {
			fname = *argv;
			fd = open(fname, O_RDONLY, 0);
			if (fd == -1) {
				if (!qflag)
					warn("ignoring %s", fname);
				continue;
			}
		} else {
			fd = run_cc(argc, argv, &fname);
			/* consume all args... */
			argv += argc - 1;
		}

		sz = lseek(fd, 0, SEEK_END);
		if (sz == 0) {
			close(fd);
			continue;
		}
		buf = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
		close(fd);

		if (buf == MAP_FAILED)
			err(EXIT_FAILURE, "unable to mmap file %s",
			    *argv);

		/* Remove leading "./" from filenames */
		for (ptr = buf; (ptr = strstr(ptr, "./")) != NULL; ptr += 2) {
			if (ptr == buf)
				continue;
			if (!isspace((unsigned char)ptr[-1]))
				continue;
			ptr[0] = ' ';
			ptr[1] = ' ';
		}

		line = eol = buf;
		for (; (eol = strchr(eol, '\n')) != NULL; line = eol) {
			eol++;
			if (line == eol - 1)
				/* empty line - ignore */
				continue;
			if (eol[-2] == '\\')
				/* Assemble continuation lines */
				continue;
			colon = strchr(line, ':');
			if (colon > eol)
				colon = NULL;
			if (colon != NULL && suffixes != NULL) {
				/* Find the .o: */
				for (suf = colon - 2; ; suf--) {
					if (suf <= line) {
						colon = NULL;
						break;
					}
					if (isspace((unsigned char)suf[1]))
						continue;
					if (suf[0] != '.' || suf[1] != 'o')
						/* not a file.o: line */
						colon = NULL;
					break;
				}
			}
			if (colon == NULL) {
				/* No dependency - just transcribe line */
				write(dependfile, line, eol - line);
				line = eol;
				continue;
			}
			if (suffixes != NULL) {
				for (s = suffixes; ; s = s1 + 1) {
					s1 = strpbrk(s, ", ");
					if (s1 == NULL)
						s1 = s + strlen(s);
					write(dependfile, line, suf - line);
					write(dependfile, s, s1 - s);
					if (*s1 == 0)
						break;
					write(dependfile, " ", 1);
				}
				write(dependfile, colon, eol - colon);
			} else
				write(dependfile, line, eol - line);

			if (oflag) {
				write(dependfile, ".OPTIONAL", 9);
				write(dependfile, colon, eol - colon);
			}
		}
		munmap(buf, sz);
	}
	close(dependfile);

	exit(EXIT_SUCCESS);
}
