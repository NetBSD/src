/*	$NetBSD: termios.c,v 1.1.6.2 2009/05/13 17:18:55 jym Exp $	*/
/*	$OpenBSD: termios.c,v 1.2 2005/05/24 20:38:20 uwe Exp $	*/

/*-
 * Copyright (c) 1989, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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

#include "boot.h"
#include "unixdev.h"

#include "compat_linux.h"
#include "termios.h"

int
linux_cfsetspeed(struct linux_termios *t, linux_speed_t speed)
{
	int mask;
	int i;

	mask = LINUX_B9600;	/* XXX default value should this be 0? */
	for (i = 0; i < __arraycount(linux_speeds); i++) {
		if (speed == linux_speeds[i]) {
			mask = linux_spmasks[i];
			break;
		}
	}
	if (i == __arraycount(linux_speeds))
		return -1;

	t->c_cflag &= ~LINUX_CBAUD;
	t->c_cflag |= mask;
	return 0;
}

void
linux_cfmakeraw(struct linux_termios *t)
{

	t->c_iflag &= ~(LINUX_IMAXBEL
		       |LINUX_IGNBRK
		       |LINUX_BRKINT
		       |LINUX_PARMRK
		       |LINUX_ISTRIP
		       |LINUX_INLCR
		       |LINUX_IGNCR
		       |LINUX_ICRNL
		       |LINUX_IXON);
	t->c_oflag &= ~LINUX_OPOST;
	t->c_lflag &= ~(LINUX_ECHO
		       |LINUX_ECHONL
		       |LINUX_ICANON
		       |LINUX_ISIG
		       |LINUX_IEXTEN);
	t->c_cflag &= ~(LINUX_CSIZE|LINUX_PARENB);
	t->c_cflag |= LINUX_CS8;
}

int
linux_tcgetattr(int fd, struct linux_termios *t)
{

	return uioctl(fd, LINUX_TCGETS, t);
}

/* This function differs slightly from tcsetattr() in libc. */
int
linux_tcsetattr(int fd, int action, struct linux_termios *t)
{

	switch (action) {
	case LINUX_TCSETS:
	case LINUX_TCSETSW:
	case LINUX_TCSETSF:
		break;

	default:
		errno = EINVAL;
		return -1;
	}
	return uioctl(fd, action, t);
}

void dummycall(void);
void
dummycall(void)
{

	(void)linux_termio_to_bsd_termios;
	(void)bsd_termios_to_linux_termio;
	(void)linux_termios_to_bsd_termios;
	(void)bsd_termios_to_linux_termios;
}
