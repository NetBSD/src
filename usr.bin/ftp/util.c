/*	$NetBSD: util.c,v 1.39 1999/01/01 03:55:26 lukem Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1985, 1989, 1993, 1994
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
__RCSID("$NetBSD: util.c,v 1.39 1999/01/01 03:55:26 lukem Exp $");
#endif /* not lint */

/*
 * FTP User Program -- Misc support routines
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/ftp.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <glob.h>
#include <termios.h>
#include <signal.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "ftp_var.h"
#include "pathnames.h"

#ifndef HAVE_TIMEGM
static time_t	sub_mkgmt __P((struct tm *tm));
#endif

/*
 * Connect to peer server and
 * auto-login, if possible.
 */
void
setpeer(argc, argv)
	int argc;
	char *argv[];
{
	char *host;
	in_port_t port;

	if (connected) {
		fprintf(ttyout, "Already connected to %s, use close first.\n",
		    hostname);
		code = -1;
		return;
	}
	if (argc < 2)
		(void)another(&argc, &argv, "to");
	if (argc < 2 || argc > 3) {
		fprintf(ttyout, "usage: %s host-name [port]\n", argv[0]);
		code = -1;
		return;
	}
	if (gatemode)
		port = gateport;
	else
		port = ftpport;
	if (argc > 2) {
		char *ep;
		long nport;

		nport = strtol(argv[2], &ep, 10);
		if (nport < 1 || nport > MAX_IN_PORT_T || *ep != '\0') {
			fprintf(ttyout, "%s: bad port number '%s'.\n",
			    argv[1], argv[2]);
			fprintf(ttyout, "usage: %s host-name [port]\n",
			    argv[0]);
			code = -1;
			return;
		}
		port = htons((in_port_t)nport);
	}

	if (gatemode) {
		if (gateserver == NULL || *gateserver == '\0')
			errx(1, "gateserver not defined (shouldn't happen)");
		host = hookup(gateserver, port);
	} else
		host = hookup(argv[1], port);

	if (host) {
		int overbose;

		if (gatemode) {
			if (command("PASSERVE %s", argv[1]) != COMPLETE)
				return;
			if (verbose)
				fprintf(ttyout,
				    "Connected via pass-through server %s\n",
				    gateserver);
		}

		connected = 1;
		/*
		 * Set up defaults for FTP.
		 */
		(void)strcpy(typename, "ascii"), type = TYPE_A;
		curtype = TYPE_A;
		(void)strcpy(formname, "non-print"), form = FORM_N;
		(void)strcpy(modename, "stream"), mode = MODE_S;
		(void)strcpy(structname, "file"), stru = STRU_F;
		(void)strcpy(bytename, "8"), bytesize = 8;
		if (autologin)
			(void)ftp_login(argv[1], NULL, NULL);

		overbose = verbose;
		if (debug == 0)
			verbose = -1;
		if (command("SYST") == COMPLETE && overbose) {
			char *cp, c;
			c = 0;
			cp = strchr(reply_string + 4, ' ');
			if (cp == NULL)
				cp = strchr(reply_string + 4, '\r');
			if (cp) {
				if (cp[-1] == '.')
					cp--;
				c = *cp;
				*cp = '\0';
			}

			fprintf(ttyout, "Remote system type is %s.\n",
			    reply_string + 4);
			if (cp)
				*cp = c;
		}
		if (!strncmp(reply_string, "215 UNIX Type: L8", 17)) {
			if (proxy)
				unix_proxy = 1;
			else
				unix_server = 1;
			/*
			 * Set type to 0 (not specified by user),
			 * meaning binary by default, but don't bother
			 * telling server.  We can use binary
			 * for text files unless changed by the user.
			 */
			type = 0;
			(void)strcpy(typename, "binary");
			if (overbose)
			    fprintf(ttyout,
				"Using %s mode to transfer files.\n",
				typename);
		} else {
			if (proxy)
				unix_proxy = 0;
			else
				unix_server = 0;
			if (overbose &&
			    !strncmp(reply_string, "215 TOPS20", 10))
				fputs(
"Remember to set tenex mode when transferring binary files from this machine.\n",
				    ttyout);
		}
		verbose = overbose;
	}
}

