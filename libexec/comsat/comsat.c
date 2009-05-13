/*	$NetBSD: comsat.c,v 1.38.6.1 2009/05/13 19:18:37 jym Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\
 The Regents of the University of California.  All rights reserved.");
#if 0
static char sccsid[] = "from: @(#)comsat.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: comsat.c,v 1.38.6.1 2009/05/13 19:18:37 jym Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <paths.h>
#include <pwd.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <vis.h>
#include <unistd.h>
#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif

#include "utmpentry.h"

#if !defined(SUPPORT_UTMP) && !defined(SUPPORT_UTMPX)
	#error "SUPPORT_UTMP and/or SUPPORT_UTMPX must be defined"
#endif

#define	dsyslog	if (debug) syslog

#define MAXIDLE	120

static int	logging;
static int	debug;
static char	hostname[MAXHOSTNAMELEN + 1];
static int	nutmp;
static struct	utmpentry *utmp = NULL;
static time_t	lastmsgtime;
static volatile sig_atomic_t needupdate;

int main(int, char *[]);
static void jkfprintf(FILE *, const char *, off_t, const char *);
static void mailfor(const char *);
static void notify(const struct utmpentry *, off_t);
static void onalrm(int);
static void checkutmp(void);

int
main(int argc, char *argv[])
{
	struct sockaddr_storage from;
	int cc, ch;
	socklen_t fromlen;
	char msgbuf[100];
	sigset_t nsigset, osigset;

	/* verify proper invocation */
	fromlen = sizeof(from);
	if (getsockname(0, (struct sockaddr *)(void *)&from, &fromlen) == -1)
		err(1, "getsockname");

	openlog("comsat", LOG_PID, LOG_DAEMON);
	while ((ch = getopt(argc, argv, "l")) != -1)
		switch (ch) {
		case 'l':
			logging = 1;
			break;
		default:
			syslog(LOG_ERR, "Usage: %s [-l]", getprogname());
			exit(1);
		}
	if (chdir(_PATH_MAILDIR) == -1) {
		syslog(LOG_ERR, "chdir: %s: %m", _PATH_MAILDIR);
		(void)recv(0, msgbuf, sizeof(msgbuf) - 1, 0);
		exit(1);
	}
	(void)time(&lastmsgtime);
	(void)gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	(void)signal(SIGALRM, onalrm);
	(void)signal(SIGTTOU, SIG_IGN);
	(void)signal(SIGCHLD, SIG_IGN);
	(void)sigemptyset(&nsigset);
	(void)sigaddset(&nsigset, SIGALRM);
	if (sigprocmask(SIG_SETMASK, NULL, &osigset) == -1) {
		syslog(LOG_ERR, "sigprocmask get failed (%m)");
		exit(1);
	}
	needupdate = 1;
	for (;;) {
		cc = recv(0, msgbuf, sizeof(msgbuf) - 1, 0);
		if (cc <= 0) {
			if (errno != EINTR)
				sleep(1);
			errno = 0;
			checkutmp();
			continue;
		} else
			checkutmp();
		if (!nutmp)		/* no one has logged in yet */
			continue;
		if (sigprocmask(SIG_SETMASK, &nsigset, NULL) == -1) {
			syslog(LOG_ERR, "sigprocmask set failed (%m)");
			exit(1);
		}
		msgbuf[cc] = '\0';
		(void)time(&lastmsgtime);
		mailfor(msgbuf);
		if (sigprocmask(SIG_SETMASK, &osigset, NULL) == -1) {
			syslog(LOG_ERR, "sigprocmask restore failed (%m)");
			exit(1);
		}
	}
}

static void
/*ARGSUSED*/
onalrm(int signo)
{
	needupdate = 1;
}

static void
checkutmp(void)
{
	if (!needupdate)
		return;
	needupdate = 0;

	if (time(NULL) - lastmsgtime >= MAXIDLE)
		exit(0);
	(void)alarm((u_int)15);
	nutmp = getutentries(NULL, &utmp);
}

static void
mailfor(const char *name)
{
	struct utmpentry *ep;
	char *cp, *fn;
	off_t offset;
	intmax_t val;

	if (!(cp = strchr(name, '@')))
		return;
	*cp = '\0';
	errno = 0;
	offset = val = strtoimax(cp + 1, &fn, 10);
	if (errno == ERANGE || offset != val)
		return;
	if (fn && *fn && *fn != '\n') {
		/*
		 * Procmail sends messages to comsat with a trailing colon
		 * and a pathname to the folder where the new message was
		 * deposited.  Since we can't reliably open only regular
		 * files, we need to ignore these.  With one exception:
		 * if it mentions the user's system mailbox.
		 */
		char maildir[MAXPATHLEN];
		int l = snprintf(maildir, sizeof(maildir), ":%s/%s",
		    _PATH_MAILDIR, name);
		if (l >= (int)sizeof(maildir) || strcmp(maildir, fn) != 0)
			return;
	}
	for (ep = utmp; ep != NULL; ep = ep->next)
		if (strcmp(ep->name, name) == 0)
			notify(ep, offset);
}

