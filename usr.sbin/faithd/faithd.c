/*	$NetBSD: faithd.c,v 1.35.2.1 2014/05/22 11:43:03 yamt Exp $	*/
/*	$KAME: faithd.c,v 1.62 2003/08/19 21:20:33 itojun Exp $	*/

/*
 * Copyright (C) 1997 and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * User level translator from IPv6 to IPv4.
 *
 * Usage: faithd [<port> <progpath> <arg1(progname)> <arg2> ...]
 *   e.g. faithd telnet /usr/local/v6/sbin/telnetd telnetd
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>

#include <net/if_types.h>
#ifdef IFT_FAITH
# define USE_ROUTE
# include <net/if.h>
# include <net/route.h>
# include <net/if_dl.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "faithd.h"
#include "prefix.h"

char *serverpath = NULL;
char *serverarg[MAXARGV + 1];
char logname[BUFSIZ];
char procname[BUFSIZ];
struct myaddrs {
	struct myaddrs *next;
	struct sockaddr *addr;
};
struct myaddrs *myaddrs = NULL;
static const char *service;
#ifdef USE_ROUTE
static int sockfd = 0;
#endif
int dflag = 0;
static int pflag = 0;
static int inetd = 0;
static char *configfile = NULL;

static int inetd_main(int, char **);
static int daemon_main(int, char **);
static void play_service(int) __dead;
static void play_child(int, struct sockaddr *) __dead;
static int faith_prefix(struct sockaddr *);
static int map6to4(struct sockaddr_in6 *, struct sockaddr_in *);
static void sig_child(int);
static void sig_terminate(int) __dead;
static void start_daemon(void);
static void exit_stderr(const char *, ...) __printflike(1, 2) __dead;
static void grab_myaddrs(void);
static void free_myaddrs(void);
static void update_myaddrs(void);
static void usage(void) __dead;

int
main(int argc, char **argv)
{

	/*
	 * Initializing stuff
	 */

	setprogname(argv[0]);

	if (strcmp(getprogname(), "faithd") != 0) {
		inetd = 1;
		return inetd_main(argc, argv);
	} else
		return daemon_main(argc, argv);
}

static int
inetd_main(int argc, char **argv)
{
	char path[MAXPATHLEN];
	struct sockaddr_storage me;
	struct sockaddr_storage from;
	socklen_t melen, fromlen;
	int i;
	int error;
	const int on = 1;
	char sbuf[NI_MAXSERV], snum[NI_MAXSERV];

	if (config_load(configfile) < 0 && configfile) {
		exit_failure("could not load config file");
		/*NOTREACHED*/
	}

	if (strrchr(argv[0], '/') == NULL)
		(void)snprintf(path, sizeof(path), "%s/%s", DEFAULT_DIR,
		    argv[0]);
	else
		(void)snprintf(path, sizeof(path), "%s", argv[0]);

#ifdef USE_ROUTE
	grab_myaddrs();

	sockfd = socket(PF_ROUTE, SOCK_RAW, PF_UNSPEC);
	if (sockfd < 0) {
		exit_failure("socket(PF_ROUTE): %s", strerror(errno));
		/*NOTREACHED*/
	}
#endif

	melen = sizeof(me);
	if (getsockname(STDIN_FILENO, (void *)&me, &melen) == -1) {
		exit_failure("getsockname: %s", strerror(errno));
		/*NOTREACHED*/
	}
	fromlen = sizeof(from);
	if (getpeername(STDIN_FILENO, (void *)&from, &fromlen) == -1) {
		exit_failure("getpeername: %s", strerror(errno));
		/*NOTREACHED*/
	}
	if (getnameinfo((void *)&me, melen, NULL, 0,
	    sbuf, (socklen_t)sizeof(sbuf), NI_NUMERICHOST) == 0)
		service = sbuf;
	else
		service = DEFAULT_PORT_NAME;
	if (getnameinfo((void *)&me, melen, NULL, 0,
	    snum, (socklen_t)sizeof(snum), NI_NUMERICHOST) != 0)
		(void)snprintf(snum, sizeof(snum), "?");

	(void)snprintf(logname, sizeof(logname), "faithd %s", snum);
	(void)snprintf(procname, sizeof(procname), "accepting port %s", snum);
	openlog(logname, LOG_PID | LOG_NOWAIT, LOG_DAEMON);

	if (argc >= MAXARGV) {
		exit_failure("too many arguments");
		/*NOTREACHED*/
	}
	serverarg[0] = serverpath = path;
	for (i = 1; i < argc; i++)
		serverarg[i] = argv[i];
	serverarg[i] = NULL;

	error = setsockopt(STDIN_FILENO, SOL_SOCKET, SO_OOBINLINE, &on,
	    (socklen_t)sizeof(on));
	if (error < 0) {
		exit_failure("setsockopt(SO_OOBINLINE): %s", strerror(errno));
		/*NOTREACHED*/
	}

	play_child(STDIN_FILENO, (void *)&from);
	exit_failure("should not reach here");
	return 0;	/*dummy!*/
}

