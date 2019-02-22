/*	$NetBSD: tip.c,v 1.61 2019/02/22 22:25:22 uwe Exp $	*/

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
#include <ctype.h>
#include <libgen.h>

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)tip.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: tip.c,v 1.61 2019/02/22 22:25:22 uwe Exp $");
#endif /* not lint */

/*
 * tip - UNIX link to other systems
 *  tip [-v] [-speed] system-name
 * or
 *  cu [options] [phone-number|"dir"]
 */
#include "tip.h"
#include "pathnames.h"

__dead static void	tipusage(void);

int	escape(void);
__dead static void	intprompt(int);
__dead static void	tipin(void);

char	PNbuf[256];			/* This limits the size of a number */

static char path_phones[] = _PATH_PHONES;

int
main(int argc, char *argv[])
{
	char *System = NULL;
	int c, i;
	char *p;
	const char *q;
	char sbuf[12];
	int cmdlineBR;
	int fcarg;

	setprogname(argv[0]);
	gid = getgid();
	egid = getegid();
	uid = getuid();
	euid = geteuid();
	if (strcmp(getprogname(), "cu") == 0) {
		cumode = 1;
		cumain(argc, argv);
		goto cucommon;
	}

	if (argc > 4)
		tipusage();

	if (!isatty(0))
		errx(EXIT_FAILURE, "must be interactive");

	cmdlineBR = 0;
	while((c = getopt(argc, argv, "v0123456789")) != -1) {
		switch(c) {

		case 'v':
			vflag++;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			cmdlineBR = cmdlineBR * 10 + (c - '0');
			BR = cmdlineBR;
			break;

		default:
			warnx("%s, unknown option", argv[1]);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		tipusage();
	else
		System = argv[0];


	if (System == NULL)
		goto notnumber;
	if (isalpha((unsigned char)*System))
		goto notnumber;
	/*
	 * System name is really a phone number...
	 * Copy the number then stomp on the original (in case the number
	 *	is private, we don't want 'ps' or 'w' to find it).
	 */
	if (strlen(System) > sizeof PNbuf - 1) {
		errx(1, "phone number too long (max = %d bytes)",
			(int)sizeof(PNbuf) - 1);
	}
	(void)strlcpy(PNbuf, System, sizeof(PNbuf));
	for (p = System; *p; p++)
		*p = '\0';
	PN = PNbuf;
	(void)snprintf(sbuf, sizeof sbuf, "tip%d", (int)BR);
	System = sbuf;

notnumber:
	(void)signal(SIGINT, cleanup);
	(void)signal(SIGQUIT, cleanup);
	(void)signal(SIGHUP, cleanup);
	(void)signal(SIGTERM, cleanup);

	if ((i = hunt(System)) == 0) {
		errx(3, "all ports busy");
	}
	if (i == -1) {
		errx(3, "link down");
	}
	setbuf(stdout, NULL);

	/*
	 * Kludge, their's no easy way to get the initialization
	 *   in the right order, so force it here
	 */
	if ((PH = getenv("PHONES")) == NULL)
		PH = path_phones;
	vinit();				/* init variables */
	setparity("none");			/* set the parity table */

	/*
	 * Hardwired connections require the
	 *  line speed set before they make any transmissions
	 *  (this is particularly true of things like a DF03-AC)
	 */
	if (HW) {
		if (ttysetup((speed_t)number(value(BAUDRATE))) != 0) {
			errx(3, "bad baud rate %d",
			    (int)number(value(BAUDRATE)));
		}
	}
	if ((q = tip_connect()) != NULL) {
		errx(1, "\07%s\n[EOT]", q);
	}
	if (!HW) {
		if (ttysetup((speed_t)number(value(BAUDRATE))) != 0) {
			errx(3, "bad baud rate %d",
			    (int)number(value(BAUDRATE)));
		}
	}


cucommon:
	/*
	 * From here down the code is shared with
	 * the "cu" version of tip.
	 */

	/*
	 * Direct connections with no carrier require using O_NONBLOCK on
	 * open, but we don't want to keep O_NONBLOCK after open because it
	 * will cause busy waits.
	 */
	if (DC &&
	    ((fcarg = fcntl(FD, F_GETFL, 0)) < 0 ||
	     fcntl(FD, F_SETFL, fcarg & ~O_NONBLOCK) < 0)) {
		err(1, "can't clear O_NONBLOCK");
	}

	(void)tcgetattr(0, &defterm);
	term = defterm;
	term.c_lflag &= ~(ICANON|IEXTEN|ECHO);
	term.c_iflag &= ~(INPCK|ICRNL);
	term.c_oflag &= ~OPOST;
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	defchars = term;
	term.c_cc[VINTR] = term.c_cc[VQUIT] = term.c_cc[VSUSP] =
		term.c_cc[VDSUSP] = term.c_cc[VDISCARD] =
	 	term.c_cc[VLNEXT] = _POSIX_VDISABLE;
	raw();

	(void)pipe(attndes);
	(void)pipe(fildes);
	(void)pipe(repdes);
	(void)signal(SIGALRM, alrmtimeout);

	/*
	 * Everything's set up now:
	 *	connection established (hardwired or dialup)
	 *	line conditioned (baud rate, mode, etc.)
	 *	internal data structures (variables)
	 * so, fork one process for local side and one for remote.
	 */
	(void)printf("%s", cumode ? "Connected\r\n" : "\07connected\r\n");
	switch (pid = fork()) {
	default:
		tipin();
		break;
	case 0:
		tipout();
		break;
	case -1:
		err(1, "can't fork");
	}
	/*NOTREACHED*/
	exit(0);	/* XXX: pacify gcc */
}

void
tipusage(void)
{
	(void)fprintf(stderr, "usage: %s [-v] [-speed] system-name\n",
	    getprogname());
	(void)fprintf(stderr, "       %s [-v] [-speed] phone-number\n",
	    getprogname());
	exit(1);
}

void
/*ARGSUSED*/
cleanup(int dummy __unused)
{

	if (odisc)
		(void)ioctl(0, TIOCSETD, &odisc);
	_exit(0);
}

/*
 * put the controlling keyboard into raw mode
 */
void
raw(void)
{

	(void)tcsetattr(0, TCSADRAIN, &term);
}


/*
 * return keyboard to normal mode
 */
void
unraw(void)
{

	(void)tcsetattr(0, TCSADRAIN, &defterm);
}

static	jmp_buf promptbuf;

/*
 * Print string ``s'', then read a string
 *  in from the terminal.  Handles signals & allows use of
 *  normal erase and kill characters.
 */
int
prompt(const char *s, char *volatile p, size_t l)
{
	int c;
	char *b = p;
	sig_t oint, oquit;

	stoprompt = 0;
	oint = signal(SIGINT, intprompt);
	oquit = signal(SIGQUIT, SIG_IGN);
	unraw();
	(void)printf("%s", s);
	if (setjmp(promptbuf) == 0)
		while ((c = getchar()) != EOF && (*p = c) != '\n' &&
		    b + l > p)
			p++;
	*p = '\0';

	raw();
	(void)signal(SIGINT, oint);
	(void)signal(SIGQUIT, oquit);
	return (stoprompt || p == b);
}

/*
 * Interrupt service routine during prompting
 */
static void
/*ARGSUSED*/
intprompt(int dummy __unused)
{

	(void)signal(SIGINT, SIG_IGN);
	stoprompt = 1;
	(void)printf("\r\n");
	longjmp(promptbuf, 1);
}

/*
 * getchar() wrapper that checks for EOF on the local end.
 */
static char
xgetchar(void)
{
	int c = getchar();
	if (__predict_false(c == EOF)) {
		cleanup(SIGHUP);
		/* NOTREACHED */
	}

	return (char)c & STRIP_PAR;
}


/*
 * ****TIPIN   TIPIN****
 */
static void
tipin(void)
{
	char gch, bol = 1;

	/*
	 * Kinda klugey here...
	 *   check for scripting being turned on from the .tiprc file,
	 *   but be careful about just using setscript(), as we may
	 *   send a SIGEMT before tipout has a chance to set up catching
	 *   it; so wait a second, then setscript()
	 */
	if (boolean(value(SCRIPT))) {
		(void)sleep(1);
		setscript();
	}

	for (;;) {
		gch = xgetchar();
		if ((gch == character(value(ESCAPE))) && bol) {
			if (!(gch = escape()))
				continue;
		} else if (!cumode &&
		    gch && gch == character(value(RAISECHAR))) {
			setboolean(value(RAISE), !boolean(value(RAISE)));
			continue;
		} else if (gch == '\r' || gch == '\n') {
			bol = 1;
			xpwrite(FD, &gch, 1);
			if (boolean(value(HALFDUPLEX)))
				(void)printf("%s\n", gch == '\r' ? "\r" : "");
			continue;
		} else if (!cumode && gch && gch == character(value(FORCE)))
			gch = xgetchar();
		bol = any(gch, value(EOL));
		if (boolean(value(RAISE)) && islower((unsigned char)gch))
			gch = toupper((unsigned char)gch);
		xpwrite(FD, &gch, 1);
		if (boolean(value(HALFDUPLEX)))
			(void)printf("%c", gch);
	}
}

/*
 * Escape handler --
 *  called on recognition of ``escapec'' at the beginning of a line
 */
int
escape(void)
{
	char gch;
	esctable_t *p;
	char c = character(value(ESCAPE));

	gch = xgetchar();
	for (p = etable; p->e_char; p++)
		if (p->e_char == gch) {
			if ((p->e_flags&PRIV) && uid)
				continue;
			(void)printf("%s", ctrl(c));
			(*p->e_func)(gch);
			return (0);
		}
	/* ESCAPE ESCAPE forces ESCAPE */
	if (c != gch)
		xpwrite(FD, &c, 1);
	return (gch);
}

int
any(char c, const char *p)
{

	while (p && *p)
		if (*p++ == c)
			return (1);
	return (0);
}

char *
interp(const char *s)
{
	static char buf[256];
	char *p = buf, c;
	const char *q;

	while ((c = *s++) != 0 && buf + sizeof buf - p > 2) {
		for (q = "\nn\rr\tt\ff\033E\bb"; *q; q++)
			if (*q++ == c) {
				*p++ = '\\'; *p++ = *q;
				goto next;
			}
		if (c < 040) {
			*p++ = '^'; *p++ = c + 'A'-1;
		} else if (c == 0177) {
			*p++ = '^'; *p++ = '?';
		} else
			*p++ = c;
	next:
		;
	}
	*p = '\0';
	return (buf);
}

char *
ctrl(char c)
{
	static char s[3];

	if (c < 040 || c == 0177) {
		s[0] = '^';
		s[1] = c == 0177 ? '?' : c+'A'-1;
		s[2] = '\0';
	} else {
		s[0] = c;
		s[1] = '\0';
	}
	return (s);
}

/*
 * Help command
 */
void
help(char c)
{
	esctable_t *p;

	(void)printf("%c\r\n", c);
	for (p = etable; p->e_char; p++) {
		if ((p->e_flags&PRIV) && uid)
			continue;
		(void)printf("%2s", ctrl(character(value(ESCAPE))));
		(void)printf("%-2s %c   %s\r\n", ctrl(p->e_char),
			p->e_flags&EXP ? '*': ' ', p->e_help);
	}
}

/*
 * Set up the "remote" tty's state
 */
int
ttysetup(speed_t spd)
{
	struct termios	cntrl;

	(void)tcgetattr(FD, &cntrl);
	(void)cfsetospeed(&cntrl, spd);
	(void)cfsetispeed(&cntrl, spd);
	cntrl.c_cflag &= ~(CSIZE|PARENB);
	cntrl.c_cflag |= CS8;
	if (DC)
		cntrl.c_cflag |= CLOCAL;
	if (boolean(value(HARDWAREFLOW)))
		cntrl.c_cflag |= CRTSCTS;
	cntrl.c_iflag &= ~(ISTRIP|ICRNL);
	cntrl.c_oflag &= ~OPOST;
	cntrl.c_lflag &= ~(ICANON|ISIG|IEXTEN|ECHO);
	cntrl.c_cc[VMIN] = 1;
	cntrl.c_cc[VTIME] = 0;
	if (boolean(value(TAND)))
		cntrl.c_iflag |= IXOFF|IXON;
	else
		cntrl.c_iflag &= ~(IXOFF|IXON);
	return tcsetattr(FD, TCSAFLUSH, &cntrl);
}

static char partab[0200];

/*
 * Do a write to the remote machine with the correct parity.
 * We are doing 8 bit wide output, so we just generate a character
 * with the right parity and output it.
 */
void
xpwrite(int fd, char *buf, size_t n)
{
	size_t i;
	char *bp;

	bp = buf;
	if (bits8 == 0)
		for (i = 0; i < n; i++) {
			*bp = partab[(*bp) & 0177];
			bp++;
		}
	if (write(fd, buf, n) < 0) {
		if (errno == EIO)
			tipabort("Lost carrier.");
		/* this is questionable */
		warn("write");
	}
}

/*
 * Build a parity table with appropriate high-order bit.
 */
void
setparity(const char *defparity)
{
	int i, flip, clr, set;
	const char *parity;
	static char *curpar;

	if (value(PARITY) == NULL || ((char *)value(PARITY))[0] == '\0') {
		if (curpar != NULL)
			free(curpar);
		value(PARITY) = curpar = strdup(defparity);
	}
	parity = value(PARITY);
	if (strcmp(parity, "none") == 0) {
		bits8 = 1;
		return;
	}
	bits8 = 0;
	flip = 0;
	clr = 0377;
	set = 0;
	if (strcmp(parity, "odd") == 0)
		flip = 0200;			/* reverse bit 7 */
	else if (strcmp(parity, "zero") == 0)
		clr = 0177;			/* turn off bit 7 */
	else if (strcmp(parity, "one") == 0)
		set = 0200;			/* turn on bit 7 */
	else if (strcmp(parity, "even") != 0) {
		(void)fprintf(stderr, "%s: unknown parity value\r\n", parity);
		(void)fflush(stderr);
	}
	for (i = 0; i < 0200; i++)
		partab[i] = ((evenpartab[i] ^ flip) | set) & clr;
}
