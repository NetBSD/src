/*	$NetBSD: popen.c,v 1.28 2003/09/15 22:30:38 cl Exp $	*/

/*
 * Copyright (c) 1988, 1993
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)popen.c	8.3 (Berkeley) 5/3/95";
#else
__RCSID("$NetBSD: popen.c,v 1.28 2003/09/15 22:30:38 cl Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <assert.h>
#include <errno.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "reentrant.h"

#ifdef __weak_alias
__weak_alias(popen,_popen)
__weak_alias(pclose,_pclose)
#endif

#ifdef _REENTRANT
extern rwlock_t __environ_lock;
#endif

static struct pid {
	struct pid *next;
	FILE *fp;
#ifdef _REENTRANT
	int fd;
#endif
	pid_t pid;
} *pidlist; 
	
#ifdef _REENTRANT
static rwlock_t pidlist_lock = RWLOCK_INITIALIZER;
#endif

FILE *
popen(command, type)
	const char *command, *type;
{
	struct pid *cur, *old;
	FILE *iop;
	int pdes[2], pid, twoway, serrno;

	_DIAGASSERT(command != NULL);
	_DIAGASSERT(type != NULL);

#ifdef __GNUC__
	/* This outrageous construct just to shut up a GCC warning. */
	(void) &cur; (void) &twoway; (void) &type;
#endif

	if (strchr(type, '+')) {
		twoway = 1;
		type = "r+";
		if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pdes) < 0)
			return (NULL);
	} else  {
		twoway = 0;
		if ((*type != 'r' && *type != 'w') || type[1] ||
		    (pipe(pdes) < 0)) {
			errno = EINVAL;
			return (NULL);
		}
	}

	if ((cur = malloc(sizeof(struct pid))) == NULL) {
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		errno = ENOMEM;
		return (NULL);
	}

	rwlock_rdlock(&pidlist_lock);
	rwlock_rdlock(&__environ_lock);
	switch (pid = vfork()) {
	case -1:			/* Error. */
		serrno = errno;
		rwlock_unlock(&__environ_lock);
		rwlock_unlock(&pidlist_lock);
		free(cur);
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		errno = serrno;
		return (NULL);
		/* NOTREACHED */
	case 0:				/* Child. */
		/* POSIX.2 B.3.2.2 "popen() shall ensure that any streams
		   from previous popen() calls that remain open in the 
		   parent process are closed in the new child process. */
		for (old = pidlist; old; old = old->next)
#ifdef _REENTRANT
			close(old->fd); /* don't allow a flush */
#else
			close(fileno(old->fp)); /* don't allow a flush */
#endif

		if (*type == 'r') {
			(void)close(pdes[0]);
			if (pdes[1] != STDOUT_FILENO) {
				(void)dup2(pdes[1], STDOUT_FILENO);
				(void)close(pdes[1]);
			}
			if (twoway)
				(void)dup2(STDOUT_FILENO, STDIN_FILENO);
		} else {
			(void)close(pdes[1]);
			if (pdes[0] != STDIN_FILENO) {
				(void)dup2(pdes[0], STDIN_FILENO);
				(void)close(pdes[0]);
			}
		}

		execl(_PATH_BSHELL, "sh", "-c", command, NULL);
		_exit(127);
		/* NOTREACHED */
	}
	rwlock_unlock(&__environ_lock);

	/* Parent; assume fdopen can't fail. */
	if (*type == 'r') {
		iop = fdopen(pdes[0], type);
#ifdef _REENTRANT
		cur->fd = pdes[0];
#endif
		(void)close(pdes[1]);
	} else {
		iop = fdopen(pdes[1], type);
#ifdef _REENTRANT
		cur->fd = pdes[1];
#endif
		(void)close(pdes[0]);
	}

	/* Link into list of file descriptors. */
	cur->fp = iop;
	cur->pid =  pid;
	cur->next = pidlist;
	pidlist = cur;
	rwlock_unlock(&pidlist_lock);

	return (iop);
}

/*
 * pclose --
 *	Pclose returns -1 if stream is not associated with a `popened' command,
 *	if already `pclosed', or waitpid returns an error.
 */
int
pclose(iop)
	FILE *iop;
{
	struct pid *cur, *last;
	int pstat;
	pid_t pid;

	_DIAGASSERT(iop != NULL);

	rwlock_wrlock(&pidlist_lock);

	/* Find the appropriate file pointer. */
	for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
		if (cur->fp == iop)
			break;
	if (cur == NULL) {
		rwlock_unlock(&pidlist_lock);
		return (-1);
	}

	(void)fclose(iop);

	/* Remove the entry from the linked list. */
	if (last == NULL)
		pidlist = cur->next;
	else
		last->next = cur->next;

	rwlock_unlock(&pidlist_lock);

	do {
		pid = waitpid(cur->pid, &pstat, 0);
	} while (pid == -1 && errno == EINTR);

	free(cur);

	return (pid == -1 ? -1 : pstat);
}
