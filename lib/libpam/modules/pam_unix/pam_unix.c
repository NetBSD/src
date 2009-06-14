/*	$NetBSD: pam_unix.c,v 1.13 2009/06/14 23:23:54 tonnerre Exp $	*/

/*-
 * Copyright 1998 Juniper Networks, Inc.
 * All rights reserved.
 * Copyright (c) 2002-2003 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * Portions of this software was developed for the FreeBSD Project by
 * ThinkSec AS and NAI Labs, the Security Research Division of Network
 * Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/lib/libpam/modules/pam_unix/pam_unix.c,v 1.49 2004/02/10 10:13:21 des Exp $");
#else
__RCSID("$NetBSD: pam_unix.c,v 1.13 2009/06/14 23:23:54 tonnerre Exp $");
#endif


#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <login_cap.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include <util.h>

#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yppasswd.h>
#endif

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define	PAM_SM_PASSWORD

#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_mod_misc.h>

/*
 * authentication management
 */
PAM_EXTERN int
/*ARGSUSED*/
pam_sm_authenticate(pam_handle_t *pamh, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	login_cap_t *lc;
	struct passwd *pwd, pwres;
	int retval;
	const char *pass, *user, *realpw;
	char pwbuf[1024];

	pwd = NULL;
	if (openpam_get_option(pamh, PAM_OPT_AUTH_AS_SELF)) {
		(void) getpwnam_r(getlogin(), &pwres, pwbuf, sizeof(pwbuf),
				  &pwd);
	} else {
		retval = pam_get_user(pamh, &user, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);
		PAM_LOG("Got user: %s", user);
		(void) getpwnam_r(user, &pwres, pwbuf, sizeof(pwbuf), &pwd);
	}

	if (pwd != NULL) {
		PAM_LOG("Doing real authentication");
		realpw = pwd->pw_passwd;
		if (realpw[0] == '\0') {
			if (!(flags & PAM_DISALLOW_NULL_AUTHTOK) &&
			    openpam_get_option(pamh, PAM_OPT_NULLOK))
				return (PAM_SUCCESS);
			realpw = "*";
		}
		lc = login_getpwclass(pwd);
	} else {
		PAM_LOG("Doing dummy authentication");
		realpw = "*";
		lc = login_getclass(NULL);
	}
	retval = pam_get_authtok(pamh, PAM_AUTHTOK, &pass, NULL);
	login_close(lc);
	if (retval != PAM_SUCCESS)
		return (retval);
	PAM_LOG("Got password");
	if (strcmp(crypt(pass, realpw), realpw) == 0)
		return (PAM_SUCCESS);

	PAM_VERBOSE_ERROR("UNIX authentication refused");
	return (PAM_AUTH_ERR);
}

