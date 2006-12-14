/*	$NetBSD: courier.c,v 1.17 2006/12/14 17:09:43 christos Exp $	*/

/*
 * Copyright (c) 1986, 1993
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
static char sccsid[] = "@(#)courier.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: courier.c,v 1.17 2006/12/14 17:09:43 christos Exp $");
#endif /* not lint */

/*
 * Routines for calling up on a Courier modem.
 * Derived from Hayes driver.
 */
#include "tip.h"

#define	MAXRETRY	5

static	int timeout = 0;
static	int connected = 0;
static	jmp_buf timeoutbuf;

static	int	cour_connect(void);
static	void	cour_nap(void);
static	void	cour_napx(int);
static	int	cour_swallow(const char *);
static	int	coursync(void);
#ifdef DEBUG
static	void	cour_verbose_read(void);
#endif
static	void	cour_write(int, const char *, int);
static	void	sigALRM(int);

int
cour_dialer(char *num, char *acu)
{
	char *cp;
	struct termios cntrl;

	if (boolean(value(VERBOSE)))
		(void)printf("Using \"%s\"\n", acu);

	(void)tcgetattr(FD, &cntrl);
	cntrl.c_cflag |= HUPCL;
	(void)tcsetattr(FD, TCSAFLUSH, &cntrl);
	/*
	 * Get in synch.
	 */
	if (!coursync()) {
badsynch:
		(void)printf("can't synchronize with courier\n");
		return (0);
	}
	cour_write(FD, "AT E0\r", 6);	/* turn off echoing */
	(void)sleep(1);
#ifdef DEBUG
	if (boolean(value(VERBOSE)))
		cour_verbose_read();
#endif
	(void)tcflush(FD, TCIOFLUSH);
	cour_write(FD, "AT C1 E0 H0 Q0 X6 V1\r", 21);
	if (!cour_swallow("\r\nOK\r\n"))
		goto badsynch;
	(void)fflush(stdout);
	cour_write(FD, "AT D", 4);
	for (cp = num; *cp; cp++)
		if (*cp == '=')
			*cp = ',';
	cour_write(FD, num, (int)strlen(num));
	cour_write(FD, "\r", 1);
	connected = cour_connect();
	if (timeout)
		cour_disconnect();
	return (connected);
}

void
cour_disconnect(void)
{

	/* first hang up the modem*/
	(void)ioctl(FD, TIOCCDTR, 0);
	(void)sleep(1);
	(void)ioctl(FD, TIOCSDTR, 0);
	(void)coursync();				/* reset */
	(void)close(FD);
}

void
cour_abort(void)
{

	cour_write(FD, "\r", 1);	/* send anything to abort the call */
	cour_disconnect();
}

static void
/*ARGSUSED*/
sigALRM(int dummy __unused)
{

	(void)printf("\07timeout waiting for reply\n");
	timeout = 1;
	longjmp(timeoutbuf, 1);
}

static int
cour_swallow(const char * volatile match)
{
	sig_t f;
	char c;

	f = signal(SIGALRM, sigALRM);
	timeout = 0;
	do {
		if (*match =='\0') {
			(void)signal(SIGALRM, f);
			return (1);
		}
		if (setjmp(timeoutbuf)) {
			(void)signal(SIGALRM, f);
			return (0);
		}
		(void)alarm((unsigned)number(value(DIALTIMEOUT)));
		(void)read(FD, &c, 1);
		(void)alarm(0);
		c &= 0177;
#ifdef DEBUG
		if (boolean(value(VERBOSE)))
			(void)putchar(c);
#endif
	} while (c == *match++);
#ifdef DEBUG
	if (boolean(value(VERBOSE)))
		(void)fflush(stdout);
#endif
	(void)signal(SIGALRM, SIG_DFL);
	return (0);
}

struct baud_msg {
	const char *msg;
	unsigned int baud;
} baud_msg[] = {
	{ "",		B300 },
	{ " 1200",	B1200 },
	{ " 2400",	B2400 },
	{ " 9600",	B9600 },
	{ " 9600/ARQ",	B9600 },
	{ 0,		0 }
};

