/*	$NetBSD: inetd.c,v 1.136 2021/09/03 21:02:04 rillig Exp $	*/

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
__RCSID("$NetBSD: inetd.c,v 1.136 2021/09/03 21:02:04 rillig Exp $");
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


#ifndef NO_RPC
#define RPC
#endif

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

#define	CNT_INTVL	((time_t)60)	/* servers in CNT_INTVL sec. */
#define	RETRYTIME	(60*10)		/* retry after bind or server fail */

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

static void	chargen_dg(int, struct servtab *);
static void	chargen_stream(int, struct servtab *);
static void	close_sep(struct servtab *);
static void	config(void);
static void	daytime_dg(int, struct servtab *);
static void	daytime_stream(int, struct servtab *);
static void	discard_dg(int, struct servtab *);
static void	discard_stream(int, struct servtab *);
static void	echo_dg(int, struct servtab *);
static void	echo_stream(int, struct servtab *);
static void	endconfig(void);
static struct servtab	*enter(struct servtab *);
static struct servtab	*getconfigent(char **);
__dead static void	goaway(void);
static void	machtime_dg(int, struct servtab *);
static void	machtime_stream(int, struct servtab *);
#ifdef DEBUG_ENABLE
static void	print_service(const char *, struct servtab *);
#endif
static void	reapchild(void);
static void	retry(void);
static void	run_service(int, struct servtab *, int);
static void	setup(struct servtab *);
static char	*skip(char **);
static void	tcpmux(int, struct servtab *);
__dead static void	usage(void);
static void	register_rpc(struct servtab *);
static void	unregister_rpc(struct servtab *);
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
static struct servtab	init_servtab(void);
static int	rl_process(struct servtab *, int);
static struct se_ip_list_node	*rl_add(struct servtab *, char *);
static void	rl_reset(struct servtab *, time_t);
static struct se_ip_list_node	*rl_try_get_ip(struct servtab *, char *);
static void	include_configs(char *);
static int	glob_error(const char *, int);
static void	read_glob_configs(char *);
static void	prepare_next_config(const char*);
static bool	is_same_service(const struct servtab *, const struct servtab *);
static char	*gen_file_pattern(const char *, const char *);
static bool	check_no_reinclude(const char *);
static void	include_matched_path(char *);
static void	purge_unchecked(void);
static void	config_root(void);
static void	clear_ip_list(struct servtab *);
static time_t	rl_time(void);
static void	rl_get_name(struct servtab *, int, char *);
static void	rl_drop_connection(struct servtab *, int);

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

	{ "tcpmux",	SOCK_STREAM,	true, false,	tcpmux },

	{ NULL, 0, false, false, NULL }
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
					"dl"
#else
					"d"
