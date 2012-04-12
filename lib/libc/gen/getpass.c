/*	$NetBSD: getpass.c,v 1.18 2012/04/12 20:08:01 christos Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: getpass.c,v 1.18 2012/04/12 20:08:01 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#ifdef TEST
#include <stdio.h>
#endif
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <paths.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __weak_alias
__weak_alias(getpass_r,_getpass_r)
__weak_alias(getpass,_getpass)
#endif

/*
 * Notes:
 *	- There is no getpass_r in POSIX
 *	- Historically EOF is documented to be treated as EOL, we provide a
 *	  tunable for that DONT_TREAT_EOF_AS_EOL to disable this.
 *	- Historically getpass ate extra characters silently, we provide
 *	  a tunable for that DONT_DISCARD_SILENTLY to disable this.
 *	- Historically getpass "worked" by echoing characters when turning
 *	  off echo failed, we provide a tunable DONT_WORK_AND_ECHO to
 *	  disable this.
 *	- Some implementations say that on interrupt the program shall
 *	  receive an interrupt signal before the function returns. We
 *	  send all the tty signals before we return, but we don't expect
 *	  suspend to do something useful unless the caller calls us again.
 */
char *
getpass_r(const char *prompt, char *ret, size_t len)
{
	struct termios gt;
	char c;
	int infd, outfd, sig;
	bool lnext, havetty;

	_DIAGASSERT(prompt != NULL);

	sig = 0;
	/*
	 * Try to use /dev/tty if possible; otherwise read from stdin and
	 * write to stderr.
	 */
	if ((outfd = infd = open(_PATH_TTY, O_RDWR)) == -1) {
		infd = STDIN_FILENO;
		outfd = STDERR_FILENO;
		havetty = false;
	} else
		havetty = true;

	if (tcgetattr(infd, &gt) == -1) {
		havetty = false;
#ifdef DONT_WORK_AND_ECHO
		goto out;
#else
		memset(&gt, -1, sizeof(gt));
#endif
	} else
		havetty = true;
		

	if (havetty) {
		struct termios st = gt;

		st.c_lflag &= ~(ECHO|ECHOK|ECHOE|ECHOKE|ECHOCTL|ISIG|ICANON);
		st.c_cc[VMIN] = 1;
		st.c_cc[VTIME] = 0;
		if (tcsetattr(infd, TCSAFLUSH|TCSASOFT, &st) == -1)
			goto out;
	}

	if (prompt != NULL) {
		size_t plen = strlen(prompt);
		(void)write(outfd, prompt, plen);
	}

	c = '\1';
	lnext = false;
	for (size_t l = 0; c != '\0'; ) {
		if (read(infd, &c, 1) != 1)
			goto restore;

#define beep() write(outfd, "\a", 1)
#define C(a, b) (gt.c_cc[(a)] == _POSIX_VDISABLE ? (b) : gt.c_cc[(a)])

		if (lnext) {
			lnext = false;
			goto add;
		}

		/* Ignored */
		if (c == C(VREPRINT, CTRL('r')) || c == C(VSTART, CTRL('q')) || 
		    c == C(VSTOP, CTRL('s')) || c == C(VSTATUS, CTRL('t')) || 
		    c == C(VDISCARD, CTRL('o')))
			continue;

		/* Literal next */
		if (c == C(VLNEXT, CTRL('v'))) {
			lnext = true;
			continue;
		}

		/* Line or word kill, treat as reset */
		if (c == C(VKILL, CTRL('u')) || c == C(VWERASE, CTRL('w'))) {
			l = 0;
			continue;
		}

		/* Character erase */
		if (c == C(VERASE, CTRL('h'))) {
			if (l == 0)
				beep();
			else
				l--;
			continue;
		}

		/* tty signal characters */
		if (c == C(VINTR, CTRL('c'))) {
			sig = SIGINT;
			goto out;
		}
		if (c == C(VQUIT, CTRL('\\'))) {
			sig = SIGQUIT;
			goto out;
		}
		if (c == C(VSUSP, CTRL('z')) || c == C(VDSUSP, CTRL('y'))) {
			sig = SIGTSTP;
			goto out;
		}

		/* EOF */
		if (c == C(VEOF, CTRL('d')))  {
#ifdef DONT_TREAT_EOF_AS_EOL
			errno = ENODATA;
			goto out;
#else
			c = '\0';
			goto add;
#endif
		}

		/* End of line */
		if (c == C(VEOL, CTRL('j')) || c == C(VEOL2, CTRL('l')))
			c = '\0';
add:
		if (l >= len) {
#ifdef DONT_DISCARD_SILENTLY
			beep();
			continue;
#else
			if (c == '\0' && l > 0)
				l--;
			else
				continue;
#endif
		}
		ret[l++] = c;
	}

	if (havetty)
		(void)tcsetattr(infd, TCSAFLUSH|TCSASOFT, &gt);
	return ret;
restore:
	c = errno;
	if (havetty)
		(void)tcsetattr(infd, TCSAFLUSH|TCSASOFT, &gt);
	errno = c;
out:
	if (sig) {
		(void)raise(sig);
		errno = EINTR;
	}
	return NULL;
}

char *
getpass(const char *prompt)
{
	static char e[] = "";
	static char *buf;
	static long bufsiz;
	char *rv;

	if (buf == NULL) {
		if ((bufsiz = sysconf(_SC_PASS_MAX)) == -1)
			return e;
		if ((buf = malloc((size_t)bufsiz)) == NULL)
			return e;
	}

	if ((rv = getpass_r(prompt, buf, (size_t)bufsiz)) == NULL)
		return e;

	return rv;
}

#ifdef TEST
int
main(int argc, char *argv[])
{
	char buf[28];
	printf("[%s]\n", getpass_r("foo>", buf, sizeof(buf)));
	return 0;
}
#endif
