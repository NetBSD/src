/*	$NetBSD: inetd.c,v 1.140 2022/08/10 03:35:38 dholland Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Matthias Scheler.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 1983, 1991, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1991, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#if 0
static char sccsid[] = "@(#)inetd.c	8.4 (Berkeley) 4/13/94";
#else
__RCSID("$NetBSD: inetd.c,v 1.140 2022/08/10 03:35:38 dholland Exp $");
#endif
#endif /* not lint */

/*
 * Inetd - Internet super-server
 *
 * This program invokes all internet services as needed.  Connection-oriented
 * services are invoked each time a connection is made, by creating a process.
 * This process is passed the connection as file descriptor 0 and is expected
 * to do a getpeername to find out the source host and port.
 *
 * Datagram oriented services are invoked when a datagram
 * arrives; a process is created and passed a pending message
 * on file descriptor 0.  Datagram servers may either connect
 * to their peer, freeing up the original socket for inetd
 * to receive further messages on, or ``take over the socket'',
 * processing all arriving datagrams and, eventually, timing
 * out.	 The first type of server is said to be ``multi-threaded'';
 * the second type of server ``single-threaded''.
 *
 * Inetd uses a configuration file which is read at startup
 * and, possibly, at some later time in response to a hangup signal.
 * The configuration file is ``free format'' with fields given in the
 * order shown below.  Continuation lines for an entry must being with
 * a space or tab.  All fields must be present in each entry.
 *
 *	service name			must be in /etc/services or must
 *					name a tcpmux service
 *	socket type[:accf[,arg]]	stream/dgram/raw/rdm/seqpacket,
					only stream can name an accept filter
 *	protocol			must be in /etc/protocols
 *	wait/nowait[:max]		single-threaded/multi-threaded, max #
 *	user[:group]			user/group to run daemon as
 *	server program			full path name
 *	server program arguments	maximum of MAXARGV (64)
 *
 * For RPC services
 *	service name/version		must be in /etc/rpc
 *	socket type			stream/dgram/raw/rdm/seqpacket
 *	protocol			must be in /etc/protocols
 *	wait/nowait[:max]		single-threaded/multi-threaded
 *	user[:group]			user to run daemon as
 *	server program			full path name
 *	server program arguments	maximum of MAXARGV (64)
 *
 * For non-RPC services, the "service name" can be of the form
 * hostaddress:servicename, in which case the hostaddress is used
 * as the host portion of the address to listen on.  If hostaddress
 * consists of a single `*' character, INADDR_ANY is used.
 *
 * A line can also consist of just
 *	hostaddress:
 * where hostaddress is as in the preceding paragraph.  Such a line must
 * have no further fields; the specified hostaddress is remembered and
 * used for all further lines that have no hostaddress specified,
 * until the next such line (or EOF).  (This is why * is provided to
 * allow explicit specification of INADDR_ANY.)  A line
 *	*:
 * is implicitly in effect at the beginning of the file.
 *
 * The hostaddress specifier may (and often will) contain dots;
 * the service name must not.
 *
 * For RPC services, host-address specifiers are accepted and will
 * work to some extent; however, because of limitations in the
 * portmapper interface, it will not work to try to give more than
 * one line for any given RPC service, even if the host-address
 * specifiers are different.
 *
 * TCP services without official port numbers are handled with the
 * RFC1078-based tcpmux internal service. Tcpmux listens on port 1 for
 * requests. When a connection is made from a foreign host, the service
 * requested is passed to tcpmux, which looks it up in the servtab list
 * and returns the proper entry for the service. Tcpmux returns a
 * negative reply if the service doesn't exist, otherwise the invoked
 * server is expected to return the positive reply if the service type in
 * inetd.conf file has the prefix "tcpmux/". If the service type has the
 * prefix "tcpmux/+", tcpmux will return the positive reply for the
 * process; this is for compatibility with older server code, and also
 * allows you to invoke programs that use stdin/stdout without putting any
 * special server code in them. Services that use tcpmux are "nowait"
 * because they do not have a well-known port and hence cannot listen
 * for new requests.
 *
 * Comment lines are indicated by a `#' in column 1.
 *
 * #ifdef IPSEC
 * Comment lines that start with "#@" denote IPsec policy string, as described
 * in ipsec_set_policy(3).  This will affect all the following items in
 * inetd.conf(8).  To reset the policy, just use "#@" line.  By default,
 * there's no IPsec policy.
 * #endif
 */

/*
 * Here's the scoop concerning the user:group feature:
 *
 * 1) set-group-option off.
 *
 * 	a) user = root:	NO setuid() or setgid() is done
 *
 * 	b) other:	setuid()
 * 			setgid(primary group as found in passwd)
 * 			initgroups(name, primary group)
 *
 * 2) set-group-option on.
 *
 * 	a) user = root:	NO setuid()
 * 			setgid(specified group)
 * 			NO initgroups()
 *
 * 	b) other:	setuid()
 * 			setgid(specified group)
 * 			initgroups(name, specified group)
 *
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <net/if.h>

#ifdef RPC
#include <rpc/rpc.h>
#include <rpc/rpcb_clnt.h>
#include <netconfig.h>
#endif

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>
#include <ifaddrs.h>

#include "inetd.h"

#ifdef LIBWRAP
# include <tcpd.h>
#ifndef LIBWRAP_ALLOW_FACILITY
# define LIBWRAP_ALLOW_FACILITY LOG_AUTH
#endif
#ifndef LIBWRAP_ALLOW_SEVERITY
# define LIBWRAP_ALLOW_SEVERITY LOG_INFO
#endif
#ifndef LIBWRAP_DENY_FACILITY
# define LIBWRAP_DENY_FACILITY LOG_AUTH
#endif
#ifndef LIBWRAP_DENY_SEVERITY
# define LIBWRAP_DENY_SEVERITY LOG_WARNING
#endif
int allow_severity = LIBWRAP_ALLOW_FACILITY|LIBWRAP_ALLOW_SEVERITY;
int deny_severity = LIBWRAP_DENY_FACILITY|LIBWRAP_DENY_SEVERITY;
#endif

static bool foreground;
int	debug;
#ifdef LIBWRAP
int	lflag;
#endif
int	maxsock;
int	kq;
int	options;
int	timingout;
const int niflags = NI_NUMERICHOST | NI_NUMERICSERV;

#ifndef OPEN_MAX
#define OPEN_MAX	64
#endif

/* Reserve some descriptors, 3 stdio + at least: 1 log, 1 conf. file */
#define FD_MARGIN	(8)
rlim_t		rlim_ofile_cur = OPEN_MAX;

