/*     $NetBSD: login.c,v 1.61 2000/08/02 16:51:17 thorpej Exp $       */

/*-
 * Copyright (c) 1980, 1987, 1988, 1991, 1993, 1994
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
__COPYRIGHT(
"@(#) Copyright (c) 1980, 1987, 1988, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)login.c	8.4 (Berkeley) 4/2/94";
#endif
__RCSID("$NetBSD: login.c,v 1.61 2000/08/02 16:51:17 thorpej Exp $");
#endif /* not lint */

/*
 * login [ name ]
 * login -h hostname	(for telnetd, etc.)
 * login -f name	(for pre-authenticated login: datakit, xterm, etc.)
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <ttyent.h>
#include <tzfile.h>
#include <unistd.h>
#include <utmp.h>
#include <util.h>
#ifdef SKEY
#include <skey.h>
#endif
#ifdef KERBEROS5
#include <krb5/krb5.h>
#include <com_err.h>
#endif
#ifdef LOGIN_CAP
#include <login_cap.h>
#endif

#include "pathnames.h"

#ifdef KERBEROS5
int login_krb5_get_tickets = 1;
int login_krb4_get_tickets = 0;
int login_krb5_forwardable_tgt = 0;
int login_krb5_retain_ccache = 0;
#endif

void	 badlogin __P((char *));
void	 checknologin __P((char *));
void	 dolastlog __P((int));
void	 getloginname __P((void));
int	 main __P((int, char *[]));
void	 motd __P((char *));
int	 rootterm __P((char *));
void	 sigint __P((int));
void	 sleepexit __P((int));
const	 char *stypeof __P((const char *));
void	 timedout __P((int));
#if defined(KERBEROS)
int	 klogin __P((struct passwd *, char *, char *, char *));
void	 kdestroy __P((void));
#endif
#ifdef KERBEROS5
int	 k5login __P((struct passwd *, char *, char *, char *));
void	 k5destroy __P((void));
int	 k5_read_creds __P((char*));
int	 k5_write_creds __P((void));
#endif
#if defined(KERBEROS) || defined(KERBEROS5)
void	 dofork __P((void));
#endif

#define	TTYGRPNAME	"tty"		/* name of group to own ttys */

#define DEFAULT_BACKOFF 3
#define DEFAULT_RETRIES 10

/*
 * This bounds the time given to login.  Not a define so it can
 * be patched on machines where it's too small.
 */
u_int	timeout = 300;

#if defined(KERBEROS) || defined(KERBEROS5)
int	notickets = 1;
char	*instance;
int	has_ccache = 0;
#endif
#ifdef KERBEROS
extern char	*krbtkfile_env;
extern int	krb_configured;
#endif
#ifdef KERBEROS5
extern krb5_context kcontext;
extern int	have_forward;
extern char	*krb5tkfile_env;
extern int	krb5_configured;
#endif

#if defined(KERBEROS) && defined(KERBEROS5)
#define	KERBEROS_CONFIGURED	(krb_configured || krb5_configured)
#elif defined(KERBEROS)
#define	KERBEROS_CONFIGURED	krb_configured
#elif defined(KERBEROS5)
#define	KERBEROS_CONFIGURED	krb5_configured
#endif

struct	passwd *pwd;
int	failures;
char	term[64], *envinit[1], *hostname, *username, *tty;

static const char copyrightstr[] = "\
Copyright (c) 1996, 1997, 1998, 1999, 2000\n\
\tThe NetBSD Foundation, Inc.  All rights reserved.\n\
Copyright (c) 1980, 1983, 1986, 1988, 1990, 1991, 1993, 1994\n\
\tThe Regents of the University of California.  All rights reserved.\n\n";

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char **environ;
	struct group *gr;
	struct stat st;
	struct timeval tp;
	struct utmp utmp;
	int ask, ch, cnt, fflag, hflag, pflag, sflag, quietlog, rootlogin, rval;
	int Fflag;
	uid_t uid, saved_uid;
	gid_t saved_gid, saved_gids[NGROUPS_MAX];
	int nsaved_gids;
	char *domain, *p, *ttyn, *pwprompt;
	const char *salt;
	char tbuf[MAXPATHLEN + 2], tname[sizeof(_PATH_TTY) + 10];
	char localhost[MAXHOSTNAMELEN + 1];
	int need_chpass, require_chpass;
	int login_retries = DEFAULT_RETRIES, 
	    login_backoff = DEFAULT_BACKOFF;
	time_t pw_warntime = _PASSWORD_WARNDAYS * SECSPERDAY;
