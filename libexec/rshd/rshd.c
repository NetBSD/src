/*	$NetBSD: rshd.c,v 1.30 2003/05/17 21:26:47 itojun Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by WIDE Project and
 *    its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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

/*-
 * Copyright (c) 1988, 1989, 1992, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1989, 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)rshd.c	8.2 (Berkeley) 4/6/94";
#else
__RCSID("$NetBSD: rshd.c,v 1.30 2003/05/17 21:26:47 itojun Exp $");
#endif
#endif /* not lint */

/*
 * remote shell server:
 *	[port]\0
 *	remuser\0
 *	locuser\0
 *	command\0
 *	data
 */
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <poll.h>
#ifdef  LOGIN_CAP
#include <login_cap.h>
#endif

int	keepalive = 1;
int	check_all;
int	log_success;		/* If TRUE, log all successful accesses */
int	sent_null;

void	 doit __P((struct sockaddr *));
void	 error __P((const char *, ...))
     __attribute__((__format__(__printf__, 1, 2)));
void	 getstr __P((char *, int, char *));
int	 local_domain __P((char *));
char	*topdomain __P((char *));
void	 usage __P((void));
int	main __P((int, char *[]));

#define	OPTIONS	"alnL"
extern int __check_rhosts_file;
extern char *__rcmd_errstr;	/* syslog hook from libc/net/rcmd.c. */

int
main(int argc, char *argv[])
{
	struct linger linger;
	int ch, on = 1, fromlen;
	struct sockaddr_storage from;

	openlog("rshd", LOG_PID, LOG_DAEMON);

	opterr = 0;
	while ((ch = getopt(argc, argv, OPTIONS)) != -1)
		switch (ch) {
		case 'a':
			check_all = 1;
			break;
		case 'l':
			__check_rhosts_file = 0;
			break;
		case 'n':
			keepalive = 0;
			break;
		case 'L':
			log_success = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}

	argc -= optind;
	argv += optind;

	fromlen = sizeof (from); /* xxx */
	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0) {
		syslog(LOG_ERR, "getpeername: %m");
		exit(1);
	}
#if 0
	if (((struct sockaddr *)&from)->sa_family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&from)->sin6_addr) &&
	    sizeof(struct sockaddr_in) <= sizeof(from)) {
		struct sockaddr_in sin;
		struct sockaddr_in6 *sin6;
		const int off = sizeof(struct sockaddr_in6) -
		    sizeof(struct sockaddr_in);

		sin6 = (struct sockaddr_in6 *)&from;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_len = sizeof(struct sockaddr_in);
		memcpy(&sin.sin_addr, &sin6->sin6_addr.s6_addr[off],
		    sizeof(sin.sin_addr));
		memcpy(&from, &sin, sizeof(sin));
		fromlen = sin.sin_len;
	}
#else
	if (((struct sockaddr *)&from)->sa_family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&from)->sin6_addr)) {
		char hbuf[NI_MAXHOST];
		if (getnameinfo((struct sockaddr *)&from, fromlen, hbuf,
				sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0) {
			strlcpy(hbuf, "invalid", sizeof(hbuf));
		}
		syslog(LOG_ERR, "malformed \"from\" address (v4 mapped, %s)",
		    hbuf);
		exit(1);
	}
#endif
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on,
	    sizeof(on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	linger.l_onoff = 1;
	linger.l_linger = 60;			/* XXX */
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *)&linger,
	    sizeof (linger)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
	doit((struct sockaddr *)&from);
	/* NOTREACHED */
#ifdef __GNUC__
	exit(0);
#endif
}

char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	path[100] = "PATH=";
char	*envinit[] =
	    {homedir, shell, path, username, 0};
char	**environ;