struct rlimit	rlim_ofile;

struct kevent	changebuf[64];
size_t		changes;

struct servtab *servtab;

static ssize_t	recvfromto(int, void * restrict, size_t, int,
    struct sockaddr * restrict, socklen_t * restrict,
    struct sockaddr * restrict, socklen_t * restrict);
static ssize_t	sendfromto(int, const void *, size_t, int,
    const struct sockaddr *, socklen_t, const struct sockaddr *, socklen_t);
static void	chargen_dg(int, struct servtab *);
static void	chargen_stream(int, struct servtab *);
static void	daytime_dg(int, struct servtab *);
static void	daytime_stream(int, struct servtab *);
static void	discard_dg(int, struct servtab *);
static void	discard_stream(int, struct servtab *);
static void	echo_dg(int, struct servtab *);
static void	echo_stream(int, struct servtab *);
__dead static void	goaway(void);
static void	machtime_dg(int, struct servtab *);
static void	machtime_stream(int, struct servtab *);
static void	reapchild(void);
static void	retry(void);
static void	run_service(int, struct servtab *, int);
static void	tcpmux(int, struct servtab *);
__dead static void	usage(void);
static void	bump_nofile(void);
static void	inetd_setproctitle(char *, int);
static void	initring(void);
static uint32_t	machtime(void);
static int	port_good_dg(struct sockaddr *);
static int	dg_broadcast(struct in_addr *);
static int	my_kevent(const struct kevent *, size_t, struct kevent *, size_t);
static struct kevent	*allocchange(void);
static int	get_line(int, char *, int);
static void	spawn(struct servtab *, int);

struct biltin {
	const char *bi_service;		/* internally provided service name */
	int	bi_socktype;		/* type of socket supported */
	short	bi_fork;		/* 1 if should fork before call */
	short	bi_wait;		/* 1 if should wait for child */
	void	(*bi_fn)(int, struct servtab *);
					/* function which performs it */
} biltins[] = {
	/* Echo received data */
	{ "echo",	SOCK_STREAM,	true, false,	echo_stream },
	{ "echo",	SOCK_DGRAM,	false, false,	echo_dg },

	/* Internet /dev/null */
	{ "discard",	SOCK_STREAM,	true, false,	discard_stream },
	{ "discard",	SOCK_DGRAM,	false, false,	discard_dg },

	/* Return 32 bit time since 1970 */
	{ "time",	SOCK_STREAM,	false, false,	machtime_stream },
	{ "time",	SOCK_DGRAM,	false, false,	machtime_dg },

	/* Return human-readable time */
	{ "daytime",	SOCK_STREAM,	false, false,	daytime_stream },
	{ "daytime",	SOCK_DGRAM,	false, false,	daytime_dg },

	/* Familiar character generator */
	{ "chargen",	SOCK_STREAM,	true, false,	chargen_stream },
	{ "chargen",	SOCK_DGRAM,	false, false,	chargen_dg },

	{ "tcpmux",	SOCK_STREAM,	true, false,	tcpmux }
};

/* list of "bad" ports. I.e. ports that are most obviously used for
 * "cycling packets" denial of service attacks. See /etc/services.
 * List must end with port number "0".
 */

u_int16_t bad_ports[] =  { 7, 9, 13, 19, 37, 0 };


#define NUMINT	(sizeof(intab) / sizeof(struct inent))
const char	*CONFIG = _PATH_INETDCONF;

static int my_signals[] =
    { SIGALRM, SIGHUP, SIGCHLD, SIGTERM, SIGINT, SIGPIPE };

