/*	$NetBSD: last.c,v 1.20 2003/07/14 12:02:00 itojun Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994
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
__COPYRIGHT(
"@(#) Copyright (c) 1987, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)last.c	8.2 (Berkeley) 4/2/94";
#endif
__RCSID("$NetBSD: last.c,v 1.20 2003/07/14 12:02:00 itojun Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif
#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif

#ifndef UT_NAMESIZE
#define UT_NAMESIZE 8
#define UT_LINESIZE 8
#define UT_HOSTSIZE 16
#endif
#ifndef SIGNATURE
#define SIGNATURE -1
#endif



#define	NO	0			/* false/no */
#define	YES	1			/* true/yes */

#define	TBUFLEN	30			/* length of time string buffer */
#define	TFMT	"%a %b %d %R"		/* strftime format string */
#define	LTFMT	"%a %b %d %Y %T"	/* strftime long format string */
#define	TFMTS	"%R"			/* strftime format string - time only */
#define	LTFMTS	"%T"			/* strftime long format string - " */

/* fmttime() flags */
#define	FULLTIME	0x1		/* show year, seconds */
#define	TIMEONLY	0x2		/* show time only, not date */
#define	GMT		0x4		/* show time at GMT, for offsets only */

#define MAXUTMP		1024;

typedef struct arg {
	char	*name;			/* argument */
#define	HOST_TYPE	-2
#define	TTY_TYPE	-3
#define	USER_TYPE	-4
	int	type;			/* type of arg */
	struct arg	*next;		/* linked list pointer */
} ARG;
static ARG	*arglist;		/* head of linked list */

typedef struct ttytab {
	time_t	logout;			/* log out time */
	char	tty[128];		/* terminal name */
	struct ttytab	*next;		/* linked list pointer */
} TTY;
static TTY	*ttylist;		/* head of linked list */

static time_t	currentout;		/* current logout value */
static long	maxrec;			/* records to display */
static int	fulltime = 0;		/* Display seconds? */

int	 main(int, char *[]);

static void	 addarg(int, char *);
static TTY	*addtty(const char *);
static void	 hostconv(char *);
static char	*ttyconv(char *);
#ifdef SUPPORT_UTMPX
static void	 wtmpx(const char *, int, int, int);
#endif
#ifdef SUPPORT_UTMP
static void	 wtmp(const char *, int, int, int);
#endif
static char	*fmttime(time_t, int);
static void	 usage(void);

static
void usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-#%s] [-T] [-f file]"
	    " [-H hostsize] [-h host] [-L linesize]\n"
	    "\t    [-N namesize] [-t tty] [user ...]\n", getprogname(),
#ifdef NOTYET_SUPPORT_UTMPX
	    "w"
#else
	    ""
#endif
	);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int ch;
	char *p;
	char	*file = NULL;
	int namesize = UT_NAMESIZE;
	int linesize = UT_LINESIZE;
	int hostsize = UT_HOSTSIZE;

	maxrec = -1;

	while ((ch = getopt(argc, argv, "0123456789f:H:h:L:N:Tt:")) != -1)
		switch (ch) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			/*
			 * kludge: last was originally designed to take
			 * a number after a dash.
			 */
			if (maxrec == -1) {
				p = argv[optind - 1];
				if (p[0] == '-' && p[1] == ch && !p[2])
					maxrec = atol(++p);
				else
					maxrec = atol(argv[optind] + 1);
				if (!maxrec)
					exit(0);
			}
			break;
		case 'f':
			file = optarg;
			break;
		case 'H':
			hostsize = atoi(optarg);
			break;
		case 'h':
			hostconv(optarg);
			addarg(HOST_TYPE, optarg);
			break;
		case 'L':
			linesize = atoi(optarg);
			break;
		case 'N':
			namesize = atoi(optarg);
			break;
		case 'T':
			fulltime = 1;
			break;
		case 't':
			addarg(TTY_TYPE, ttyconv(optarg));
			break;
		case '?':
		default:
			usage();
		}

	if (argc) {
		setlinebuf(stdout);
		for (argv += optind; *argv; ++argv) {
#define	COMPATIBILITY
#ifdef	COMPATIBILITY
			/* code to allow "last p5" to work */
			addarg(TTY_TYPE, ttyconv(*argv));
#endif
			addarg(USER_TYPE, *argv);
		}
	}
	if (file == NULL) {
#ifdef SUPPORT_UTMPX
		if (access(_PATH_WTMPX, R_OK) == 0)
			file = _PATH_WTMPX;
		else
#endif
#ifdef SUPPORT_UTMP
		if (access(_PATH_WTMP, R_OK) == 0)
			file = _PATH_WTMP;
#endif
		if (file == NULL)
#if defined(SUPPORT_UTMPX) && defined(SUPPORT_UTMP)
			errx(1, "Cannot access `%s' or `%s'", _PATH_WTMPX,
			    _PATH_WTMP);
#elif defined(SUPPORT_UTMPX)
			errx(1, "Cannot access `%s'", _PATH_WTMPX);
#elif defined(SUPPORT_UTMP)
			errx(1, "Cannot access `%s'", _PATH_WTMP);
#else
			errx(1, "No utmp or utmpx support compiled in.");
#endif
	}
