/*	$NetBSD: syslogd.c,v 1.83 2006/10/21 09:42:26 yamt Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)syslogd.c	8.3 (Berkeley) 4/4/94";
#else
__RCSID("$NetBSD: syslogd.c,v 1.83 2006/10/21 09:42:26 yamt Exp $");
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
 * Extension to log by program name as well as facility and priority
 *   by Peter da Silva.
 * -U and -v by Harlan Stenn.
 * Priority comparison code by Harlan Stenn.
 */

#define	MAXLINE		1024		/* maximum line length */
#define	MAXSVLINE	120		/* maximum saved line length */
#define DEFUPRI		(LOG_USER|LOG_NOTICE)
#define DEFSPRI		(LOG_KERN|LOG_NOTICE)
#define TIMERINTVL	30		/* interval for checking flush, mark */
#define TTYMSGTIME	1		/* timeout passed to ttymsg */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/event.h>

#include <netinet/in.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <locale.h>
#include <netdb.h>
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
#define	ISKERNEL	0x010	/* kernel generated message */

/*
 * This structure represents the files that will have log
 * copies printed.
 * We require f_file to be valid if f_type is F_FILE, F_CONSOLE, F_TTY,
 * or if f_type is F_PIPE and f_pid > 0.
 */

struct filed {
	struct	filed *f_next;		/* next in linked list */
	short	f_type;			/* entry type, see below */
	short	f_file;			/* file descriptor */
	time_t	f_time;			/* time this was last written */
	char	*f_host;		/* host from which to record */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	u_char	f_pcmp[LOG_NFACILITIES+1];	/* compare priority */
#define	PRI_LT	0x1
#define	PRI_EQ	0x2
#define	PRI_GT	0x4
	char	*f_program;		/* program this applies to */
	union {
		char	f_uname[MAXUNAMES][UT_NAMESIZE+1];
		struct {
			char	f_hname[MAXHOSTNAMELEN];
			struct	addrinfo *f_addr;
		} f_forw;		/* forwarding address */
		char	f_fname[MAXPATHLEN];
		struct {
			char	f_pname[MAXPATHLEN];
			pid_t	f_pid;
		} f_pipe;
	} f_un;
	char	f_prevline[MAXSVLINE];		/* last message logged */
	char	f_lasttime[16];			/* time of last occurrence */
	char	f_prevhost[MAXHOSTNAMELEN];	/* host from which recd. */
	int	f_prevpri;			/* pri of f_prevline */
	int	f_prevlen;			/* length of f_prevline */
	int	f_prevcount;			/* repetition cnt of prevline */
	int	f_repeatcount;			/* number of "repeated" msgs */
	int	f_lasterror;			/* last error on writev() */
	int	f_flags;			/* file-specific flags */
#define	FFLAG_SYNC	0x01
};

/*
 * Queue of about-to-be-dead processes we should watch out for.
 */
TAILQ_HEAD(, deadq_entry) deadq_head = TAILQ_HEAD_INITIALIZER(deadq_head);

typedef struct deadq_entry {
	pid_t				dq_pid;
	int				dq_timeout;
	TAILQ_ENTRY(deadq_entry)	dq_entries;
} *dq_t;

/*
 * The timeout to apply to processes waiting on the dead queue.  Unit
 * of measure is "mark intervals", i.e. 20 minutes by default.
 * Processes on the dead queue will be terminated after that time.
 */
#define	DQ_TIMO_INIT	2

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
#define	F_PIPE		7		/* pipe to program */

char	*TypeNames[8] = {
	"UNUSED",	"FILE",		"TTY",		"CONSOLE",
	"FORW",		"USERS",	"WALL",		"PIPE"
};

struct	filed *Files;
struct	filed consfile;

int	Debug;			/* debug flag */
int	daemonized = 0;		/* we are not daemonized yet */
char	LocalHostName[MAXHOSTNAMELEN];	/* our hostname */
char	oldLocalHostName[MAXHOSTNAMELEN];/* previous hostname */
char	*LocalDomain;		/* our local domain name */
size_t	LocalDomainLen;		/* length of LocalDomain */
int	*finet = NULL;		/* Internet datagram sockets */
int	Initialized;		/* set when we have initialized ourselves */
int	ShuttingDown;		/* set when we die() */
int	MarkInterval = 20 * 60;	/* interval between marks in seconds */
int	MarkSeq = 0;		/* mark sequence number */
int	SecureMode = 0;		/* listen only on unix domain socks */
int	UseNameService = 1;	/* make domain name queries */
int	NumForwards = 0;	/* number of forwarding actions in conf file */
char	**LogPaths;		/* array of pathnames to read messages from */
int	NoRepeat = 0;		/* disable "repeated"; log always */
int	RemoteAddDate = 0;	/* always add date to messages from network */
int	SyncKernel = 0;		/* write kernel messages synchronously */
int	UniquePriority = 0;	/* only log specified priority */
int	LogFacPri = 0;		/* put facility and priority in log messages: */
				/* 0=no, 1=numeric, 2=names */

void	cfline(char *, struct filed *, char *, char *);
char   *cvthname(struct sockaddr_storage *);
void	deadq_enter(pid_t, const char *);
int	deadq_remove(pid_t);
int	decode(const char *, CODE *);
void	die(struct kevent *);	/* SIGTERM kevent dispatch routine */
void	domark(struct kevent *);/* timer kevent dispatch routine */
void	fprintlog(struct filed *, int, char *);
int	getmsgbufsize(void);
int*	socksetup(int, const char *);
void	init(struct kevent *);	/* SIGHUP kevent dispatch routine */
void	logerror(const char *, ...);
void	logmsg(int, char *, char *, int);
void	log_deadchild(pid_t, int, const char *);
int	matches_spec(const char *, const char *,
		     char *(*)(const char *, const char *));
void	printline(char *, char *, int);
void	printsys(char *);
int	p_open(char *, pid_t *);
void	trim_localdomain(char *);
void	reapchild(struct kevent *); /* SIGCHLD kevent dispatch routine */
void	usage(void);
void	wallmsg(struct filed *, struct iovec *, size_t);
int	main(int, char *[]);
void	logpath_add(char ***, int *, int *, char *);
void	logpath_fileadd(char ***, int *, int *, char *);