static int
daemon_main(int argc, char **argv)
{
	struct addrinfo hints, *res;
	int s_wld, error, i, serverargc, on = 1;
	int family = AF_INET6;
	int c;

	while ((c = getopt(argc, argv, "df:p")) != -1) {
		switch (c) {
		case 'd':
			dflag++;
			break;
		case 'f':
			configfile = optarg;
			break;
		case 'p':
			pflag++;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (config_load(configfile) < 0 && configfile) {
		exit_failure("could not load config file");
		/*NOTREACHED*/
	}


#ifdef USE_ROUTE
	grab_myaddrs();
#endif

	switch (argc) {
	case 0:
		usage();
		/*NOTREACHED*/
	default:
		serverargc = argc - NUMARG;
		if (serverargc >= MAXARGV)
			exit_stderr("too many arguments");

		serverpath = strdup(argv[NUMPRG]);
		if (!serverpath)
			exit_stderr("not enough core");
		for (i = 0; i < serverargc; i++) {
			serverarg[i] = strdup(argv[i + NUMARG]);
			if (!serverarg[i])
				exit_stderr("not enough core");
		}
		serverarg[i] = NULL;
		/*FALLTHROUGH*/
	case 1:	/* no local service */
		service = argv[NUMPRT];
		break;
	}

	start_daemon();

	/*
	 * Opening wild card socket for this service.
	 */

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;	/* SCTP? */
	error = getaddrinfo(NULL, service, &hints, &res);
	if (error)
		exit_failure("getaddrinfo: %s", gai_strerror(error));

	s_wld = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (s_wld == -1)
		exit_failure("socket: %s", strerror(errno));

#ifdef IPV6_FAITH
	if (res->ai_family == AF_INET6) {
		error = setsockopt(s_wld, IPPROTO_IPV6, IPV6_FAITH, &on,
		    (socklen_t)sizeof(on));
		if (error == -1)
			exit_failure("setsockopt(IPV6_FAITH): %s",
			    strerror(errno));
	}
#endif

	error = setsockopt(s_wld, SOL_SOCKET, SO_REUSEADDR, &on,
	    (socklen_t)sizeof(on));
	if (error == -1)
		exit_failure("setsockopt(SO_REUSEADDR): %s", strerror(errno));
	
	error = setsockopt(s_wld, SOL_SOCKET, SO_OOBINLINE, &on,
	    (socklen_t)sizeof(on));
	if (error == -1)
		exit_failure("setsockopt(SO_OOBINLINE): %s", strerror(errno));

#ifdef IPV6_V6ONLY
	error = setsockopt(s_wld, IPPROTO_IPV6, IPV6_V6ONLY, &on,
	    (socklen_t)sizeof(on));
	if (error == -1)
		exit_failure("setsockopt(IPV6_V6ONLY): %s", strerror(errno));
#endif

	error = bind(s_wld, (struct sockaddr *)res->ai_addr, res->ai_addrlen);
	if (error == -1)
		exit_failure("bind: %s", strerror(errno));

	error = listen(s_wld, 5);
	if (error == -1)
		exit_failure("listen: %s", strerror(errno));

#ifdef USE_ROUTE
	sockfd = socket(PF_ROUTE, SOCK_RAW, PF_UNSPEC);
	if (sockfd < 0) {
		exit_failure("socket(PF_ROUTE): %s", strerror(errno));
		/*NOTREACHED*/
	}
#endif

	/*
	 * Everything is OK.
	 */

	(void)snprintf(logname, sizeof(logname), "faithd %s", service);
	(void)snprintf(procname, sizeof(procname), "accepting port %s", service);
	openlog(logname, LOG_PID | LOG_NOWAIT, LOG_DAEMON);
	syslog(LOG_INFO, "Staring faith daemon for %s port", service);

	play_service(s_wld);
	return 1;
}

static void
play_service(int s_wld)
{
	struct sockaddr_storage srcaddr;
	socklen_t len;
	int s_src;
	pid_t child_pid;
	struct pollfd pfd[2];
	int error;

	/*
	 * Wait, accept, fork, faith....
	 */
again:
	setproctitle("%s", procname);

	pfd[0].fd = s_wld;
	pfd[0].events = POLLIN;
	pfd[1].fd = -1;
	pfd[1].revents = 0;
#ifdef USE_ROUTE
	if (sockfd) {
		pfd[1].fd = sockfd;
		pfd[1].events = POLLIN;
	}
#endif

	error = poll(pfd, (unsigned int)(sizeof(pfd) / sizeof(pfd[0])), INFTIM);
	if (error < 0) {
		if (errno == EINTR)
			goto again;
		exit_failure("select: %s", strerror(errno));
		/*NOTREACHED*/
	}

#ifdef USE_ROUTE
	if (pfd[1].revents & POLLIN)
	{
		update_myaddrs();
	}
#endif
	if (pfd[0].revents & POLLIN)
	{
		len = sizeof(srcaddr);
		s_src = accept(s_wld, (void *)&srcaddr, &len);
		if (s_src < 0) {
			if (errno == ECONNABORTED)
				goto again;
			exit_failure("socket: %s", strerror(errno));
			/*NOTREACHED*/
		}
		if (srcaddr.ss_family == AF_INET6 &&
		    IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)(void *)&srcaddr)->sin6_addr)) {
			(void)close(s_src);
			syslog(LOG_ERR, "connection from IPv4 mapped address?");
			goto again;
		}

		child_pid = fork();

		if (child_pid == 0) {
			/* child process */
			(void)close(s_wld);
			closelog();
			openlog(logname, LOG_PID | LOG_NOWAIT, LOG_DAEMON);
			play_child(s_src, (void *)&srcaddr);
			exit_failure("should never reach here");
			/*NOTREACHED*/
		} else {
			/* parent process */
			(void)close(s_src);
			if (child_pid == -1)
				syslog(LOG_ERR, "can't fork");
		}
	}
	goto again;
}

