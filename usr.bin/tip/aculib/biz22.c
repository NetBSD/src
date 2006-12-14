/*	$NetBSD: biz22.c,v 1.13 2006/12/14 17:09:43 christos Exp $	*/

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
static char sccsid[] = "@(#)biz22.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: biz22.c,v 1.13 2006/12/14 17:09:43 christos Exp $");
#endif /* not lint */

#include "tip.h"

#define DISCONNECT_CMD	"\20\04"	/* disconnection string */

static	int btimeout = 0;
static	jmp_buf timeoutbuf;

static	int	biz_dialer(char *, const char *);
static	int	cmd(const char *);
static	int	detect(const char *);
static	void	sigALRM(int);

/*
 * Dial up on a BIZCOMP Model 1022 with either
 * 	tone dialing (mod = "V")
 *	pulse dialing (mod = "W")
 */
static int
biz_dialer(char *num, const char *mod)
{
	int connected = 0;
	char cbuf[40];

	if (boolean(value(VERBOSE)))
		(void)printf("\nstarting call...");
	/*
	 * Disable auto-answer and configure for tone/pulse
	 *  dialing
	 */
	if (cmd("\02K\r")) {
		(void)printf("can't initialize bizcomp...");
		return (0);
	}
	(void)strncpy(cbuf, "\02.\r", sizeof(cbuf) - 1);
	cbuf[1] = *mod;
	if (cmd(cbuf)) {
		(void)printf("can't set dialing mode...");
		return (0);
	}
	(void)snprintf(cbuf, sizeof cbuf, "\02D%s\r", num);
	(void)write(FD, cbuf, strlen(cbuf));
	if (!detect("7\r")) {
		(void)printf("can't get dial tone...");
		return (0);
	}
	if (boolean(value(VERBOSE)))
		(void)printf("ringing...");
	/*
	 * The reply from the BIZCOMP should be:
	 *	2 \r or 7 \r	failure
	 *	1 \r		success
	 */
	connected = detect("1\r");
	if (btimeout)
		biz22_disconnect();	/* insurance */
	return (connected);
}

int
/*ARGSUSED*/
biz22w_dialer(char *num, char *acu __unused)
{

	return (biz_dialer(num, "W"));
}

int
/*ARGSUSED*/
biz22f_dialer(char *num, char *acu __unused)
{

	return (biz_dialer(num, "V"));
}

void
biz22_disconnect(void)
{

	(void)write(FD, DISCONNECT_CMD, 4);
	(void)sleep(2);
	(void)tcflush(FD, TCIOFLUSH);
}

void
biz22_abort(void)
{

	(void)write(FD, "\02", 1);
}

static void
/*ARGSUSED*/
sigALRM(int dummy __unused)
{

	btimeout = 1;
	longjmp(timeoutbuf, 1);
}

static int
cmd(const char *s)
{
	sig_t f;
	char c;

	(void)write(FD, s, strlen(s));
	f = signal(SIGALRM, sigALRM);
	if (setjmp(timeoutbuf)) {
		biz22_abort();
		(void)signal(SIGALRM, f);
		return (1);
	}
	(void)alarm((unsigned)number(value(DIALTIMEOUT)));
	(void)read(FD, &c, 1);
	(void)alarm(0);
	(void)signal(SIGALRM, f);
	c &= 0177;
	return (c != '\r');
}

static int
detect(const char * volatile s)
{
	sig_t f;
	char c;

	f = signal(SIGALRM, sigALRM);
	btimeout = 0;
	while (*s) {
		if (setjmp(timeoutbuf)) {
			biz22_abort();
			break;
		}
		(void)alarm((unsigned)number(value(DIALTIMEOUT)));
		(void)read(FD, &c, 1);
		(void)alarm(0);
		c &= 0177;
		if (c != *s++)
			return (0);
	}
	(void)signal(SIGALRM, f);
	return (btimeout == 0);
}
