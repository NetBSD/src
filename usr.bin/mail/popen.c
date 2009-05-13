/*	$NetBSD: popen.c,v 1.24.14.1 2009/05/13 19:19:56 jym Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
#if 0
static char sccsid[] = "@(#)popen.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: popen.c,v 1.24.14.1 2009/05/13 19:19:56 jym Exp $");
#endif
#endif /* not lint */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <util.h>
#include <sys/wait.h>

#include "rcv.h"
#include "extern.h"
#include "sig.h"

#define READ 0
#define WRITE 1

struct fp {
	FILE *fp;
	int pipe;
	pid_t pid;
	struct fp *link;
};
static struct fp *fp_head;

struct child {
	pid_t pid;
	char done;
	char free;
	int status;
	struct child *link;
};
static struct child *child, *child_freelist = NULL;


#if 0	/* XXX - debugging stuff.  This should go away eventually! */
static void
show_one_file(FILE *fo, struct fp *fpp)
{
	(void)fprintf(fo, ">>> fp: %p,  pipe: %d,  pid: %d,  link: %p\n",
	    fpp->fp, fpp->pipe, fpp->pid, fpp->link);
}

void show_all_files(FILE *fo);
__unused
PUBLIC void
show_all_files(FILE *fo)
{
	struct fp *fpp;

	(void)fprintf(fo, ">> FILES\n");
	for (fpp = fp_head; fpp; fpp = fpp->link)
		show_one_file(fo, fpp);
	(void)fprintf(fo, ">> -------\n");
	(void)fflush(fo);
}
#endif /* end debugging stuff */


static void
unregister_file(FILE *fp)
{
	struct fp **pp, *p;

	for (pp = &fp_head; (p = *pp) != NULL; pp = &p->link)
		if (p->fp == fp) {
			*pp = p->link;
			(void)free(p);
			return;
		}
	errx(1, "Invalid file pointer");
}

PUBLIC void
register_file(FILE *fp, int pipefd, pid_t pid)
{
	struct fp *fpp;

	fpp = emalloc(sizeof(*fpp));
	fpp->fp = fp;
	fpp->pipe = pipefd;
	fpp->pid = pid;
	fpp->link = fp_head;
	fp_head = fpp;
}

PUBLIC FILE *
Fopen(const char *fn, const char *mode)
{
	FILE *fp;

	if ((fp = fopen(fn, mode)) != NULL) {
		register_file(fp, 0, 0);
		(void)fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	}
	return fp;
}

PUBLIC FILE *
Fdopen(int fd, const char *mode)
{
	FILE *fp;

	if ((fp = fdopen(fd, mode)) != NULL) {
		register_file(fp, 0, 0);
		(void)fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	}
	return fp;
}

PUBLIC int
Fclose(FILE *fp)
{

	if (fp == NULL)
		return 0;

	unregister_file(fp);
	return fclose(fp);
}

PUBLIC void
prepare_child(sigset_t *nset, int infd, int outfd)
{
	int i;
	sigset_t eset;

	/*
	 * All file descriptors other than 0, 1, and 2 are supposed to be
	 * close-on-exec.
	 */
	if (infd > 0) {
		(void)dup2(infd, 0);
	} else if (infd != 0) {
		/* we don't want the child stealing my stdin input */
		(void)close(0);
		(void)open(_PATH_DEVNULL, O_RDONLY, 0);
	}
	if (outfd >= 0 && outfd != 1)
		(void)dup2(outfd, 1);

	if (nset != NULL) {
		for (i = 1; i < NSIG; i++) {
			if (sigismember(nset, i))
				(void)signal(i, SIG_IGN);
		}
		if (!sigismember(nset, SIGINT))
			(void)signal(SIGINT, SIG_DFL);
		(void)sigemptyset(&eset);
		(void)sigprocmask(SIG_SETMASK, &eset, NULL);
	}
}

/*
 * Run a command without a shell, with optional arguments and splicing
 * of stdin (-1 means none) and stdout.  The command name can be a sequence
 * of words.
 * Signals must be handled by the caller.
 * "nset" contains the signals to ignore in the new process.
 * SIGINT is enabled unless it's in "nset".
 */