#endif
					   )) != -1)
		switch(ch) {
		case 'd':
			debug = true;
			options |= SO_DEBUG;
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

	if (!debug)
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
			if (debug)
				setsid();
		}
	}
	if (pid == 0) {
		run_service(ctrl, sep, dofork);
		if (dofork)
			exit(0);
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

size_t	line_number;

/*
 * Recursively merge loaded service definitions with any defined
 * in the current or included config files.
 */
static void
config(void)
{
	struct servtab *sep, *cp;
	/*
	 * Current position in line, used with key-values notation,
	 * saves cp across getconfigent calls.
	 */
	char *current_pos;
	size_t n;

	/* open config file from beginning */
	fconfig = fopen(CONFIG, "r");
	if(fconfig == NULL) {
		syslog(LOG_ERR, "%s: %m", CONFIG);
		return;
	}

	/* First call to nextline will advance line_number to 1 */
	line_number = 0;

	/* Start parsing at the beginning of the first line */
	current_pos = nextline(fconfig);

	while ((cp = getconfigent(&current_pos)) != NULL) {
		/* Find an already existing service definition */
		for (sep = servtab; sep != NULL; sep = sep->se_next)
			if (is_same_service(sep, cp))
				break;
		if (sep != NULL) {
			int i;

#define SWAP(type, a, b) {type c = a; a = b; b = c;}

			/*
			 * sep->se_wait may be holding the pid of a daemon
			 * that we're waiting for.  If so, don't overwrite
			 * it unless the config file explicitly says don't
			 * wait.
			 */
			if (cp->se_bi == 0 &&
			    (sep->se_wait == 1 || cp->se_wait == 0))
				sep->se_wait = cp->se_wait;
			SWAP(char *, sep->se_user, cp->se_user);
			SWAP(char *, sep->se_group, cp->se_group);
			SWAP(char *, sep->se_server, cp->se_server);
			for (i = 0; i < MAXARGV; i++)
				SWAP(char *, sep->se_argv[i], cp->se_argv[i]);
#ifdef IPSEC
			SWAP(char *, sep->se_policy, cp->se_policy);
#endif
			SWAP(service_type, cp->se_type, sep->se_type);
			SWAP(size_t, cp->se_service_max, sep->se_service_max);
			SWAP(size_t, cp->se_ip_max, sep->se_ip_max);
#undef SWAP
			if (isrpcservice(sep))
				unregister_rpc(sep);
			sep->se_rpcversl = cp->se_rpcversl;
			sep->se_rpcversh = cp->se_rpcversh;
			freeconfig(cp);
#ifdef DEBUG_ENABLE
			if (debug)
				print_service("REDO", sep);
#endif
		} else {
			sep = enter(cp);
#ifdef DEBUG_ENABLE
			if (debug)
				print_service("ADD ", sep);
#endif
		}
		sep->se_checked = 1;

		/*
		 * Remainder of config(void) checks validity of servtab options
		 * and sets up the service by setting up sockets (in setup(servtab)).
		 */
		switch (sep->se_family) {
		case AF_LOCAL:
			if (sep->se_fd != -1)
				break;
			n = strlen(sep->se_service);
			if (n >= sizeof(sep->se_ctrladdr_un.sun_path)) {
				syslog(LOG_ERR, "%s/%s: address too long",
				    sep->se_service, sep->se_proto);
				sep->se_checked = 0;
				continue;
			}
			(void)unlink(sep->se_service);
			strlcpy(sep->se_ctrladdr_un.sun_path,
			    sep->se_service, n + 1);
			sep->se_ctrladdr_un.sun_family = AF_LOCAL;
			sep->se_ctrladdr_size = (socklen_t)(n +
			    sizeof(sep->se_ctrladdr_un) -
			    sizeof(sep->se_ctrladdr_un.sun_path));
			if (!ISMUX(sep))
				setup(sep);
			break;
		case AF_INET:
#ifdef INET6
		case AF_INET6:
#endif
		    {
			struct addrinfo hints, *res;
			char *host;
			const char *port;
			int error;
			int s;

			/* check if the family is supported */
			s = socket(sep->se_family, SOCK_DGRAM, 0);
			if (s < 0) {
				syslog(LOG_WARNING,
				    "%s/%s: %s: the address family is not "
				    "supported by the kernel",
				    sep->se_service, sep->se_proto,
				    sep->se_hostaddr);
				sep->se_checked = false;
				continue;
			}
			close(s);

			memset(&hints, 0, sizeof(hints));
			hints.ai_family = sep->se_family;
			hints.ai_socktype = sep->se_socktype;
			hints.ai_flags = AI_PASSIVE;
			if (strcmp(sep->se_hostaddr, "*") == 0)
				host = NULL;
			else
				host = sep->se_hostaddr;
			if (isrpcservice(sep) || ISMUX(sep))
				port = "0";
			else
				port = sep->se_service;
			error = getaddrinfo(host, port, &hints, &res);
			if (error != 0) {
				if (error == EAI_SERVICE) {
					/* gai_strerror not friendly enough */
					syslog(LOG_WARNING, SERV_FMT ": "
					    "unknown service",
					    SERV_PARAMS(sep));
				} else {
					syslog(LOG_ERR, SERV_FMT ": %s: %s",
					    SERV_PARAMS(sep),
					    sep->se_hostaddr,
					    gai_strerror(error));
				}
				sep->se_checked = false;
				continue;
			}
			if (res->ai_next != NULL) {
				syslog(LOG_ERR,
					SERV_FMT ": %s: resolved to multiple addr",
				    SERV_PARAMS(sep),
				    sep->se_hostaddr);
				sep->se_checked = false;
				freeaddrinfo(res);
				continue;
			}
			memcpy(&sep->se_ctrladdr, res->ai_addr,
				res->ai_addrlen);
			if (ISMUX(sep)) {
				sep->se_fd = -1;
				freeaddrinfo(res);
				continue;
			}
			sep->se_ctrladdr_size = res->ai_addrlen;
			freeaddrinfo(res);
#ifdef RPC
			if (isrpcservice(sep)) {
				struct rpcent *rp;

				sep->se_rpcprog = atoi(sep->se_service);
				if (sep->se_rpcprog == 0) {
					rp = getrpcbyname(sep->se_service);
					if (rp == 0) {
						syslog(LOG_ERR,
						    SERV_FMT
						    ": unknown service",
						    SERV_PARAMS(sep));
						sep->se_checked = false;
						continue;
					}
					sep->se_rpcprog = rp->r_number;
				}
				if (sep->se_fd == -1 && !ISMUX(sep))
					setup(sep);
				if (sep->se_fd != -1)
					register_rpc(sep);
			} else
#endif
			{
				if (sep->se_fd >= 0)
					close_sep(sep);
				if (sep->se_fd == -1 && !ISMUX(sep))
					setup(sep);
			}
		    }
		}
	}
	endconfig();
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
	exit(0);
}

static void
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
static void
close_sep(struct servtab *sep)
{

	if (sep->se_fd >= 0) {
		(void) close(sep->se_fd);
		sep->se_fd = -1;
	}
	sep->se_count = 0;
	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		clear_ip_list(sep);
	}
}

static void
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

static void
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


static struct servtab *
enter(struct servtab *cp)
{
	struct servtab *sep;

	sep = malloc(sizeof (*sep));
	if (sep == NULL) {
		syslog(LOG_ERR, "Out of memory.");
		exit(EXIT_FAILURE);
	}
	*sep = *cp;
	sep->se_fd = -1;
	sep->se_rpcprog = -1;
	sep->se_next = servtab;
	servtab = sep;
	return (sep);
}

FILE	*fconfig;
/* Temporary storage for new servtab */
static struct	servtab serv;
/* Current line from current config file */
static char	line[LINE_MAX];
char    *defhost;
#ifdef IPSEC
char *policy;
#endif

static void
endconfig(void)
{
	if (fconfig != NULL) {
		(void) fclose(fconfig);
		fconfig = NULL;
	}
	if (defhost != NULL) {
		free(defhost);
		defhost = NULL;
	}

#ifdef IPSEC
	if (policy != NULL) {
		free(policy);
		policy = NULL;
	}
#endif

}

#define LOG_EARLY_ENDCONF() \
	ERR("Exiting %s early. Some services will be unavailable", CONFIG)

#define LOG_TOO_FEW_ARGS() \
	ERR("Expected more arguments")

