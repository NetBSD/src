/*	$NetBSD: syslogd.c,v 1.59 2003/05/14 23:58:23 itojun Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)syslogd.c	8.3 (Berkeley) 4/4/94";
#else
__RCSID("$NetBSD: syslogd.c,v 1.59 2003/05/14 23:58:23 itojun Exp $");
#endif
#endif /* not lint */

/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as "<n>" as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximimum line length that can be handled.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 * more extensive changes by Eric Allman (again)
 */

#define	MAXLINE		1024		/* maximum line length */
#define	MAXSVLINE	120		/* maximum saved line length */
#define DEFUPRI		(LOG_USER|LOG_NOTICE)
#define DEFSPRI		(LOG_KERN|LOG_CRIT)
#define TIMERINTVL	30		/* interval for checking flush, mark */
#define TTYMSGTIME	1		/* timeout passed to ttymsg */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <locale.h>
#include <netdb.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "utmpentry.h"
#include "pathnames.h"

#define SYSLOG_NAMES
#include <sys/syslog.h>

#ifdef LIBWRAP
#include <tcpd.h>

int allow_severity = LOG_AUTH|LOG_INFO;
int deny_severity = LOG_AUTH|LOG_WARNING;
#endif

char	*ConfFile = _PATH_LOGCONF;
char	ctty[] = _PATH_CONSOLE;

#define FDMASK(fd)	(1 << (fd))

#define	dprintf		if (Debug) printf

#define MAXUNAMES	20	/* maximum number of user names */

/*
 * Flags to logmsg().
 */

#define IGN_CONS	0x001	/* don't print on console */
#define SYNC_FILE	0x002	/* do fsync on file after printing */
#define ADDDATE		0x004	/* add a date to the message */
#define MARK		0x008	/* this message is a mark */

/*
 * This structure represents the files that will have log
 * copies printed.
 */

struct filed {
	struct	filed *f_next;		/* next in linked list */
	short	f_type;			/* entry type, see below */
	short	f_file;			/* file descriptor */
	time_t	f_time;			/* time this was last written */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	union {
		char	f_uname[MAXUNAMES][UT_NAMESIZE+1];
		struct {
			char	f_hname[MAXHOSTNAMELEN+1];
			struct	addrinfo *f_addr;
		} f_forw;		/* forwarding address */
		char	f_fname[MAXPATHLEN];
	} f_un;
	char	f_prevline[MAXSVLINE];		/* last message logged */
	char	f_lasttime[16];			/* time of last occurrence */
	char	f_prevhost[MAXHOSTNAMELEN+1];	/* host from which recd. */
	int	f_prevpri;			/* pri of f_prevline */
	int	f_prevlen;			/* length of f_prevline */
	int	f_prevcount;			/* repetition cnt of prevline */
	int	f_repeatcount;			/* number of "repeated" msgs */
};

/*
 * Intervals at which we flush out "message repeated" messages,
 * in seconds after previous message is logged.  After each flush,
 * we move to the next interval until we reach the largest.
 */
int	repeatinterval[] = { 30, 120, 600 };	/* # of secs before flush */
#define	MAXREPEAT ((sizeof(repeatinterval) / sizeof(repeatinterval[0])) - 1)
#define	REPEATTIME(f)	((f)->f_time + repeatinterval[(f)->f_repeatcount])
#define	BACKOFF(f)	{ if (++(f)->f_repeatcount > MAXREPEAT) \
				 (f)->f_repeatcount = MAXREPEAT; \
			}

/* values for f_type */
#define F_UNUSED	0		/* unused entry */
#define F_FILE		1		/* regular file */
#define F_TTY		2		/* terminal */
#define F_CONSOLE	3		/* console terminal */
#define F_FORW		4		/* remote machine */
#define F_USERS		5		/* list of users */
#define F_WALL		6		/* everyone logged on */

char	*TypeNames[7] = {
	"UNUSED",	"FILE",		"TTY",		"CONSOLE",
	"FORW",		"USERS",	"WALL"
};

struct	filed *Files;
struct	filed consfile;

int	Debug;			/* debug flag */
int	daemonized = 0;		/* we are not daemonized yet */
char	LocalHostName[MAXHOSTNAMELEN+1];	/* our hostname */
char	*LocalDomain;		/* our local domain name */
int	*finet = NULL;		/* Internet datagram sockets */
int	Initialized = 0;	/* set when we have initialized ourselves */
int	MarkInterval = 20 * 60;	/* interval between marks in seconds */
int	MarkSeq = 0;		/* mark sequence number */
int	SecureMode = 0;		/* listen only on unix domain socks */
int	UseNameService = 1;	/* make domain name queries */
int	NumForwards = 0;	/* number of forwarding actions in conf file */
char	**LogPaths;		/* array of pathnames to read messages from */

void	cfline(char *, struct filed *);
char   *cvthname(struct sockaddr_storage *);
int	decode(const char *, CODE *);
void	die(int);
void	domark(int);
void	fprintlog(struct filed *, int, char *);
int	getmsgbufsize(void);
int*	socksetup(int);
void	init(int);
void	logerror(const char *, ...);
void	logmsg(int, char *, char *, int);
void	printline(char *, char *);
void	printsys(char *);
void	reapchild(int);
void	usage(void);
void	wallmsg(struct filed *, struct iovec *);
int	main(int, char *[]);
void	logpath_add(char ***, int *, int *, char *);
void	logpath_fileadd(char ***, int *, int *, char *);