static pid_t
start_commandv(const char *cmd, sigset_t *nset, int infd, int outfd,
    va_list args)
{
	pid_t pid;

	sig_check();
	if ((pid = fork()) < 0) {
		warn("fork");
		return -1;
	}
	if (pid == 0) {
		char *argv[100];
		size_t i;

		i = getrawlist(cmd, argv, (int)__arraycount(argv));
		while (i < __arraycount(argv) - 1 &&
		    (argv[i++] = va_arg(args, char *)) != NULL)
			continue;
		argv[i] = NULL;
		prepare_child(nset, infd, outfd);
		(void)execvp(argv[0], argv);
		warn("%s", argv[0]);
		_exit(1);
	}
	return pid;
}

PUBLIC int
start_command(const char *cmd, sigset_t *nset, int infd, int outfd, ...)
{
	va_list args;
	int r;

	va_start(args, outfd);
	r = start_commandv(cmd, nset, infd, outfd, args);
	va_end(args);
	return r;
}

PUBLIC FILE *
Popen(const char *cmd, const char *mode)
{
	int p[2];
	int myside, hisside, fd0, fd1;
	pid_t pid;
	sigset_t nset;
	FILE *fp;
	char *shellcmd;

	if (pipe(p) < 0)
		return NULL;
	(void)fcntl(p[READ], F_SETFD, FD_CLOEXEC);
	(void)fcntl(p[WRITE], F_SETFD, FD_CLOEXEC);
	if (*mode == 'r') {
		myside = p[READ];
		hisside = fd0 = fd1 = p[WRITE];
	} else {
		myside = p[WRITE];
		hisside = fd0 = p[READ];
		fd1 = -1;
	}
	(void)sigemptyset(&nset);
	if ((shellcmd = value(ENAME_SHELL)) == NULL)
		shellcmd = __UNCONST(_PATH_CSHELL);
	pid = start_command(shellcmd, &nset, fd0, fd1, "-c", cmd, NULL);
	if (pid < 0) {
		(void)close(p[READ]);
		(void)close(p[WRITE]);
		return NULL;
	}
	(void)close(hisside);
	if ((fp = fdopen(myside, mode)) != NULL)
		register_file(fp, 1, pid);
	return fp;
}

static struct child *
findchild(pid_t pid, int dont_alloc)
{
	struct child **cpp;

	for (cpp = &child; *cpp != NULL && (*cpp)->pid != pid;
	     cpp = &(*cpp)->link)
			continue;
	if (*cpp == NULL) {
		if (dont_alloc)
			return NULL;
		if (child_freelist) {
			*cpp = child_freelist;
			child_freelist = (*cpp)->link;
		} else
			*cpp = emalloc(sizeof(**cpp));

		(*cpp)->pid = pid;
		(*cpp)->done = (*cpp)->free = 0;
		(*cpp)->link = NULL;
	}
	return *cpp;
}

static void
delchild(struct child *cp)
{
	struct child **cpp;

	for (cpp = &child; *cpp != cp; cpp = &(*cpp)->link)
		continue;
	*cpp = cp->link;
	cp->link = child_freelist;
	child_freelist = cp;
}

/*
 * Wait for a specific child to die.
 */
PUBLIC int
wait_child(pid_t pid)
{
	struct child *cp;
	sigset_t nset, oset;
	pid_t rv = 0;

	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, SIGCHLD);
	(void)sigprocmask(SIG_BLOCK, &nset, &oset);
	/*
	 * If we have not already waited on the pid (via sigchild)
	 * wait on it now.  Otherwise, use the wait status stashed
	 * by sigchild.
	 */
	cp = findchild(pid, 1);
	if (cp == NULL || !cp->done)
		rv = waitpid(pid, &wait_status, 0);
	else
		wait_status = cp->status;
	if (cp != NULL)
		delchild(cp);
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	if (rv == -1 || (WIFEXITED(wait_status) && WEXITSTATUS(wait_status)))
		return -1;
	else
		return 0;
}

static pid_t
file_pid(FILE *fp)
{
	struct fp *p;

	for (p = fp_head; p; p = p->link)
		if (p->fp == fp)
			return p->pid;
	errx(1, "Invalid file pointer");
	/*NOTREACHED*/
}

PUBLIC int
Pclose(FILE *ptr)
{
	int i;
	sigset_t nset, oset;

	if (ptr == NULL)
		return 0;

	i = file_pid(ptr);
	unregister_file(ptr);
	(void)fclose(ptr);
	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, SIGINT);
	(void)sigaddset(&nset, SIGHUP);
	(void)sigprocmask(SIG_BLOCK, &nset, &oset);
	i = wait_child(i);
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	return i;
}