/* Parse the next service and apply any directives, and returns it as servtab */
static struct servtab *
getconfigent(char **current_pos)
{
	struct servtab *sep = &serv;
	int argc, val;
	char *cp, *cp0, *arg, *buf0, *buf1, *sz0, *sz1;
	static char TCPMUX_TOKEN[] = "tcpmux/";
#define MUX_LEN		(sizeof(TCPMUX_TOKEN)-1)
	char *hostdelim;

	/*
	 * Pre-condition: current_pos points into line,
	 * line contains config line. Continue where the last getconfigent left off.
	 * Allows for multiple service definitions per line.
	 */
	cp = *current_pos;

	if (false) {
		/*
		 * Go to the next line, but only after attemting to read the current
		 * one! Keep reading until we find a valid definition or EOF.
		 */
more:
		cp = nextline(fconfig);
	}

	if (cp == NULL) {
		/* EOF or I/O error, let config() know to exit the file */
		return NULL;
	}

	/* Comments and IPsec policies */
	if (cp[0] == '#') {
#ifdef IPSEC
		/* lines starting with #@ is not a comment, but the policy */
		if (cp[1] == '@') {
			char *p;
			for (p = cp + 2; isspace((unsigned char)*p); p++)
				;
			if (*p == '\0') {
				if (policy)
					free(policy);
				policy = NULL;
			} else {
				if (ipsecsetup_test(p) < 0) {
					ERR("Invalid IPsec policy \"%s\"", p);
					LOG_EARLY_ENDCONF();
					/*
					* Stop reading the current config to prevent services
					* from being run without IPsec.
					*/
					return NULL;
				} else {
					if (policy)
						free(policy);
					policy = newstr(p);
				}
			}
		}
#endif

		goto more;
	}

	/* Parse next token: listen-addr/hostname, service-spec, .include */
	arg = skip(&cp);

	if (cp == NULL) {
		goto more;
	}

	if(arg[0] == '.') {
		if (strcmp(&arg[1], "include") == 0) {
			/* include directive */
			arg = skip(&cp);
			if(arg == NULL) {
				LOG_TOO_FEW_ARGS();
				return NULL;
			}
			include_configs(arg);
			goto more;
		} else {
			ERR("Unknown directive '%s'", &arg[1]);
			goto more;
		}
	}

	/* After this point, we might need to store data in a servtab */
	*sep = init_servtab();

	/* Check for a host name. */
	hostdelim = strrchr(arg, ':');
	if (hostdelim != NULL) {
		*hostdelim = '\0';
		if (arg[0] == '[' && hostdelim > arg && hostdelim[-1] == ']') {
			hostdelim[-1] = '\0';
			sep->se_hostaddr = newstr(arg + 1);
		} else
			sep->se_hostaddr = newstr(arg);
		arg = hostdelim + 1;
		/*
		 * If the line is of the form `host:', then just change the
		 * default host for the following lines.
		 */
		if (*arg == '\0') {
			arg = skip(&cp);
			if (cp == NULL) {
				free(defhost);
				defhost = sep->se_hostaddr;
				goto more;
			}
		}
	} else {
		/* No host address found, set it to NULL to indicate absence */
		sep->se_hostaddr = NULL;
	}
	if (strncmp(arg, TCPMUX_TOKEN, MUX_LEN) == 0) {
		char *c = arg + MUX_LEN;
		if (*c == '+') {
			sep->se_type = MUXPLUS_TYPE;
			c++;
		} else
			sep->se_type = MUX_TYPE;
		sep->se_service = newstr(c);
	} else {
		sep->se_service = newstr(arg);
		sep->se_type = NORM_TYPE;
	}

	DPRINTCONF("Found service definition '%s'", sep->se_service);

	/* on/off/socktype */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}

	/* Check for new v2 syntax */
	if (strcmp(arg, "on") == 0 || strncmp(arg, "on#", 3) == 0) {

		if (arg[2] == '#') {
			cp = nextline(fconfig);
		}

		switch(parse_syntax_v2(sep, &cp)) {
		case V2_SUCCESS:
			*current_pos = cp;
			return sep;
		case V2_SKIP:
			/* Skip invalid definitions, freeconfig is called in parse_v2.c */
			*current_pos = cp;
			freeconfig(sep);
			goto more;
		case V2_ERROR:
			/*
			 * Unrecoverable error, stop reading. freeconfig is called
			 * in parse_v2.c
			 */
			LOG_EARLY_ENDCONF();
			freeconfig(sep);
			return NULL;
		}
	} else if (strcmp(arg, "off") == 0 || strncmp(arg, "off#", 4) == 0) {

		if (arg[3] == '#') {
			cp = nextline(fconfig);
		}

		/* Parse syntax the same as with 'on', but ignore the result */
		switch(parse_syntax_v2(sep, &cp)) {
		case V2_SUCCESS:
		case V2_SKIP:
			*current_pos = cp;
			freeconfig(sep);
			goto more;
		case V2_ERROR:
			/* Unrecoverable error, stop reading */
			LOG_EARLY_ENDCONF();
			freeconfig(sep);
			return NULL;
		}
	} else {
		/* continue parsing v1 */
		parse_socktype(arg, sep);
		if (sep->se_socktype == SOCK_STREAM) {
			parse_accept_filter(arg, sep);
		}
		if (sep->se_hostaddr == NULL) {
			/* Set host to current default */
			sep->se_hostaddr = newstr(defhost);
		}
	}

	/* protocol */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}
	if (sep->se_type == NORM_TYPE &&
	    strncmp(arg, "faith/", strlen("faith/")) == 0) {
		arg += strlen("faith/");
		sep->se_type = FAITH_TYPE;
	}
	sep->se_proto = newstr(arg);

#define	MALFORMED(arg) \
do { \
	ERR("%s: malformed buffer size option `%s'", \
	    sep->se_service, (arg)); \
	freeconfig(sep); \
	goto more; \
} while (false)

#define	GETVAL(arg) \
do { \
	if (!isdigit((unsigned char)*(arg))) \
		MALFORMED(arg); \
	val = (int)strtol((arg), &cp0, 10); \
	if (cp0 != NULL) { \
		if (cp0[1] != '\0') \
			MALFORMED((arg)); \
		if (cp0[0] == 'k') \
			val *= 1024; \
		if (cp0[0] == 'm') \
			val *= 1024 * 1024; \
	} \
	if (val < 1) { \
		ERR("%s: invalid buffer size `%s'", \
		    sep->se_service, (arg)); \
		freeconfig(sep); \
		goto more; \
	} \
} while (false)

