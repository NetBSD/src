/*	$NetBSD: rlogind.c,v 1.41.8.1 2012/11/20 03:00:46 tls Exp $	*/

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
 * Copyright (c) 1983, 1988, 1989, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1989, 1993\
 The Regents of the University of California.  All rights reserved.");
#if 0
static char sccsid[] = "@(#)rlogind.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: rlogind.c,v 1.41.8.1 2012/11/20 03:00:46 tls Exp $");
#endif
#endif /* not lint */

/*
 * remote login server:
 *	\0
 *	remuser\0
 *	locuser\0
 *	terminal_type/speed\0
 *	data
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <poll.h>
#include <vis.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pwd.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif
#include <util.h>
#include "pathnames.h"

#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif

#define		OPTIONS			"alnL"

static char	*env[2];
#define	NMAX 30
static char	lusername[NMAX+1], rusername[NMAX+1];
static	char term[64] = "TERM=";
#define	ENVSIZE	(sizeof("TERM=")-1)	/* skip null for concatenation */
static int	keepalive = 1;
static int	check_all = 0;
static int	log_success = 0;

static struct	passwd *pwd;

__dead static void	doit(int, struct sockaddr_storage *);
static int	control(int, char *, int);
static void	protocol(int, int);
__dead static void	cleanup(int);
__dead static void	fatal(int, const char *, int);
static int	do_rlogin(struct sockaddr *, char *);
static void	getstr(char *, int, const char *);
static void	setup_term(int);
#if 0
static int	do_krb_login(union sockunion *);
#endif
__dead static void	usage(void);
static int	local_domain(char *);
static char	*topdomain(char *);

extern int __check_rhosts_file;
extern char *__rcmd_errstr;	/* syslog hook from libc/net/rcmd.c */
extern char **environ;

int
main(int argc, char *argv[])
{
	struct sockaddr_storage from;
	int ch, on;
	socklen_t fromlen = sizeof(from);

	openlog("rlogind", LOG_PID, LOG_AUTH);

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

	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0) {
		syslog(LOG_ERR,"Can't get peer name of remote host: %m");
		fatal(STDERR_FILENO, "Can't get peer name of remote host", 1);
	}
#ifdef INET6
	if (from.ss_family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&from)->sin6_addr) &&
	    sizeof(struct sockaddr_in) <= sizeof(from)) {
		struct sockaddr_in sin4;
		struct sockaddr_in6 *sin6;
		const int off = sizeof(struct sockaddr_in6) -
		    sizeof(struct sockaddr_in);

		sin6 = (struct sockaddr_in6 *)&from;
		memset(&sin4, 0, sizeof(sin4));
		sin4.sin_family = AF_INET;
		sin4.sin_len = sizeof(struct sockaddr_in);
		memcpy(&sin4.sin_addr, &sin6->sin6_addr.s6_addr[off],
		    sizeof(sin4.sin_addr));
		memcpy(&from, &sin4, sizeof(sin4));
	}
#else
	if (from.ss_family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&from)->sin6_addr)) {
		char hbuf[NI_MAXHOST];
		if (getnameinfo((struct sockaddr *)&from, fromlen, hbuf,
				sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0) {
			strlcpy(hbuf, "invalid", sizeof(hbuf));
		}
		syslog(LOG_ERR, "malformed \"from\" address (v4 mapped, %s)\n",
		    hbuf);
		exit(1);
	}
#endif
	on = 1;
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
#if defined(IP_TOS)
	if (from.ss_family == AF_INET) {
		on = IPTOS_LOWDELAY;
		if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
			syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
	}
#endif
	doit(0, &from);
	/* NOTREACHED */
#ifdef __GNUC__
	exit(0);
#endif
}

static int	netf;
static char	line[MAXPATHLEN];
static int	confirmed;

static struct winsize win = { 0, 0, 0, 0 };

