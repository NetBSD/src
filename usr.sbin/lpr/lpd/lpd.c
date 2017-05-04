/*	$NetBSD: lpd.c,v 1.58 2017/05/04 16:26:09 sevan Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)lpd.c	8.7 (Berkeley) 5/10/95";
#else
__RCSID("$NetBSD: lpd.c,v 1.58 2017/05/04 16:26:09 sevan Exp $");
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
#include <sys/poll.h>
#include <netinet/in.h>

#include <err.h>
#include <netdb.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#ifdef LIBWRAP
#include <tcpd.h>
#endif

#include "lp.h"
#include "lp.local.h"
#include "pathnames.h"
#include "extern.h"

/* XXX from libc/net/rcmd.c */
extern int __ivaliduser_sa(FILE *, const struct sockaddr *, socklen_t,
			   const char *, const char *);

#ifdef LIBWRAP
int allow_severity = LOG_AUTH|LOG_INFO;
int deny_severity = LOG_AUTH|LOG_WARNING;
#endif

int	lflag;				/* log requests flag */
int	rflag;				/* allow of for remote printers */
int	sflag;				/* secure (no inet) flag */
int	from_remote;			/* from remote socket */
char	**blist;			/* list of addresses to bind(2) to */
int	blist_size;
int	blist_addrs;

static void		reapchild(int);
__dead static void	mcleanup(int);
static void		doit(void);
static void		startup(void);
static void		chkhost(struct sockaddr *, int);
__dead static void	usage(void);
static struct pollfd	*socksetup(int, int, const char *, int *);
static void		chkplushost(int, FILE *, char*);

uid_t	uid, euid;
int child_count;

#define LPD_NOPORTCHK	0001		/* skip reserved-port check */

