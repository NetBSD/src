/*	$NetBSD: lpr.c,v 1.45.2.1 2012/10/30 19:00:37 yamt Exp $	*/

/*
 * Copyright (c) 1983, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#if 0
static char sccsid[] = "@(#)lpr.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: lpr.c,v 1.45.2.1 2012/10/30 19:00:37 yamt Exp $");
#endif
#endif /* not lint */

/*
 *      lpr -- off line print
 *
 * Allows multiple printers and printers on remote machines by
 * using information from a printer data base.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include "lp.h"
#include "lp.local.h"
#include "pathnames.h"

static char	*cfname;	/* daemon control files, linked from tf's */
static char	*class = host;	/* class title on header page */
static char	*dfname;	/* data files */
static char	*fonts[4];	/* troff font names */
static char	 format = 'f';	/* format char for printing files */
static int	 hdr = 1;	/* print header or not (default is yes) */
static int	 iflag;		/* indentation wanted */
static int	 inchar;	/* location to increment char in file names */
static int	 indent;	/* amount to indent */
static const char *jobname;	/* job name on header page */
static int	 mailflg;	/* send mail */
static int	 nact;		/* number of jobs to act on */
static int	 ncopies = 1;	/* # of copies to make */
static const char *person;	/* user name */
static int	 qflag;		/* q job, but don't exec daemon */
static int	 reqid;		/* request id */
static int	 rflag;		/* remove files upon completion */	
static int	 Rflag;		/* print request id - like POSIX lp */
static int	 sflag;		/* symbolic link flag */
static int	 tfd;		/* control file descriptor */
static char	*tfname;	/* tmp copy of cf before linking */
static char	*title;		/* pr'ing title */
static int	 userid;	/* user id */
static char	*width;		/* width for versatec printing */

static struct stat statb;

static void	 card(int, const char *);
static void	 chkprinter(const char *);
static void	 cleanup(int) __dead;
static void	 copy(int, const char *);
static char	*itoa(int);
static const char	*linked(const char *);
static char	*lmktemp(const char *, int, int);
static void	 mktemps(void);
static int	 nfile(char *);
static int	 test(const char *);
static void	 usage(void) __dead;

uid_t	uid, euid;