#if defined(SUPPORT_UTMPX) && defined(SUPPORT_UTMP)
	if (file[strlen(file) - 1] == 'x')
		wtmpx(file, namesize, linesize, hostsize);
	else
		wtmp(file, namesize, linesize, hostsize);
#elif defined(SUPPORT_UTMPX)
	wtmpx(file, namesize, linesize, hostsize);
#elif defined(SUPPORT_UTMP)
	wtmp(file, namesize, linesize, hostsize);
#else
	errx(1, "No utmp or utmpx support compiled in.");
#endif
	exit(0);
}


/*
 * addarg --
 *	add an entry to a linked list of arguments
 */
static void
addarg(int type, char *arg)
{
	ARG *cur;

	if (!(cur = (ARG *)malloc((u_int)sizeof(ARG))))
		err(1, "malloc failure");
	cur->next = arglist;
	cur->type = type;
	cur->name = arg;
	arglist = cur;
}

/*
 * addtty --
 *	add an entry to a linked list of ttys
 */
static TTY *
addtty(const char *ttyname)
{
	TTY *cur;

	if (!(cur = (TTY *)malloc((u_int)sizeof(TTY))))
		err(1, "malloc failure");
	cur->next = ttylist;
	cur->logout = currentout;
	memmove(cur->tty, ttyname, sizeof(cur->tty));
	return (ttylist = cur);
}

/*
 * hostconv --
 *	convert the hostname to search pattern; if the supplied host name
 *	has a domain attached that is the same as the current domain, rip
 *	off the domain suffix since that's what login(1) does.
 */
static void
hostconv(char *arg)
{
	static int first = 1;
	static char *hostdot, name[MAXHOSTNAMELEN + 1];
	char *argdot;

	if (!(argdot = strchr(arg, '.')))
		return;
	if (first) {
		first = 0;
		if (gethostname(name, sizeof(name)))
			err(1, "gethostname");
		name[sizeof(name) - 1] = '\0';
		hostdot = strchr(name, '.');
	}
	if (hostdot && !strcasecmp(hostdot, argdot))
		*argdot = '\0';
}

/*
 * ttyconv --
 *	convert tty to correct name.
 */
static char *
ttyconv(char *arg)
{
	char *mval;

	/*
	 * kludge -- we assume that all tty's end with
	 * a two character suffix.
	 */
	if (strlen(arg) == 2) {
		if (!strcmp(arg, "co"))
			mval = strdup("console");
		else
			asprintf(&mval, "tty%s", arg);
		if (!mval)
			err(1, "malloc failure");
		return (mval);
	}
	if (!strncmp(arg, _PATH_DEV, sizeof(_PATH_DEV) - 1))
		return (arg + 5);
	return (arg);
}

/*
 * fmttime --
 *	return pointer to (static) formatted time string.
 */
static char *
fmttime(time_t t, int flags)
{
	struct tm *tm;
	static char tbuf[TBUFLEN];

	tm = (flags & GMT) ? gmtime(&t) : localtime(&t);
	strftime(tbuf, sizeof(tbuf),
	    (flags & TIMEONLY)
	     ? (flags & FULLTIME ? LTFMTS : TFMTS)
	     : (flags & FULLTIME ? LTFMT : TFMT),
	    tm);
	return (tbuf);
}

#ifdef SUPPORT_UTMP
#define TYPE(a)	0
#define NAMESIZE UT_NAMESIZE
#define LINESIZE UT_LINESIZE
#define HOSTSIZE UT_HOSTSIZE
#define ut_timefld ut_time
#define FIRSTVALID 0
#include "want.c"
#undef TYPE /*(a)*/
#undef NAMESIZE
#undef LINESIZE
#undef HOSTSIZE
#undef ut_timefld
#undef FIRSTVALID
#endif

#ifdef SUPPORT_UTMPX
#define utmp utmpx
#define want wantx
#define wtmp wtmpx
#define buf bufx
#define onintr onintrx
#define TYPE(a) (a)->ut_type
#define NAMESIZE UTX_USERSIZE
#define LINESIZE UTX_LINESIZE
#define HOSTSIZE UTX_HOSTSIZE
#define ut_timefld ut_xtime
#define FIRSTVALID 1
#include "want.c"
#endif
