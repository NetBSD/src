/*	$NetBSD: lpd.c,v 1.28 2001/03/28 03:17:42 simonb Exp $	*/

/*
 * Copyright (c) 1983, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)lpd.c	8.7 (Berkeley) 5/10/95";
#else
__RCSID("$NetBSD: lpd.c,v 1.28 2001/03/28 03:17:42 simonb Exp $");
#endif
#endif /* not lint */

/*
 * lpd -- line printer daemon.
 *
 * Listen for a connection and perform the requested operation.
 * Operations are:
 *	\1printer\n
 *		check the queue for jobs and print any found.
 *	\2printer\n
 *		receive a job from another machine and queue it.
 *	\3printer [users ...] [jobs ...]\n
 *		return the current state of the queue (short form).
 *	\4printer [users ...] [jobs ...]\n
 *		return the current state of the queue (long form).
 *	\5printer person [users ...] [jobs ...]\n
 *		remove jobs from the queue.
 *
 * Strategy to maintain protected spooling area:
 *	1. Spooling area is writable only by daemon and spooling group
 *	2. lpr runs setuid root and setgrp spooling group; it uses
 *	   root to access any file it wants (verifying things before
 *	   with an access call) and group id to know how it should
 *	   set up ownership of files in the spooling area.
 *	3. Files in spooling area are owned by root, group spooling
 *	   group, with mode 660.
 *	4. lpd, lpq and lprm run setuid daemon and setgrp spooling group to
 *	   access files and printer.  Users can't get to anything
 *	   w/o help of lpq and lprm programs.
 */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <netinet/in.h>

#include <err.h>
#include <netdb.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "lp.h"
#include "lp.local.h"
#include "pathnames.h"
#include "extern.h"

/* XXX from libc/net/rcmd.c */
extern int __ivaliduser_sa __P((FILE *, struct sockaddr *, socklen_t,
		const char *, const char *));

int	lflag;				/* log requests flag */
int	rflag;				/* allow of for remote printers */
int	sflag;				/* secure (no inet) flag */
int	from_remote;			/* from remote socket */
char	**blist;			/* list of addresses to bind(2) to */
int	blist_size;
int	blist_addrs;

int               main __P((int, char **));
static void       reapchild __P((int));
static void       mcleanup __P((int));
static void       doit __P((void));
static void       startup __P((void));
static void       chkhost __P((struct sockaddr *));
static int	  ckqueue __P((char *));
static void	  usage __P((void));
static int	  *socksetup __P((int, int, const char *));

uid_t	uid, euid;
int child_count;