#ifdef KERBEROS5
	krb5_error_code kerror;
#endif
#if defined(KERBEROS) || defined(KERBEROS5)
	int got_tickets = 0;
#endif
#ifdef LOGIN_CAP
	char *shell = NULL;
	login_cap_t *lc = NULL;
#endif

	tbuf[0] = '\0';
	rval = 0;
	pwprompt = NULL;
	need_chpass = require_chpass = 0;

	(void)signal(SIGALRM, timedout);
	(void)alarm(timeout);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)setpriority(PRIO_PROCESS, 0, 0);

	openlog("login", LOG_ODELAY, LOG_AUTH);

	/*
	 * -p is used by getty to tell login not to destroy the environment
	 * -f is used to skip a second login authentication
	 * -h is used by other servers to pass the name of the remote
	 *    host to login so that it may be placed in utmp and wtmp
	 * -s is used to force use of S/Key or equivalent.
	 */
	domain = NULL;
	if (gethostname(localhost, sizeof(localhost)) < 0)
		syslog(LOG_ERR, "couldn't get local hostname: %m");
	else
		domain = strchr(localhost, '.');
	localhost[sizeof(localhost) - 1] = '\0';

	Fflag = fflag = hflag = pflag = sflag = 0;
#ifdef KERBEROS5
	have_forward = 0;