/*
 * login to remote host, using given username & password if supplied
 */
int
ftp_login(host, user, pass)
	const char *host;
	const char *user, *pass;
{
	char tmp[80];
	const char *acct;
	char anonpass[MAXLOGNAME + 2]; /* "user@" */
	struct passwd *pw;
	int n, aflag = 0;

	acct = NULL;
	if (user == NULL) {
		if (ruserpass(host, &user, &pass, &acct) < 0) {
			code = -1;
			return (0);
		}
	}

	/*
	 * Set up arguments for an anonymous FTP session, if necessary.
	 */
	if ((user == NULL || pass == NULL) && anonftp) {
		memset(anonpass, 0, sizeof(anonpass));

		/*
		 * Set up anonymous login password.
		 */
		if ((pass = getenv("FTPANONPASS")) == NULL) {
			if ((pass = getlogin()) == NULL) {
				if ((pw = getpwuid(getuid())) == NULL)
					pass = "anonymous";
				else
					pass = pw->pw_name;
			}
			/*
			 * Every anonymous FTP server I've encountered
			 * will accept the string "username@", and will
			 * append the hostname itself.  We do this by default
			 * since many servers are picky about not having
			 * a FQDN in the anonymous password.
			 * - thorpej@netbsd.org
			 */
			snprintf(anonpass, sizeof(anonpass) - 1, "%s@", pass);
			pass = anonpass;
		}
		user = "anonymous";	/* as per RFC 1635 */
	}

	while (user == NULL) {
		const char *myname = getlogin();

		if (myname == NULL && (pw = getpwuid(getuid())) != NULL)
			myname = pw->pw_name;
		if (myname)
			fprintf(ttyout, "Name (%s:%s): ", host, myname);
		else
			fprintf(ttyout, "Name (%s): ", host);
		*tmp = '\0';
		(void)fgets(tmp, sizeof(tmp) - 1, stdin);
		tmp[strlen(tmp) - 1] = '\0';
		if (*tmp == '\0')
			user = myname;
		else
			user = tmp;
	}
	n = command("USER %s", user);
	if (n == CONTINUE) {
		if (pass == NULL)
			pass = getpass("Password:");
		n = command("PASS %s", pass);
	}
	if (n == CONTINUE) {
		aflag++;
		if (acct == NULL)
			acct = getpass("Account:");
		n = command("ACCT %s", acct);
	}
	if ((n != COMPLETE) ||
	    (!aflag && acct != NULL && command("ACCT %s", acct) != COMPLETE)) {
		warnx("Login failed.");
		return (0);
	}
	if (proxy)
		return (1);
	connected = -1;
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void)strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
	return (1);
}

/*
 * `another' gets another argument, and stores the new argc and argv.
 * It reverts to the top level (via main.c's intr()) on EOF/error.
 *
 * Returns false if no new arguments have been added.
 */
int
another(pargc, pargv, prompt)
	int *pargc;
	char ***pargv;
	const char *prompt;
{
	int len = strlen(line), ret;

	if (len >= sizeof(line) - 3) {
		fputs("sorry, arguments too long.\n", ttyout);
		intr();
	}
	fprintf(ttyout, "(%s) ", prompt);
	line[len++] = ' ';
	if (fgets(&line[len], sizeof(line) - len, stdin) == NULL)
		intr();
	len += strlen(&line[len]);
	if (len > 0 && line[len - 1] == '\n')
		line[len - 1] = '\0';
	makeargv();
	ret = margc > *pargc;
	*pargc = margc;
	*pargv = margv;
	return (ret);
}

/*
 * glob files given in argv[] from the remote server.
 * if errbuf isn't NULL, store error messages there instead
 * of writing to the screen.
 */