#define	ASSIGN(arg) \
do { \
	if (strcmp((arg), "sndbuf") == 0) \
		sep->se_sndbuf = val; \
	else if (strcmp((arg), "rcvbuf") == 0) \
		sep->se_rcvbuf = val; \
	else \
		MALFORMED((arg)); \
} while (false)

	/*
	 * Extract the send and receive buffer sizes before parsing
	 * the protocol.
	 */
	sep->se_sndbuf = sep->se_rcvbuf = 0;
	buf0 = buf1 = sz0 = sz1 = NULL;
	if ((buf0 = strchr(sep->se_proto, ',')) != NULL) {
		/* Not meaningful for Tcpmux services. */
		if (ISMUX(sep)) {
			ERR("%s: can't specify buffer sizes for "
			    "tcpmux services", sep->se_service);
			goto more;
		}

		/* Skip the , */
		*buf0++ = '\0';

		/* Check to see if another socket buffer size was specified. */
		if ((buf1 = strchr(buf0, ',')) != NULL) {
			/* Skip the , */
			*buf1++ = '\0';

			/* Make sure a 3rd one wasn't specified. */
			if (strchr(buf1, ',') != NULL) {
				ERR("%s: too many buffer sizes", sep->se_service);
				goto more;
			}

			/* Locate the size. */
			if ((sz1 = strchr(buf1, '=')) == NULL)
				MALFORMED(buf1);

			/* Skip the = */
			*sz1++ = '\0';
		}

		/* Locate the size. */
		if ((sz0 = strchr(buf0, '=')) == NULL)
			MALFORMED(buf0);

		/* Skip the = */
		*sz0++ = '\0';

		GETVAL(sz0);
		ASSIGN(buf0);

		if (buf1 != NULL) {
			GETVAL(sz1);
			ASSIGN(buf1);
		}
	}

#undef ASSIGN
#undef GETVAL
#undef MALFORMED

	if (parse_protocol(sep)) {
		freeconfig(sep);
		goto more;
	}

	/* wait/nowait:max */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}

	/* Rate limiting parsing */ {
		char *cp1;
		if ((cp1 = strchr(arg, ':')) == NULL)
			cp1 = strchr(arg, '.');
		if (cp1 != NULL) {
			int rstatus;
			*cp1++ = '\0';
			sep->se_service_max = (size_t)strtou(cp1, NULL, 10, 0,
			    SERVTAB_COUNT_MAX, &rstatus);

			if (rstatus != 0) {
				if (rstatus != ERANGE) {
					/* For compatibility with atoi parsing */
					sep->se_service_max = 0;
				}

				WRN("Improper \"max\" value '%s', "
				    "using '%zu' instead: %s",
				    cp1,
				    sep->se_service_max,
				    strerror(rstatus));
			}

		} else
			sep->se_service_max = TOOMANY;
	}
	if (parse_wait(sep, strcmp(arg, "wait") == 0)) {
		freeconfig(sep);
		goto more;
	}

	/* Parse user:group token */
	arg = skip(&cp);
	if(arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}
	char* separator = strchr(arg, ':');
	if (separator == NULL) {
		/* Backwards compatibility, allow dot instead of colon */
		separator = strchr(arg, '.');
	}

	if (separator == NULL) {
		/* Only user was specified */
		sep->se_group = NULL;
	} else {
		*separator = '\0';
		sep->se_group = newstr(separator + 1);
	}

	sep->se_user = newstr(arg);

	/* Parser server-program (path to binary or "internal") */
	arg = skip(&cp);
	if (arg == NULL) {
		LOG_TOO_FEW_ARGS();
		freeconfig(sep);
		goto more;
	}
	if (parse_server(sep, arg)) {
		freeconfig(sep);
		goto more;
	}

	argc = 0;
	for (arg = skip(&cp); cp != NULL; arg = skip(&cp)) {
		if (argc < MAXARGV)
			sep->se_argv[argc++] = newstr(arg);
	}
	while (argc <= MAXARGV)
		sep->se_argv[argc++] = NULL;
#ifdef IPSEC
	sep->se_policy = policy != NULL ? newstr(policy) : NULL;
#endif
	/* getconfigent read a positional service def, move to next line */
	*current_pos = nextline(fconfig);
	return (sep);
}

void
freeconfig(struct servtab *cp)
{
	int i;

	free(cp->se_hostaddr);
	free(cp->se_service);
	free(cp->se_proto);
	free(cp->se_user);
	free(cp->se_group);
	free(cp->se_server);
	for (i = 0; i < MAXARGV; i++)
		free(cp->se_argv[i]);
#ifdef IPSEC
	free(cp->se_policy);
#endif
}

/*
 * Get next token *in the current service definition* from config file.
 * Allows multi-line parse if single space or single tab-indented.
 * Things in quotes are considered single token.
 * Advances cp to next token.
 */
static char *
skip(char **cpp)
{
	char *cp = *cpp;
	char *start;
	char quote;

	if (*cpp == NULL)
		return (NULL);

again:
	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '\0') {
		int c;

		c = getc(fconfig);
		(void) ungetc(c, fconfig);
		if (c == ' ' || c == '\t')
			if ((cp = nextline(fconfig)) != NULL)
				goto again;
		*cpp = NULL;
		return (NULL);
	}
	start = cp;
	/* Parse shell-style quotes */
	quote = '\0';
	while (*cp != '\0' && (quote != '\0' || (*cp != ' ' && *cp != '\t'))) {
		if (*cp == '\'' || *cp == '"') {
			if (quote != '\0' && *cp != quote)
				cp++;
			else {
				if (quote != '\0')
					quote = '\0';
				else
					quote = *cp;
				memmove(cp, cp+1, strlen(cp));
			}
		} else
			cp++;
	}
	if (*cp != '\0')
		*cp++ = '\0';
	*cpp = cp;
	return (start);
}

char *
nextline(FILE *fd)
{
	char *cp;

	if (fgets(line, (int)sizeof(line), fd) == NULL) {
		if (ferror(fd) != 0) {
			ERR("Error when reading next line: %s", strerror(errno));
		}
		return NULL;
	}
	cp = strchr(line, '\n');
	if (cp != NULL)
		*cp = '\0';
	line_number++;
	return line;
}

char *
newstr(const char *cp)
{
	char *dp;
	if ((dp = strdup((cp != NULL) ? cp : "")) != NULL)
		return (dp);
	syslog(LOG_ERR, "strdup: %m");
	exit(EXIT_FAILURE);
	/*NOTREACHED*/
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
		;
}

