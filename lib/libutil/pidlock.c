/*	$NetBSD: pidlock.c,v 1.1 1997/10/11 02:56:23 cjs Exp $ */

/*
 * Copyright 1996, 1997 by Curt Sampson <cjs@netbsd.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
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
__RCSID("$NetBSD: pidlock.c,v 1.1 1997/10/11 02:56:23 cjs Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

/*
 * Create a lockfile. Return 0 when locked, -1 on error.
 */
int
pidlock(lockfile, flags, locker, info)
	const char *lockfile;
	int flags;
	pid_t *locker;
	const char *info;
{
	char	tempfile[MAXPATHLEN];
	char	hostname[256];
	pid_t	pid2 = -1;
	int	f;
	char	s[256];
	char	*p;

	/* Build a path to the temporary file. */
	snprintf(s, sizeof(s), "%d", getpid());
	/* XXX This breaks if PIDs can be longer than ten decimal digits. */
	/* XXX But then again, so does the lockfile format. :-) */
	strncpy(tempfile, lockfile, sizeof(tempfile)-26);
	if (strlen(tempfile) < strlen(lockfile))  {
		errno = ENAMETOOLONG;
		return -1;
	}
	strcat(tempfile, ".tmp.");
	strcat(tempfile, s);

	/* Open it, write pid, hostname, info. */
	if ( (f = open(tempfile, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1 )  {
		unlink(tempfile);
		return -1;
	}
	snprintf(s, sizeof(s), "%10d\n", getpid());	/* pid */
	if (write(f, s, 11) != 11)  {
		close(f); unlink(tempfile); return -1;
	}
	if (gethostname(hostname, sizeof(hostname)))	/* hostname */
		return -1;
	if ((flags & PIDLOCK_USEHOSTNAME))  {
		if ((write(f, hostname, strlen(hostname)) != strlen(hostname))
		    || (write(f, "\n", 1) != 1))  {
			close(f); unlink(tempfile); return -1;
		}
	}
	if (info)  {					/* info */
		if (!(flags & PIDLOCK_USEHOSTNAME))  {
			/* write blank line because there's no hostname */
			if (write(f, "\n", 1) != 1)  {
				close(f); unlink(tempfile); return -1;
			}
		}
		if (write(f, info, strlen(info)) != strlen(info) ||
		    (write(f, "\n", 1) != 1))  {
			close(f); unlink(tempfile); return -1;
		}
	}
	close(f);

	/* Hard link the temporary file to the real lock file. */
	/* This is an atomic operation. */
	while (link(tempfile, lockfile) != 0)  {
		if (errno != EEXIST)  {
			unlink(tempfile);
			return -1;
		}
		/* Find out who has this lockfile. */
		if (!(f = open(lockfile, O_RDONLY, 0)))
			goto retry;
		read(f, s, 11);
		pid2 = atoi(s);
		read(f, s, sizeof(s)-2);
		s[sizeof(s)-1] = '\0';
		if ((p=strchr(s, '\n')))
			*p = '\0';
		close(f);

		if (!((flags & PIDLOCK_USEHOSTNAME) && strcmp(s, hostname)))  {
			if ((kill(pid2, 0) != 0) && (errno == ESRCH))  {
				/* process doesn't exist */
				unlink(lockfile);
				continue;
			}
		}
retry:
		if (flags & PIDLOCK_NONBLOCK)  {
			if (locker)
				*locker = pid2;
			unlink(tempfile);
			errno = EWOULDBLOCK;
			return -1;
		} else
			sleep(5);
	}

	/* return this process's PID on lock */
	if (locker)
		*locker = getpid();
	unlink(tempfile);

	return 0;
}

#define LOCKPATH	"/var/spool/lock/LCK.."
#define	DEVPATH		"/dev/"

int
ttylock(tty, flags, locker)
	const char *tty;
	int flags;
	pid_t *locker;
{
	char	lockfile[MAXPATHLEN];
	char	ttyfile[MAXPATHLEN];
	struct stat sb;

	/* make sure the tty exists */
	strcpy(ttyfile, DEVPATH);
	strncat(ttyfile, tty, MAXPATHLEN-strlen(DEVPATH));
	if (stat(ttyfile, &sb))  {
		errno = ENOENT; return -1;
	}
	if (!(sb.st_mode & S_IFCHR))  {
		errno = ENOENT; return -1;
	}

	/* do the lock */
	strcpy(lockfile, LOCKPATH);
	strncat(lockfile, tty, MAXPATHLEN-strlen(LOCKPATH));
	return pidlock(lockfile, 0, 0, 0);
}

int
ttyunlock(tty)
	const char *tty;
{
	char	lockfile[MAXPATHLEN];
	char	ttyfile[MAXPATHLEN];
	struct stat sb;

	/* make sure the tty exists */
	strcpy(ttyfile, DEVPATH);
	strncat(ttyfile, tty, MAXPATHLEN-strlen(DEVPATH));
	if (stat(ttyfile, &sb))  {
		errno = ENOENT; return -1;
	}
	if (!(sb.st_mode & S_IFCHR))  {
		errno = ENOENT; return -1;
	}

	/* undo the lock */
	strcpy(lockfile, LOCKPATH);
	strncat(lockfile, tty, MAXPATHLEN-strlen(LOCKPATH));
	return unlink(lockfile);
}
