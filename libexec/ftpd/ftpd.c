/*	$NetBSD: ftpd.c,v 1.65 1999/05/24 21:57:19 ross Exp $	*/

/*
 * Copyright (c) 1985, 1988, 1990, 1992, 1993, 1994
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

/*
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT(
"@(#) Copyright (c) 1985, 1988, 1990, 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ftpd.c	8.5 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: ftpd.c,v 1.65 1999/05/24 21:57:19 ross Exp $");
#endif
#endif /* not lint */

/*
 * FTP server.
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define	FTP_NAMES
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#ifdef SKEY
#include <skey.h>
#endif
#ifdef KERBEROS5
#include <krb5.h>
#endif

#include "extern.h"
#include "pathnames.h"

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
 
#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

const char version[] = "Version: 7.2.0";

struct	sockaddr_in ctrl_addr;
struct	sockaddr_in data_source;
struct	sockaddr_in data_dest;
struct	sockaddr_in his_addr;
struct	sockaddr_in pasv_addr;

int	data;
jmp_buf	errcatch, urgcatch;
int	logged_in;
struct	passwd *pw;
int	debug;
int	sflag;
int	logging;
int	guest;
int	dochroot;
int	type;
int	form;
int	stru;			/* avoid C keyword */
int	mode;
int	usedefault = 1;		/* for data transfers */
int	pdata = -1;		/* for passive mode */
sig_atomic_t transflag;
off_t	file_size;
off_t	byte_count;
char	tmpline[7];
char	hostname[MAXHOSTNAMELEN+1];
char	remotehost[MAXHOSTNAMELEN+1];
static char ttyline[20];
char	*tty = ttyline;		/* for klogin */

off_t	total_data_in;		/* total file data bytes received */
off_t	total_data_out;		/* total file data bytes sent data */
off_t	total_data;		/* total file data bytes transferred data */
off_t	total_files_in;		/* total number of data files received */ 
off_t	total_files_out;	/* total number of data files sent */ 
off_t	total_files;		/* total number of data files transferred */ 
off_t	total_bytes_in;		/* total bytes received */
off_t	total_bytes_out;	/* total bytes sent */
off_t	total_bytes;		/* total bytes transferred */
off_t	total_xfers_in;		/* total number of xfers incoming */
off_t	total_xfers_out;	/* total number of xfers outgoing */
off_t	total_xfers;		/* total number of xfers */

static char *anondir = NULL;
static char confdir[MAXPATHLEN];

#if defined(KERBEROS) || defined(KERBEROS5)
int	notickets = 1;
char	*krbtkfile_env = NULL;
#endif 

/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */
#define	SWAITMAX	90	/* wait at most 90 seconds */
#define	SWAITINT	5	/* interval between retries */

int	swaitmax = SWAITMAX;
int	swaitint = SWAITINT;

#ifdef HASSETPROCTITLE
char	proctitle[BUFSIZ];	/* initial part of title */
#endif /* HASSETPROCTITLE */

static void	 ack __P((const char *));
static void	 myoob __P((int));
static int	 checkuser __P((const char *, const char *, int, int));
static int	 checkaccess __P((const char *));
static FILE	*dataconn __P((const char *, off_t, const char *));
static void	 dolog __P((struct sockaddr_in *));
static void	 end_login __P((void));
static FILE	*getdatasock __P((const char *));
static char	*gunique __P((const char *));
static void	 lostconn __P((int));
static int	 receive_data __P((FILE *, FILE *));
static void	 replydirname __P((const char *, const char *));
static int	 send_data __P((FILE *, FILE *, off_t, int));
static struct passwd *
		 sgetpwnam __P((const char *));

int	main __P((int, char *[]));

#if defined(KERBEROS) || defined(KERBEROS5)
int	klogin __P((struct passwd *, char *, char *, char *));
void	kdestroy __P((void));
#endif

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int addrlen, ch, on = 1, tos;
	char *cp, line[LINE_MAX];
	FILE *fd;
#ifdef KERBEROS5  
	krb5_error_code kerror;