void
doit(struct sockaddr *fromp)
{
	struct passwd *pwd;
	in_port_t port;
	struct pollfd set[2];
	int cc, pv[2], pid, s = -1;	/* XXX gcc */
	int one = 1;
	char *hostname, *errorstr, *errorhost = NULL;	/* XXX gcc */
	const char *cp;
	char sig, buf[BUFSIZ];
	char cmdbuf[NCARGS+1], locuser[16], remuser[16];
	char remotehost[2 * MAXHOSTNAMELEN + 1];
	char hostnamebuf[2 * MAXHOSTNAMELEN + 1];
#ifdef  LOGIN_CAP
	login_cap_t *lc;
#endif
	char naddr[NI_MAXHOST];
	char saddr[NI_MAXHOST];
	char raddr[NI_MAXHOST];
	char pbuf[NI_MAXSERV];
	int af = fromp->sa_family;
	u_int16_t *portp;
	struct addrinfo hints, *res, *res0;
	int gaierror;
#ifdef NI_WITHSCOPEID
	const int niflags = NI_NUMERICHOST | NI_NUMERICSERV | NI_WITHSCOPEID;
#else
	const int niflags = NI_NUMERICHOST | NI_NUMERICSERV;
#endif

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef DEBUG
	{ int t = open(_PATH_TTY, 2);
	  if (t >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	  }
	}
#endif
	switch (af) {
	case AF_INET:
		portp = &((struct sockaddr_in *)fromp)->sin_port;
		break;
#ifdef INET6
	case AF_INET6:
		portp = &((struct sockaddr_in6 *)fromp)->sin6_port;
		break;
#endif
	default:
		syslog(LOG_ERR, "malformed \"from\" address (af %d)", af);
		exit(1);
	}
	if (getnameinfo(fromp, fromp->sa_len, naddr, sizeof(naddr),
			pbuf, sizeof(pbuf), niflags) != 0) {
		syslog(LOG_ERR, "malformed \"from\" address (af %d)", af);
		exit(1);
	}
#ifdef IP_OPTIONS
	if (af == AF_INET)
      {
	u_char optbuf[BUFSIZ/3], *cp;
	char lbuf[BUFSIZ], *lp, *ep;
	int optsize = sizeof(optbuf), ipproto;
	struct protoent *ip;

	if ((ip = getprotobyname("ip")) != NULL)
		ipproto = ip->p_proto;
	else
		ipproto = IPPROTO_IP;
	if (!getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf, &optsize) &&
	    optsize != 0) {
		lp = lbuf;
		ep = lbuf + sizeof(lbuf);
		for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
			snprintf(lp, ep - lp, " %2.2x", *cp);
		syslog(LOG_NOTICE,
		    "Connection received from %s using IP options (ignored):%s",
		    naddr, lbuf);
		if (setsockopt(0, ipproto, IP_OPTIONS,
		    (char *)NULL, optsize) != 0) {
			syslog(LOG_ERR, "setsockopt IP_OPTIONS NULL: %m");
			exit(1);
		}
	}
      }
#endif

	if (ntohs(*portp) >= IPPORT_RESERVED
	 || ntohs(*portp) < IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE|LOG_AUTH,
		    "Connection from %s on illegal port %u",
		    naddr, ntohs(*portp));
		exit(1);
	}

	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;

		if ((cc = read(STDIN_FILENO, &c, 1)) != 1) {
			if (cc < 0)
				syslog(LOG_ERR, "read: %m");
			shutdown(0, SHUT_RDWR);
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}

	(void) alarm(0);
	if (port != 0) {
		int lport = IPPORT_RESERVED - 1;
		s = rresvport_af(&lport, af);
		if (s < 0) {
			syslog(LOG_ERR, "can't get stderr port: %m");
			exit(1);
		}
		if (port >= IPPORT_RESERVED) {
			syslog(LOG_ERR, "2nd port not reserved");
			exit(1);
		}
		*portp = htons(port);
		if (connect(s, (struct sockaddr *)fromp, fromp->sa_len) < 0) {
			syslog(LOG_ERR, "connect second port %d: %m", port);
			exit(1);
		}
	}