int
main(int argc, char *argv[])
{
	int ch, *funix, i, j, fklog, len, linesize;
	int *nfinetix, nfklogix, nfunixbaseix, nfds;
	int funixsize = 0, funixmaxsize = 0;
	struct sockaddr_un sunx, fromunix;
	struct sockaddr_storage frominet;
	char *p, *line, **pp;
	struct pollfd *readfds;
	uid_t uid = 0;
	gid_t gid = 0;
	char *user = NULL;
	char *group = NULL;
	char *root = "/";
	char *endp;
	struct group   *gr;
	struct passwd  *pw;
	unsigned long l;

	(void)setlocale(LC_ALL, "");

	while ((ch = getopt(argc, argv, "dnsf:m:p:P:u:g:t:")) != -1)
		switch(ch) {
		case 'd':		/* debug */
			Debug++;
			break;
		case 'f':		/* configuration file */
			ConfFile = optarg;
			break;
		case 'g':
			group = optarg;
			if (*group == '\0')
				usage();
			break;
		case 'm':		/* mark interval */
			MarkInterval = atoi(optarg) * 60;
			break;
		case 'n':		/* turn off DNS queries */
			UseNameService = 0;
			break;
		case 'p':		/* path */
			logpath_add(&LogPaths, &funixsize, 
			    &funixmaxsize, optarg);
			break;
		case 'P':		/* file of paths */
			logpath_fileadd(&LogPaths, &funixsize, 
			    &funixmaxsize, optarg);
			break;
		case 's':		/* no network listen mode */
			SecureMode++;
			break;
		case 't':
			root = optarg;
			if (*root == '\0')
				usage();
			break;
		case 'u':
			user = optarg;
			if (*user == '\0')
				usage();
			break;
		case '?':
		default:
			usage();
		}
	if ((argc -= optind) != 0)
		usage();

	setlinebuf(stdout);

	if (user != NULL) {
		if (isdigit((unsigned char)*user)) {
			errno = 0;
			endp = NULL;
			l = strtoul(user, &endp, 0);
			if (errno || *endp != '\0')
	    			goto getuser;
			uid = (uid_t)l;
			if (uid != l) {
				errno = 0;
				logerror("UID out of range");
				die(0);
			}
		} else {
getuser:
			if ((pw = getpwnam(user)) != NULL) {
				uid = pw->pw_uid;
			} else {
				errno = 0;  
				logerror("Cannot find user `%s'", user);
				die (0);
			}
		}
	}

	if (group != NULL) {
		if (isdigit((unsigned char)*group)) {
			errno = 0;
			endp = NULL;
			l = strtoul(group, &endp, 0);
			if (errno || *endp != '\0')
	    			goto getgroup;
			gid = (gid_t)l;
			if (gid != l) {
				errno = 0;
				logerror("GID out of range");
				die(0);
			}
		} else {
getgroup:
			if ((gr = getgrnam(group)) != NULL) {
				gid = gr->gr_gid;
			} else {
				errno = 0;
				logerror("Cannot find group `%s'", group);
				die(0);
			}
		}
	}

	if (access (root, F_OK | R_OK)) {
		logerror("Cannot access `%s'", root);
		die (0);
	}

	consfile.f_type = F_CONSOLE;
	(void)strlcpy(consfile.f_un.f_fname, ctty,
	    sizeof(consfile.f_un.f_fname));
	(void)gethostname(LocalHostName, sizeof(LocalHostName));
	LocalHostName[sizeof(LocalHostName) - 1] = '\0';
	if ((p = strchr(LocalHostName, '.')) != NULL) {
		*p++ = '\0';
		LocalDomain = p;
	} else
		LocalDomain = "";
	linesize = getmsgbufsize();
	if (linesize < MAXLINE)
		linesize = MAXLINE;
	linesize++;
	line = malloc(linesize);
	if (line == NULL) {
		logerror("Couldn't allocate line buffer");
		die(0);
	}
	(void)signal(SIGTERM, die);
	(void)signal(SIGINT, Debug ? die : SIG_IGN);
	(void)signal(SIGQUIT, Debug ? die : SIG_IGN);
	(void)signal(SIGCHLD, reapchild);
	(void)signal(SIGALRM, domark);
	(void)alarm(TIMERINTVL);

#ifndef SUN_LEN
#define SUN_LEN(unp) (strlen((unp)->sun_path) + 2)
#endif
	if (funixsize == 0)
		logpath_add(&LogPaths, &funixsize, 
		    &funixmaxsize, _PATH_LOG);
	funix = (int *)malloc(sizeof(int) * funixsize);
	if (funix == NULL) {
		logerror("Couldn't allocate funix descriptors");
		die(0);
	}
	for (j = 0, pp = LogPaths; *pp; pp++, j++) {
		dprintf("Making unix dgram socket `%s'\n", *pp);
		unlink(*pp);
		memset(&sunx, 0, sizeof(sunx));
		sunx.sun_family = AF_LOCAL;
		(void)strncpy(sunx.sun_path, *pp, sizeof(sunx.sun_path));
		funix[j] = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if (funix[j] < 0 || bind(funix[j],
		    (struct sockaddr *)&sunx, SUN_LEN(&sunx)) < 0 ||
		    chmod(*pp, 0666) < 0) {
			logerror("Cannot create `%s'", *pp);
			die(0);
		}
		dprintf("Listening on unix dgram socket `%s'\n", *pp);
	}

	init(0);

	if ((fklog = open(_PATH_KLOG, O_RDONLY, 0)) < 0) {
		dprintf("Can't open `%s' (%d)\n", _PATH_KLOG, errno);
	} else {
		dprintf("Listening on kernel log `%s'\n", _PATH_KLOG);
	}

	dprintf("Off & running....\n");

	(void)signal(SIGHUP, init);

	/* setup pollfd set. */
	readfds = (struct pollfd *)malloc(sizeof(struct pollfd) *
			(funixsize + (finet ? *finet : 0) + 1));
	if (readfds == NULL) {
		logerror("Couldn't allocate pollfds");
		die(0);
	}
	nfds = 0;
	if (fklog >= 0) {
		nfklogix = nfds++;
		readfds[nfklogix].fd = fklog;
		readfds[nfklogix].events = POLLIN | POLLPRI;
	}
	if (finet && !SecureMode) {
		nfinetix = malloc(*finet * sizeof(*nfinetix));
		for (j = 0; j < *finet; j++) {
			nfinetix[j] = nfds++;
			readfds[nfinetix[j]].fd = finet[j+1];
			readfds[nfinetix[j]].events = POLLIN | POLLPRI;
		}
	}
	nfunixbaseix = nfds;
	for (j = 0, pp = LogPaths; *pp; pp++) {
		readfds[nfds].fd = funix[j++];
		readfds[nfds++].events = POLLIN | POLLPRI;
	}

	/* 
	 * All files are open, we can drop privileges and chroot
	 */
	dprintf("Attempt to chroot to `%s'\n", root);  
	if (chroot(root)) {
		logerror("Failed to chroot to `%s'", root);
		die(0);
	}
	dprintf("Attempt to set GID/EGID to `%d'\n", gid);  
	if (setgid(gid) || setegid(gid)) {
		logerror("Failed to set gid to `%d'", gid);
		die(0);
	}
	dprintf("Attempt to set UID/EUID to `%d'\n", uid);  
	if (setuid(uid) || seteuid(uid)) {
		logerror("Failed to set uid to `%d'", uid);
		die(0);
	}

	/* 
	 * We cannot detach from the terminal before we are sure we won't 
	 * have a fatal error, because error message would not go to the
	 * terminal and would not be logged because syslogd dies. 
	 * All die() calls are behind us, we can call daemon()
	 */
	if (!Debug) {
		(void)daemon(0, 0);
		daemonized = 1;

		/* tuck my process id away, if i'm not in debug mode */
		pidfile(NULL);
	}

	for (;;) {
		int rv;

		rv = poll(readfds, nfds, INFTIM);
		if (rv == 0)
			continue;
		if (rv < 0) {
			if (errno != EINTR)
				logerror("poll() failed");
			continue;
		}
		dprintf("Got a message (%d)\n", rv);
		if (fklog >= 0 &&
		    (readfds[nfklogix].revents & (POLLIN | POLLPRI))) {
			dprintf("Kernel log active\n");
			i = read(fklog, line, linesize - 1);
			if (i > 0) {
				line[i] = '\0';
				printsys(line);
			} else if (i < 0 && errno != EINTR) {
				logerror("klog failed");
				fklog = -1;
			}
		}
		for (j = 0, pp = LogPaths; *pp; pp++, j++) {
			if ((readfds[nfunixbaseix + j].revents &
			    (POLLIN | POLLPRI)) == 0)
				continue;

			dprintf("Unix socket (%s) active\n", *pp);
			len = sizeof(fromunix);
			i = recvfrom(funix[j], line, MAXLINE, 0,
			    (struct sockaddr *)&fromunix, &len);
			if (i > 0) {
				line[i] = '\0';
				printline(LocalHostName, line);
			} else if (i < 0 && errno != EINTR) {
				logerror("recvfrom() unix `%s'", *pp);
			}
		}
		if (finet && !SecureMode) {
			for (j = 0; j < *finet; j++) {
		    		if (readfds[nfinetix[j]].revents &
				    (POLLIN | POLLPRI)) {
#ifdef LIBWRAP
					struct request_info req;
#endif
					int reject = 0;

					dprintf("inet socket active\n");

#ifdef LIBWRAP
					request_init(&req, RQ_DAEMON, "syslogd",
					    RQ_FILE, finet[j + 1], NULL);
					fromhost(&req);
					reject = !hosts_access(&req);
					if (reject)
						dprintf("access denied\n");
#endif

					len = sizeof(frominet);
					i = recvfrom(finet[j+1], line, MAXLINE,
					    0, (struct sockaddr *)&frominet,
					    &len);
					if (i == 0 || (i < 0 && errno == EINTR))
						continue;
					else if (i < 0) {
						logerror("recvfrom inet");
						continue;
					}

					line[i] = '\0';
					if (!reject)
						printline(cvthname(&frominet),
						    line);
				}
			}
		}
	}
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-dns] [-f config_file] [-g group] [-m mark_interval]\n"
	    "\t[-P file_list] [-p log_socket [-p log_socket2 ...]]\n"
	    "\t[-t chroot_dir] [-u user]\n", getprogname());
	exit(1);
}

