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
__RCSID("$NetBSD: fexec.c,v 1.1 2003/08/24 21:10:47 tron Exp $");
#endif

int
fexec(const char *arg, ...)
{
	va_list		ap;
	int		max, argc, status;
	const char	**argv;
	pid_t		child;

	max = 4;
	argv = malloc(max * sizeof(const char *));
	if (argv == NULL) {
		warnx("fexec can't alloc arg space");
		return -1;
	}
	argv[0] = arg;
	argc = 1;

	va_start(ap, arg);
	do {
		if (argc == max) {
			const char **ptr;

			max <<= 1;
			ptr = realloc(argv, max * sizeof(const char *));
			if (ptr == NULL) {
				warnx("fexec can't alloc arg space");
				free(argv);
				va_end(ap);
				return -1;
			}
			argv = ptr;
		}
		arg = va_arg(ap, const char *);
		argv[argc++] = arg;
	} while (arg != NULL);
	va_end(ap);

#ifdef FEXEC_DEBUG
	(void) printf("fexec(\"%s\"", argv[0]);
	argc = 1;
	while (argv[argc] != NULL)
		(void) printf(", \"%s\"", argv[argc++]);
	(void) printf(")\n");
#endif

	child = vfork();
	if (child == 0) {
		(void) execvp(argv[0], (char ** const)argv);
		_exit(127);
		/* NOTREACHED */
	}

	free(argv);
	if (child < 0)
		return -1;

	while (waitpid(child, &status, 0) < 0) {
		if (errno != EINTR)
			return -1;
	}

	if (!WIFEXITED(status))
		return -1;

	return WEXITSTATUS(status);
}
