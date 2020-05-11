/*	$NetBSD: posix_spawnp.c,v 1.4 2020/05/11 14:54:34 kre Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann <martin@NetBSD.org>.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: posix_spawnp.c,v 1.4 2020/05/11 14:54:34 kre Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <paths.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int posix_spawnp(pid_t * __restrict pid, const char * __restrict file,
    const posix_spawn_file_actions_t *fa,
    const posix_spawnattr_t * __restrict sa,
    char * const *__restrict cav, char * const *__restrict env)
{
	char fpath[FILENAME_MAX];
	const char *path, *p;
	size_t lp, ln;
	int err;

	_DIAGASSERT(file != NULL);

	/*
	 * If there is a / in the name, fall straight through to posix_spawn().
	 */
	if (strchr(file, '/') != NULL)
		return posix_spawn(pid, file, fa, sa, cav, env);

	/* Get the path we're searching. */
	if ((path = getenv("PATH")) == NULL)
		path = _PATH_DEFPATH;

	/*
	 * Find an executable image with the given name in the PATH
	 */

	ln = strlen(file);
	err = 0;
	do {
		/* Find the end of this path element. */
		for (p = path; *path != 0 && *path != ':'; path++)
			continue;
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (p == path) {
			p = ".";
			lp = 1;
		} else
			lp = (size_t)(path - p);

		/*
		 * Once we gain chdir/fchdir file actions, this will need
		 * serious work, as we must treat "." relative to the
		 * target of the (final) chdir performed.
		 *
		 * Fortunately, that day is yet to come.
		 */

		/*
		 * If the path is too long complain.  This is a possible
		 * security issue; given a way to make the path too long
		 * the user may execute the wrong program.
		 */
		if (lp + ln + 2 > sizeof(fpath)) {
			(void)write(STDERR_FILENO, "posix_spawnp: ", 14);
			(void)write(STDERR_FILENO, p, lp);
			(void)write(STDERR_FILENO, ": path too long\n", 16);
			continue;
		}
		memcpy(fpath, p, lp);
		fpath[lp] = '/';
		memcpy(fpath + lp + 1, file, ln);
		fpath[lp + ln + 1] = '\0';

		/*
		 * It would be nice (much better) to try posix_spawn()
		 * here, using the current fpath as the filename, but
		 * there's no guarantee that it is safe to execute it
		 * twice (the file actions may screw us) so that we
		 * cannot do.   This test is weak, barely even adequate.
		 * but unless we are forced into making posix_spawmp()
		 * become a system call (with PATH as an arg, or an array
		 * of possible paths to try, based upon PATH and file)
		 * we really have no better method.
		 */
		if (access(fpath, X_OK) == 0)
			break;

		if (err == 0)
			err = errno;

		fpath[0] = '\0';


	} while (*path++ == ':');	/* Otherwise, *path was NUL */

	if (fpath[0] == '\0')
		return err;

	/*
	 * Use posix_spawn() with the found binary
	 */
	return posix_spawn(pid, fpath, fa, sa, cav, env);
}