#endif
	uid = getuid();
	while ((ch = getopt(argc, argv, "Ffh:ps")) != -1)
		switch (ch) {
		case 'F':
			Fflag = 1;
			/* FALLTHROUGH */
		case 'f':
			fflag = 1;
			break;
		case 'h':
			if (uid)
				errx(1, "-h option: %s", strerror(EPERM));
			hflag = 1;
			if (domain && (p = strchr(optarg, '.')) != NULL &&
			    strcasecmp(p, domain) == 0)
				*p = 0;
			hostname = optarg;
			break;
		case 'p':
			pflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		default:
		case '?':
			(void)fprintf(stderr,
			    "usage: login [-fps] [-h hostname] [username]\n");
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (*argv) {
		username = *argv;
		ask = 0;
	} else
		ask = 1;

	for (cnt = getdtablesize(); cnt > 2; cnt--)
		(void)close(cnt);

	ttyn = ttyname(STDIN_FILENO);
	if (ttyn == NULL || *ttyn == '\0') {
		(void)snprintf(tname, sizeof(tname), "%s??", _PATH_TTY);
		ttyn = tname;
	}
	if ((tty = strrchr(ttyn, '/')) != NULL)
		++tty;
	else
		tty = ttyn;

#ifdef LOGIN_CAP
	/* Get "login-retries" and "login-backoff" from default class */
	if ((lc = login_getclass(NULL)) != NULL) {
		login_retries = (int)login_getcapnum(lc, "login-retries",
		    DEFAULT_RETRIES, DEFAULT_RETRIES);
		login_backoff = (int)login_getcapnum(lc, "login-backoff", 
		    DEFAULT_BACKOFF, DEFAULT_BACKOFF);
		login_close(lc);
		lc = NULL;
	}
#endif

#ifdef KERBEROS5
	kerror = krb5_init_context(&kcontext);
	if (kerror) {
		/*
		 * If Kerberos is not configured, that is, we are
		 * not using Kerberos, do not log the error message.
		 * However, if Kerberos is configured,  and the
		 * context init fails for some other reason, we need
		 * to issue a no tickets warning to the user when the
		 * login succeeds.
		 */
		if (kerror != ENXIO) {	/* XXX NetBSD-local Heimdal hack */
			syslog(LOG_NOTICE,
			    "%s when initializing Kerberos context",
			    error_message(kerror));
			krb5_configured = 1;
		}
		login_krb5_get_tickets = 0;
	}
#endif KERBEROS5

	for (cnt = 0;; ask = 1) {
#if defined(KERBEROS)
	        kdestroy();
#endif
#if defined(KERBEROS5)
		if (login_krb5_get_tickets)
			k5destroy();
#endif
		if (ask) {
			fflag = 0;
			getloginname();
		}
		rootlogin = 0;
#ifdef KERBEROS
		if ((instance = strchr(username, '.')) != NULL)
			*instance++ = '\0';
		else
			instance = "";
#endif
#ifdef KERBEROS5
		if ((instance = strchr(username, '/')) != NULL)
			*instance++ = '\0';
		else
			instance = "";
#endif
		if (strlen(username) > MAXLOGNAME)
			username[MAXLOGNAME] = '\0';

		/*
		 * Note if trying multiple user names; log failures for
		 * previous user name, but don't bother logging one failure
		 * for nonexistent name (mistyped username).
		 */
		if (failures && strcmp(tbuf, username)) {
			if (failures > (pwd ? 0 : 1))
				badlogin(tbuf);
			failures = 0;
		}
		(void)strncpy(tbuf, username, sizeof(tbuf) - 1);
		tbuf[sizeof(tbuf) - 1] = '\0';

		if ((pwd = getpwnam(username)) != NULL)
			salt = pwd->pw_passwd;
		else
			salt = "xx";

#ifdef LOGIN_CAP
		/*
		 * Establish the class now, before we might goto
		 * within the next block. pwd can be NULL since it
		 * falls back to the "default" class if it is.
		 */
		lc = login_getclass(pwd ? pwd->pw_class : NULL);
#endif
		/*
		 * if we have a valid account name, and it doesn't have a
		 * password, or the -f option was specified and the caller
		 * is root or the caller isn't changing their uid, don't
		 * authenticate.
		 */
		if (pwd) {
			if (pwd->pw_uid == 0)
				rootlogin = 1;

			if (fflag && (uid == 0 || uid == pwd->pw_uid)) {
				/* already authenticated */
#ifdef KERBEROS5
				if (login_krb5_get_tickets && Fflag)
					k5_read_creds(username);
#endif
				break;
			} else if (pwd->pw_passwd[0] == '\0') {
				/* pretend password okay */
				rval = 0;
				goto ttycheck;
			}
		}

		fflag = 0;

		(void)setpriority(PRIO_PROCESS, 0, -4);

#ifdef SKEY
		if (skey_haskey(username) == 0) {
			static char skprompt[80];
			const char *skinfo = skey_keyinfo(username);
				
			(void)snprintf(skprompt, sizeof(skprompt)-1,
			    "Password [%s]:",
			    skinfo ? skinfo : "error getting challenge");
			pwprompt = skprompt;
		} else
#endif
			pwprompt = "Password:";

		p = getpass(pwprompt);

		if (pwd == NULL) {
			rval = 1;
			goto skip;
		}
#ifdef KERBEROS
		if (
#ifdef KERBEROS5
		    /* allow a user to get both krb4 and krb5 tickets, if
		     * desired.  If krb5 is compiled in, the default action
		     * is to ignore krb4 and get krb5 tickets, but the user
		     * can override this in the krb5.conf. */
		    login_krb4_get_tickets &&
#endif
		    klogin(pwd, instance, localhost, p) == 0) {
			rval = 0;
			got_tickets = 1;
		}
#endif
#ifdef KERBEROS5
		if (login_krb5_get_tickets &&
		    k5login(pwd, instance, localhost, p) == 0) {
			rval = 0;
			got_tickets = 1;
		}
#endif
#if defined(KERBEROS) || defined(KERBEROS5)
		if (got_tickets)
			goto skip;
#endif
#ifdef SKEY
		if (skey_haskey(username) == 0 &&
		    skey_passcheck(username, p) != -1) {
			rval = 0;
			goto skip;
		}
#endif
		if (!sflag && *pwd->pw_passwd != '\0' &&
		    !strcmp(crypt(p, pwd->pw_passwd), pwd->pw_passwd)) {
			rval = 0;
			require_chpass = 1;
			goto skip;
		}
		rval = 1;

	skip:
		memset(p, 0, strlen(p));

		(void)setpriority(PRIO_PROCESS, 0, 0);

	ttycheck:
		/*
		 * If trying to log in as root without Kerberos,
		 * but with insecure terminal, refuse the login attempt.
		 */
		if (pwd && !rval && rootlogin && !rootterm(tty)) {
			(void)fprintf(stderr,
			    "%s login refused on this terminal.\n",
			    pwd->pw_name);
			if (hostname)
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED FROM %s ON TTY %s",
				    pwd->pw_name, hostname, tty);
			else
				syslog(LOG_NOTICE,
				    "LOGIN %s REFUSED ON TTY %s",
				     pwd->pw_name, tty);
			continue;
		}

		if (pwd && !rval)
			break;

		(void)printf("Login incorrect\n");
		failures++;
		cnt++;
		/* we allow 10 tries, but after 3 we start backing off */
		if (cnt > login_backoff) {
			if (cnt >= login_retries) {
				badlogin(username);
				sleepexit(1);
			}
			sleep((u_int)((cnt - 3) * 5));
		}
	}

	/* committed to login -- turn off timeout */
	(void)alarm((u_int)0);

	endpwent();

	/* if user not super-user, check for disabled logins */
