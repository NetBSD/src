/*	$NetBSD: popen.c,v 1.14 1999/05/17 15:14:54 lukem Exp $	*/

/*
 * Copyright (c) 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software written by Ken Arnold and
 * published in UNIX Review, Vol. 6, No. 8.
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
 *
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)popen.c	8.3 (Berkeley) 4/6/94";
#else
__RCSID("$NetBSD: popen.c,v 1.14 1999/05/17 15:14:54 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#ifdef KERBEROS5
#include <krb5.h>
#endif

#include "extern.h"

#define INCR	100
/*
 * Special version of popen which avoids call to shell.  This ensures no-one
 * may create a pipe to a hidden program as a side effect of a list or dir
 * command.
 * If stderrfd != -1, then send stderr of a read command there,
 * otherwise close stderr.
 */
static int *pids;
static int fds;

extern int ls_main __P((int, char *[]));

FILE *
ftpd_popen(program, type, stderrfd)
	char *program, *type;
	int stderrfd;
{
	char *cp;
	FILE *iop = NULL;
	int argc, gargc, pdes[2], pid, isls;
	char **pop, **np;
	char **argv = NULL, **gargv = NULL;
	size_t nargc = 100, ngargc = 100;
#ifdef __GNUC__
	(void) &iop;
	(void) &gargc;
	(void) &gargv;
	(void) &argv;
#endif
	isls = 0;

	if ((*type != 'r' && *type != 'w') || type[1])
		return (NULL);

	if (!pids) {
		if ((fds = getdtablesize()) <= 0)
			return (NULL);
		if ((pids = (int *)malloc((u_int)(fds * sizeof(int)))) == NULL)
			return (NULL);
		memset(pids, 0, fds * sizeof(int));
	}
	if (pipe(pdes) < 0)
		return (NULL);

	if ((argv = malloc(nargc * sizeof(char *))) == NULL)
		return NULL;

#define CHECKMORE(c, v, n) \
	if (c >= n + 2) { \
		n += INCR; \
		if ((np = realloc(v, n * sizeof(char *))) == NULL) \
			goto pfree; \
		else \
			v = np; \
	}

	/* break up string into pieces */
	for (argc = 0, cp = program;; cp = NULL) {
		CHECKMORE(argc, argv, nargc)
		if (!(argv[argc++] = strtok(cp, " \t\n")))
			break;
	}

	if ((gargv = malloc(ngargc * sizeof(char *))) == NULL)
		goto pfree;

	/* glob each piece */
	gargv[0] = argv[0];
	for (gargc = argc = 1; argv[argc]; argc++) {
		glob_t gl;
		int flags = GLOB_BRACE|GLOB_NOCHECK|GLOB_TILDE;

		memset(&gl, 0, sizeof(gl));
		if (glob(argv[argc], flags, NULL, &gl)) {
			CHECKMORE(gargc, gargv, ngargc)
			if ((gargv[gargc++] = strdup(argv[argc])) == NULL)
				goto pfree;
		}
		else
			for (pop = gl.gl_pathv; *pop; pop++) {
				CHECKMORE(gargc, gargv, ngargc)
				if ((gargv[gargc++] = strdup(*pop)) == NULL)
					goto pfree;
			}
		globfree(&gl);
	}
	gargv[gargc] = NULL;

	isls = (strcmp(gargv[0], "/bin/ls") == 0);

	pid = isls ? fork() : vfork();
	switch (pid) {
	case -1:			/* error */
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		goto pfree;
		/* NOTREACHED */
	case 0:				/* child */
		if (*type == 'r') {
			if (pdes[1] != STDOUT_FILENO) {
				dup2(pdes[1], STDOUT_FILENO);
				(void)close(pdes[1]);
			}
			if (stderrfd == -1)
				(void)close(STDERR_FILENO);
			else
				dup2(stderrfd, STDERR_FILENO);
			(void)close(pdes[0]);
		} else {
			if (pdes[0] != STDIN_FILENO) {
				dup2(pdes[0], STDIN_FILENO);
				(void)close(pdes[0]);
			}
			(void)close(pdes[1]);
		}
		if (isls) {	/* use internal ls */
			optreset = optind = optopt = 1;
			closelog();
			exit(ls_main(gargc, gargv));
		}
		execv(gargv[0], gargv);
		_exit(1);
	}
	/* parent; assume fdopen can't fail...  */
	if (*type == 'r') {
		iop = fdopen(pdes[0], type);
		(void)close(pdes[1]);
	} else {
		iop = fdopen(pdes[1], type);
		(void)close(pdes[0]);
	}
	pids[fileno(iop)] = pid;

pfree:	if (gargv) {
		for (argc = 1; argc < gargc; argc++)
			free(gargv[argc]);
		free(gargv);
	}
	if (argv)
		free(argv);

	return (iop);
}

int
ftpd_pclose(iop)
	FILE *iop;
{
	int fdes, status;
	pid_t pid;
	sigset_t sigset, osigset;

	/*
	 * pclose returns -1 if stream is not associated with a
	 * `popened' command, or, if already `pclosed'.
	 */
	if (pids == 0 || pids[fdes = fileno(iop)] == 0)
		return (-1);
	(void)fclose(iop);
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	sigaddset(&sigset, SIGHUP);
	sigprocmask(SIG_BLOCK, &sigset, &osigset);
	while ((pid = waitpid(pids[fdes], &status, 0)) < 0 && errno == EINTR)
		continue;
	sigprocmask(SIG_SETMASK, &osigset, NULL);
	pids[fdes] = 0;
	if (pid < 0)
		return (pid);
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	return (1);
}