static int fkq;

static struct kevent *allocevchange(void);
static int wait_for_events(struct kevent *, size_t);

static void dispatch_read_klog(struct kevent *);
static void dispatch_read_finet(struct kevent *);
static void dispatch_read_funix(struct kevent *);

/*
 * Global line buffer.  Since we only process one event at a time,
 * a global one will do.
 */
static char *linebuf;
static size_t linebufsize;
static const char *bindhostname = NULL;

#define	A_CNT(x)	(sizeof((x)) / sizeof((x)[0]))

int
main(int argc, char *argv[])
{
	int ch, *funix, j, fklog;
	int funixsize = 0, funixmaxsize = 0;
	struct kevent events[16];
	struct sockaddr_un sunx;
	char **pp;
	struct kevent *ev;
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

	while ((ch = getopt(argc, argv, "b:dnsSf:m:p:P:ru:g:t:TUv")) != -1)
		switch(ch) {
		case 'b':
			bindhostname = optarg;
			break;
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
		case 'r':		/* disable "repeated" compression */
			NoRepeat++;
			break;
		case 's':		/* no network listen mode */
			SecureMode++;
			break;
		case 'S':
			SyncKernel = 1;
			break;
		case 't':
			root = optarg;
			if (*root == '\0')
				usage();
			break;
		case 'T':
			RemoteAddDate = 1;
			break;
		case 'u':
			user = optarg;
			if (*user == '\0')
				usage();
			break;
		case 'U':		/* only log specified priority */
			UniquePriority = 1;
			break;
		case 'v':		/* log facility and priority */
			if (LogFacPri < 2)
				LogFacPri++;
			break;
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
				die(NULL);
			}
		} else {
getuser:
			if ((pw = getpwnam(user)) != NULL) {
				uid = pw->pw_uid;
			} else {
				errno = 0;  
				logerror("Cannot find user `%s'", user);
				die(NULL);
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
				die(NULL);
			}
		} else {
getgroup:
			if ((gr = getgrnam(group)) != NULL) {
				gid = gr->gr_gid;
			} else {
				errno = 0;
				logerror("Cannot find group `%s'", group);
				die(NULL);
			}
		}
	}

	if (access(root, F_OK | R_OK)) {
		logerror("Cannot access `%s'", root);
		die(NULL);
	}

	consfile.f_type = F_CONSOLE;
	(void)strlcpy(consfile.f_un.f_fname, ctty,
	    sizeof(consfile.f_un.f_fname));
	linebufsize = getmsgbufsize();
	if (linebufsize < MAXLINE)
		linebufsize = MAXLINE;
	linebufsize++;
	linebuf = malloc(linebufsize);
	if (linebuf == NULL) {
		logerror("Couldn't allocate line buffer");
		die(NULL);
	}

#ifndef SUN_LEN
#define SUN_LEN(unp) (strlen((unp)->sun_path) + 2)
#endif
	if (funixsize == 0)
		logpath_add(&LogPaths, &funixsize, 
		    &funixmaxsize, _PATH_LOG);
	funix = (int *)malloc(sizeof(int) * funixsize);
	if (funix == NULL) {
		logerror("Couldn't allocate funix descriptors");
		die(NULL);
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
			die(NULL);
		}
		dprintf("Listening on unix dgram socket `%s'\n", *pp);
	}

	if ((fklog = open(_PATH_KLOG, O_RDONLY, 0)) < 0) {
		dprintf("Can't open `%s' (%d)\n", _PATH_KLOG, errno);
	} else {
		dprintf("Listening on kernel log `%s'\n", _PATH_KLOG);
	}

	/* 
	 * All files are open, we can drop privileges and chroot
	 */
	dprintf("Attempt to chroot to `%s'\n", root);  
	if (chroot(root)) {
		logerror("Failed to chroot to `%s'", root);
		die(NULL);
	}
	dprintf("Attempt to set GID/EGID to `%d'\n", gid);  
	if (setgid(gid) || setegid(gid)) {
		logerror("Failed to set gid to `%d'", gid);
		die(NULL);
	}
	dprintf("Attempt to set UID/EUID to `%d'\n", uid);  
	if (setuid(uid) || seteuid(uid)) {
		logerror("Failed to set uid to `%d'", uid);
		die(NULL);
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

	/*
	 * Create the global kernel event descriptor.
	 *
	 * NOTE: We MUST do this after daemon(), bacause the kqueue()
	 * API dictates that kqueue descriptors are not inherited
	 * across forks (lame!).
	 */
	if ((fkq = kqueue()) < 0) {
		logerror("Cannot create event queue");
		die(NULL);	/* XXX This error is lost! */
	}

	/*
	 * We must read the configuration file for the first time
	 * after the kqueue descriptor is created, because we install
	 * events during this process.
	 */
	init(NULL);

	/*
	 * Always exit on SIGTERM.  Also exit on SIGINT and SIGQUIT
	 * if we're debugging.
	 */
	(void)signal(SIGTERM, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	ev = allocevchange();
	EV_SET(ev, SIGTERM, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0,
	    (intptr_t) die);
	if (Debug) {
		ev = allocevchange();
		EV_SET(ev, SIGINT, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0,
		    (intptr_t) die);

		ev = allocevchange();
		EV_SET(ev, SIGQUIT, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0,
		    (intptr_t) die);
	}

	ev = allocevchange();
	EV_SET(ev, SIGCHLD, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0,
	    (intptr_t) reapchild);

	ev = allocevchange();
	EV_SET(ev, 0, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0,
	    TIMERINTVL * 1000 /* seconds -> ms */, (intptr_t) domark);

	(void)signal(SIGPIPE, SIG_IGN);	/* We'll catch EPIPE instead. */

	/* Re-read configuration on SIGHUP. */
	(void) signal(SIGHUP, SIG_IGN);
	ev = allocevchange();
	EV_SET(ev, SIGHUP, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0,
	    (intptr_t) init);

	if (fklog >= 0) {
		ev = allocevchange();
		EV_SET(ev, fklog, EVFILT_READ, EV_ADD | EV_ENABLE,
		    0, 0, (intptr_t) dispatch_read_klog);
	}
	for (j = 0, pp = LogPaths; *pp; pp++, j++) {
		ev = allocevchange();
		EV_SET(ev, funix[j], EVFILT_READ, EV_ADD | EV_ENABLE,
		    0, 0, (intptr_t) dispatch_read_funix);
	}

	dprintf("Off & running....\n");

	for (;;) {
		void (*handler)(struct kevent *);
		int i, rv;

		rv = wait_for_events(events, A_CNT(events));
		if (rv == 0)
			continue;
		if (rv < 0) {
			if (errno != EINTR)
				logerror("kevent() failed");
			continue;
		}
		dprintf("Got an event (%d)\n", rv);
		for (i = 0; i < rv; i++) {
			handler = (void *) events[i].udata;
			(*handler)(&events[i]);
		}
	}
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-dnrSsTUv] [-b bind_address] [-f config_file] [-g group]\n"
	    "\t[-m mark_interval] [-P file_list] [-p log_socket\n"
	    "\t[-p log_socket2 ...]] [-t chroot_dir] [-u user]\n",
	    getprogname());
	exit(1);
}