int
main(int argc, char *argv[])
{
	int		ch, n, reload = 1;

	while ((ch = getopt(argc, argv,
#ifdef LIBWRAP
					"dfl"
#else
					"df"
#endif
					   )) != -1)
		switch(ch) {
		case 'd':
			foreground = true;
			debug = true;
			options |= SO_DEBUG;
			break;
		case 'f':
			foreground = true;
			break;
#ifdef LIBWRAP
		case 'l':
			lflag = true;
			break;
#endif
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		CONFIG = argv[0];

	if (!foreground)
		daemon(0, 0);
	openlog("inetd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);
	pidfile(NULL);

	kq = kqueue();
	if (kq < 0) {
		syslog(LOG_ERR, "kqueue: %m");
		return (EXIT_FAILURE);
	}

	if (getrlimit(RLIMIT_NOFILE, &rlim_ofile) < 0) {
		syslog(LOG_ERR, "getrlimit: %m");
	} else {
		rlim_ofile_cur = rlim_ofile.rlim_cur;
		if (rlim_ofile_cur == RLIM_INFINITY)	/* ! */
			rlim_ofile_cur = OPEN_MAX;
	}

	for (n = 0; n < (int)__arraycount(my_signals); n++) {
		int	signum;

		signum = my_signals[n];
		if (signum != SIGCHLD)
			(void) signal(signum, SIG_IGN);

		if (signum != SIGPIPE) {
			struct kevent	*ev;

			ev = allocchange();
			EV_SET(ev, signum, EVFILT_SIGNAL, EV_ADD | EV_ENABLE,
			    0, 0, 0);
		}
	}

	for (;;) {
		int		ctrl;
		struct kevent	eventbuf[64], *ev;
		struct servtab	*sep;

		if (reload) {
			reload = false;
			config_root();
		}

		n = my_kevent(changebuf, changes, eventbuf, __arraycount(eventbuf));
		changes = 0;

		for (ev = eventbuf; n > 0; ev++, n--) {
			if (ev->filter == EVFILT_SIGNAL) {
				switch (ev->ident) {
				case SIGALRM:
					retry();
					break;
				case SIGCHLD:
					reapchild();
					break;
				case SIGTERM:
				case SIGINT:
					goaway();
					break;
				case SIGHUP:
					reload = true;
					break;
				}
				continue;
			}
			if (ev->filter != EVFILT_READ)
				continue;
			sep = (struct servtab *)ev->udata;
			/* Paranoia */
			if ((int)ev->ident != sep->se_fd)
				continue;
			DPRINTF(SERV_FMT ": service requested" , SERV_PARAMS(sep));
			if (sep->se_wait == 0 && sep->se_socktype == SOCK_STREAM) {
				/* XXX here do the libwrap check-before-accept*/
				ctrl = accept(sep->se_fd, NULL, NULL);
				DPRINTF(SERV_FMT ": accept, ctrl fd %d",
				    SERV_PARAMS(sep), ctrl);
				if (ctrl < 0) {
					if (errno != EINTR)
						syslog(LOG_WARNING,
						    SERV_FMT ": accept: %m",
						    SERV_PARAMS(sep));
					continue;
				}
			} else
				ctrl = sep->se_fd;
			spawn(sep, ctrl);
		}
	}
}

static void
spawn(struct servtab *sep, int ctrl)
{
	int dofork;
	pid_t pid;

	pid = 0;
#ifdef LIBWRAP_INTERNAL
	dofork = true;
#else
	dofork = (sep->se_bi == NULL || sep->se_bi->bi_fork);
#endif
	if (dofork) {
		if (rl_process(sep, ctrl)) {
			return;
		}
		pid = fork();
		if (pid < 0) {
			syslog(LOG_ERR, "fork: %m");
			if (sep->se_wait == 0 && sep->se_socktype == SOCK_STREAM)
				close(ctrl);
			sleep(1);
			return;
		}
		if (pid != 0 && sep->se_wait != 0) {
			struct kevent	*ev;

			sep->se_wait = pid;
			ev = allocchange();
			EV_SET(ev, sep->se_fd, EVFILT_READ,
			    EV_DELETE, 0, 0, 0);
		}
		if (pid == 0) {
			size_t	n;

			for (n = 0; n < __arraycount(my_signals); n++)
				(void) signal(my_signals[n], SIG_DFL);
			/* Don't put services in terminal session */
			if (foreground)
				setsid();
		}
	}
	if (pid == 0) {
		run_service(ctrl, sep, dofork);
		if (dofork)
			exit(EXIT_SUCCESS);
	}
	if (sep->se_wait == 0 && sep->se_socktype == SOCK_STREAM)
		close(ctrl);
}

static void
run_service(int ctrl, struct servtab *sep, int didfork)
{
	struct passwd *pwd;
	struct group *grp = NULL;	/* XXX gcc */
	char buf[NI_MAXSERV];
	struct servtab *s;
#ifdef LIBWRAP
	char abuf[BUFSIZ];
	struct request_info req;
	int denied;
	char *service = NULL;	/* XXX gcc */
#endif

#ifdef LIBWRAP
#ifndef LIBWRAP_INTERNAL
	if (sep->se_bi == 0)
#endif
	if (sep->se_wait == 0 && sep->se_socktype == SOCK_STREAM) {
		request_init(&req, RQ_DAEMON, sep->se_argv[0] != NULL ?
		    sep->se_argv[0] : sep->se_service, RQ_FILE, ctrl, NULL);
		fromhost(&req);
		denied = hosts_access(&req) == 0;
		if (denied || lflag) {
			if (getnameinfo(&sep->se_ctrladdr,
			    (socklen_t)sep->se_ctrladdr.sa_len, NULL, 0,
			    buf, sizeof(buf), 0) != 0) {
				/* shouldn't happen */
				(void)snprintf(buf, sizeof buf, "%d",
				    ntohs(sep->se_ctrladdr_in.sin_port));
			}
			service = buf;
			if (req.client->sin != NULL) {
				sockaddr_snprintf(abuf, sizeof(abuf), "%a",
				    req.client->sin);
			} else {
				strcpy(abuf, "(null)");
			}
		}
		if (denied) {
			syslog(deny_severity,
			    "refused connection from %.500s(%s), service %s (%s)",
			    eval_client(&req), abuf, service, sep->se_proto);
			goto reject;
		}
		if (lflag) {
			syslog(allow_severity,
			    "connection from %.500s(%s), service %s (%s)",
			    eval_client(&req), abuf, service, sep->se_proto);
		}
	}
#endif /* LIBWRAP */

	if (sep->se_bi != NULL) {
		if (didfork) {
			for (s = servtab; s != NULL; s = s->se_next)
				if (s->se_fd != -1 && s->se_fd != ctrl) {
					close(s->se_fd);
					s->se_fd = -1;
				}
		}
		(*sep->se_bi->bi_fn)(ctrl, sep);
	} else {
		if ((pwd = getpwnam(sep->se_user)) == NULL) {
			syslog(LOG_ERR, "%s/%s: %s: No such user",
			    sep->se_service, sep->se_proto, sep->se_user);
			goto reject;
		}
		if (sep->se_group != NULL &&
		    (grp = getgrnam(sep->se_group)) == NULL) {
			syslog(LOG_ERR, "%s/%s: %s: No such group",
			    sep->se_service, sep->se_proto, sep->se_group);
			goto reject;
		}
		if (pwd->pw_uid != 0) {
			if (sep->se_group != NULL)
				pwd->pw_gid = grp->gr_gid;
			if (setgid(pwd->pw_gid) < 0) {
				syslog(LOG_ERR,
				 "%s/%s: can't set gid %d: %m", sep->se_service,
				    sep->se_proto, pwd->pw_gid);
				goto reject;
			}
			(void) initgroups(pwd->pw_name,
			    pwd->pw_gid);
			if (setuid(pwd->pw_uid) < 0) {
				syslog(LOG_ERR,
				 "%s/%s: can't set uid %d: %m", sep->se_service,
				    sep->se_proto, pwd->pw_uid);
				goto reject;
			}
		} else if (sep->se_group != NULL) {
			(void) setgid((gid_t)grp->gr_gid);
		}
		DPRINTF("%d execl %s",
		    getpid(), sep->se_server);
		/* Set our control descriptor to not close-on-exec... */
		if (fcntl(ctrl, F_SETFD, 0) < 0)
			syslog(LOG_ERR, "fcntl (%d, F_SETFD, 0): %m", ctrl);
		/* ...and dup it to stdin, stdout, and stderr. */
		if (ctrl != 0) {
			dup2(ctrl, 0);
			close(ctrl);
			ctrl = 0;
		}
		dup2(0, 1);
		dup2(0, 2);
		if (rlim_ofile.rlim_cur != rlim_ofile_cur &&
		    setrlimit(RLIMIT_NOFILE, &rlim_ofile) < 0)
			syslog(LOG_ERR, "setrlimit: %m");
		execv(sep->se_server, sep->se_argv);
		syslog(LOG_ERR, "cannot execute %s: %m", sep->se_server);
	reject:
		if (sep->se_socktype != SOCK_STREAM)
			recv(ctrl, buf, sizeof (buf), 0);
		_exit(EXIT_FAILURE);
	}
}

static void
reapchild(void)
{
	int status;
	pid_t pid;
	struct servtab *sep;

	for (;;) {
		pid = wait3(&status, WNOHANG, NULL);
		if (pid <= 0)
			break;
		DPRINTF("%d reaped, status %#x", pid, status);
		for (sep = servtab; sep != NULL; sep = sep->se_next)
			if (sep->se_wait == pid) {
				struct kevent	*ev;

				if (WIFEXITED(status) && WEXITSTATUS(status))
					syslog(LOG_WARNING,
					    "%s: exit status %u",
					    sep->se_server, WEXITSTATUS(status));
				else if (WIFSIGNALED(status))
					syslog(LOG_WARNING,
					    "%s: exit signal %u",
					    sep->se_server, WTERMSIG(status));
				sep->se_wait = 1;
				ev = allocchange();
				EV_SET(ev, sep->se_fd, EVFILT_READ,
				    EV_ADD | EV_ENABLE, 0, 0, (intptr_t)sep);
				DPRINTF("restored %s, fd %d",
				    sep->se_service, sep->se_fd);
			}
	}
}

static void
retry(void)
{
	struct servtab *sep;

	timingout = false;
	for (sep = servtab; sep != NULL; sep = sep->se_next) {
		if (sep->se_fd == -1 && !ISMUX(sep)) {
			switch (sep->se_family) {
			case AF_LOCAL:
			case AF_INET:
#ifdef INET6
			case AF_INET6:
#endif
				setup(sep);
				if (sep->se_fd >= 0 && isrpcservice(sep))
					register_rpc(sep);
				break;
			}
		}
	}
}

static void
goaway(void)
{
	struct servtab *sep;

	for (sep = servtab; sep != NULL; sep = sep->se_next) {
		if (sep->se_fd == -1)
			continue;

		switch (sep->se_family) {
		case AF_LOCAL:
			(void)unlink(sep->se_service);
			break;
		case AF_INET:
#ifdef INET6
		case AF_INET6:
#endif
			if (sep->se_wait == 1 && isrpcservice(sep))
				unregister_rpc(sep);
			break;
		}
		(void)close(sep->se_fd);
		sep->se_fd = -1;
	}

	DPRINTF("Going away.");

	exit(EXIT_SUCCESS);
}

void
setup(struct servtab *sep)
{
	int		on = 1;
#ifdef INET6
	int		off = 0;
#endif
	struct kevent	*ev;

	if ((sep->se_fd = socket(sep->se_family, sep->se_socktype, 0)) < 0) {
		DPRINTF("socket failed on " SERV_FMT ": %s",
		    SERV_PARAMS(sep), strerror(errno));
		syslog(LOG_ERR, "%s/%s: socket: %m",
		    sep->se_service, sep->se_proto);
		return;
	}
	/* Set all listening sockets to close-on-exec. */
	if (fcntl(sep->se_fd, F_SETFD, FD_CLOEXEC) < 0) {
		syslog(LOG_ERR, SERV_FMT ": fcntl(F_SETFD, FD_CLOEXEC): %m",
		    SERV_PARAMS(sep));
		close(sep->se_fd);
		sep->se_fd = -1;
		return;
	}

#define	turnon(fd, opt) \
setsockopt(fd, SOL_SOCKET, opt, &on, (socklen_t)sizeof(on))
	if (strcmp(sep->se_proto, "tcp") == 0 && (options & SO_DEBUG) &&
	    turnon(sep->se_fd, SO_DEBUG) < 0)
		syslog(LOG_ERR, "setsockopt (SO_DEBUG): %m");
	if (turnon(sep->se_fd, SO_REUSEADDR) < 0)
		syslog(LOG_ERR, "setsockopt (SO_REUSEADDR): %m");
#undef turnon

	/* Set the socket buffer sizes, if specified. */
	if (sep->se_sndbuf != 0 && setsockopt(sep->se_fd, SOL_SOCKET,
	    SO_SNDBUF, &sep->se_sndbuf, (socklen_t)sizeof(sep->se_sndbuf)) < 0)
		syslog(LOG_ERR, "setsockopt (SO_SNDBUF %d): %m",
		    sep->se_sndbuf);
	if (sep->se_rcvbuf != 0 && setsockopt(sep->se_fd, SOL_SOCKET,
	    SO_RCVBUF, &sep->se_rcvbuf, (socklen_t)sizeof(sep->se_rcvbuf)) < 0)
		syslog(LOG_ERR, "setsockopt (SO_RCVBUF %d): %m",
		    sep->se_rcvbuf);
#ifdef INET6
	if (sep->se_family == AF_INET6) {
		int *v;
		v = (sep->se_type == FAITH_TYPE) ? &on : &off;
		if (setsockopt(sep->se_fd, IPPROTO_IPV6, IPV6_FAITH,
		    v, (socklen_t)sizeof(*v)) < 0)
			syslog(LOG_ERR, "setsockopt (IPV6_FAITH): %m");
	}
#endif
#ifdef IPSEC
	/* Avoid setting a policy if a policy specifier doesn't exist. */
	if (sep->se_policy != NULL) {
		int e = ipsecsetup(sep->se_family, sep->se_fd, sep->se_policy);
		if (e < 0) {
			syslog(LOG_ERR, SERV_FMT ": ipsec setup failed",
			    SERV_PARAMS(sep));
			(void)close(sep->se_fd);
			sep->se_fd = -1;
			return;
		}
	}
#endif

	if (bind(sep->se_fd, &sep->se_ctrladdr, sep->se_ctrladdr_size) < 0) {
		DPRINTF(SERV_FMT ": bind failed: %s",
			SERV_PARAMS(sep), strerror(errno));
		syslog(LOG_ERR, SERV_FMT ": bind: %m",
		    SERV_PARAMS(sep));
		(void) close(sep->se_fd);
		sep->se_fd = -1;
		if (!timingout) {
			timingout = true;
			alarm(RETRYTIME);
		}
		return;
	}
	if (sep->se_socktype == SOCK_STREAM)
		listen(sep->se_fd, 10);

	/* for internal dgram, setsockopt() is required for recvfromto() */
	if (sep->se_socktype == SOCK_DGRAM && sep->se_bi != NULL) {
		switch (sep->se_family) {
		case AF_INET:
			if (setsockopt(sep->se_fd, IPPROTO_IP,
			    IP_RECVDSTADDR, &on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt (IP_RECVDSTADDR): %m");
			break;
#ifdef INET6
		case AF_INET6:
			if (setsockopt(sep->se_fd, IPPROTO_IPV6,
			    IPV6_RECVPKTINFO, &on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt (IPV6_RECVPKTINFO): %m");
			break;
#endif
		}
	}

	/* Set the accept filter, if specified. To be done after listen.*/
	if (sep->se_accf.af_name[0] != 0 && setsockopt(sep->se_fd, SOL_SOCKET,
	    SO_ACCEPTFILTER, &sep->se_accf,
	    (socklen_t)sizeof(sep->se_accf)) < 0)
		syslog(LOG_ERR, "setsockopt(SO_ACCEPTFILTER %s): %m",
		    sep->se_accf.af_name);

	ev = allocchange();
	EV_SET(ev, sep->se_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
	    (intptr_t)sep);
	if (sep->se_fd > maxsock) {
		maxsock = sep->se_fd;
		if (maxsock > (int)(rlim_ofile_cur - FD_MARGIN))
			bump_nofile();
	}
	DPRINTF(SERV_FMT ": registered on fd %d", SERV_PARAMS(sep), sep->se_fd);
}

/*
 * Finish with a service and its socket.
 */
void
close_sep(struct servtab *sep)
{

	if (sep->se_fd >= 0) {
		(void) close(sep->se_fd);
		sep->se_fd = -1;
	}
	sep->se_count = 0;
	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		rl_clear_ip_list(sep);
	}
}

void
register_rpc(struct servtab *sep)
{
#ifdef RPC
	struct netbuf nbuf;
	struct sockaddr_storage ss;
	struct netconfig *nconf;
	socklen_t socklen;
	int n;

	if ((nconf = getnetconfigent(sep->se_proto+4)) == NULL) {
		syslog(LOG_ERR, "%s: getnetconfigent failed",
		    sep->se_proto);
		return;
	}
	socklen = sizeof ss;
	if (getsockname(sep->se_fd, (struct sockaddr *)(void *)&ss, &socklen) < 0) {
		syslog(LOG_ERR, SERV_FMT ": getsockname: %m",
		    SERV_PARAMS(sep));
		return;
	}

	nbuf.buf = &ss;
	nbuf.len = ss.ss_len;
	nbuf.maxlen = sizeof (struct sockaddr_storage);
	for (n = sep->se_rpcversl; n <= sep->se_rpcversh; n++) {
		DPRINTF("rpcb_set: %u %d %s %s",
		    sep->se_rpcprog, n, nconf->nc_netid,
		    taddr2uaddr(nconf, &nbuf));
		(void)rpcb_unset((unsigned int)sep->se_rpcprog, (unsigned int)n, nconf);
		if (rpcb_set((unsigned int)sep->se_rpcprog, (unsigned int)n, nconf, &nbuf) == 0)
			syslog(LOG_ERR, "rpcb_set: %u %d %s %s%s",
			    sep->se_rpcprog, n, nconf->nc_netid,
			    taddr2uaddr(nconf, &nbuf), clnt_spcreateerror(""));
	}
#endif /* RPC */
}

void
unregister_rpc(struct servtab *sep)
{
#ifdef RPC
	int n;
	struct netconfig *nconf;

	if ((nconf = getnetconfigent(sep->se_proto+4)) == NULL) {
		syslog(LOG_ERR, "%s: getnetconfigent failed",
		    sep->se_proto);
		return;
	}

	for (n = sep->se_rpcversl; n <= sep->se_rpcversh; n++) {
		DPRINTF("rpcb_unset(%u, %d, %s)",
		    sep->se_rpcprog, n, nconf->nc_netid);
		if (rpcb_unset((unsigned int)sep->se_rpcprog, (unsigned int)n, nconf) == 0)
			syslog(LOG_ERR, "rpcb_unset(%u, %d, %s) failed\n",
			    sep->se_rpcprog, n, nconf->nc_netid);
	}
#endif /* RPC */
}

static void
inetd_setproctitle(char *a, int s)
{
	socklen_t size;
	struct sockaddr_storage ss;
	char hbuf[NI_MAXHOST];
	const char *hp;
	struct sockaddr *sa;

	size = sizeof(ss);
	sa = (struct sockaddr *)(void *)&ss;
	if (getpeername(s, sa, &size) == 0) {
		if (getnameinfo(sa, size, hbuf, (socklen_t)sizeof(hbuf), NULL,
		    0, niflags) != 0)
			hp = "?";
		else
			hp = hbuf;
		setproctitle("-%s [%s]", a, hp);
	} else
		setproctitle("-%s", a);
}

static void
bump_nofile(void)
{
#define FD_CHUNK	32
	struct rlimit rl;

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
		syslog(LOG_ERR, "getrlimit: %m");
		return;
	}
	rl.rlim_cur = MIN(rl.rlim_max, rl.rlim_cur + FD_CHUNK);
	if (rl.rlim_cur <= rlim_ofile_cur) {
		syslog(LOG_ERR,
		    "bump_nofile: cannot extend file limit, max = %d",
		    (int)rl.rlim_cur);
		return;
	}

	if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
		syslog(LOG_ERR, "setrlimit: %m");
		return;
	}

	rlim_ofile_cur = rl.rlim_cur;
	return;
}

/*
 * In order to get the destination address (`to') with recvfromto(),
 * IP_RECVDSTADDR or IP_RECVPKTINFO for AF_INET, or IPV6_RECVPKTINFO
 * for AF_INET6, must be enabled with setsockopt(2).
 *
 * .sin_port and .sin6_port in 'to' are always stored as zero.
 * If necessary, extract them using getsockname(2).
 */
static ssize_t
recvfromto(int s, void * restrict buf, size_t len, int flags,
    struct sockaddr * restrict from, socklen_t * restrict fromlen,
    struct sockaddr * restrict to, socklen_t * restrict tolen)
{
	struct msghdr msg;
	struct iovec vec;
	struct cmsghdr *cmsg;
	struct sockaddr_storage ss;
	char cmsgbuf[1024];
	ssize_t rc;

	if (to == NULL)
		return recvfrom(s, buf, len, flags, from, fromlen);

	if (tolen == NULL || fromlen == NULL) {
		errno = EFAULT;
		return -1;
	}

	vec.iov_base = buf;
	vec.iov_len = len;
	msg.msg_name = from;
	msg.msg_namelen = *fromlen;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	rc = recvmsg(s, &msg, flags);
	if (rc < 0)
		return rc;
	*fromlen = msg.msg_namelen;

	memset(&ss, 0, sizeof(ss));
	for (cmsg = (struct cmsghdr *)CMSG_FIRSTHDR(&msg); cmsg != NULL;
	    cmsg = (struct cmsghdr *)CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP &&
		    cmsg->cmsg_type == IP_RECVDSTADDR) {
			struct in_addr *dst = (struct in_addr *)CMSG_DATA(cmsg);
			struct sockaddr_in *sin = (struct sockaddr_in *)&ss;

			sin->sin_len = sizeof(*sin);
			sin->sin_family = AF_INET;
			sin->sin_addr = *dst;
			break;
		}
		if (cmsg->cmsg_level == IPPROTO_IP &&
		    cmsg->cmsg_type == IP_PKTINFO) {
			struct in_pktinfo *pi =
			    (struct in_pktinfo *)CMSG_DATA(cmsg);
			struct sockaddr_in *sin = (struct sockaddr_in *)&ss;

			sin->sin_len = sizeof(*sin);
			sin->sin_family = AF_INET;
			sin->sin_addr = pi->ipi_addr;
			break;
		}
#ifdef INET6
		if (cmsg->cmsg_level == IPPROTO_IPV6 &&
		    cmsg->cmsg_type == IPV6_PKTINFO) {
			struct in6_pktinfo *pi6 =
			    (struct in6_pktinfo *)CMSG_DATA(cmsg);
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;

			sin6->sin6_len = sizeof(*sin6);
			sin6->sin6_family = AF_INET6;
			sin6->sin6_addr = pi6->ipi6_addr;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) ||
			    IN6_IS_ADDR_MC_LINKLOCAL(&sin6->sin6_addr))
				sin6->sin6_scope_id = pi6->ipi6_ifindex;
			else
				sin6->sin6_scope_id = 0;
			break;
		}
#endif /* INET6 */
	}

	socklen_t sslen = (*tolen < ss.ss_len) ? *tolen : ss.ss_len;
	if (sslen > 0)
		memcpy(to, &ss, sslen);
	*tolen = sslen;

	return rc;
}