char *
remglob(argv, doswitch, errbuf)
        char *argv[];
        int doswitch;
	char **errbuf;
{
        char temp[MAXPATHLEN];
        static char buf[MAXPATHLEN];
        static FILE *ftemp = NULL;
        static char **args;
        int oldverbose, oldhash, fd;
        char *cp, *mode;

        if (!mflag) {
                if (!doglob)
                        args = NULL;
                else {
                        if (ftemp) {
                                (void)fclose(ftemp);
                                ftemp = NULL;
                        }
                }
                return (NULL);
        }
        if (!doglob) {
                if (args == NULL)
                        args = argv;
                if ((cp = *++args) == NULL)
                        args = NULL;
                return (cp);
        }
        if (ftemp == NULL) {
                (void)snprintf(temp, sizeof(temp), "%s/%s", tmpdir, TMPFILE);
                if ((fd = mkstemp(temp)) < 0) {
                        warn("unable to create temporary file %s", temp);
                        return (NULL);
                }
                close(fd);
                oldverbose = verbose;
		verbose = (errbuf != NULL) ? -1 : 0;
                oldhash = hash;
                hash = 0;
                if (doswitch)
                        pswitch(!proxy);
                for (mode = "w"; *++argv != NULL; mode = "a")
                        recvrequest("NLST", temp, *argv, mode, 0, 0);
		if ((code / 100) != COMPLETE) {
			if (errbuf != NULL)
				*errbuf = reply_string;
		}
                if (doswitch)
                        pswitch(!proxy);
                verbose = oldverbose;
		hash = oldhash;
                ftemp = fopen(temp, "r");
                (void)unlink(temp);
                if (ftemp == NULL) {
			if (errbuf == NULL)
				fputs(
				    "can't find list of remote files, oops.\n",
				    ttyout);
			else
				*errbuf =
				    "can't find list of remote files, oops.";
                        return (NULL);
                }
        }
        if (fgets(buf, sizeof(buf), ftemp) == NULL) {
                (void)fclose(ftemp);
		ftemp = NULL;
                return (NULL);
        }
        if ((cp = strchr(buf, '\n')) != NULL)
                *cp = '\0';
        return (buf);
}

int
confirm(cmd, file)
	const char *cmd, *file;
{
	char line[BUFSIZ];

	if (!interactive || confirmrest)
		return (1);
	fprintf(ttyout, "%s %s? ", cmd, file);
	(void)fflush(ttyout);
	if (fgets(line, sizeof(line), stdin) == NULL)
		return (0);
	switch (tolower(*line)) {
		case 'n':
			return (0);
		case 'p':
			interactive = 0;
			fputs("Interactive mode: off.\n", ttyout);
			break;
		case 'a':
			confirmrest = 1;
			fprintf(ttyout, "Prompting off for duration of %s.\n",
			    cmd);
			break;
	}
	return (1);
}

/*
 * Glob a local file name specification with
 * the expectation of a single return value.
 * Can't control multiple values being expanded
 * from the expression, we return only the first.
 */
int
globulize(cpp)
	char **cpp;
{
	glob_t gl;
	int flags;

	if (!doglob)
		return (1);

	flags = GLOB_BRACE|GLOB_NOCHECK|GLOB_TILDE;
	memset(&gl, 0, sizeof(gl));
	if (glob(*cpp, flags, NULL, &gl) ||
	    gl.gl_pathc == 0) {
		warnx("%s: not found", *cpp);
		globfree(&gl);
		return (0);
	}
		/* XXX: caller should check if *cpp changed, and
		 *	free(*cpp) if that is the case
		 */
	*cpp = xstrdup(gl.gl_pathv[0]);
	globfree(&gl);
	return (1);
}

/*
 * determine size of remote file
 */
off_t
remotesize(file, noisy)
	const char *file;
	int noisy;
{
	int overbose;
	off_t size;

	overbose = verbose;
	size = -1;
	if (debug == 0)
		verbose = -1;
	if (command("SIZE %s", file) == COMPLETE) {
		char *cp, *ep;

		cp = strchr(reply_string, ' ');
		if (cp != NULL) {
			cp++;
#ifndef NO_QUAD
			size = strtoq(cp, &ep, 10);
#else
			size = strtol(cp, &ep, 10);
#endif
			if (*ep != '\0' && !isspace((unsigned char)*ep))
				size = -1;
		}
	} else if (noisy && debug == 0) {
		fputs(reply_string, ttyout);
		putc('\n', ttyout);
	}
	verbose = overbose;
	return (size);
}

