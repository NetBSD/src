/*	$NetBSD: rlogind.c,v 1.19 1999/12/31 08:01:26 itojun Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "@(#)rlogind.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: rlogind.c,v 1.19 1999/12/31 08:01:26 itojun Exp $");
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

#define	FD_SETSIZE	16		/* don't need many bits for select */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

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
#include <util.h>
#include <utmp.h>
#include "pathnames.h"

#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif

#define		OPTIONS			"alnL"

char	*env[2];
#define	NMAX 30
char	lusername[NMAX+1], rusername[NMAX+1];
static	char term[64] = "TERM=";
#define	ENVSIZE	(sizeof("TERM=")-1)	/* skip null for concatenation */
int	keepalive = 1;
int	check_all = 0;
int	log_success = 0;

struct	passwd *pwd;

void	doit __P((int, struct sockaddr_in *));
int	control __P((int, char *, int));
void	protocol __P((int, int));
void	cleanup __P((int));
void	fatal __P((int, char *, int));
int	do_rlogin __P((struct sockaddr_in *, char *));
void	getstr __P((char *, int, char *));
void	setup_term __P((int));
int	do_krb_login __P((struct sockaddr_in *));
void	usage __P((void));
int	local_domain __P((char *));
char	*topdomain __P((char *));
int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int __check_rhosts_file;
	struct sockaddr_in from;
	int ch, fromlen, on;

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

	fromlen = sizeof (from);
	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0) {
		syslog(LOG_ERR,"Can't get peer name of remote host: %m");
		fatal(STDERR_FILENO, "Can't get peer name of remote host", 1);
	}
	on = 1;
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	on = IPTOS_LOWDELAY;
	if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
	doit(0, &from);
	/* NOTREACHED */
#ifdef __GNUC__
	exit(0);
#endif
}

int	child;
int	netf;
char	line[MAXPATHLEN];
int	confirmed;

struct winsize win = { 0, 0, 0, 0 };


