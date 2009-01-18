/*	$NetBSD: hayes.c,v 1.16 2009/01/18 07:12:39 lukem Exp $	*/

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
static char sccsid[] = "@(#)hayes.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: hayes.c,v 1.16 2009/01/18 07:12:39 lukem Exp $");
#endif /* not lint */

/*
 * Routines for calling up on a Hayes Modem
 * (based on the old VenTel driver).
 * The modem is expected to be strapped for "echo".
 * Also, the switches enabling the DTR and CD lines
 * must be set correctly.
 * NOTICE:
 * The easy way to hang up a modem is always simply to
 * clear the DTR signal. However, if the +++ sequence
 * (which switches the modem back to local mode) is sent
 * before modem is hung up, removal of the DTR signal
 * has no effect (except that it prevents the modem from
 * recognizing commands).
 * (by Helge Skrivervik, Calma Company, Sunnyvale, CA. 1984)
 */
/*
 * TODO:
 * It is probably not a good idea to switch the modem
 * state between 'verbose' and terse (status messages).
 * This should be kicked out and we should use verbose
 * mode only. This would make it consistent with normal
 * interactive use thru the command 'tip dialer'.
 */
#include "tip.h"

#define	min(a,b)	((a < b) ? a : b)

static	int timeout = 0;
static	jmp_buf timeoutbuf;
#define DUMBUFLEN	40
static char dumbuf[DUMBUFLEN];

static	void	error_rep(char);
static	char	gobble(const char *);
static	void	goodbye(void);
static	int	hay_sync(void);
static	void	sigALRM(int);

int
/*ARGSUSED*/
hay_dialer(char *num, char *acu __unused)
{
	char *cp;
	int connected = 0;
	char dummy;
	struct termios cntrl;

	if (hay_sync() == 0)		/* make sure we can talk to the modem */
		return(0);
	if (boolean(value(VERBOSE)))
		(void)printf("\ndialing...");
	(void)fflush(stdout);
	(void)tcgetattr(FD, &cntrl);
	cntrl.c_cflag |= HUPCL;
	(void)tcsetattr(FD, TCSANOW, &cntrl);
	(void)tcflush(FD, TCIOFLUSH);
	(void)write(FD, "ATv0\r", 5);	/* tell modem to use short status codes */
	(void)gobble("\r");
	(void)gobble("\r");
	(void)write(FD, "ATTD", 4);	/* send dial command */
	for (cp = num; *cp; cp++)
		if (*cp == '=')
			*cp = ',';
	(void)write(FD, num, strlen(num));
	(void)write(FD, "\r", 1);
	connected = 0;
	if (gobble("\r")) {
		if ((dummy = gobble("01234")) != '1')
			error_rep(dummy);
		else
			connected = 1;
	}
	if (!connected)
		return (connected);	/* lets get out of here.. */
	(void)tcflush(FD, TCIOFLUSH);
	if (timeout)
		hay_disconnect();	/* insurance */
	return (connected);
}


void
hay_disconnect(void)
{

	/* first hang up the modem*/
#ifdef DEBUG
	(void)printf("\rdisconnecting modem....\n\r");
#endif
	(void)ioctl(FD, TIOCCDTR, 0);
	(void)sleep(1);
	(void)ioctl(FD, TIOCSDTR, 0);
	goodbye();
}

void
hay_abort(void)
{

	(void)write(FD, "\r", 1);	/* send anything to abort the call */
	hay_disconnect();
}

static void
/*ARGSUSED*/
sigALRM(int dummy __unused)
{

	(void)printf("\07timeout waiting for reply\n\r");
	timeout = 1;
	longjmp(timeoutbuf, 1);
}

