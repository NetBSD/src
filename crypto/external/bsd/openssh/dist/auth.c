/*	$NetBSD: auth.c,v 1.5.2.1 2013/01/16 05:25:58 yamt Exp $	*/
/* $OpenBSD: auth.c,v 1.96 2012/05/13 01:42:32 dtucker Exp $ */
/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"
__RCSID("$NetBSD: auth.c,v 1.5.2.1 2013/01/16 05:25:58 yamt Exp $");
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <login_cap.h>
#include <paths.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "xmalloc.h"
#include "match.h"
#include "groupaccess.h"
#include "log.h"
#include "buffer.h"
#include "servconf.h"
#include "key.h"
#include "hostfile.h"
#include "auth.h"
#include "auth-options.h"
#include "canohost.h"
#include "uidswap.h"
#include "misc.h"
#include "packet.h"
#ifdef GSSAPI
#include "ssh-gss.h"
#endif
#include "authfile.h"
#include "monitor_wrap.h"

#ifdef HAVE_LOGIN_CAP
#include <login_cap.h>
#endif

/* import */
extern ServerOptions options;
extern int use_privsep;

/* Debugging messages */
Buffer auth_debug;
int auth_debug_init;

/*
 * Check if the user is allowed to log in via ssh. If user is listed
 * in DenyUsers or one of user's groups is listed in DenyGroups, false
 * will be returned. If AllowUsers isn't empty and user isn't listed
 * there, or if AllowGroups isn't empty and one of user's groups isn't
 * listed there, false will be returned.
 * If the user's shell is not executable, false will be returned.
 * Otherwise true is returned.
 */
