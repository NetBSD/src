/* $NetBSD: mkdep.c,v 1.10 2002/01/31 22:43:55 tv Exp $ */

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

#include <sys/cdefs.h>
#if defined(__COPYRIGHT) && !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1999 The NetBSD Foundation, Inc.\n\
	All rights reserved.\n");
#endif /* not lint */

#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: mkdep.c,v 1.10 2002/01/31 22:43:55 tv Exp $");
#endif /* not lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>
#include <locale.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_CC		"cc"
#define DEFAULT_PATH		_PATH_DEFPATH
#define DEFAULT_FILENAME	".depend"

static void	usage __P((void));
static char    *findcc __P((const char *));
int		main __P((int, char **));

static void
usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-a] [-p] [-f file] flags file ...\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static char *
findcc(progname)
	const char	*progname;
{
	char   *path, *dir, *next;
	char   buffer[MAXPATHLEN];

	if ((next = strchr(progname, ' ')) != NULL) {
		*next = '\0';
	}

	if (strchr(progname, '/') != NULL)
		return access(progname, X_OK) ? NULL : strdup(progname);

	if (((path = getenv("PATH")) == NULL) ||
	    ((path = strdup(path)) == NULL))
		return NULL;

	dir = path;
	while (dir != NULL) {
		if ((next = strchr(dir, ':')) != NULL)
			*next++ = '\0';

		if (snprintf(buffer, sizeof(buffer),
		    "%s/%s", dir, progname) < sizeof(buffer)) {
			if (!access(buffer, X_OK)) {
				free(path);
				return strdup(buffer);
			}
		}
		dir = next;
	}

	free(path);
	return NULL;
}

int
main(argc, argv)
	int     argc;
	char  **argv;
{
	/* LINTED local definition of index */
	int 	aflag, pflag, index, tmpfd, status;
	pid_t	cpid, pid;
	char   *filename, *CC, *pathname, tmpfilename[MAXPATHLEN], **args;
	const char *tmpdir;
	/* LINTED local definition of tmpfile */
	FILE   *tmpfile, *dependfile;
	char	buffer[32768];

	setlocale(LC_ALL, "");
	setprogname(argv[0]);

	aflag = 0;
	pflag = 0;
	filename = DEFAULT_FILENAME;

	/* XXX should use getopt(). */
	for (index = 1; index < argc; index++) {
		if (strcmp(argv[index], "-a") == 0)
			aflag = 1;
		else if (strcmp(argv[index], "-f") == 0) {
			if (++index < argc)
				filename = argv[index];
		} else if (strcmp(argv[index], "-p") == 0)
			pflag = 1;
		else
			break;
	}

	argc -= index;
	argv += index;
	if (argc == 0)
		usage();

	if ((CC = getenv("CC")) == NULL)
		CC = DEFAULT_CC;
	if ((pathname = findcc(CC)) == NULL)
		if (!setenv("PATH", DEFAULT_PATH, 1))
			pathname = findcc(CC);
	if (pathname == NULL) {
		(void)fprintf(stderr, "%s: %s: not found\n", getprogname(), CC);
		exit(EXIT_FAILURE);
	}

	if ((args = malloc((argc + 3) * sizeof(char *))) == NULL) {
		perror(getprogname());
		exit(EXIT_FAILURE);
	}
	args[0] = CC;
	args[1] = "-M";
	(void)memcpy(&args[2], argv, (argc + 1) * sizeof(char *));

	if ((tmpdir = getenv("TMPDIR")) == NULL)
		tmpdir = _PATH_TMP;
	(void)snprintf(tmpfilename, sizeof (tmpfilename), "%s/%s", tmpdir,
	    "mkdepXXXXXX");
	if ((tmpfd = mkstemp (tmpfilename)) < 0) {
		warn("unable to create temporary file %s", tmpfilename);
		exit(EXIT_FAILURE);
	}

	switch (cpid = vfork()) {
	case 0:
		(void)dup2(tmpfd, STDOUT_FILENO);
		(void)close(tmpfd);

		(void)execv(pathname, args);
		_exit(EXIT_FAILURE);
		/* NOTREACHED */

	case -1:
		(void)fprintf(stderr, "%s: unable to fork.\n", getprogname());
		(void)close(tmpfd);
		(void)unlink(tmpfilename);
		exit(EXIT_FAILURE);
	}

	while (((pid = wait(&status)) != cpid) && (pid >= 0))
		continue;

	if (status) {
		(void)fprintf(stderr, "%s: compile failed.\n", getprogname());
		(void)close(tmpfd);
		(void)unlink(tmpfilename);
		exit(EXIT_FAILURE);
	}

	(void)lseek(tmpfd, (off_t)0, SEEK_SET);
	if ((tmpfile = fdopen(tmpfd, "r")) == NULL) {
		(void)fprintf(stderr, "%s: unable to read temporary file %s\n",
		    getprogname(), tmpfilename);
		(void)close(tmpfd);
		(void)unlink(tmpfilename);
		exit(EXIT_FAILURE);
	}

	if ((dependfile = fopen(filename, aflag ? "a" : "w")) == NULL) {
		(void)fprintf(stderr, "%s: unable to %s to file %s\n",
		    getprogname(), aflag ? "append" : "write", filename);
		(void)fclose(tmpfile);
		(void)unlink(tmpfilename);
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, sizeof(buffer), tmpfile) != NULL) {
		char   *ptr;

		if (pflag && ((ptr = strstr(buffer, ".o")) != NULL)) {
			char   *colon;

			colon = ptr + 2;
			while (isspace(*colon)) colon++;
			if (*colon == ':')
				(void)strcpy(ptr, colon);
		}

		ptr = buffer;
		while (*ptr) {
			if (isspace(*ptr++))
				if ((ptr[0] == '.') && (ptr[1] == '/'))
					(void)strcpy(ptr, ptr + 2);
		}

		(void)fputs(buffer, dependfile);
	}

	(void)fclose(dependfile);
	(void)fclose(tmpfile);
	(void)unlink(tmpfilename);

	exit(EXIT_SUCCESS);
}