PUBLIC void
close_all_files(void)
{
	while (fp_head)
		if (fp_head->pipe)
			(void)Pclose(fp_head->fp);
		else
			(void)Fclose(fp_head->fp);
}

PUBLIC FILE *
last_registered_file(int last_pipe)
{
	struct fp *fpp;

	if (last_pipe == 0)
		return fp_head ? fp_head->fp : NULL;

	for (fpp = fp_head; fpp; fpp = fpp->link)
		if (fpp->pipe)
			return fpp->fp;
	return NULL;
}

PUBLIC void
close_top_files(FILE *fp_stop)
{
	while (fp_head && fp_head->fp != fp_stop)
		if (fp_head->pipe)
			(void)Pclose(fp_head->fp);
		else
			(void)Fclose(fp_head->fp);
}

#ifdef MIME_SUPPORT
PUBLIC void
flush_files(FILE *fo, int only_pipes)
{
	struct fp *fpp;

	if (fo)
		(void)fflush(fo);

	for (fpp = fp_head; fpp; fpp = fpp->link)
		if (!only_pipes || fpp->pipe)
			(void)fflush(fpp->fp);

	(void)fflush(stdout);
}
#endif /* MIME_SUPPORT */

static int
wait_command(pid_t pid)
{

	if (wait_child(pid) < 0) {
		(void)puts("Fatal error in process.");
		return -1;
	}
	return 0;
}

PUBLIC int
run_command(const char *cmd, sigset_t *nset, int infd, int outfd, ...)
{
	pid_t pid;
	va_list args;
	int rval;

#ifdef BROKEN_EXEC_TTY_RESTORE
	struct termios ttybuf;
	int tcrval;
	/*
	 * XXX - grab the tty settings as currently they can get
	 * trashed by emacs-21 when suspending with bash-3.2.25 as the
	 * shell.
	 *
	 * 1) from the mail editor, start "emacs -nw" (21.4)
	 * 2) suspend emacs to the shell (bash 3.2.25)
	 * 3) resume emacs
	 * 4) exit emacs back to the mail editor
	 * 5) discover the tty is screwed: the mail editor is no
	 *    longer receiving characters
	 *
	 * - This occurs on both i386 and amd64.
	 * - This did _NOT_ occur before 4.99.10.
	 * - This does _NOT_ occur if the editor is vi(1) or if the shell
	 *   is /bin/sh.
	 * - This _DOES_ happen with the old mail(1) from 2006-01-01 (long
	 *   before my changes).
	 *
	 * This is the commit that introduced this "feature":
	 * http://mail-index.netbsd.org/source-changes/2007/02/09/0020.html
	 */
	if ((tcrval = tcgetattr(fileno(stdin), &ttybuf)) == -1)
		warn("tcgetattr");
#endif
	va_start(args, outfd);
	pid = start_commandv(cmd, nset, infd, outfd, args);
	va_end(args);
	if (pid < 0)
		return -1;
	rval = wait_command(pid);
#ifdef BROKEN_EXEC_TTY_RESTORE
	if (tcrval != -1 && tcsetattr(fileno(stdin), TCSADRAIN, &ttybuf) == -1)
		warn("tcsetattr");
#endif
	return rval;

}

/*ARGSUSED*/
PUBLIC void
sigchild(int signo __unused)
{
	pid_t pid;
	int status;
	struct child *cp;
	int save_errno;

	save_errno = errno;
	while ((pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0) {
		cp = findchild(pid, 1);	/* async-signal-safe: we don't alloc */
		if (!cp)
			continue;
		if (cp->free)
			delchild(cp);	/* async-signal-safe: list changes */
		else {
			cp->done = 1;
			cp->status = status;
		}
	}
	errno = save_errno;
}

/*
 * Mark a child as don't care.
 */
PUBLIC void
free_child(pid_t pid)
{
	struct child *cp;
	sigset_t nset, oset;

	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, SIGCHLD);
	(void)sigprocmask(SIG_BLOCK, &nset, &oset);
	if ((cp = findchild(pid, 0)) != NULL) {
		if (cp->done)
			delchild(cp);
		else
			cp->free = 1;
	}
	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
}
