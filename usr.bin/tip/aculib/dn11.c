/*	$NetBSD: dn11.c,v 1.10 2006/12/14 17:09:43 christos Exp $	*/

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
static char sccsid[] = "@(#)dn11.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: dn11.c,v 1.10 2006/12/14 17:09:43 christos Exp $");
#endif /* not lint */

/*
 * Routines for dialing up on DN-11
 */
#include "tip.h"

static jmp_buf jmpbuf;
static int child = -1, dn;

static	void	alarmtr(int);

int
dn_dialer(char *num, char *acu)
{
	int lt, nw;
	int timelim;
	struct termios cntrl;

	if (boolean(value(VERBOSE)))
		(void)printf("\nstarting call...");
	if ((dn = open(acu, O_WRONLY)) < 0) {
		if (errno == EBUSY)
			(void)printf("line busy...");
		else
			(void)printf("acu open error...");
		return (0);
	}
	if (setjmp(jmpbuf)) {
		(void)kill(child, SIGKILL);
		(void)close(dn);
		return (0);
	}
	(void)signal(SIGALRM, alarmtr);
	timelim = 5 * strlen(num);
	(void)alarm((unsigned)(timelim < 30 ? 30 : timelim));
	if ((child = fork()) == 0) {
		/*
		 * ignore this stuff for aborts
		 */
		(void)signal(SIGALRM, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)sleep(2);
		nw = write(dn, num, (size_t)(lt = strlen(num)));
		exit(nw != lt);
	}
	/*
	 * open line - will return on carrier
	 */
	if ((FD = open(DV, O_RDWR)) < 0) {
		if (errno == EIO)
			(void)printf("lost carrier...");
		else
			(void)printf("dialup line open failed...");
		(void)alarm(0);
		(void)kill(child, SIGKILL);
		(void)close(dn);
		return (0);
	}
	(void)alarm(0);
	(void)tcgetattr(dn, &cntrl);
	cntrl.c_cflag |= HUPCL;
	(void)tcsetattr(dn, TCSANOW, &cntrl);
	(void)signal(SIGALRM, SIG_DFL);
	while ((nw = wait(&lt)) != child && nw != -1)
		;
	(void)fflush(stdout);
	(void)close(dn);
	if (lt != 0) {
		(void)close(FD);
		return (0);
	}
	return (1);
}

static void
/*ARGSUSED*/
alarmtr(int dummy __unused)
{

	(void)alarm(0);
	longjmp(jmpbuf, 1);
}

/*
 * Insurance, for some reason we don't seem to be
 *  hanging up...
 */
void
dn_disconnect(void)
{

	(void)sleep(2);
	if (FD > 0)
		(void)ioctl(FD, TIOCCDTR, 0);
	(void)close(FD);
}

void
dn_abort(void)
{

	(void)sleep(2);
	if (child > 0)
		(void)kill(child, SIGKILL);
	if (dn > 0)
		(void)close(dn);
	if (FD > 0)
		(void)ioctl(FD, TIOCCDTR, 0);
	(void)close(FD);
}
