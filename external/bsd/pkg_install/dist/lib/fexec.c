/*	$NetBSD: fexec.c,v 1.4 2024/06/11 09:26:57 wiz Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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


#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "lib.h"

/*
 * Newer macOS releases are not able to correctly handle vfork() when the
 * underlying file is changed or removed, as is the case when upgrading
 * pkg_install itself.  The manual pages suggest using posix_spawn()
 * instead, which seems to work ok.
 */
#if defined(__APPLE__) && \
	((__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__-0) >= 1050)
#define FEXEC_USE_POSIX_SPAWN	1
#else
#define FEXEC_USE_POSIX_SPAWN	0
#endif

#if FEXEC_USE_POSIX_SPAWN
#include <spawn.h>
extern char **environ;

#ifndef O_CLOEXEC
#define O_CLOEXEC	0
#endif

#ifndef O_DIRECTORY
#define O_DIRECTORY	0
#endif
#endif

__RCSID("$NetBSD: fexec.c,v 1.4 2024/06/11 09:26:57 wiz Exp $");

static int	vfcexec(const char *, int, const char *, va_list);

/*
 * fork, then change current working directory to path and
 * execute the command and arguments in the argv array.
 * wait for the command to finish, then return the exit status.
 *
 * macOS uses posix_spawn() instead due to reasons explained above.
 */
int
pfcexec(const char *path, const char *file, const char **argv)
{
	pid_t			child;
	int			status;

#if FEXEC_USE_POSIX_SPAWN
	int prevcwd;

	if ((prevcwd = open(".", O_RDONLY|O_CLOEXEC|O_DIRECTORY)) < 0) {
		warn("open prevcwd failed");
		return -1;
	}

	if ((path != NULL) && (chdir(path) < 0)) {
		warn("chdir %s failed", path);
		return -1;
	}

	if (posix_spawn(&child, file, NULL, NULL, (char **)argv, environ) < 0) {
		warn("posix_spawn failed");
		return -1;
	}

	if (fchdir(prevcwd) < 0) {
		warn("fchdir prevcwd failed");
		return -1;
	}

	(void)close(prevcwd);
#else
	child = vfork();
	switch (child) {
	case 0:
		if ((path != NULL) && (chdir(path) < 0))
			_exit(127);

		(void)execvp(file, __UNCONST(argv));
		_exit(127);
		/* NOTREACHED */
	case -1:
		return -1;
	}
#endif

	while (waitpid(child, &status, 0) < 0) {
		if (errno != EINTR)
			return -1;
	}

	if (!WIFEXITED(status))
		return -1;

	return WEXITSTATUS(status);
}

static int
vfcexec(const char *path, int skipempty, const char *arg, va_list ap)
{
	const char **argv;
	size_t argv_size, argc;
	int retval;

	argv_size = 16;
	argv = xcalloc(argv_size, sizeof(*argv));

	argv[0] = arg;
	argc = 1;

	do {
		if (argc == argv_size) {
			argv_size *= 2;
			argv = xrealloc(argv, argv_size * sizeof(*argv));
		}
		arg = va_arg(ap, const char *);
		if (skipempty && arg && strlen(arg) == 0)
		    continue;
		argv[argc++] = arg;
	} while (arg != NULL);

	retval = pfcexec(path, argv[0], argv);
	free(argv);
	return retval;
}

int
fexec(const char *arg, ...)
{
	va_list	ap;
	int	result;

	va_start(ap, arg);
	result = vfcexec(NULL, 0, arg, ap);
	va_end(ap);

	return result;
}

int
fexec_skipempty(const char *arg, ...)
{
	va_list	ap;
	int	result;

	va_start(ap, arg);
	result = vfcexec(NULL, 1, arg, ap);
	va_end(ap);

	return result;
}

int
fcexec(const char *path, const char *arg, ...)
{
	va_list	ap;
	int	result;

	va_start(ap, arg);
	result = vfcexec(path, 0, arg, ap);
	va_end(ap);

	return result;
}