/*
 * When sending, the source port is selected as the one bind(2)'ed
 * to the socket.
 * .sin_port and .sin6_port in `from' are always ignored.
 */
static ssize_t
sendfromto(int s, const void *buf, size_t len, int flags,
    const struct sockaddr *from, socklen_t fromlen,
    const struct sockaddr *to, socklen_t tolen)
{
	struct msghdr msg;
	struct iovec vec;
	struct cmsghdr *cmsg;
	char cmsgbuf[256];
	__CTASSERT(sizeof(cmsgbuf) > CMSG_SPACE(sizeof(struct in_pktinfo)));
#ifdef INET6
	__CTASSERT(sizeof(cmsgbuf) > CMSG_SPACE(sizeof(struct in6_pktinfo)));
#endif

	if (from == NULL || fromlen == 0)
		return sendto(s, buf, len, flags, to, tolen);

	vec.iov_base = __UNCONST(buf);
	vec.iov_len = len;
	msg.msg_name = __UNCONST(to);
	msg.msg_namelen = tolen;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = 0;

	if (fromlen < 2) {	/* sa_len + sa_family */
		errno = EINVAL;
		return -1;
	}

	cmsg = (struct cmsghdr *)cmsgbuf;
	if (from->sa_family == AF_INET) {
		const struct sockaddr_in *from4 =
		    (const struct sockaddr_in *)from;
		struct in_pktinfo *pi;

		if (fromlen != sizeof(struct sockaddr_in) ||
		    from4->sin_family != AF_INET) {
			errno = EINVAL;
			return -1;
		}

		msg.msg_controllen += CMSG_SPACE(sizeof(struct in_pktinfo));
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
		cmsg->cmsg_level = IPPROTO_IP;
		cmsg->cmsg_type = IP_PKTINFO;

		pi = (struct in_pktinfo *)CMSG_DATA(cmsg);
		pi->ipi_addr = from4->sin_addr;
		pi->ipi_ifindex = 0;
#ifdef INET6
	} else if (from->sa_family == AF_INET6) {
		const struct sockaddr_in6 *from6 =
		    (const struct sockaddr_in6 *)from;
		struct in6_pktinfo *pi6;

		if (fromlen != sizeof(struct sockaddr_in6) ||
		    from6->sin6_family != AF_INET6) {
			errno = EINVAL;
			return -1;
		}

		msg.msg_controllen += CMSG_SPACE(sizeof(struct in6_pktinfo));
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
		cmsg->cmsg_level = IPPROTO_IPV6;
		cmsg->cmsg_type = IPV6_PKTINFO;

		pi6 = (struct in6_pktinfo *)CMSG_DATA(cmsg);
		pi6->ipi6_addr = from6->sin6_addr;
		if (IN6_IS_ADDR_LINKLOCAL(&from6->sin6_addr) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(&from6->sin6_addr)) {
			pi6->ipi6_ifindex = from6->sin6_scope_id;
		} else {
			pi6->ipi6_ifindex = 0;
		}
#endif /* INET6 */
	} else {
		return sendto(s, buf, len, flags, to, tolen);
	}

	return sendmsg(s, &msg, flags);
}