/*
 * determine last modification time (in GMT) of remote file
 */
time_t
remotemodtime(file, noisy)
	const char *file;
	int noisy;
{
	int overbose;
	time_t rtime;
	int ocode;

	overbose = verbose;
	ocode = code;
	rtime = -1;
	if (debug == 0)
		verbose = -1;
	if (command("MDTM %s", file) == COMPLETE) {
		struct tm timebuf;
		int yy, mo, day, hour, min, sec;
		sscanf(reply_string, "%*s %04d%02d%02d%02d%02d%02d", &yy, &mo,
			&day, &hour, &min, &sec);
		memset(&timebuf, 0, sizeof(timebuf));
		timebuf.tm_sec = sec;
		timebuf.tm_min = min;
		timebuf.tm_hour = hour;
		timebuf.tm_mday = day;
		timebuf.tm_mon = mo - 1;
		timebuf.tm_year = yy - TM_YEAR_BASE;
		timebuf.tm_isdst = -1;
		rtime = mkgmtime(&timebuf);
		if (rtime == -1 && (noisy || debug != 0))
			fprintf(ttyout, "Can't convert %s to a time.\n",
			    reply_string);
	} else if (noisy && debug == 0) {
		fputs(reply_string, ttyout);
		putc('\n', ttyout);
	}
	verbose = overbose;
	if (rtime == -1)
		code = ocode;
	return (rtime);
}

/*
 * UTC version of mktime(3)
 */
#ifdef HAVE_TIMEGM
time_t
mkgmtime(tm)
	struct tm *tm;
{

	/* This is very clean, but not portable at all. */
	return (timegm(tm));
}

#else	/* not HAVE_TIMEGM */

/*
 * This code is not portable, but works on most Unix-like systems.
 * If the local timezone has no summer time, using mktime(3) function
 * and adjusting offset would be usable (adjusting leap seconds
 * is still required, though), but the assumption is not always true.
 *
 * Anyway, no portable and correct implementation of UTC to time_t
 * conversion exists....
 */

