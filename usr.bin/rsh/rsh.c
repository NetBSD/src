/*	$NetBSD: rsh.c,v 1.4.2.5 1997/05/21 06:40:43 mrg Exp $	*/

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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)rsh.c	8.4 (Berkeley) 4/29/95";*/
static char rcsid[] = "$NetBSD: rsh.c,v 1.4.2.5 1997/05/21 06:40:43 mrg Exp $";
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <varargs.h>

#include "pathnames.h"

#ifdef KERBEROS
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>

CREDENTIALS cred;
Key_schedule schedule;
int use_kerberos = 1, doencrypt;
char dst_realm_buf[REALM_SZ], *dest_realm;
extern char *krb_realmofhost();
#endif

/*
 * rsh - remote shell
 */
extern	char *__progname;		/* XXX */
int	rfd2;

char   *copyargs __P((char **));
void	sendsig __P((int));
void	talk __P((int, long, pid_t, int));
void	usage __P((void));
void	warning __P(());

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct passwd *pw;
	struct servent *sp;
	long omask;
#ifdef IN_RCMD
	char	*locuser = 0, *loop;
#endif /* IN_RCMD */
	int argoff, asrsh, ch, dflag, nflag, one, rem;
	pid_t pid;
	uid_t uid;
	char *args, *host, *p, *user;

	argoff = asrsh = dflag = nflag = 0;
	one = 1;
	host = user = NULL;

	/*
	 * If called as something other than "rsh" or "rcmd", use it as the
	 * host name
	 */
	p = __progname;
	if (strcmp(p, "rsh") == 0
#ifdef IN_RCMD
	    || strcmp(p, "rcmd") == 0
#endif /* IN_RCMD */
	    )
		asrsh = 1;
	else
		host = p;

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
#   define	OPTIONS	"8KLdek:l:nu:wx"
#  else
#   define	OPTIONS	"8KLdek:l:nu:w"
#  endif
# else
#  define	OPTIONS	"8KLdel:nu:w"
# endif

#else /* IN_RCMD */

# ifdef KERBEROS
#  ifdef CRYPT
#   define	OPTIONS	"8KLdek:l:nwx"
#  else
#   define	OPTIONS	"8KLdek:l:nw"
#  endif
# else
#  define	OPTIONS	"8KLdel:nw"
# endif

#endif /* IN_RCMD */

	if (!(pw = getpwuid(uid = getuid())))
		errx(1, "unknown user id");

	while ((ch = getopt(argc - argoff, argv + argoff, OPTIONS)) != EOF)
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
			dest_realm = dst_realm_buf;
			strncpy(dest_realm, optarg, REALM_SZ);
			break;
#endif
		case 'n':
			nflag = 1;
			break;
#ifdef IN_RCMD
		case 'u':
			if (getuid() != 0 && optarg && pw->pw_name && strcmp(pw->pw_name, optarg) != 0)
				errx(1, "only super user can use the -u option");
			locuser = optarg;
			break;
#endif /* IN_RCMD */
#ifdef KERBEROS
#ifdef CRYPT
		case 'x':
			doencrypt = 1;
			des_set_key(cred.session, schedule);
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
		user = pw->pw_name;

#ifdef KERBEROS
#ifdef CRYPT
	/* -x turns off -n */
	if (doencrypt)
		nflag = 0;
#endif
#endif

	args = copyargs(argv);

	sp = NULL;
#ifdef KERBEROS
	if (use_kerberos) {
		sp = getservbyname((doencrypt ? "ekshell" : "kshell"), "tcp");
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
			err(1, NULL);
#endif

		rem = KSUCCESS;
		errno = 0;
		if (dest_realm == NULL)
			dest_realm = krb_realmofhost(host);

#ifdef CRYPT
		if (doencrypt)
			rem = krcmd_mutual(&host, sp->s_port, user, args,
			    &rfd2, dest_realm, &cred, schedule);
		else
#endif
			rem = krcmd(&host, sp->s_port, user, args, &rfd2,
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
		rem = orcmd(&host, sp->s_port, locuser ? locuser :
#else
		rem = rcmd(&host, sp->s_port,
#endif
		    pw->pw_name,
		    user, args, &rfd2);
	}
#else /* KERBEROS */

#ifdef IN_RCMD
	rem = orcmd(&host, sp->s_port, locuser ? locuser :
#else
	rem = rcmd(&host, sp->s_port,
#endif
	    pw->pw_name, user,
	    args, &rfd2);
#endif /* KERBEROS */

	if (rem < 0)
		exit(1);

	if (rfd2 < 0)
		errx(1, "can't establish stderr");
	if (dflag) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt remote");
		if (setsockopt(rfd2, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt stderr");
	}

	(void)setuid(uid);
	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGTERM));
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, sendsig);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGQUIT, sendsig);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void)signal(SIGTERM, sendsig);

	if (!nflag) {
		pid = fork();
		if (pid < 0)
			err(1, "fork");
	}