static void
doit(int f, struct sockaddr_storage *fromp)
{
	int master, pid, on = 1;
	int authenticated = 0;
	char *hostname;
	char hostnamebuf[2 * MAXHOSTNAMELEN + 1];
	char hostaddrbuf[sizeof(*fromp) * 4 + 1];
	char c;
	char naddr[NI_MAXHOST];
	char saddr[NI_MAXHOST];
	char raddr[NI_MAXHOST];
	int af = fromp->ss_family;
	u_int16_t *portp;
	struct addrinfo hints, *res, *res0;
	int gaierror;
	socklen_t fromlen = fromp->ss_len > sizeof(*fromp)
	    ? sizeof(*fromp) : fromp->ss_len;
	const int niflags = NI_NUMERICHOST | NI_NUMERICSERV;

	alarm(60);
	read(f, &c, 1);

	if (c != 0)
		exit(1);

	alarm(0);
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
		syslog(LOG_ERR, "malformed \"from\" address (af %d)\n", af);
		exit(1);
	}
	if (getnameinfo((struct sockaddr *)fromp, fromlen,
		    naddr, sizeof(naddr), NULL, 0, niflags) != 0) {
		syslog(LOG_ERR, "malformed \"from\" address (af %d)\n", af);
		exit(1);
	}

	if (getnameinfo((struct sockaddr *)fromp, fromlen,
		    saddr, sizeof(saddr), NULL, 0, NI_NAMEREQD) == 0) {
		/*
		 * If name returned by getnameinfo is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
		hostname = saddr;
		res0 = NULL;
		if (check_all || local_domain(saddr)) {
			strlcpy(hostnamebuf, saddr, sizeof(hostnamebuf));
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = fromp->ss_family;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_CANONNAME;
			gaierror = getaddrinfo(hostnamebuf, "0", &hints, &res0);
			if (gaierror) {
				syslog(LOG_NOTICE,
				    "Couldn't look up address for %s: %s",
				    hostnamebuf, gai_strerror(gaierror));
				hostname = naddr;
			} else {
				for (res = res0; res; res = res->ai_next) {
					if (res->ai_family != fromp->ss_family)
						continue;
					if (res->ai_addrlen != fromp->ss_len)
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
		hostname = hostnamebuf;
	}

	if (ntohs(*portp) >= IPPORT_RESERVED ||
	    ntohs(*portp) < IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "Connection from %s on illegal port",
		       naddr);
		fatal(f, "Permission denied", 0);
	}
#ifdef IP_OPTIONS
	if (fromp->ss_family == AF_INET) {
		u_char optbuf[BUFSIZ/3], *cp;
		char lbuf[BUFSIZ], *lp, *ep;
		socklen_t optsize = sizeof(optbuf);
		int ipproto;
		struct protoent *ip;

		if ((ip = getprotobyname("ip")) != NULL)
			ipproto = ip->p_proto;
		else
			ipproto = IPPROTO_IP;
		if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf,
		    &optsize) == 0 && optsize != 0) {
			lp = lbuf;
			ep = lbuf + sizeof(lbuf);
			for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
				snprintf(lp, ep - lp, " %2.2x", *cp);
			syslog(LOG_NOTICE,
			    "Connection received using IP options (ignored):%s",
			    lbuf);
			if (setsockopt(0, ipproto, IP_OPTIONS,
			    NULL, optsize) != 0) {
				syslog(LOG_ERR,
				    "setsockopt IP_OPTIONS NULL: %m");
				exit(1);
			}
		}
	}
#endif
	if (do_rlogin((struct sockaddr *)fromp, hostname) == 0)
		authenticated++;
	if (confirmed == 0) {
		write(f, "", 1);
		confirmed = 1;		/* we sent the null! */
	}
	netf = f;

	pid = forkpty(&master, line, NULL, &win);
	if (pid < 0) {
		if (errno == ENOENT)
			fatal(f, "Out of ptys", 0);
		else
			fatal(f, "Forkpty", 1);
	}
	if (pid == 0) {
		if (f > 2)	/* f should always be 0, but... */
			(void) close(f);
		setup_term(0);
		(void)strvisx(hostaddrbuf, (const char *)(const void *)fromp,
		    sizeof(*fromp), VIS_WHITE);
		if (authenticated)
			execl(_PATH_LOGIN, "login", "-p",
			    "-h", hostname, "-a", hostaddrbuf,
			    "-f", "--", lusername, (char *)0);
		else
			execl(_PATH_LOGIN, "login", "-p",
			    "-h", hostname, "-a", hostaddrbuf,
			    "--", lusername, (char *)0);
		fatal(STDERR_FILENO, _PATH_LOGIN, 1);
		/*NOTREACHED*/
	}
	ioctl(f, FIONBIO, &on);
	ioctl(master, FIONBIO, &on);
	ioctl(master, TIOCPKT, &on);
	signal(SIGCHLD, cleanup);
	protocol(f, master);
	signal(SIGCHLD, SIG_IGN);
	cleanup(0);
}

