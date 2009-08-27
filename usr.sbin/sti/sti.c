/*	$NetBSD: sti.c,v 1.8 2009/08/27 19:40:06 christos Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
#ifdef __RCSID
__RCSID("$NetBSD: sti.c,v 1.8 2009/08/27 19:40:06 christos Exp $");
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#ifdef __RCSID
#include <vis.h>
#else
#define setprogname(a)
extern const char *__progname;
#define getprogname() __progname
#endif

static int
unescape(const char **pp, int *state)
{
	char ch, out;

#ifdef __RCSID
	while ((ch = *(*pp)++) != '\0') {
		switch(unvis(&out, ch, state, 0)) {
		case 0:
		case UNVIS_NOCHAR:
		        break;
		case UNVIS_VALID:
			return out;
		case UNVIS_VALIDPUSH:
		        (*pp)--;
			return out;
		case UNVIS_SYNBAD:
			errno = EILSEQ;
			return -1;
		}
	}
	if (unvis(&out, '\0', state, UNVIS_END) == UNVIS_VALID)
		return out;
#else
	switch ((ch = *(*pp)++)) {
	case '\0':
		goto out;
	case '^':
		ch = *(*pp)++;
		return CTRL(ch);
	case '\\':
		switch (ch = *(*pp)++) {
		case 'a': return '\a';
		case 'b': return '\b';
		case 'e': return '\e';
		case 'f': return '\f';
		case 't': return '\t';
		case 'n': return '\n';
		case 'r': return '\r';
		case 'v': return '\v';
		case '\\': return '\\';

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			out = 0;
			if (ch >= '0' && ch < '8') {
				out = out * 8 + ch - '0';
				ch = *(*pp)++;
				if (ch >= '0' && ch < '8') {
					out = out * 8 + ch - '0';
					ch = *(*pp)++;
					if (ch >= '0' && ch < '8')
						out = out * 8 + ch - '0';
				}
			}
			return out;
		default:
		    break;
		}
		break;
	default:
		return ch;
	}
out:
#endif
	errno = ENODATA;
	return -1;
}

static void
sti(int fd, int c)
{
	char ch = c;

	if (ioctl(fd, TIOCSTI, &ch) == -1)
		err(1, "Cannot simulate terminal input");
}

static void
sendstr(int fd, const char *str)
{
	int c, state = 0;
	const char *ptr = str;

	while ((c = unescape(&ptr, &state)) != -1)
		sti(fd, c);

	if (c == -1 && errno != ENODATA)
		warn("Cannot decode `%s'", str);
}

int
main(int argc, char *argv[])
{
	const char *tty;
	char ttydev[MAXPATHLEN];
	int fd;

	setprogname(*argv);

	if (argc < 2) {
		(void)fprintf(stderr, "Usage: %s <tty> [arg ...]\n",
		    getprogname());
		return 1;
	}

	argc--;
	argv++;

	tty = *argv++;
	argc--;

	if (strncmp(tty, "/dev/", 5) == 0)
		(void)snprintf(ttydev, sizeof(ttydev), "%s", tty);
	else if (strncmp(tty, "tty", 3) == 0 || strncmp(tty, "pty", 3) == 0 ||
	    strncmp(tty, "pts/", 4) == 0)
		(void)snprintf(ttydev, sizeof(ttydev), "/dev/%s", tty);
	else if (isdigit((unsigned char)*tty))
		(void)snprintf(ttydev, sizeof(ttydev), "/dev/pts/%s", tty);
	else
		(void)snprintf(ttydev, sizeof(ttydev), "/dev/tty%s", tty);

	if ((fd = open(ttydev, O_RDWR)) == -1)
		err(1, "Cannot open `%s'", ttydev);

	if (argc == 0) {
		char *line;
#ifndef __RCSID
		line = malloc(10240);
		while (fgets(line, 10240, stdin) != NULL) {
			char *p;
			if ((p = strrchr(line, '\n')) != NULL)
				*p = '\0';
			sendstr(fd, line);
		}
		free(line);
#else
		while ((line = fparseln(stdin, NULL, NULL, NULL, 0)) != NULL) {
			sendstr(fd, line);
			free(line);
		}
#endif
	} else {
		for (; argc--; argv++) {
			sendstr(fd, *argv);
			if (argc != 0)
				sti(fd, ' ');
		}
	}

	(void)close(fd);
	return 0;
}