#ifdef KERBEROS
#ifdef CRYPT
	if (!doencrypt)
#endif
#endif
	{
		(void)ioctl(rfd2, FIONBIO, &one);
		(void)ioctl(rem, FIONBIO, &one);
	}

	talk(nflag, omask, pid, rem);

	if (!nflag)
		(void)kill(pid, SIGKILL);
	exit(0);
}

void
talk(nflag, omask, pid, rem)
	int nflag;
	long omask;
	pid_t pid;
	int rem;
{
	int cc, wc, nfds;
	struct pollfd fds[2], *fdp = &fds[0];
	char *bp, buf[BUFSIZ];

	if (!nflag && pid == 0) {
		(void)close(rfd2);

reread:		errno = 0;
		if ((cc = read(0, buf, sizeof buf)) <= 0)
			goto done;
		bp = buf;

rewrite:	fdp->events = POLLOUT;
		fdp->fd = rem;
		if (poll(fdp, 1, 0) < 0) {
			if (errno != EINTR)
				err(1, "poll");
			goto rewrite;
		}
		if ((fdp->revents & POLLOUT) == 0)
			goto rewrite;
#ifdef KERBEROS
#ifdef CRYPT
		if (doencrypt)
			wc = des_write(rem, bp, cc);
		else
#endif
#endif
			wc = write(rem, bp, cc);
		if (wc < 0) {
			if (errno == EWOULDBLOCK)
				goto rewrite;
			goto done;
		}
		bp += wc;
		cc -= wc;
		if (cc == 0)
			goto reread;
		goto rewrite;
done:
		(void)shutdown(rem, 1);
		exit(0);
	}

	(void)sigsetmask(omask);
	fds[0].events = fds[1].events = POLLIN;
	fds[0].fd = rfd2;
	fds[1].fd = rem;
	fdp = &fds[0];
	nfds = 2;
	do {
		if (poll(fdp, nfds, 0) < 0) {
			if (errno != EINTR)
				err(1, "poll");
			continue;
		}
		if (fds[0].events == POLLIN && (fds[0].revents & POLLIN)) {
			errno = 0;
#ifdef KERBEROS
#ifdef CRYPT
			if (doencrypt)
				cc = des_read(rfd2, buf, sizeof buf);
			else
#endif
#endif
				cc = read(rfd2, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK) {
					nfds--;
					fds[0].events = 0;
					fdp = &fds[1];
				}
			} else
				(void)write(2, buf, cc);
		}
		if (fds[1].events == POLLIN && (fds[1].revents & POLLIN)) {
			errno = 0;
#ifdef KERBEROS
#ifdef CRYPT
			if (doencrypt)
				cc = des_read(rem, buf, sizeof buf);
			else
#endif
#endif
				cc = read(rem, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK) {
					nfds--;
					fds[1].events = 0;
				}
			} else
				(void)write(1, buf, cc);
		}
	} while (nfds);
}

void
sendsig(sig)
	int sig;
{
	char signo;

	signo = sig;
#ifdef KERBEROS
#ifdef CRYPT
	if (doencrypt)
		(void)des_write(rfd2, &signo, 1);
	else
#endif
#endif
		(void)write(rfd2, &signo, 1);
}

#ifdef KERBEROS
/* VARARGS */
void
warning(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;

	(void)fprintf(stderr, "rsh: warning, using standard rsh: ");
	va_start(ap);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, ".\n");
}
#endif

char *
copyargs(argv)
	char **argv;
{
	int cc;
	char **ap, *args, *p;

	cc = 0;
	for (ap = argv; *ap; ++ap)
		cc += strlen(*ap) + 1;
	if (!(args = malloc((u_int)cc)))
		errx(1, "%s", strerror(ENOMEM));
	for (p = args, *p = '\0', ap = argv; *ap; ++ap) {
		strcat(p, *ap);
		p += strlen(p);
		if (ap[1])
			*p++ = ' ';
	}
	*p = '\0';
	return (args);
}

void
usage()
{

	(void)fprintf(stderr,
	    "usage: %s [-nd%s]%s[-l login]%s [login@]host %s\n", __progname,
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