/*
 * given a pointer to an array of char *'s, a pointer to it's current
 * size and current allocated max size, and a new char * to add, add
 * it, update everything as necessary, possibly allocating a new array
 */
void
logpath_add(char ***lp, int *szp, int *maxszp, char *new)
{

	dprintf("Adding `%s' to the %p logpath list\n", new, *lp);
	if (*szp == *maxszp) {
		if (*maxszp == 0) {
			*maxszp = 4;	/* start of with enough for now */
			*lp = NULL;
		} else
			*maxszp *= 2;
		*lp = realloc(*lp, sizeof(char *) * (*maxszp + 1));
		if (*lp == NULL) {
			logerror("Couldn't allocate line buffer");
			die(0);
		}
	}
	if (((*lp)[(*szp)++] = strdup(new)) == NULL) {
		logerror("Couldn't allocate logpath");
		die(0);
	}
	(*lp)[(*szp)] = NULL;		/* always keep it NULL terminated */
}

/* do a file of log sockets */
void
logpath_fileadd(char ***lp, int *szp, int *maxszp, char *file)
{
	FILE *fp;
	char *line;
	size_t len;

	fp = fopen(file, "r");
	if (fp == NULL) {
		logerror("Could not open socket file list `%s'", file);
		die(0);
	}

	while ((line = fgetln(fp, &len))) {
		line[len - 1] = 0;
		logpath_add(lp, szp, maxszp, line);
	}
	fclose(fp);
}