static char	magic[2] = { 0377, 0377 };
static char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
static int
control(int pty, char *cp, int n)
{
	struct winsize w;

	if (n < (int)(4+sizeof (w)) || cp[2] != 's' || cp[3] != 's')
		return (0);
	oobdata[0] &= ~TIOCPKT_WINDOW;	/* we know he heard */
	memmove(&w, cp+4, sizeof(w));
	w.ws_row = ntohs(w.ws_row);
	w.ws_col = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
	(void)ioctl(pty, TIOCSWINSZ, &w);
	return (4+sizeof (w));
}

/*
 * rlogin "protocol" machine.
 */
static void
protocol(int f, int p)
{
	char pibuf[1024+1], fibuf[1024], *pbp = NULL, *fbp = NULL;
					/* XXX gcc above */
	int pcc = 0, fcc = 0;
	int cc, nfd;
	char cntl;
	struct pollfd set[2];

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	set[0].fd = p;
	set[1].fd = f;
	for (;;) {
		set[0].events = POLLPRI;
		set[1].events = 0;
		if (fcc)
			set[0].events |= POLLOUT;
		else
			set[1].events |= POLLIN;
		if (pcc >= 0) {
			if (pcc)
				set[1].events |= POLLOUT;
			else
				set[0].events |= POLLIN;
		}
		if ((nfd = poll(set, 2, INFTIM)) < 0) {
			if (errno == EINTR)
				continue;
			fatal(f, "poll", 1);
		}
		if (nfd == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (set[0].revents & POLLPRI) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE)
					pcc = 0;
			}
		}
		if (set[1].revents & POLLIN) {
			fcc = read(f, fibuf, sizeof(fibuf));
			if (fcc < 0 && errno == EWOULDBLOCK)
				fcc = 0;
			else {
				char *cp;
				int left, n;

				if (fcc <= 0)
					break;
				fbp = fibuf;

			top:
				for (cp = fibuf; cp < fibuf+fcc-1; cp++)
					if (cp[0] == magic[0] &&
					    cp[1] == magic[1]) {
						left = fcc - (cp-fibuf);
						n = control(p, cp, left);
						if (n) {
							left -= n;
							if (left > 0)
								memmove(cp,
								    cp+n,
								    left);
							fcc -= n;
							goto top; /* n^2 */
						}
					}
			}
		}

		if (set[0].revents & POLLOUT && fcc > 0) {
			cc = write(p, fbp, fcc);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (set[0].revents & POLLIN) {
			pcc = read(p, pibuf, sizeof (pibuf));
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else if (pcc <= 0)
				break;
			else if (pibuf[0] == 0) {
				pbp++, pcc--;
			} else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);
				}
				pcc = 0;
			}
		}
		if (set[1].revents & POLLOUT && pcc > 0) {
			cc = write(f, pbp, pcc);
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}