#ifdef LOGIN_CAP
        if (!login_getcapbool(lc, "ignorenologin", rootlogin))
		checknologin(login_getcapstr(lc, "nologin", NULL, NULL));
#else
        if (!rootlogin)
                checknologin(NULL);
#endif

#ifdef LOGIN_CAP
        quietlog = login_getcapbool(lc, "hushlogin", 0);
#else
        quietlog = 0;
#endif
	/* Temporarily give up special privileges so we can change */
	/* into NFS-mounted homes that are exported for non-root */
	/* access and have mode 7x0 */
	saved_uid = geteuid();
	saved_gid = getegid();
	nsaved_gids = getgroups(NGROUPS_MAX, saved_gids);
	
	(void)setegid(pwd->pw_gid);
	initgroups(username, pwd->pw_gid);
	(void)seteuid(pwd->pw_uid);
	
	if (chdir(pwd->pw_dir) < 0) {
#ifdef LOGIN_CAP
                if (login_getcapbool(lc, "requirehome", 0)) {
			(void)printf("Home directory %s required\n",
			    pwd->pw_dir);
                        sleepexit(1);
		}
#endif	
		(void)printf("No home directory %s!\n", pwd->pw_dir);
		if (chdir("/"))
			exit(0);
		pwd->pw_dir = "/";
		(void)printf("Logging in with home = \"/\".\n");
	}

	if (!quietlog)
		quietlog = access(_PATH_HUSHLOGIN, F_OK) == 0;

	/* regain special privileges */
	(void)seteuid(saved_uid);
	setgroups(nsaved_gids, saved_gids);
	(void)setegid(saved_gid);

#ifdef LOGIN_CAP
        pw_warntime = login_getcaptime(lc, "password-warn",
            _PASSWORD_WARNDAYS * SECSPERDAY,
            _PASSWORD_WARNDAYS * SECSPERDAY);