void
doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	int master, pid, on = 1;
	int authenticated = 0;
	struct hostent *hp;
	char utmphost[UT_HOSTSIZE + 1];
	char *hostname;
	char hostnamebuf[2 * MAXHOSTNAMELEN + 1];
	char c;

	alarm(60);
	read(f, &c, 1);

	if (c != 0)
		exit(1);

	alarm(0);
	fromp->sin_port = ntohs((in_port_t)fromp->sin_port);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof(struct in_addr),
	    fromp->sin_family);
	if (hp) {
		/*
		 * If name returned by gethostbyaddr is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
		hostname = hp->h_name;
		if (check_all || local_domain(hp->h_name)) {
			strncpy(hostnamebuf, hp->h_name,
			    sizeof(hostnamebuf) - 1);
			hostnamebuf[sizeof(hostnamebuf) - 1] = 0;
			hp = gethostbyname(hostnamebuf);
			if (hp == NULL) {
				syslog(LOG_INFO,
				    "Couldn't look up address for %s",
				    hostnamebuf);
				hostname = inet_ntoa(fromp->sin_addr);
			} else for (; ; hp->h_addr_list++) {
				if (hp->h_addr_list[0] == NULL) {
					syslog(LOG_NOTICE,
					  "Host addr %s not listed for host %s",
					    inet_ntoa(fromp->sin_addr),
					    hp->h_name);
					hostname = inet_ntoa(fromp->sin_addr);
					break;
				}
				if (!memcmp(hp->h_addr_list[0],
				    (caddr_t)&fromp->sin_addr,
				    sizeof(fromp->sin_addr))) {
					hostname = hp->h_name;
					break;
				}
			}
		}
		hostname = strncpy(hostnamebuf, hostname,
				   sizeof(hostnamebuf) - 1);
	} else
		hostname = strncpy(hostnamebuf, inet_ntoa(fromp->sin_addr),
				   sizeof(hostnamebuf) - 1);

	hostnamebuf[sizeof(hostnamebuf) - 1] = '\0';

	if (strlen(hostname) < sizeof(utmphost))
		(void)strcpy(utmphost, hostname);
	else
		(void)strncpy(utmphost, inet_ntoa(fromp->sin_addr),
				sizeof(utmphost));
	utmphost[sizeof(utmphost) - 1] = '\0';

	if (fromp->sin_family != AF_INET ||
	    fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "Connection from %s on illegal port",
			inet_ntoa(fromp->sin_addr));
		fatal(f, "Permission denied", 0);
	}
#ifdef IP_OPTIONS
	{
		u_char optbuf[BUFSIZ/3], *cp;
		char lbuf[BUFSIZ], *lp;
		int optsize = sizeof(optbuf), ipproto;
		struct protoent *ip;

		if ((ip = getprotobyname("ip")) != NULL)
			ipproto = ip->p_proto;
		else
			ipproto = IPPROTO_IP;
		if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf,
		    &optsize) == 0 && optsize != 0) {
			lp = lbuf;
			for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
				sprintf(lp, " %2.2x", *cp);
			syslog(LOG_NOTICE,
			    "Connection received using IP options (ignored):%s",
			    lbuf);
			if (setsockopt(0, ipproto, IP_OPTIONS,
			    (char *)NULL, optsize) != 0) {
				syslog(LOG_ERR,
				    "setsockopt IP_OPTIONS NULL: %m");
				exit(1);
			}
		}
	}
#endif
	if (do_rlogin(fromp, hostname) == 0)
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
		if (authenticated)
			execl(_PATH_LOGIN, "login", "-p",
			    "-h", utmphost, "-f", "--", lusername, (char *)0);
		else
			execl(_PATH_LOGIN, "login", "-p",
			    "-h", utmphost, "--", lusername, (char *)0);
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

char	magic[2] = { 0377, 0377 };
char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
int
control(pty, cp, n)
	int pty;
	char *cp;
	int n;
{
	struct winsize w;

	if (n < 4+sizeof (w) || cp[2] != 's' || cp[3] != 's')
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
void
protocol(f, p)
	int f, p;
{
	char pibuf[1024+1], fibuf[1024], *pbp = NULL, *fbp = NULL;
					/* XXX gcc above */
	int pcc = 0, fcc = 0;
	int cc, nfd, n;
	char cntl;

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	if (f > p)
		nfd = f + 1;
	else
		nfd = p + 1;
	if (nfd > FD_SETSIZE) {
		syslog(LOG_ERR, "select mask too small, increase FD_SETSIZE");
		fatal(f, "internal error (select mask too small)", 0);
	}
	for (;;) {
		fd_set ibits, obits, ebits, *omask;

		FD_ZERO(&ebits);
		FD_ZERO(&ibits);
		FD_ZERO(&obits);
		omask = (fd_set *)NULL;
		if (fcc) {
			FD_SET(p, &obits);
			omask = &obits;
		} else
			FD_SET(f, &ibits);
		if (pcc >= 0) {
			if (pcc) {
				FD_SET(f, &obits);
				omask = &obits;
			} else
				FD_SET(p, &ibits);
		}
		FD_SET(p, &ebits);
		if ((n = select(nfd, &ibits, omask, &ebits, 0)) < 0) {
			if (errno == EINTR)
				continue;
			fatal(f, "select", 1);
		}
		if (n == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (FD_ISSET(p, &ebits)) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE) {
					pcc = 0;
					FD_CLR(p, &ibits);
				}
			}
		}
		if (FD_ISSET(f, &ibits)) {
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
				FD_SET(p, &obits);		/* try write */
			}
		}

		if (FD_ISSET(p, &obits) && fcc > 0) {
			cc = write(p, fbp, fcc);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (FD_ISSET(p, &ibits)) {
			pcc = read(p, pibuf, sizeof (pibuf));
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
			else if (pcc <= 0)
				break;
			else if (pibuf[0] == 0) {
				pbp++, pcc--;
					FD_SET(f, &obits);	/* try write */
			} else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);
				}
				pcc = 0;
			}
		}
		if ((FD_ISSET(f, &obits)) && pcc > 0) {
				cc = write(f, pbp, pcc);
			if (cc < 0 && errno == EWOULDBLOCK) {
				/*
				 * This happens when we try write after read
				 * from p, but some old kernels balk at large
				 * writes even when select returns true.
				 */
				if (!FD_ISSET(p, &ibits))
					sleep(5);
				continue;
			}
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}

void
cleanup(signo)
	int signo;
{
	char *p, c;

	p = line + sizeof(_PATH_DEV) - 1;
	if (logout(p))
		logwtmp(p, "", "");
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

void
fatal(f, msg, syserr)
	int f;
	char *msg;
	int syserr;
{
	int len;
	char buf[BUFSIZ], *bp = buf;

	/*
	 * Prepend binary one to message if we haven't sent
	 * the magic null as confirmation.
	 */
	if (!confirmed)
		*bp++ = '\01';		/* error indicator */
	if (syserr)
		len = sprintf(bp, "rlogind: %s: %s.\r\n",
		    msg, strerror(errno));
	else
		len = sprintf(bp, "rlogind: %s.\r\n", msg);
	(void) write(f, buf, bp + len - buf);
	exit(1);
}

int
do_rlogin(dest, host)
	struct sockaddr_in *dest;
	char *host;
{
	extern char *__rcmd_errstr;	/* syslog hook from libc/net/rcmd.c */
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
	retval = iruserok(dest->sin_addr.s_addr, pwd->pw_uid == 0, rusername,
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

void
getstr(buf, cnt, errmsg)
	char *buf;
	int cnt;
	char *errmsg;
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

extern	char **environ;

void
setup_term(fd)
	int fd;
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


void
usage()
{
	syslog(LOG_ERR, "usage: rlogind [-alnL]");
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
local_domain(h)
	char *h;
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

char *
topdomain(h)
	char *h;
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