#endif

	debug = 0;
	logging = 0;
	sflag = 0;
	(void)strcpy(confdir, _DEFAULT_CONFDIR);

	while ((ch = getopt(argc, argv, "a:c:C:dlst:T:u:v")) != -1) {
		switch (ch) {
		case 'a':
			anondir = optarg;
			break;

		case 'c':
			(void)strncpy(confdir, optarg, sizeof(confdir));
			confdir[sizeof(confdir)-1] = '\0';
			break;
 
		case 'C':
			exit(checkaccess(optarg));
			/* NOTREACHED */

		case 'd':
		case 'v':		/* deprecated */
			debug = 1;
			break;

		case 'l':
			logging++;	/* > 1 == extra logging */
			break;

		case 's':
			sflag = 1;
			break;

		case 't':
		case 'T':
		case 'u':
			warnx("-%c has been deprecated in favour of ftpd.conf",
			    ch);
			break;

		default:
			if (optopt == 'a' || optopt == 'C')
				exit(1);
			warnx("unknown flag -%c ignored", optopt);
			break;
		}
	}

	/*
	 * LOG_NDELAY sets up the logging connection immediately,
	 * necessary for anonymous ftp's that chroot and can't do it later.
	 */
	openlog("ftpd", LOG_PID | LOG_NDELAY, LOG_FTP);
	addrlen = sizeof(his_addr);
	if (getpeername(0, (struct sockaddr *)&his_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getpeername (%s): %m",argv[0]);
		exit(1);
	}
	addrlen = sizeof(ctrl_addr);
	if (getsockname(0, (struct sockaddr *)&ctrl_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getsockname (%s): %m",argv[0]);
		exit(1);
	}
#ifdef IP_TOS
	tos = IPTOS_LOWDELAY;
	if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	data_source.sin_port = htons(ntohs(ctrl_addr.sin_port) - 1);

	/* set this here so klogin can use it... */
	(void)snprintf(ttyline, sizeof(ttyline), "ftp%d", getpid());

	(void) freopen(_PATH_DEVNULL, "w", stderr);
	(void) signal(SIGPIPE, lostconn);
	(void) signal(SIGCHLD, SIG_IGN);
	if ((long)signal(SIGURG, myoob) < 0)
		syslog(LOG_ERR, "signal: %m");

	/* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
	if (setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on)) < 0)
		syslog(LOG_ERR, "setsockopt: %m");
#endif

#ifdef	F_SETOWN
	if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
		syslog(LOG_ERR, "fcntl F_SETOWN: %m");
#endif
	dolog(&his_addr);
	/*
	 * Set up default state
	 */
	data = -1;
	type = TYPE_A;
	form = FORM_N;
	stru = STRU_F;
	mode = MODE_S;
	tmpline[0] = '\0';
	hasyyerrored = 0;

#ifdef KERBEROS5
	kerror = krb5_init_context(&kcontext);
	if (kerror) {
		syslog(LOG_NOTICE, "%s when initializing Kerberos context",
		    error_message(kerror));
		exit(0);
	}
#endif /* KERBEROS5 */

	/* If logins are disabled, print out the message. */
	if ((fd = fopen(_PATH_NOLOGIN,"r")) != NULL) {
		lreply(530, "");
		while (fgets(line, sizeof(line), fd) != NULL) {
			if ((cp = strchr(line, '\n')) != NULL)
				*cp = '\0';
			lreply(0, "%s", line);
		}
		(void) fflush(stdout);
		(void) fclose(fd);
		reply(530, "System not available.");
		exit(0);
	}
	if ((fd = fopen(conffilename(_PATH_FTPWELCOME), "r")) != NULL) {
		lreply(220, "");
		while (fgets(line, sizeof(line), fd) != NULL) {
			if ((cp = strchr(line, '\n')) != NULL)
				*cp = '\0';
			lreply(0, "%s", line);
		}
		(void) fflush(stdout);
		(void) fclose(fd);
		/* reply(220,) must follow */
	}
	(void)gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	reply(220, "%s FTP server (%s) ready.", hostname, version);
	curclass.timeout = 300;		/* 5 minutes, as per login(1) */
	(void) setjmp(errcatch);
	for (;;)
		(void) yyparse();
	/* NOTREACHED */
}

static void
lostconn(signo)
	int signo;
{

	if (debug)
		syslog(LOG_DEBUG, "lost connection");
	dologout(1);
}

/*
 * Save the result of a getpwnam.  Used for USER command, since
 * the data returned must not be clobbered by any other command
 * (e.g., globbing).
 */
static struct passwd *
sgetpwnam(name)
	const char *name;
{
	static struct passwd save;
	struct passwd *p;

	if ((p = getpwnam(name)) == NULL)
		return (p);
	if (save.pw_name) {
		free((char *)save.pw_name);
		free((char *)save.pw_passwd);
		free((char *)save.pw_gecos);
		free((char *)save.pw_dir);
		free((char *)save.pw_shell);
	}
	save = *p;
	save.pw_name = xstrdup(p->pw_name);
	save.pw_passwd = xstrdup(p->pw_passwd);
	save.pw_gecos = xstrdup(p->pw_gecos);
	save.pw_dir = xstrdup(p->pw_dir);
	save.pw_shell = xstrdup(p->pw_shell);
	return (&save);
}

static int login_attempts;	/* number of failed login attempts */
static int askpasswd;		/* had user command, ask for passwd */
static char curname[10];	/* current USER name */

/*
 * USER command.
 * Sets global passwd pointer pw if named account exists and is acceptable;
 * sets askpasswd if a PASS command is expected.  If logged in previously,
 * need to reset state.  If name is "ftp" or "anonymous", the name is not in
 * _PATH_FTPUSERS, and ftp account exists, set guest and pw, then just return.
 * If account doesn't exist, ask for passwd anyway.  Otherwise, check user
 * requesting login privileges.  Disallow anyone who does not have a standard
 * shell as returned by getusershell().  Disallow anyone mentioned in the file
 * _PATH_FTPUSERS to allow people such as root and uucp to be avoided.
 */
void
user(name)
	const char *name;
{
	if (logged_in) {
		if (guest) {
			reply(530, "Can't change user from guest login.");
			return;
		} else if (dochroot) {
			reply(530, "Can't change user from chroot user.");
			return;
		}
		end_login();
	}

#if defined(KERBEROS) || defined(KERBEROS5)
	kdestroy();
#endif

	guest = 0;
	if (strcmp(name, "ftp") == 0 || strcmp(name, "anonymous") == 0) {
		if (checkaccess("ftp") || checkaccess("anonymous"))
			reply(530, "User %s access denied.", name);
		else if ((pw = sgetpwnam("ftp")) != NULL) {
			guest = 1;
			askpasswd = 1;
			reply(331,
			    "Guest login ok, type your name as password.");
		} else
			reply(530, "User %s unknown.", name);
		if (!askpasswd && logging)
			syslog(LOG_NOTICE,
			    "ANONYMOUS FTP LOGIN REFUSED FROM %s", remotehost);
		return;
	}

	pw = sgetpwnam(name);
	if (logging)
		strncpy(curname, name, sizeof(curname)-1);

#ifdef SKEY
	if (skey_haskey(name) == 0) {
		char *myskey;

		myskey = skey_keyinfo(name);
		reply(331, "Password [%s] required for %s.",
		    myskey ? myskey : "error getting challenge", name);
	} else
#endif
		reply(331, "Password required for %s.", name);

	askpasswd = 1;
	/*
	 * Delay before reading passwd after first failed
	 * attempt to slow down passwd-guessing programs.
	 */
	if (login_attempts)
		sleep((unsigned) login_attempts);
}

/*
 * Determine whether something is to happen (allow access, chroot)
 * for a user. Each line is a shell-style glob followed by
 * `yes' or `no'.
 *
 * For backward compatability, `allow' and `deny' are synonymns
 * for `yes' and `no', respectively.
 *
 * Each glob is matched against the username in turn, and the first
 * match found is used. If no match is found, the result is the
 * argument `def'. If a match is found but without and explicit
 * `yes'/`no', the result is the opposite of def.
 *
 * If the file doesn't exist at all, the result is the argument
 * `nofile'
 *
 * Any line starting with `#' is considered a comment and ignored.
 *
 * Returns FALSE if the user is denied, or TRUE if they are allowed.
 */
int
checkuser(fname, name, def, nofile)
	const char *fname, *name;
	int def, nofile;
{
	FILE	*fd;
	int	 retval;
	char	*glob, *perm, line[BUFSIZ];

	retval = def;
	if ((fd = fopen(conffilename(fname), "r")) == NULL)
		return nofile;

	while (fgets(line, sizeof(line), fd) != NULL)  {
		glob = strtok(line, " \t\n");
		if (glob[0] == '#')
			continue;
		perm = strtok(NULL, " \t\n");
		if (fnmatch(glob, name, 0) == 0)  {
			if (perm != NULL &&
			    ((strcasecmp(perm, "allow") == 0) ||
			    (strcasecmp(perm, "yes") == 0)))
				retval = TRUE;
			else if (perm != NULL &&
			    ((strcasecmp(perm, "deny") == 0) ||
			    (strcasecmp(perm, "no") == 0)))
				retval = FALSE;
			else
				retval = !def;
			break;
		}
	}
	(void) fclose(fd);
	return (retval);
}

/*
 * Check if user is allowed by /etc/ftpusers
 * returns 0 for yes, 1 for no
 */
int
checkaccess(name)
	const char *name;
{

	return (! checkuser(_PATH_FTPUSERS, name, TRUE, FALSE));
}

/*
 * Terminate login as previous user, if any, resetting state;
 * used when USER command is given or login fails.
 */
static void
end_login()
{

	(void) seteuid((uid_t)0);
	if (logged_in)
		logwtmp(ttyline, "", "");
	pw = NULL;
	logged_in = 0;
	guest = 0;
	dochroot = 0;
}

void
pass(passwd)
	const char *passwd;
{
	int rval;
	FILE *fd;
	char const *cp, *shell, *home;

	if (logged_in || askpasswd == 0) {
		reply(503, "Login with USER first.");
		return;
	}
	askpasswd = 0;
	if (!guest) {		/* "ftp" is only account allowed no password */
		if (pw == NULL) {
			rval = 1;	/* failure below */
			goto skip;
		}
#ifdef KERBEROS
		if (klogin(pw, "", hostname, (char *)passwd) == 0) {
			rval = 0;
			goto skip;
		}
#endif
#ifdef KERBEROS5
		if (klogin(pw, "", hostname, (char *)passwd) == 0) {
			rval = 0;
			goto skip;
		}
#endif
#ifdef SKEY
		if (skey_haskey(pw->pw_name) == 0) {
			char *p;
			int r;

			p = xstrdup(passwd);
			r = skey_passcheck(pw->pw_name, p);
			free(p);
			if (r != -1) {
				rval = 0;
				goto skip;
			}
		}
#endif
		if (!sflag && *pw->pw_passwd != '\0' &&
		    !strcmp(crypt(passwd, pw->pw_passwd), pw->pw_passwd)) {
			rval = 0;
			goto skip;
		}
		rval = 1;

skip:
		if (pw != NULL && pw->pw_expire && time(NULL) >= pw->pw_expire)
			rval = 2;
		/*
		 * If rval > 0, the user failed the authentication check
		 * above.  If rval == 0, either Kerberos or local authentication
		 * succeeded.
		 */
		if (rval) {
			reply(530, rval == 2 ? "Password expired." :
			    "Login incorrect.");
			if (logging) {
				syslog(LOG_NOTICE,
				    "FTP LOGIN FAILED FROM %s", remotehost);
				syslog(LOG_AUTHPRIV | LOG_NOTICE,
				    "FTP LOGIN FAILED FROM %s, %s",
				    remotehost, curname);
			}
			pw = NULL;
			if (login_attempts++ >= 5) {
				syslog(LOG_NOTICE,
				    "repeated login failures from %s",
				    remotehost);
				exit(0);
			}
			return;
		}
	}

	/* password was ok; see if anything else prevents login */
	if (checkaccess(pw->pw_name))  {
		reply(530, "User %s may not use FTP.", pw->pw_name);
		if (logging)
			syslog(LOG_NOTICE, "FTP LOGIN REFUSED FROM %s, %s",
			    remotehost, pw->pw_name);
		pw = (struct passwd *) NULL;
		return;
	}
	/* check for valid shell, if not guest user */
	if ((shell = pw->pw_shell) == NULL || *shell == 0)
		shell = _PATH_BSHELL;
	while ((cp = getusershell()) != NULL)
		if (strcmp(cp, shell) == 0)
			break;
	endusershell();
	if (cp == NULL && guest == 0) {
		reply(530, "User %s may not use FTP.", pw->pw_name);
		if (logging)
			syslog(LOG_NOTICE, "FTP LOGIN REFUSED FROM %s, %s",
			    remotehost, pw->pw_name);
		pw = (struct passwd *) NULL;
		return;
	}

	login_attempts = 0;		/* this time successful */
	if (setegid((gid_t)pw->pw_gid) < 0) {
		reply(550, "Can't set gid.");
		return;
	}
	(void) initgroups(pw->pw_name, pw->pw_gid);

	/* open wtmp before chroot */
	logwtmp(ttyline, pw->pw_name, remotehost);
	logged_in = 1;

	dochroot = checkuser(_PATH_FTPCHROOT, pw->pw_name, FALSE, FALSE);

	/* parse ftpd.conf, setting up various parameters */
	if (guest)
		parse_conf(CLASS_GUEST);
	else if (dochroot)
		parse_conf(CLASS_CHROOT);
	else
		parse_conf(CLASS_REAL);

	home = "/";
	if (guest) {
		/*
		 * We MUST do a chdir() after the chroot. Otherwise
		 * the old current directory will be accessible as "."
		 * outside the new root!
		 */
		if (chroot(anondir ? anondir : pw->pw_dir) < 0 ||
		    chdir("/") < 0) {
			reply(550, "Can't set guest privileges.");
			goto bad;
		}
	} else if (dochroot) {
		if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
			reply(550, "Can't change root.");
			goto bad;
		}
	} else if (chdir(pw->pw_dir) < 0) {
		if (chdir("/") < 0) {
			reply(530, "User %s: can't change directory to %s.",
			    pw->pw_name, pw->pw_dir);
			goto bad;
		} else
			lreply(230, "No directory! Logging in with home=/");
	} else
		home = pw->pw_dir;
	if (seteuid((uid_t)pw->pw_uid) < 0) {
		reply(550, "Can't set uid.");
		goto bad;
	}
	setenv("HOME", home, 1);


	/*
	 * Display a login message, if it exists.
	 * N.B. reply(230,) must follow the message.
	 */
	if ((fd = fopen(conffilename(_PATH_FTPLOGINMESG), "r")) != NULL) {
		char *cp, line[LINE_MAX];

		lreply(230, "");
		while (fgets(line, sizeof(line), fd) != NULL) {
			if ((cp = strchr(line, '\n')) != NULL)
				*cp = '\0';
			lreply(0, "%s", line);
		}
		(void) fflush(stdout);
		(void) fclose(fd);
	}
	show_chdir_messages(230);
	if (guest) {
		reply(230, "Guest login ok, access restrictions apply.");
#ifdef HASSETPROCTITLE
		snprintf(proctitle, sizeof(proctitle),
		    "%s: anonymous/%.*s", remotehost,
		    (int) (sizeof(proctitle) - sizeof(remotehost) -
		    sizeof(": anonymous/")), passwd);
		setproctitle(proctitle);
#endif /* HASSETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s, %s",
			    remotehost, passwd);
	} else {
		reply(230, "User %s logged in.", pw->pw_name);
#ifdef HASSETPROCTITLE
		snprintf(proctitle, sizeof(proctitle),
		    "%s: %s", remotehost, pw->pw_name);
		setproctitle(proctitle);
#endif /* HASSETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, "FTP LOGIN FROM %s as %s",
			    remotehost, pw->pw_name);
	}
	(void) umask(curclass.umask);
	return;