#endif

	if (pwd->pw_change || pwd->pw_expire)
		(void)gettimeofday(&tp, (struct timezone *)NULL);
	if (pwd->pw_expire) {
		if (tp.tv_sec >= pwd->pw_expire) {
			(void)printf("Sorry -- your account has expired.\n");
			sleepexit(1);
		} else if (pwd->pw_expire - tp.tv_sec < pw_warntime && 
		    !quietlog)
			(void)printf("Warning: your account expires on %s",
			    ctime(&pwd->pw_expire));
	}
	if (pwd->pw_change) {
		if (pwd->pw_change == _PASSWORD_CHGNOW)
			need_chpass = 1;
		else if (tp.tv_sec >= pwd->pw_change) {
			(void)printf("Sorry -- your password has expired.\n");
			sleepexit(1);
		} else if (pwd->pw_change - tp.tv_sec < pw_warntime && 
		    !quietlog)
			(void)printf("Warning: your password expires on %s",
			    ctime(&pwd->pw_change));

	}
	/* Nothing else left to fail -- really log in. */
	memset((void *)&utmp, 0, sizeof(utmp));
	(void)time(&utmp.ut_time);
	(void)strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
	if (hostname)
		(void)strncpy(utmp.ut_host, hostname, sizeof(utmp.ut_host));
	(void)strncpy(utmp.ut_line, tty, sizeof(utmp.ut_line));
	login(&utmp);

	dolastlog(quietlog);

	(void)chown(ttyn, pwd->pw_uid,
	    (gr = getgrnam(TTYGRPNAME)) ? gr->gr_gid : pwd->pw_gid);

	if (ttyaction(ttyn, "login", pwd->pw_name))
		(void)printf("Warning: ttyaction failed.\n");

#if defined(KERBEROS) || defined(KERBEROS5)
	/* Fork so that we can call kdestroy */
	if (
#ifdef KERBEROS5
	    ! login_krb5_retain_ccache &&
#endif
	    has_ccache)
		dofork();
#endif

	/* Destroy environment unless user has requested its preservation. */
	if (!pflag)
		environ = envinit;

#ifdef LOGIN_CAP
	if (setusercontext(lc, pwd, pwd->pw_uid, 
	    LOGIN_SETALL & ~LOGIN_SETPATH) != 0) {
		syslog(LOG_ERR, "setusercontext failed");
		exit(1);
	}
#else
	(void)setgid(pwd->pw_gid);

	initgroups(username, pwd->pw_gid);
	
	if (setlogin(pwd->pw_name) < 0)
		syslog(LOG_ERR, "setlogin() failure: %m");

	/* Discard permissions last so can't get killed and drop core. */
	if (rootlogin)
		(void)setuid(0);
	else
		(void)setuid(pwd->pw_uid);
#endif

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;
#ifdef LOGIN_CAP
	if ((shell = login_getcapstr(lc, "shell", NULL, NULL)) != NULL) {
		if ((shell = strdup(shell)) == NULL) {
                	syslog(LOG_NOTICE, "Cannot alloc mem");
                	sleepexit(1);
		}
		pwd->pw_shell = shell;
	}
#endif
	
	(void)setenv("HOME", pwd->pw_dir, 1);
	(void)setenv("SHELL", pwd->pw_shell, 1);
	if (term[0] == '\0') {
		char *tt = (char *)stypeof(tty);
#ifdef LOGIN_CAP
		if (tt == NULL)
			tt = login_getcapstr(lc, "term", NULL, NULL);
#endif
		/* unknown term -> "su" */
		(void)strncpy(term, tt != NULL ? tt : "su", sizeof(term));
	}
	(void)setenv("TERM", term, 0);
	(void)setenv("LOGNAME", pwd->pw_name, 1);
	(void)setenv("USER", pwd->pw_name, 1);

#ifdef LOGIN_CAP
	setusercontext(lc, pwd, pwd->pw_uid, LOGIN_SETPATH);
#else
	(void)setenv("PATH", _PATH_DEFPATH, 0);
#endif

#ifdef KERBEROS
	if (krbtkfile_env)
		(void)setenv("KRBTKFILE", krbtkfile_env, 1);
#endif
#ifdef KERBEROS5
	if (krb5tkfile_env)
		(void)setenv("KRB5CCNAME", krb5tkfile_env, 1);