/*
 * Internet services provided internally by inetd:
 */
#define	BUFSIZE	4096

/* ARGSUSED */
static void
echo_stream(int s, struct servtab *sep)	/* Echo service -- echo data back */
{
	char buffer[BUFSIZE];
	ssize_t i;

	inetd_setproctitle(sep->se_service, s);
	while ((i = read(s, buffer, sizeof(buffer))) > 0 &&
	    write(s, buffer, (size_t)i) > 0)
		continue;
}

/* ARGSUSED */
static void
echo_dg(int s, struct servtab *sep)	/* Echo service -- echo data back */
{
	char buffer[BUFSIZE];
	ssize_t i;
	socklen_t rsize, lsize;
	struct sockaddr_storage remote, local;
	struct sockaddr *lsa, *rsa;

	rsa = (struct sockaddr *)(void *)&remote;
	lsa = (struct sockaddr *)(void *)&local;
	rsize = sizeof(remote);
	lsize = sizeof(local);
	if ((i = recvfromto(s, buffer, sizeof(buffer), 0,
	    rsa, &rsize, lsa, &lsize)) < 0)
		return;
	if (port_good_dg(rsa))
		(void) sendfromto(s, buffer, (size_t)i, 0,
		    lsa, lsize, rsa, rsize);
}