/*
 * Dispatch routine for reading /dev/klog
 */
static void
dispatch_read_klog(struct kevent *ev)
{
	ssize_t rv;
	int fd = ev->ident;

	dprintf("Kernel log active\n");

	rv = read(fd, linebuf, linebufsize - 1);
	if (rv > 0) {
		linebuf[rv] = '\0';
		printsys(linebuf);
	} else if (rv < 0 && errno != EINTR) {
		/*
		 * /dev/klog has croaked.  Disable the event
		 * so it won't bother us again.
		 */
		struct kevent *cev = allocevchange();
		logerror("klog failed");
		EV_SET(cev, fd, EVFILT_READ, EV_DISABLE,
		    0, 0, (intptr_t) dispatch_read_klog);
	}
}

/*
 * Dispatch routine for reading Unix domain sockets.
 */
static void
dispatch_read_funix(struct kevent *ev)
{
	struct sockaddr_un myname, fromunix;
	ssize_t rv;
	socklen_t sunlen;
	int fd = ev->ident;

	sunlen = sizeof(myname);
	if (getsockname(fd, (struct sockaddr *)&myname, &sunlen) != 0) {
		/*
		 * This should never happen, so ensure that it doesn't
		 * happen again.
		 */
		struct kevent *cev = allocevchange();
		logerror("getsockname() unix failed");
		EV_SET(cev, fd, EVFILT_READ, EV_DISABLE,
		    0, 0, (intptr_t) dispatch_read_funix);
		return;
	}

	dprintf("Unix socket (%s) active\n", myname.sun_path);

	sunlen = sizeof(fromunix);
	rv = recvfrom(fd, linebuf, MAXLINE, 0,
	    (struct sockaddr *)&fromunix, &sunlen);
	if (rv > 0) {
		linebuf[rv] = '\0';
		printline(LocalHostName, linebuf, 0);
	} else if (rv < 0 && errno != EINTR) {
		logerror("recvfrom() unix `%s'", myname.sun_path);
	}
}

/*
 * Dispatch routine for reading Internet sockets.
 */
static void
dispatch_read_finet(struct kevent *ev)
{
#ifdef LIBWRAP
	struct request_info req;
#endif
	struct sockaddr_storage frominet;
	ssize_t rv;
	socklen_t len;
	int fd = ev->ident;
	int reject = 0;

	dprintf("inet socket active\n");

#ifdef LIBWRAP
	request_init(&req, RQ_DAEMON, "syslogd", RQ_FILE, fd, NULL);
	fromhost(&req);
	reject = !hosts_access(&req);
	if (reject)
		dprintf("access denied\n");
#endif

	len = sizeof(frominet);
	rv = recvfrom(fd, linebuf, MAXLINE, 0,
	    (struct sockaddr *)&frominet, &len);
	if (rv == 0 || (rv < 0 && errno == EINTR))
		return;
	else if (rv < 0) {
		logerror("recvfrom inet");
		return;
	}

	linebuf[rv] = '\0';
	if (!reject)
		printline(cvthname(&frominet), linebuf,
			  RemoteAddDate ? ADDDATE : 0);
}

/*
 * given a pointer to an array of char *'s, a pointer to its current
 * size and current allocated max size, and a new char * to add, add
 * it, update everything as necessary, possibly allocating a new array
 */