static void
play_child(int s_src, struct sockaddr *srcaddr)
{
	struct sockaddr_storage dstaddr6;
	struct sockaddr_storage dstaddr4;
	char src[NI_MAXHOST];
	char dst6[NI_MAXHOST];
	char dst4[NI_MAXHOST];
	socklen_t len = sizeof(dstaddr6);
	int s_dst, error, hport, nresvport, on = 1;
	struct timeval tv;
	struct sockaddr *sa4;
	const struct config *conf;
	
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	(void)getnameinfo(srcaddr, (socklen_t)srcaddr->sa_len,
	    src, (socklen_t)sizeof(src), NULL, 0, NI_NUMERICHOST);
	syslog(LOG_INFO, "accepted a client from %s", src);

	error = getsockname(s_src, (void *)&dstaddr6, &len);
	if (error == -1) {
		exit_failure("getsockname: %s", strerror(errno));
		/*NOTREACHED*/
	}

	(void)getnameinfo((void *)&dstaddr6, len,
	    dst6, (socklen_t)sizeof(dst6), NULL, 0, NI_NUMERICHOST);
	syslog(LOG_INFO, "the client is connecting to %s", dst6);
	
	if (!faith_prefix((void *)&dstaddr6)) {
		if (serverpath) {
			/*
			 * Local service
			 */
			syslog(LOG_INFO, "executing local %s", serverpath);
			if (!inetd) {
				(void)dup2(s_src, 0);
				(void)close(s_src);
				(void)dup2(0, 1);
				(void)dup2(0, 2);
			}
			(void)execv(serverpath, serverarg);
			syslog(LOG_ERR, "execv %s: %s", serverpath,
			    strerror(errno));
			_exit(EXIT_FAILURE);
		} else {
			(void)close(s_src);
			exit_success("no local service for %s", service);
		}
	}

	/*
	 * Act as a translator
	 */

	switch (((struct sockaddr *)(void *)&dstaddr6)->sa_family) {
	case AF_INET6:
		if (!map6to4((struct sockaddr_in6 *)(void *)&dstaddr6,
		    (struct sockaddr_in *)(void *)&dstaddr4)) {
			(void)close(s_src);
			exit_failure("map6to4 failed");
			/*NOTREACHED*/
		}
		syslog(LOG_INFO, "translating from v6 to v4");
		break;
	default:
		(void)close(s_src);
		exit_failure("family not supported");
		/*NOTREACHED*/
	}

	sa4 = (void *)&dstaddr4;
	(void)getnameinfo(sa4, (socklen_t)sa4->sa_len,
	    dst4, (socklen_t)sizeof(dst4), NULL, 0, NI_NUMERICHOST);

	conf = config_match(srcaddr, sa4);
	if (!conf || !conf->permit) {
		(void)close(s_src);
		if (conf) {
			exit_failure("translation to %s not permitted for %s",
			    dst4, prefix_string(&conf->match));
			/*NOTREACHED*/
		} else {
			exit_failure("translation to %s not permitted", dst4);
			/*NOTREACHED*/
		}
	}

	syslog(LOG_INFO, "the translator is connecting to %s", dst4);

	setproctitle("port %s, %s -> %s", service, src, dst4);

	if (sa4->sa_family == AF_INET6)
		hport = ntohs(((struct sockaddr_in6 *)(void *)&dstaddr4)->sin6_port);
	else /* AF_INET */
		hport = ntohs(((struct sockaddr_in *)(void *)&dstaddr4)->sin_port);

	if (pflag)
		s_dst = rresvport_af(&nresvport, sa4->sa_family);
	else
		s_dst = socket(sa4->sa_family, SOCK_STREAM, 0);
	if (s_dst < 0) {
		exit_failure("socket: %s", strerror(errno));
		/*NOTREACHED*/
	}

	if (conf->src.a.ss_family) {
		if (bind(s_dst, (const void*)&conf->src.a,
		    (socklen_t)conf->src.a.ss_len) < 0) {
			exit_failure("bind: %s", strerror(errno));
			/*NOTREACHED*/
		}
	}

	error = setsockopt(s_dst, SOL_SOCKET, SO_OOBINLINE, &on,
	    (socklen_t)sizeof(on));
	if (error < 0) {
		exit_failure("setsockopt(SO_OOBINLINE): %s", strerror(errno));
		/*NOTREACHED*/
	}

	error = setsockopt(s_src, SOL_SOCKET, SO_SNDTIMEO, &tv,
	    (socklen_t)sizeof(tv));
	if (error < 0) {
		exit_failure("setsockopt(SO_SNDTIMEO): %s", strerror(errno));
		/*NOTREACHED*/
	}
	error = setsockopt(s_dst, SOL_SOCKET, SO_SNDTIMEO, &tv,
	    (socklen_t)sizeof(tv));
	if (error < 0) {
		exit_failure("setsockopt(SO_SNDTIMEO): %s", strerror(errno));
		/*NOTREACHED*/
	}

	error = connect(s_dst, sa4, (socklen_t)sa4->sa_len);
	if (error < 0) {
		exit_failure("connect: %s", strerror(errno));
		/*NOTREACHED*/
	}

	switch (hport) {
	case FTP_PORT:
		ftp_relay(s_src, s_dst);
		break;
	default:
		tcp_relay(s_src, s_dst, service);
		break;
	}

	/* NOTREACHED */
}