#endif

	if (tty[sizeof("tty")-1] == 'd')
		syslog(LOG_INFO, "DIALUP %s, %s", tty, pwd->pw_name);

	/* If fflag is on, assume caller/authenticator has logged root login. */
	if (rootlogin && fflag == 0) {
		if (hostname)
			syslog(LOG_NOTICE, "ROOT LOGIN (%s) ON %s FROM %s",
			    username, tty, hostname);
		else
			syslog(LOG_NOTICE, "ROOT LOGIN (%s) ON %s",
			    username, tty);
	}

#if defined(KERBEROS) || defined(KERBEROS5)
	if (KERBEROS_CONFIGURED && !quietlog && notickets == 1)
		(void)printf("Warning: no Kerberos tickets issued.\n");
#endif

	if (!quietlog) {
		char *fname;
#ifdef LOGIN_CAP
		fname = login_getcapstr(lc, "copyright", NULL, NULL);
		if (fname != NULL && access(fname, F_OK) == 0)
			motd(fname);
		else
#endif
			(void)printf(copyrightstr);

#ifdef LOGIN_CAP
                fname = login_getcapstr(lc, "welcome", NULL, NULL);
                if (fname == NULL || access(fname, F_OK) != 0)
#endif
                        fname = _PATH_MOTDFILE;
                motd(fname);

		(void)snprintf(tbuf,
		    sizeof(tbuf), "%s/%s", _PATH_MAILDIR, pwd->pw_name);
		if (stat(tbuf, &st) == 0 && st.st_size != 0)
			(void)printf("You have %smail.\n",
			    (st.st_mtime > st.st_atime) ? "new " : "");
	}

#ifdef LOGIN_CAP
	login_close(lc);
#endif

	(void)signal(SIGALRM, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGTSTP, SIG_IGN);

	tbuf[0] = '-';
	(void)strncpy(tbuf + 1, (p = strrchr(pwd->pw_shell, '/')) ?
	    p + 1 : pwd->pw_shell, sizeof(tbuf) - 2);

	/* Wait to change password until we're unprivileged */
	if (need_chpass) {
		if (!require_chpass)
			(void)printf(
"Warning: your password has expired. Please change it as soon as possible.\n");
		else {
			int	status;

			(void)printf(
		    "Your password has expired. Please choose a new one.\n");
			switch (fork()) {
			case -1:
				warn("fork");
				sleepexit(1);
			case 0:
				execl(_PATH_BINPASSWD, "passwd", 0);
				_exit(1);
			default:
				if (wait(&status) == -1 ||
				    WEXITSTATUS(status))
					sleepexit(1);
			}
		}
	}

#ifdef KERBEROS5
	if (login_krb5_get_tickets)
		k5_write_creds();
#endif
	execlp(pwd->pw_shell, tbuf, 0);
	err(1, "%s", pwd->pw_shell);
}

#if defined(KERBEROS) || defined(KERBEROS5)
#define	NBUFSIZ		(MAXLOGNAME + 1 + 5)	/* .root suffix */
#else
#define	NBUFSIZ		(MAXLOGNAME + 1)
#endif

#if defined(KERBEROS) || defined(KERBEROS5)
/*
 * This routine handles cleanup stuff, and the like.
 * It exists only in the child process.
 */
#include <sys/wait.h>
void
dofork()
{
	int child;

	if (!(child = fork()))
		return; /* Child process */

	/*
	 * Setup stuff?  This would be things we could do in parallel
	 * with login
	 */
	(void)chdir("/");	/* Let's not keep the fs busy... */

	/* If we're the parent, watch the child until it dies */
	while (wait(0) != child)
		;

	/* Cleanup stuff */
	/* Run kdestroy to destroy tickets */
#ifdef KERBEROS
	kdestroy();
#endif
#ifdef KERBEROS5
	if (login_krb5_get_tickets)
		k5destroy();
#endif

	/* Leave */
	exit(0);
}
#endif

