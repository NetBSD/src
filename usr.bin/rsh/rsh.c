/*	$NetBSD: rsh.c,v 1.19 2003/05/22 02:14:03 hubertf Exp $	*/

/*-
 * Copyright (c) 1983, 1990, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)rsh.c	8.4 (Berkeley) 4/29/95";
#else
__RCSID("$NetBSD: rsh.c,v 1.19 2003/05/22 02:14:03 hubertf Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <poll.h>

#include <netinet/in.h>
#include <netdb.h>

#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

#ifdef KERBEROS
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>

CREDENTIALS cred;
Key_schedule schedule;
int use_kerberos = 1, doencrypt;
char dst_realm_buf[REALM_SZ], *dest_realm;

void	warning(const char *, ...);
#endif

/*
 * rsh - remote shell
 */
int	remerr;

static int sigs[] = { SIGINT, SIGTERM, SIGQUIT };

char   *copyargs(char **);
void	sendsig(int);
int	checkfd(struct pollfd *, int);
void	talk(int, sigset_t *, pid_t, int);
void	usage(void);
int	main(int, char **);
#ifdef IN_RCMD
int	 orcmd(char **, int, const char *,
    const char *, const char *, int *);
int	 orcmd_af(char **, int, const char *,
    const char *, const char *, int *, int);
#endif

