/*	$NetBSD: su.c,v 1.42 2000/07/13 08:37:10 assar Exp $	*/

/*
 * Copyright (c) 1988 The Regents of the University of California.
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
    "@(#) Copyright (c) 1988 The Regents of the University of California.\n\
 All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)su.c	8.3 (Berkeley) 4/2/94";*/
#else
__RCSID("$NetBSD: su.c,v 1.42 2000/07/13 08:37:10 assar Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#ifdef SKEY
#include <skey.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#ifdef LOGIN_CAP
#include <login_cap.h>
#endif

#ifdef KERBEROS
#include <des.h>
#include <krb.h>
#include <netdb.h>

static int kerberos __P((char *, char *, int));
static int koktologin __P((char *, char *, char *));

#endif

#ifdef KERBEROS5
#include <krb5.h>

static int kerberos5 __P((char *, char *, int));

#endif

#if defined(KERBEROS) || defined(KERBEROS5)

#define	ARGSTRX	"-Kflm"

int use_kerberos = 1;

#else
#define	ARGSTRX	"-flm"
#endif

#ifndef	SUGROUP
#define	SUGROUP	"wheel"
#endif

#ifdef LOGIN_CAP
#define ARGSTR	ARGSTRX "c:"
#else
#define ARGSTR ARGSTRX
#endif

int main __P((int, char **));

static int chshell __P((const char *));
static char *ontty __P((void));


int
main(argc, argv)
	int argc;
	char **argv;
{
	extern char *__progname;
	extern char **environ;
	struct passwd *pwd;
	char *p;
	struct group *gr;
#ifdef BSD4_4
	struct timeval tp;
#endif
	uid_t ruid;
	int asme, ch, asthem, fastlogin, prio;
	enum { UNSET, YES, NO } iscsh = UNSET;
	char *user, *shell, *avshell, *username, **np;
	char *userpass, *class;
	char shellbuf[MAXPATHLEN], avshellbuf[MAXPATHLEN];
	time_t pw_warntime = _PASSWORD_WARNDAYS * SECSPERDAY;
#ifdef LOGIN_CAP
	login_cap_t *lc;
#endif

	asme = asthem = fastlogin = 0;
	shell = class = NULL;
	while ((ch = getopt(argc, argv, ARGSTR)) != -1)
		switch((char)ch) {
#if defined(KERBEROS) || defined(KERBEROS5)
		case 'K':
			use_kerberos = 0;
			break;
#endif
#ifdef LOGIN_CAP
		case 'c':
			class = optarg;
			break;
#endif
		case 'f':
			fastlogin = 1;
			break;
		case '-':
		case 'l':
			asme = 0;
			asthem = 1;
			break;
		case 'm':
			asme = 1;
			asthem = 0;
			break;
		case '?':
		default:
			(void)fprintf(stderr,
			    "Usage: %s [%s] [login [shell arguments]]\n",
			    __progname, ARGSTR);
			exit(1);
		}
	argv += optind;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (errno)
		prio = 0;
	(void)setpriority(PRIO_PROCESS, 0, -2);
	openlog("su", LOG_CONS, 0);

	/* get current login name and shell */
	ruid = getuid();
	username = getlogin();
	if (username == NULL || (pwd = getpwnam(username)) == NULL ||
	    pwd->pw_uid != ruid)
		pwd = getpwuid(ruid);
	if (pwd == NULL)
		errx(1, "who are you?");
	username = strdup(pwd->pw_name);
	userpass = strdup(pwd->pw_passwd);
	if (username == NULL || userpass == NULL)
		err(1, "strdup");


	if (asme) {
		if (pwd->pw_shell && *pwd->pw_shell) {
			shell = strncpy(shellbuf, pwd->pw_shell,
			    sizeof(shellbuf) - 1);
			shellbuf[sizeof(shellbuf) - 1] = '\0';
		} else {
			shell = _PATH_BSHELL;
			iscsh = NO;
		}
	}
	/* get target login information, default to root */
	user = *argv ? *argv : "root";
	np = *argv ? argv : argv-1;

	if ((pwd = getpwnam(user)) == NULL)
		errx(1, "unknown login %s", user);

#ifdef LOGIN_CAP
	/* force the usage of specified class */
	if (class) {
		if (ruid)
			errx(1, "Only root may use -c");

		pwd->pw_class = class;
	}
	lc = login_getclass(pwd->pw_class);

	pw_warntime = login_getcaptime(lc, "password-warn",  
                                    _PASSWORD_WARNDAYS * SECSPERDAY,
                                    _PASSWORD_WARNDAYS * SECSPERDAY);
#endif

	if (ruid
#ifdef KERBEROS5
	    && (!use_kerberos || kerberos5(username, user, pwd->pw_uid))
#endif
#ifdef KERBEROS
	    && (!use_kerberos || kerberos(username, user, pwd->pw_uid))
#endif
	    ) {
		char *pass = pwd->pw_passwd;
		int ok = pwd->pw_uid != 0;
		char **g;

#ifdef ROOTAUTH
		/*
		 * Allow those in group rootauth to su to root, by supplying
		 * their own password.
		 */
		if (!ok && (gr = getgrnam(ROOTAUTH)))
			for (g = gr->gr_mem;; ++g) {
				if (!*g) {
					ok = 0;
					break;
				}
				if (!strcmp(username, *g)) {
					pass = userpass;
					user = username;
					ok = 1;
					break;
				}
			}
#endif
		/*
		 * Only allow those in group SUGROUP to su to root,
		 * but only if that group has any members.
		 * If SUGROUP has no members, allow anyone to su root
		 */
		if (!ok) {
			if ( !(gr = getgrnam(SUGROUP)) || !*gr->gr_mem)
				ok = 1;
			else
				for (g = gr->gr_mem; ; g++) {
					if (*g == NULL) {
						ok = 0;
						break;
					}
					if (strcmp(username, *g) == 0) {
						ok = 1;
						break;
					}
				}
		}
		if (!ok)
			errx(1,
	    "you are not listed in the correct secondary group (%s) to su %s.",
					    SUGROUP, user);
		/* if target requires a password, verify it */
		if (*pass) {
			p = getpass("Password:");
#ifdef SKEY
			if (strcasecmp(p, "s/key") == 0) {
				if (skey_haskey(user))
					errx(1, "Sorry, you have no s/key.");
				else {
					if (skey_authenticate(user)) {
						goto badlogin;
					}
				}

			} else
#endif
			if (strcmp(pass, crypt(p, pass))) {
#ifdef SKEY
badlogin:
#endif
				fprintf(stderr, "Sorry\n");
				syslog(LOG_AUTH|LOG_WARNING,
					"BAD SU %s to %s%s", username,
					pwd->pw_name, ontty());
				exit(1);
			}
		}
	}

	if (asme) {
		/* if asme and non-standard target shell, must be root */
		if (!chshell(pwd->pw_shell) && ruid)
			errx(1,"permission denied (shell).");
	} else if (pwd->pw_shell && *pwd->pw_shell) {
		shell = pwd->pw_shell;
		iscsh = UNSET;
	} else {
		shell = _PATH_BSHELL;
		iscsh = NO;
	}

	if ((p = strrchr(shell, '/')) != NULL)
		avshell = p+1;
	else
		avshell = shell;

	/* if we're forking a csh, we want to slightly muck the args */
	if (iscsh == UNSET)
		iscsh = strstr(avshell, "csh") ? YES : NO;

#ifndef LOGIN_CAP /* This is done by setusercontext() */
	/* set permissions */
	if (setgid(pwd->pw_gid) < 0)
		err(1, "setgid");
	if (initgroups(user, pwd->pw_gid))
		errx(1, "initgroups failed");
	if (setuid(pwd->pw_uid) < 0)
		err(1, "setuid");
#endif

	if (!asme) {
		if (asthem) {
			p = getenv("TERM");
			/* Create an empty environment */
			if ((environ = malloc(sizeof(char *))) == NULL)
				err(1, NULL);
			environ[0] = NULL;
#ifdef LOGIN_CAP
			if (setusercontext(lc,
			    pwd, pwd->pw_uid, LOGIN_SETPATH))
				err(1, "setting user context");
#else
			(void)setenv("PATH", _PATH_DEFPATH, 1);
#endif
			if (p)
				(void)setenv("TERM", p, 1);
			if (chdir(pwd->pw_dir) < 0)
				errx(1, "no directory");
		} 

		if (asthem || pwd->pw_uid)
			(void)setenv("USER", pwd->pw_name, 1);
		(void)setenv("HOME", pwd->pw_dir, 1);
		(void)setenv("SHELL", shell, 1);
	}
	(void)setenv("SU_FROM", username, 1);

	if (iscsh == YES) {
		if (fastlogin)
			*np-- = "-f";
		if (asme)
			*np-- = "-m";
	}

	if (asthem) {
		avshellbuf[0] = '-';
		(void)strncpy(avshellbuf+1, avshell, sizeof(avshellbuf) - 2);
		avshell = avshellbuf;
	} else if (iscsh == YES) {
		/* csh strips the first character... */
		avshellbuf[0] = '_';
		(void)strncpy(avshellbuf+1, avshell, sizeof(avshellbuf) - 2);
		avshell = avshellbuf;
	}
	*np = avshell;

#ifdef BSD4_4
	if (pwd->pw_change || pwd->pw_expire)
		(void)gettimeofday(&tp, (struct timezone *)NULL);
	if (pwd->pw_change) {
		if (tp.tv_sec >= pwd->pw_change) {
			(void)printf("%s -- %s's password has expired.\n",
				     (ruid ? "Sorry" : "Note"), user);
			if (ruid != 0)
				exit(1);
		} else if (pwd->pw_change - tp.tv_sec < pw_warntime)
			(void)printf("Warning: %s's password expires on %s",
				     user, ctime(&pwd->pw_change));
	}
	if (pwd->pw_expire) {
		if (tp.tv_sec >= pwd->pw_expire) {
			(void)printf("%s -- %s's account has expired.\n",
				     (ruid ? "Sorry" : "Note"), user);
			if (ruid != 0)
				exit(1);
		} else if (pwd->pw_expire - tp.tv_sec <
		    _PASSWORD_WARNDAYS * SECSPERDAY)
			(void)printf("Warning: %s's account expires on %s",
				     user, ctime(&pwd->pw_expire));
 	}
#endif
	if (ruid != 0)
		syslog(LOG_NOTICE|LOG_AUTH, "%s to %s%s",
		    username, pwd->pw_name, ontty());

	(void)setpriority(PRIO_PROCESS, 0, prio);
#ifdef LOGIN_CAP
	if (setusercontext(lc, pwd, pwd->pw_uid,
	    (asthem ? (LOGIN_SETPRIORITY | LOGIN_SETUMASK) : 0) |
	    LOGIN_SETRESOURCES | LOGIN_SETGROUP | LOGIN_SETUSER))
		err(1, "setting user context");
#endif

	execv(shell, np);
	err(1, "%s", shell);
        /* NOTREACHED */
}

static int
chshell(sh)
	const char *sh;
{
	const char *cp;

	while ((cp = getusershell()) != NULL)
		if (!strcmp(cp, sh))
			return (1);
	return (0);
}

static char *
ontty()
{
	char *p;
	static char buf[MAXPATHLEN + 4];

	buf[0] = 0;
	if ((p = ttyname(STDERR_FILENO)) != NULL)
		(void)snprintf(buf, sizeof buf, " on %s", p);
	return (buf);
}

#ifdef KERBEROS5
static int
kerberos5(username, user, uid)
	char *username, *user;
	int uid;
{
	krb5_error_code ret;
	krb5_context context;
	krb5_principal princ = NULL;
	krb5_ccache ccache, ccache2;
	char *cc_name;

	ret = krb5_init_context(&context);
	if (ret)
		return (1);

	if (strcmp (user, "root") == 0)
		ret = krb5_make_principal(context, &princ,
					  NULL, username, "root", NULL);
	else
		ret = krb5_make_principal(context, &princ,
					  NULL, user, NULL);
	if (ret)
		goto fail;
	if (!krb5_kuserok(context, princ, user) && !uid) {
		warnx ("kerberos5: not in %s's ACL.", user);
		goto fail;
	}
	ret = krb5_cc_gen_new(context, &krb5_mcc_ops, &ccache);
	if (ret)
		goto fail;
	ret = krb5_verify_user_lrealm(context, princ, ccache, NULL, TRUE,
				      NULL);
	if (ret) {
		krb5_cc_destroy(context, ccache);
		switch (ret) {
		case KRB5_LIBOS_PWDINTR :
			break;
		case KRB5KRB_AP_ERR_BAD_INTEGRITY:
		case KRB5KRB_AP_ERR_MODIFIED:
			krb5_warnx(context, "Password incorrect");
			break;
		default :
			krb5_warn(context, ret, "krb5_verify_user");
			break;
		}
		goto fail;
	}
	ret = krb5_cc_gen_new(context, &krb5_fcc_ops, &ccache2);
	if (ret) {
		krb5_cc_destroy(context, ccache);
		goto fail;
	}
	ret = krb5_cc_copy_cache(context, ccache, ccache2);
	if (ret) {
		krb5_cc_destroy(context, ccache);
		krb5_cc_destroy(context, ccache2);
		goto fail;
	}

	asprintf(&cc_name, "%s:%s", krb5_cc_get_type(context, ccache2),
		 krb5_cc_get_name(context, ccache2));
	setenv("KRB5CCNAME", cc_name, 1);
	free(cc_name);
	krb5_cc_close(context, ccache2);
	krb5_cc_destroy(context, ccache);
	return 0;

 fail:
	if (princ != NULL)
		krb5_free_principal (context, princ);
	krb5_free_context (context);
	return (1);
}
#endif

#ifdef KERBEROS
static int
kerberos(username, user, uid)
	char *username, *user;
	int uid;
{
	KTEXT_ST ticket;
	AUTH_DAT authdata;
	struct hostent *hp;
	int kerno;
	u_long faddr;
	char lrealm[REALM_SZ], krbtkfile[MAXPATHLEN];
	char hostname[MAXHOSTNAMELEN + 1], savehost[MAXHOSTNAMELEN + 1];

	if (krb_get_lrealm(lrealm, 1) != KSUCCESS)
		return (1);
	if (koktologin(username, lrealm, user) && !uid) {
		warnx("kerberos: not in %s's ACL.", user);
		return (1);
	}
	(void)snprintf(krbtkfile, sizeof krbtkfile, "%s_%s_%d", TKT_ROOT,
	    user, getuid());

	(void)setenv("KRBTKFILE", krbtkfile, 1);
	(void)krb_set_tkt_string(krbtkfile);
	/*
	 * Set real as well as effective ID to 0 for the moment,
	 * to make the kerberos library do the right thing.
	 */
	if (setuid(0) < 0) {
		warn("setuid");
		return (1);
	}

	/*
	 * Little trick here -- if we are su'ing to root,
	 * we need to get a ticket for "xxx.root", where xxx represents
	 * the name of the person su'ing.  Otherwise (non-root case),
	 * we need to get a ticket for "yyy.", where yyy represents
	 * the name of the person being su'd to, and the instance is null
	 *
	 * We should have a way to set the ticket lifetime,
	 * with a system default for root.
	 */
	{
		char prompt[128];
		char passw[256];

		(void)snprintf (prompt, sizeof(prompt),
			  "%s's Password: ",
			  krb_unparse_name_long ((uid == 0 ? username : user),
						 (uid == 0 ? "root" : ""),
						 lrealm));
		if (des_read_pw_string (passw, sizeof (passw), prompt, 0)) {
			memset (passw, 0, sizeof (passw));
			return (1);
		}
		if (strlen(passw) == 0)
			return (1); /* Empty passwords are not allowed */

		kerno = krb_get_pw_in_tkt((uid == 0 ? username : user),
					  (uid == 0 ? "root" : ""), lrealm,
					  KRB_TICKET_GRANTING_TICKET,
					  lrealm,
					  DEFAULT_TKT_LIFE,
					  passw);
		memset (passw, 0, strlen (passw));
	}

	if (kerno != KSUCCESS) {
		if (kerno == KDC_PR_UNKNOWN) {
			warnx("kerberos: principal unknown: %s.%s@%s",
				(uid == 0 ? username : user),
				(uid == 0 ? "root" : ""), lrealm);
			return (1);
		}
		warnx("kerberos: unable to su: %s", krb_err_txt[kerno]);
		syslog(LOG_NOTICE|LOG_AUTH,
		    "BAD Kerberos SU: %s to %s%s: %s",
		    username, user, ontty(), krb_err_txt[kerno]);
		return (1);
	}

	if (chown(krbtkfile, uid, -1) < 0) {
		warn("chown");
		(void)unlink(krbtkfile);
		return (1);
	}

	(void)setpriority(PRIO_PROCESS, 0, -2);

	if (gethostname(hostname, sizeof(hostname)) == -1) {
		warn("gethostname");
		dest_tkt();
		return (1);
	}
	hostname[sizeof(hostname) - 1] = '\0';

	(void)strlcpy(savehost, krb_get_phost(hostname), sizeof(savehost));
	savehost[sizeof(savehost) - 1] = '\0';

	kerno = krb_mk_req(&ticket, "rcmd", savehost, lrealm, 33);

	if (kerno == KDC_PR_UNKNOWN) {
		warnx("Warning: TGT not verified.");
		syslog(LOG_NOTICE|LOG_AUTH,
		    "%s to %s%s, TGT not verified (%s); %s.%s not registered?",
		    username, user, ontty(), krb_err_txt[kerno],
		    "rcmd", savehost);
	} else if (kerno != KSUCCESS) {
		warnx("Unable to use TGT: %s", krb_err_txt[kerno]);
		syslog(LOG_NOTICE|LOG_AUTH, "failed su: %s to %s%s: %s",
		    username, user, ontty(), krb_err_txt[kerno]);
		dest_tkt();
		return (1);
	} else {
		if (!(hp = gethostbyname(hostname))) {
			warnx("can't get addr of %s", hostname);
			dest_tkt();
			return (1);
		}
		memmove((char *)&faddr, (char *)hp->h_addr, sizeof(faddr));

		if ((kerno = krb_rd_req(&ticket, "rcmd", savehost, faddr,
		    &authdata, "")) != KSUCCESS) {
			warnx("kerberos: unable to verify rcmd ticket: %s\n",
			    krb_err_txt[kerno]);
			syslog(LOG_NOTICE|LOG_AUTH,
			    "failed su: %s to %s%s: %s", username,
			     user, ontty(), krb_err_txt[kerno]);
			dest_tkt();
			return (1);
		}
	}
	return (0);
}

static int
koktologin(name, realm, toname)
	char *name, *realm, *toname;
{
	return krb_kuserok(name,
			   strcmp (toname, "root") == 0 ? "root" : "",
			   realm,
			   toname);
}
#endif
