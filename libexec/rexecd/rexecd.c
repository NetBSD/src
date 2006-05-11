/*	$NetBSD: rexecd.c,v 1.24 2006/05/11 00:22:52 mrg Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "from: @(#)rexecd.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: rexecd.c,v 1.24 2006/05/11 00:22:52 mrg Exp $");
#endif
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <paths.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/openpam.h>
#endif

int main(int, char *[]);
static void rexecd_errx(int, const char *, ...)
     __attribute__((__noreturn__, __format__(__printf__, 2, 3)));
static void doit(struct sockaddr *) __attribute__((__noreturn__));
static void getstr(char *, int, const char *);
static void usage(void) __attribute__((__noreturn__));

#ifdef USE_PAM
static pam_handle_t *pamh;
static struct pam_conv pamc = {
	openpam_nullconv,
	NULL
};
static int pam_flags = PAM_SILENT|PAM_DISALLOW_NULL_AUTHTOK;
static int pam_err;
#define pam_ok(err) ((pam_err = (err)) == PAM_SUCCESS)
#endif

extern char	**environ;
static int	dolog;
#ifndef USE_PAM
static char	username[32 + 1] = "USER=";
static char	logname[32 + 3 + 1] = "LOGNAME=";
static char	homedir[PATH_MAX + 1] = "HOME=";
static char	shell[PATH_MAX + 1] = "SHELL=";
static char	path[sizeof(_PATH_DEFPATH) + sizeof("PATH=")] = "PATH=";
static char	*envinit[] = { homedir, shell, path, username, logname, 0 };
#endif

/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */
int
main(int argc, char *argv[])
{
	struct sockaddr_storage from;
	socklen_t fromlen;
	int ch;

	while ((ch = getopt(argc, argv, "l")) != -1)
		switch (ch) {
		case 'l':
			dolog = 1;
			openlog("rexecd", LOG_PID, LOG_DAEMON);
			break;
		default:
			usage();
		}

	fromlen = sizeof(from);
	if (getpeername(STDIN_FILENO, (struct sockaddr *)&from, &fromlen) < 0)
		err(EXIT_FAILURE, "getpeername");

	if (((struct sockaddr *)&from)->sa_family == AF_INET6 &&
	    IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)&from)->sin6_addr)) {
		char hbuf[NI_MAXHOST];
		if (getnameinfo((struct sockaddr *)&from, fromlen, hbuf,
		    sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0) {
			(void)strlcpy(hbuf, "invalid", sizeof(hbuf));
		}
		if (dolog)
			syslog(LOG_ERR,
			    "malformed \"from\" address (v4 mapped, %s)",
			    hbuf);
		return EXIT_FAILURE;
	}

	doit((struct sockaddr *)&from);
}