int
allowed_user(struct passwd * pw)
{
#ifdef HAVE_LOGIN_CAP
	extern login_cap_t *lc;
	int match_name, match_ip;
	char *cap_hlist, *hp;
#endif
	struct stat st;
	const char *hostname = NULL, *ipaddr = NULL;
	u_int i;

	/* Shouldn't be called if pw is NULL, but better safe than sorry... */
	if (!pw || !pw->pw_name)
		return 0;

#ifdef HAVE_LOGIN_CAP
	hostname = get_canonical_hostname(1);
	ipaddr = get_remote_ipaddr();

	lc = login_getclass(pw->pw_class);

	/*
	 * Check the deny list.
	 */
	cap_hlist = login_getcapstr(lc, "host.deny", NULL, NULL);
	if (cap_hlist != NULL) {
		hp = strtok(cap_hlist, ",");
		while (hp != NULL) {
			match_name = match_hostname(hostname,
			    hp, strlen(hp));
			match_ip = match_hostname(ipaddr,
			    hp, strlen(hp));
			/*
			 * Only a positive match here causes a "deny".
			 */
			if (match_name > 0 || match_ip > 0) {
				free(cap_hlist);
				login_close(lc);
				return 0;
			}
			hp = strtok(NULL, ",");
		}
		free(cap_hlist);
	}

	/*
	 * Check the allow list.  If the allow list exists, and the
	 * remote host is not in it, the user is implicitly denied.
	 */
	cap_hlist = login_getcapstr(lc, "host.allow", NULL, NULL);
	if (cap_hlist != NULL) {
		hp = strtok(cap_hlist, ",");
		if (hp == NULL) {
			/* Just in case there's an empty string... */
			free(cap_hlist);
			login_close(lc);
			return 0;
		}
		while (hp != NULL) {
			match_name = match_hostname(hostname,
			    hp, strlen(hp));
			match_ip = match_hostname(ipaddr,
			    hp, strlen(hp));
			/*
			 * Negative match causes an immediate "deny".
			 * Positive match causes us to break out
			 * of the loop (allowing a fallthrough).
			 */
			if (match_name < 0 || match_ip < 0) {
				free(cap_hlist);
				login_close(lc);
				return 0;
			}
			if (match_name > 0 || match_ip > 0)
				break;
			hp = strtok(NULL, ",");
		}
		free(cap_hlist);
		if (hp == NULL) {
			login_close(lc);
			return 0;
		}
	}

	login_close(lc);
#endif

#ifdef USE_PAM
	if (!options.use_pam) {
#endif
	/*
	 * password/account expiration.
	 */
	if (pw->pw_change || pw->pw_expire) {
		struct timeval tv;

		(void)gettimeofday(&tv, (struct timezone *)NULL);
		if (pw->pw_expire) {
			if (tv.tv_sec >= pw->pw_expire) {
				logit("User %.100s not allowed because account has expired",
				    pw->pw_name);
				return 0;	/* expired */
			}
		}
#ifdef _PASSWORD_CHGNOW
		if (pw->pw_change == _PASSWORD_CHGNOW) {
			logit("User %.100s not allowed because password needs to be changed",
			    pw->pw_name);

			return 0;	/* can't force password change (yet) */
		}
#endif
		if (pw->pw_change) {
			if (tv.tv_sec >= pw->pw_change) {
				logit("User %.100s not allowed because password has expired",
				    pw->pw_name);
				return 0;	/* expired */
			}
		}
	}
#ifdef USE_PAM
	}
#endif

	/*
	 * Deny if shell does not exist or is not executable unless we
	 * are chrooting.
	 */
	/*
	 * XXX Should check to see if it is executable by the
	 * XXX requesting user.  --thorpej
	 */
	if (options.chroot_directory == NULL ||
	    strcasecmp(options.chroot_directory, "none") == 0) {
		char *shell = xstrdup((pw->pw_shell[0] == '\0') ?
		    _PATH_BSHELL : pw->pw_shell); /* empty = /bin/sh */

		if (stat(shell, &st) != 0) {
			logit("User %.100s not allowed because shell %.100s "
			    "does not exist", pw->pw_name, shell);
			xfree(shell);
			return 0;
		}
		if (S_ISREG(st.st_mode) == 0 ||
		    (st.st_mode & (S_IXOTH|S_IXUSR|S_IXGRP)) == 0) {
			logit("User %.100s not allowed because shell %.100s "
			    "is not executable", pw->pw_name, shell);
			xfree(shell);
			return 0;
		}
		xfree(shell);
	}
	/*
	 * XXX Consider nuking {Allow,Deny}{Users,Groups}.  We have the
	 * XXX login_cap(3) mechanism which covers all other types of
	 * XXX logins, too.
	 */

	if (options.num_deny_users > 0 || options.num_allow_users > 0 ||
	    options.num_deny_groups > 0 || options.num_allow_groups > 0) {
		hostname = get_canonical_hostname(options.use_dns);
		ipaddr = get_remote_ipaddr();
	}

	/* Return false if user is listed in DenyUsers */
	if (options.num_deny_users > 0) {
		for (i = 0; i < options.num_deny_users; i++)
			if (match_user(pw->pw_name, hostname, ipaddr,
			    options.deny_users[i])) {
				logit("User %.100s from %.100s not allowed "
				    "because listed in DenyUsers",
				    pw->pw_name, hostname);
				return 0;
			}
	}
	/* Return false if AllowUsers isn't empty and user isn't listed there */
	if (options.num_allow_users > 0) {
		for (i = 0; i < options.num_allow_users; i++)
			if (match_user(pw->pw_name, hostname, ipaddr,
			    options.allow_users[i]))
				break;
		/* i < options.num_allow_users iff we break for loop */
		if (i >= options.num_allow_users) {
			logit("User %.100s from %.100s not allowed because "
			    "not listed in AllowUsers", pw->pw_name, hostname);
			return 0;
		}
	}
	if (options.num_deny_groups > 0 || options.num_allow_groups > 0) {
		/* Get the user's group access list (primary and supplementary) */
		if (ga_init(pw->pw_name, pw->pw_gid) == 0) {
			logit("User %.100s from %.100s not allowed because "
			    "not in any group", pw->pw_name, hostname);
			return 0;
		}

		/* Return false if one of user's groups is listed in DenyGroups */
		if (options.num_deny_groups > 0)
			if (ga_match(options.deny_groups,
			    options.num_deny_groups)) {
				ga_free();
				logit("User %.100s from %.100s not allowed "
				    "because a group is listed in DenyGroups",
				    pw->pw_name, hostname);
				return 0;
			}
		/*
		 * Return false if AllowGroups isn't empty and one of user's groups
		 * isn't listed there
		 */
		if (options.num_allow_groups > 0)
			if (!ga_match(options.allow_groups,
			    options.num_allow_groups)) {
				ga_free();
				logit("User %.100s from %.100s not allowed "
				    "because none of user's groups are listed "
				    "in AllowGroups", pw->pw_name, hostname);
				return 0;
			}
		ga_free();
	}
	/* We found no reason not to let this user try to log on... */
	return 1;
}

