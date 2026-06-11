/*	$NetBSD: pidfile.c,v 1.20 2026/06/11 21:13:47 roy Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 1999, 2016, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, Matthias Scheler, Julio Merino and Roy Marples.
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

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <paths.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef PIDFILE_LOCAL
#include "pidfile.h"
#else
#include <util.h>
#endif

#ifdef __RCSID
__RCSID("$NetBSD: pidfile.c,v 1.20 2026/06/11 21:13:47 roy Exp $");
#endif

static char *pf_path;
static int pf_fd = -1;
static bool pf_removeable = true;

/* Reads a pid from a file descriptor */
static pid_t
pidfile_readfd(int fd)
{
	char buf[16], *eptr;
	int error;
	ssize_t n;
	pid_t pid = -1;

	if (lseek(fd, 0, SEEK_SET) == -1)
		return -1;

	n = read(fd, buf, sizeof(buf) - 1);
	if (n == -1)
		return -1;

	buf[n] = '\0';
	pid = (pid_t)strtoi(buf, &eptr, 10, 1, INT_MAX, &error);
	if (error != 0 && !(error == ENOTSUP && *eptr == '\n')) {
		errno = error;
		return -1;
	}

	return pid;
}

int
pidfile_fd(void)
{

	return pf_fd;
}

const char *
pidfile_path(void)
{

	return pf_path;
}

void
pidfile_unremoveable(void)
{

	pf_removeable = false;
}

/* Releases pidfile resources.
 *
 * Returns 0 on success, otherwise -1. */
int
pidfile_unlock(void)
{
	int error;

	if (pf_fd == -1) {
		error = -1;
		errno = EBADF;
	} else {
		error = close(pf_fd);
		pf_fd = -1;
	}
	free(pf_path);
	pf_path = NULL;
	return error;
}

/* Truncate, close and unlink an existent pidfile,
 * if and only if it was created by this process.
 * The pidfile is truncated because we may have dropped permissions
 * or entered a chroot and thus unable to unlink it.
 *
 * Returns 0 on success, otherwise -1. */
int
pidfile_clean(void)
{
	int error;
	pid_t pid;

	if (pf_fd == -1) {
		errno = EBADF;
		return -1;
	}

	pid = pidfile_readfd(pf_fd);
	if (pid == -1)
		error = errno;
	else if (pid != getpid())
		error = EPERM;
	else if (ftruncate(pf_fd, 0) == -1)
		error = errno;
	else if (pf_removeable && unlink(pf_path) == -1)
		error = errno;
	else
		error = 0;

	(void)pidfile_unlock();

	if (error != 0) {
		errno = error;
		return -1;
	}
	return 0;
}

/* atexit shim for pidfile_clean */
static void
pidfile_cleanup(void)
{

	(void)pidfile_clean();
}

/* Constructs a name for a pidfile in the default location (/var/run).
 * If 'bname' is NULL, uses the name of the current program for the name of
 * the pidfile.
 *
 * Returns 0 on success, otherwise -1. */
static int
pidfile_varrun_path(char **path, const char *bname)
{

	if (bname == NULL)
		bname = getprogname();

	/* _PATH_VARRUN includes trailing / */
	return asprintf(path, "%s%s.pid", _PATH_VARRUN, bname);
}

/* Returns the process ID inside path on success, otherwise -1.
 * If no path is given, use the last pidfile path, otherwise the default one. */
pid_t
pidfile_read(const char *path)
{
	char *dpath = NULL;
	int fd;
	pid_t pid = -1;

	if (path == NULL && pf_path != NULL)
		path = pf_path;
	if (path == NULL || strchr(path, '/') == NULL) {
		if (pidfile_varrun_path(&dpath, path) == -1)
			goto out;
		path = dpath;
	}

	if (pf_fd != -1 && strcmp(path, pf_path) == 0)
		fd = pf_fd;
	else if ((fd = open(path, O_RDONLY | O_NONBLOCK)) == -1)
		goto out;

	pid = pidfile_readfd(fd);
	if (fd != pf_fd)
		(void)close(fd);

out:
	free(dpath);
	return pid;
}

/* Locks the pidfile specified by path and writes the process pid to it.
 * The new pidfile is "registered" in the global variables pf_fd,
 * pf_path and so that any further call to pidfile_lock(3)
 * can check if we are recreating the same file or a new one.
 *
 * Returns 0 on success, otherwise the pid of the process who owns the
 * lock if it can be read, otherwise -1. */
pid_t
pidfile_lock(const char *path)
{
	char *dpath = NULL;
	static bool registered_atexit = false;
	pid_t pid = -1;

	/* Register for cleanup with atexit. */
	if (!registered_atexit) {
		if (atexit(pidfile_cleanup) == -1)
			goto out;
		registered_atexit = true;
	}

	if (path == NULL || strchr(path, '/') == NULL) {
		if (pidfile_varrun_path(&dpath, path) == -1)
			goto out;
		path = dpath;
	}

	/* If path has changed (no good reason), clean up the old pidfile. */
	if (pf_fd != -1 && strcmp(pf_path, path) != 0)
		(void)pidfile_clean();

	if (pf_fd == -1) {
		int fd, opts;

		opts = O_RDWR | O_CREAT | O_NONBLOCK;
#ifdef O_CLOEXEC
		opts |= O_CLOEXEC;
#endif
#ifdef O_EXLOCK
		opts |= O_EXLOCK;
#endif
		if ((fd = open(path, opts, 0644)) == -1)
			goto return_pid;

#ifndef O_CLOEXEC
		if ((opts = fcntl(fd, F_GETFD)) == -1 ||
		    fcntl(fd, F_SETFD, opts | FD_CLOEXEC) == -1) {
			int error = errno;

			(void)close(fd);
			errno = error;
			goto out;
		}
#endif
#ifndef O_EXLOCK
		if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
			int error = errno;

			(void)close(fd);
			if (error != EAGAIN) {
				errno = error;
				goto out;
			}
			fd = -1;
		}
#endif

return_pid:
		if (fd == -1) {
			if (errno == EAGAIN) {
				/* The pidfile is locked, return the process ID
				 * it contains.
				 * If successful, set errno to EEXIST.
				 * Otherwise set EAGAIN to show it's locked,
				 * as that is more important than why a pid
				 * cannot be read from the locked file. */
				if ((pid = pidfile_read(path)) != -1)
					errno = EEXIST;
				else
					errno = EAGAIN;
			} else
				pid = -1;

			goto out;
		}

		pf_fd = fd;
		if (path == dpath) {
			pf_path = dpath;
			dpath = NULL;
		} else {
			pf_path = strdup(path);
			if (pf_path == NULL) {
				int error = errno;

				(void)close(pf_fd);
				pf_fd = -1;
				errno = error;
				goto out;
			}
		}
	}

	/* Truncate the file, as we could be re-writing it.
	 * Then write the process ID. */
	if (ftruncate(pf_fd, 0) == -1 || lseek(pf_fd, 0, SEEK_SET) == -1 ||
	    dprintf(pf_fd, "%ld\n", (long)getpid()) == -1) {
		int error = errno;

		(void)pidfile_clean();
		errno = error;
		goto out;
	}

	/* Hold the fd open to persist the lock. */
	pid = 0;

out:
	free(dpath);
	return pid;
}

/* The old function.
 * Historical behaviour is that pidfile is not re-written
 * if path has not changed.
 *
 * Returns 0 on success, otherwise -1.
 * As such we have no way of knowing the process ID who owns the lock. */
int
pidfile(const char *path)
{
	pid_t pid;

	pid = pidfile_lock(path);
	return pid == 0 ? 0 : -1;
}