static time_t
sub_mkgmt(tm)
	struct tm *tm;
{
	int y, nleapdays;
	time_t t;
	/* days before the month */
	static const unsigned short moff[12] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};

	/*
	 * XXX This code assumes the given time to be normalized.
	 * Normalizing here is impossible in case the given time is a leap
	 * second but the local time library is ignorant of leap seconds.
	 */

	/* minimal sanity checking not to access outside of the array */
	if ((unsigned) tm->tm_mon >= 12)
		return (time_t) -1;
	if (tm->tm_year < EPOCH_YEAR - TM_YEAR_BASE)
		return (time_t) -1;

	y = tm->tm_year + TM_YEAR_BASE - (tm->tm_mon < 2);
	nleapdays = y / 4 - y / 100 + y / 400 -
	    ((EPOCH_YEAR-1) / 4 - (EPOCH_YEAR-1) / 100 + (EPOCH_YEAR-1) / 400);
	t = ((((time_t) (tm->tm_year - (EPOCH_YEAR - TM_YEAR_BASE)) * 365 +
			moff[tm->tm_mon] + tm->tm_mday - 1 + nleapdays) * 24 +
		tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec;

	return (t < 0 ? (time_t) -1 : t);
}

time_t
mkgmtime(tm)
	struct tm *tm;
{
	time_t t, t2;
	struct tm *tm2;
	int sec;

	/* Do the first guess. */
	if ((t = sub_mkgmt(tm)) == (time_t) -1)
		return (time_t) -1;

	/* save value in case *tm is overwritten by gmtime() */
	sec = tm->tm_sec;

	tm2 = gmtime(&t);
	if ((t2 = sub_mkgmt(tm2)) == (time_t) -1)
		return (time_t) -1;

	if (t2 < t || tm2->tm_sec != sec) {
		/*
		 * Adjust for leap seconds.
		 *
		 *     real time_t time
		 *           |
		 *          tm
		 *         /	... (a) first sub_mkgmt() conversion
		 *       t
		 *       |
		 *      tm2
		 *     /	... (b) second sub_mkgmt() conversion
		 *   t2
		 *			--->time
		 */
		/*
		 * Do the second guess, assuming (a) and (b) are almost equal.
		 */
		t += t - t2;
		tm2 = gmtime(&t);

		/*
		 * Either (a) or (b), may include one or two extra
		 * leap seconds.  Try t, t + 2, t - 2, t + 1, and t - 1.
		 */
		if (tm2->tm_sec == sec
		    || (t += 2, tm2 = gmtime(&t), tm2->tm_sec == sec)
		    || (t -= 4, tm2 = gmtime(&t), tm2->tm_sec == sec)
		    || (t += 3, tm2 = gmtime(&t), tm2->tm_sec == sec)
		    || (t -= 2, tm2 = gmtime(&t), tm2->tm_sec == sec))
			;	/* found */
		else {
			/*
			 * Not found.
			 */
			if (sec >= 60)
				/*
				 * The given time is a leap second
				 * (sec 60 or 61), but the time library
				 * is ignorant of the leap second.
				 */
				;	/* treat sec 60 as 59,
					   sec 61 as 0 of the next minute */
			else
				/* The given time may not be normalized. */
				t++;	/* restore t */
		}
	}

	return (t < 0 ? (time_t) -1 : t);
}
#endif	/* not HAVE_TIMEGM */

#ifndef	SMALL

/*
 * return non-zero if we're the current foreground process
 */
int
foregroundproc()
{
	static pid_t pgrp = -1;
	int ctty_pgrp;

	if (pgrp == -1)
		pgrp = getpgrp();

	return ((ioctl(fileno(ttyout), TIOCGPGRP, &ctty_pgrp) != -1 &&
	    ctty_pgrp == (int)pgrp));
}


static void updateprogressmeter __P((int));

static void
updateprogressmeter(dummy)
	int dummy;
{

	/*
	 * print progress bar only if we are foreground process.
	 */
	if (foregroundproc())
		progressmeter(0);
}
#endif	/* SMALL */


/*
 * List of order of magnitude prefixes.
 * The last is `P', as 2^64 = 16384 Petabytes
 */
static const char prefixes[] = " KMGTP";

/*
 * Display a transfer progress bar if progress is non-zero.
 * SIGALRM is hijacked for use by this function.
 * - Before the transfer, set filesize to size of file (or -1 if unknown),
 *   and call with flag = -1. This starts the once per second timer,
 *   and a call to updateprogressmeter() upon SIGALRM.
 * - During the transfer, updateprogressmeter will call progressmeter
 *   with flag = 0
 * - After the transfer, call with flag = 1
 */
static struct timeval start;
static struct timeval lastupdate;

void
progressmeter(flag)
	int flag;
{
#ifndef	SMALL

	static off_t lastsize;
	struct timeval now, td, wait;
	off_t cursize, abbrevsize, bytespersec;
	double elapsed;
	int ratio, barlength, i, len, remaining;
	char buf[256];

	len = 0;

	if (flag == -1) {
		(void)gettimeofday(&start, NULL);
		lastupdate = start;
		lastsize = restart_point;
	}
	(void)gettimeofday(&now, NULL);
	if (!progress || filesize <= 0)
		return;
	cursize = bytes + restart_point;

	ratio = (int)((double)cursize * 100.0 / (double)filesize);
	ratio = MAX(ratio, 0);
	ratio = MIN(ratio, 100);
	len += snprintf(buf + len, sizeof(buf) - len, "\r%3d%% ", ratio);

	barlength = ttywidth - 43;
	if (barlength > 0) {
		i = barlength * ratio / 100;
		len += snprintf(buf + len, sizeof(buf) - len,
		    "|%.*s%*s|", i, 
"*****************************************************************************"
"*****************************************************************************",
		    barlength - i, "");
	}

	abbrevsize = cursize;
	for (i = 0; abbrevsize >= 100000 && i < sizeof(prefixes); i++)
		abbrevsize >>= 10;
	len += snprintf(buf + len, sizeof(buf) - len,
#ifndef NO_QUAD
	    " %5qd %c%c ", (long long)abbrevsize,
#else
	    " %5ld %c%c ", (long)abbrevsize,
#endif
	    prefixes[i],
	    i == 0 ? ' ' : 'B');

	timersub(&now, &lastupdate, &wait);
	if (cursize > lastsize) {
		lastupdate = now;
		lastsize = cursize;
		if (wait.tv_sec >= STALLTIME) {	/* fudge out stalled time */
			start.tv_sec += wait.tv_sec;
			start.tv_usec += wait.tv_usec;
		}
		wait.tv_sec = 0;
	}

	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);

	bytespersec = 0;
	if (bytes > 0) {
		bytespersec = bytes;
		if (elapsed > 0.0)
			bytespersec /= elapsed;
	}
	for (i = 1; bytespersec >= 1024000 && i < sizeof(prefixes); i++)
		bytespersec >>= 10;
	len += snprintf(buf + len, sizeof(buf) - len,
#ifndef NO_QUAD
	    " %3qd.%02d %cB/s ", (long long)bytespersec / 1024,
#else
	    " %3ld.%02d %cB/s ", (long)bytespersec / 1024,
#endif
	    (int)((bytespersec % 1024) * 100 / 1024),
	    prefixes[i]);

	if (bytes <= 0 || elapsed <= 0.0 || cursize > filesize) {
		len += snprintf(buf + len, sizeof(buf) - len,
		    "   --:-- ETA");
	} else if (wait.tv_sec >= STALLTIME) {
		len += snprintf(buf + len, sizeof(buf) - len,
		    " - stalled -");
	} else {
		remaining = (int)
		    ((filesize - restart_point) / (bytes / elapsed) - elapsed);
		if (remaining >= 100 * SECSPERHOUR)
			len += snprintf(buf + len, sizeof(buf) - len,
			    "   --:-- ETA");
		else {
			i = remaining / SECSPERHOUR;
			if (i)
				len += snprintf(buf + len, sizeof(buf) - len,
				    "%2d:", i);
			else
				len += snprintf(buf + len, sizeof(buf) - len,
				    "   ");
			i = remaining % SECSPERHOUR;
			len += snprintf(buf + len, sizeof(buf) - len,
			    "%02d:%02d ETA", i / 60, i % 60);
		}
	}
	(void)write(fileno(ttyout), buf, len);

	if (flag == -1) {
		(void)xsignal(SIGALRM, updateprogressmeter);
		alarmtimer(1);		/* set alarm timer for 1 Hz */
	} else if (flag == 1) {
		(void)xsignal(SIGALRM, SIG_DFL);
		alarmtimer(0);
		(void)putc('\n', ttyout);
	}
	fflush(ttyout);
#endif	/* SMALL */
}