#ifdef notdef
	/* from inetd, socket is already on 0, 1, 2 */
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
#endif
	errorstr = NULL;
	if (getnameinfo(fromp, fromp->sa_len, saddr, sizeof(saddr),
			NULL, 0, NI_NAMEREQD) == 0) {
		/*
		 * If name returned by getnameinfo is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
		hostname = saddr;
		res0 = NULL;
		if (check_all || local_domain(saddr)) {
			strlcpy(remotehost, saddr, sizeof(remotehost));
			errorhost = remotehost;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = fromp->sa_family;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_CANONNAME;
			gaierror = getaddrinfo(remotehost, pbuf, &hints, &res0);
			if (gaierror) {
				syslog(LOG_NOTICE,
				    "Couldn't look up address for %s: %s",
				    remotehost, gai_strerror(gaierror));
				errorstr =
				"Couldn't look up address for your host (%s)\n";
				hostname = naddr;
			} else {
				for (res = res0; res; res = res->ai_next) {
					if (res->ai_family != fromp->sa_family)
						continue;
					if (res->ai_addrlen != fromp->sa_len)
						continue;
					if (getnameinfo(res->ai_addr,
						res->ai_addrlen,
						raddr, sizeof(raddr), NULL, 0,
						niflags) == 0
					 && strcmp(naddr, raddr) == 0) {
						hostname = res->ai_canonname
							? res->ai_canonname
							: saddr;
						break;
					}
				}
				if (res == NULL) {
					syslog(LOG_NOTICE,
					  "Host addr %s not listed for host %s",
					    naddr, res0->ai_canonname
						    ? res0->ai_canonname
						    : saddr);
					errorstr =
					    "Host address mismatch for %s\n";
					hostname = naddr;
				}
			}
		}
		strlcpy(hostnamebuf, hostname, sizeof(hostnamebuf));
		hostname = hostnamebuf;
		if (res0)
			freeaddrinfo(res0);
	} else {
		strlcpy(hostnamebuf, naddr, sizeof(hostnamebuf));
		errorhost = hostname = hostnamebuf;
	}

	getstr(remuser, sizeof(remuser), "remuser");
	getstr(locuser, sizeof(locuser), "locuser");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	setpwent();
	pwd = getpwnam(locuser);
	if (pwd == NULL) {
		syslog(LOG_INFO|LOG_AUTH,
		    "%s@%s as %s: unknown login. cmd='%.80s'",
		    remuser, hostname, locuser, cmdbuf);
		if (errorstr == NULL)
			errorstr = "Permission denied.\n";
		goto fail;
	}
#ifdef LOGIN_CAP
	lc = login_getclass(pwd ? pwd->pw_class : NULL);
#endif	

	if (chdir(pwd->pw_dir) < 0) {
#ifdef LOGIN_CAP
		if (chdir("/") < 0 ||
		    login_getcapbool(lc, "requirehome", pwd->pw_uid ? 1 : 0)) {
			syslog(LOG_INFO|LOG_AUTH,
			    "%s@%s as %s: no home directory. cmd='%.80s'",
			    remuser, hostname, locuser, cmdbuf);
			error("No remote home directory.\n");
			exit(0);
		}
#else
		(void) chdir("/");
#ifdef notdef
		syslog(LOG_INFO|LOG_AUTH,
		    "%s@%s as %s: no home directory. cmd='%.80s'",
		    remuser, hostname, locuser, cmdbuf);
		error("No remote directory.\n");
		exit(1);
#endif /* notdef */
#endif /* LOGIN_CAP */
	}


	if (errorstr ||
	    (pwd->pw_passwd != 0 && *pwd->pw_passwd != '\0' &&
		iruserok_sa(fromp, fromp->sa_len, pwd->pw_uid == 0, remuser,
			locuser) < 0)) {
		if (__rcmd_errstr)
			syslog(LOG_INFO|LOG_AUTH,
			    "%s@%s as %s: permission denied (%s). cmd='%.80s'",
			    remuser, hostname, locuser, __rcmd_errstr,
			    cmdbuf);
		else
			syslog(LOG_INFO|LOG_AUTH,
			    "%s@%s as %s: permission denied. cmd='%.80s'",
			    remuser, hostname, locuser, cmdbuf);
fail:
		if (errorstr == NULL)
			errorstr = "Permission denied.\n";
		error(errorstr, errorhost);
		exit(1);
	}

	if (pwd->pw_uid && !access(_PATH_NOLOGIN, F_OK)) {
		error("Logins currently disabled.\n");
		exit(1);
	}

	(void) write(STDERR_FILENO, "\0", 1);
	sent_null = 1;

	if (port) {
		if (pipe(pv) < 0) {
			error("Can't make pipe.\n");
			exit(1);
		}
		pid = fork();
		if (pid == -1)  {
			error("Can't fork; try again.\n");
			exit(1);
		}
		if (pid) {
			{
				(void) close(0);
				(void) close(1);
			}
			(void) close(2);
			(void) close(pv[1]);

			set[0].fd = s;
			set[0].events = POLLIN;
			set[1].fd = pv[0];
			set[1].events = POLLIN;
			ioctl(pv[0], FIONBIO, (char *)&one);

			/* should set s nbio! */
			do {
				if (poll(set, 2, INFTIM) < 0)
					break;
				if (set[0].revents & POLLIN) {
					int	ret;

					ret = read(s, &sig, 1);
					if (ret <= 0)
						set[0].events = 0;
					else
						killpg(pid, sig);
				}
				if (set[1].revents & POLLIN) {
					errno = 0;
					cc = read(pv[0], buf, sizeof(buf));
					if (cc <= 0) {
						shutdown(s, SHUT_RDWR);
						set[1].events = 0;
					} else {
						(void) write(s, buf, cc);
					}
				}

			} while ((set[0].revents | set[1].revents) & POLLIN);
			exit(0);
		}
		(void) close(s);
		(void) close(pv[0]);
		dup2(pv[1], 2);
		close(pv[1]);
	}
	setsid();

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;
#ifdef LOGIN_CAP
	{
	char *sh;
	
	if((sh = login_getcapstr(lc, "shell", NULL, NULL))) {
		if(!(sh = strdup(sh))) {
                	syslog(LOG_ERR, "Cannot alloc mem");
                	exit(1);
		}
		pwd->pw_shell = sh;
	}
	}