/* 0: non faith, 1: faith */
static int
faith_prefix(struct sockaddr *dst)
{
#ifndef USE_ROUTE
	int mib[4], size;
	struct in6_addr faith_prefix;
	struct sockaddr_in6 *dst6 = (struct sockaddr_in *)dst;

	if (dst->sa_family != AF_INET6)
		return 0;

	mib[0] = CTL_NET;
	mib[1] = PF_INET6;
	mib[2] = IPPROTO_IPV6;
	mib[3] = IPV6CTL_FAITH_PREFIX;
	size = sizeof(struct in6_addr);
	if (sysctl(mib, 4, &faith_prefix, &size, NULL, 0) < 0) {
		exit_failure("sysctl: %s", strerror(errno));
		/*NOTREACHED*/
	}

	return memcmp(dst, &faith_prefix,
	    sizeof(struct in6_addr) - sizeof(struct in_addr)) == 0;
#else
	struct myaddrs *p;
	struct sockaddr_in6 *sin6;
	struct sockaddr_in *sin4;
	struct sockaddr_in6 *dst6;
	struct sockaddr_in *dst4;
	struct sockaddr_in dstmap;

	dst6 = (void *)dst;
	if (dst->sa_family == AF_INET6
	 && IN6_IS_ADDR_V4MAPPED(&dst6->sin6_addr)) {
		/* ugly... */
		memset(&dstmap, 0, sizeof(dstmap));
		dstmap.sin_family = AF_INET;
		dstmap.sin_len = sizeof(dstmap);
		memcpy(&dstmap.sin_addr, &dst6->sin6_addr.s6_addr[12],
			sizeof(dstmap.sin_addr));
		dst = (void *)&dstmap;
	}

	dst6 = (void *)dst;
	dst4 = (void *)dst;

	for (p = myaddrs; p; p = p->next) {
		sin6 = (void *)p->addr;
		sin4 = (void *)p->addr;

		if (p->addr->sa_len != dst->sa_len
		 || p->addr->sa_family != dst->sa_family)
			continue;

		switch (dst->sa_family) {
		case AF_INET6:
			if (sin6->sin6_scope_id == dst6->sin6_scope_id
			 && IN6_ARE_ADDR_EQUAL(&sin6->sin6_addr, &dst6->sin6_addr))
				return 0;
			break;
		case AF_INET:
			if (sin4->sin_addr.s_addr == dst4->sin_addr.s_addr)
				return 0;
			break;
		}
	}
	return 1;
#endif
}