/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */
void
printline(char *hname, char *msg)
{
	int c, pri;
	char *p, *q, line[MAXLINE + 1];

	/* test for special codes */
	pri = DEFUPRI;
	p = msg;
	if (*p == '<') {
		pri = 0;
		while (isdigit(*++p))
			pri = 10 * pri + (*p - '0');
		if (*p == '>')
			++p;
	}
	if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
		pri = DEFUPRI;

	/* don't allow users to log kernel messages */
	if (LOG_FAC(pri) == LOG_KERN)
		pri = LOG_MAKEPRI(LOG_USER, LOG_PRI(pri));

	q = line;

	while ((c = *p++) != '\0' &&
	    q < &line[sizeof(line) - 2]) {
		c &= 0177;
		if (iscntrl(c))
			if (c == '\n')
				*q++ = ' ';
			else if (c == '\t')
				*q++ = '\t';
			else {
				*q++ = '^';
				*q++ = c ^ 0100;
			}
		else
			*q++ = c;
	}
	*q = '\0';

	logmsg(pri, line, hname, 0);
}

/*
 * Take a raw input line from /dev/klog, split and format similar to syslog().
 */
void
printsys(char *msg)
{
	int c, pri, flags;
	char *lp, *p, *q, line[MAXLINE + 1];

	(void)strlcpy(line, _PATH_UNIX, sizeof(line));
	(void)strlcat(line, ": ", sizeof(line));
	lp = line + strlen(line);
	for (p = msg; *p != '\0'; ) {
		flags = SYNC_FILE | ADDDATE;	/* fsync file after write */
		pri = DEFSPRI;
		if (*p == '<') {
			pri = 0;
			while (isdigit(*++p))
				pri = 10 * pri + (*p - '0');
			if (*p == '>')
				++p;
		} else {
			/* kernel printf's come out on console */
			flags |= IGN_CONS;
		}
		if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
			pri = DEFSPRI;
		q = lp;
		while (*p != '\0' && (c = *p++) != '\n' &&
		    q < &line[MAXLINE])
			*q++ = c;
		*q = '\0';
		logmsg(pri, line, LocalHostName, flags);
	}
}