/*
 * Display transfer statistics.
 * Requires start to be initialised by progressmeter(-1),
 * direction to be defined by xfer routines, and filesize and bytes
 * to be updated by xfer routines
 * If siginfo is nonzero, an ETA is displayed, and the output goes to stderr
 * instead of ttyout.
 */
void
ptransfer(siginfo)
	int siginfo;
{
#ifndef	SMALL
	struct timeval now, td, wait;
	double elapsed;
	off_t bytespersec;
	int remaining, hh, i, len;
	char buf[100];

	if (!verbose && !siginfo)
		return;

	(void)gettimeofday(&now, NULL);
	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);
	bytespersec = 0;
	if (bytes > 0) {
		bytespersec = bytes;
		if (elapsed > 0.0)
			bytespersec /= elapsed;
	}
	len = 0;
	len += snprintf(buf + len, sizeof(buf) - len,
#ifndef NO_QUAD
	    "%qd byte%s %s in ", (long long)bytes,
#else
	    "%ld byte%s %s in ", (long)bytes,
#endif
	    bytes == 1 ? "" : "s", direction);
	remaining = (int)elapsed;
	if (remaining > SECSPERDAY) {
		int days;

		days = remaining / SECSPERDAY;
		remaining %= SECSPERDAY;
		len += snprintf(buf + len, sizeof(buf) - len,
		    "%d day%s ", days, days == 1 ? "" : "s");
	}
	hh = remaining / SECSPERHOUR;
	remaining %= SECSPERHOUR;
	if (hh)
		len += snprintf(buf + len, sizeof(buf) - len, "%2d:", hh);
	len += snprintf(buf + len, sizeof(buf) - len,
	    "%02d:%02d ", remaining / 60, remaining % 60);

	for (i = 1; bytespersec >= 1024000 && i < sizeof(prefixes); i++)
		bytespersec >>= 10;
	len += snprintf(buf + len, sizeof(buf) - len,
#ifndef NO_QUAD
	    "(%qd.%02d %cB/s)", (long long)bytespersec / 1024,
#else
	    "(%ld.%02d %cB/s)", (long)bytespersec / 1024,
#endif
	    (int)((bytespersec % 1024) * 100 / 1024),
	    prefixes[i]);

	if (siginfo && bytes > 0 && elapsed > 0.0 && filesize >= 0
	    && bytes + restart_point <= filesize) {
		remaining = (int)((filesize - restart_point) /
				  (bytes / elapsed) - elapsed);
		hh = remaining / SECSPERHOUR;
		remaining %= SECSPERHOUR;
		len += snprintf(buf + len, sizeof(buf) - len, "  ETA: ");
		if (hh)
			len += snprintf(buf + len, sizeof(buf) - len, "%2d:",
			    hh);
		len += snprintf(buf + len, sizeof(buf) - len,
		    "%02d:%02d", remaining / 60, remaining % 60);
		timersub(&now, &lastupdate, &wait);
		if (wait.tv_sec >= STALLTIME)
			len += snprintf(buf + len, sizeof(buf) - len,
			    "  (stalled)");
	}
	len += snprintf(buf + len, sizeof(buf) - len, "\n");
	(void)write(siginfo ? STDERR_FILENO : fileno(ttyout), buf, len);