void
doit(struct sockaddr *fromp)
{
	struct pollfd fds[2];
	char cmdbuf[NCARGS + 1];
	const char *cp;
	char user[16], pass[16];
	char buf[BUFSIZ], sig;
	struct passwd *pwd, pwres;
	int s = -1; /* XXX gcc */
	int pv[2], pid, cc;
	int one = 1;
	in_port_t port;
	char hostname[2 * MAXHOSTNAMELEN + 1];
	char pbuf[NI_MAXSERV];
	const int niflags = NI_NUMERICHOST | NI_NUMERICSERV;
#ifndef USE_PAM
	char *namep;
#endif
	char pwbuf[1024];

	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGTERM, SIG_DFL);
	(void)dup2(STDIN_FILENO, STDOUT_FILENO);
	(void)dup2(STDIN_FILENO, STDERR_FILENO);

	if (getnameinfo(fromp, fromp->sa_len, hostname, sizeof(hostname),
	    pbuf, sizeof(pbuf), niflags) != 0) {
		if (dolog)
			syslog(LOG_ERR, "malformed \"from\" address (af %d)",
			       fromp->sa_family);
		exit(EXIT_FAILURE);
	}

	(void)alarm(60);
	port = 0;
	for (;;) {
		char c;
		if (read(STDIN_FILENO, &c, 1) != 1) {
			if (dolog)
				syslog(LOG_ERR, "initial read failed");
			exit(EXIT_FAILURE);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	if (port != 0) {
		s = socket(fromp->sa_family, SOCK_STREAM, 0);
		if (s < 0) {
			if (dolog)
				syslog(LOG_ERR, "socket: %m");
			exit(EXIT_FAILURE);
		}
		(void)alarm(60);
		switch (fromp->sa_family) {
		case AF_INET:
			((struct sockaddr_in *)fromp)->sin_port = htons(port);
			break;
		case AF_INET6:
			((struct sockaddr_in6 *)fromp)->sin6_port = htons(port);
			break;
		default:
			syslog(LOG_ERR, "unsupported address family");
			exit(EXIT_FAILURE);
		}
		if (connect(s, (struct sockaddr *)fromp, fromp->sa_len) < 0) {
			if (dolog)
				syslog(LOG_ERR, "connect: %m");
			exit(EXIT_FAILURE);
		}
		(void)alarm(0);
	}
	(void)alarm(60);
	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	(void)alarm(0);
	if (getpwnam_r(user, &pwres, pwbuf, sizeof(pwbuf), &pwd) != 0 ||
	    pwd == NULL) {
		if (dolog)
			syslog(LOG_ERR, "no such user %s", user);
		rexecd_errx(EXIT_FAILURE, "Login incorrect.");
	}
#ifdef USE_PAM
	if (!pam_ok(pam_start("rexecd", user, &pamc, &pamh)) ||
	    !pam_ok(pam_set_item(pamh, PAM_RHOST, hostname)) ||
	    !pam_ok(pam_set_item(pamh, PAM_AUTHTOK, pass))) {
		if (dolog)
			syslog(LOG_ERR, "PAM ERROR %s@%s (%s)", user,
			   hostname, pam_strerror(pamh, pam_err));
		rexecd_errx(EXIT_FAILURE, "Try again.");
	}
	if (!pam_ok(pam_authenticate(pamh, pam_flags)) ||
	    !pam_ok(pam_acct_mgmt(pamh, pam_flags))) {
		if (dolog)
			syslog(LOG_ERR, "LOGIN REFUSED for %s@%s (%s)", user,
			   hostname, pam_strerror(pamh, pam_err));
		rexecd_errx(EXIT_FAILURE, "Password incorrect.");
	}
#else
	if (*pwd->pw_passwd != '\0') {
		namep = crypt(pass, pwd->pw_passwd);
		if (strcmp(namep, pwd->pw_passwd) != 0) {
			if (dolog)
				syslog(LOG_ERR, "incorrect password for %s",
				    user);
			rexecd_errx(EXIT_FAILURE,
				    "Password incorrect.");/* XXX: wrong! */
		}
	} else
		(void)crypt("dummy password", "PA");    /* must always crypt */
#endif
	if (chdir(pwd->pw_dir) < 0) {
		if (dolog)
			syslog(LOG_ERR, "%s does not exist for %s", pwd->pw_dir,
			       user);
		rexecd_errx(EXIT_FAILURE, "No remote directory.");
	}

	if (dolog)
		syslog(LOG_INFO, "login from %s as %s", hostname, user);
	(void)write(STDERR_FILENO, "\0", 1);
	if (port) {
		if (pipe(pv) < 0 || (pid = fork()) == -1) {
			if (dolog)
				syslog(LOG_ERR,"pipe or fork failed for %s: %m",
				    user);
			rexecd_errx(EXIT_FAILURE, "Try again.");
		}
		if (pid) {
			/* parent */
#ifdef USE_PAM
			(void)pam_end(pamh, pam_err);
#endif
			(void)close(STDIN_FILENO);
			(void)close(STDOUT_FILENO);
			(void)close(STDERR_FILENO);
			(void)close(pv[1]);
			fds[0].fd = s;
			fds[1].fd = pv[0];
			fds[0].events = fds[1].events = POLLIN;
			if (ioctl(pv[1], FIONBIO, (char *)&one) < 0)
				_exit(1);
			/* should set s nbio! */
			do {
				if (poll(fds, 2, 0) < 0) {
					(void)close(s);
					(void)close(pv[0]);
					_exit(1);
				}
				if (fds[0].revents & POLLIN) {
					if (read(s, &sig, 1) <= 0)
						fds[0].events = 0;
					else
						(void)killpg(pid, sig);
				}
				if (fds[1].revents & POLLIN) {
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						(void)shutdown(s, SHUT_RDWR);
						fds[1].events = 0;
					} else
						(void)write(s, buf, cc);
				}
			} while ((fds[0].events | fds[1].events) & POLLIN);
			_exit(0);
		}
		/* child */
		(void)close(s);
		(void)close(pv[0]);
		if (dup2(pv[1], STDERR_FILENO) < 0) {
			if (dolog)
				syslog(LOG_ERR, "dup2 failed for %s", user);
			rexecd_errx(EXIT_FAILURE, "Try again.");
		}
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = __UNCONST(_PATH_BSHELL);
	if (setsid() < 0 ||
	    setlogin(pwd->pw_name) < 0 ||
	    initgroups(pwd->pw_name, pwd->pw_gid) < 0 ||
#ifdef USE_PAM
	    setgid((gid_t)pwd->pw_gid) < 0) {
#else
	    setgid((gid_t)pwd->pw_gid) < 0 ||
	    setuid((uid_t)pwd->pw_uid) < 0) {
#endif
		rexecd_errx(EXIT_FAILURE, "Try again.");
		if (dolog)
			syslog(LOG_ERR, "could not set permissions for %s: %m",
			    user);
		exit(EXIT_FAILURE);
	}
#ifdef USE_PAM
	if (!pam_ok(pam_setcred(pamh, PAM_ESTABLISH_CRED)))
		syslog(LOG_ERR, "pam_setcred() failed: %s",
		       pam_strerror(pamh, pam_err));
	(void)pam_setenv(pamh, "HOME", pwd->pw_dir, 1);
	(void)pam_setenv(pamh, "SHELL", pwd->pw_shell, 1);
	(void)pam_setenv(pamh, "USER", pwd->pw_name, 1);
	(void)pam_setenv(pamh, "LOGNAME", pwd->pw_name, 1);
	(void)pam_setenv(pamh, "PATH", _PATH_DEFPATH, 1);
	environ = pam_getenvlist(pamh);
	(void)pam_end(pamh, pam_err);
	if (setuid((uid_t)pwd->pw_uid) < 0) {
                if (dolog)
                        syslog(LOG_ERR, "could not set uid for %s: %m",
                            user);
                rexecd_errx(EXIT_FAILURE, "Try again.");
        }
#else
	(void)strlcat(path, _PATH_DEFPATH, sizeof(path));
	environ = envinit;
	(void)strlcat(homedir, pwd->pw_dir, sizeof(homedir));
	(void)strlcat(shell, pwd->pw_shell, sizeof(shell));
	(void)strlcat(username, pwd->pw_name, sizeof(username));
	(void)strlcat(logname, pwd->pw_name, sizeof(logname));
#endif

	cp = strrchr(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	if (dolog)
		syslog(LOG_INFO, "running command for %s: %s", user, cmdbuf);
	(void)execl(pwd->pw_shell, cp, "-c", cmdbuf, NULL);
	if (dolog)
		syslog(LOG_ERR, "execl failed for %s: %m", user);
	err(EXIT_FAILURE, "%s", pwd->pw_shell);
}

void
rexecd_errx(int ex, const char *fmt, ...)
{
	char buf[BUFSIZ];
	va_list ap;
	ssize_t len;

	va_start(ap, fmt);
	buf[0] = 1;
	len = vsnprintf(buf + 1, sizeof(buf) - 1, fmt, ap) + 1;
	buf[len++] = '\n';
	(void)write(STDERR_FILENO, buf, len);
	va_end(ap);
	exit(ex);
}

void
getstr(char *buf, int cnt, const char *emsg)
{
	char c;

	do {
		if (read(STDIN_FILENO, &c, 1) != 1)
			exit(EXIT_FAILURE);
		*buf++ = c;
		if (--cnt == 0)
			rexecd_errx(EXIT_FAILURE, "%s too long", emsg);
	} while (c != 0);
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-l]\n", getprogname());
	exit(EXIT_FAILURE);
}