/* ARGSUSED */
static void
discard_stream(int s, struct servtab *sep) /* Discard service -- ignore data */
{
	char buffer[BUFSIZE];

	inetd_setproctitle(sep->se_service, s);
	while ((errno = 0, read(s, buffer, sizeof(buffer)) > 0) ||
			errno == EINTR)
		;
}

/* ARGSUSED */
static void
discard_dg(int s, struct servtab *sep)	/* Discard service -- ignore data */

{
	char buffer[BUFSIZE];

	(void) read(s, buffer, sizeof(buffer));
}

#define LINESIZ 72
char ring[128];
char *endring;

static void
initring(void)
{
	int i;

	endring = ring;

	for (i = 0; i <= 128; ++i)
		if (isprint(i))
			*endring++ = (char)i;
}

/* ARGSUSED */
static void
chargen_stream(int s, struct servtab *sep)	/* Character generator */
{
	size_t len;
	char *rs, text[LINESIZ+2];

	inetd_setproctitle(sep->se_service, s);

	if (endring == NULL) {
		initring();
		rs = ring;
	}

	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	for (rs = ring;;) {
		if ((len = (size_t)(endring - rs)) >= LINESIZ)
			memmove(text, rs, LINESIZ);
		else {
			memmove(text, rs, len);
			memmove(text + len, ring, LINESIZ - len);
		}
		if (++rs == endring)
			rs = ring;
		if (write(s, text, sizeof(text)) != sizeof(text))
			break;
	}
}