/* ARGSUSED */
static void
echo_dg(int s, struct servtab *sep)	/* Echo service -- echo data back */
{
	char buffer[BUFSIZE];
	ssize_t i;
	socklen_t size;
	struct sockaddr_storage ss;
	struct sockaddr *sa;

	sa = (struct sockaddr *)(void *)&ss;
	size = sizeof(ss);
	if ((i = recvfrom(s, buffer, sizeof(buffer), 0, sa, &size)) < 0)
		return;
	if (port_good_dg(sa))
		(void) sendto(s, buffer, (size_t)i, 0, sa, size);
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
	struct sockaddr_storage ss;
	struct sockaddr *sa;
	static char *rs;
	size_t len;
	socklen_t size;
	char text[LINESIZ+2];

	if (endring == 0) {
		initring();
		rs = ring;
	}

	sa = (struct sockaddr *)(void *)&ss;
	size = sizeof(ss);
	if (recvfrom(s, text, sizeof(text), 0, sa, &size) < 0)
		return;

	if (!port_good_dg(sa))
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
	(void) sendto(s, text, sizeof(text), 0, sa, size);
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
	struct sockaddr_storage ss;
	struct sockaddr *sa;
	socklen_t size;

	sa = (struct sockaddr *)(void *)&ss;
	size = sizeof(ss);
	if (recvfrom(s, &result, sizeof(result), 0, sa, &size) < 0)
		return;
	if (!port_good_dg(sa))
		return;
	result = machtime();
	(void)sendto(s, &result, sizeof(result), 0, sa, size);
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
	struct sockaddr_storage ss;
	struct sockaddr *sa;
	socklen_t size;
	int len;

	clk = time((time_t *) 0);

	sa = (struct sockaddr *)(void *)&ss;
	size = sizeof(ss);
	if (recvfrom(s, buffer, sizeof(buffer), 0, sa, &size) < 0)
		return;
	if (!port_good_dg(sa))
		return;
	len = snprintf(buffer, sizeof buffer, "%.24s\r\n", ctime(&clk));
	(void) sendto(s, buffer, (size_t)len, 0, sa, size);
}

#ifdef DEBUG_ENABLE
/*
 * print_service:
 *	Dump relevant information to stderr
 */
static void
print_service(const char *action, struct servtab *sep)
{

	if (isrpcservice(sep))
		fprintf(stderr,
		    "%s: %s rpcprog=%d, rpcvers = %d/%d, proto=%s, wait.max=%d.%zu, user:group=%s:%s builtin=%lx server=%s"
#ifdef IPSEC
		    " policy=\"%s\""
#endif
		    "\n",
		    action, sep->se_service,
		    sep->se_rpcprog, sep->se_rpcversh, sep->se_rpcversl, sep->se_proto,
		    sep->se_wait, sep->se_service_max, sep->se_user, sep->se_group,
		    (long)sep->se_bi, sep->se_server
#ifdef IPSEC
		    , (sep->se_policy != NULL ? sep->se_policy : "")
#endif
		    );
	else
		fprintf(stderr,
		    "%s: %s:%s proto=%s%s, wait.max=%d.%zu, user:group=%s:%s builtin=%lx server=%s"
#ifdef IPSEC
		    " policy=%s"
#endif
		    "\n",
		    action, sep->se_hostaddr, sep->se_service,
		    sep->se_type == FAITH_TYPE ? "faith/" : "",
		    sep->se_proto,
		    sep->se_wait, sep->se_service_max, sep->se_user, sep->se_group,
		    (long)sep->se_bi, sep->se_server
#ifdef IPSEC
		    , (sep->se_policy != NULL ? sep->se_policy : "")
#endif
		    );
}
#endif

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

static void
config_root(void)
{
	struct servtab *sep;
	/* Uncheck services */
	for (sep = servtab; sep != NULL; sep = sep->se_next) {
		sep->se_checked = false;
	}
	defhost = newstr("*");
#ifdef IPSEC
	policy = NULL;
#endif
	fconfig = NULL;
	config();
	purge_unchecked();
}

static void
purge_unchecked(void)
{
	struct servtab *sep, **sepp = &servtab;
	int servtab_count = 0;
	while ((sep = *sepp) != NULL) {
		if (sep->se_checked) {
			sepp = &sep->se_next;
			servtab_count++;
			continue;
		}
		*sepp = sep->se_next;
		if (sep->se_fd >= 0)
			close_sep(sep);
		if (isrpcservice(sep))
			unregister_rpc(sep);
		if (sep->se_family == AF_LOCAL)
			(void)unlink(sep->se_service);
#ifdef DEBUG_ENABLE
		if (debug)
			print_service("FREE", sep);
#endif
		freeconfig(sep);
		free(sep);
	}
	DPRINTF("%d service(s) loaded.", servtab_count);
}

static bool
is_same_service(const struct servtab *sep, const struct servtab *cp)
{
	return
	    strcmp(sep->se_service, cp->se_service) == 0 &&
	    strcmp(sep->se_hostaddr, cp->se_hostaddr) == 0 &&
	    strcmp(sep->se_proto, cp->se_proto) == 0 &&
	    sep->se_family == cp->se_family &&
	    ISMUX(sep) == ISMUX(cp);
}

int
parse_protocol(struct servtab *sep)
{
	int val;

	if (strcmp(sep->se_proto, "unix") == 0) {
		sep->se_family = AF_LOCAL;
	} else {
		val = (int)strlen(sep->se_proto);
		if (val == 0) {
			ERR("%s: invalid protocol specified",
			    sep->se_service);
			return -1;
		}
		val = sep->se_proto[val - 1];
		switch (val) {
		case '4':	/*tcp4 or udp4*/
			sep->se_family = AF_INET;
			break;
#ifdef INET6
		case '6':	/*tcp6 or udp6*/
			sep->se_family = AF_INET6;
			break;
#endif
		default:
			/* Use 'default' IP version which is IPv4, may eventually be
			 * changed to AF_INET6 */
			sep->se_family = AF_INET;
			break;
		}
		if (strncmp(sep->se_proto, "rpc/", 4) == 0) {
#ifdef RPC
			char *cp1, *ccp;
			cp1 = strchr(sep->se_service, '/');
			if (cp1 == 0) {
				ERR("%s: no rpc version",
				    sep->se_service);
				return -1;
			}
			*cp1++ = '\0';
			sep->se_rpcversl = sep->se_rpcversh =
			    (int)strtol(cp1, &ccp, 0);
			if (ccp == cp1) {
		badafterall:
				ERR("%s/%s: bad rpc version",
				    sep->se_service, cp1);
				return -1;
			}
			if (*ccp == '-') {
				cp1 = ccp + 1;
				sep->se_rpcversh = (int)strtol(cp1, &ccp, 0);
				if (ccp == cp1)
					goto badafterall;
			}
#else
			ERR("%s: rpc services not supported",
			    sep->se_service);
			return -1;
#endif /* RPC */
		}
	}
	return 0;
}