time_t	now;

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */
void
logmsg(int pri, char *msg, char *from, int flags)
{
	struct filed *f;
	int fac, msglen, omask, prilev;
	char *timestamp;

	dprintf("logmsg: pri 0%o, flags 0x%x, from %s, msg %s\n",
	    pri, flags, from, msg);

	omask = sigblock(sigmask(SIGHUP)|sigmask(SIGALRM));

	/*
	 * Check to see if msg looks non-standard.
	 */
	msglen = strlen(msg);
	if (msglen < 16 || msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ')
		flags |= ADDDATE;

	(void)time(&now);
	if (flags & ADDDATE)
		timestamp = ctime(&now) + 4;
	else {
		timestamp = msg;
		msg += 16;
		msglen -= 16;
	}

	/* extract facility and priority level */
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	else
		fac = LOG_FAC(pri);
	prilev = LOG_PRI(pri);

	/* log the message to the particular outputs */
	if (!Initialized) {
		f = &consfile;
		f->f_file = open(ctty, O_WRONLY, 0);

		if (f->f_file >= 0) {
			fprintlog(f, flags, msg);
			(void)close(f->f_file);
		}
		(void)sigsetmask(omask);
		return;
	}
	for (f = Files; f; f = f->f_next) {
		/* skip messages that are incorrect priority */
		if (f->f_pmask[fac] < prilev ||
		    f->f_pmask[fac] == INTERNAL_NOPRI)
			continue;

		if (f->f_type == F_CONSOLE && (flags & IGN_CONS))
			continue;

		/* don't output marks to recently written files */
		if ((flags & MARK) && (now - f->f_time) < MarkInterval / 2)
			continue;

		/*
		 * suppress duplicate lines to this file
		 */
		if ((flags & MARK) == 0 && msglen == f->f_prevlen &&
		    !strcmp(msg, f->f_prevline) &&
		    !strcmp(from, f->f_prevhost)) {
			(void)strncpy(f->f_lasttime, timestamp, 15);
			f->f_prevcount++;
			dprintf("Msg repeated %d times, %ld sec of %d\n",
			    f->f_prevcount, (long)(now - f->f_time),
			    repeatinterval[f->f_repeatcount]);
			/*
			 * If domark would have logged this by now,
			 * flush it now (so we don't hold isolated messages),
			 * but back off so we'll flush less often
			 * in the future.
			 */
			if (now > REPEATTIME(f)) {
				fprintlog(f, flags, (char *)NULL);
				BACKOFF(f);
			}
		} else {
			/* new line, save it */
			if (f->f_prevcount)
				fprintlog(f, 0, (char *)NULL);
			f->f_repeatcount = 0;
			f->f_prevpri = pri;
			(void)strncpy(f->f_lasttime, timestamp, 15);
			(void)strncpy(f->f_prevhost, from,
					sizeof(f->f_prevhost));
			if (msglen < MAXSVLINE) {
				f->f_prevlen = msglen;
				(void)strlcpy(f->f_prevline, msg,
				    sizeof(f->f_prevline));
				fprintlog(f, flags, (char *)NULL);
			} else {
				f->f_prevline[0] = 0;
				f->f_prevlen = 0;
				fprintlog(f, flags, msg);
			}
		}
	}
	(void)sigsetmask(omask);
}

void
fprintlog(struct filed *f, int flags, char *msg)
{
	struct iovec iov[6];
	struct iovec *v;
	struct addrinfo *r;
	int j, l, lsent;
	char line[MAXLINE + 1], repbuf[80], greetings[200];

	v = iov;
	if (f->f_type == F_WALL) {
		v->iov_base = greetings;
		v->iov_len = snprintf(greetings, sizeof greetings,
		    "\r\n\7Message from syslogd@%s at %.24s ...\r\n",
		    f->f_prevhost, ctime(&now));
		v++;
		v->iov_base = "";
		v->iov_len = 0;
		v++;
	} else {
		v->iov_base = f->f_lasttime;
		v->iov_len = 15;
		v++;
		v->iov_base = " ";
		v->iov_len = 1;
		v++;
	}
	v->iov_base = f->f_prevhost;
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;

	if (msg) {
		v->iov_base = msg;
		v->iov_len = strlen(msg);
	} else if (f->f_prevcount > 1) {
		v->iov_base = repbuf;
		v->iov_len = snprintf(repbuf, sizeof repbuf,
		    "last message repeated %d times", f->f_prevcount);
	} else {
		v->iov_base = f->f_prevline;
		v->iov_len = f->f_prevlen;
	}
	v++;

	dprintf("Logging to %s", TypeNames[f->f_type]);
	f->f_time = now;

	switch (f->f_type) {
	case F_UNUSED:
		dprintf("\n");
		break;

	case F_FORW:
		dprintf(" %s\n", f->f_un.f_forw.f_hname);
			/*
			 * check for local vs remote messages
			 * (from FreeBSD PR#bin/7055)
			 */
		if (strcmp(f->f_prevhost, LocalHostName)) {
			l = snprintf(line, sizeof(line) - 1,
				     "<%d>%.15s [%s]: %s",
				     f->f_prevpri, (char *) iov[0].iov_base,
				     f->f_prevhost, (char *) iov[4].iov_base);
		} else {
			l = snprintf(line, sizeof(line) - 1, "<%d>%.15s %s",
				     f->f_prevpri, (char *) iov[0].iov_base,
				     (char *) iov[4].iov_base);
		}
		if (l > MAXLINE)
			l = MAXLINE;
		if (finet) {
			for (r = f->f_un.f_forw.f_addr; r; r = r->ai_next) {
				for (j = 0; j < *finet; j++) {
#if 0 
					/*
					 * should we check AF first, or just
					 * trial and error? FWD
					 */
					if (r->ai_family ==
					    address_family_of(finet[j+1])) 
#endif
					lsent = sendto(finet[j+1], line, l, 0,
					    r->ai_addr, r->ai_addrlen);
					if (lsent == l) 
						break;
				}
			}
			if (lsent != l) {
				f->f_type = F_UNUSED;
				logerror("sendto() failed");
			}
		}
		break;

	case F_CONSOLE:
		if (flags & IGN_CONS) {
			dprintf(" (ignored)\n");
			break;
		}
		/* FALLTHROUGH */

	case F_TTY:
	case F_FILE:
		dprintf(" %s\n", f->f_un.f_fname);
		if (f->f_type != F_FILE) {
			v->iov_base = "\r\n";
			v->iov_len = 2;
		} else {
			v->iov_base = "\n";
			v->iov_len = 1;
		}
	again:
		if (writev(f->f_file, iov, 6) < 0) {
			int e = errno;
			(void)close(f->f_file);
			/*
			 * Check for errors on TTY's due to loss of tty
			 */
			if ((e == EIO || e == EBADF) && f->f_type != F_FILE) {
				f->f_file = open(f->f_un.f_fname,
				    O_WRONLY|O_APPEND, 0);
				if (f->f_file < 0) {
					f->f_type = F_UNUSED;
					logerror(f->f_un.f_fname);
				} else
					goto again;
			} else {
				f->f_type = F_UNUSED;
				errno = e;
				logerror(f->f_un.f_fname);
			}
		} else if (flags & SYNC_FILE)
			(void)fsync(f->f_file);
		break;

	case F_USERS:
	case F_WALL:
		dprintf("\n");
		v->iov_base = "\r\n";
		v->iov_len = 2;
		wallmsg(f, iov);
		break;
	}
	f->f_prevcount = 0;
}

/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */
void
wallmsg(struct filed *f, struct iovec *iov)
{
	static int reenter;			/* avoid calling ourselves */
	int i;
	char *p;
	static struct utmpentry *ohead = NULL;
	struct utmpentry *ep;

	if (reenter++)
		return;

	(void)getutentries(NULL, &ep);
	if (ep != ohead) {
		freeutentries(ohead);
		ohead = ep;
	}
	/* NOSTRICT */
	for (; ep; ep = ep->next) {
		if (f->f_type == F_WALL) {
			if ((p = ttymsg(iov, 6, ep->line, TTYMSGTIME))
			    != NULL) {
				errno = 0;	/* already in msg */
				logerror(p);
			}
			continue;
		}
		/* should we send the message to this user? */
		for (i = 0; i < MAXUNAMES; i++) {
			if (!f->f_un.f_uname[i][0])
				break;
			if (strcmp(f->f_un.f_uname[i], ep->name) == 0) {
				if ((p = ttymsg(iov, 6, ep->line, TTYMSGTIME))
				    != NULL) {
					errno = 0;	/* already in msg */
					logerror(p);
				}
				break;
			}
		}
	}
	reenter = 0;
}

void
reapchild(int signo)
{
	union wait status;

	while (wait3((int *)&status, WNOHANG, (struct rusage *)NULL) > 0)
		;
}

/*
 * Return a printable representation of a host address.
 */
char *
cvthname(struct sockaddr_storage *f)
{
	int error;
	char *p;
	const int niflag = NI_DGRAM;
	static char host[NI_MAXHOST], ip[NI_MAXHOST];

	error = getnameinfo((struct sockaddr*)f, ((struct sockaddr*)f)->sa_len,
			ip, sizeof ip, NULL, 0, NI_NUMERICHOST|niflag);

	dprintf("cvthname(%s)\n", ip);

	if (error) {
		dprintf("Malformed from address %s\n", gai_strerror(error));
		return ("???");
	}

	if (!UseNameService)
		return (ip);

	error = getnameinfo((struct sockaddr*)f, ((struct sockaddr*)f)->sa_len,
			host, sizeof host, NULL, 0, niflag);
	if (error) {
		dprintf("Host name for your address (%s) unknown\n", ip);
		return (ip);
	}
	if ((p = strchr(host, '.')) && strcmp(p + 1, LocalDomain) == 0)
		*p = '\0';
	return (host);
}

void
domark(int signo)
{
	struct filed *f;

	now = time((time_t *)NULL);
	MarkSeq += TIMERINTVL;
	if (MarkSeq >= MarkInterval) {
		logmsg(LOG_INFO, "-- MARK --", LocalHostName, ADDDATE|MARK);
		MarkSeq = 0;
	}

	for (f = Files; f; f = f->f_next) {
		if (f->f_prevcount && now >= REPEATTIME(f)) {
			dprintf("Flush %s: repeated %d times, %d sec.\n",
			    TypeNames[f->f_type], f->f_prevcount,
			    repeatinterval[f->f_repeatcount]);
			fprintlog(f, 0, (char *)NULL);
			BACKOFF(f);
		}
	}
	(void)alarm(TIMERINTVL);
}

/*
 * Print syslogd errors some place.
 */
void
logerror(const char *fmt, ...)
{
	va_list ap;
	char tmpbuf[BUFSIZ];
	char buf[BUFSIZ];

	va_start(ap, fmt);

	(void)vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, ap);

	va_end(ap);

	if (errno)
		(void)snprintf(buf, sizeof(buf), "syslogd: %s: %s", 
		    tmpbuf, strerror(errno));
	else
		(void)snprintf(buf, sizeof(buf), "syslogd: %s", tmpbuf);

	if (daemonized) 
		logmsg(LOG_SYSLOG|LOG_ERR, buf, LocalHostName, ADDDATE);
	if (!daemonized && Debug)
		dprintf("%s\n", buf);
	if (!daemonized && !Debug)
		printf("%s\n", buf);

	return;
}