bad:
	/* Forget all about it... */
	end_login();
}

void
retrieve(cmd, name)
	const char *cmd, *name;
{
	FILE *fin = NULL, *dout;
	struct stat st;
	int (*closefunc) __P((FILE *)) = NULL;
	int log, sendrv, closerv, stderrfd, isconversion, isdata, isls;
	struct timeval start, finish, td, *tdp;
	char tline[BUFSIZ];
	const char *dispname;

	sendrv = closerv = stderrfd = -1;
	isconversion = isdata = isls = log = 0;
	tdp = NULL;
	dispname = name;
	if (cmd == NULL) {
		log = 1;
		isdata = 1;
		fin = fopen(name, "r");
		closefunc = fclose;
		if (fin == NULL)
			cmd = do_conversion(name);
		if (cmd != NULL) {
			isconversion++;
			syslog(LOG_INFO, "get command: '%s'", cmd);
		}
	}
	if (cmd != NULL) {
		char temp[MAXPATHLEN + 1];

		if (strncmp(cmd, INTERNAL_LS, sizeof(INTERNAL_LS) - 1) == 0 &&
		    strchr(" \t\n", cmd[sizeof(INTERNAL_LS) - 1]) != NULL) {
			isls = 1;
			stderrfd = -1;
		} else {
			(void)snprintf(temp, sizeof(temp), "%s", TMPFILE);
			stderrfd = mkstemp(temp);
			if (stderrfd != -1)
				(void)unlink(temp);
		}
		(void)snprintf(tline, sizeof(tline), cmd, name);
		dispname = tline;
		fin = ftpd_popen(tline, "r", stderrfd);
		closefunc = ftpd_pclose;
		st.st_size = -1;
		st.st_blksize = BUFSIZ;
	}
	if (fin == NULL) {
		if (errno != 0) {
			perror_reply(550, dispname);
			if (log)
				logcmd("get", -1, name, NULL, NULL,
				    strerror(errno));
		}
		if (stderrfd != -1)
			(void)close(stderrfd);
		return;
	}
	byte_count = -1;
	if (cmd == NULL
	    && (fstat(fileno(fin), &st) < 0 || !S_ISREG(st.st_mode))) {
		reply(550, "%s: not a plain file.", dispname);
		goto done;
	}
	if (restart_point) {
		if (type == TYPE_A) {
			off_t i;
			int c;

			for (i = 0; i < restart_point; i++) {
				if ((c=getc(fin)) == EOF) {
					perror_reply(550, dispname);
					goto done;
				}
				if (c == '\n')
					i++;
			}
		} else if (lseek(fileno(fin), restart_point, SEEK_SET) < 0) {
			perror_reply(550, dispname);
			goto done;
		}
	}
	dout = dataconn(dispname, st.st_size, "w");
	if (dout == NULL)
		goto done;

	(void)gettimeofday(&start, NULL);
	sendrv = send_data(fin, dout, st.st_blksize, isdata);
	(void)gettimeofday(&finish, NULL);
	(void) fclose(dout);
	timersub(&finish, &start, &td);
	tdp = &td;
	data = -1;
	pdata = -1;
done:
	if (log)
		logcmd("get", byte_count, name, NULL, tdp, NULL);
	closerv = (*closefunc)(fin);
	if (sendrv == 0) {
		FILE *err;
		struct stat sb;

		if (!isls && cmd != NULL && closerv != 0) {
			lreply(226,
			    "Command returned an exit status of %d",
			    closerv);
			if (isconversion)
				syslog(LOG_INFO,
				    "get command: '%s' returned %d",
				    cmd, closerv);
		}
		if (!isls && cmd != NULL && stderrfd != -1 &&
		    (fstat(stderrfd, &sb) == 0) && sb.st_size > 0 &&
		    ((err = fdopen(stderrfd, "r")) != NULL)) {
			char *cp, line[LINE_MAX];

			lreply(226, "Command error messages:");
			rewind(err);
			while (fgets(line, sizeof(line), err) != NULL) {
				if ((cp = strchr(line, '\n')) != NULL)
					*cp = '\0';
				lreply(0, "  %s", line);
			}
			(void) fflush(stdout);
			(void) fclose(err);
				/* a reply(226,) must follow */
		}
		reply(226, "Transfer complete.");
	}
	if (stderrfd != -1)
		(void)close(stderrfd);
}