/* ARGSUSED */
static void
chargen_dg(int s, struct servtab *sep)		/* Character generator */
{
	struct sockaddr_storage remote, local;
	struct sockaddr *rsa, *lsa;
	static char *rs;
	size_t len;
	socklen_t rsize, lsize;
	char text[LINESIZ+2];

	if (endring == 0) {
		initring();
		rs = ring;
	}

	rsa = (struct sockaddr *)(void *)&remote;
	lsa = (struct sockaddr *)(void *)&local;
	rsize = sizeof(remote);
	lsize = sizeof(local);
	if (recvfromto(s, text, sizeof(text), 0,
	    rsa, &rsize, lsa, &lsize) < 0)
		return;

	if (!port_good_dg(rsa))
		return;

	if ((len = (size_t)(endring - rs)) >= LINESIZ)
		memmove(text, rs, LINESIZ);
	else {
		memmove(text, rs, len);
		memmove(text + len, ring, LINESIZ - len);
	}
	if (++rs == endring)
		rs = ring;
	text[LINESIZ] = '\r';
	text[LINESIZ + 1] = '\n';
	(void) sendfromto(s, text, sizeof(text), 0, lsa, lsize, rsa, rsize);
}

/*
 * Return a machine readable date and time, in the form of the
 * number of seconds since midnight, Jan 1, 1900.  Since gettimeofday
 * returns the number of seconds since midnight, Jan 1, 1970,
 * we must add 2208988800 seconds to this figure to make up for
 * some seventy years Bell Labs was asleep.
 */

static uint32_t
machtime(void)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0) {
		DPRINTF("Unable to get time of day");
		return (0);
	}
#define	OFFSET ((uint32_t)25567 * 24*60*60)
	return (htonl((uint32_t)(tv.tv_sec + OFFSET)));
#undef OFFSET
}

/* ARGSUSED */
static void
machtime_stream(int s, struct servtab *sep)
{
	uint32_t result;

	result = machtime();
	(void) write(s, &result, sizeof(result));
}

/* ARGSUSED */
void
machtime_dg(int s, struct servtab *sep)
{
	uint32_t result;
	struct sockaddr_storage remote, local;
	struct sockaddr *rsa, *lsa;
	socklen_t rsize, lsize;

	rsa = (struct sockaddr *)(void *)&remote;
	lsa = (struct sockaddr *)(void *)&local;
	rsize = sizeof(remote);
	lsize = sizeof(local);
	if (recvfromto(s, &result, sizeof(result), 0,
	    rsa, &rsize, lsa, &lsize) < 0)
		return;
	if (!port_good_dg(rsa))
		return;
	result = machtime();
	(void)sendfromto(s, &result, sizeof(result), 0, lsa, lsize, rsa, rsize);
}

/* ARGSUSED */
static void
daytime_stream(int s,struct servtab *sep)
/* Return human-readable time of day */
{
	char buffer[256];
	time_t clk;
	int len;

	clk = time((time_t *) 0);

	len = snprintf(buffer, sizeof buffer, "%.24s\r\n", ctime(&clk));
	(void) write(s, buffer, (size_t)len);
}

/* ARGSUSED */
void
daytime_dg(int s, struct servtab *sep)
/* Return human-readable time of day */
{
	char buffer[256];
	time_t clk;
	struct sockaddr_storage remote, local;
	struct sockaddr *rsa, *lsa;
	socklen_t rsize, lsize;
	int len;

	clk = time((time_t *) 0);

	rsa = (struct sockaddr *)(void *)&remote;
	lsa = (struct sockaddr *)(void *)&local;
	rsize = sizeof(remote);
	lsize = sizeof(local);
	if (recvfromto(s, buffer, sizeof(buffer), 0,
	    rsa, &rsize, lsa, &lsize) < 0)
		return;
	if (!port_good_dg(rsa))
		return;
	len = snprintf(buffer, sizeof buffer, "%.24s\r\n", ctime(&clk));
	(void) sendfromto(s, buffer, (size_t)len, 0, lsa, lsize, rsa, rsize);
}

