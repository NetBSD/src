/*	$NetBSD: popen.c,v 1.5.4.1 2018/06/25 07:25:12 pgoyette Exp $	*/

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

/* this came out of the ftpd sources; it's been modified to avoid the
 * globbing stuff since we don't need it.  also execvp instead of execv.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static sccsid[] = "@(#)popen.c	8.3 (Berkeley) 4/6/94";
static char rcsid[] = "Id: popen.c,v 1.6 2003/02/16 04:40:01 vixie Exp";
#else
__RCSID("$NetBSD: popen.c,v 1.5.4.1 2018/06/25 07:25:12 pgoyette Exp $");
#endif
#endif /* not lint */

#include "cron.h"

#define MAX_ARGV	100
#define MAX_GARGV	1000

/*
 * Special version of popen which avoids call to shell.  This ensures noone
 * may create a pipe to a hidden program as a side effect of a list or dir
 * command.
 */
static PID_T *pids;

FILE *
cron_popen(char *program, const char *type, struct passwd *pw) {
	char *cp;
	FILE *iop;
	int argc, pdes[2];
	PID_T pid;
	char *argv[MAX_ARGV];

	if ((*type != 'r' && *type != 'w') || type[1] != '\0')
		return (NULL);

	if (!pids) {
		size_t len;
		long fds;
		if ((fds = sysconf(_SC_OPEN_MAX)) <= 0)
			return (NULL);
		len = (size_t)fds * sizeof(*pids);
		if ((pids = malloc(len)) == NULL)
			return (NULL);
		(void)memset(pids, 0, len);
	}
	if (pipe(pdes) < 0)
		return (NULL);

	/* break up string into pieces */
	for (argc = 0, cp = program; argc < MAX_ARGV - 1; cp = NULL)
		if (!(argv[argc++] = strtok(cp, " \t\n")))
			break;
	argv[MAX_ARGV-1] = NULL;

	switch (pid = vfork()) {
	case -1:			/* error */
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		return (NULL);
		/* NOTREACHED */
	case 0:				/* child */
		if (pw) {
			if (setsid() == -1)
				warn("setsid() failed for %s", pw->pw_name);
#ifdef LOGIN_CAP
			if (setusercontext(0, pw, pw->pw_uid, LOGIN_SETALL) < 0)
			{
				warn("setusercontext() failed for %s",
				    pw->pw_name);
				_exit(ERROR_EXIT);
			}
#else
			if (setgid(pw->pw_gid) < 0 ||
			    initgroups(pw->pw_name, pw->pw_gid) < 0) {
				warn("unable to set groups for %s",
				    pw->pw_name);
				_exit(ERROR_EXIT);
			}
#if (defined(BSD)) && (BSD >= 199103)
			if (setlogin(pw->pw_name) < 0) {
				warn("setlogin() failed for %s",
				    pw->pw_name);
				_exit(ERROR_EXIT);
			}
#endif /* BSD */
#ifdef USE_PAM
			if (!cron_pam_setcred())
				_exit(1);
			cron_pam_child_close();
#endif
			if (setuid(pw->pw_uid)) {
				warn("unable to set uid for %s", pw->pw_name);
				_exit(ERROR_EXIT);
			}
#endif /* LOGIN_CAP */
		}
		if (*type == 'r') {
			if (pdes[1] != STDOUT) {
				(void)dup2(pdes[1], STDOUT);
				(void)close(pdes[1]);
			}
			(void)dup2(STDOUT, STDERR);	/* stderr too! */
			(void)close(pdes[0]);
		} else {
			if (pdes[0] != STDIN) {
				(void)dup2(pdes[0], STDIN);
				(void)close(pdes[0]);
			}
			(void)close(pdes[1]);
		}
		(void)execvp(argv[0], argv);
		_exit(ERROR_EXIT);
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

	return (iop);
}

static int
cron_finalize(FILE *iop, int sig) {
	int fdes;
	PID_T pid;
	WAIT_T status;
	sigset_t sset, osset;

	/*
	 * pclose returns -1 if stream is not associated with a
	 * `popened' command, or, if already `pclosed'.
	 */
	if (pids == 0 || pids[fdes = fileno(iop)] == 0)
		return (-1);

	if (sig) {
		if (kill(pids[fdes], sig) == -1)
			return -1;
	} else {
		(void)fclose(iop);
	}
	(void)sigemptyset(&sset);
	(void)sigaddset(&sset, SIGINT);
	(void)sigaddset(&sset, SIGQUIT);
	(void)sigaddset(&sset, SIGHUP);
	(void)sigprocmask(SIG_BLOCK, &sset, &osset);
	while ((pid = waitpid(pids[fdes], &status, 0)) < 0 && errno == EINTR)
		continue;
	(void)sigprocmask(SIG_SETMASK, &osset, NULL);
	if (sig)
		(void)fclose(iop);
	pids[fdes] = 0;
	if (pid < 0)
		return pid;
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	else
		return WTERMSIG(status);
}

int
cron_pclose(FILE *iop) {
	return cron_finalize(iop, 0);
}

int
cron_pabort(FILE *iop) {
	int e = cron_finalize(iop, SIGKILL);
	return e == SIGKILL ? 0 : e;
}