void
logpath_add(char ***lp, int *szp, int *maxszp, char *new)
{
	char **nlp;
	int newmaxsz;

	dprintf("Adding `%s' to the %p logpath list\n", new, *lp);
	if (*szp == *maxszp) {
		if (*maxszp == 0) {
			newmaxsz = 4;	/* start of with enough for now */
			*lp = NULL;
		} else
			newmaxsz = *maxszp * 2;
		nlp = realloc(*lp, sizeof(char *) * (newmaxsz + 1));
		if (nlp == NULL) {
			logerror("Couldn't allocate line buffer");
			die(NULL);
		}
		*lp = nlp;
		*maxszp = newmaxsz;
	}
	if (((*lp)[(*szp)++] = strdup(new)) == NULL) {
		logerror("Couldn't allocate logpath");
		die(NULL);
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
		die(NULL);
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
printline(char *hname, char *msg, int flags)
{
	int c, pri;
	char *p, *q, line[MAXLINE + 1];
	long n;

	/* test for special codes */
	pri = DEFUPRI;
	p = msg;
	if (*p == '<') {
		errno = 0;
		n = strtol(p + 1, &q, 10);
		if (*q == '>' && n >= 0 && n < INT_MAX && errno == 0) {
			p = q + 1;
			pri = (int)n;
		}
	}
	if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
		pri = DEFUPRI;

	/*
	 * Don't allow users to log kernel messages.
	 * NOTE: Since LOG_KERN == 0, this will also match
	 *	 messages with no facility specified.
	 */
	if ((pri & LOG_FACMASK) == LOG_KERN)
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

	logmsg(pri, line, hname, flags);
}

/*
 * Take a raw input line from /dev/klog, split and format similar to syslog().
 */
void
printsys(char *msg)
{
	int n, pri, flags, is_printf;
	char *p, *q;

	for (p = msg; *p != '\0'; ) {
		flags = ISKERNEL | ADDDATE;
		if (SyncKernel)
			flags |= SYNC_FILE;
		pri = DEFSPRI;
		is_printf = 1;
		if (*p == '<') {
			errno = 0;
			n = (int)strtol(p + 1, &q, 10);
			if (*q == '>' && n >= 0 && n < INT_MAX && errno == 0) {
				p = q + 1;
				pri = n;
				is_printf = 0;
			}
		}
		if (is_printf) {
			/* kernel printf's come out on console */
			flags |= IGN_CONS;
		}
		if (pri &~ (LOG_FACMASK|LOG_PRIMASK))
			pri = DEFSPRI;
		for (q = p; *q != '\0' && *q != '\n'; q++)
			/* look for end of line */;
		if (*q != '\0')
			*q++ = '\0';
		logmsg(pri, p, LocalHostName, flags);
		p = q;
	}
}

time_t	now;

/*
 * Check to see if `name' matches the provided specification, using the
 * specified strstr function.
 */
int
matches_spec(const char *name, const char *spec,
    char *(*check)(const char *, const char *))
{
	const char *s;
	const char *cursor;
	char prev, next;

	if (strchr(name, ',')) /* sanity */
		return (0);

	cursor = spec;
	while ((s = (*check)(cursor, name)) != NULL) {
		prev = s == spec ? ',' : *(s - 1);
		cursor = s + strlen(name);
		next = *cursor;

		if (prev == ',' && (next == '\0' || next == ','))
			return (1);
	}

	return (0);
}

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */
void
logmsg(int pri, char *msg, char *from, int flags)
{
	struct filed *f;
	int fac, msglen, omask, prilev, i;
	char *timestamp;
	char prog[NAME_MAX + 1];
	char buf[MAXLINE + 1];

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

	/* skip leading whitespace */
	while (isspace((unsigned char)*msg)) {
		msg++;
		msglen--;
	}

	/* extract facility and priority level */
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	else
		fac = LOG_FAC(pri);
	prilev = LOG_PRI(pri);

	/* extract program name */
	for (i = 0; i < NAME_MAX; i++) {
		if (!isprint((unsigned char)msg[i]) ||
		    msg[i] == ':' || msg[i] == '[')
			break;
		prog[i] = msg[i];
	}
	prog[i] = '\0';

	/* add kernel prefix for kernel messages */
	if (flags & ISKERNEL) {
		snprintf(buf, sizeof(buf), "%s: %s",
		    _PATH_UNIX, msg);
		msg = buf;
		msglen = strlen(buf);
	}

	/* log the message to the particular outputs */
	if (!Initialized) {
		f = &consfile;
		f->f_file = open(ctty, O_WRONLY, 0);

		if (f->f_file >= 0) {
			(void)strncpy(f->f_lasttime, timestamp, 15);
			fprintlog(f, flags, msg);
			(void)close(f->f_file);
		}
		(void)sigsetmask(omask);
		return;
	}
	for (f = Files; f; f = f->f_next) {
		/* skip messages that are incorrect priority */
		if (!(((f->f_pcmp[fac] & PRI_EQ) && (f->f_pmask[fac] == prilev))
		     ||((f->f_pcmp[fac] & PRI_LT) && (f->f_pmask[fac] < prilev))
		     ||((f->f_pcmp[fac] & PRI_GT) && (f->f_pmask[fac] > prilev))
		     )
		    || f->f_pmask[fac] == INTERNAL_NOPRI)
			continue;

		/* skip messages with the incorrect host name */
		if (f->f_host != NULL) {
			switch (f->f_host[0]) {
			case '+':
				if (! matches_spec(from, f->f_host + 1,
						   strcasestr))
					continue;
				break;
			case '-':
				if (matches_spec(from, f->f_host + 1,
						 strcasestr))
					continue;
				break;
			}
		}

		/* skip messages with the incorrect program name */
		if (f->f_program != NULL) {
			switch (f->f_program[0]) {
			case '+':
				if (! matches_spec(prog, f->f_program + 1,
						   strstr))
					continue;
				break;
			case '-':
				if (matches_spec(prog, f->f_program + 1,
						 strstr))
					continue;
				break;
			default:
				if (! matches_spec(prog, f->f_program,
						   strstr))
					continue;
				break;
			}
		}

		if (f->f_type == F_CONSOLE && (flags & IGN_CONS))
			continue;

		/* don't output marks to recently written files */
		if ((flags & MARK) && (now - f->f_time) < MarkInterval / 2)
			continue;

		/*
		 * suppress duplicate lines to this file unless NoRepeat
		 */
		if ((flags & MARK) == 0 && msglen == f->f_prevlen &&
		    !NoRepeat &&
		    !strcmp(msg, f->f_prevline) &&
		    !strcasecmp(from, f->f_prevhost)) {
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
	struct iovec iov[10];
	struct iovec *v;
	struct addrinfo *r;
	int j, l, lsent, fail, retry;
	char line[MAXLINE + 1], repbuf[80], greetings[200];
#define ADDEV() assert(++v - iov < A_CNT(iov))

	v = iov;
	if (f->f_type == F_WALL) {
		v->iov_base = greetings;
		v->iov_len = snprintf(greetings, sizeof greetings,
		    "\r\n\7Message from syslogd@%s at %.24s ...\r\n",
		    f->f_prevhost, ctime(&now));
		ADDEV();
		v->iov_base = "";
		v->iov_len = 0;
		ADDEV();
	} else {
		v->iov_base = f->f_lasttime;
		v->iov_len = 15;
		ADDEV();
		v->iov_base = " ";
		v->iov_len = 1;
		ADDEV();
	}

	if (LogFacPri) {
		static char fp_buf[30];
		const char *f_s = NULL, *p_s = NULL;
		int fac = f->f_prevpri & LOG_FACMASK;
		int pri = LOG_PRI(f->f_prevpri);
		char f_n[5], p_n[5];

		if (LogFacPri > 1) {
			CODE *c;

			for (c = facilitynames; c->c_name != NULL; c++) {
				if (c->c_val == fac) {
					f_s = c->c_name;
					break;
				}
			}
			for (c = prioritynames; c->c_name != NULL; c++) {
				if (c->c_val == pri) {
					p_s = c->c_name;
					break;
				}
			}
		}
		if (f_s == NULL) {
			snprintf(f_n, sizeof(f_n), "%d", LOG_FAC(fac));
			f_s = f_n;
		}
		if (p_s == NULL) {
			snprintf(p_n, sizeof(p_n), "%d", pri);
			p_s = p_n;
		}
		snprintf(fp_buf, sizeof(fp_buf), "<%s.%s>", f_s, p_s);
		v->iov_base = fp_buf;
		v->iov_len = strlen(fp_buf);
	} else {
		v->iov_base = "";
		v->iov_len = 0;
	}
	ADDEV();

	v->iov_base = f->f_prevhost;
	v->iov_len = strlen(v->iov_base);
	ADDEV();
	v->iov_base = " ";
	v->iov_len = 1;
	ADDEV();

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
	ADDEV();

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
		if (strcasecmp(f->f_prevhost, LocalHostName)) {
			l = snprintf(line, sizeof(line) - 1,
				     "<%d>%.15s [%s]: %s",
				     f->f_prevpri, (char *) iov[0].iov_base,
				     f->f_prevhost, (char *) iov[5].iov_base);
		} else {
			l = snprintf(line, sizeof(line) - 1, "<%d>%.15s %s",
				     f->f_prevpri, (char *) iov[0].iov_base,
				     (char *) iov[5].iov_base);
		}
		if (l > MAXLINE)
			l = MAXLINE;
		if (finet) {
			lsent = -1;
			fail = 0;
			for (r = f->f_un.f_forw.f_addr; r; r = r->ai_next) {
				retry = 0;
				for (j = 0; j < *finet; j++) {
#if 0 
					/*
					 * should we check AF first, or just
					 * trial and error? FWD
					 */
					if (r->ai_family ==
					    address_family_of(finet[j+1])) 
#endif
sendagain:
					lsent = sendto(finet[j+1], line, l, 0,
					    r->ai_addr, r->ai_addrlen);
					if (lsent == -1) {
						switch (errno) {
						case ENOBUFS:
							/* wait/retry/drop */
							if (++retry < 5) {
								usleep(1000);
								goto sendagain;
							}
							break;
						case EHOSTDOWN:
						case EHOSTUNREACH:
						case ENETDOWN:
							/* drop */
							break;
						default:
							/* busted */
							fail++;
							break;
						}
					} else if (lsent == l) 
						break;
				}
			}
			if (lsent != l && fail) {
				f->f_type = F_UNUSED;
				logerror("sendto() failed");
			}
		}
		break;

	case F_PIPE:
		dprintf(" %s\n", f->f_un.f_pipe.f_pname);
		v->iov_base = "\n";
		v->iov_len = 1;
		ADDEV();
		if (f->f_un.f_pipe.f_pid == 0) {
			if ((f->f_file = p_open(f->f_un.f_pipe.f_pname,
						&f->f_un.f_pipe.f_pid)) < 0) {
				f->f_type = F_UNUSED;
				logerror(f->f_un.f_pipe.f_pname);
				break;
			}
		}
		if (writev(f->f_file, iov, v - iov) < 0) {
			int e = errno;
			if (f->f_un.f_pipe.f_pid > 0) {
				(void) close(f->f_file);
				deadq_enter(f->f_un.f_pipe.f_pid,
					    f->f_un.f_pipe.f_pname);
			}
			f->f_un.f_pipe.f_pid = 0;
			/*
			 * If the error was EPIPE, then what is likely
			 * has happened is we have a command that is
			 * designed to take a single message line and
			 * then exit, but we tried to feed it another
			 * one before we reaped the child and thus
			 * reset our state.
			 *
			 * Well, now we've reset our state, so try opening
			 * the pipe and sending the message again if EPIPE
			 * was the error.
			 */
			if (e == EPIPE) {
				if ((f->f_file = p_open(f->f_un.f_pipe.f_pname,
				     &f->f_un.f_pipe.f_pid)) < 0) {
					f->f_type = F_UNUSED;
					logerror(f->f_un.f_pipe.f_pname);
					break;
				}
				if (writev(f->f_file, iov, v - iov) < 0) {
					e = errno;
					if (f->f_un.f_pipe.f_pid > 0) {
					    (void) close(f->f_file);
					    deadq_enter(f->f_un.f_pipe.f_pid,
							f->f_un.f_pipe.f_pname);
					}
					f->f_un.f_pipe.f_pid = 0;
				} else
					e = 0;
			}
			if (e != 0) {
				errno = e;
				logerror(f->f_un.f_pipe.f_pname);
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
		ADDEV();
	again:
		if (writev(f->f_file, iov, v - iov) < 0) {
			int e = errno;
			if (f->f_type == F_FILE && e == ENOSPC) {
				int lasterror = f->f_lasterror;
				f->f_lasterror = e;
				if (lasterror != e)
					logerror(f->f_un.f_fname);
				break;
			}
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
				f->f_lasterror = e;
				logerror(f->f_un.f_fname);
			}
		} else {
			f->f_lasterror = 0;
			if ((flags & SYNC_FILE) && (f->f_flags & FFLAG_SYNC))
				(void)fsync(f->f_file);
		}
		break;

	case F_USERS:
	case F_WALL:
		dprintf("\n");
		v->iov_base = "\r\n";
		v->iov_len = 2;
		ADDEV();
		wallmsg(f, iov, v - iov);
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
wallmsg(struct filed *f, struct iovec *iov, size_t iovcnt)
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
			if ((p = ttymsg(iov, iovcnt, ep->line, TTYMSGTIME))
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
				if ((p = ttymsg(iov, iovcnt, ep->line,
				    TTYMSGTIME)) != NULL) {
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
reapchild(struct kevent *ev)
{
	int status;
	pid_t pid;
	struct filed *f;

	while ((pid = wait3(&status, WNOHANG, NULL)) > 0) {
		if (!Initialized || ShuttingDown) {
			/*
			 * Be silent while we are initializing or
			 * shutting down.
			 */
			continue;
		}

		if (deadq_remove(pid))
			continue;

		/* Now, look in the list of active processes. */
		for (f = Files; f != NULL; f = f->f_next) {
			if (f->f_type == F_PIPE &&
			    f->f_un.f_pipe.f_pid == pid) {
				(void) close(f->f_file);
				f->f_un.f_pipe.f_pid = 0;
				log_deadchild(pid, status,
					      f->f_un.f_pipe.f_pname);
				break;
			}
		}
	}
}

/*
 * Return a printable representation of a host address.
 */
char *
cvthname(struct sockaddr_storage *f)
{
	int error;
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

	trim_localdomain(host);

	return (host);
}

void
trim_localdomain(char *host)
{
	size_t hl;

	hl = strlen(host);
	if (hl > 0 && host[hl - 1] == '.')
		host[--hl] = '\0';

	if (hl > LocalDomainLen && host[hl - LocalDomainLen - 1] == '.' &&
	    strcasecmp(&host[hl - LocalDomainLen], LocalDomain) == 0)
		host[hl - LocalDomainLen - 1] = '\0';
}

void
domark(struct kevent *ev)
{
	struct filed *f;
	dq_t q, nextq;

	/*
	 * XXX Should we bother to adjust for the # of times the timer
	 * has expired (i.e. in case we miss one?).  This information is
	 * returned to us in ev->data.
	 */

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

	/* Walk the dead queue, and see if we should signal somebody. */
	for (q = TAILQ_FIRST(&deadq_head); q != NULL; q = nextq) {
		nextq = TAILQ_NEXT(q, dq_entries);
		switch (q->dq_timeout) {
		case 0:
			/* Already signalled once, try harder now. */
			if (kill(q->dq_pid, SIGKILL) != 0)
				(void) deadq_remove(q->dq_pid);
			break;

		case 1:
			/*
			 * Timed out on the dead queue, send terminate
			 * signal.  Note that we leave the removal from
			 * the dead queue to reapchild(), which will
			 * also log the event (unless the process
			 * didn't even really exist, in case we simply
			 * drop it from the dead queue).
			 */
			if (kill(q->dq_pid, SIGTERM) != 0) {
				(void) deadq_remove(q->dq_pid);
				break;
			}
			/* FALLTHROUGH */

		default:
			q->dq_timeout--;
		}
	}
}

/*
 * Print syslogd errors some place.
 */
void
logerror(const char *fmt, ...)
{
	static int logerror_running;
	va_list ap;
	char tmpbuf[BUFSIZ];
	char buf[BUFSIZ];

	/* If there's an error while trying to log an error, give up. */
	if (logerror_running)
		return;
	logerror_running = 1;

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

	logerror_running = 0;
}

void
die(struct kevent *ev)
{
	struct filed *f;
	char **p;

	ShuttingDown = 1;	/* Don't log SIGCHLDs. */
	for (f = Files; f != NULL; f = f->f_next) {
		/* flush any pending output */
		if (f->f_prevcount)
			fprintlog(f, 0, (char *)NULL);
		if (f->f_type == F_PIPE && f->f_un.f_pipe.f_pid > 0) {
			(void) close(f->f_file);
			f->f_un.f_pipe.f_pid = 0;
		}
	}
	errno = 0;
	if (ev != NULL)
		logerror("Exiting on signal %d", (int) ev->ident);
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
init(struct kevent *ev)
{
	size_t i;
	FILE *cf;
	struct filed *f, *next, **nextp;
	char *p;
	char cline[LINE_MAX];
	char prog[NAME_MAX + 1];
	char host[MAXHOSTNAMELEN];
	char hostMsg[2*MAXHOSTNAMELEN + 40];

	dprintf("init\n");

	(void)strlcpy(oldLocalHostName, LocalHostName,
		      sizeof(oldLocalHostName));
	(void)gethostname(LocalHostName, sizeof(LocalHostName));
	if ((p = strchr(LocalHostName, '.')) != NULL) {
		*p++ = '\0';
		LocalDomain = p;
	} else
		LocalDomain = "";
	LocalDomainLen = strlen(LocalDomain);

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
		case F_PIPE:
			if (f->f_un.f_pipe.f_pid > 0) {
				(void)close(f->f_file);
				deadq_enter(f->f_un.f_pipe.f_pid,
					    f->f_un.f_pipe.f_pname);
			}
			f->f_un.f_pipe.f_pid = 0;
			break;
		case F_FORW:
			if (f->f_un.f_forw.f_addr)
				freeaddrinfo(f->f_un.f_forw.f_addr);
			break;
		}
		next = f->f_next;
		if (f->f_program != NULL)
			free(f->f_program);
		if (f->f_host != NULL)
			free(f->f_host);
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
				die(NULL);
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
		cfline("*.ERR\t/dev/console", *nextp, "*", "*");
		(*nextp)->f_next = (struct filed *)calloc(1, sizeof(*f));
		cfline("*.PANIC\t*", (*nextp)->f_next, "*", "*");
		Initialized = 1;
		return;
	}

	/*
	 *  Foreach line in the conf table, open that file.
	 */
	f = NULL;
	strcpy(prog, "*");
	strcpy(host, "*");
	while (fgets(cline, sizeof(cline), cf) != NULL) {
		/*
		 * check for end-of-section, comments, strip off trailing
		 * spaces and newline character.  #!prog is treated specially:
		 * following lines apply only to that program.
		 */
		for (p = cline; isspace((unsigned char)*p); ++p)
			continue;
		if (*p == '\0')
			continue;
		if (*p == '#') {
			p++;
			if (*p != '!' && *p != '+' && *p != '-')
				continue;
		}
		if (*p == '+' || *p == '-') {
			host[0] = *p++;
			while (isspace((unsigned char)*p))
				p++;
			if (*p == '\0' || *p == '*') {
				strcpy(host, "*");
				continue;
			}
			for (i = 1; i < MAXHOSTNAMELEN - 1; i++) {
				if (*p == '@') {
					(void)strncpy(&host[i], LocalHostName,
					    sizeof(host) - 1 - i);
					host[sizeof(host) - 1] = '\0';
					i = strlen(host) - 1;
					p++;
					continue;
				}
				if (!isalnum((unsigned char)*p) &&
				    *p != '.' && *p != '-' && *p != ',')
					break;
				host[i] = *p++;
			}
			host[i] = '\0';
			continue;
		}
		if (*p == '!') {
			p++;
			while (isspace((unsigned char)*p))
				p++;
			if (*p == '\0' || *p == '*') {
				strcpy(prog, "*");
				continue;
			}
			for (i = 0; i < NAME_MAX; i++) {
				if (!isprint((unsigned char)p[i]))
					break;
				prog[i] = p[i];
			}
			prog[i] = '\0';
			continue;
		}
		for (p = strchr(cline, '\0'); isspace((unsigned char)*--p);)
			continue;
		*++p = '\0';
		f = (struct filed *)calloc(1, sizeof(*f));
		*nextp = f;
		nextp = &f->f_next;
		cfline(cline, f, prog, host);
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

			case F_PIPE:
				printf("%s", f->f_un.f_pipe.f_pname);
				break;

			case F_USERS:
				for (i = 0;
				    i < MAXUNAMES && *f->f_un.f_uname[i]; i++)
					printf("%s, ", f->f_un.f_uname[i]);
				break;
			}
			if (f->f_program != NULL)
				printf(" (%s)", f->f_program);
			printf("\n");
		}
	}

	finet = socksetup(PF_UNSPEC, bindhostname);
	if (finet) {
		if (SecureMode) {
			for (i = 0; i < *finet; i++) {
				if (shutdown(finet[i+1], SHUT_RD) < 0) {
					logerror("shutdown() failed");
					die(NULL);
				}
			}
		} else
			dprintf("Listening on inet and/or inet6 socket\n");
		dprintf("Sending on inet and/or inet6 socket\n");
	}

	logmsg(LOG_SYSLOG|LOG_INFO, "syslogd: restart", LocalHostName, ADDDATE);
	dprintf("syslogd: restarted\n");
	/*
	 * Log a change in hostname, but only on a restart (we detect this
	 * by checking to see if we're passed a kevent).
	 */
	if (ev != NULL && strcmp(oldLocalHostName, LocalHostName) != 0) {
		(void)snprintf(hostMsg, sizeof(hostMsg),
		    "syslogd: host name changed, \"%s\" to \"%s\"",
		    oldLocalHostName, LocalHostName);
		logmsg(LOG_SYSLOG|LOG_INFO, hostMsg, LocalHostName, ADDDATE);
		dprintf("%s\n", hostMsg);
	}
}

/*
 * Crack a configuration file line
 */
void
cfline(char *line, struct filed *f, char *prog, char *host)
{
	struct addrinfo hints, *res;
	int    error, i, pri, syncfile;
	char   *bp, *p, *q;
	char   buf[MAXLINE];

	dprintf("cfline(\"%s\", f, \"%s\", \"%s\")\n", line, prog, host);

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
		while (*q++ && isblank((unsigned char)*q))
			/* skip blanks */;
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
		return;
	}
	
	/* save host name, if any */
	if (*host == '*')
		f->f_host = NULL;
	else {
		f->f_host = strdup(host);
		trim_localdomain(f->f_host);
	}

	/* save program name, if any */
	if (*prog == '*')
		f->f_program = NULL;
	else
		f->f_program = strdup(prog);

	/* scan through the list of selectors */
	for (p = line; *p && !isblank((unsigned char)*p);) {
		int pri_done, pri_cmp, pri_invert;

		/* find the end of this facility name list */
		for (q = p; *q && !isblank((unsigned char)*q) && *q++ != '.'; )
			continue;

		/* get the priority comparison */
		pri_cmp = 0;
		pri_done = 0;
		pri_invert = 0;
		if (*q == '!') {
			pri_invert = 1;
			q++;
		}
		while (! pri_done) {
			switch (*q) {
			case '<':
				pri_cmp = PRI_LT;
				q++;
				break;
			case '=':
				pri_cmp = PRI_EQ;
				q++;
				break;
			case '>':
				pri_cmp = PRI_GT;
				q++;
				break;
			default:
				pri_done = 1;
				break;
			}
		}

		/* collect priority name */
		for (bp = buf; *q && !strchr("\t ,;", *q); )
			*bp++ = *q++;
		*bp = '\0';

		/* skip cruft */
		while (strchr(",;", *q))
			q++;

		/* decode priority name */
		if (*buf == '*') {
			pri = LOG_PRIMASK + 1;
			pri_cmp = PRI_LT | PRI_EQ | PRI_GT;
		} else {
			pri = decode(buf, prioritynames);
			if (pri < 0) {
				errno = 0;
				logerror("Unknown priority name `%s'", buf);
				return;
			}
		}
		if (pri_cmp == 0)
			pri_cmp = UniquePriority ? PRI_EQ
						 : PRI_EQ | PRI_GT;
		if (pri_invert)
			pri_cmp ^= PRI_LT | PRI_EQ | PRI_GT;

		/* scan facilities */
		while (*p && !strchr("\t .;", *p)) {
			for (bp = buf; *p && !strchr("\t ,;.", *p); )
				*bp++ = *p++;
			*bp = '\0';
			if (*buf == '*')
				for (i = 0; i < LOG_NFACILITIES; i++) {
					f->f_pmask[i] = pri;
					f->f_pcmp[i] = pri_cmp;
				}
			else {
				i = decode(buf, facilitynames);
				if (i < 0) {
					errno = 0;
					logerror("Unknown facility name `%s'",
					    buf);
					return;
				}
				f->f_pmask[i >> 3] = pri;
				f->f_pcmp[i >> 3] = pri_cmp;
			}
			while (*p == ',' || *p == ' ')
				p++;
		}

		p = q;
	}

	/* skip to action part */
	while (isblank((unsigned char)*p))
		p++;

	if (*p == '-') {
		syncfile = 0;
		p++;
	} else
		syncfile = 1;

	switch (*p) {
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
		if (syncfile)
			f->f_flags |= FFLAG_SYNC;
		if (isatty(f->f_file))
			f->f_type = F_TTY;
		else
			f->f_type = F_FILE;
		if (strcmp(p, ctty) == 0)
			f->f_type = F_CONSOLE;
		break;

	case '|':
		f->f_un.f_pipe.f_pid = 0;
		(void) strlcpy(f->f_un.f_pipe.f_pname, p + 1,
		    sizeof(f->f_un.f_pipe.f_pname));
		f->f_type = F_PIPE;
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

	if (isdigit((unsigned char)*name))
		return (atoi(name));

	for (p = buf; *name && p < &buf[sizeof(buf) - 1]; p++, name++) {
		if (isupper((unsigned char)*name))
			*p = tolower((unsigned char)*name);
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
socksetup(int af, const char *hostname)
{
	struct addrinfo hints, *res, *r;
	struct kevent *ev;
	int error, maxs, *s, *socks;
	const int on = 1;

	if(SecureMode && !NumForwards)
		return(NULL);

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = af;
	hints.ai_socktype = SOCK_DGRAM;
	error = getaddrinfo(hostname, "syslog", &hints, &res);
	if (error) {
		logerror(gai_strerror(error));
		errno = 0;
		die(NULL);
	}

	/* Count max number of sockets we may open */
	for (maxs = 0, r = res; r; r = r->ai_next, maxs++)
		continue;
	socks = malloc((maxs+1) * sizeof(int));
	if (!socks) {
		logerror("Couldn't allocate memory for sockets");
		die(NULL);
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

		if (!SecureMode) {
			if (bind(*s, r->ai_addr, r->ai_addrlen) < 0) {
				logerror("bind() failed");
				close(*s);
				continue;
			}
			ev = allocevchange();
			EV_SET(ev, *s, EVFILT_READ, EV_ADD | EV_ENABLE,
			    0, 0, (intptr_t) dispatch_read_finet);
		}

		*socks = *socks + 1;
		s++;
	}

	if (*socks == 0) {
		free (socks);
		if(Debug)
			return(NULL);
		else
			die(NULL);
	}
	if (res)
		freeaddrinfo(res);

	return(socks);
}

/*
 * Fairly similar to popen(3), but returns an open descriptor, as opposed
 * to a FILE *.
 */
int
p_open(char *prog, pid_t *rpid)
{
	int pfd[2], nulldesc, i;
	pid_t pid;
	char *argv[4];	/* sh -c cmd NULL */
	char errmsg[200];

	if (pipe(pfd) == -1)
		return (-1);
	if ((nulldesc = open(_PATH_DEVNULL, O_RDWR)) == -1) {
		/* We are royally screwed anyway. */
		return (-1);
	}

	switch ((pid = fork())) {
	case -1:
		(void) close(nulldesc);
		return (-1);

	case 0:
		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = prog;
		argv[3] = NULL;

		(void) setsid();	/* avoid catching SIGHUPs. */

		/*
		 * Reset ignored signals to their default behavior.
		 */
		(void)signal(SIGTERM, SIG_DFL);
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);
		(void)signal(SIGPIPE, SIG_DFL);
		(void)signal(SIGHUP, SIG_DFL);

		dup2(pfd[0], STDIN_FILENO);
		dup2(nulldesc, STDOUT_FILENO);
		dup2(nulldesc, STDERR_FILENO);
		for (i = getdtablesize(); i > 2; i--)
			(void) close(i);

		(void) execvp(_PATH_BSHELL, argv);
		_exit(255);
	}

	(void) close(nulldesc);
	(void) close(pfd[0]);

	/*
	 * Avoid blocking on a hung pipe.  With O_NONBLOCK, we are
	 * supposed to get an EWOULDBLOCK on writev(2), which is
	 * caught by the logic above anyway, which will in turn
	 * close the pipe, and fork a new logging subprocess if
	 * necessary.  The stale subprocess will be killed some
	 * time later unless it terminated itself due to closing
	 * its input pipe.
	 */
	if (fcntl(pfd[1], F_SETFL, O_NONBLOCK) == -1) {
		/* This is bad. */
		(void) snprintf(errmsg, sizeof(errmsg),
		    "Warning: cannot change pipe to pid %d to "
		    "non-blocking.", (int) pid);
		logerror(errmsg);
	}
	*rpid = pid;
	return (pfd[1]);
}

void
deadq_enter(pid_t pid, const char *name)
{
	dq_t p;
	int status;

	/*
	 * Be paranoid: if we can't signal the process, don't enter it
	 * into the dead queue (perhaps it's already dead).  If possible,
	 * we try to fetch and log the child's status.
	 */
	if (kill(pid, 0) != 0) {
		if (waitpid(pid, &status, WNOHANG) > 0)
			log_deadchild(pid, status, name);
		return;
	}

	p = malloc(sizeof(*p));
	if (p == NULL) {
		errno = 0;
		logerror("panic: out of memory!");
		exit(1);
	}

	p->dq_pid = pid;
	p->dq_timeout = DQ_TIMO_INIT;
	TAILQ_INSERT_TAIL(&deadq_head, p, dq_entries);
}

int
deadq_remove(pid_t pid)
{
	dq_t q;

	for (q = TAILQ_FIRST(&deadq_head); q != NULL;
	     q = TAILQ_NEXT(q, dq_entries)) {
		if (q->dq_pid == pid) {
			TAILQ_REMOVE(&deadq_head, q, dq_entries);
			free(q);
			return (1);
		}
	}
	return (0);
}

void
log_deadchild(pid_t pid, int status, const char *name)
{
	int code;
	char buf[256];
	const char *reason;

	/* Keep strerror() struff out of logerror messages. */
	errno = 0;
	if (WIFSIGNALED(status)) {
		reason = "due to signal";
		code = WTERMSIG(status);
	} else {
		reason = "with status";
		code = WEXITSTATUS(status);
		if (code == 0)
			return;
	}
	(void) snprintf(buf, sizeof(buf),
	    "Logging subprocess %d (%s) exited %s %d.",
	    pid, name, reason, code);
	logerror(buf);
}

static struct kevent changebuf[8];
static int nchanges;

static struct kevent *
allocevchange(void)
{

	if (nchanges == A_CNT(changebuf)) {
		/* XXX Error handling could be improved. */
		(void) wait_for_events(NULL, 0);
	}

	return (&changebuf[nchanges++]);
}

static int
wait_for_events(struct kevent *events, size_t nevents)
{
	int rv;

	rv = kevent(fkq, nchanges ? changebuf : NULL, nchanges,
		    events, nevents, NULL);
	nchanges = 0;
	return (rv);
}
