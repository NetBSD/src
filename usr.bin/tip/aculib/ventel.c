/*	$NetBSD: ventel.c,v 1.17 2020/07/22 01:24:40 msaitoh Exp $	*/

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
static char sccsid[] = "@(#)ventel.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: ventel.c,v 1.17 2020/07/22 01:24:40 msaitoh Exp $");
#endif /* not lint */

/*
 * Routines for calling up on a Ventel Modem
 * The Ventel is expected to be strapped for local echo (just like uucp)
 */
#include "tip.h"

#define	MAXRETRY	5

static	int timeout = 0;
static	jmp_buf timeoutbuf;

static	void	echo(const char *);
static	int	gobble(char, char *);
static	void	sigALRM(int);
static	int	vensync(int);

/*
 * some sleep calls have been replaced by usleep(DELAYUS)
 * because some ventel modems require two <cr>s in less than
 * a second in order to 'wake up'
 */
#define	DELAYUS		100000		/* delay in microseconds */

int
/*ARGSUSED*/
ven_dialer(char *num, char *acu __unused)
{
	char *cp;
	int connected = 0;
	char *msg, line[80];
	struct termios	cntrl;

	/*
	 * Get in synch with a couple of carriage returns
	 */
	if (!vensync(FD)) {
		(void)printf("can't synchronize with ventel\n");
		return (0);
	}
	if (boolean(value(VERBOSE)))
		(void)printf("\ndialing...");
	(void)fflush(stdout);
	(void)tcgetattr(FD, &cntrl);
	cntrl.c_cflag |= HUPCL;
	(void)tcsetattr(FD, TCSANOW, &cntrl);
	echo("#k$\r$\n$D$I$A$L$:$ ");
	for (cp = num; *cp; cp++) {
		(void)usleep(DELAYUS);
		(void)write(FD, cp, 1);
	}
	(void)usleep(DELAYUS);
	(void)write(FD, "\r", 1);
	(void)gobble('\n', line);
	if (gobble('\n', line))
		connected = gobble('!', line);
	(void)tcflush(FD, TCIOFLUSH);
	if (timeout)
		ven_disconnect();	/* insurance */
	if (connected || timeout || !boolean(value(VERBOSE)))
		return (connected);
	/* call failed, parse response for user */
	cp = strchr(line, '\r');
	if (cp)
		*cp = '\0';
	for (cp = line; (cp = strchr(cp, ' ')) != NULL; cp++)
		if (cp[1] == ' ')
			break;
	if (cp) {
		while (*cp == ' ')
			cp++;
		msg = cp;
		while (*cp) {
			if (isupper((unsigned char)*cp))
				*cp = tolower((unsigned char)*cp);
			cp++;
		}
		(void)printf("%s...", msg);
	}
	return (connected);
}

void
ven_disconnect(void)
{

	(void)close(FD);
}

void
ven_abort(void)
{

	(void)write(FD, "\03", 1);
	(void)close(FD);
}

static void
echo(const char *s)
{
	char c;

	while ((c = *s++) != 0) switch (c) {

	case '$':
		(void)read(FD, &c, 1);
		s++;
		break;

	case '#':
		c = *s++;
		(void)write(FD, &c, 1);
		break;

	default:
		(void)write(FD, &c, 1);
		(void)read(FD, &c, 1);
	}
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
gobble(char match, char response[])
{
	char * volatile cp;
	sig_t f;
	char c;

	cp = response;
	f = signal(SIGALRM, sigALRM);
	timeout = 0;
	do {
		if (setjmp(timeoutbuf)) {
			(void)signal(SIGALRM, f);
			*cp = '\0';
			return (0);
		}
		(void)alarm((unsigned)number(value(DIALTIMEOUT)));
		(void)read(FD, cp, 1);
		(void)alarm(0);
		c = (*cp++ &= 0177);
#ifdef notdef
		if (boolean(value(VERBOSE)))
			(void)putchar(c);
#endif
	} while (c != '\n' && c != match);
	(void)signal(SIGALRM, SIG_DFL);
	*cp = '\0';
	return (c == match);
}

#define min(a,b)	((a)>(b)?(b):(a))
/*
 * This convoluted piece of code attempts to get
 * the ventel in sync.  If you don't have FIONREAD
 * there are gory ways to simulate this.
 */
static int
vensync(int fd)
{
	int already = 0, nread;
	char buf[60];

	/*
	 * Toggle DTR to force anyone off that might have left
	 * the modem connected, and insure a consistent state
	 * to start from.
	 *
	 * If you don't have the ioctl calls to diddle directly
	 * with DTR, you can always try setting the baud rate to 0.
	 */
	(void)ioctl(FD, TIOCCDTR, 0);
	(void)sleep(1);
	(void)ioctl(FD, TIOCSDTR, 0);
	while (already < MAXRETRY) {
		/*
		 * After resetting the modem, send it two \r's to
		 * autobaud on. Make sure to delay between them
		 * so the modem can frame the incoming characters.
		 */
		(void)write(fd, "\r", 1);
		(void)usleep(DELAYUS);
		(void)write(fd, "\r", 1);
		(void)sleep(2);
		if (ioctl(fd, FIONREAD, &nread) < 0) {
			perror("tip: ioctl");
			continue;
		}
		while (nread > 0) {
			(void)read(fd, buf, min(nread, sizeof buf));
			if ((buf[min(nread, sizeof buf) - 1] & 0177) == '$')
				return (1);
			nread -= min(nread, 60);
		}
		(void)sleep(1);
		already++;
	}
	return (0);
}