int
main(argc, argv)
	int argc;
	char **argv;
{
	fd_set defreadfds;
	struct sockaddr_un un, fromunix;
	struct sockaddr_storage frominet;
	sigset_t nmask, omask;
	int lfd, errs, i, f, funix, *finet;
	int child_max = 32;	/* more then enough to hose the system */
	int options = 0;
	struct servent *sp;
	const char *port = "printer";

	euid = geteuid();	/* these shouldn't be different */
	uid = getuid();
	gethostname(host, sizeof(host));
	host[sizeof(host) - 1] = '\0';
	name = argv[0];

	errs = 0;
	while ((i = getopt(argc, argv, "b:dln:srw:")) != -1)
		switch (i) {
		case 'b':
			if (blist_addrs >= blist_size) {
				blist_size += sizeof(char *) * 4;
				if (blist == NULL)
					blist = malloc(blist_size);
				else
					blist = realloc(blist, blist_size);
				if (blist == NULL)
					err(1, "cant allocate bind addr list");
			}
			blist[blist_addrs++] = strdup(optarg);
			break;
		case 'd':
			options |= SO_DEBUG;
			break;
		case 'l':
			lflag++;
			break;
		case 'n':
			child_max = atoi(optarg);
			if (child_max < 0 || child_max > 1024)
				errx(1, "invalid number of children: %s",
				    optarg);
			break;
		case 'r':
			rflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'w':
			wait_time = atoi(optarg);
			if (wait_time < 0)
				errx(1, "wait time must be postive: %s",
				    optarg);
			if (wait_time < 30)
			    warnx("warning: wait time less than 30 seconds");
			break;
		default:
			errs++;
		}
	argc -= optind;
	argv += optind;
	if (errs)
		usage();

	switch (argc) {
	case 1:
		if ((i = atoi(argv[0])) == 0)
			usage();
		if (i < 0 || i > USHRT_MAX)
			errx(1, "port # %d is invalid", i);

		port = argv[0];
		break;
	case 0:
		sp = getservbyname(port, "tcp");
		if (sp == NULL)
			errx(1, "%s/tcp: unknown service", port);
		break;
	default:
		usage();
	}

#ifndef DEBUG
	/*
	 * Set up standard environment by detaching from the parent.
	 */
	daemon(0, 0);
#endif

	openlog("lpd", LOG_PID, LOG_LPR);
	syslog(LOG_INFO, "restarted");
	(void)umask(0);
	lfd = open(_PATH_MASTERLOCK, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		syslog(LOG_ERR, "%s: %m", _PATH_MASTERLOCK);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active deamon present */
			exit(0);
		syslog(LOG_ERR, "%s: %m", _PATH_MASTERLOCK);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	(void)snprintf(line, sizeof(line), "%u\n", getpid());
	f = strlen(line);
	if (write(lfd, line, f) != f) {
		syslog(LOG_ERR, "%s: %m", _PATH_MASTERLOCK);
		exit(1);
	}
	signal(SIGCHLD, reapchild);
	/*
	 * Restart all the printers.
	 */
	startup();
	(void)unlink(_PATH_SOCKETNAME);
	funix = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (funix < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}

	sigemptyset(&nmask);
	sigaddset(&nmask, SIGHUP);
	sigaddset(&nmask, SIGINT);
	sigaddset(&nmask, SIGQUIT);
	sigaddset(&nmask, SIGTERM);
	sigprocmask(SIG_BLOCK, &nmask, &omask);

	(void) umask(07);
	signal(SIGHUP, mcleanup);
	signal(SIGINT, mcleanup);
	signal(SIGQUIT, mcleanup);
	signal(SIGTERM, mcleanup);
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_LOCAL;
	strncpy(un.sun_path, _PATH_SOCKETNAME, sizeof(un.sun_path) - 1);
#ifndef SUN_LEN
#define SUN_LEN(unp) (strlen((unp)->sun_path) + 2)
#endif
	if (bind(funix, (struct sockaddr *)&un, SUN_LEN(&un)) < 0) {
		syslog(LOG_ERR, "ubind: %m");
		exit(1);
	}
	(void) umask(0);
	sigprocmask(SIG_SETMASK, &omask, (sigset_t *)0);
	FD_ZERO(&defreadfds);
	FD_SET(funix, &defreadfds);
	listen(funix, 5);
	if (!sflag || blist_addrs)
		finet = socksetup(PF_UNSPEC, options, port);
	else
		finet = NULL;	/* pretend we couldn't open TCP socket. */

	if (blist != NULL) {
		for (i = 0; i < blist_addrs; i++)
			free(blist[i]);
		free(blist);
	}

	if (finet) {
		for (i = 1; i <= *finet; i++) {
			FD_SET(finet[i], &defreadfds);
			listen(finet[i], 5);
		}
	}
	/*
	 * Main loop: accept, do a request, continue.
	 */
	memset(&frominet, 0, sizeof(frominet));
	memset(&fromunix, 0, sizeof(fromunix));
	for (;;) {
		int domain, nfds, s, fromlen;
		fd_set readfds;
		/* "short" so it overflows in about 2 hours */
		short sleeptime = 10;

		while (child_max < child_count) {
			syslog(LOG_WARNING,
			    "too many children, sleeping for %d seconds",
				sleeptime);
			sleep(sleeptime);
			sleeptime <<= 1;
			if (sleeptime < 0) {
				syslog(LOG_CRIT, "sleeptime overflowed! help!");
				sleeptime = 10;
			}
		}

		FD_COPY(&defreadfds, &readfds);
		nfds = select(20, &readfds, 0, 0, 0);
		if (nfds <= 0) {
			if (nfds < 0 && errno != EINTR)
				syslog(LOG_WARNING, "select: %m");
			continue;
		}
		if (FD_ISSET(funix, &readfds)) {
			domain = AF_LOCAL;
			fromlen = sizeof(fromunix);
			s = accept(funix,
			    (struct sockaddr *)&fromunix, &fromlen);
		} else {
                        for (i = 1; i <= *finet; i++) 
				if (FD_ISSET(finet[i], &readfds)) {
					domain = AF_INET, fromlen = sizeof(frominet);
					s = accept(finet[i], (struct sockaddr *)&frominet, &fromlen);
				}
		}
		if (s < 0) {
			if (errno != EINTR)
				syslog(LOG_WARNING, "accept: %m");
			continue;
		}
		
		switch (fork()) {
		case 0:
			signal(SIGCHLD, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
			(void)close(funix);
			if (!sflag && finet)
                        	for (i = 1; i <= *finet; i++) 
					(void)close(finet[i]);
			dup2(s, 1);
			(void)close(s);
			if (domain == AF_INET) {
				/* for both AF_INET and AF_INET6 */
				from_remote = 1;
				chkhost((struct sockaddr *)&frominet);
			} else
				from_remote = 0;
			doit();
			exit(0);
		case -1:
			syslog(LOG_WARNING, "fork: %m, sleeping for 10 seconds...");
			sleep(10);
			continue;
		default:
			child_count++;
		}
		(void)close(s);
	}
}

static void
reapchild(signo)
	int signo;
{
	union wait status;

	while (wait3((int *)&status, WNOHANG, 0) > 0)
		child_count--;
}

static void
mcleanup(signo)
	int signo;
{
	if (lflag)
		syslog(LOG_INFO, "exiting");
	unlink(_PATH_SOCKETNAME);
	exit(0);
}

/*
 * Stuff for handling job specifications
 */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

char	fromb[NI_MAXHOST];	/* buffer for client's machine name */
char	cbuf[BUFSIZ];		/* command line buffer */
char	*cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob"
};

static void
doit()
{
	char *cp;
	int n;

	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long");
			if ((n = read(1, cp, 1)) != 1) {
				if (n < 0)
					fatal("Lost connection");
				return;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = cbuf;
		if (lflag) {
			if (*cp >= '\1' && *cp <= '\5') {
				syslog(LOG_INFO, "%s requests %s %s",
					from, cmdnames[(int)*cp], cp+1);
				setproctitle("serving %s: %s %s", from,
				    cmdnames[(int)*cp], cp+1);
			}
			else
				syslog(LOG_INFO, "bad request (%d) from %s",
					*cp, from);
		}
		switch (*cp++) {
		case '\1':	/* check the queue and print any jobs there */
			printer = cp;
			printjob();
			break;
		case '\2':	/* receive files to be queued */
			if (!from_remote) {
				syslog(LOG_INFO, "illegal request (%d)", *cp);
				exit(1);
			}
			printer = cp;
			recvjob();
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			displayq(cbuf[0] - '\3');
			exit(0);
		case '\5':	/* remove a job from the queue */
			if (!from_remote) {
				syslog(LOG_INFO, "illegal request (%d)", *cp);
				exit(1);
			}
			printer = cp;
			while (*cp && *cp != ' ')
				cp++;
			if (!*cp)
				break;
			*cp++ = '\0';
			person = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			rmjob();
			break;
		}
		fatal("Illegal service request");
	}
}

/*
 * Make a pass through the printcap database and start printing any
 * files left from the last time the machine went down.
 */
static void
startup()
{
	char *buf;
	char *cp;

	/*
	 * Restart the daemons.
	 */
	while (cgetnext(&buf, printcapdb) > 0) {
		if (ckqueue(buf) <= 0) {
			free(buf);
			continue;	/* no work to do for this printer */
		}
		for (cp = buf; *cp; cp++)
			if (*cp == '|' || *cp == ':') {
				*cp = '\0';
				break;
			}
		if (lflag)
			syslog(LOG_INFO, "work for %s", buf);
		switch (fork()) {
		case -1:
			syslog(LOG_WARNING, "startup: cannot fork");
			mcleanup(0);
		case 0:
			printer = buf;
			setproctitle("working on printer %s", printer);
			cgetclose();
			printjob();
			/* NOTREACHED */
		default:
			child_count++;
			free(buf);
		}
	}
}

/*
 * Make sure there's some work to do before forking off a child
 */
static int
ckqueue(cap)
	char *cap;
{
	struct dirent *d;
	DIR *dirp;
	char *spooldir;

	if (cgetstr(cap, "sd", &spooldir) == -1)
		spooldir = _PATH_DEFSPOOL;
	if ((dirp = opendir(spooldir)) == NULL)
		return (-1);
	while ((d = readdir(dirp)) != NULL) {
		if (d->d_name[0] != 'c' || d->d_name[1] != 'f')
			continue;	/* daemon control files only */
		closedir(dirp);
		return (1);		/* found something */
	}
	closedir(dirp);
	return (0);
}

#define DUMMY ":nobody::"

/*
 * Check to see if the from host has access to the line printer.
 */
static void
chkhost(f)
	struct sockaddr *f;
{
	struct addrinfo hints, *res, *r;
	FILE *hostf;
	int first = 1, good = 0;
	char host[NI_MAXHOST], ip[NI_MAXHOST];
	char serv[NI_MAXSERV];
	int error;

	error = getnameinfo(f, f->sa_len, NULL, 0, serv, sizeof(serv),
			    NI_NUMERICSERV);
	if (error || atoi(serv) >= IPPORT_RESERVED)
		fatal("Malformed from address");

	/* Need real hostname for temporary filenames */
	error = getnameinfo(f, f->sa_len, host, sizeof(host), NULL, 0,
			    NI_NAMEREQD);
	if (error) {
		error = getnameinfo(f, f->sa_len, host, sizeof(host), NULL, 0,
				    NI_NUMERICHOST);
		if (error)
			fatal("Host name for your address unknown");
		else
			fatal("Host name for your address (%s) unknown", host);
	}

	(void)strncpy(fromb, host, sizeof(fromb) - 1);
	fromb[sizeof(fromb) - 1] = '\0';
	from = fromb;

	/* need address in stringform for comparison (no DNS lookup here) */
	error = getnameinfo(f, f->sa_len, host, sizeof(host), NULL, 0,
			    NI_NUMERICHOST);
	if (error)
		fatal("Cannot print address");

	/* Check for spoof, ala rlogind */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;	/*dummy*/
	error = getaddrinfo(fromb, NULL, &hints, &res);
	if (error) {
		fatal("hostname for your address (%s) unknown: %s", host,
		    gai_strerror(error));
	}
	good = 0;
	for (r = res; good == 0 && r; r = r->ai_next) {
		error = getnameinfo(r->ai_addr, r->ai_addrlen, ip, sizeof(ip),
				    NULL, 0, NI_NUMERICHOST);
		if (!error && !strcmp(host, ip))
			good = 1;
	}
	if (res)
		freeaddrinfo(res);
	if (good == 0)
		fatal("address for your hostname (%s) not matched", host);
	setproctitle("serving %s", from);
	hostf = fopen(_PATH_HOSTSEQUIV, "r");
again:
	if (hostf) {
		if (__ivaliduser_sa(hostf, f, f->sa_len, DUMMY, DUMMY) == 0) {
			(void)fclose(hostf);
			return;
		}
		(void)fclose(hostf);
	}
	if (first == 1) {
		first = 0;
		hostf = fopen(_PATH_HOSTSLPD, "r");
		goto again;
	}
	fatal("Your host does not have line printer access");
	/*NOTREACHED*/
}

static void
usage()
{

	fprintf(stderr, "usage: %s [-dlrs] [-b bind-address] [-n maxchild] "
	    "[-w maxwait] [port]\n", getprogname());
	exit(1);
}

/* setup server socket for specified address family */
/* if af is PF_UNSPEC more than one socket may be returned */
/* the returned list is dynamically allocated, so caller needs to free it */
int *
socksetup(af, options, port)
        int af, options;
	const char *port;
{
	struct addrinfo hints, *res, *r;
	int error, maxs = 0, *s, *socks = NULL, blidx = 0;
	const int on = 1;

	do {
		memset(&hints, 0, sizeof(hints));
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = af;
		hints.ai_socktype = SOCK_STREAM;
		error = getaddrinfo((blist_addrs == 0) ? NULL : blist[blidx],
		    port ? port : "printer", &hints, &res);
		if (error) {
			if (blist_addrs)
				syslog(LOG_ERR, "%s: %s", blist[blidx],
				    gai_strerror(error));
			else
				syslog(LOG_ERR, "%s", gai_strerror(error));
			mcleanup(0);
		}

		/* Count max number of sockets we may open */
		for (r = res; r; r = r->ai_next, maxs++)
			;
		if (socks == NULL) {
			socks = malloc((maxs + 1) * sizeof(int));
			if (socks)
				*socks = 0; /* num of sockets ctr at start */
		} else
			socks = realloc(socks, (maxs + 1) * sizeof(int));
		if (!socks) {
			syslog(LOG_ERR, "couldn't allocate memory for sockets");
			mcleanup(0);
		}

		s = socks + *socks + 1;
		for (r = res; r; r = r->ai_next) {
			*s = socket(r->ai_family, r->ai_socktype,
			            r->ai_protocol);
			if (*s < 0) {
				syslog(LOG_DEBUG, "socket(): %m");
				continue;
			}
			if (options & SO_DEBUG)
				if (setsockopt(*s, SOL_SOCKET, SO_DEBUG,
					       &on, sizeof(on)) < 0) {
					syslog(LOG_ERR,
					       "setsockopt (SO_DEBUG): %m");
					close (*s);
					continue;
				}
			if (bind(*s, r->ai_addr, r->ai_addrlen) < 0) {
				syslog(LOG_DEBUG, "bind(): %m");
				close (*s);
				continue;
			}
			*socks = *socks + 1;
			s++;
		}

		if (res)
			freeaddrinfo(res);
	} while (++blidx < blist_addrs);

	if (socks == NULL || *socks == 0) {
		syslog(LOG_ERR, "Couldn't bind to any socket");
		if (socks != NULL)
			free(socks);
		mcleanup(0);
	}
	return(socks);
}