static void
notify(const struct utmpentry *ep, off_t offset)
{
	FILE *tp;
	struct passwd *p;
	struct stat stb;
	struct termios ttybuf;
	char tty[sizeof(_PATH_DEV) + sizeof(ep->line) + 1];
	const char *cr = ep->line;

	if (strncmp(cr, "pts/", 4) == 0)
		cr += 4;
	if (strchr(cr, '/')) {
		/* A slash is an attempt to break security... */
		syslog(LOG_AUTH | LOG_NOTICE, "Unexpected `/' in `%s'",
		    ep->line);
		return;
	}
	(void)snprintf(tty, sizeof(tty), "%s%s", _PATH_DEV, ep->line);
	if (stat(tty, &stb) == -1 || !(stb.st_mode & S_IEXEC)) {
		dsyslog(LOG_DEBUG, "%s: wrong mode on %s", ep->name, tty);
		return;
	}
	dsyslog(LOG_DEBUG, "notify %s on %s", ep->name, tty);
	switch (fork()) {
	case -1:
		syslog(LOG_NOTICE, "fork failed (%m)");
		return;
	case 0:
		break;
	default:
		return;
	}
	(void)signal(SIGALRM, SIG_DFL);
	(void)alarm((u_int)30);
	if ((tp = fopen(tty, "w")) == NULL) {
		dsyslog(LOG_ERR, "open `%s' (%s)", tty, strerror(errno));
		_exit(1);
	}
	if (tcgetattr(fileno(tp), &ttybuf) == -1) {
		dsyslog(LOG_ERR, "tcgetattr `%s' (%s)", tty, strerror(errno));
		_exit(1);
	}
	cr = (ttybuf.c_oflag & ONLCR) && (ttybuf.c_oflag & OPOST) ?
	    "\n" : "\n\r";
	/* Set uid/gid/groups to users in case mail drop is on nfs */
	if ((p = getpwnam(ep->name)) == NULL ||
	    initgroups(p->pw_name, p->pw_gid) == -1 ||
	    setgid(p->pw_gid) == -1 ||
	    setuid(p->pw_uid) == -1)
		_exit(1);

	if (logging)
		syslog(LOG_INFO, "biff message for %s", ep->name);

	(void)fprintf(tp, "%s\007New mail for %s@%.*s\007 has arrived:%s----%s",
	    cr, ep->name, (int)sizeof(hostname), hostname, cr, cr);
	jkfprintf(tp, ep->name, offset, cr);
	(void)fclose(tp);
	_exit(0);
}

static void
jkfprintf(FILE *tp, const char *name, off_t offset, const char *cr)
{
	FILE *fi;
	int linecnt, charcnt, inheader;
	char line[BUFSIZ], visline[BUFSIZ * 4 + 1], *nl;

	if ((fi = fopen(name, "r")) == NULL)
		return;

	(void)fseeko(fi, offset, SEEK_SET);
	/*
	 * Print the first 7 lines or 560 characters of the new mail
	 * (whichever comes first).  Skip header crap other than
	 * From, Subject, To, and Date.
	 */
	linecnt = 7;
	charcnt = 560;
	inheader = 1;
	while (fgets(line, sizeof(line), fi) != NULL) {
		line[sizeof(line) - 1] = '\0';
		if (inheader) {
			if (line[0] == '\n') {
				inheader = 0;
				continue;
			}
			if (line[0] == ' ' || line[0] == '\t' ||
			    (strncasecmp(line, "From:", 5) &&
			    strncasecmp(line, "Subject:", 8)))
				continue;
		}
		if (strncmp(line, "From ", 5) == 0) {
			(void)fprintf(tp, "----%s", cr);
			(void)fclose(fi);
			return;
		}
		if (linecnt <= 0 || charcnt <= 0) {
			(void)fprintf(tp, "...more...%s", cr);
			(void)fclose(fi);
			return;
		}
		if ((nl = strchr(line, '\n')) != NULL)
			*nl = '\0';
		/* strip weird stuff so can't trojan horse stupid terminals */
		(void)strvis(visline, line, VIS_CSTYLE);
		(void)fputs(visline, tp);
		(void)fputs(cr, tp);
		--linecnt;
	}
	(void)fprintf(tp, "----%s\n", cr);
	(void)fclose(fi);
}