int
main(int argc, char **argv)
{
	struct passwd *pw;
	struct servent *sp;
	sigset_t oset, nset;

#ifdef IN_RCMD
	char	*locuser = 0, *loop;
#endif /* IN_RCMD */
	int argoff, asrsh, ch, dflag, nflag, one, rem, i;
	pid_t pid;
	uid_t uid;
	char *args, *host, *p, *user, *name;
	char *service=NULL;

	argoff = asrsh = dflag = nflag = 0;
	one = 1;
	host = user = NULL;
	sp = NULL;

#ifndef IN_RCMD
	/*
	 * If called as something other than "rsh" use it as the host name,
	 * only for rsh.
	 */
	if (strcmp(getprogname(), "rsh") == 0)
		asrsh = 1;
	else {
		host = strdup(getprogname());
		if (host == NULL)
			err(1, NULL);
	}
#endif /* IN_RCMD */

	/* handle "rsh host flags" */
	if (!host && argc > 2 && argv[1][0] != '-') {
		host = argv[1];
		argoff = 1;
	}

#ifdef IN_RCMD
	if ((loop = getenv("RCMD_LOOP")) && strcmp(loop, "YES") == 0)
		warnx("rcmd appears to be looping!");

	putenv("RCMD_LOOP=YES");

# ifdef KERBEROS
#  ifdef CRYPT
#   define	OPTIONS	"8KLdek:l:np:u:wx"
#  else
#   define	OPTIONS	"8KLdek:l:np:u:w"
#  endif
# else
#  define	OPTIONS	"8KLdel:np:u:w"
# endif

#else /* IN_RCMD */

# ifdef KERBEROS
#  ifdef CRYPT
#   define	OPTIONS	"8KLdek:l:np:wx"
#  else
#   define	OPTIONS	"8KLdek:l:np:w"
#  endif
# else
#  define	OPTIONS	"8KLdel:np:w"
# endif

#endif /* IN_RCMD */

	if (!(pw = getpwuid(uid = getuid())))
		errx(1, "unknown user id");

	if ((name = strdup(pw->pw_name)) == NULL)
		err(1, "malloc");
	while ((ch = getopt(argc - argoff, argv + argoff, OPTIONS)) != -1)
		switch(ch) {
		case 'K':
#ifdef KERBEROS
			use_kerberos = 0;
#endif
			break;
		case 'L':	/* -8Lew are ignored to allow rlogin aliases */
		case 'e':
		case 'w':
		case '8':
			break;
		case 'd':
			dflag = 1;
			break;
		case 'l':
			user = optarg;
			break;
#ifdef KERBEROS
		case 'k':
			strlcpy(dest_realm_buf, optarg, sizeof(dest_realm_buf));
			dest_realm = dst_realm_buf;
			break;
#endif
		case 'n':
			nflag = 1;
			break;
		case 'p':
			service = optarg;
			sp = getservbyname(service, "tcp");
			if (sp == NULL) {	/* number given, no name */
				sp = malloc(sizeof(*sp));
				memset(sp, 0, sizeof(*sp));
				sp->s_name = service;
				sp->s_proto = "tcp"; 
				sp->s_port = atoi(service);
				if (sp->s_port <= 0 || sp->s_port > IPPORT_ANONMAX)
					errx(1,"port must be between 1 and %d", IPPORT_ANONMAX);
			}
			break;
#ifdef IN_RCMD
		case 'u':
			if (getuid() != 0 && optarg && name &&
			    strcmp(name, optarg) != 0)
				errx(1,"only super user can use the -u option");
			locuser = optarg;
			break;
#endif /* IN_RCMD */
#ifdef KERBEROS
#ifdef CRYPT
		case 'x':
			doencrypt = 1;
			des_set_key((des_cblock *) cred.session, schedule);
			break;
#endif
#endif
		case '?':
		default:
			usage();
		}
	optind += argoff;

	/* if haven't gotten a host yet, do so */
	if (!host && !(host = argv[optind++]))
		usage();

	/* if no further arguments, must have been called as rlogin. */
	if (!argv[optind]) {
#ifdef IN_RCMD
		usage();
#else
		if (asrsh)
			*argv = "rlogin";
		execv(_PATH_RLOGIN, argv);
		err(1, "can't exec %s", _PATH_RLOGIN);
#endif
	}

	argc -= optind;
	argv += optind;

	/* Accept user1@host format, though "-l user2" overrides user1 */
	p = strchr(host, '@');
	if (p) {
		*p = '\0';
		if (!user && p > host)
			user = host;
		host = p + 1;
		if (*host == '\0')
			usage();
	}
	if (!user)
		user = name;

#ifdef KERBEROS
#ifdef CRYPT
	/* -x turns off -n */
	if (doencrypt)
		nflag = 0;
#endif
#endif

	args = copyargs(argv);

#ifdef KERBEROS
	if (use_kerberos) {
		if (sp == NULL) {
			sp = getservbyname((doencrypt ? "ekshell" : "kshell"), "tcp");
		}
		if (sp == NULL) {
			use_kerberos = 0;
			warning("can't get entry for %s/tcp service",
			    doencrypt ? "ekshell" : "kshell");
		}
	}
#endif
	if (sp == NULL)
		sp = getservbyname("shell", "tcp");
	if (sp == NULL)
		errx(1, "shell/tcp: unknown service");

#ifdef KERBEROS
try_connect:
	if (use_kerberos) {
#if 1
		struct hostent *hp;

		/* fully qualify hostname (needed for krb_realmofhost) */
		hp = gethostbyname(host);
		if (hp != NULL && !(host = strdup(hp->h_name)))
			err(1, "strdup");
#endif

		rem = KSUCCESS;
		errno = 0;
		if (dest_realm == NULL)
			dest_realm = krb_realmofhost(host);

#ifdef CRYPT
		if (doencrypt)
			rem = krcmd_mutual(&host, sp->s_port, user, args,
			    &remerr, dest_realm, &cred, schedule);
		else
#endif
			rem = krcmd(&host, sp->s_port, user, args, &remerr,
			    dest_realm);
		if (rem < 0) {
			use_kerberos = 0;
			sp = getservbyname("shell", "tcp");
			if (sp == NULL)
				errx(1, "shell/tcp: unknown service");
			if (errno == ECONNREFUSED)
				warning("remote host doesn't support Kerberos");
			if (errno == ENOENT)
				warning("can't provide Kerberos auth data");
			goto try_connect;
		}
	} else {
		if (doencrypt)
			errx(1, "the -x flag requires Kerberos authentication.");
#ifdef IN_RCMD
		rem = orcmd_af(&host, sp->s_port, locuser ? locuser :
#else
		rem = rcmd_af(&host, sp->s_port,
#endif
		    name,
		    user, args, &remerr, PF_UNSPEC);
	}
#else /* KERBEROS */

#ifdef IN_RCMD
	rem = orcmd_af(&host, sp->s_port, locuser ? locuser :
#else
	rem = rcmd_af(&host, sp->s_port,
#endif
	    name, user, args, &remerr, PF_UNSPEC);
#endif /* KERBEROS */
	(void)free(name);

	if (rem < 0)
		exit(1);

	if (remerr < 0)
		errx(1, "can't establish stderr");
	if (dflag) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt remote");
		if (setsockopt(remerr, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt stderr");
	}

	(void) setuid(uid);

	(void) sigemptyset(&nset);
	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
		(void) sigaddset(&nset, sigs[i]);

	(void) sigprocmask(SIG_BLOCK, &nset, &oset);

	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
		struct sigaction sa;

		if (sa.sa_handler != SIG_IGN) {
			sa.sa_handler = sendsig;		
			(void) sigaction(sigs[i], &sa, NULL);
		}
	}

	if (!nflag) {
		pid = fork();
		if (pid < 0)
			err(1, "fork");
	}
	else
		pid = -1;

#if defined(KERBEROS) && defined(CRYPT)
	if (!doencrypt)
#endif
	{
		(void)ioctl(remerr, FIONBIO, &one);
		(void)ioctl(rem, FIONBIO, &one);
	}

	talk(nflag, &oset, pid, rem);

	if (!nflag)
		(void)kill(pid, SIGKILL);
	exit(0);
}

int
checkfd(struct pollfd *fdp, int outfd)
{
	int nr, nw;
	char buf[BUFSIZ];

	if (fdp->revents & (POLLNVAL|POLLERR|POLLHUP))
		return -1;
	   
	if ((fdp->revents & POLLIN) == 0)
		return 0;

	errno = 0;
#if defined(KERBEROS) && defined(CRYPT)
	if (doencrypt)
		nr = des_read(fdp->fd, buf, sizeof buf);
	else
#endif
		nr = read(fdp->fd, buf, sizeof buf);

	if (nr <= 0) {
		if (errno != EAGAIN)
			return -1;
		else
			return 0;
	}
	else {
		char *bc = buf;
		while (nr) {
			if ((nw = write(outfd, bc, nr)) <= 0)
				return -1;
			nr -= nw;
			bc += nw;
		}
		return 0;
	}
}

void
talk(int nflag, sigset_t *oset, __pid_t pid, int rem)
{
	int nr, nw, nfds;
	struct pollfd fds[2], *fdp = &fds[0];
	char *bp, buf[BUFSIZ];


	if (!nflag && pid == 0) {
		(void)close(remerr);

		fdp->events = POLLOUT|POLLNVAL|POLLERR|POLLHUP;
		fdp->fd = rem;
		nr = 0;
		bp = buf;

		for (;;) {
			errno = 0;

			if (nr == 0) {
				if ((nr = read(0, buf, sizeof buf)) == 0)
					goto done;
				if (nr == -1) {
					if (errno == EIO)
						goto done;
					if (errno == EINTR)
						continue;
					err(1, "read");
				}
				bp = buf;
			}

rewrite:		if (poll(fdp, 1, INFTIM) == -1) {
				if (errno != EINTR)
					err(1, "poll");
				goto rewrite;
			}

			if (fdp->revents & (POLLNVAL|POLLERR|POLLHUP))
				err(1, "poll");

			if ((fdp->revents & POLLOUT) == 0)
				goto rewrite;

#if defined(KERBEROS) && defined(CRYPT)
			if (doencrypt)
				nw = des_write(rem, bp, nr);
			else
#endif
				nw = write(rem, bp, nr);

			if (nw < 0) {
				if (errno == EAGAIN)
					continue;
				err(1, "write");
			}
			bp += nw;
			nr -= nw;
		}
done:
		(void)shutdown(rem, 1);
		exit(0);
	}

	(void) sigprocmask(SIG_SETMASK, oset, NULL);
	fds[0].events = fds[1].events = POLLIN|POLLNVAL|POLLERR|POLLHUP;
	fds[0].fd = remerr;
	fds[1].fd = rem;
	fdp = &fds[0];
	nfds = 2;
	do {
		if (poll(fdp, nfds, INFTIM) == -1) {
			if (errno != EINTR)
				err(1, "poll");
			continue;
		}
		if (fds[0].events != 0 && checkfd(&fds[0], 2) == -1) {
			nfds--;
			fds[0].events = 0;
			fdp = &fds[1];
		}
		if (fds[1].events != 0 && checkfd(&fds[1], 1) == -1) {
			nfds--;
			fds[1].events = 0;
		}
	}
	while (nfds);
}

void
sendsig(int sig)
{
	char signo;

	signo = sig;
#ifdef KERBEROS
#ifdef CRYPT
	if (doencrypt)
		(void)des_write(remerr, &signo, 1);
	else
#endif
#endif
		(void)write(remerr, &signo, 1);
}

#ifdef KERBEROS
/* VARARGS */
void
warning(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) fprintf(stderr, "%s: warning, using standard rsh: ",
	    getprogname());
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void) fprintf(stderr, ".\n");
}
#endif

char *
copyargs(char **argv)
{
	int cc;
	char **ap, *args, *p, *ep;

	cc = 0;
	for (ap = argv; *ap; ++ap)
		cc += strlen(*ap) + 1;
	if (!(args = malloc((u_int)cc)))
		err(1, "malloc");
	ep = args + cc;
	for (p = args, *p = '\0', ap = argv; *ap; ++ap) {
		(void)strlcpy(p, *ap, ep - p);
		p += strlen(p);
		if (ap[1])
			*p++ = ' ';
	}
	*p = '\0';
	return (args);
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-nd%s]%s[-l login] [-p port]%s [login@]host %s\n", getprogname(),
#ifdef KERBEROS
#ifdef CRYPT
	    "x", " [-k realm] ",
#else
	    "", " [-k realm] ",
#endif
#else
	    "", " ",
#endif
#ifdef IN_RCMD
	    " [-u locuser]", "command"
#else
	    "", "[command]"
#endif
	    );
	exit(1);
}