void
getloginname()
{
	int ch;
	char *p;
	static char nbuf[NBUFSIZ];

	for (;;) {
		(void)printf("login: ");
		for (p = nbuf; (ch = getchar()) != '\n'; ) {
			if (ch == EOF) {
				badlogin(username);
				exit(0);
			}
			if (p < nbuf + (NBUFSIZ - 1))
				*p++ = ch;
		}
		if (p > nbuf) {
			if (nbuf[0] == '-')
				(void)fprintf(stderr,
				    "login names may not start with '-'.\n");
			else {
				*p = '\0';
				username = nbuf;
				break;
			}
		}
	}
}

int
rootterm(ttyn)
	char *ttyn;
{
	struct ttyent *t;

	return ((t = getttynam(ttyn)) && t->ty_status & TTY_SECURE);
}

jmp_buf motdinterrupt;

void
motd(fname)
	char *fname;
{
	int fd, nchars;
	sig_t oldint;
	char tbuf[8192];

	if ((fd = open(fname ? fname : _PATH_MOTDFILE, O_RDONLY, 0)) < 0)
		return;
	oldint = signal(SIGINT, sigint);
	if (setjmp(motdinterrupt) == 0)
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
	(void)signal(SIGINT, oldint);
	(void)close(fd);
}

/* ARGSUSED */
void
sigint(signo)
	int signo;
{

	longjmp(motdinterrupt, 1);
}

/* ARGSUSED */
void
timedout(signo)
	int signo;
{

	(void)fprintf(stderr, "Login timed out after %d seconds\n", timeout);
	exit(0);
}

void
checknologin(fname)
	char *fname;
{
	int fd, nchars;
	char tbuf[8192];

	if ((fd = open(fname ? fname : _PATH_NOLOGIN, O_RDONLY, 0)) >= 0) {
		while ((nchars = read(fd, tbuf, sizeof(tbuf))) > 0)
			(void)write(fileno(stdout), tbuf, nchars);
		sleepexit(0);
	}
}

void
dolastlog(quiet)
	int quiet;
{
	struct lastlog ll;
	int fd;

	if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) >= 0) {
		(void)lseek(fd, (off_t)(pwd->pw_uid * sizeof(ll)), SEEK_SET);
		if (!quiet) {
			if (read(fd, (char *)&ll, sizeof(ll)) == sizeof(ll) &&
			    ll.ll_time != 0) {
				(void)printf("Last login: %.*s ",
				    24, (char *)ctime(&ll.ll_time));
				if (*ll.ll_host != '\0')
					(void)printf("from %.*s\n",
					    (int)sizeof(ll.ll_host),
					    ll.ll_host);
				else
					(void)printf("on %.*s\n",
					    (int)sizeof(ll.ll_line),
					    ll.ll_line);
			}
			(void)lseek(fd, (off_t)(pwd->pw_uid * sizeof(ll)),
			    SEEK_SET);
		}
		memset((void *)&ll, 0, sizeof(ll));
		(void)time(&ll.ll_time);
		(void)strncpy(ll.ll_line, tty, sizeof(ll.ll_line));
		if (hostname)
			(void)strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
		(void)write(fd, (char *)&ll, sizeof(ll));
		(void)close(fd);
	}
}

void
badlogin(name)
	char *name;
{

	if (failures == 0)
		return;
	if (hostname) {
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s FROM %s",
		    failures, failures > 1 ? "S" : "", hostname);
		syslog(LOG_AUTHPRIV|LOG_NOTICE,
		    "%d LOGIN FAILURE%s FROM %s, %s",
		    failures, failures > 1 ? "S" : "", hostname, name);
	} else {
		syslog(LOG_NOTICE, "%d LOGIN FAILURE%s ON %s",
		    failures, failures > 1 ? "S" : "", tty);
		syslog(LOG_AUTHPRIV|LOG_NOTICE,
		    "%d LOGIN FAILURE%s ON %s, %s",
		    failures, failures > 1 ? "S" : "", tty, name);
	}
}

const char *
stypeof(ttyid)
	const char *ttyid;
{
	struct ttyent *t;

	return (ttyid && (t = getttynam(ttyid)) ? t->ty_type : NULL);
}

void
sleepexit(eval)
	int eval;
{

	(void)sleep(5);
	exit(eval);
}