static int
cour_connect(void)
{
	char c;
	int volatile nc;
	int volatile nl;
	int n;
	char dialer_buf[64];
	struct baud_msg *bm;
	sig_t f;

	if (cour_swallow("\r\n") == 0)
		return (0);
	f = signal(SIGALRM, sigALRM);
again:
	(void)memset(dialer_buf, 0, sizeof(dialer_buf));
	timeout = 0;
	for (nc = 0, nl = sizeof(dialer_buf) - 1 ; nl > 0 ; nc++, nl--) {
		if (setjmp(timeoutbuf))
			break;
		(void)alarm((unsigned)number(value(DIALTIMEOUT)));
		n = read(FD, &c, 1);
		(void)alarm(0);
		if (n <= 0)
			break;
		c &= 0x7f;
		if (c == '\r') {
			if (cour_swallow("\n") == 0)
				break;
			if (!dialer_buf[0])
				goto again;
			if (strcmp(dialer_buf, "RINGING") == 0 &&
			    boolean(value(VERBOSE))) {
#ifdef DEBUG
				(void)printf("%s\r\n", dialer_buf);
#endif
				goto again;
			}
			if (strncmp(dialer_buf, "CONNECT",
				    sizeof("CONNECT")-1) != 0)
				break;
			for (bm = baud_msg ; bm->msg ; bm++)
				if (strcmp(bm->msg,
				    dialer_buf+sizeof("CONNECT")-1) == 0) {
					struct termios	cntrl;

					(void)tcgetattr(FD, &cntrl);
					(void)cfsetospeed(&cntrl, bm->baud);
					(void)cfsetispeed(&cntrl, bm->baud);
					(void)tcsetattr(FD, TCSAFLUSH, &cntrl);
					(void)signal(SIGALRM, f);
#ifdef DEBUG
					if (boolean(value(VERBOSE)))
						(void)printf("%s\r\n", dialer_buf);
#endif
					return (1);
				}
			break;
		}
		dialer_buf[nc] = c;
#ifdef notdef
		if (boolean(value(VERBOSE)))
			(void)putchar(c);
#endif
	}
	(void)printf("%s\r\n", dialer_buf);
	(void)signal(SIGALRM, f);
	return (0);
}

/*
 * This convoluted piece of code attempts to get
 * the courier in sync.
 */
static int
coursync(void)
{
	int already = 0;
	int len;
	char buf[40];

	while (already++ < MAXRETRY) {
		(void)tcflush(FD, TCIOFLUSH);
		cour_write(FD, "\rAT Z\r", 6);	/* reset modem */
		(void)memset(buf, 0, sizeof(buf));
		(void)sleep(1);
		(void)ioctl(FD, FIONREAD, &len);
		if (len) {
			len = read(FD, buf, sizeof(buf));
#ifdef DEBUG
			buf[len] = '\0';
			(void)printf("coursync: (\"%s\")\n\r", buf);
#endif
			if (strchr(buf, '0') ||
			   (strchr(buf, 'O') && strchr(buf, 'K')))
				return(1);
		}
		/*
		 * If not strapped for DTR control,
		 * try to get command mode.
		 */
		(void)sleep(1);
		cour_write(FD, "+++", 3);
		(void)sleep(1);
		/*
		 * Toggle DTR to force anyone off that might have left
		 * the modem connected.
		 */
		(void)ioctl(FD, TIOCCDTR, 0);
		(void)sleep(1);
		(void)ioctl(FD, TIOCSDTR, 0);
	}
	cour_write(FD, "\rAT Z\r", 6);
	return (0);
}

static void
cour_write(int fd, const char *cp, int n)
{

#ifdef notdef
	if (boolean(value(VERBOSE)))
		(void)write(1, cp, n);
#endif
	(void)tcdrain(fd);
	cour_nap();
	for ( ; n-- ; cp++) {
		(void)write(fd, cp, 1);
		(void)tcdrain(fd);
		cour_nap();
	}
}

#ifdef DEBUG
static void
cour_verbose_read(void)
{
	int n = 0;
	char buf[BUFSIZ];

	if (ioctl(FD, FIONREAD, &n) < 0)
		return;
	if (n <= 0)
		return;
	if (read(FD, buf, n) != n)
		return;
	(void)write(1, buf, n);
}
#endif

#define setsa(sa, a) \
	sa.sa_handler = a; (void)sigemptyset(&sa.sa_mask); sa.sa_flags = 0

static int napms = 50; /* Give the courier 50 milliseconds between characters */

static int ringring;

void
cour_nap(void)
{

	struct itimerval itv, oitv;
	struct itimerval *itp = &itv;
	struct sigaction sa, osa;
	sigset_t sm, osm;

	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return;

	(void)sigemptyset(&sm);
	(void)sigaddset(&sm, SIGALRM);
	(void)sigprocmask(SIG_BLOCK, &sm, &osm);

	itp->it_value.tv_sec = napms/1000;
	itp->it_value.tv_usec = ((napms%1000)*1000);

	setsa(sa, cour_napx);
	(void)sigaction(SIGALRM, &sa, &osa);

	(void)setitimer(ITIMER_REAL, itp, NULL);

	sm = osm;
	(void)sigdelset(&sm, SIGALRM);

	for (ringring = 0; !ringring; )
		(void)sigsuspend(&sm);

	(void)sigaction(SIGALRM, &osa, NULL);
	(void)setitimer(ITIMER_REAL, &oitv, NULL);
	(void)sigprocmask(SIG_SETMASK, &osm, NULL);
}

static void
/*ARGSUSED*/
cour_napx(int dummy __unused)
{

	ringring = 1;
}