#endif	/* SMALL */
}

/*
 * List words in stringlist, vertically arranged
 */
void
list_vertical(sl)
	StringList *sl;
{
	int i, j, w;
	int columns, width, lines, items;
	char *p;

	width = items = 0;

	for (i = 0 ; i < sl->sl_cur ; i++) {
		w = strlen(sl->sl_str[i]);
		if (w > width)
			width = w;
	}
	width = (width + 8) &~ 7;

	columns = ttywidth / width;
	if (columns == 0)
		columns = 1;
	lines = (sl->sl_cur + columns - 1) / columns;
	for (i = 0; i < lines; i++) {
		for (j = 0; j < columns; j++) {
			p = sl->sl_str[j * lines + i];
			if (p)
				fputs(p, ttyout);
			if (j * lines + i + lines >= sl->sl_cur) {
				putc('\n', ttyout);
				break;
			}
			w = strlen(p);
			while (w < width) {
				w = (w + 8) &~ 7;
				(void)putc('\t', ttyout);
			}
		}
	}
}

/*
 * Update the global ttywidth value, using TIOCGWINSZ.
 */
void
setttywidth(a)
	int a;
{
	struct winsize winsize;

	if (ioctl(fileno(ttyout), TIOCGWINSZ, &winsize) != -1 &&
	    winsize.ws_col != 0)
		ttywidth = winsize.ws_col;
	else
		ttywidth = 80;
}

/*
 * Set the SIGALRM interval timer for wait seconds, 0 to disable.
 */
void
alarmtimer(wait)
	int wait;
{
	struct itimerval itv;

	itv.it_value.tv_sec = wait;
	itv.it_value.tv_usec = 0;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL);
}

/*
 * Setup or cleanup EditLine structures
 */
