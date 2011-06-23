/*	$NetBSD: script.c,v 1.19.4.1 2011/06/23 14:20:45 cherry Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1980, 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)script.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: script.c,v 1.19.4.1 2011/06/23 14:20:45 cherry Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/param.h>
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

#define	DEF_BUF	65536

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
int	quiet, flush;
const char *fname;

struct	termios tt;

void	done(void);
void	dooutput(void);
void	doshell(const char *);
void	fail(void);
void	finish(int);
int	main(int, char **);
void	scriptflush(int);
void	record(FILE *, char *, size_t, int);
void	consume(FILE *, off_t, char *, int);
void	playback(FILE *);

int
main(int argc, char *argv[])
{
	int cc;
	struct termios rtt;
	struct winsize win;
	int aflg, pflg, ch;
	char ibuf[BUFSIZ];
	const char *command;

	aflg = 0;
	pflg = 0;
	usesleep = 1;
	rawout = 0;
	quiet = 0;
	flush = 0;
	command = NULL;
	while ((ch = getopt(argc, argv, "ac:dfpqr")) != -1)
		switch(ch) {
		case 'a':
			aflg = 1;
			break;
		case 'c':
			command = optarg;
			break;
		case 'd':
			usesleep = 0;
			break;
		case 'f':
			flush = 1;
			break;
		case 'p':
			pflg = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'r':
			rawout = 1;
			break;
		case '?':
		default:
			(void)fprintf(stderr,
			    "Usage: %s [-c <command>][-adfpqr] [file]\n",
			    getprogname());
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

	if (!quiet)
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
			doshell(command);
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
finish(int signo)
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
dooutput(void)
{
	struct itimerval value;
	int cc;
	time_t tvec;
	char obuf[BUFSIZ];

	(void)close(STDIN_FILENO);
	tvec = time(NULL);
	if (rawout)
		record(fscript, NULL, 0, 's');
	else if (!quiet)
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
		if (flush)
			(void)fflush(fscript);
	}
	done();
}

void
scriptflush(int signo)
{
	if (outcc) {
		(void)fflush(fscript);
		outcc = 0;
	}
}

void
doshell(const char *command)
{
	const char *shell;

	(void)close(master);
	(void)fclose(fscript);
	login_tty(slave);
	if (command == NULL) {
		shell = getenv("SHELL");
		if (shell == NULL)
			shell = _PATH_BSHELL;
		execl(shell, shell, "-i", NULL);
		warn("execl `%s'", shell);
	} else {
		if (system(command) == -1)
			warn("system `%s'", command);
	}

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
		else if (!quiet)
			(void)fprintf(fscript,"\nScript done on %s",
			    ctime(&tvec));
		(void)fclose(fscript);
		(void)close(master);
	} else {
		(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);
		if (!quiet)
			(void)printf("Script done, output file is %s\n", fname);
	}
	exit(0);
}

void
record(FILE *fp, char *buf, size_t cc, int direction)
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
	if (writev(fileno(fp), &iov[0], 2) == -1)
		err(1, "writev");
}

void
consume(FILE *fp, off_t len, char *buf, int reg)
{
	size_t l;

	if (reg) {
		if (fseeko(fp, len, SEEK_CUR) == -1)
			err(1, NULL);
	}
	else {
		while (len > 0) {
			l = MIN(DEF_BUF, len);
			if (fread(buf, sizeof(char), l, fp) != l)
				err(1, "cannot read buffer");
			len -= l;
		}
	}
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
playback(FILE *fp)
{
	struct timespec tsi, tso;
	struct stamp stamp;
	struct stat pst;
	char buf[DEF_BUF];
	off_t nread, save_len;
	size_t l;
	time_t tclock;
	int reg;

	if (fstat(fileno(fp), &pst) == -1)
		err(1, "fstat failed");	

	reg = S_ISREG(pst.st_mode);

	for (nread = 0; !reg || nread < pst.st_size; nread += save_len) {
		if (fread(&stamp, sizeof(stamp), 1, fp) != 1) {
			if (reg)
				err(1, "reading playback header");
			else
				break;
		}
		swapstamp(stamp);
		save_len = sizeof(stamp);

		if (reg && stamp.scr_len >
		    (uint64_t)(pst.st_size - save_len) - nread)
			errx(1, "invalid stamp");

		save_len += stamp.scr_len;
		tclock = stamp.scr_sec;
		tso.tv_sec = stamp.scr_sec;
		tso.tv_nsec = stamp.scr_usec * 1000;

		switch (stamp.scr_direction) {
		case 's':
			if (!quiet)
			    (void)printf("Script started on %s",
				ctime(&tclock));
			tsi = tso;
			(void)consume(fp, stamp.scr_len, buf, reg);
			break;
		case 'e':
			if (!quiet)
				(void)printf("\nScript done on %s",
				    ctime(&tclock));
			(void)consume(fp, stamp.scr_len, buf, reg);
			break;
		case 'i':
			/* throw input away */
			(void)consume(fp, stamp.scr_len, buf, reg);
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
			while (stamp.scr_len > 0) {
				l = MIN(DEF_BUF, stamp.scr_len);
				if (fread(buf, sizeof(char), l, fp) != l)
					err(1, "cannot read buffer");

				(void)write(STDOUT_FILENO, buf, l);
				stamp.scr_len -= l;
			}
			break;
		default:
			errx(1, "invalid direction");
		}
	}
	(void)fclose(fp);
	exit(0);
}