static void
usage(void)
{
#ifdef LIBWRAP
	(void)fprintf(stderr, "usage: %s [-dl] [conf]\n", getprogname());
#else
	(void)fprintf(stderr, "usage: %s [-d] [conf]\n", getprogname());
#endif
	exit(EXIT_FAILURE);
}


/*
 *  Based on TCPMUX.C by Mark K. Lottor November 1988
 *  sri-nic::ps:<mkl>tcpmux.c
 */

static int		/* # of characters upto \r,\n or \0 */
get_line(int fd,	char *buf, int len)
{
	int count = 0;
	ssize_t n;

	do {
		n = read(fd, buf, (size_t)(len - count));
		if (n == 0)
			return (count);
		if (n < 0)
			return (-1);
		while (--n >= 0) {
			if (*buf == '\r' || *buf == '\n' || *buf == '\0')
				return (count);
			count++;
			buf++;
		}
	} while (count < len);
	return (count);
}

#define MAX_SERV_LEN	(256+2)		/* 2 bytes for \r\n */

#define strwrite(fd, buf)	(void) write(fd, buf, sizeof(buf)-1)

static void
tcpmux(int ctrl, struct servtab *sep)
{
	char service[MAX_SERV_LEN+1];
	int len;

	/* Get requested service name */
	if ((len = get_line(ctrl, service, MAX_SERV_LEN)) < 0) {
		strwrite(ctrl, "-Error reading service name\r\n");
		goto reject;
	}
	service[len] = '\0';

	DPRINTF("tcpmux: %s: service requested", service);

	/*
	 * Help is a required command, and lists available services,
	 * one per line.
	 */
	if (strcasecmp(service, "help") == 0) {
		strwrite(ctrl, "+Available services:\r\n");
		strwrite(ctrl, "help\r\n");
		for (sep = servtab; sep != NULL; sep = sep->se_next) {
			if (!ISMUX(sep))
				continue;
			(void)write(ctrl, sep->se_service,
			    strlen(sep->se_service));
			strwrite(ctrl, "\r\n");
		}
		goto reject;
	}

	/* Try matching a service in inetd.conf with the request */
	for (sep = servtab; sep != NULL; sep = sep->se_next) {
		if (!ISMUX(sep))
			continue;
		if (strcasecmp(service, sep->se_service) == 0) {
			if (ISMUXPLUS(sep))
				strwrite(ctrl, "+Go\r\n");
			run_service(ctrl, sep, true /* forked */);
			return;
		}
	}
	strwrite(ctrl, "-Service not available\r\n");
reject:
	_exit(EXIT_FAILURE);
}

/*
 * check if the address/port where send data to is one of the obvious ports
 * that are used for denial of service attacks like two echo ports
 * just echoing data between them
 */
static int
port_good_dg(struct sockaddr *sa)
{
	struct in_addr in;
	struct sockaddr_in *sin;
#ifdef INET6
	struct in6_addr *in6;
	struct sockaddr_in6 *sin6;
#endif
	u_int16_t port;
	int i;
	char hbuf[NI_MAXHOST];

	switch (sa->sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in *)(void *)sa;
		in.s_addr = ntohl(sin->sin_addr.s_addr);
		port = ntohs(sin->sin_port);
#ifdef INET6
	v4chk:
#endif
		if (IN_MULTICAST(in.s_addr))
			goto bad;
		switch ((in.s_addr & 0xff000000) >> 24) {
		case 0: case 127: case 255:
			goto bad;
		}
		if (dg_broadcast(&in))
			goto bad;
		break;
#ifdef INET6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)(void *)sa;
		in6 = &sin6->sin6_addr;
		port = ntohs(sin6->sin6_port);
		if (IN6_IS_ADDR_MULTICAST(in6) || IN6_IS_ADDR_UNSPECIFIED(in6))
			goto bad;
		if (IN6_IS_ADDR_V4MAPPED(in6) || IN6_IS_ADDR_V4COMPAT(in6)) {
			memcpy(&in, &in6->s6_addr[12], sizeof(in));
			in.s_addr = ntohl(in.s_addr);
			goto v4chk;
		}
		break;
#endif
	default:
		/* XXX unsupported af, is it safe to assume it to be safe? */
		return true;
	}

	for (i = 0; bad_ports[i] != 0; i++) {
		if (port == bad_ports[i])
			goto bad;
	}

	return true;

bad:
	if (getnameinfo(sa, sa->sa_len, hbuf, (socklen_t)sizeof(hbuf), NULL, 0,
	    niflags) != 0)
		strlcpy(hbuf, "?", sizeof(hbuf));
	syslog(LOG_WARNING,"Possible DoS attack from %s, Port %d",
		hbuf, port);
	return false;
}

/* XXX need optimization */
static int
dg_broadcast(struct in_addr *in)
{
	struct ifaddrs *ifa, *ifap;
	struct sockaddr_in *sin;

	if (getifaddrs(&ifap) < 0)
		return false;
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET ||
		    (ifa->ifa_flags & IFF_BROADCAST) == 0)
			continue;
		sin = (struct sockaddr_in *)(void *)ifa->ifa_broadaddr;
		if (sin->sin_addr.s_addr == in->s_addr) {
			freeifaddrs(ifap);
			return true;
		}
	}
	freeifaddrs(ifap);
	return false;
}

static int
my_kevent(const struct kevent *changelist, size_t nchanges,
    struct kevent *eventlist, size_t nevents)
{
	int	result;

	while ((result = kevent(kq, changelist, nchanges, eventlist, nevents,
	    NULL)) < 0)
		if (errno != EINTR) {
			syslog(LOG_ERR, "kevent: %m");
			exit(EXIT_FAILURE);
		}

	return (result);
}

static struct kevent *
allocchange(void)
{
	if (changes == __arraycount(changebuf)) {
		(void) my_kevent(changebuf, __arraycount(changebuf), NULL, 0);
		changes = 0;
	}

	return (&changebuf[changes++]);
}

bool
try_biltin(struct servtab *sep)
{
	for (size_t i = 0; i < __arraycount(biltins); i++) {
		if (biltins[i].bi_socktype == sep->se_socktype &&
		    strcmp(biltins[i].bi_service, sep->se_service) == 0) {
			sep->se_bi = &biltins[i];
			sep->se_wait = biltins[i].bi_wait;
			return true;
		}
	}
	return false;
}
