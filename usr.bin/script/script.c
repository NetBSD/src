/*	$NetBSD: script.c,v 1.8 2002/06/21 18:46:31 atatat Exp $	*/

/*
 * Copyright (c) 1980, 1992, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)script.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: script.c,v 1.8 2002/06/21 18:46:31 atatat Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>

struct stamp {
	uint64_t scr_len;	/* amount of data */
	uint64_t scr_sec;	/* time it arrived in seconds... */
	uint32_t scr_usec;	/* ...and microseconds */
	uint32_t scr_direction;	/* 'i', 'o', etc (also indicates endianness) */
};

FILE	*fscript;
int	master, slave;
int	child, subchild;
int	outcc;
int	usesleep, rawout;
char	*fname;

struct	termios tt;

void	done __P((void));
void	dooutput __P((void));
void	doshell __P((void));
void	fail __P((void));
void	finish __P((int));
int	main __P((int, char **));
void	scriptflush __P((int));
void	record __P((FILE *, char *, size_t, int));
void	playback __P((FILE *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int cc;
	struct termios rtt;
	struct winsize win;
	int aflg, pflg, ch;
	char ibuf[BUFSIZ];

	aflg = 0;
	pflg = 0;
	usesleep = 1;
	rawout = 0;
	while ((ch = getopt(argc, argv, "adpr")) != -1)
		switch(ch) {
		case 'a':
			aflg = 1;
			break;
		case 'd':
			usesleep = 0;
			break;
		case 'p':
			pflg = 1;
			break;
		case 'r':
			rawout = 1;
			break;
		case '?':
		default:
			(void)fprintf(stderr, "usage: script [-apr] [file]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		fname = argv[0];
	else
		fname = "typescript";

	if ((fscript = fopen(fname, pflg ? "r" : aflg ? "a" : "w")) == NULL)
		err(1, "fopen %s", fname);

	if (pflg)
		playback(fscript);

	(void)tcgetattr(STDIN_FILENO, &tt);
	(void)ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
	if (openpty(&master, &slave, NULL, &tt, &win) == -1)
		err(1, "openpty");

	(void)printf("Script started, output file is %s\n", fname);
	rtt = tt;
	cfmakeraw(&rtt);
	rtt.c_lflag &= ~ECHO;
	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &rtt);

	(void)signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		warn("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			warn("fork");
			fail();
		}
		if (child)
			dooutput();
		else
			doshell();
	}

	if (!rawout)
		(void)fclose(fscript);
	while ((cc = read(STDIN_FILENO, ibuf, BUFSIZ)) > 0) {
		if (rawout)
			record(fscript, ibuf, cc, 'i');
		(void)write(master, ibuf, cc);
	}
	done();
	/* NOTREACHED */
	return (0);
}

void
finish(signo)
	int signo;
{
	int die, pid, status;

	die = 0;
	while ((pid = wait3(&status, WNOHANG, 0)) > 0)
		if (pid == child)
			die = 1;

	if (die)
		done();
}

void
dooutput()
{
	struct itimerval value;
	int cc;
	time_t tvec;
	char obuf[BUFSIZ];

	(void)close(STDIN_FILENO);
	tvec = time(NULL);
	if (rawout)
		record(fscript, NULL, 0, 's');
	else
		(void)fprintf(fscript, "Script started on %s", ctime(&tvec));

	(void)signal(SIGALRM, scriptflush);
	value.it_interval.tv_sec = SECSPERMIN / 2;
	value.it_interval.tv_usec = 0;
	value.it_value = value.it_interval;
	(void)setitimer(ITIMER_REAL, &value, NULL);
	for (;;) {
		cc = read(master, obuf, sizeof (obuf));
		if (cc <= 0)
			break;
		(void)write(1, obuf, cc);
		if (rawout)
			record(fscript, obuf, cc, 'o');
		else
			(void)fwrite(obuf, 1, cc, fscript);
		outcc += cc;
	}
	done();
}

void
scriptflush(signo)
	int signo;
{
	if (outcc) {
		(void)fflush(fscript);
		outcc = 0;
	}
}

void
doshell()
{
	char *shell;

	shell = getenv("SHELL");
	if (shell == NULL)
		shell = _PATH_BSHELL;

	(void)close(master);
	(void)fclose(fscript);
	login_tty(slave);
	execl(shell, shell, "-i", NULL);
	warn("execl %s", shell);
	fail();
}

void
fail()
{

	(void)kill(0, SIGTERM);
	done();
}

void
done()
{
	time_t tvec;

	if (subchild) {
		tvec = time(NULL);
		if (rawout)
			record(fscript, NULL, 0, 'e');
		else
			(void)fprintf(fscript,"\nScript done on %s",
			    ctime(&tvec));
		(void)fclose(fscript);
		(void)close(master);
	} else {
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);
		(void)printf("Script done, output file is %s\n", fname);
	}
	exit(0);
}

void
record(fscript, buf, cc, direction)
	FILE *fscript;
	char *buf;
	size_t cc;
	int direction;
{
	struct iovec iov[2];
	struct stamp stamp;
	struct timeval tv;

	(void)gettimeofday(&tv, NULL);
	stamp.scr_len = cc;
	stamp.scr_sec = tv.tv_sec;
	stamp.scr_usec = tv.tv_usec;
	stamp.scr_direction = direction;
	iov[0].iov_len = sizeof(stamp);
	iov[0].iov_base = &stamp;
	iov[1].iov_len = cc;
	iov[1].iov_base = buf;
	if (writev(fileno(fscript), &iov[0], 2) == -1)
		err(1, "writev");
}

#define swapstamp(stamp) do { \
	if (stamp.scr_direction > 0xff) { \
		stamp.scr_len = bswap64(stamp.scr_len); \
		stamp.scr_sec = bswap64(stamp.scr_sec); \
		stamp.scr_usec = bswap32(stamp.scr_usec); \
		stamp.scr_direction = bswap32(stamp.scr_direction); \
	} \
} while (0/*CONSTCOND*/)

void
playback(fscript)
	FILE *fscript;
{
	struct timespec tsi, tso;
	struct stamp stamp;
	char buf[BUFSIZ];
	size_t l;
	time_t clock;

	do {
		if (fread(&stamp, sizeof(stamp), 1, fscript) != 1)
			err(1, "reading playback header");

		swapstamp(stamp);
		l = fread(buf, 1, stamp.scr_len, fscript);
		clock = stamp.scr_sec;
		tso.tv_sec = stamp.scr_sec;
		tso.tv_nsec = stamp.scr_usec * 1000;

		switch (stamp.scr_direction) {
		case 's':
			(void)printf("Script started on %s", ctime(&clock));
			tsi = tso;
			break;
		case 'e':
			(void)printf("\nScript done on %s", ctime(&clock));
			break;
		case 'i':
			/* throw input away */
			break;
		case 'o':
			tsi.tv_sec = tso.tv_sec - tsi.tv_sec;
			tsi.tv_nsec = tso.tv_nsec - tsi.tv_nsec;
			if (tsi.tv_nsec < 0) {
				tsi.tv_sec -= 1;
				tsi.tv_nsec += 1000000000;
			}
			if (usesleep)
				(void)nanosleep(&tsi, NULL);
			tsi = tso;
			(void)write(STDOUT_FILENO, buf, l);
			break;
		}
	} while (stamp.scr_direction != 'e');

	(void)fclose(fscript);
	exit(0);
}
