/* $NetBSD: mkdep.c,v 1.21 2003/12/07 20:22:49 dsl Exp $ */

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
__RCSID("$NetBSD: mkdep.c,v 1.21 2003/12/07 20:22:49 dsl Exp $");
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

typedef struct opt opt_t;
struct opt {
	opt_t	*left;
	opt_t	*right;
	int	len;
	int	count;
	char	name[4];
};

/* tree of includes for -o processing */
opt_t *opt;
int width;

#define DEFAULT_PATH		_PATH_DEFPATH
#define DEFAULT_FILENAME	".depend"

static void save_for_optional(const char *, const char *);
static int write_optional(int, opt_t *, int);


static inline void *
deconst(const void *p)
{
	return (const char *)p - (const char *)0 + (char *)0;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-adopq] [-f file] [-s suffix_list] flags file ...\n",
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
	char	*buf, *lim, *ptr, *line, *suf, *colon, *eol;
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
	for (;;) {
		ok_ind = optind;
		ch = getopt(argc, argv, "adf:opqs:");
		switch (ch) {
		case -1:
			ok_ind = optind;
			break;
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
			err(EXIT_FAILURE, "unable to mmap file %s", *argv);
		lim = buf + sz - 1;

		/* Remove leading "./" from filenames */
		for (ptr = buf; ptr < lim; ptr++) {
			if (ptr[1] != '.' || ptr[2] != '/'
			    || !isspace((unsigned char)ptr[0]))
				continue;
			ptr[1] = ' ';
			ptr[2] = ' ';
		}

		for (line = eol = buf; eol <= lim;) {
			while (eol <= lim && *eol++ != '\n')
				continue;
			if (line == eol - 1) {
				/* empty line - ignore */
				line = eol;
				continue;
			}
			if (eol[-2] == '\\')
				/* Assemble continuation lines */
				continue;
			for (colon = line; *colon != ':'; colon++) {
				if (colon >= eol) {
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
			s = suffixes;
			if (s != NULL) {
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
						s = NULL;
					break;
				}
			}
			if (s != NULL) {
				for (; ; s = s1 + 1) {
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

			if (oflag)
				save_for_optional(colon + 1, eol);
			line = eol;
		}
		munmap(buf, sz);
	}

	if (oflag && opt != NULL) {
		write(dependfile, ".OPTIONAL:", 10);
		width = 9;
		sz = write_optional(dependfile, opt, 0);
		/* 'depth' is about 39 for an i386 kernel */
		/* fprintf(stderr, "Recursion depth %d\n", sz); */
	}
	close(dependfile);

	exit(EXIT_SUCCESS);
}


/*
 * Only save each file once - the kernel .depend is 3MB and there is
 * no point doubling its size.
 * The data seems to be 'random enough' so the simple binary tree
 * only has a reasonable depth.
 */
static void
save_for_optional(const char *start, const char *limit)
{
	opt_t **l, *n;
	const char *name, *end;
	int c;

	while (start < limit && strchr(" \t\n\\", *start)) 
		start++;
	for (name = start; ; name = end) {
		while (name < limit && strchr(" \t\n\\", *name)) 
			name++;
		for (end = name; end < limit && !strchr(" \t\n\\", *end);)
			end++;
		if (name >= limit)
			break;
		if (end[-1] == 'c' && end[-2] == '.' && name == start)
			/* ignore dependency on the files own .c */
			continue;
		for (l = &opt;;) {
			n = *l;
			if (n == NULL) {
				n = malloc(sizeof *n + (end - name));
				n->left = n->right = 0;
				n->len = end - name;
				n->count = 1;
				n->name[0] = ' ';
				memcpy(n->name + 1, name, end - name);
				*l = n;
				break;
			}
			c = (end - name) - n->len;
			if (c == 0)
				c = memcmp(n->name + 1, name, (end - name));
			if (c == 0) {
				/* Duplicate */
				n->count++;
				break;
			}
			if (c < 0)
				l = &n->left;
			else
				l = &n->right;
		}
	}
}

static int
write_optional(int fd, opt_t *node, int depth)
{
	int d1 = ++depth;

	if (node->left)
		d1 = write_optional(fd, node->left, d1);
	if (width > 76 - node->len) {
		write(fd, " \\\n ", 4);
		width = 1;
	}
	width += 1 + node->len;
	write(fd, node->name, 1 + node->len);
	if (node->right)
		depth = write_optional(fd, node->right, depth);
	return d1 > depth ? d1 : depth;
}