int
parse_wait(struct servtab *sep, int wait)
{
	if (!ISMUX(sep)) {
		sep->se_wait = wait;
		return 0;
	}
	/*
	 * Silently enforce "nowait" for TCPMUX services since
	 * they don't have an assigned port to listen on.
	 */
	sep->se_wait = 0;

	if (strncmp(sep->se_proto, "tcp", 3)) {
		ERR("bad protocol for tcpmux service %s",
			sep->se_service);
		return -1;
	}
	if (sep->se_socktype != SOCK_STREAM) {
		ERR("bad socket type for tcpmux service %s",
		    sep->se_service);
		return -1;
	}
	return 0;
}

int
parse_server(struct servtab *sep, const char *arg){
	sep->se_server = newstr(arg);
	if (strcmp(sep->se_server, "internal") != 0) {
		sep->se_bi = NULL;
		return 0;
	}
	struct biltin *bi;

	for (bi = biltins; bi->bi_service; bi++)
		if (bi->bi_socktype == sep->se_socktype &&
		    strcmp(bi->bi_service, sep->se_service) == 0)
			break;
	if (bi->bi_service == NULL) {
		ERR("Internal service %s unknown",
		    sep->se_service);
		return -1;
	}
	sep->se_bi = bi;
	sep->se_wait = bi->bi_wait;
	return 0;
}

/* TODO test to make sure accept filter still works */
void
parse_accept_filter(char *arg, struct servtab *sep) {
	char *accf, *accf_arg;
	/* one and only one accept filter */
	accf = strchr(arg, ':');
	if (accf == NULL)
		return;
	if (accf != strrchr(arg, ':') || *(accf + 1) == '\0') {
		/* more than one ||  nothing beyond */
		sep->se_socktype = -1;
		return;
	}

	accf++;			/* skip delimiter */
	strlcpy(sep->se_accf.af_name, accf, sizeof(sep->se_accf.af_name));
	accf_arg = strchr(accf, ',');
	if (accf_arg == NULL)	/* zero or one arg, no more */
		return;

	if (strrchr(accf, ',') != accf_arg) {
		sep->se_socktype = -1;
	} else {
		accf_arg++;
		strlcpy(sep->se_accf.af_arg, accf_arg,
		    sizeof(sep->se_accf.af_arg));
	}
}

void
parse_socktype(char* arg, struct servtab* sep)
{
	/* stream socket may have an accept filter, only check first chars */
	if (strncmp(arg, "stream", sizeof("stream") - 1) == 0)
		sep->se_socktype = SOCK_STREAM;
	else if (strcmp(arg, "dgram") == 0)
		sep->se_socktype = SOCK_DGRAM;
	else if (strcmp(arg, "rdm") == 0)
		sep->se_socktype = SOCK_RDM;
	else if (strcmp(arg, "seqpacket") == 0)
		sep->se_socktype = SOCK_SEQPACKET;
	else if (strcmp(arg, "raw") == 0)
		sep->se_socktype = SOCK_RAW;
	else
		sep->se_socktype = -1;
}

static struct servtab
init_servtab(void)
{
	/* This does not set every field to default. See enter() as well */
	return (struct servtab) {
		/*
		 * Set se_max to non-zero so uninitialized value is not
	 	 * a valid value. Useful in v2 syntax parsing.
		 */
		.se_service_max = SERVTAB_UNSPEC_SIZE_T,
		.se_ip_max = SERVTAB_UNSPEC_SIZE_T,
		.se_wait = SERVTAB_UNSPEC_VAL,
		.se_socktype = SERVTAB_UNSPEC_VAL
		/* All other fields initialized to 0 or null */
	};
}

/* Include directives bookkeeping structure */
struct file_list {
	/* Absolute path used for checking for circular references */
	char *abs;
	/* Pointer to the absolute path of the parent config file,
	 * on the stack */
	struct file_list *next;
} *file_list_head;

static void
include_configs(char *pattern)
{
	/* Allocate global per-config state on the thread stack */
	const char* save_CONFIG;
	FILE	*save_fconfig;
	size_t	save_line_number;
	char    *save_defhost;
	struct	file_list new_file;
#ifdef IPSEC
	char *save_policy;
#endif

	/* Store current globals on the stack */
	save_CONFIG = CONFIG;
	save_fconfig = fconfig;
	save_line_number = line_number;
	save_defhost = defhost;
	new_file.abs = realpath(CONFIG, NULL);
	new_file.next = file_list_head;
#ifdef IPSEC
	save_policy = policy;
#endif
	/* Put new_file at the top of the config stack */
	file_list_head = &new_file;
	read_glob_configs(pattern);
	free(new_file.abs);
	/* Pop new_file off the stack */
	file_list_head = new_file.next;

	/* Restore global per-config state */
	CONFIG = save_CONFIG;
	fconfig = save_fconfig;
	line_number = save_line_number;
	defhost = save_defhost;
#ifdef IPSEC
	policy = save_policy;
#endif
}

static void
prepare_next_config(const char *file_name)
{
	/* Setup new state that is normally only done in main */
	CONFIG = file_name;

	/* Inherit default host and IPsec policy */
	defhost = newstr(defhost);

#ifdef IPSEC
	policy = (policy == NULL) ? NULL : newstr(policy);
#endif
}

static void
read_glob_configs(char *pattern) {
	glob_t results;
	char *full_pattern;
	int glob_result;
	full_pattern = gen_file_pattern(CONFIG, pattern);

	DPRINTCONF("Found include directive '%s'", full_pattern);

	glob_result = glob(full_pattern, GLOB_NOSORT, glob_error, &results);
	switch(glob_result) {
	case 0:
		/* No glob errors */
		break;
	case GLOB_ABORTED:
		ERR("Error while searching for include files");
		break;
	case GLOB_NOMATCH:
		/* It's fine if no files were matched. */
		DPRINTCONF("No files matched pattern '%s'", full_pattern);
		break;
	case GLOB_NOSPACE:
		ERR("Error when searching for include files: %s",
		    strerror(errno));
		break;
	default:
		ERR("Unknown glob(3) error %d", errno);
		break;
	}
	free(full_pattern);

	for (size_t i = 0; i < results.gl_pathc; i++) {
		include_matched_path(results.gl_pathv[i]);
	}

	globfree(&results);
}