/* 0: non faith, 1: faith */
static int
map6to4(struct sockaddr_in6 *dst6, struct sockaddr_in *dst4)
{
	memset(dst4, 0, sizeof(*dst4));
	dst4->sin_len = sizeof(*dst4);
	dst4->sin_family = AF_INET;
	dst4->sin_port = dst6->sin6_port;
	memcpy(&dst4->sin_addr, &dst6->sin6_addr.s6_addr[12],
		sizeof(dst4->sin_addr));

	if (dst4->sin_addr.s_addr == INADDR_ANY
	 || dst4->sin_addr.s_addr == INADDR_BROADCAST
	 || IN_MULTICAST(ntohl(dst4->sin_addr.s_addr)))
		return 0;

	return 1;
}


static void
/*ARGSUSED*/
sig_child(int sig)
{
	int status;
	pid_t pid;

	while ((pid = wait3(&status, WNOHANG, (struct rusage *)0)) > 0)
		if (WEXITSTATUS(status))
			syslog(LOG_WARNING, "child %ld exit status 0x%x",
			    (long)pid, status);
}

static void
/*ARGSUSED*/
sig_terminate(int sig)
{
	syslog(LOG_INFO, "Terminating faith daemon");	
	exit(EXIT_SUCCESS);
}

static void
start_daemon(void)
{
#ifdef SA_NOCLDWAIT
	struct sigaction sa;
#endif

	if (daemon(0, 0) == -1)
		exit_stderr("daemon: %s", strerror(errno));

#ifdef SA_NOCLDWAIT
	(void)memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_child;
	sa.sa_flags = SA_NOCLDWAIT;
	(void)sigemptyset(&sa.sa_mask);
	(void)sigaction(SIGCHLD, &sa, (struct sigaction *)0);
#else
	if (signal(SIGCHLD, sig_child) == SIG_ERR) {
		exit_failure("signal CHLD: %s", strerror(errno));
		/*NOTREACHED*/
	}
#endif

	if (signal(SIGTERM, sig_terminate) == SIG_ERR) {
		exit_failure("signal TERM: %s", strerror(errno));
		/*NOTREACHED*/
	}
}