void
auth_log(Authctxt *authctxt, int authenticated, const char *method,
    const char *info)
{
	void (*authlog) (const char *fmt,...) = verbose;
	const char *authmsg;

	if (use_privsep && !mm_is_monitor() && !authctxt->postponed)
		return;

	/* Raise logging level */
	if (authenticated == 1 ||
	    !authctxt->valid ||
	    authctxt->failures >= options.max_authtries / 2 ||
	    strcmp(method, "password") == 0)
		authlog = logit;

	if (authctxt->postponed)
		authmsg = "Postponed";
	else
		authmsg = authenticated ? "Accepted" : "Failed";

	authlog("%s %s for %s%.100s from %.200s port %d%s",
	    authmsg,
	    method,
	    authctxt->valid ? "" : "invalid user ",
	    authctxt->user,
	    get_remote_ipaddr(),
	    get_remote_port(),
	    info);
}

/*
 * Check whether root logins are disallowed.
 */
int
auth_root_allowed(const char *method)
{
	switch (options.permit_root_login) {
	case PERMIT_YES:
		return 1;
	case PERMIT_NO_PASSWD:
		if (strcmp(method, "password") != 0)
			return 1;
		break;
	case PERMIT_FORCED_ONLY:
		if (forced_command) {
			logit("Root login accepted for forced command.");
			return 1;
		}
		break;
	}
	logit("ROOT LOGIN REFUSED FROM %.200s", get_remote_ipaddr());
	return 0;
}


/*
 * Given a template and a passwd structure, build a filename
 * by substituting % tokenised options. Currently, %% becomes '%',
 * %h becomes the home directory and %u the username.
 *
 * This returns a buffer allocated by xmalloc.
 */
char *
expand_authorized_keys(const char *filename, struct passwd *pw)
{
	char *file, ret[MAXPATHLEN];
	int i;

	file = percent_expand(filename, "h", pw->pw_dir,
	    "u", pw->pw_name, (char *)NULL);

	/*
	 * Ensure that filename starts anchored. If not, be backward
	 * compatible and prepend the '%h/'
	 */
	if (*file == '/')
		return (file);

	i = snprintf(ret, sizeof(ret), "%s/%s", pw->pw_dir, file);
	if (i < 0 || (size_t)i >= sizeof(ret))
		fatal("expand_authorized_keys: path too long");
	xfree(file);
	return (xstrdup(ret));
}

char *
authorized_principals_file(struct passwd *pw)
{
	if (options.authorized_principals_file == NULL ||
	    strcasecmp(options.authorized_principals_file, "none") == 0)
		return NULL;
	return expand_authorized_keys(options.authorized_principals_file, pw);
}

