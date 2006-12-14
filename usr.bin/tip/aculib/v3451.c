/*	$NetBSD: v3451.c,v 1.14 2006/12/14 17:09:43 christos Exp $	*/

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
static char sccsid[] = "@(#)v3451.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: v3451.c,v 1.14 2006/12/14 17:09:43 christos Exp $");
#endif /* not lint */

/*
 * Routines for calling up on a Vadic 3451 Modem
 */
#include "tip.h"

static	jmp_buf Sjbuf;

static	void	alarmtr(int);
static	int	expect(const char *);
static	int	notin(const char *, char *);
static	int	prefix(const char *, char *);
static	void	vawrite(const char *, int);

int
/*ARGSUSED*/
v3451_dialer(char *num, char *acu __unused)
{
	sig_t func;
	int ok;
	int slow = number(value(BAUDRATE)) < 1200;
	char phone[50];
	struct termios cntrl;

	/*
	 * Get in synch
	 */
	vawrite("I\r", 1 + slow);
	vawrite("I\r", 1 + slow);
	vawrite("I\r", 1 + slow);
	vawrite("\005\r", 2 + slow);
	if (!expect("READY")) {
		(void)printf("can't synchronize with vadic 3451\n");
		return (0);
	}
	(void)tcgetattr(FD, &cntrl);
	term.c_cflag |= HUPCL;
	(void)tcsetattr(FD, TCSANOW, &cntrl);
	(void)sleep(1);
	vawrite("D\r", 2 + slow);
	if (!expect("NUMBER?")) {
		(void)printf("Vadic will not accept dial command\n");
		return (0);
	}
	(void)snprintf(phone, sizeof phone, "%s\r", num);
	vawrite(phone, 1 + slow);
	if (!expect(phone)) {
		(void)printf("Vadic will not accept phone number\n");
		return (0);
	}
	func = signal(SIGINT,SIG_IGN);
	/*
	 * You cannot interrupt the Vadic when its dialing;
	 * even dropping DTR does not work (definitely a
	 * brain damaged design).
	 */
	vawrite("\r", 1 + slow);
	vawrite("\r", 1 + slow);
	if (!expect("DIALING:")) {
		(void)printf("Vadic failed to dial\n");
		return (0);
	}
	if (boolean(value(VERBOSE)))
		(void)printf("\ndialing...");
	ok = expect("ON LINE");
	(void)signal(SIGINT, func);
	if (!ok) {
		(void)printf("call failed\n");
		return (0);
	}
	(void)tcflush(FD, TCIOFLUSH);
	return (1);
}

void
v3451_disconnect(void)
{

	(void)close(FD);
}

void
v3451_abort(void)
{

	(void)close(FD);
}

static void
vawrite(const char *cp, int delay)
{

	for (/*EMPTY*/; *cp; cp++) {
		(void)write(FD, cp, 1);
		(void)sleep((unsigned)delay);
	}
}

static int
expect(const char *cp)
{
	char buf[300];
	char * volatile rp;
	int volatile timeout;
	int volatile online;

	rp = buf;
	timeout = 30;
	online = 0;

	if (strcmp(cp, "\"\"") == 0)
		return (1);
	*rp = 0;
	/*
	 * If we are waiting for the Vadic to complete
	 * dialing and get a connection, allow more time
	 * Unfortunately, the Vadic times out 24 seconds after
	 * the last digit is dialed
	 */
	online = strcmp(cp, "ON LINE") == 0;
	if (online)
		timeout = number(value(DIALTIMEOUT));
	(void)signal(SIGALRM, alarmtr);
	if (setjmp(Sjbuf))
		return (0);
	(void)alarm((unsigned)timeout);
	while (notin(cp, buf) && rp < buf + sizeof (buf) - 1) {
		if (online && notin("FAILED CALL", buf) == 0)
			return (0);
		if (read(FD, rp, 1) < 0) {
			(void)alarm(0);
			return (0);
		}
		if (*rp &= 0177)
			rp++;
		*rp = '\0';
	}
	(void)alarm(0);
	return (1);
}

static void
/*ARGSUSED*/
alarmtr(int dummy __unused)
{

	longjmp(Sjbuf, 1);
}

static int
notin(const char *sh, char *lg)
{

	for (; *lg; lg++)
		if (prefix(sh, lg))
			return (0);
	return (1);
}

static int
prefix(const char *s1, char *s2)
{
	char c;

	while ((c = *s1++) == *s2++)
		if (c == '\0')
			return (1);
	return (c == '\0');
}