int
main(int argc, char **argv)
{
	struct sockaddr_storage frm;
	socklen_t frmlen;
	sigset_t nmask, omask;
	int lfd, errs, i, f, nfds;
	struct pollfd *socks;
	int child_max = 32;	/* more than enough to hose the system */
	int options = 0, check_options = 0;
	struct servent *sp;
	const char *port = "printer";
	char **newblist;

	euid = geteuid();	/* these shouldn't be different */
	uid = getuid();
	gethostname(host, sizeof(host));
	host[sizeof(host) - 1] = '\0';
	setprogname(*argv);

	errs = 0;
	while ((i = getopt(argc, argv, "b:dln:srw:W")) != -1)
		switch (i) {
		case 'b':
			if (blist_addrs >= blist_size) {
				newblist = realloc(blist,
				    blist_size + sizeof(char *) * 4);
				if (newblist == NULL)
					err(1, "cant allocate bind addr list");
				blist = newblist;
				blist_size += sizeof(char *) * 4;
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
		case 'W':/* allow connections coming from a non-reserved port */
			 /* (done by some lpr-implementations for MS-Windows) */
			check_options |= LPD_NOPORTCHK;
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
		if (errno == EWOULDBLOCK) {	/* active daemon present */
			syslog(LOG_ERR, "%s is locked; another lpd is running",
			    _PATH_MASTERLOCK);
			exit(0);
		}
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

	sigemptyset(&nmask);
	sigaddset(&nmask, SIGHUP);
	sigaddset(&nmask, SIGINT);
	sigaddset(&nmask, SIGQUIT);
	sigaddset(&nmask, SIGTERM);
	sigprocmask(SIG_BLOCK, &nmask, &omask);

	signal(SIGHUP, mcleanup);
	signal(SIGINT, mcleanup);
	signal(SIGQUIT, mcleanup);
	signal(SIGTERM, mcleanup);

	socks = socksetup(PF_UNSPEC, options, port, &nfds);

	sigprocmask(SIG_SETMASK, &omask, (sigset_t *)0);

	if (blist != NULL) {
		for (i = 0; i < blist_addrs; i++)
			free(blist[i]);
		free(blist);
	}

	/*
	 * Main loop: accept, do a request, continue.
	 */
	memset(&frm, 0, sizeof(frm));
	for (;;) {
		int rv, s;
		/* "short" so it overflows in about 2 hours */
		struct timespec sleeptime = {10, 0};

		while (child_max < child_count) {
			syslog(LOG_WARNING,
			    "too many children, sleeping for %ld seconds",
				(long)sleeptime.tv_sec);
			nanosleep(&sleeptime, NULL);
			sleeptime.tv_sec <<= 1;
			if (sleeptime.tv_sec <= 0) {
				syslog(LOG_CRIT, "sleeptime overflowed! help!");
				sleeptime.tv_sec = 10;
			}
		}

		rv = poll(socks, nfds, INFTIM);
		if (rv <= 0) {
			if (rv < 0 && errno != EINTR)
				syslog(LOG_WARNING, "poll: %m");
			continue;
		}
		s = -1;
                for (i = 0; i < nfds; i++) 
			if (socks[i].revents & POLLIN) {
				frmlen = sizeof(frm);
				s = accept(socks[i].fd,
				    (struct sockaddr *)&frm, &frmlen);
				break;
			}
		if (s < 0) {
			if (errno != EINTR)
				syslog(LOG_WARNING, "accept: %m");
			continue;
		}
		
		switch (fork()) {
		case 0:
			signal(SIGCHLD, SIG_DFL);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
                       	for (i = 0; i < nfds; i++) 
				(void)close(socks[i].fd);
			dup2(s, STDOUT_FILENO);
			(void)close(s);
			if (frm.ss_family != AF_LOCAL) {
				/* for both AF_INET and AF_INET6 */
				from_remote = 1;
				chkhost((struct sockaddr *)&frm, check_options);
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

/*
 * If there was a forward/backward name resolution mismatch, check
 * that there's a '+' entry in fhost.
 */

void
chkplushost(int good, FILE *fhost, char *hst)
{
	int c1, c2, c3;

	if (good) {
		return;
	}

	rewind(fhost);
	while (EOF != (c1 = fgetc(fhost))) {
		if (c1 == '+') {
			c2 = fgetc(fhost);
			if (c2 == ' ' || c2 == '\t' || c2 == '\n') {
				return;
			}
		}
		do {
			c3 = fgetc(fhost);
		} while (c3 != EOF && c3 != '\n');
	}
	fatal("address for your hostname (%s) not matched", hst);
}

static void
reapchild(int signo)
{
	union wait status;

	while (wait3((int *)&status, WNOHANG, 0) > 0)
		child_count--;
}

static void
mcleanup(int signo)
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
const char *cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob"
};

static void
doit(void)
{
	char *cp;
	int n;

	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long");
			if ((n = read(STDOUT_FILENO, cp, 1)) != 1) {
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
			if (*printer == '\0')
				printer = DEFLP;
			printjob();
			break;
		case '\2':	/* receive files to be queued */
			if (!from_remote) {
				syslog(LOG_INFO, "illegal request (%d)", *cp);
				exit(1);
			}
			printer = cp;
			if (*printer == '\0')
				printer = DEFLP;
			recvjob();
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;
			if (*printer == '\0')
				printer = DEFLP;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace((unsigned char)*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit((unsigned char)*cp)) {
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
			if (*printer == '\0')
				printer = DEFLP;
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
				while (isspace((unsigned char)*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit((unsigned char)*cp)) {
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
startup(void)
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

#define DUMMY ":nobody::"

/*
 * Check to see if the from host has access to the line printer.
 */
static void
chkhost(struct sockaddr *f, int check_opts)
{
	struct addrinfo hints, *res, *r;
	FILE *hostf;
	int good = 0;
	char hst[NI_MAXHOST], ip[NI_MAXHOST];
	char serv[NI_MAXSERV];
	int error;
#ifdef LIBWRAP
	struct request_info req;
#endif

	error = getnameinfo(f, f->sa_len, NULL, 0, serv, sizeof(serv),
			    NI_NUMERICSERV);
	if (error)
		fatal("Malformed from address: %s", gai_strerror(error));

         if (!(check_opts & LPD_NOPORTCHK) &&
	       atoi(serv) >= IPPORT_RESERVED)
		fatal("Connect from invalid port (%s)", serv);

	/* Need real hostname for temporary filenames */
	error = getnameinfo(f, f->sa_len, hst, sizeof(hst), NULL, 0,
			    NI_NAMEREQD);
	if (error) {
		error = getnameinfo(f, f->sa_len, hst, sizeof(hst), NULL, 0,
				    NI_NUMERICHOST);
		if (error)
			fatal("Host name for your address unknown");
		else
			fatal("Host name for your address (%s) unknown", hst);
	}

	(void)strlcpy(fromb, hst, sizeof(fromb));
	from = fromb;

	/* need address in stringform for comparison (no DNS lookup here) */
	error = getnameinfo(f, f->sa_len, hst, sizeof(hst), NULL, 0,
			    NI_NUMERICHOST);
	if (error)
		fatal("Cannot print address");

	/* Check for spoof, ala rlogind */
	good = 0;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;	/*dummy*/
	error = getaddrinfo(fromb, NULL, &hints, &res);
	if (!error) {
		for (r = res; good == 0 && r; r = r->ai_next) {
			error = getnameinfo(r->ai_addr, r->ai_addrlen,
				    ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
			if (!error && !strcmp(hst, ip))
				good = 1;
		}
		if (res)
			freeaddrinfo(res);
	}

	/* complain about !good later in chkplushost if needed. */

	setproctitle("serving %s", from);

#ifdef LIBWRAP
	request_init(&req, RQ_DAEMON, "lpd", RQ_CLIENT_SIN, f,
	    RQ_FILE, STDOUT_FILENO, NULL);
	fromhost(&req);
	if (!hosts_access(&req))
		goto denied;
#endif

	hostf = fopen(_PATH_HOSTSEQUIV, "r");
	if (hostf) {
		if (__ivaliduser_sa(hostf, f, f->sa_len, DUMMY, DUMMY) == 0) {
			chkplushost(good, hostf, hst);
			(void)fclose(hostf);
			return;
		}
		(void)fclose(hostf);
	}
	hostf = fopen(_PATH_HOSTSLPD, "r");
	if (hostf) {
		if (__ivaliduser_sa(hostf, f, f->sa_len, DUMMY, DUMMY) == 0) {
			chkplushost(good, hostf, hst);
			(void)fclose(hostf);
			return;
		}
		(void)fclose(hostf);
	}
#ifdef LIBWRAP
  denied:
#endif
	fatal("Your host does not have line printer access");
	/*NOTREACHED*/
}


static void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-dlrsW] [-b bind-address] [-n maxchild] "
	    "[-w maxwait] [port]\n", getprogname());
	exit(1);
}

/* setup server socket for specified address family */
/* if af is PF_UNSPEC more than one socket may be returned */
/* the returned list is dynamically allocated, so caller needs to free it */
struct pollfd *
socksetup(int af, int options, const char *port, int *nfds)
{
	struct sockaddr_un un;
	struct addrinfo hints, *res, *r;
	int error, s, blidx = 0, n;
	struct pollfd *socks, *newsocks;
	const int on = 1;

	*nfds = 0;

	socks = malloc(1 * sizeof(socks[0]));
	if (!socks) {
		syslog(LOG_ERR, "couldn't allocate memory for sockets");
		mcleanup(0);
	}

	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s < 0) {
		syslog(LOG_ERR, "socket(): %m");
		exit(1);
	}
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_LOCAL;
	strncpy(un.sun_path, _PATH_SOCKETNAME, sizeof(un.sun_path) - 1);
	un.sun_len = SUN_LEN(&un);
	(void)umask(07);
	(void)unlink(_PATH_SOCKETNAME);
	if (bind(s, (struct sockaddr *)&un, un.sun_len) < 0) {
		syslog(LOG_ERR, "bind(): %m");
		exit(1);
	}
	(void)umask(0);
	listen(s, 5);
	socks[*nfds].fd = s;
	socks[*nfds].events = POLLIN;
	(*nfds)++;

	if (sflag && !blist_addrs)
		return (socks);

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
		for (r = res, n = 0; r; r = r->ai_next, n++)
			;
		newsocks = realloc(socks, (*nfds + n) * sizeof(socks[0]));
		if (!newsocks) {
			syslog(LOG_ERR, "couldn't allocate memory for sockets");
			mcleanup(0);
		}
		socks = newsocks;

		for (r = res; r; r = r->ai_next) {
			s = socket(r->ai_family, r->ai_socktype,
			    r->ai_protocol);
			if (s < 0) {
				syslog(LOG_DEBUG, "socket(): %m");
				continue;
			}
			if (options & SO_DEBUG)
				if (setsockopt(s, SOL_SOCKET, SO_DEBUG,
					       &on, sizeof(on)) < 0) {
					syslog(LOG_ERR,
					       "setsockopt (SO_DEBUG): %m");
					close(s);
					continue;
				}
			if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on,
			    sizeof(on)) < 0) {
				syslog(LOG_ERR,
				    "setsockopt (SO_REUSEPORT): %m");
				close(s);
				continue;
			}
			if (r->ai_family == AF_INET6 && setsockopt(s,
			    IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
				syslog(LOG_ERR,
				    "setsockopt (IPV6_V6ONLY): %m");
				close(s);
				continue;
			}
			if (bind(s, r->ai_addr, r->ai_addrlen) < 0) {
				syslog(LOG_DEBUG, "bind(): %m");
				close(s);
				continue;
			}
			listen(s, 5);
			socks[*nfds].fd = s;
			socks[*nfds].events = POLLIN;
			(*nfds)++;
		}

		if (res)
			freeaddrinfo(res);
	} while (++blidx < blist_addrs);

	return (socks);
}