static char
gobble(const char *match)
{
	char c;
	sig_t f;
	size_t i;
	volatile int status = 0;

	f = signal(SIGALRM, sigALRM);
	timeout = 0;
#ifdef DEBUG
	(void)printf("\ngobble: waiting for %s\n", match);
#endif
	do {
		if (setjmp(timeoutbuf)) {
			(void)signal(SIGALRM, f);
			return (0);
		}
		(void)alarm((unsigned int)number(value(DIALTIMEOUT)));
		(void)read(FD, &c, 1);
		(void)alarm(0);
		c &= 0177;
#ifdef DEBUG
		(void)printf("%c 0x%x ", c, c);
#endif
		for (i = 0; i < strlen(match); i++)
			if (c == match[i])
				status = c;
	} while (status == 0);
	(void)signal(SIGALRM, SIG_DFL);
#ifdef DEBUG
	(void)printf("\n");
#endif
	return (status);
}

static void
error_rep(char c)
{

	(void)printf("\n\r");
	switch (c) {

	case '0':
		(void)printf("OK");
		break;

	case '1':
		(void)printf("CONNECT");
		break;

	case '2':
		(void)printf("RING");
		break;

	case '3':
		(void)printf("NO CARRIER");
		break;

	case '4':
		(void)printf("ERROR in input");
		break;

	case '5':
		(void)printf("CONNECT 1200");
		break;

	default:
		(void)printf("Unknown Modem error: %c (0x%x)", c, c);
	}
	(void)printf("\n\r");
	return;
}

/*
 * set modem back to normal verbose status codes.
 */
void
goodbye(void)
{
	int len;
	char c;

	(void)tcflush(FD, TCIOFLUSH);
	if (hay_sync()) {
		(void)sleep(1);
#ifndef DEBUG
		(void)tcflush(FD, TCIOFLUSH);
#endif
		(void)write(FD, "ATH0\r", 5);		/* insurance */
#ifndef DEBUG
		c = gobble("03");
		if (c != '0' && c != '3') {
			(void)printf("cannot hang up modem\n\r");
			(void)printf("please use 'tip dialer' to make sure the line is hung up\n\r");
		}
#endif
		(void)sleep(1);
		(void)ioctl(FD, FIONREAD, &len);
#ifdef DEBUG
		(void)printf("goodbye1: len=%d -- ", len);
		rlen = read(FD, dumbuf, min(len, DUMBUFLEN));
		dumbuf[rlen] = '\0';
		(void)printf("read (%d): %s\r\n", rlen, dumbuf);
#endif
		(void)write(FD, "ATv1\r", 5);
		(void)sleep(1);
#ifdef DEBUG
		(void)ioctl(FD, FIONREAD, &len);
		(void)printf("goodbye2: len=%d -- ", len);
		rlen = read(FD, dumbuf, min(len, DUMBUFLEN));
		dumbuf[rlen] = '\0';
		(void)printf("read (%d): %s\r\n", rlen, dumbuf);
#endif
	}
	(void)tcflush(FD, TCIOFLUSH);
	(void)ioctl(FD, TIOCCDTR, 0);		/* clear DTR (insurance) */
	(void)close(FD);
}

#define MAXRETRY	5

int
hay_sync(void)
{
	int len, retry = 0;

	while (retry++ <= MAXRETRY) {
		(void)write(FD, "AT\r", 3);
		(void)sleep(1);
		(void)ioctl(FD, FIONREAD, &len);
		if (len) {
			len = read(FD, dumbuf, (size_t)min(len, DUMBUFLEN));
			if (strchr(dumbuf, '0') ||
		   	(strchr(dumbuf, 'O') && strchr(dumbuf, 'K')))
				return(1);
#ifdef DEBUG
			dumbuf[len] = '\0';
			(void)printf("hay_sync: (\"%s\") %d\n\r", dumbuf, retry);
#endif
		}
		(void)ioctl(FD, TIOCCDTR, 0);
		(void)ioctl(FD, TIOCSDTR, 0);
	}
	(void)printf("Cannot synchronize with hayes...\n\r");
	return(0);
}