static void
exit_stderr(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)fprintf(stderr, "%s: ", getprogname());
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
exit_failure(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
exit_success(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog(LOG_INFO, fmt, ap);
	va_end(ap);
	exit(EXIT_SUCCESS);
}

#ifdef USE_ROUTE
static void
grab_myaddrs(void)
{
	struct ifaddrs *ifap, *ifa;
	struct myaddrs *p;

	if (getifaddrs(&ifap) != 0) {
		exit_failure("getifaddrs");
		/*NOTREACHED*/
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		switch (ifa->ifa_addr->sa_family) {
		case AF_INET:
		case AF_INET6:
			break;
		default:
			continue;
		}

		p = (struct myaddrs *)malloc(sizeof(struct myaddrs) +
		    ifa->ifa_addr->sa_len);
		if (!p) {
			exit_failure("not enough core");
			/*NOTREACHED*/
		}
		memcpy(p + 1, ifa->ifa_addr, ifa->ifa_addr->sa_len);
		p->next = myaddrs;
		p->addr = (void *)(p + 1);
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6 = (void *)p->addr;
			inet6_getscopeid(sin6, INET6_IS_ADDR_LINKLOCAL|
				INET6_IS_ADDR_SITELOCAL);
		}
		myaddrs = p;
		if (dflag) {
			char hbuf[NI_MAXHOST];
			(void)getnameinfo(p->addr, (socklen_t)p->addr->sa_len,
			    hbuf, (socklen_t)sizeof(hbuf), NULL, 0,
			    NI_NUMERICHOST);
			syslog(LOG_INFO, "my interface: %s %s", hbuf,
			    ifa->ifa_name);
		}
	}

	freeifaddrs(ifap);
}

static void
free_myaddrs(void)
{
	struct myaddrs *p, *q;

	p = myaddrs;
	while (p) {
		q = p->next;
		free(p);
		p = q;
	}
	myaddrs = NULL;
}

static void
update_myaddrs(void)
{
	char msg[BUFSIZ];
	ssize_t len;
	struct rt_msghdr *rtm;

	len = read(sockfd, msg, sizeof(msg));
	if (len < 0) {
		syslog(LOG_ERR, "read(PF_ROUTE) failed");
		return;
	}
	rtm = (void *)msg;
	if (len < 4 || len < rtm->rtm_msglen) {
		syslog(LOG_ERR, "read(PF_ROUTE) short read");
		return;
	}
	if (rtm->rtm_version != RTM_VERSION) {
		syslog(LOG_ERR, "routing socket version mismatch");
		(void)close(sockfd);
		sockfd = 0;
		return;
	}
	switch (rtm->rtm_type) {
	case RTM_NEWADDR:
	case RTM_DELADDR:
	case RTM_IFINFO:
		break;
	default:
		return;
	}
	/* XXX more filters here? */

	syslog(LOG_INFO, "update interface address list");
	free_myaddrs();
	grab_myaddrs();
}
#endif /*USE_ROUTE*/

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-dp] [-f conf] service [serverpath [serverargs]]\n",
	    getprogname());
	exit(0);
}
