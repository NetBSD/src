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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib.h"

#ifndef lint
__RCSID("$NetBSD: fexec.c,v 1.6 2003/09/23 09:47:21 wiz Exp $");
#endif

static int	vfcexec(const char *, int, const char *, va_list);

static int
vfcexec(const char *path, int skipempty, const char *arg, va_list ap)
{
	static unsigned int	max = 4;
	static const char	**argv = NULL;
	unsigned int		argc;
	pid_t			child;
	int			status;

	if (argv == NULL) {
		argv = malloc(max * sizeof(const char *));
		if (argv == NULL) {
			warnx("vfcexec can't alloc arg space");
			return -1;
		}
	}

	argv[0] = arg;
	argc = 1;

	do {
		if (argc == max) {
			unsigned int	new;
			const char	**ptr;

			new = max * 2;
			ptr = realloc(argv, new * sizeof(const char *));
			if (ptr == NULL) {
				warnx("vfcexec can't alloc arg space");
				free(argv);
				return -1;
			}
			argv = ptr;
			max = new;
		}
		arg = va_arg(ap, const char *);
		if (skipempty && arg && strlen(arg) == 0)
		    continue;
		argv[argc++] = arg;
	} while (arg != NULL);

	child = vfork();
	switch (child) {
	case 0:
		if ((path != NULL) && (chdir(path) < 0))
			_exit(127);

		(void) execvp(argv[0], (char ** const)argv);
		_exit(127);
		/* NOTREACHED */
	case -1:
		return -1;
	}

	while (waitpid(child, &status, 0) < 0) {
		if (errno != EINTR)
			return -1;
	}

	if (!WIFEXITED(status))
		return -1;

	return WEXITSTATUS(status);
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
