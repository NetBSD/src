/*	$NetBSD: os.c,v 1.5 2003/08/06 13:36:54 itojun Exp $	*/

/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
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
static char sccsid[] = "@(#)os.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: os.c,v 1.5 2003/08/06 13:36:54 itojun Exp $");
#endif
#endif /* not lint */

/*
 * Operating system dependent routines.
 *
 * Most of the stuff in here is based on Unix, but an attempt
 * has been made to make things work on other operating systems.
 * This will sometimes result in a loss of functionality, unless
 * someone rewrites code specifically for the new operating system.
 *
 * The makefile provides defines to decide whether various
 * Unix features are present.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "less.h"
#include "extern.h"
#include "pathnames.h"

int reading;

static jmp_buf read_label;

/*
 * Pass the specified command to a shell to be executed.
 * Like plain "system()", but handles resetting terminal modes, etc.
 */
void
lsystem(cmd)
	char *cmd;
{
	int inp;
	char cmdbuf[256];
	char *shell;

	/*
	 * Print the command which is to be executed,
	 * unless the command starts with a "-".
	 */
	if (cmd[0] == '-')
		cmd++;
	else
	{
		lower_left();
		clear_eol();
		putstr("!");
		putstr(cmd);
		putstr("\n");
	}

	/*
	 * De-initialize the terminal and take out of raw mode.
	 */
	deinit();
	flush();
	raw_mode(0);

	/*
	 * Restore signals to their defaults.
	 */
	init_signals(0);

	/*
	 * Force standard input to be the terminal, "/dev/tty",
	 * even if less's standard input is coming from a pipe.
	 */
	inp = dup(0);
	(void)close(0);
	if (open(_PATH_TTY, O_RDONLY, 0) < 0)
		(void)dup(inp);

	/*
	 * Pass the command to the system to be executed.
	 * If we have a SHELL environment variable, use
	 * <$SHELL -c "command"> instead of just <command>.
	 * If the command is empty, just invoke a shell.
	 */
	if ((shell = getenv("SHELL")) != NULL && *shell != '\0')
	{
		if (*cmd == '\0')
			cmd = shell;
		else
		{
			(void)snprintf(cmdbuf, sizeof(cmdbuf), "%s -c \"%s\"",
			    shell, cmd);
			cmd = cmdbuf;
		}
	}
	if (*cmd == '\0')
		cmd = "sh";

	(void)system(cmd);

	/*
	 * Restore standard input, reset signals, raw mode, etc.
	 */
	(void)close(0);
	(void)dup(inp);
	(void)close(inp);

	init_signals(1);
	raw_mode(1);
	init();
	screen_trashed = 1;
#if defined(SIGWINCH) || defined(SIGWIND)
	/*
	 * Since we were ignoring window change signals while we executed
	 * the system command, we must assume the window changed.
	 */
	winch(SIGWINCH);
#endif
}

/*
 * Like read() system call, but is deliberately interruptable.
 * A call to intread() from a signal handler will interrupt
 * any pending iread().
 */
int
iread(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int n;

	if (setjmp(read_label))
		/*
		 * We jumped here from intread.
		 */
		return (READ_INTR);

	flush();
	reading = 1;
	n = read(fd, buf, len);
	reading = 0;
	if (n < 0)
		return (-1);
	return (n);
}

void
intread()
{
	(void)sigsetmask(0L);
	longjmp(read_label, 1);
}

/*
 * Expand a filename, substituting any environment variables, etc.
 * The implementation of this is necessarily very operating system
 * dependent.  This implementation is unabashedly only for Unix systems.
 */
char *
glob(filename)
	char *filename;
{
	FILE *f;
	char *p;
	int ch;
	char *cmd;
	static char buffer[MAXPATHLEN];
	size_t l;

	if (filename[0] == '#')
		return (filename);

	/*
	 * We get the shell to expand the filename for us by passing
	 * an "echo" command to the shell and reading its output.
	 */
	p = getenv("SHELL");
	if (p == NULL || *p == '\0')
	{
		/*
		 * Read the output of <echo filename>.
		 */
		asprintf(&cmd, "echo \"%s\"", filename);
		if (cmd == NULL)
			return (filename);
	} else
	{
		/*
		 * Read the output of <$SHELL -c "echo filename">.
		 */
		asprintf(&cmd, "%s -c \"echo %s\"", p, filename);
		if (cmd == NULL)
			return (filename);
	}

	if ((f = popen(cmd, "r")) == NULL)
		return (filename);
	free(cmd);

	for (p = buffer; p < &buffer[sizeof(buffer)-1];  p++)
	{
		if ((ch = getc(f)) == '\n' || ch == EOF)
			break;
		*p = ch;
	}
	*p = '\0';
	(void)pclose(f);
	return(buffer);
}

char *
bad_file(filename, message, len)
	char *filename, *message;
	u_int len;
{
	struct stat statbuf;

	if (stat(filename, &statbuf) < 0) {
		(void)snprintf(message, len, "%s: %s", filename,
		    strerror(errno));
		return(message);
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
		static char is_dir[] = " is a directory";

		strtcpy(message, filename, (int)(len-sizeof(is_dir)-1));
		(void)strlcat(message, is_dir, len);
		return(message);
	}
	return((char *)NULL);
}

/*
 * Copy a string, truncating to the specified length if necessary.
 * Unlike strncpy(), the resulting string is guaranteed to be null-terminated.
 */
void
strtcpy(to, from, len)
	char *to, *from;
	int len;
{
	(void)strncpy(to, from, (int)len);
	to[len-1] = '\0';
}