/* return ok if key exists in sysfile or userfile */
HostStatus
check_key_in_hostfiles(struct passwd *pw, Key *key, const char *host,
    const char *sysfile, const char *userfile)
{
	char *user_hostfile;
	struct stat st;
	HostStatus host_status;
	struct hostkeys *hostkeys;
	const struct hostkey_entry *found;

	hostkeys = init_hostkeys();
	load_hostkeys(hostkeys, host, sysfile);
	if (userfile != NULL) {
		user_hostfile = tilde_expand_filename(userfile, pw->pw_uid);
		if (options.strict_modes &&
		    (stat(user_hostfile, &st) == 0) &&
		    ((st.st_uid != 0 && st.st_uid != pw->pw_uid) ||
		    (st.st_mode & 022) != 0)) {
			logit("Authentication refused for %.100s: "
			    "bad owner or modes for %.200s",
			    pw->pw_name, user_hostfile);
			auth_debug_add("Ignored %.200s: bad ownership or modes",
			    user_hostfile);
		} else {
			temporarily_use_uid(pw);
			load_hostkeys(hostkeys, host, user_hostfile);
			restore_uid();
		}
		xfree(user_hostfile);
	}
	host_status = check_key_in_hostkeys(hostkeys, key, &found);
	if (host_status == HOST_REVOKED)
		error("WARNING: revoked key for %s attempted authentication",
		    found->host);
	else if (host_status == HOST_OK)
		debug("%s: key for %s found at %s:%ld", __func__,
		    found->host, found->file, found->line);
	else
		debug("%s: key for host %s not found", __func__, host);

	free_hostkeys(hostkeys);

	return host_status;
}


/*
 * Check a given file for security. This is defined as all components
 * of the path to the file must be owned by either the owner of
 * of the file or root and no directories must be group or world writable.
 *
 * XXX Should any specific check be done for sym links ?
 *
 * Takes an open file descriptor, the file name, a uid and and
 * error buffer plus max size as arguments.
 *
 * Returns 0 on success and -1 on failure
 */
static int
secure_filename(FILE *f, const char *file, struct passwd *pw,
    char *err, size_t errlen)
{
	uid_t uid = pw->pw_uid;
	char buf[MAXPATHLEN], homedir[MAXPATHLEN];
	char *cp;
	int comparehome = 0;
	struct stat st;

	if (realpath(file, buf) == NULL) {
		snprintf(err, errlen, "realpath %s failed: %s", file,
		    strerror(errno));
		return -1;
	}
	if (realpath(pw->pw_dir, homedir) != NULL)
		comparehome = 1;

	/* check the open file to avoid races */
	if (fstat(fileno(f), &st) < 0 ||
	    (st.st_uid != 0 && st.st_uid != uid) ||
	    (st.st_mode & 022) != 0) {
		snprintf(err, errlen, "bad ownership or modes for file %s",
		    buf);
		return -1;
	}

	/* for each component of the canonical path, walking upwards */
	for (;;) {
		if ((cp = dirname(buf)) == NULL) {
			snprintf(err, errlen, "dirname() failed");
			return -1;
		}
		strlcpy(buf, cp, sizeof(buf));

		if (stat(buf, &st) < 0 ||
		    (st.st_uid != 0 && st.st_uid != uid) ||
		    (st.st_mode & 022) != 0) {
			snprintf(err, errlen,
			    "bad ownership or modes for directory %s", buf);
			return -1;
		}

		/* If are past the homedir then we can stop */
		if (comparehome && strcmp(homedir, buf) == 0)
			break;

		/*
		 * dirname should always complete with a "/" path,
		 * but we can be paranoid and check for "." too
		 */
		if ((strcmp("/", buf) == 0) || (strcmp(".", buf) == 0))
			break;
	}
	return 0;
}

static FILE *
auth_openfile(const char *file, struct passwd *pw, int strict_modes,
    int log_missing, const char *file_type)
{
	char line[1024];
	struct stat st;
	int fd;
	FILE *f;

	if ((fd = open(file, O_RDONLY|O_NONBLOCK)) == -1) {
		if (log_missing || errno != ENOENT)
			debug("Could not open %s '%s': %s", file_type, file,
			   strerror(errno));
		return NULL;
	}

	if (fstat(fd, &st) < 0) {
		close(fd);
		return NULL;
	}
	if (!S_ISREG(st.st_mode)) {
		logit("User %s %s %s is not a regular file",
		    pw->pw_name, file_type, file);
		close(fd);
		return NULL;
	}
	unset_nonblock(fd);
	if ((f = fdopen(fd, "r")) == NULL) {
		close(fd);
		return NULL;
	}
	if (strict_modes &&
	    secure_filename(f, file, pw, line, sizeof(line)) != 0) {
		fclose(f);
		logit("Authentication refused: %s", line);
		auth_debug_add("Ignored %s: %s", file_type, line);
		return NULL;
	}

	return f;
}