void
die(int signo)
{
	struct filed *f;
	char **p;

	for (f = Files; f != NULL; f = f->f_next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);
	}
	errno = 0;
	if (signo)
		logerror("Exiting on signal %d", signo);
	else
		logerror("Fatal error, exiting");
	for (p = LogPaths; p && *p; p++)
		unlink(*p);
	exit(0);
}

/*
 *  INIT -- Initialize syslogd from configuration table
 */
void
init(int signo)
{
	int i;
	FILE *cf;
	struct filed *f, *next, **nextp;
	char *p;
	char cline[LINE_MAX];

	dprintf("init\n");

	/*
	 *  Close all open log files.
	 */
	Initialized = 0;
	for (f = Files; f != NULL; f = next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);

		switch (f->f_type) {
		case F_FILE:
		case F_TTY:
		case F_CONSOLE:
			(void)close(f->f_file);
			break;
		case F_FORW:
			if (f->f_un.f_forw.f_addr)
				freeaddrinfo(f->f_un.f_forw.f_addr);
			break;
		}
		next = f->f_next;
		free((char *)f);
	}
	Files = NULL;
	nextp = &Files;

	/*
	 *  Close all open sockets
	 */

	if (finet) {
		for (i = 0; i < *finet; i++) {
			if (close(finet[i+1]) < 0) {
				logerror("close() failed");
				die(0);
			}
		}
	}

	/*
	 *  Reset counter of forwarding actions
	 */

	NumForwards=0;

	/* open the configuration file */
	if ((cf = fopen(ConfFile, "r")) == NULL) {
		dprintf("Cannot open `%s'\n", ConfFile);
		*nextp = (struct filed *)calloc(1, sizeof(*f));
		cfline("*.ERR\t/dev/console", *nextp);
		(*nextp)->f_next = (struct filed *)calloc(1, sizeof(*f));
		cfline("*.PANIC\t*", (*nextp)->f_next);
		Initialized = 1;
		return;
	}

	/*
	 *  Foreach line in the conf table, open that file.
	 */
	f = NULL;
	while (fgets(cline, sizeof(cline), cf) != NULL) {
		/*
		 * check for end-of-section, comments, strip off trailing
		 * spaces and newline character.
		 */
		for (p = cline; isspace(*p); ++p)
			continue;
		if (*p == '\0' || *p == '#')
			continue;
		for (p = strchr(cline, '\0'); isspace(*--p);)
			continue;
		*++p = '\0';
		f = (struct filed *)calloc(1, sizeof(*f));
		*nextp = f;
		nextp = &f->f_next;
		cfline(cline, f);
	}

	/* close the configuration file */
	(void)fclose(cf);

	Initialized = 1;

	if (Debug) {
		for (f = Files; f; f = f->f_next) {
			for (i = 0; i <= LOG_NFACILITIES; i++)
				if (f->f_pmask[i] == INTERNAL_NOPRI)
					printf("X ");
				else
					printf("%d ", f->f_pmask[i]);
			printf("%s: ", TypeNames[f->f_type]);
			switch (f->f_type) {
			case F_FILE:
			case F_TTY:
			case F_CONSOLE:
				printf("%s", f->f_un.f_fname);
				break;

			case F_FORW:
				printf("%s", f->f_un.f_forw.f_hname);
				break;

			case F_USERS:
				for (i = 0;
				    i < MAXUNAMES && *f->f_un.f_uname[i]; i++)
					printf("%s, ", f->f_un.f_uname[i]);
				break;
			}
			printf("\n");
		}
	}

	finet = socksetup(PF_UNSPEC);
	if (finet) {
		if (SecureMode) {
			for (i = 0; i < *finet; i++) {
				if (shutdown(finet[i+1], SHUT_RD) < 0) {
					logerror("shutdown() failed");
					die(0);
				}
			}
		} else
			dprintf("Listening on inet and/or inet6 socket\n");
		dprintf("Sending on inet and/or inet6 socket\n");
	}

	logmsg(LOG_SYSLOG|LOG_INFO, "syslogd: restart", LocalHostName, ADDDATE);
	dprintf("syslogd: restarted\n");
}