int
main(int argc, char *argv[])
{
	struct passwd *pw;
	struct group *gptr;
	char *arg;
	const char *cp;
	char buf[MAXPATHLEN];
	int i, f, errs, c;
	struct stat stb;
	int oerrno;

	euid = geteuid();
	uid = getuid();
	seteuid(uid);

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, cleanup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, cleanup);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, cleanup);

	setprogname(*argv);
	gethostname(host, sizeof (host));
	host[sizeof(host) - 1] = '\0';
	openlog("lpd", 0, LOG_LPR);

	errs = 0;
	while ((c = getopt(argc, argv,
	    ":#:1:2:3:4:C:J:P:RT:U:cdfghi:lmnopqrstvw:")) != -1) {
		switch (c) {

		case '#':		/* n copies */
			if (isdigit((unsigned char)*optarg)) {
				i = atoi(optarg);
				if (i > 0)
					ncopies = i;
			}
			break;

		case '4':		/* troff fonts */
		case '3':
		case '2':
		case '1':
			fonts[optopt - '1'] = optarg;
			break;

		case 'C':		/* classification spec */
			hdr++;
			class = optarg;
			break;

		case 'J':		/* job name */
			hdr++;
			jobname = optarg;
			break;

		case 'P':		/* specifiy printer name */
			printer = optarg;
			break;

		case 'R':		/* print request id */
			Rflag++;
			break;
			
		case 'T':		/* pr's title line */
			title = optarg;
			break;

		case 'U':		/* user name */
			hdr++;
			person = optarg;
			break;

		case 'c':		/* print cifplot output */
		case 'd':		/* print tex output (dvi files) */
		case 'g':		/* print graph(1G) output */
		case 'l':		/* literal output */
		case 'o':		/* print postscript output */
		case 'n':		/* print ditroff output */
		case 'p':		/* print using ``pr'' */
		case 't':		/* print troff output (cat files) */
		case 'v':		/* print vplot output */
			format = optopt;
			break;

		case 'f':		/* print fortran output */
			format = 'r';
			break;

		case 'h':		/* toggle want of header page */
			hdr = !hdr;
			break;

		case 'i':		/* indent output */
			iflag++;
			indent = atoi(optarg);
			if (indent < 0)
				indent = 8;
			break;

		case 'm':		/* send mail when done */
			mailflg++;
			break;

		case 'q':		/* just q job */
			qflag++;
			break;

		case 'r':		/* remove file when done */
			rflag++;
			break;

		case 's':		/* try to link files */
			sflag++;
			break;

		case 'w':		/* versatec page width */
			width = optarg;
			break;

		case ':':               /* catch "missing argument" error */
			if (optopt == 'i') {
				iflag++; /* -i without args is valid */
				indent = 8;
			} else
				errs++;
			break;

		default:
			errs++;
		}
	}
	argc -= optind;
	argv += optind;
	if (errs)
		usage();
	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;
	chkprinter(printer);
	if (SC && ncopies > 1)
		errx(EXIT_FAILURE, "multiple copies are not allowed");
	if (MC > 0 && ncopies > MC)
		errx(EXIT_FAILURE, "only %ld copies are allowed", MC);
	/*
	 * Get the identity of the person doing the lpr using the same
	 * algorithm as lprm. 
	 */
	userid = getuid();
	if (userid != DU || person == 0) {
		if ((pw = getpwuid(userid)) == NULL)
			errx(EXIT_FAILURE, "Who are you?");
		person = pw->pw_name;
	}
	/*
	 * Check for restricted group access.
	 */
	if (RG != NULL && userid != DU) {
		if ((gptr = getgrnam(RG)) == NULL)
			errx(EXIT_FAILURE,
			     "Restricted group specified incorrectly");
		if (gptr->gr_gid != getgid()) {
			while (*gptr->gr_mem != NULL) {
				if ((strcmp(person, *gptr->gr_mem)) == 0)
					break;
				gptr->gr_mem++;
			}
			if (*gptr->gr_mem == NULL)
				errx(EXIT_FAILURE,
				     "Not a member of the restricted group");
		}
	}
	/*
	 * Check to make sure queuing is enabled if userid is not root.
	 */
	(void)snprintf(buf, sizeof buf, "%s/%s", SD, LO);
	if (userid && stat(buf, &stb) == 0 && (stb.st_mode & S_IXGRP))
		errx(EXIT_FAILURE, "Printer queue is disabled");
	/*
	 * Initialize the control file.
	 */
	mktemps();
	tfd = nfile(tfname);
	seteuid(euid);
	(void)fchown(tfd, DU, -1);	/* owned by daemon for protection */
	seteuid(uid);
	card('H', host);
	card('P', person);
	if (hdr && !SH) {
		if (jobname == NULL) {
			if (argc == 0)
				jobname = "stdin";
			else
				jobname = (arg = strrchr(argv[0], '/')) ?
				    arg+1 : argv[0];
		}
		card('J', jobname);
		card('C', class);
		card('L', person);
	}
	if (iflag)
		card('I', itoa(indent));
	if (mailflg)
		card('M', person);
	if (format == 't' || format == 'n' || format == 'd')
		for (i = 0; i < 4; i++)
			if (fonts[i] != NULL)
				card('1'+i, fonts[i]);
	if (width != NULL)
		card('W', width);

	/*
	 * Read the files and spool them.
	 */
	if (argc == 0)
		copy(0, " ");
	else while (argc--) {
		if (argv[0][0] == '-' && argv[0][1] == '\0') {
			/* use stdin */
			copy(0, " ");
			argv++;
			continue;
		}
		if ((f = test(arg = *argv++)) < 0)
			continue;	/* file unreasonable */

		if (sflag && (cp = linked(arg)) != NULL) {
			(void)snprintf(buf, sizeof buf,
			    "%llu %llu",
			    (unsigned long long)statb.st_dev,
			    (unsigned long long)statb.st_ino);
			card('S', buf);
			if (format == 'p')
				card('T', title ? title : arg);
			for (i = 0; i < ncopies; i++)
				card(format, &dfname[inchar-2]);
			card('U', &dfname[inchar-2]);
			if (f)
				card('U', cp);
			card('N', arg);
			dfname[inchar]++;
			nact++;
			continue;
		}
		if (sflag)
			warnx("%s: not linked, copying instead", arg);
		seteuid(uid);
		if ((i = open(arg, O_RDONLY)) < 0) {
			oerrno = errno;
			seteuid(uid);
			errno = oerrno;
			warn("cannot open %s", arg);
			continue;
		} else {
			copy(i, arg);
			(void)close(i);
			if (f && unlink(arg) < 0)
				warn("%s: not removed", arg);
		}
		seteuid(uid);
	}

	if (nact) {
		(void)close(tfd);
		tfname[inchar]--;
		/*
		 * Touch the control file to fix position in the queue.
		 */
		seteuid(euid);
		if ((tfd = open(tfname, O_RDWR)) >= 0) {
			char ch;

			if (read(tfd, &ch, 1) == 1 &&
			    lseek(tfd, (off_t)0, 0) == 0 &&
			    write(tfd, &ch, 1) != 1) {
				warn("cannot touch %s", tfname);
				tfname[inchar]++;
				cleanup(0);
			}
			(void)close(tfd);
		}
		if (link(tfname, cfname) < 0) {
			warn("cannot rename %s", cfname);
			tfname[inchar]++;
			cleanup(0);
		}
		unlink(tfname);
		seteuid(uid);
		if (Rflag)
			printf("request id is %d\n", reqid);
		if (qflag)		/* just queue things up */
			exit(0);
		if (!startdaemon(printer))
			printf("jobs queued, but cannot start daemon.\n");
		exit(0);
	}
	cleanup(0);
