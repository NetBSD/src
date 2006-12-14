/*	$NetBSD: df.c,v 1.10 2006/12/14 17:09:43 christos Exp $	*/

/*
 * Copyright (c) 1983, 1993
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)df.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: df.c,v 1.10 2006/12/14 17:09:43 christos Exp $");
#endif /* not lint */

/*
 * Dial the DF02-AC or DF03-AC
 */

#include "tip.h"

static jmp_buf Sjbuf;

static	int	df_dialer(char *, char *, int);
static	void	timeout(int);

int
df02_dialer(char *num, char *acu)
{

	return (df_dialer(num, acu, 0));
}

int
df03_dialer(char *num, char *acu)
{

	return (df_dialer(num, acu, 1));
}

static int
/*ARGSUSED*/
df_dialer(char *num, char *acu __unused, int df03)
{
	int f = FD;
	struct termios cntrl;
	speed_t volatile spd;
	char c;

	spd = 0;
	c = '\0';
	(void)tcgetattr(f, &cntrl);
	cntrl.c_cflag |= HUPCL;
	(void)tcsetattr(f, TCSANOW, &cntrl);
	if (setjmp(Sjbuf)) {
		(void)printf("connection timed out\r\n");
		df_disconnect();
		return (0);
	}
	if (boolean(value(VERBOSE)))
		(void)printf("\ndialing...");
	(void)fflush(stdout);
#ifdef TIOCMSET
	if (df03) {
		int st = TIOCM_ST;	/* secondary Transmit flag */

		(void)tcgetattr(f, &cntrl);
		spd = cfgetospeed(&cntrl);
		if (spd != B1200) {	/* must dial at 1200 baud */
			(void)cfsetospeed(&cntrl, B1200);
			(void)cfsetispeed(&cntrl, B1200);
			(void)tcsetattr(f, TCSAFLUSH, &cntrl);
			(void)ioctl(f, TIOCMBIC, &st); /* clear ST for 300 baud */
		} else
			(void)ioctl(f, TIOCMBIS, &st); /* set ST for 1200 baud */
	}
#endif
	(void)signal(SIGALRM, timeout);
	(void)alarm(5 * strlen(num) + 10);
	(void)tcflush(f, TCIOFLUSH);
	(void)write(f, "\001", 1);
	(void)sleep(1);
	(void)write(f, "\002", 1);
	(void)write(f, num, strlen(num));
	(void)read(f, &c, 1);
#ifdef TIOCMSET
	if (df03 && spd != B1200) {
		(void)cfsetospeed(&cntrl, spd);
		(void)cfsetispeed(&cntrl, spd);
		(void)tcsetattr(f, TCSAFLUSH, &cntrl);
	}
#endif
	return (c == 'A');
}

void
df_disconnect(void)
{

	(void)write(FD, "\001", 1);
	(void)sleep(1);
	(void)tcflush(FD, TCIOFLUSH);
}


void
df_abort(void)
{

	df_disconnect();
}


static void
/*ARGSUSED*/
timeout(int dummy __unused)
{

	longjmp(Sjbuf, 1);
}