static void
cleanup(int signo)
{
	char *p, c;

	p = line + sizeof(_PATH_DEV) - 1;
#ifdef SUPPORT_UTMP
	if (logout(p))
		logwtmp(p, "", "");
#endif
#ifdef SUPPORT_UTMPX
	if (logoutx(p, 0, DEAD_PROCESS))
		logwtmpx(p, "", "", 0, DEAD_PROCESS);
#endif
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	c = *p; *p = 'p';
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	*p = c;
	if (ttyaction(line, "rlogind", "root"))
		syslog(LOG_ERR, "%s: ttyaction failed", line);
	shutdown(netf, 2);
	exit(1);
}

static void
fatal(int f, const char *msg, int syserr)
{
	int len;
	char buf[BUFSIZ], *bp, *ep;

	bp = buf;
	ep = buf + sizeof(buf);

	/*
	 * Prepend binary one to message if we haven't sent
	 * the magic null as confirmation.
	 */
	if (!confirmed)
		*bp++ = '\001';		/* error indicator */
	if (syserr)
		len = snprintf(bp, ep - bp, "rlogind: %s: %s.\r\n",
		    msg, strerror(errno));
	else
		len = snprintf(bp, ep - bp, "rlogind: %s.\r\n", msg);
	(void) write(f, buf, bp + len - buf);
	exit(1);
}

static int
do_rlogin(struct sockaddr *dest, char *host)
{
	int retval;

	getstr(rusername, sizeof(rusername), "remuser too long");
	getstr(lusername, sizeof(lusername), "locuser too long");
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type too long");

	pwd = getpwnam(lusername);
	if (pwd == NULL) {
		syslog(LOG_INFO,
		    "%s@%s as %s: unknown login.", rusername, host, lusername);
		return (-1);
	}

	retval = iruserok_sa(dest, dest->sa_len, pwd->pw_uid == 0, rusername,
		lusername);
/* XXX put inet_ntoa(dest->sin_addr.s_addr) into all messages below */
	if (retval == 0) {
		if (log_success)
			syslog(LOG_INFO, "%s@%s as %s: iruserok succeeded",
			    rusername, host, lusername);
	} else {
		if (__rcmd_errstr)
			syslog(LOG_INFO, "%s@%s as %s: iruserok failed (%s)",
			    rusername, host, lusername, __rcmd_errstr);
		else
			syslog(LOG_INFO, "%s@%s as %s: iruserok failed",
			    rusername, host, lusername);
	}
	return(retval);
}

static void
getstr(char *buf, int cnt, const char *errmsg)
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		if (--cnt < 0)
			fatal(STDOUT_FILENO, errmsg, 0);
		*buf++ = c;
	} while (c != 0);
}


static void
setup_term(int fd)
{
	char *cp = index(term+ENVSIZE, '/');
	char *speed;
	struct termios tt;

#ifndef notyet
	tcgetattr(fd, &tt);
	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = index(speed, '/');
		if (cp)
			*cp++ = '\0';
		cfsetspeed(&tt, atoi(speed));
	}

	tt.c_iflag = TTYDEF_IFLAG;
	tt.c_oflag = TTYDEF_OFLAG;
	tt.c_lflag = TTYDEF_LFLAG;
	tcsetattr(fd, TCSAFLUSH, &tt);
#else
	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = index(speed, '/');
		if (cp)
			*cp++ = '\0';
		tcgetattr(fd, &tt);
		cfsetspeed(&tt, atoi(speed));
		tcsetattr(fd, TCSAFLUSH, &tt);
	}
#endif

	env[0] = term;
	env[1] = 0;
	environ = env;
}


static void
usage(void)
{
	syslog(LOG_ERR, "usage: rlogind [-alnL]");
	exit(1);
}

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
static int
local_domain(char *h)
{
	char localhost[MAXHOSTNAMELEN + 1];
	char *p1, *p2;

	localhost[0] = 0;
	(void) gethostname(localhost, sizeof(localhost));
	localhost[sizeof(localhost) - 1] = '\0';
	p1 = topdomain(localhost);
	p2 = topdomain(h);
	if (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2))
		return (1);
	return (0);
}

static char *
topdomain(char *h)
{
	char *p;
	char *maybe = NULL;
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