#ifdef __GNUC__
	return (0);
#endif
	/* NOTREACHED */
}

/*
 * Create the file n and copy from file descriptor f.
 */
static void
copy(int f, const char *n)
{
	int fd, i, nr;
	size_t nc;
	char buf[BUFSIZ];

	if (format == 'p')
		card('T', title ? title : n);
	for (i = 0; i < ncopies; i++)
		card(format, &dfname[inchar-2]);
	card('U', &dfname[inchar-2]);
	card('N', n);
	fd = nfile(dfname);
	nr = nc = 0;
	while ((i = read(f, buf, sizeof buf)) > 0) {
		if (write(fd, buf, i) != i) {
			warn("%s: temp file write error", n);
			break;
		}
		nc += i;
		if (nc >= sizeof buf) {
			nc -= sizeof buf;
			nr++;
			if (MX > 0 && nr > MX) {
				warnx("%s: copy file is too large "
				    "(check :mx:?)\n", n);
				break;
			}
		}
	}
	(void)close(fd);
	if (nc == 0 && nr == 0) 
		printf("%s: %s: empty input file\n", getprogname(),
		    f ? n : "stdin");
	else
		nact++;
}

/*
 * Try and link the file to dfname. Return a pointer to the full
 * path name if successful.
 */
static const char *
linked(const char *file)
{
	char *cp;
	static char buf[BUFSIZ];
	int ret;

	if (*file != '/') {
		/* XXX: 2 and file for "/file" */
		if (getcwd(buf, sizeof(buf) - 2 - strlen(file)) == NULL)
			return(NULL);
		while (file[0] == '.') {
			switch (file[1]) {
			case '/':
				file += 2;
				continue;
			case '.':
				if (file[2] == '/') {
					if ((cp = strrchr(buf, '/')) != NULL)
						*cp = '\0';
					file += 3;
					continue;
				}
			}
			break;
		}
		strlcat(buf, "/", sizeof(buf));
		strlcat(buf, file, sizeof(buf));
		file = buf;
	}
	seteuid(euid);
	ret = symlink(file, dfname);
	seteuid(uid);
	return(ret ? NULL : file);
}

/*
 * Put a line into the control file.
 */
static void
card(int c, const char *p2)
{
	char buf[BUFSIZ];
	char *p1 = buf;
	size_t len = 2;

	if (strlen(p2) > BUFSIZ - 2)
		errx(EXIT_FAILURE,
		     "Internal error:  String longer than %d", BUFSIZ);

	*p1++ = c;
	while ((c = *p2++) != '\0') {
		*p1++ = (c == '\n') ? ' ' : c;
		len++;
	}
	*p1++ = '\n';
	if (write(tfd, buf, len) != (ssize_t)len)
		warn("Control file write error");
}

/*
 * Create a new file in the spool directory.
 */
static int
nfile(char *n)
{
	int f;
	int oldumask = umask(0);		/* should block signals */
	int oerrno;

	seteuid(euid);
	f = open(n, O_WRONLY | O_EXCL | O_CREAT, FILMOD);
	oerrno = errno;
	(void)umask(oldumask);
	if (f < 0) {
		errno = oerrno;
		warn("cannot create %s", n);
		cleanup(0);
	}
	if (fchown(f, userid, -1) < 0) {
		warn("cannot chown %s", n);
		cleanup(0);	/* cleanup does exit */
	}
	seteuid(uid);
	if (++n[inchar] > 'z') {
		if (++n[inchar-2] == 't') {
			printf("too many files - break up the job\n");
			cleanup(0);
		}
		n[inchar] = 'A';
	} else if (n[inchar] == '[')
		n[inchar] = 'a';
	return(f);
}