#ifndef SMALL
void
controlediting()
{
	if (editing && el == NULL && hist == NULL) {
		HistEvent ev;
		int editmode;

		el = el_init(__progname, stdin, ttyout, stderr);
		/* init editline */
		hist = history_init();		/* init the builtin history */
		history(hist, &ev, H_SETSIZE, 100);/* remember 100 events */
		el_set(el, EL_HIST, history, hist);	/* use history */

		el_set(el, EL_EDITOR, "emacs");	/* default editor is emacs */
		el_set(el, EL_PROMPT, prompt);	/* set the prompt function */

		/* add local file completion, bind to TAB */
		el_set(el, EL_ADDFN, "ftp-complete",
		    "Context sensitive argument completion",
		    complete);
		el_set(el, EL_BIND, "^I", "ftp-complete", NULL);
		el_source(el, NULL);	/* read ~/.editrc */
		if ((el_get(el, EL_EDITMODE, &editmode) != -1) && editmode == 0)
			editing = 0;	/* the user doesn't want editing,
					 * so disable, and let statement
					 * below cleanup */
		else
			el_set(el, EL_SIGNAL, 1);
	}
	if (!editing) {
		if (hist) {
			history_end(hist);
			hist = NULL;
		}
		if (el) {
			el_end(el);
			el = NULL;
		}
	}
}
#endif /* !SMALL */

/*
 * Parse the specified socket buffer size.
 */
int
getsockbufsize(arg)
	const char *arg;
{
	char *cp;
	int val;

	if (!isdigit((unsigned char)arg[0]))
		return (-1);

	val = strtol(arg, &cp, 10);
	if (cp != NULL) {
		if (cp[1] != '\0')
			 return (-1);
		if (cp[0] == 'k')
			val *= 1024;
		if (cp[0] == 'm')
			val *= 1024 * 1024;
	}

	if (val < 0)
		return (-1);

	return (val);
}

/*
 * Set up socket buffer sizes before a connection is made.
 */
void
setupsockbufsize(sock)
	int sock;
{
	static int sndbuf_default, rcvbuf_default;
	int len, size;

	/*
	 * Get the default socket buffer sizes if we don't already
	 * have them.  It doesn't matter which socket we do this
	 * to, because on the first call no socket buffer sizes
	 * will have been modified, so we are guaranteed to get
	 * the system defaults.
	 */
	if (sndbuf_default == 0) {
		len = sizeof(sndbuf_default);
		if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf_default,
		    &len) < 0)
			err(1, "unable to get default sndbuf size");
		len = sizeof(rcvbuf_default);
		if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf_default,
		    &len) < 0)
			err(1, "unable to get default rcvbuf size");

	}

	size = sndbuf_size ? sndbuf_size : sndbuf_default;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0)
		warn("unable to set sndbuf size %d", size);

	size = rcvbuf_size ? rcvbuf_size : rcvbuf_default;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0)
		warn("unable to set rcvbuf size %d", size);
}

/*
 * If the socket buffer sizes were not set manually (i.e. came from a
 * configuration file), reset them so the right thing will happen on
 * subsequent connections.
 */
void
resetsockbufsize()
{

	if (sndbuf_manual == 0)
		sndbuf_size = 0;
	if (rcvbuf_manual == 0)
		rcvbuf_size = 0;
}

/*
 * Internal version of connect(2); sets socket buffer sizes first.
 */
int
xconnect(sock, name, namelen)
	int sock;
	const struct sockaddr *name;
	int namelen;
{

	setupsockbufsize(sock);
	return (connect(sock, name, namelen));
}

/*
 * Internal version of listen(2); sets socket buffer sizes first.
 */
int
xlisten(sock, backlog)
	int sock, backlog;
{

	setupsockbufsize(sock);
	return (listen(sock, backlog));
}

void *
xmalloc(size)
	size_t size;
{
	void *p;

	p = malloc(size);
	if (p == NULL)
		err(1, "Unable to allocate %ld bytes of memory", (long)size);
	return (p);
}

char *
xstrdup(str)
	const char *str;
{
	char *s;

	if (str == NULL)
		errx(1, "xstrdup() called with NULL argument");
	s = strdup(str);
	if (s == NULL)
		err(1, "Unable to allocate memory for string copy");
	return (s);
}

sig_t
xsignal(sig, func)
	int sig;
	void (*func) __P((int));
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	if (sigaction(sig, &act, &oact) < 0)
		return (SIG_ERR);
	return (oact.sa_handler);
}
