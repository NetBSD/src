/*	$NetBSD: rexecd.c,v 1.16 2003/05/17 22:56:31 itojun Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#if 0
static char sccsid[] = "from: @(#)rexecd.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: rexecd.c,v 1.16 2003/05/17 22:56:31 itojun Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

void error __P((const char *, ...))
     __attribute__((__format__(__printf__, 1, 2)));
int main __P((int, char **));
void doit __P((int, struct sockaddr *));
void getstr __P((char *, int, char *));

char	username[32 + 1] = "USER=";
char	homedir[PATH_MAX + 1] = "HOME=";
char	shell[PATH_MAX + 1] = "SHELL=";
char	path[sizeof(_PATH_DEFPATH) + sizeof("PATH=")] = "PATH=";
char	*envinit[] = { homedir, shell, path, username, 0 };
char	**environ;
int	dolog;

/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_storage from;
	int fromlen, ch;

	while ((ch = getopt(argc, argv, "l")) != -1)
		switch (ch) {
		case 'l':
			dolog = 1;
			openlog("rexecd", LOG_PID, LOG_DAEMON);
			break;
		default:
			exit(1);
		}

	fromlen = sizeof (from);
	if (getpeername(0, (struct sockaddr *)&from, &fromlen) < 0)
		err(1, "getpeername");

	doit(0, (struct sockaddr *)&from);
	exit(0);
}

void
doit(f, fromp)
	int f;
	struct sockaddr *fromp;
{
	struct pollfd fds[2];
	char cmdbuf[NCARGS+1], *namep;
	const char *cp;
	char user[16], pass[16];
	char buf[BUFSIZ], sig;
	struct passwd *pwd;
	int s = -1; /* XXX gcc */
	int pv[2], pid, cc;
	int one = 1;
	in_port_t port;

	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGTERM, SIG_DFL);
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	(void)alarm(60);
	port = 0;
	for (;;) {
		char c;
		if (read(f, &c, 1) != 1) {
			if (dolog)
				syslog(LOG_ERR,
				    "initial read failed");
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void)alarm(0);
	if (port != 0) {
		s = socket(fromp->sa_family, SOCK_STREAM, 0);
		if (s < 0) {
			if (dolog)
				syslog(LOG_ERR, "socket: %m");
			exit(1);
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
			exit(1);
		}
		if (connect(s, (struct sockaddr *)fromp, fromp->sa_len) < 0) {
			if (dolog)
				syslog(LOG_ERR, "connect: %m");
			exit(1);
		}
		(void)alarm(0);
	}
	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
	setpwent();
	pwd = getpwnam(user);
	if (pwd == NULL) {
		error("Login incorrect.\n");
		if (dolog)
			syslog(LOG_ERR, "no such user %s", user);
		exit(1);
	}
	endpwent();
	if (*pwd->pw_passwd != '\0') {
		namep = crypt(pass, pwd->pw_passwd);
		if (strcmp(namep, pwd->pw_passwd)) {
			error("Password incorrect.\n");	/* XXX: wrong! */
			if (dolog)
				syslog(LOG_ERR, "incorrect password for %s",
				    user);
			exit(1);
		}
	} else
		(void)crypt("dummy password", "PA");	/* must always crypt */
	if (chdir(pwd->pw_dir) < 0) {
		error("No remote directory.\n");
		if (dolog)
			syslog(LOG_ERR, "%s does not exist for %s", pwd->pw_dir,
			    user);
		exit(1);
	}
	(void)write(2, "\0", 1);
	if (port) {
		if (pipe(pv) < 0 || (pid = fork()) == -1) {
			error("Try again.\n");
			if (dolog)
				syslog(LOG_ERR,"pipe or fork failed for %s: %m",
				    user);
			exit(1);
		}
		if (pid) {
			(void)close(0);
			(void)close(1);
			(void)close(2);
			(void)close(f);
			(void)close(pv[1]);
			fds[0].fd = s;
			fds[1].fd = pv[0];
			fds[0].events = fds[1].events = POLLIN;
			if (ioctl(pv[1], FIONBIO, (char *)&one) < 0)
				_exit(1);
			/* should set s nbio! */
			do {
				if (poll(fds, 2, 0) < 0) {
					close(s);
					close(pv[0]);
					_exit(1);
				}
				if (fds[0].revents & POLLIN) {
					if (read(s, &sig, 1) <= 0)
						fds[0].events = 0;
					else
						killpg(pid, sig);
				}
				if (fds[1].revents & POLLIN) {
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						shutdown(s, 1+1);
						fds[1].events = 0;
					} else
						(void)write(s, buf, cc);
				}
			} while ((fds[0].events | fds[1].events) & POLLIN);
			_exit(0);
		}
		(void)close(s);
		(void)close(pv[0]);
		if (dup2(pv[1], 2) < 0) {
			error("Try again.\n");
			if (dolog)
				syslog(LOG_ERR, "dup2 failed for %s", user);
			exit(1);
		}
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;
	if (f > 2)
		(void)close(f);
	if (setsid() < 0 ||
	    setlogin(pwd->pw_name) < 0 ||
	    initgroups(pwd->pw_name, pwd->pw_gid) < 0 ||
	    setgid((gid_t)pwd->pw_gid) < 0 || 
	    setuid((uid_t)pwd->pw_uid) < 0) {
		error("Try again.\n");
		if (dolog)
			syslog(LOG_ERR, "could not set permissions for %s: %m",
			    user);
		exit(1);
	}
	(void)strlcat(path, _PATH_DEFPATH, sizeof(path));
	environ = envinit;
	strlcat(homedir, pwd->pw_dir, sizeof(homedir));
	strlcat(shell, pwd->pw_shell, sizeof(shell));
	strlcat(username, pwd->pw_name, sizeof(username));
	cp = strrchr(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	if (dolog)
		syslog(LOG_INFO, "running command for %s: %s", user, cmdbuf);
	execl(pwd->pw_shell, cp, "-c", cmdbuf, 0);
	perror(pwd->pw_shell);
	if (dolog)
		syslog(LOG_ERR, "execl failed for %s: %m", user);
	exit(1);
}

void
error(const char *fmt, ...)
{
	char buf[BUFSIZ];
	va_list ap;

	va_start(ap, fmt);
	buf[0] = 1;
	(void)vsnprintf(buf+1, sizeof(buf) - 1, fmt, ap);
	(void)write(2, buf, strlen(buf));
	va_end(ap);
}

void
getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err);
			exit(1);
		}
	} while (c != 0);
}