#endif
	environ = envinit;
	strlcat(homedir, pwd->pw_dir, sizeof(homedir));
	strlcat(path, _PATH_DEFPATH, sizeof(path));
	strlcat(shell, pwd->pw_shell, sizeof(shell));
	strlcat(username, pwd->pw_name, sizeof(username));
#ifdef LOGIN_CAP
	if (setusercontext(lc, pwd, pwd->pw_uid, LOGIN_SETALL) != 0) {
		syslog(LOG_ERR, "setusercontext: %m");
		exit(1);
		}
	login_close(lc);
#else
	(void) setgid((gid_t)pwd->pw_gid);
	initgroups(pwd->pw_name, pwd->pw_gid);
	(void) setuid((uid_t)pwd->pw_uid);
#endif

	endpwent();
	if (log_success || pwd->pw_uid == 0) {
		syslog(LOG_INFO|LOG_AUTH, "%s@%s as %s: cmd='%.80s'",
		    remuser, hostname, locuser, cmdbuf);
	}
	cp = strrchr(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	execl(pwd->pw_shell, cp, "-c", cmdbuf, NULL);
	perror(pwd->pw_shell);
	exit(1);
}

/*
 * Report error to client.  Note: can't be used until second socket has
 * connected to client, or older clients will hang waiting for that
 * connection first.
 */

#include <stdarg.h>

void error(const char *fmt, ...)
{
	va_list ap;
	int len;
	char *bp, buf[BUFSIZ];
	va_start(ap, fmt);
	bp = buf;
	if (sent_null == 0) {
		*bp++ = 1;
		len = 1;
	} else
		len = 0;
	(void)vsnprintf(bp, sizeof(buf) - 1, fmt, ap);
	(void)write(STDERR_FILENO, buf, len + strlen(bp));
	va_end(ap);
}

void
getstr(char *buf, int cnt, char *err)
{
	char c;

	do {
		if (read(STDIN_FILENO, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err);
			exit(1);
		}
	} while (c != 0);
}

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
int
local_domain(char *h)
{
	char localhost[MAXHOSTNAMELEN + 1];
	char *p1, *p2;

	localhost[0] = 0;
	(void)gethostname(localhost, sizeof(localhost));
	localhost[sizeof(localhost) - 1] = '\0';
	p1 = topdomain(localhost);
	p2 = topdomain(h);
	if (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2))
		return (1);
	return (0);
}

char *
topdomain(char *h)
{
	char *p, *maybe = NULL;
	int dots = 0;

	for (p = h + strlen(h); p >= h; p--) {
		if (*p == '.') {
			if (++dots == 2)
				return (p);
			maybe = p;
		}
	}
	return (maybe);
}

void
usage(void)
{

	syslog(LOG_ERR, "usage: rshd [-%s]", OPTIONS);
	exit(2);
}
