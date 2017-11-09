/*	$NetBSD: mime_child.c,v 1.9 2017/11/09 20:27:50 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
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


#ifdef MIME_SUPPORT

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: mime_child.c,v 1.9 2017/11/09 20:27:50 christos Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "def.h"
#include "extern.h"

#ifdef MIME_SUPPORT
#include "mime.h"
#include "mime_child.h"
#endif

/*
 * This module contains the core routines used by mime modules that
 * need to fork children.  Nothing else in the mime modules should be
 * doing a pipe(), fork(), or a call to any popen functions that do.
 * These routines are tied to the file registry in popen.c and hence
 * share much of popen code.
 */

#define READ 0
#define WRITE 1

static int
get_cmd_flags(const char *cmd, const char **cmd_start)
{
	const char *cp;
	int flags;

	flags = 0;
	for (cp = cmd; cp && *cp; cp++)
		switch (*cp) {
		case '+':
			flags |= CMD_FLAG_ALTERNATIVE;
			break;
		case '-':
			flags |= CMD_FLAG_NO_DECODE;
			break;
		case '!':
			flags |= CMD_FLAG_SHELLCMD;
			break;
		default:
			goto done;
		}
 done:
	if (cmd_start)
		*cmd_start = cp;

	return flags;
}


static int
prepare_pipe(sigset_t *nset, int p[2])
{
	if (pipe2(p, O_CLOEXEC) == -1)
		return -1;

	/*
	 * We _must_ ignore SIGINT and SIGPIPE or the child
	 * will end up in our earlier handlers.
	 */
	(void)sigemptyset(nset);
	(void)sigaddset(nset, SIGINT);
	(void)sigaddset(nset, SIGPIPE);
	(void)sigaddset(nset, SIGHUP);
	(void)sigaddset(nset, SIGTSTP);
	(void)sigaddset(nset, SIGTTOU);
	(void)sigaddset(nset, SIGTTIN);

	return 0;
}


PUBLIC int
mime_run_command(const char *cmd, FILE *fo)
{
	sigset_t nset;
	FILE *nfo;
	pid_t pid;
	int p[2];
	int flags;

	if (cmd == NULL)
		return 0;

	flags = get_cmd_flags(cmd, &cmd);
	if (fo == NULL)		/* no output file, just return the flags! */
		return flags;

	if ((flags & CMD_FLAG_SHELLCMD) != 0) {	/* run command under the shell */
		char *cp;
		char *shellcmd;
		if ((shellcmd = value(ENAME_SHELL)) == NULL)
			shellcmd = __UNCONST(_PATH_CSHELL);
		(void)sasprintf(&cp, "%s -c '%s'", shellcmd, cmd);
		cmd = cp;
	}
	if (prepare_pipe(&nset, p) != 0) {
		warn("mime_run_command: prepare_pipe");
		return flags;	/* XXX - this or -1? */
	}
	flush_files(fo, 0); /* flush fo, all registered files, and stdout */

	switch (pid = start_command(cmd, &nset, p[READ], fileno(fo), NULL)) {
	case -1:	/* error */
		/* start_command already did a warn(). */
		warnx("mime_run_command: %s", cmd); /* tell a bit more */
		(void)close(p[READ]);
		(void)close(p[WRITE]);
		return flags;			/* XXX - this or -1? */

	case 0:		/* child */
		assert(/*CONSTCOND*/ 0);	/* a real coding error! */
		/* NOTREACHED */

	default:	/* parent */
		(void)close(p[READ]);

		nfo = fdopen(p[WRITE], "wef");
		if (nfo == NULL) {
			warn("mime_run_command: fdopen");
			(void)close(p[WRITE]);
			warn("fdopen");
			return flags;
		}
		register_file(nfo, 1, pid);
		return flags;
	}
}


PUBLIC void
mime_run_function(void (*fn)(FILE *, FILE *, void *), FILE *fo, void *cookie)
{
	sigset_t nset;
	FILE *nfo;
	pid_t pid;
	int p[2];

	if (prepare_pipe(&nset, p) != 0) {
		warn("mime_run_function: pipe");
		return;
	}
	flush_files(fo, 0); /* flush fo, all registered files, and stdout */

	switch (pid = fork()) {
	case -1:	/* error */
		warn("mime_run_function: fork");
		(void)close(p[READ]);
		(void)close(p[WRITE]);
		return;

	case 0:		/* child */
		(void)close(p[WRITE]);
		prepare_child(&nset, p[READ], fileno(fo));
		fn(stdin, stdout, cookie);
		(void)fflush(stdout);
		_exit(0);
		/* NOTREACHED */

	default:	/* parent */
		(void)close(p[READ]);
		nfo = fdopen(p[WRITE], "wef");
		if (nfo == NULL) {
			warn("run_function: fdopen");
			(void)close(p[WRITE]);
			return;
		}
		register_file(nfo, 1, pid);
		return;
	}
}

#endif /* MIME_SUPPORT */
