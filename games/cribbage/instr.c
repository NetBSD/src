/*	$NetBSD: instr.c,v 1.8.4.1 1999/12/27 18:28:59 wrstuden Exp $	*/

/*-
 * Copyright (c) 1990, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)instr.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: instr.c,v 1.8.4.1 1999/12/27 18:28:59 wrstuden Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <curses.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "deck.h"
#include "cribbage.h"
#include "pathnames.h"

void
instructions()
{
	int pstat;
	int fd;
	pid_t pid;
	const char *path;

	switch (pid = vfork()) {
	case -1:
		err(1, "vfork");
	case 0:
		/* Follow the same behaviour for pagers as defined in POSIX.2
		 * for mailx and man.  We only use a pager if stdout is
		 * a terminal, and we pass the file on stdin to sh -c pager.
		 */
		if (!isatty(1))
			path = "cat";
		else {
			if (!(path = getenv("PAGER")) || (*path == 0))
				path = _PATH_MORE;
		}
		if ((fd = open(_PATH_INSTR, O_RDONLY)) == -1) {
			warn("open %s", _PATH_INSTR);
			_exit(1);
		}
		if (dup2(fd, 0) == -1) {
			warn("dup2");
			_exit(1);
		}
		execl("/bin/sh", "sh", "-c", path, NULL);
		warn(NULL);
		_exit(1);
	default:
		do {
			pid = waitpid(pid, &pstat, 0);
		} while (pid == -1 && errno == EINTR);
		if (pid == -1 || WEXITSTATUS(pstat))
			exit(1);
	}
}