/*
 * Cleanup after interrupts and errors.
 */
static void
cleanup(int signo)
{
	int i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	i = inchar;
	seteuid(euid);
	if (tfname)
		do
			unlink(tfname);
		while (tfname[i]-- != 'A');
	if (cfname)
		do
			unlink(cfname);
		while (cfname[i]-- != 'A');
	if (dfname)
		do {
			do
				unlink(dfname);
			while (dfname[i]-- != 'A');
			dfname[i] = 'z';
		} while (dfname[i-2]-- != 'd');
	exit(1);
}

/*
 * Test to see if this is a printable file.
 * Return -1 if it is not, 0 if its printable, and 1 if
 * we should remove it after printing.
 */
static int
test(const char *file)
{
	int fd;
	char *cp;

	seteuid(uid);
	if (access(file, 4) < 0) {
		warn("cannot access %s", file);
		goto bad;
	}
	if (stat(file, &statb) < 0) {
		warn("cannot stat %s", file);
		goto bad;
	}
	if (S_ISDIR(statb.st_mode)) {
		warnx("%s is a directory", file);
		goto bad;
	}
	if (statb.st_size == 0) {
		warnx("%s is an empty file", file);
		goto bad;
 	}
	if ((fd = open(file, O_RDONLY)) < 0) {
		warn("cannot open %s", file);
		goto bad;
	}
	(void)close(fd);
	if (rflag) {
		if ((cp = strrchr(file, '/')) == NULL) {
			if (access(".", 2) == 0)
				return(1);
		} else {
			if (cp == file) {
				fd = access("/", 2);
			} else {
				*cp = '\0';
				fd = access(file, 2);
				*cp = '/';
			}
			if (fd == 0)
				return(1);
		}
		warnx("%s: is not removable by you", file);
	}
	return(0);
bad:
	seteuid(uid);
	return(-1);
}

/*
 * itoa - integer to string conversion
 */
static char *
itoa(int i)
{
	static char b[10] = "########";
	char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

/*
 * Perform lookup for printer name or abbreviation --
 */
static void
chkprinter(const char *s)
{
	char *cp;

	getprintcap(s);
	RG = cgetstr(bp, "rg", &cp) == -1 ? NULL : cp;
	if (cgetnum(bp, "mx", &MX) < 0)
		MX = DEFMX;
	if (cgetnum(bp,"mc", &MC) < 0)
		MC = DEFMAXCOPIES;
	if (cgetnum(bp, "du", &DU) < 0)
		DU = DEFUID;
	SC = (cgetcap(bp, "sc", ':') != NULL);
}

/*
 * Make the temp files.
 */
static void
mktemps(void)
{
	int len, fd, n;
	char *cp;
	char buf[MAXPATHLEN];

	(void)snprintf(buf, sizeof(buf), "%s/.seq", SD);
	seteuid(euid);
	if ((fd = open(buf, O_RDWR|O_CREAT, 0664)) < 0)
		err(1, "cannot create %s", buf);
	if (flock(fd, LOCK_EX))
		err(1, "cannot lock %s", buf);
	seteuid(uid);
	n = 0;
	if ((len = read(fd, buf, sizeof(buf))) > 0) {
		for (cp = buf; len--; ) {
			if (*cp < '0' || *cp > '9')
				break;
			n = n * 10 + (*cp++ - '0');
		}
	}
	reqid = n;
	len = strlen(SD) + strlen(host) + 8;
	tfname = lmktemp("tf", n, len);
	cfname = lmktemp("cf", n, len);
	dfname = lmktemp("df", n, len);
	inchar = strlen(SD) + 3;
	n = (n + 1) % 1000;
	(void)lseek(fd, (off_t)0, 0);
	(void)snprintf(buf, sizeof(buf), "%03d\n", n);
	(void)write(fd, buf, strlen(buf));
	(void)close(fd);	/* unlocks as well */
}

/*
 * Make a temp file name.
 */
static char *
lmktemp(const char *id, int num, int len)
{
	char *s;

	if ((s = malloc(len)) == NULL)
		err(EXIT_FAILURE, NULL);
	(void)snprintf(s, len, "%s/%sA%03d%s", SD, id, num, host);
	return(s);
}

static void
usage(void)
{

	fprintf(stderr, 
	    "Usage: %s [-Pprinter] [-#num] [-C class] [-J job] [-T title] "
	    "[-U user]\n"
	    "%s [-i[numcols]] [-1234 font] [-wnum] [-cdfghlmnopqRrstv] "
	    "[name ...]\n", getprogname(), getprogname());
	exit(1);
}