PAM_EXTERN int
/*ARGSUSED*/
pam_sm_setcred(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{

	return (PAM_SUCCESS);
}

/*
 * account management
 */
PAM_EXTERN int
/*ARGSUSED*/
pam_sm_acct_mgmt(pam_handle_t *pamh, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	struct passwd *pwd, pwres;
	struct timeval now;
	login_cap_t *lc;
	time_t warntime;
	int retval;
	const char *user;
	char pwbuf[1024];

	retval = pam_get_user(pamh, &user, NULL);
	if (retval != PAM_SUCCESS)
		return (retval);

	if (user == NULL ||
	    getpwnam_r(user, &pwres, pwbuf, sizeof(pwbuf), &pwd) != 0 ||
	    pwd == NULL)
		return (PAM_SERVICE_ERR);

	PAM_LOG("Got user: %s", user);

	if (*pwd->pw_passwd == '\0' &&
	    (flags & PAM_DISALLOW_NULL_AUTHTOK) != 0)
		return (PAM_NEW_AUTHTOK_REQD);

	lc = login_getpwclass(pwd);
	if (lc == NULL) {
		PAM_LOG("Unable to get login class for user %s", user);
		return (PAM_SERVICE_ERR);
	}

	PAM_LOG("Got login_cap");

	if (pwd->pw_change || pwd->pw_expire)
		(void) gettimeofday(&now, NULL);

	warntime = (time_t)login_getcaptime(lc, "password-warn",
	    (quad_t)(_PASSWORD_WARNDAYS * SECSPERDAY),
	    (quad_t)(_PASSWORD_WARNDAYS * SECSPERDAY));

	/*
	 * Check pw_expire before pw_change - no point in letting the
	 * user change the password on an expired account.
	 */

	if (pwd->pw_expire) {
		if (now.tv_sec >= pwd->pw_expire) {
			login_close(lc);
			return (PAM_ACCT_EXPIRED);
		} else if (pwd->pw_expire - now.tv_sec < warntime &&
		    (flags & PAM_SILENT) == 0) {
			pam_error(pamh, "Warning: your account expires on %s",
			    ctime(&pwd->pw_expire));
		}
	}

	if (pwd->pw_change) {
		/* XXX How to handle _PASSWORD_CHGNOW?  --thorpej */
		if (now.tv_sec >= pwd->pw_change) {
			login_close(lc);
			return (PAM_NEW_AUTHTOK_REQD);
		} else if (pwd->pw_change - now.tv_sec < warntime &&
		    (flags & PAM_SILENT) == 0) {
			pam_error(pamh, "Warning: your password expires on %s",
			    ctime(&pwd->pw_change));
		}
	}

	login_close(lc);

	return (PAM_SUCCESS);
}

#ifdef YP
/*
 * yp_check_user:
 *
 *	Helper function; check that a user exists in the NIS
 *	password map.
 */
static int
yp_check_user(const char *domain, const char *user)
{
	char *val;
	int reason, vallen;

	val = NULL;
	reason = yp_match(domain, "passwd.byname", user, (int)strlen(user),
	    &val, &vallen);
	if (reason != 0) {
		if (val != NULL)
			free(val);
		return (0);
	}
	free(val);
	return (1);
}

static int
/*ARGSUSED*/
yp_set_password(pam_handle_t *pamh, struct passwd *opwd,
    struct passwd *pwd, const char *old_pass, const char *domain)
{
	char *master;
	int r, rpcport, status;
	struct yppasswd yppwd;
	CLIENT *client;
	uid_t uid;
	int retval = PAM_SERVICE_ERR;
	struct timeval tv;

	/*
	 * Find the master for the passwd map; it should be running
	 * rpc.yppasswdd.
	 */
	if ((r = yp_master(domain, "passwd.byname", &master)) != 0) {
		pam_error(pamh, "Can't find master NIS server.  Reason: %s",
		    yperr_string(r));
		return (PAM_SERVICE_ERR);
	}

	/*
	 * Ask the portmapper for the port of rpc.yppasswdd.
	 */
	if ((rpcport = getrpcport(master, YPPASSWDPROG,
				  YPPASSWDPROC_UPDATE, IPPROTO_UDP)) == 0) {
		pam_error(pamh,
		    "Master NIS server not runing yppasswd daemon.\n\t"
		    "Can't change NIS password.");
		return (PAM_SERVICE_ERR);
	}

	/*
	 * Be sure the port is privileged.
	 */
	if (rpcport >= IPPORT_RESERVED) {
		pam_error(pamh, "yppasswd daemon is on an invalid port.");
		return (PAM_SERVICE_ERR);
	}

	uid = getuid();
	if (uid != 0 && uid != pwd->pw_uid) {
		pam_error(pamh, "You may only change your own password: %s",
		    strerror(EACCES));
		return (PAM_SERVICE_ERR);
	}

	/*
	 * Fill in the yppasswd structure for yppasswdd.
	 */
	memset(&yppwd, 0, sizeof(yppwd));
	yppwd.oldpass = strdup(old_pass);
	if ((yppwd.newpw.pw_passwd = strdup(pwd->pw_passwd)) == NULL)
		goto malloc_failure;
	if ((yppwd.newpw.pw_name = strdup(pwd->pw_name)) == NULL)
		goto malloc_failure;
	yppwd.newpw.pw_uid = pwd->pw_uid;
	yppwd.newpw.pw_gid = pwd->pw_gid;
	if ((yppwd.newpw.pw_gecos = strdup(pwd->pw_gecos)) == NULL)
		goto malloc_failure;
	if ((yppwd.newpw.pw_dir = strdup(pwd->pw_dir)) == NULL)
		goto malloc_failure;
	if ((yppwd.newpw.pw_shell = strdup(pwd->pw_shell)) == NULL)
		goto malloc_failure;

	client = clnt_create(master, YPPASSWDPROG, YPPASSWDVERS, "udp");
	if (client == NULL) {
		pam_error(pamh, "Can't contact yppasswdd on %s: Reason: %s",
		    master, yperr_string(YPERR_YPBIND));
		goto out;
	}

	client->cl_auth = authunix_create_default();
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	r = clnt_call(client, YPPASSWDPROC_UPDATE,
	    xdr_yppasswd, &yppwd, xdr_int, &status, tv);
	if (r)
		pam_error(pamh, "RPC to yppasswdd failed.");
	else if (status)
		pam_error(pamh, "Couldn't change NIS password.");
	else {
		pam_info(pamh, "The NIS password has been changed on %s, "
		    "the master NIS passwd server.", master);
		retval = PAM_SUCCESS;
	}

 out:
	if (yppwd.oldpass != NULL)
		free(yppwd.oldpass);
	if (yppwd.newpw.pw_passwd != NULL)
		free(yppwd.newpw.pw_passwd);
	if (yppwd.newpw.pw_name != NULL)
		free(yppwd.newpw.pw_name);
	if (yppwd.newpw.pw_gecos != NULL)
		free(yppwd.newpw.pw_gecos);
	if (yppwd.newpw.pw_dir != NULL)
		free(yppwd.newpw.pw_dir);
	if (yppwd.newpw.pw_shell != NULL)
		free(yppwd.newpw.pw_shell);
	return (retval);

 malloc_failure:
	pam_error(pamh, "memory allocation failure");
	goto out;
}
#endif /* YP */

static int
local_set_password(pam_handle_t *pamh, struct passwd *opwd,
    struct passwd *pwd)
{
	char errbuf[200];
	int tfd, pfd;

	pw_init();
	tfd = pw_lock(0);
	if (tfd < 0) {
		pam_error(pamh, "The password file is busy, waiting...");
		tfd = pw_lock(10);
		if (tfd < 0) {
			pam_error(pamh, "The password file is still busy, "
			    "try again later.");
			return (PAM_SERVICE_ERR);
		}
	}

	pfd = open(_PATH_MASTERPASSWD, O_RDONLY, 0);
	if (pfd < 0) {
		pam_error(pamh, "%s: %s", _PATH_MASTERPASSWD, strerror(errno));
		pw_abort();
		return (PAM_SERVICE_ERR);
	}

	if (pw_copyx(pfd, tfd, pwd, opwd, errbuf, sizeof(errbuf)) == 0) {
		pam_error(pamh, "Unable to update password entry: %s",
		    errbuf);
		pw_abort();
		return (PAM_SERVICE_ERR);
	}

	if (pw_mkdb(pwd->pw_name, opwd->pw_change == pwd->pw_change) < 0) {
		pam_error(pamh, "Unable to rebuild local password database.");
		pw_abort();
		return (PAM_SERVICE_ERR);
	}

	return (PAM_SUCCESS);
}

/*
 * password management
 *
 * standard Unix and NIS password changing
 */
PAM_EXTERN int
/*ARGSUSED*/
pam_sm_chauthtok(pam_handle_t *pamh, int flags,
    int argc __unused, const char *argv[] __unused)
{
	struct passwd *pwd, new_pwd, old_pwd;
	login_cap_t *lc;
	const char *user, *passwd_db, *new_pass, *old_pass, *p;
	int retval, tries, min_pw_len = 0, pw_expiry = 0;
	char salt[_PASSWORD_LEN+1];
	char old_pwbuf[1024];
#ifdef YP
	char *domain;
	int r;
#endif

	pwd = NULL;
	if (openpam_get_option(pamh, PAM_OPT_AUTH_AS_SELF)) {
		if ((user = getlogin()) == NULL) {
			pam_error(pamh, "Unable to determine user.");
			return (PAM_SERVICE_ERR);
		}
		(void) getpwnam_r(user, &old_pwd, old_pwbuf,
				  sizeof(old_pwbuf), &pwd);
	} else {
		retval = pam_get_user(pamh, &user, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);
		(void) getpwnam_r(user, &old_pwd, old_pwbuf,
				  sizeof(old_pwbuf), &pwd);
	}

	if (pwd == NULL)
		return (PAM_AUTHTOK_RECOVERY_ERR);

	PAM_LOG("Got user: %s", user);

	/*
	 * Determine which password type we're going to change, and
	 * remember it.
	 *
	 * NOTE: domain does not need to be freed; its storage is
	 * allocated statically in libc.
	 */
	passwd_db = openpam_get_option(pamh, "passwd_db");
	if (passwd_db == NULL) {
#ifdef YP
		/* Prefer YP, if configured. */
		if (_yp_check(NULL)) {
			/* If _yp_check() succeeded, then this must. */
			if ((r = yp_get_default_domain(&domain)) != 0) {
				pam_error(pamh,
				    "Unable to get NIS domain, reason: %s",
				    yperr_string(r));
				return (PAM_SERVICE_ERR);
			}
			if (yp_check_user(domain, user))
				passwd_db = "nis";
		}
#endif
		/* Otherwise we always use local files. */
		if (passwd_db == NULL) {
			/* XXX Any validation to do here? */
			passwd_db = "files";
		}

		if ((retval = openpam_set_option(pamh, "passwd_db",
		    passwd_db)) != PAM_SUCCESS) {
			return (retval);
		}
	} else {
		/* Check to see if the specified password DB is usable. */
#ifdef YP
		if (strcmp(passwd_db, "nis") == 0) {
			if (_yp_check(NULL) == 0) {
				pam_error(pamh, "NIS not in use.");
				return (PAM_SERVICE_ERR);
			}
			if ((r = yp_get_default_domain(&domain)) != 0) {
				pam_error(pamh,
				    "Unable to get NIS domain, reason: %s",
				    yperr_string(r));
				return (PAM_SERVICE_ERR);
			}
			if (yp_check_user(domain, user) == 0) {
				pam_error(pamh,
				    "User %s does not exist in NIS.", user);
				return (PAM_USER_UNKNOWN);
			}
			goto known_passwd_db;
		}
#endif
		if (strcmp(passwd_db, "files") == 0) {
			/* XXX Any validation to do here? */
			goto known_passwd_db;
		}
		pam_error(pamh, "Unknown Unix password DB: %s", passwd_db);
		return (PAM_SERVICE_ERR);
	}
 known_passwd_db:

	if (flags & PAM_PRELIM_CHECK) {
		PAM_LOG("PRELIM round");

		if (strcmp(passwd_db, "files") == 0) {
			if (getuid() == 0) {
				/* Root doesn't need the old password. */
				return (pam_set_item(pamh, PAM_OLDAUTHTOK, ""));
			}
			/*
			 * Apparently we're not root, so let's forbid editing
			 * root.
			 * XXX Check for some flag to indicate if this
			 * XXX is the desired behavior.
			 */
			if (pwd->pw_uid == 0)
				return (PAM_PERM_DENIED);
		}

		if (pwd->pw_passwd[0] == '\0') {
			/*
			 * No password case.
			 * XXX Are we giviing too much away by not prompting
			 * XXX for a password?
			 * XXX Check PAM_DISALLOW_NULL_AUTHTOK
			 */
			return (pam_set_item(pamh, PAM_OLDAUTHTOK, ""));
		} else {
			retval = pam_get_authtok(pamh, PAM_OLDAUTHTOK,
			    &old_pass, NULL);
			if (retval != PAM_SUCCESS)
				return (retval);
			if (strcmp(crypt(old_pass, pwd->pw_passwd),
				   pwd->pw_passwd) != 0)
				return (PAM_PERM_DENIED);
			return (PAM_SUCCESS);
		}
	}

	if (flags & PAM_UPDATE_AUTHTOK) {
		char option[LINE_MAX], *key, *opt;

		PAM_LOG("UPDATE round");

		if ((lc = login_getclass(pwd->pw_class)) != NULL) {
			min_pw_len = (int) login_getcapnum(lc,
			    "minpasswordlen", (quad_t)0, (quad_t)0);
			pw_expiry = (int) login_getcapnum(lc,
			    "passwordtime", (quad_t)0, (quad_t)0);
			login_close(lc);
		}

		retval = pam_get_authtok(pamh, PAM_OLDAUTHTOK, &old_pass, NULL);
		if (retval != PAM_SUCCESS)
			return (retval);

		/* Get the new password. */
		for (tries = 0;;) {
			pam_set_item(pamh, PAM_AUTHTOK, NULL);
			retval = pam_get_authtok(pamh, PAM_AUTHTOK, &new_pass,
			    NULL);
			if (retval == PAM_TRY_AGAIN) {
				pam_error(pamh,
				    "Mismatch; try again, EOF to quit.");
				continue;
			}
			if (retval != PAM_SUCCESS) {
				PAM_VERBOSE_ERROR("Unable to get new password");
				return (retval);
			}
			/* Successfully got new password. */
			if (new_pass[0] == '\0') {
				pam_info(pamh, "Password unchanged.");
				return (PAM_SUCCESS);
			}
			if (min_pw_len > 0 && strlen(new_pass) < (size_t)min_pw_len) {
				pam_error(pamh, "Password is too short.");
				continue;
			}
			if (strlen(new_pass) <= 5 && ++tries < 2) {
				pam_error(pamh,
				    "Please enter a longer password.");
				continue;
			}
			for (p = new_pass; *p && islower((unsigned char)*p); ++p);
			if (!*p && ++tries < 2) {
				pam_error(pamh,
				    "Please don't use an all-lower case "
				    "password.\nUnusual capitalization, "
				    "control characters or digits are "
				    "suggested.");
				continue;
			}
			/* Password is OK. */
			break;
		}
		pw_getpwconf(option, sizeof(option), pwd, 
#ifdef YP
		    strcmp(passwd_db, "nis") == 0 ? "ypcipher" :
#endif
		    "localcipher");
		opt = option;
		key = strsep(&opt, ",");

		if (pw_gensalt(salt, _PASSWORD_LEN, key, opt) == -1) {
			pam_error(pamh, "Couldn't generate salt.");
			return (PAM_SERVICE_ERR);
		}

		new_pwd = old_pwd;
		pwd = &new_pwd;
		pwd->pw_passwd = crypt(new_pass, salt);
		pwd->pw_change = pw_expiry ? pw_expiry + time(NULL) : 0;

		retval = PAM_SERVICE_ERR;
		if (strcmp(passwd_db, "files") == 0)
			retval = local_set_password(pamh, &old_pwd, pwd);
#ifdef YP
		if (strcmp(passwd_db, "nis") == 0)
			retval = yp_set_password(pamh, &old_pwd, pwd, old_pass,
			    domain);
#endif
		return (retval);
	}

	PAM_LOG("Illegal flags argument");
	return (PAM_ABORT);
}

PAM_MODULE_ENTRY("pam_unix");