void
store(name, mode, unique)
	const char *name, *mode;
	int unique;
{
	FILE *fout, *din;
	struct stat st;
	int (*closefunc) __P((FILE *));
	struct timeval start, finish, td, *tdp;
	char *desc;

	desc = (*mode == 'w') ? "put" : "append";
	if (unique && stat(name, &st) == 0 &&
	    (name = gunique(name)) == NULL) {
		logcmd(desc, -1, name, NULL, NULL, "cannot create unique file");
		return;
	}

	if (restart_point)
		mode = "r+";
	fout = fopen(name, mode);
	closefunc = fclose;
	tdp = NULL;
	if (fout == NULL) {
		perror_reply(553, name);
		logcmd(desc, -1, name, NULL, NULL, strerror(errno));
		return;
	}
	byte_count = -1;
	if (restart_point) {
		if (type == TYPE_A) {
			off_t i;
			int c;

			for (i = 0; i < restart_point; i++) {
				if ((c=getc(fout)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}
			/*
			 * We must do this seek to "current" position
			 * because we are changing from reading to
			 * writing.
			 */
			if (fseek(fout, 0L, SEEK_CUR) < 0) {
				perror_reply(550, name);
				goto done;
			}
		} else if (lseek(fileno(fout), restart_point, SEEK_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	din = dataconn(name, (off_t)-1, "r");
	if (din == NULL)
		goto done;
	(void)gettimeofday(&start, NULL);
	if (receive_data(din, fout) == 0) {
		if (unique)
			reply(226, "Transfer complete (unique file name:%s).",
			    name);
		else
			reply(226, "Transfer complete.");
	}
	(void)gettimeofday(&finish, NULL);
	(void) fclose(din);
	timersub(&finish, &start, &td);
	tdp = &td;
	data = -1;
	pdata = -1;
done:
	logcmd(desc, byte_count, name, NULL, tdp, NULL);
	(*closefunc)(fout);
}

static FILE *
getdatasock(mode)
	const char *mode;
{
	int on = 1, s, t, tries;

	if (data >= 0)
		return (fdopen(data, mode));
	(void) seteuid((uid_t)0);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		goto bad;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	    (char *) &on, sizeof(on)) < 0)
		goto bad;
	/* anchor socket to avoid multi-homing problems */
	data_source.sin_len = sizeof(struct sockaddr_in);
	data_source.sin_family = AF_INET;
	data_source.sin_addr = ctrl_addr.sin_addr;
	for (tries = 1; ; tries++) {
		if (bind(s, (struct sockaddr *)&data_source,
		    sizeof(data_source)) >= 0)
			break;
		if (errno != EADDRINUSE || tries > 10)
			goto bad;
		sleep(tries);
	}
	(void) seteuid((uid_t)pw->pw_uid);
#ifdef IP_TOS
	on = IPTOS_THROUGHPUT;
	if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	return (fdopen(s, mode));
bad:
	/* Return the real value of errno (close may change it) */
	t = errno;
	(void) seteuid((uid_t)pw->pw_uid);
	(void) close(s);
	errno = t;
	return (NULL);
}

static FILE *
dataconn(name, size, mode)
	const char *name;
	off_t size;
	const char *mode;
{
	char sizebuf[32];
	FILE *file;
	int retry = 0, tos;

	file_size = size;
	byte_count = 0;
	if (size != (off_t) -1)
		(void)snprintf(sizebuf, sizeof(sizebuf), " (%qd byte%s)",
		    (qdfmt_t)size, PLURAL(size));
	else
		sizebuf[0] = '\0';
	if (pdata >= 0) {
		struct sockaddr_in from;
		int s, fromlen = sizeof(from);

		(void) alarm(curclass.timeout);
		s = accept(pdata, (struct sockaddr *)&from, &fromlen);
		(void) alarm(0);
		if (s < 0) {
			reply(425, "Can't open data connection.");
			(void) close(pdata);
			pdata = -1;
			return (NULL);
		}
		(void) close(pdata);
		pdata = s;
#ifdef IP_TOS
		tos = IPTOS_THROUGHPUT;
		(void) setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&tos,
		    sizeof(int));
#endif
		reply(150, "Opening %s mode data connection for '%s'%s.",
		     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
		return (fdopen(pdata, mode));
	}
	if (data >= 0) {
		reply(125, "Using existing data connection for '%s'%s.",
		    name, sizebuf);
		usedefault = 1;
		return (fdopen(data, mode));
	}
	if (usedefault)
		data_dest = his_addr;
	usedefault = 1;
	file = getdatasock(mode);
	if (file == NULL) {
		reply(425, "Can't create data socket (%s,%d): %s.",
		    inet_ntoa(data_source.sin_addr),
		    ntohs(data_source.sin_port), strerror(errno));
		return (NULL);
	}
	data = fileno(file);
	while (connect(data, (struct sockaddr *)&data_dest,
	    sizeof(data_dest)) < 0) {
		if (errno == EADDRINUSE && retry < swaitmax) {
			sleep((unsigned) swaitint);
			retry += swaitint;
			continue;
		}
		perror_reply(425, "Can't build data connection");
		(void) fclose(file);
		data = -1;
		return (NULL);
	}
	reply(150, "Opening %s mode data connection for '%s'%s.",
	     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
	return (file);
}

/*
 * Tranfer the contents of "instr" to "outstr" peer using the appropriate
 * encapsulation of the data subject * to Mode, Structure, and Type.
 *
 * NB: Form isn't handled.
 */
static int
send_data(instr, outstr, blksize, isdata)
	FILE *instr, *outstr;
	off_t blksize;
	int isdata;
{
	int	 c, cnt, filefd, netfd, rval;
	char	*buf;

	transflag = 1;
	rval = -1;
	buf = NULL;
	if (setjmp(urgcatch))
		goto cleanup_send_data;

	switch (type) {

	case TYPE_A:
		(void) alarm(curclass.timeout);
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n') {
				if (ferror(outstr))
					goto data_err;
				(void) putc('\r', outstr);
				if (isdata) {
					total_data_out++;
					total_data++;
				}
				total_bytes_out++;
				total_bytes++;
			}
			(void) putc(c, outstr);
			if (isdata) {
				total_data_out++;
				total_data++;
			}
			total_bytes_out++;
			total_bytes++;
			if ((byte_count % 4096) == 0)
				(void) alarm(curclass.timeout);
		}
		(void) alarm(0);
		fflush(outstr);
		if (ferror(instr))
			goto file_err;
		if (ferror(outstr))
			goto data_err;
		rval = 0;
		goto cleanup_send_data;

	case TYPE_I:
	case TYPE_L:
		if ((buf = malloc((size_t)blksize)) == NULL) {
			perror_reply(451, "Local resource failure: malloc");
			goto cleanup_send_data;
		}
		filefd = fileno(instr);
		netfd = fileno(outstr);
		(void) alarm(curclass.timeout);
		while ((cnt = read(filefd, buf, (size_t)blksize)) > 0) {
			if (write(netfd, buf, cnt) != cnt)
				goto data_err;
			(void) alarm(curclass.timeout);
			byte_count += cnt;
			if (isdata) {
				total_data_out += cnt;
				total_data += cnt;
			}
			total_bytes_out += cnt;
			total_bytes += cnt;
		}
		if (cnt < 0)
			goto file_err;
		rval = 0;
		goto cleanup_send_data;

	default:
		reply(550, "Unimplemented TYPE %d in send_data", type);
		goto cleanup_send_data;
	}

data_err:
	(void) alarm(0);
	perror_reply(426, "Data connection");
	goto cleanup_send_data;

file_err:
	(void) alarm(0);
	perror_reply(551, "Error on input file");
		/* FALLTHROUGH */
	
cleanup_send_data:
	(void) alarm(0);
	transflag = 0;
	if (buf)
		free(buf);
	if (isdata) {
		total_files_out++;
		total_files++;
	}
	total_xfers_out++;
	total_xfers++;
	return (rval);
}

/*
 * Transfer data from peer to "outstr" using the appropriate encapulation of
 * the data subject to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled.
 */
static int
receive_data(instr, outstr)
	FILE *instr, *outstr;
{
	int	c, cnt, bare_lfs, netfd, filefd, rval;
	char	buf[BUFSIZ];
#ifdef __GNUC__
	(void) &bare_lfs;
#endif

	bare_lfs = 0;
	transflag = 1;
	rval = -1;
	if (setjmp(urgcatch))
		goto cleanup_recv_data;

	switch (type) {

	case TYPE_I:
	case TYPE_L:
		netfd = fileno(instr);
		filefd = fileno(outstr);
		(void) alarm(curclass.timeout);
		while ((cnt = read(netfd, buf, sizeof(buf))) > 0) {
			if (write(filefd, buf, cnt) != cnt)
				goto file_err;
			(void) alarm(curclass.timeout);
			byte_count += cnt;
			total_data_in += cnt;
			total_data += cnt;
			total_bytes_in += cnt;
			total_bytes += cnt;
		}
		if (cnt < 0)
			goto data_err;
		rval = 0;
		goto cleanup_recv_data;

	case TYPE_E:
		reply(553, "TYPE E not implemented.");
		goto cleanup_recv_data;

	case TYPE_A:
		(void) alarm(curclass.timeout);
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			total_data_in++;
			total_data++;
			total_bytes_in++;
			total_bytes++;
			if ((byte_count % 4096) == 0)
				(void) alarm(curclass.timeout);
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				if (ferror(outstr))
					goto data_err;
				if ((c = getc(instr)) != '\n') {
					byte_count++;
					total_data_in++;
					total_data++;
					total_bytes_in++;
					total_bytes++;
					if ((byte_count % 4096) == 0)
						(void) alarm(curclass.timeout);
					(void) putc ('\r', outstr);
					if (c == '\0' || c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, outstr);
	contin2:	;
		}
		(void) alarm(0);
		fflush(outstr);
		if (ferror(instr))
			goto data_err;
		if (ferror(outstr))
			goto file_err;
		if (bare_lfs) {
			lreply(226,
			    "WARNING! %d bare linefeeds received in ASCII mode",
			    bare_lfs);
			lreply(0, "File may not have transferred correctly.");
		}
		rval = 0;
		goto cleanup_recv_data;

	default:
		reply(550, "Unimplemented TYPE %d in receive_data", type);
		goto cleanup_recv_data;
	}

data_err:
	(void) alarm(0);
	perror_reply(426, "Data Connection");
	goto cleanup_recv_data;

file_err:
	(void) alarm(0);
	perror_reply(452, "Error writing file");
	goto cleanup_recv_data;

cleanup_recv_data:
	(void) alarm(0);
	transflag = 0;
	total_files_in++;
	total_files++;
	total_xfers_in++;
	total_xfers++;
	return (rval);
}

void
statfilecmd(filename)
	const char *filename;
{
	FILE *fin;
	int c;
	char line[LINE_MAX];

	(void)snprintf(line, sizeof(line), "/bin/ls -lgA %s", filename);
	fin = ftpd_popen(line, "r", STDOUT_FILENO);
	lreply(211, "status of %s:", filename);
	while ((c = getc(fin)) != EOF) {
		if (c == '\n') {
			if (ferror(stdout)){
				perror_reply(421, "control connection");
				(void) ftpd_pclose(fin);
				dologout(1);
				/* NOTREACHED */
			}
			if (ferror(fin)) {
				perror_reply(551, filename);
				(void) ftpd_pclose(fin);
				return;
			}
			(void) putc('\r', stdout);
			total_bytes++;
			total_bytes_out++;
		}
		(void) putc(c, stdout);
		total_bytes++;
		total_bytes_out++;
	}
	(void) ftpd_pclose(fin);
	reply(211, "End of Status");
}

void
statcmd()
{
	struct sockaddr_in *sin;
	u_char *a, *p;
	off_t b, otbi, otbo, otb;

	lreply(211, "%s FTP server status:", hostname);
	lreply(0, "%s", version);
	if (isdigit(remotehost[0]))
		lreply(0, "Connected to %s", remotehost);
	else
		lreply(0, "Connected to %s (%s)", remotehost,
		    inet_ntoa(his_addr.sin_addr));
	if (logged_in) {
		if (guest)
			lreply(0, "Logged in anonymously");
		else
			lreply(0, "Logged in as %s", pw->pw_name);
	} else if (askpasswd)
		lreply(0, "Waiting for password");
	else
		lreply(0, "Waiting for user name");
	b = printf("    TYPE: %s", typenames[type]);
	total_bytes += b;
	total_bytes_out += b;
	if (type == TYPE_A || type == TYPE_E) {
		b = printf(", FORM: %s", formnames[form]);
		total_bytes += b;
		total_bytes_out += b;
	}
	if (type == TYPE_L) {
#if NBBY == 8
		b = printf(" %d", NBBY);
#else
			/* XXX: `bytesize' needs to be defined in this case */
		b = printf(" %d", bytesize);
#endif
		total_bytes += b;
		total_bytes_out += b;
	}
	b = printf("; STRUcture: %s; transfer MODE: %s\r\n",
	    strunames[stru], modenames[mode]);
	total_bytes += b;
	total_bytes_out += b;
	if (data != -1)
		lreply(0, "Data connection open");
	else if (pdata != -1) {
		b = printf("                in Passive mode");
		total_bytes += b;
		total_bytes_out += b;
		sin = &pasv_addr;
		goto printaddr;
	} else if (usedefault == 0) {
		b = printf("                PORT");
		total_bytes += b;
		total_bytes_out += b;
		sin = &data_dest;
printaddr:
		a = (u_char *) &sin->sin_addr;
		p = (u_char *) &sin->sin_port;
#define UC(b) (((int) b) & 0xff)
		b = printf(" (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
			UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
#undef UC
		total_bytes += b;
		total_bytes_out += b;
	} else
		lreply(0, "No data connection");

	if (logged_in) {
		lreply(0, "Data sent:        %qd byte%s in %qd file%s",
		    (qdfmt_t)total_data_out, PLURAL(total_data_out),
		    (qdfmt_t)total_files_out, PLURAL(total_files_out));
		lreply(0, "Data received:    %qd byte%s in %qd file%s",
		    (qdfmt_t)total_data_in, PLURAL(total_data_in),
		    (qdfmt_t)total_files_in, PLURAL(total_files_in));
		lreply(0, "Total data:       %qd byte%s in %qd file%s",
		    (qdfmt_t)total_data, PLURAL(total_data),
		    (qdfmt_t)total_files, PLURAL(total_files));
	}
	otbi = total_bytes_in;
	otbo = total_bytes_out;
	otb = total_bytes;
	lreply(0, "Traffic sent:     %qd byte%s in %qd transfer%s",
	    (qdfmt_t)otbo, PLURAL(otbo),
	    (qdfmt_t)total_xfers_out, PLURAL(total_xfers_out));
	lreply(0, "Traffic received: %qd byte%s in %qd transfer%s",
	    (qdfmt_t)otbi, PLURAL(otbi),
	    (qdfmt_t)total_xfers_in, PLURAL(total_xfers_in));
	lreply(0, "Total traffic:    %qd byte%s in %qd transfer%s",
	    (qdfmt_t)otb, PLURAL(otb),
	    (qdfmt_t)total_xfers, PLURAL(total_xfers));

	if (logged_in) {
		struct ftpconv *cp;

		lreply(0, "");
		lreply(0, "Class: %s", curclass.classname);
		lreply(0, "Check PORT commands: %sabled", 
		    curclass.checkportcmd ? "en" : "dis");
		if (curclass.display)
			lreply(0, "Display file: %s", curclass.display);
		if (curclass.notify)
			lreply(0, "Notify fileglob: %s", curclass.notify);
		lreply(0, "Idle timeout: %d, maximum timeout: %d",
		    curclass.timeout, curclass.maxtimeout);
		lreply(0, "DELE, MKD, RMD, UMASK, CHMOD commands: %sabled",
		    curclass.modify ? "en" : "dis");
		lreply(0, "Umask: %.04o", curclass.umask);
		for (cp = curclass.conversions; cp != NULL; cp=cp->next) {
			if (cp->suffix == NULL || cp->types == NULL ||
			    cp->command == NULL)
				continue;
			lreply(0, 
			    "Conversion: %s [%s] disable: %s, command: %s",
			    cp->suffix, cp->types, cp->disable, cp->command);
		}
	}

	reply(211, "End of status");
}

void
fatal(s)
	const char *s;
{

	reply(451, "Error in server: %s\n", s);
	reply(221, "Closing connection due to server error.");
	dologout(0);
	/* NOTREACHED */
}

void
#ifdef __STDC__
reply(int n, const char *fmt, ...)
#else
reply(n, fmt, va_alist)
	int n;
	char *fmt;
        va_dcl
#endif
{
	off_t b;
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	b = 0;
	b += printf("%d ", n);
	b += vprintf(fmt, ap);
	b += printf("\r\n");
	total_bytes += b;
	total_bytes_out += b;
	(void)fflush(stdout);
	if (debug) {
		syslog(LOG_DEBUG, "<--- %d ", n);
		vsyslog(LOG_DEBUG, fmt, ap);
	}
}

void
#ifdef __STDC__
lreply(int n, const char *fmt, ...)
#else
lreply(n, fmt, va_alist)
	int n;
	char *fmt;
        va_dcl
#endif
{
	off_t b;
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	b = 0;
	switch (n) {
		case 0:
			b += printf("    ");
		case -1:
			break;
		default:
			b += printf("%d-", n);
			break;
	}
	b += vprintf(fmt, ap);
	b += printf("\r\n");
	total_bytes += b;
	total_bytes_out += b;
	(void)fflush(stdout);
	if (debug) {
		syslog(LOG_DEBUG, "<--- %d- ", n);
		vsyslog(LOG_DEBUG, fmt, ap);
	}
}

static void
ack(s)
	const char *s;
{

	reply(250, "%s command successful.", s);
}

void
delete(name)
	const char *name;
{
	char *p = NULL;

	if (remove(name) < 0) {
		p = strerror(errno);
		perror_reply(550, name);
	} else
		ack("DELE");
	logcmd("delete", -1, name, NULL, NULL, p);
}

void
cwd(path)
	const char *path;
{

	if (chdir(path) < 0)
		perror_reply(550, path);
	else {
		show_chdir_messages(250);
		ack("CWD");
	}
}

static void
replydirname(name, message)
	const char *name, *message;
{
	char npath[MAXPATHLEN + 1];
	int i;

	for (i = 0; *name != '\0' && i < sizeof(npath) - 1; i++, name++) {
		npath[i] = *name;
		if (*name == '"')
			npath[++i] = '"';
	}
	npath[i] = '\0';
	reply(257, "\"%s\" %s", npath, message);
}

void
makedir(name)
	const char *name;
{
	char *p = NULL;

	if (mkdir(name, 0777) < 0) {
		p = strerror(errno);
		perror_reply(550, name);
	} else
		replydirname(name, "directory created.");
	logcmd("mkdir", -1, name, NULL, NULL, p);
}

void
removedir(name)
	const char *name;
{
	char *p = NULL;

	if (rmdir(name) < 0) {
		p = strerror(errno);
		perror_reply(550, name);
	} else
		ack("RMD");
	logcmd("rmdir", -1, name, NULL, NULL, p);
}

void
pwd()
{
	char path[MAXPATHLEN + 1];

	if (getcwd(path, sizeof(path) - 1) == NULL)
		reply(550, "Can't get the current directory: %s.",
		    strerror(errno));
	else
		replydirname(path, "is the current directory.");
}

char *
renamefrom(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return (NULL);
	}
	reply(350, "File exists, ready for destination name");
	return (name);
}

void
renamecmd(from, to)
	const char *from, *to;
{
	char *p = NULL;

	if (rename(from, to) < 0) {
		p = strerror(errno);
		perror_reply(550, "rename");
	} else
		ack("RNTO");
	logcmd("rename", -1, from, to, NULL, p);
}

static void
dolog(sin)
	struct sockaddr_in *sin;
{
	struct hostent *hp = gethostbyaddr((char *)&sin->sin_addr,
		sizeof(struct in_addr), AF_INET);

	if (hp)
		(void)strncpy(remotehost, hp->h_name, sizeof(remotehost));
	else
		(void)strncpy(remotehost, inet_ntoa(sin->sin_addr),
		    sizeof(remotehost));
	remotehost[sizeof(remotehost) - 1] = '\0';
#ifdef HASSETPROCTITLE
	snprintf(proctitle, sizeof(proctitle), "%s: connected", remotehost);
	setproctitle(proctitle);
#endif /* HASSETPROCTITLE */

	if (logging)
		syslog(LOG_INFO, "connection from %s", remotehost);
}

/*
 * Record logout in wtmp file
 * and exit with supplied status.
 */
void
dologout(status)
	int status;
{
	/*
	* Prevent reception of SIGURG from resulting in a resumption
	* back to the main program loop.
	*/
	transflag = 0;

	if (logged_in) {
		(void) seteuid((uid_t)0);
		logwtmp(ttyline, "", "");
#ifdef KERBEROS
		if (!notickets && krbtkfile_env)
			unlink(krbtkfile_env);
#endif
	}
	/* beware of flushing buffers after a SIGPIPE */
	_exit(status);
}

static void
myoob(signo)
	int signo;
{
	char *cp;

	/* only process if transfer occurring */
	if (!transflag)
		return;
	cp = tmpline;
	if (getline(cp, 7, stdin) == NULL) {
		reply(221, "You could at least say goodbye.");
		dologout(0);
	}
	if (strcasecmp(cp, "ABOR\r\n") == 0) {
		tmpline[0] = '\0';
		reply(426, "Transfer aborted. Data connection closed.");
		reply(226, "Abort successful");
		longjmp(urgcatch, 1);
	}
	if (strcasecmp(cp, "STAT\r\n") == 0) {
		if (file_size != (off_t) -1)
			reply(213, "Status: %qd of %qd byte%s transferred",
			    (qdfmt_t)byte_count, (qdfmt_t)file_size,
			    PLURAL(byte_count));
		else
			reply(213, "Status: %qd byte%s transferred",
			    (qdfmt_t)byte_count, PLURAL(byte_count));
	}
}

/*
 * Note: a response of 425 is not mentioned as a possible response to
 *	the PASV command in RFC959. However, it has been blessed as
 *	a legitimate response by Jon Postel in a telephone conversation
 *	with Rick Adams on 25 Jan 89.
 */
void
passive()
{
	int len;
	char *p, *a;

	pdata = socket(AF_INET, SOCK_STREAM, 0);
	if (pdata < 0 || !logged_in) {
		perror_reply(425, "Can't open passive connection");
		return;
	}
	pasv_addr = ctrl_addr;
	pasv_addr.sin_port = 0;
	(void) seteuid((uid_t)0);
	if (bind(pdata, (struct sockaddr *)&pasv_addr, sizeof(pasv_addr)) < 0) {
		(void) seteuid((uid_t)pw->pw_uid);
		goto pasv_error;
	}
	(void) seteuid((uid_t)pw->pw_uid);
	len = sizeof(pasv_addr);
	if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
		goto pasv_error;
	if (listen(pdata, 1) < 0)
		goto pasv_error;
	a = (char *) &pasv_addr.sin_addr;
	p = (char *) &pasv_addr.sin_port;

#define UC(b) (((int) b) & 0xff)

	reply(227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", UC(a[0]),
		UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
	return;

pasv_error:
	(void) close(pdata);
	pdata = -1;
	perror_reply(425, "Can't open passive connection");
	return;
}

/*
 * Generate unique name for file with basename "local".
 * The file named "local" is already known to exist.
 * Generates failure reply on error.
 *
 * XXX this function should under go changes similar to
 * the mktemp(3)/mkstemp(3) changes.
 */
static char *
gunique(local)
	const char *local;
{
	static char new[MAXPATHLEN + 1];
	struct stat st;
	int count, len;
	char *cp;

	cp = strrchr(local, '/');
	if (cp)
		*cp = '\0';
	if (stat(cp ? local : ".", &st) < 0) {
		perror_reply(553, cp ? local : ".");
		return (NULL);
	}
	if (cp)
		*cp = '/';
	(void) strcpy(new, local);
	len = strlen(new);
	cp = new + len;
	*cp++ = '.';
	for (count = 1; count < 100; count++) {
		(void)snprintf(cp, sizeof(new) - len - 2, "%d", count);
		if (stat(new, &st) < 0)
			return (new);
	}
	reply(452, "Unique file name cannot be created.");
	return (NULL);
}

/*
 * Format and send reply containing system error number.
 */
void
perror_reply(code, string)
	int code;
	const char *string;
{
	int save_errno;

	save_errno = errno;
	reply(code, "%s: %s.", string, strerror(errno));
	errno = save_errno;
}

static char *onefile[] = {
	"",
	0
};

void
send_file_list(whichf)
	const char *whichf;
{
	struct stat st;
	DIR *dirp = NULL;
	struct dirent *dir;
	FILE *dout = NULL;
	char **dirlist, *dirname, *p;
	int simple = 0;
	int freeglob = 0;
	glob_t gl;
	off_t b;

#ifdef __GNUC__
	(void) &dout;
	(void) &dirlist;
	(void) &simple;
	(void) &freeglob;
#endif

	p = NULL;
	if (strpbrk(whichf, "~{[*?") != NULL) {
		int flags = GLOB_BRACE|GLOB_NOCHECK|GLOB_TILDE;

		memset(&gl, 0, sizeof(gl));
		freeglob = 1;
		if (glob(whichf, flags, 0, &gl)) {
			reply(550, "not found");
			goto out;
		} else if (gl.gl_pathc == 0) {
			errno = ENOENT;
			perror_reply(550, whichf);
			goto out;
		}
		dirlist = gl.gl_pathv;
	} else {
		p = xstrdup(whichf);
		onefile[0] = p;
		dirlist = onefile;
		simple = 1;
	}
					/* XXX: } for vi sm */

	if (setjmp(urgcatch)) {
		transflag = 0;
		goto out;
	}
	while ((dirname = *dirlist++) != NULL) {
		int trailingslash = 0;

		if (stat(dirname, &st) < 0) {
			/*
			 * If user typed "ls -l", etc, and the client
			 * used NLST, do what the user meant.
			 */
			if (dirname[0] == '-' && *dirlist == NULL &&
			    transflag == 0) {
				retrieve("/bin/ls %s", dirname);
				goto out;
			}
			perror_reply(550, whichf);
			if (dout != NULL) {
				(void) fclose(dout);
				transflag = 0;
				data = -1;
				pdata = -1;
			}
			goto out;
		}

		if (S_ISREG(st.st_mode)) {
			if (dout == NULL) {
				dout = dataconn("file list", (off_t)-1, "w");
				if (dout == NULL)
					goto out;
				transflag++;
			}
			b = fprintf(dout, "%s%s\n", dirname,
			    type == TYPE_A ? "\r" : "");
			total_bytes += b;
			total_bytes_out += b;
			byte_count += strlen(dirname) + 1;
			continue;
		} else if (!S_ISDIR(st.st_mode))
			continue;

		if (dirname[strlen(dirname) - 1] == '/')
			trailingslash++;

		if ((dirp = opendir(dirname)) == NULL)
			continue;

		while ((dir = readdir(dirp)) != NULL) {
			char nbuf[MAXPATHLEN + 1];

			if (dir->d_name[0] == '.' && dir->d_namlen == 1)
				continue;
			if (dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
			    dir->d_namlen == 2)
				continue;

			(void)snprintf(nbuf, sizeof(nbuf), "%s%s%s", dirname,
			    trailingslash ? "" : "/", dir->d_name);

			/*
			 * We have to do a stat to ensure it's
			 * not a directory or special file.
			 */
			if (simple || (stat(nbuf, &st) == 0 &&
			    S_ISREG(st.st_mode))) {
				char *p;

				if (dout == NULL) {
					dout = dataconn("file list", (off_t)-1,
						"w");
					if (dout == NULL)
						goto out;
					transflag++;
				}
				p = nbuf;
				if (nbuf[0] == '.' && nbuf[1] == '/')
					p = &nbuf[2];
				b = fprintf(dout, "%s%s\n", p,
				    type == TYPE_A ? "\r" : "");
				total_bytes += b;
				total_bytes_out += b;
				byte_count += strlen(nbuf) + 1;
			}
		}
		(void) closedir(dirp);
	}

	if (dout == NULL)
		reply(550, "No files found.");
	else if (ferror(dout) != 0)
		perror_reply(550, "Data connection");
	else
		reply(226, "Transfer complete.");

	transflag = 0;
	if (dout != NULL)
		(void) fclose(dout);
	data = -1;
	pdata = -1;
out:
	total_xfers++;
	total_xfers_out++;
	if (p)
		free(p);
	if (freeglob)
		globfree(&gl);
}

char *
conffilename(s)
	const char *s;
{
	static char filename[MAXPATHLEN + 1];

	(void)snprintf(filename, sizeof(filename), "%s/%s", confdir ,s);
	return filename;
}

/*
 * logcmd --
 *	based on the arguments, syslog a message:
 *	 if bytes != -1		"<command> <file1> = <bytes> bytes"
 *	 else if file2 != NULL	"<command> <file1> <file2>"
 *	 else			"<command> <file1>"
 *	if elapsed != NULL, append "in xxx.yyy seconds"
 *	if error != NULL, append ": " + error
 */

void
logcmd(command, bytes, file1, file2, elapsed, error)
	const char		*command;
	off_t			 bytes;
	const char		*file1;
	const char		*file2;
	const struct timeval	*elapsed;
	const char		*error;
{
	char	buf[MAXPATHLEN + 100], realfile[MAXPATHLEN + 1];
	const char *p;
	size_t	len;

	if (logging <=1)
		return;

	if ((p = realpath(file1, realfile)) == NULL) {
#if 0	/* XXX: too noisy */
		syslog(LOG_WARNING, "realpath `%s' failed: %s",
		    realfile, strerror(errno));
#endif
		p = file1;
	}
	len = snprintf(buf, sizeof(buf), "%s %s", command, p);

	if (bytes != (off_t)-1) {
		len += snprintf(buf + len, sizeof(buf) - len,
		    " = %qd byte%s", (qdfmt_t) bytes, PLURAL(bytes));
	} else if (file2 != NULL) {
		if ((p = realpath(file2, realfile)) == NULL) {
#if 0	/* XXX: too noisy */
			syslog(LOG_WARNING, "realpath `%s' failed: %s",
			    realfile, strerror(errno));
#endif
			p = file2;
		}
		len += snprintf(buf + len, sizeof(buf) - len, " %s", p);
	}

	if (elapsed != NULL) {
		len += snprintf(buf + len, sizeof(buf) - len,
		    " in %ld.%.03d seconds", elapsed->tv_sec,
		    (int)(elapsed->tv_usec / 1000));
	}

	if (error != NULL)
		len += snprintf(buf + len, sizeof(buf) - len, ": %s", error);

	syslog(LOG_INFO, "%s", buf);
}

char *
xstrdup(s)
	const char *s;
{
	char *new = strdup(s);

	if (new == NULL)
		fatal("Local resource failure: malloc");
		/* NOTREACHED */
	return (new);
}
