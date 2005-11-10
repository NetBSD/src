/*	$NetBSD: sti.c,v 1.1 2005/11/10 16:54:05 christos Exp $	*/

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
__RCSID("$NetBSD: sti.c,v 1.1 2005/11/10 16:54:05 christos Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#define isodigit(c) (isdigit(c) && ((c) < '8'))
#define nextc(a) (unsigned char)*((*a)++) 

#ifndef CTRL
#define CTRL(a) ((a)&037)
#endif

static int
unescape(char **pp)
{
	int c;

	switch (c = nextc(pp)) {
	case '\\':
		switch (c = nextc(pp)) {
		case 'a':
			return '\007';         /* Bell */
		case 'b':
			return '\010';         /* Backspace */
		case 't':
			return '\011';         /* Horizontal Tab */
		case 'n':
			return '\012';         /* New Line */
		case 'v':
			return '\013';         /* Vertical Tab */
		case 'f':
			return '\014';         /* Form Feed */
		case 'r':
			return '\015';         /* Carriage Return */
		case 'e':
			return '\033';         /* Escape */
		default:
			if (isodigit(c)) {
				int x = c;
				for (;;) {
					c = nextc(pp);
					if (!c || !isodigit(c)) {
						--(*pp);
						return x;
					}
					x <<= 3;
					x |= c - '0';
				}
			}
			return c;
		}
	case '^':
		return CTRL(nextc(pp));
	default:
		return c;
	}
}

static void
sti(int fd, int c)
{
	char ch = c;

	if (ioctl(fd, TIOCSTI, &ch) == -1)
		err(1, "Cannot simulate terminal input");
}

int
main(int argc, char *argv[])
{
	char *tty, *ptr;
	char ttydev[MAXPATHLEN];
	int fd, c;

	setprogname(*argv);

	if (argc < 2) {
		(void)fprintf(stderr, "Usage: %s tty arg ...", getprogname());
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

	while (argc--) {
		for (ptr = *argv++; (c = unescape(&ptr)) != 0;)
                        sti(fd, c);
		if (argc != 0)
                        sti(fd, ' ');
	}

	(void)close(fd);
	return 0;
}