static void
include_matched_path(char *glob_path)
{
	struct stat sb;
	char *tmp;

	if (lstat(glob_path, &sb) != 0) {
		ERR("Error calling stat on path '%s': %s", glob_path,
		    strerror(errno));
		return;
	}

	if (!S_ISREG(sb.st_mode) && !S_ISLNK(sb.st_mode)) {
		DPRINTCONF("'%s' is not a file.", glob_path);
		ERR("The matched path '%s' is not a regular file", glob_path);
		return;
	}

	DPRINTCONF("Include '%s'", glob_path);

	if (S_ISLNK(sb.st_mode)) {
		tmp = glob_path;
		glob_path = realpath(tmp, NULL);
	}

	/* Ensure the file is not being reincluded .*/
	if (check_no_reinclude(glob_path)) {
		prepare_next_config(glob_path);
		config();
	} else {
		DPRINTCONF("File '%s' already included in current include "
		    "chain", glob_path);
		WRN("Including file '%s' would cause a circular "
		    "dependency", glob_path);
	}

	if (S_ISLNK(sb.st_mode)) {
		free(glob_path);
		glob_path = tmp;
	}
}

static bool
check_no_reinclude(const char *glob_path)
{
	struct file_list *cur = file_list_head;
	char *abs_path = realpath(glob_path, NULL);

	if (abs_path == NULL) {
		ERR("Error checking real path for '%s': %s",
			glob_path, strerror(errno));
		return false;
	}

	DPRINTCONF("Absolute path '%s'", abs_path);

	for (cur = file_list_head; cur != NULL; cur = cur->next) {
		if (strcmp(cur->abs, abs_path) == 0) {
			/* file included more than once */
			/* TODO relative or abs path for logging error? */
			free(abs_path);
			return false;
		}
	}
	free(abs_path);
	return true;
}

/* Resolve the pattern relative to the config file the pattern is from  */
static char *
gen_file_pattern(const char *cur_config, const char *pattern)
{
	if (pattern[0] == '/') {
		/* Absolute paths don't need any normalization */
		return newstr(pattern);
	}

	/* pattern is relative */
	/* Find the end of the file's directory */
	size_t i, last = 0;
	for (i = 0; cur_config[i] != '\0'; i++) {
		if (cur_config[i] == '/') {
			last = i;
		}
	}

	if (last == 0) {
		/* cur_config is just a filename, pattern already correct */
		return newstr(pattern);
	}

	/* Relativize pattern to cur_config file's directory */
	char *full_pattern = malloc(last + 1 + strlen(pattern) + 1);
	if (full_pattern == NULL) {
		syslog(LOG_ERR, "Out of memory.");
		exit(EXIT_FAILURE);
	}
	memcpy(full_pattern, cur_config, last);
	full_pattern[last] = '/';
	strcpy(&full_pattern[last + 1], pattern);
	return full_pattern;
}

static int
glob_error(const char *path, int error)
{
	WRN("Error while resolving path '%s': %s", path, strerror(error));
	return 0;
}

/* Return 0 on allow, -1 if connection should be blocked */
static int
rl_process(struct servtab *sep, int ctrl)
{
	struct se_ip_list_node *node;
	time_t now = 0; /* 0 prevents GCC from complaining */
	bool istimevalid = false;
	char hbuf[NI_MAXHOST];

	DPRINTF(SERV_FMT ": processing rate-limiting",
	    SERV_PARAMS(sep));
	DPRINTF(SERV_FMT ": se_service_max "
	    "%zu and se_count %zu", SERV_PARAMS(sep),
	    sep->se_service_max, sep->se_count);

	/* se_count is incremented if rl_process will return 0 */
	if (sep->se_count == 0) {
		now = rl_time();
		sep->se_time = now;
		istimevalid = true;
	}

	if (sep->se_count >= sep->se_service_max) {
		if(!istimevalid) {
			now = rl_time();
			istimevalid = true;
		}

		if (now - sep->se_time > CNT_INTVL) {
			rl_reset(sep, now);
		} else {
			syslog(LOG_ERR,
			    SERV_FMT ": max spawn rate (%zu in %ji seconds) "
			    "already met, closing until end of timeout in "
			    "%ju seconds",
			    SERV_PARAMS(sep),
			    sep->se_service_max,
			    (intmax_t)CNT_INTVL,
			    (uintmax_t)RETRYTIME);

			DPRINTF(SERV_FMT ": service not started",
			    SERV_PARAMS(sep));

			rl_drop_connection(sep, ctrl);

			/* Close the server for 10 minutes */
			close_sep(sep);
			if (!timingout) {
				timingout = true;
				alarm(RETRYTIME);
			}

			return -1;
		}
	}

	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		rl_get_name(sep, ctrl, hbuf);
		node = rl_try_get_ip(sep, hbuf);
		if(node == NULL) {
			node = rl_add(sep, hbuf);
		}

		DPRINTF(
		    SERV_FMT ": se_ip_max %zu and ip_count %zu",
		    SERV_PARAMS(sep), sep->se_ip_max, node->count);

		if (node->count >= sep->se_ip_max) {
			if (!istimevalid) {
				/*
				 * Only get the clock time if we didn't
				 * already
				 */
				now = rl_time();
				istimevalid = true;
			}

			if (now - sep->se_time > CNT_INTVL) {
				rl_reset(sep, now);
				node = rl_add(sep, hbuf);
			} else {
				if (debug && node->count == sep->se_ip_max) {
					/*
					 * Only log first failed request to
					 * prevent DoS attack writing to system
					 * log
					 */
					syslog(LOG_ERR, SERV_FMT
					    ": max ip spawn rate (%zu in "
					    "%ji seconds) for "
					    "%." TOSTRING(NI_MAXHOST) "s "
					    "already met; service not started",
					    SERV_PARAMS(sep),
					    sep->se_ip_max,
					    (intmax_t)CNT_INTVL,
					    node->address);
				}

				DPRINTF(SERV_FMT ": service not started",
   				    SERV_PARAMS(sep));

				rl_drop_connection(sep, ctrl);
				/*
				 * Increment so debug-syslog message will
				 * trigger only once
				 */
				node->count++;
				return -1;
			}
		}
		node->count++;
	}

	DPRINTF(SERV_FMT ": running service ", SERV_PARAMS(sep));

	sep->se_count++;
	return 0;
}

