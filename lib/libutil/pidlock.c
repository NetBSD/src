/*	$NetBSD: pidlock.c,v 1.9 2000/07/05 11:46:42 ad Exp $ */

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
__RCSID("$NetBSD: pidlock.c,v 1.9 2000/07/05 11:46:42 ad Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
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
pidlock(const char *lockfile, int flags, pid_t *locker, const char *info)
{
	char	tempfile[MAXPATHLEN];
	char	hostname[MAXHOSTNAMELEN + 1];
	pid_t	pid2 = -1;
	struct	stat st;
	int	err;
	int	f;
	char	s[256];
	char	*p;

	_DIAGASSERT(lockfile != NULL);
	/* locker may be NULL */
	/* info may be NULL */


	if (gethostname(hostname, sizeof(hostname)))
		return -1;
	hostname[sizeof(hostname) - 1] = '\0';

	/*
	 * Build a path to the temporary file.
	 * We use the path with the PID and hostname appended.
	 * XXX This is not thread safe.
	 */
	if (snprintf(tempfile, sizeof(tempfile), "%s.%d.%s", lockfile,
	    (int) getpid(), hostname) >= sizeof(tempfile))  {
		errno = ENAMETOOLONG;
		return -1;
	}

	/* Open it, write pid, hostname, info. */
	if ( (f = open(tempfile, O_WRONLY|O_CREAT|O_TRUNC, 0600)) == -1 )  {
		err = errno;
		unlink(tempfile);
		errno = err;
		return -1;
	}
	snprintf(s, sizeof(s), "%10d\n", getpid());	/* pid */
	if (write(f, s, 11) != 11)  {
		err = errno;
		close(f);
		unlink(tempfile);
		errno = err;
		return -1;
	}
	if ((flags & PIDLOCK_USEHOSTNAME))  {		/* hostname */
		if ((write(f, hostname, strlen(hostname)) != strlen(hostname))
		    || (write(f, "\n", 1) != 1))  {
			err = errno;
			close(f);
			unlink(tempfile);
			errno = err;
			return -1;
		}
	}
	if (info)  {					/* info */
		if (!(flags & PIDLOCK_USEHOSTNAME))  {
			/* write blank line because there's no hostname */
			if (write(f, "\n", 1) != 1)  {
				err = errno;
				close(f);
				unlink(tempfile);
				errno = err;
				return -1;
			}
		}
		if (write(f, info, strlen(info)) != strlen(info) ||
		    (write(f, "\n", 1) != 1))  {
			err = errno;
			close(f);
			unlink(tempfile);
			errno = err;
			return -1;
		}
	}
	close(f);

	/* Hard link the temporary file to the real lock file. */
	/* This is an atomic operation. */
lockfailed:
	while (link(tempfile, lockfile) != 0)  {
		if (errno != EEXIST)  {
			err = errno;
			unlink(tempfile);
			errno = err;
			return -1;
		}
		/* Find out who has this lockfile. */
		if ((f = open(lockfile, O_RDONLY, 0)) != 0)  {
			read(f, s, 11);
			pid2 = atoi(s);
			read(f, s, sizeof(s)-2);
			s[sizeof(s)-1] = '\0';
			if ((p=strchr(s, '\n')) != NULL)
				*p = '\0';
			close(f);

			if (!((flags & PIDLOCK_USEHOSTNAME) &&
			    strcmp(s, hostname)))  {
				if ((kill(pid2, 0) != 0) && (errno == ESRCH))  {
					/* process doesn't exist */
					unlink(lockfile);
					continue;
				}
			}
		}
		if (flags & PIDLOCK_NONBLOCK)  {
			if (locker)
				*locker = pid2;
			unlink(tempfile);
			errno = EWOULDBLOCK;
			return -1;
		} else
			sleep(5);
	}
	/*
	 * Check to see that we really were successful (in case we're
	 * using NFS) by making sure that something really is linked
	 * to our tempfile (reference count is two).
	 */
	if (stat(tempfile, &st) != 0)  {
		err = errno;
		/*
		 * We don't know if lockfile was really created by us,
		 * so we can't remove it.
		 */
		unlink(tempfile);
		errno = err;
		return -1;
	}
	if (st.st_nlink != 2)
		goto lockfailed;

	unlink(tempfile);
 	if (locker)
 		*locker = getpid();	/* return this process's PID on lock */
	errno = 0;
	return 0;
}

#define LOCKPATH	"/var/spool/lock/LCK.."
#define	DEVPATH		"/dev/"

/*ARGSUSED*/
int
ttylock(const char *tty, int flags, pid_t *locker)
{
	char	lockfile[MAXPATHLEN];
	char	ttyfile[MAXPATHLEN];
	struct stat sb;

	_DIAGASSERT(tty != NULL);
	/* locker is not used */

	/* make sure the tty exists */
	strcpy(ttyfile, DEVPATH);
	strncat(ttyfile, tty, MAXPATHLEN-strlen(DEVPATH));
	if (stat(ttyfile, &sb))  {
		errno = ENOENT; return -1;
	}
	if (!S_ISCHR(sb.st_mode))  {
		errno = ENOENT; return -1;
	}

	/* do the lock */
	strcpy(lockfile, LOCKPATH);
	strncat(lockfile, tty, MAXPATHLEN-strlen(LOCKPATH));
	return pidlock(lockfile, 0, 0, 0);
}

int
ttyunlock(const char *tty)
{
	char	lockfile[MAXPATHLEN];
	char	ttyfile[MAXPATHLEN];
	struct stat sb;

	_DIAGASSERT(tty != NULL);

	/* make sure the tty exists */
	strcpy(ttyfile, DEVPATH);
	strncat(ttyfile, tty, MAXPATHLEN-strlen(DEVPATH));
	if (stat(ttyfile, &sb))  {
		errno = ENOENT; return -1;
	}
	if (!S_ISCHR(sb.st_mode))  {
		errno = ENOENT; return -1;
	}

	/* undo the lock */
	strcpy(lockfile, LOCKPATH);
	strncat(lockfile, tty, MAXPATHLEN-strlen(LOCKPATH));
	return unlink(lockfile);
}