/*
 * Crack a configuration file line
 */
void
cfline(char *line, struct filed *f)
{
	struct addrinfo hints, *res;
	int    error, i, pri;
	char   *bp, *p, *q;
	char   buf[MAXLINE];
	int    sp_err;

	dprintf("cfline(%s)\n", line);

	errno = 0;	/* keep strerror() stuff out of logerror messages */

	/* clear out file entry */
	memset(f, 0, sizeof(*f));
	for (i = 0; i <= LOG_NFACILITIES; i++)
		f->f_pmask[i] = INTERNAL_NOPRI;
	
	/* 
	 * There should not be any space before the log facility.
	 * Check this is okay, complain and fix if it is not.
	 */
	q = line;
	if (isblank((unsigned char)*line)) {
		errno = 0;
		logerror(
		    "Warning: `%s' space or tab before the log facility",
		    line);
		/* Fix: strip all spaces/tabs before the log facility */
		while (*q++ && isblank((unsigned char)*q));
		line = q; 
	}

	/* 
	 * q is now at the first char of the log facility
	 * There should be at least one tab after the log facility 
	 * Check this is okay, and complain and fix if it is not.
	 */
	q = line + strlen(line);
	while (!isblank((unsigned char)*q) && (q != line))
		q--;
	if ((q == line) && strlen(line)) { 
		/* No tabs or space in a non empty line: complain */
		errno = 0;
		logerror(
		    "Error: `%s' log facility or log target missing",
		    line);
	}
	
	/* q is at the end of the blank between the two fields */
	sp_err = 0;
	while (isblank((unsigned char)*q) && (q != line))
		if (*q-- == ' ')
			sp_err = 1;

	if (sp_err) {
		/* 
		 * A space somewhere between the log facility 
		 * and the log target: complain 
		 */
		errno = 0;
		logerror(
		    "Warning: `%s' space found where tab is expected",
		    line);
		/* ... and fix the problem: replace all spaces by tabs */
		while (*++q && isblank((unsigned char)*q))
			if (*q == ' ')
				*q='\t';	
	}	

	/* scan through the list of selectors */
	for (p = line; *p && *p != '\t';) {

		/* find the end of this facility name list */
		for (q = p; *q && *q != '\t' && *q++ != '.'; )
			continue;

		/* collect priority name */
		for (bp = buf; *q && !strchr("\t,;", *q); )
			*bp++ = *q++;
		*bp = '\0';

		/* skip cruft */
		while (strchr(", ;", *q))
			q++;

		/* decode priority name */
		if (*buf == '*')
			pri = LOG_PRIMASK + 1;
		else {
			pri = decode(buf, prioritynames);
			if (pri < 0) {
				errno = 0;
				logerror("Unknown priority name `%s'", buf);
				return;
			}
		}

		/* scan facilities */
		while (*p && !strchr("\t.;", *p)) {
			for (bp = buf; *p && !strchr("\t,;.", *p); )
				*bp++ = *p++;
			*bp = '\0';
			if (*buf == '*')
				for (i = 0; i < LOG_NFACILITIES; i++)
					f->f_pmask[i] = pri;
			else {
				i = decode(buf, facilitynames);
				if (i < 0) {
					errno = 0;
					logerror("Unknown facility name `%s'",
					    buf);
					return;
				}
				f->f_pmask[i >> 3] = pri;
			}
			while (*p == ',' || *p == ' ')
				p++;
		}

		p = q;
	}

	/* skip to action part */
	sp_err = 0;
	while ((*p == '\t') || (*p == ' '))
		p++;

	switch (*p)
	{
	case '@':
		(void)strlcpy(f->f_un.f_forw.f_hname, ++p,
		    sizeof(f->f_un.f_forw.f_hname));
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = 0;
		error = getaddrinfo(f->f_un.f_forw.f_hname, "syslog", &hints,
		    &res);
		if (error) {
			logerror(gai_strerror(error));
			break;
		}
		f->f_un.f_forw.f_addr = res;
		f->f_type = F_FORW;
		NumForwards++;
		break;

	case '/':
		(void)strlcpy(f->f_un.f_fname, p, sizeof(f->f_un.f_fname));
		if ((f->f_file = open(p, O_WRONLY|O_APPEND, 0)) < 0) {
			f->f_type = F_UNUSED;
			logerror(p);
			break;
		}
		if (isatty(f->f_file))
			f->f_type = F_TTY;
		else
			f->f_type = F_FILE;
		if (strcmp(p, ctty) == 0)
			f->f_type = F_CONSOLE;
		break;

	case '*':
		f->f_type = F_WALL;
		break;

	default:
		for (i = 0; i < MAXUNAMES && *p; i++) {
			for (q = p; *q && *q != ','; )
				q++;
			(void)strncpy(f->f_un.f_uname[i], p, UT_NAMESIZE);
			if ((q - p) > UT_NAMESIZE)
				f->f_un.f_uname[i][UT_NAMESIZE] = '\0';
			else
				f->f_un.f_uname[i][q - p] = '\0';
			while (*q == ',' || *q == ' ')
				q++;
			p = q;
		}
		f->f_type = F_USERS;
		break;
	}
}