FILE *
auth_openkeyfile(const char *file, struct passwd *pw, int strict_modes)
{
	return auth_openfile(file, pw, strict_modes, 1, "authorized keys");
}

FILE *
auth_openprincipals(const char *file, struct passwd *pw, int strict_modes)
{
	return auth_openfile(file, pw, strict_modes, 0,
	    "authorized principals");
}

struct passwd *
getpwnamallow(const char *user)
{
#ifdef HAVE_LOGIN_CAP
 	extern login_cap_t *lc;
#ifdef BSD_AUTH
 	auth_session_t *as;
#endif
#endif
	struct passwd *pw;
	struct connection_info *ci = get_connection_info(1, options.use_dns);

	ci->user = user;
	parse_server_match_config(&options, ci);

	pw = getpwnam(user);
	if (pw == NULL) {
		logit("Invalid user %.100s from %.100s",
		    user, get_remote_ipaddr());
		return (NULL);
	}
	if (!allowed_user(pw))
		return (NULL);
#ifdef HAVE_LOGIN_CAP
	if ((lc = login_getclass(pw->pw_class)) == NULL) {
		debug("unable to get login class: %s", user);
		return (NULL);
	}
#ifdef BSD_AUTH
	if ((as = auth_open()) == NULL || auth_setpwd(as, pw) != 0 ||
	    auth_approval(as, lc, pw->pw_name, "ssh") <= 0) {
		debug("Approval failure for %s", user);
		pw = NULL;
	}
	if (as != NULL)
		auth_close(as);
#endif
#endif
	if (pw != NULL)
		return (pwcopy(pw));
	return (NULL);
}

/* Returns 1 if key is revoked by revoked_keys_file, 0 otherwise */
int
auth_key_is_revoked(Key *key)
{
	char *key_fp;

	if (options.revoked_keys_file == NULL)
		return 0;

	switch (key_in_file(key, options.revoked_keys_file, 0)) {
	case 0:
		/* key not revoked */
		return 0;
	case -1:
		/* Error opening revoked_keys_file: refuse all keys */
		error("Revoked keys file is unreadable: refusing public key "
		    "authentication");
		return 1;
	case 1:
		/* Key revoked */
		key_fp = key_fingerprint(key, SSH_FP_MD5, SSH_FP_HEX);
		error("WARNING: authentication attempt with a revoked "
		    "%s key %s ", key_type(key), key_fp);
		xfree(key_fp);
		return 1;
	}
	fatal("key_in_file returned junk");
}

void
auth_debug_add(const char *fmt,...)
{
	char buf[1024];
	va_list args;

	if (!auth_debug_init)
		return;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	buffer_put_cstring(&auth_debug, buf);
}

void
auth_debug_send(void)
{
	char *msg;

	if (!auth_debug_init)
		return;
	while (buffer_len(&auth_debug)) {
		msg = buffer_get_string(&auth_debug, NULL);
		packet_send_debug("%s", msg);
		xfree(msg);
	}
}

void
auth_debug_reset(void)
{
	if (auth_debug_init)
		buffer_clear(&auth_debug);
	else {
		buffer_init(&auth_debug);
		auth_debug_init = 1;
	}
}

struct passwd *
fakepw(void)
{
	static struct passwd fake;
	static char nouser[] = "NOUSER";
	static char nonexist[] = "/nonexist";

	memset(&fake, 0, sizeof(fake));
	fake.pw_name = nouser;
	fake.pw_passwd = __UNCONST(
	    "$2a$06$r3.juUaHZDlIbQaO2dS9FuYxL1W9M81R1Tc92PoSNmzvpEqLkLGrK");
	fake.pw_gecos = nouser;
	fake.pw_uid = (uid_t)-1;
	fake.pw_gid = (gid_t)-1;
	fake.pw_class = __UNCONST("");
	fake.pw_dir = nonexist;
	fake.pw_shell = nonexist;

	return (&fake);
}