/* Get the remote's IP address in textual form into hbuf of size NI_MAXHOST */
static void
rl_get_name(struct servtab *sep, int ctrl, char *hbuf)
{
	struct sockaddr_storage addr;
	socklen_t len = sizeof(struct sockaddr_storage);
	switch (sep->se_socktype) {
	case SOCK_STREAM:
		if (getpeername(ctrl, (struct sockaddr *)&addr, &len) != 0) {
			/* error, log it and skip ip rate limiting */
			syslog(LOG_ERR,
				SERV_FMT " failed to get peer name of the "
				"connection", SERV_PARAMS(sep));
			exit(EXIT_FAILURE);
		}
		break;
	case SOCK_DGRAM: {
		struct msghdr header = {
			.msg_name = &addr,
			.msg_namelen = sizeof(struct sockaddr_storage),
			/* scatter/gather and control info is null */
		};
		ssize_t count;

		/* Peek so service can still get the packet */
		count = recvmsg(ctrl, &header, MSG_PEEK);
		if (count == -1) {
			syslog(LOG_ERR,
			    "failed to get dgram source address: %s; exiting",
			    strerror(errno));
			exit(EXIT_FAILURE);
		}
		break;
	}
	default:
		DPRINTF(SERV_FMT ": ip_max rate limiting not supported for "
		    "socktype", SERV_PARAMS(sep));
		syslog(LOG_ERR, SERV_FMT
		    ": ip_max rate limiting not supported for socktype",
		    SERV_PARAMS(sep));
		exit(EXIT_FAILURE);
	}

	if (getnameinfo((struct sockaddr *)&addr,
		addr.ss_len, hbuf,
		NI_MAXHOST, NULL, 0, NI_NUMERICHOST)) {
		/* error, log it and skip ip rate limiting */
		syslog(LOG_ERR,
		    SERV_FMT ": failed to get name info of the incoming "
		    "connection; exiting",
		    SERV_PARAMS(sep));
		exit(EXIT_FAILURE);
	}
}

static void
rl_drop_connection(struct servtab *sep, int ctrl)
{

	if (sep->se_wait == 0 && sep->se_socktype == SOCK_STREAM) {
		/*
		 * If the fd isn't a listen socket,
		 * close the individual connection too.
		 */
		close(ctrl);
		return;
	}
	if (sep->se_socktype != SOCK_DGRAM) {
		return;
	}
	/*
	 * Drop the single datagram the service would have
	 * consumed if nowait. If this is a wait service, this
	 * will consume 1 datagram, and further received packets
	 * will be removed in the same way.
	 */
	struct msghdr header = {
		/* All fields null, just consume one message */
	};
	ssize_t count;

	count = recvmsg(ctrl, &header, 0);
	if (count == -1) {
		syslog(LOG_ERR,
		    SERV_FMT ": failed to consume nowait dgram: %s",
		    SERV_PARAMS(sep), strerror(errno));
		exit(EXIT_FAILURE);
	}
	DPRINTF(SERV_FMT ": dropped dgram message",
	    SERV_PARAMS(sep));
}

static time_t
rl_time(void)
{
	struct timespec time;
	if(clock_gettime(CLOCK_MONOTONIC, &time) == -1) {
		syslog(LOG_ERR, "clock_gettime for rate limiting failed: %s; "
		    "exiting", strerror(errno));
		/* Exit inetd if rate limiting fails */
		exit(EXIT_FAILURE);
	}
	return time.tv_sec;
}

static struct se_ip_list_node*
rl_add(struct servtab *sep, char* ip)
{
	DPRINTF(
	    SERV_FMT ": add ip %s to rate limiting tracking",
	    SERV_PARAMS(sep), ip);

	/*
	 * TODO memory could be saved by using a variable length malloc
	 * with only the length of ip instead of the existing address field
	 * NI_MAXHOST in length.
	 */
	struct se_ip_list_node* temp = malloc(sizeof(*temp));
	if (temp == NULL) {
		syslog(LOG_ERR, "Out of memory.");
		exit(EXIT_FAILURE);
	}
	temp->count = 0;
	temp->next = NULL;
	strlcpy(temp->address, ip, sizeof(temp->address));

	if (sep->se_ip_list_head == NULL) {
		/* List empty, insert as head */
		sep->se_ip_list_head = temp;
	} else {
		/* List not empty, insert as head, point next to prev head */
		temp->next = sep->se_ip_list_head;
		sep->se_ip_list_head = temp;
	}

	return temp;
}

static void
rl_reset(struct servtab *sep, time_t now)
{
	DPRINTF(SERV_FMT ": %ji seconds passed; resetting rate limiting ",
	    SERV_PARAMS(sep), (intmax_t)(now - sep->se_time));

	sep->se_count = 0;
	sep->se_time = now;
	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		clear_ip_list(sep);
	}
}

static void
clear_ip_list(struct servtab *sep) {
	struct se_ip_list_node *curr, *next;
	curr = sep->se_ip_list_head;

	while (curr != NULL) {
		next = curr->next;
		free(curr);
		curr = next;
	}
	sep->se_ip_list_head = NULL;
}

static struct se_ip_list_node *
rl_try_get_ip(struct servtab *sep, char *ip)
{
	struct se_ip_list_node *curr;

	DPRINTF(
	    SERV_FMT ": look up ip %s for ip_max rate limiting",
	    SERV_PARAMS(sep), ip);

	for (curr = sep->se_ip_list_head; curr != NULL; curr = curr->next) {
		if (!strncmp(curr->address, ip, NI_MAXHOST)) {
			/* IP addr match */
			return curr;
		}
	}
	return NULL;
}