/*
 *  Decode a symbolic name to a numeric value
 */
int
decode(const char *name, CODE *codetab)
{
	CODE *c;
	char *p, buf[40];

	if (isdigit(*name))
		return (atoi(name));

	for (p = buf; *name && p < &buf[sizeof(buf) - 1]; p++, name++) {
		if (isupper(*name))
			*p = tolower(*name);
		else
			*p = *name;
	}
	*p = '\0';
	for (c = codetab; c->c_name; c++)
		if (!strcmp(buf, c->c_name))
			return (c->c_val);

	return (-1);
}

/*
 * Retrieve the size of the kernel message buffer, via sysctl.
 */
int
getmsgbufsize(void)
{
	int msgbufsize, mib[2];
	size_t size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_MSGBUFSIZE;
	size = sizeof msgbufsize;
	if (sysctl(mib, 2, &msgbufsize, &size, NULL, 0) == -1) {
		dprintf("Couldn't get kern.msgbufsize\n");
		return (0);
	}
	return (msgbufsize);
}

int *
socksetup(int af)
{
	struct addrinfo hints, *res, *r;
	int error, maxs, *s, *socks;
	const int on = 1;

	if(SecureMode && !NumForwards)
		return(NULL);

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = af;
	hints.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(NULL, "syslog", &hints, &res);
	if (error) {
		logerror(gai_strerror(error));
		errno = 0;
		die(0);
	}

	/* Count max number of sockets we may open */
	for (maxs = 0, r = res; r; r = r->ai_next, maxs++)
		continue;
	socks = malloc((maxs+1) * sizeof(int));
	if (!socks) {
		logerror("Couldn't allocate memory for sockets");
		die(0);
	}

	*socks = 0;   /* num of sockets counter at start of array */
	s = socks + 1;
	for (r = res; r; r = r->ai_next) {
		*s = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
		if (*s < 0) {
			logerror("socket() failed");
			continue;
		}
		if (r->ai_family == AF_INET6 && setsockopt(*s, IPPROTO_IPV6,
		    IPV6_V6ONLY, &on, sizeof(on)) < 0) {
			logerror("setsockopt(IPV6_V6ONLY) failed");
			close(*s);
			continue;
		}
		if (!SecureMode && bind(*s, r->ai_addr, r->ai_addrlen) < 0) {
			logerror("bind() failed");
			close (*s);
			continue;
		}

		*socks = *socks + 1;
		s++;
	}

	if (*socks == 0) {
		free (socks);
		if(Debug)
			return(NULL);
		else
			die(0);
	}
	if (res)
		freeaddrinfo(res);

	return(socks);
}
