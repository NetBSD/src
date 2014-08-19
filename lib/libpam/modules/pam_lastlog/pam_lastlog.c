/*	$NetBSD: pam_lastlog.c,v 1.14.6.1 2014/08/20 00:02:19 tls Exp $	*/

/*-
 * Copyright (c) 1980, 1987, 1988, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2001 Mark R V Murray
 * All rights reserved.
 * Copyright (c) 2001 Networks Associates Technology, Inc.
 * All rights reserved.
 * Copyright (c) 2004 Joe R. Doupnik
 * All rights reserved.
 *
 * Portions of this software were developed for the FreeBSD Project by
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
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
__FBSDID("$FreeBSD: src/lib/libpam/modules/pam_lastlog/pam_lastlog.c,v 1.20 2004/01/26 19:28:37 des Exp $");
#else
__RCSID("$NetBSD: pam_lastlog.c,v 1.14.6.1 2014/08/20 00:02:19 tls Exp $");
#endif

#include <sys/param.h>

#include <fcntl.h>
#include <util.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef LOGIN_CAP
#include <login_cap.h>
#endif

#define PAM_SM_SESSION

#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_mod_misc.h>

#ifdef SUPPORT_UTMP
#include <utmp.h>
static void doutmp(const char *, const char *, const char *,
    const struct timeval *);
static void dolastlog(pam_handle_t *, int, const struct passwd *, const char *,
    const char *, const struct timeval *);
#endif

#ifdef SUPPORT_UTMPX
#include <utmpx.h>
static void doutmpx(const char *, const char *, const char *,
    const struct sockaddr_storage *ss, const struct timeval *);
static void dolastlogx(pam_handle_t *, int, const struct passwd *, const char *,
    const char *, const struct sockaddr_storage *ss, const struct timeval *);
#endif

#if defined(SUPPORT_UTMPX) || defined(SUPPORT_UTMP)
static void domsg(pam_handle_t *, time_t, const char *, size_t, const char *,
    size_t);
#endif

__printflike(2, 3)
static void
logit(int level, const char *fmt, ...)
{
	va_list ap;
	struct syslog_data data = SYSLOG_DATA_INIT;

	openlog_r("pam_lastlog", LOG_PID, LOG_AUTHPRIV, &data);
	va_start(ap, fmt);
	vsyslog_r(level, &data, fmt, ap);
	va_end(ap);
	closelog_r(&data);
}


PAM_EXTERN int
pam_sm_open_session(pam_handle_t *pamh, int flags,
    int argc __unused, const char *argv[] __unused)
{
	struct passwd *pwd, pwres;
	struct timeval now;
	const char *user, *rhost, *tty, *nuser;
	const void *vrhost, *vtty, *vss, *vnuser;
	const struct sockaddr_storage *ss;
	int pam_err;
	char pwbuf[1024];
#ifdef LOGIN_CAP
	login_cap_t *lc;
#endif

	pam_err = pam_get_user(pamh, &user, NULL);
	if (pam_err != PAM_SUCCESS)
		return pam_err;

	if (user == NULL ||
	    getpwnam_r(user, &pwres, pwbuf, sizeof(pwbuf), &pwd) != 0 ||
	    pwd == NULL)
		return PAM_SERVICE_ERR;

	PAM_LOG("Got user: %s", user);

	pam_err = pam_get_item(pamh, PAM_RHOST, &vrhost);
	if (pam_err != PAM_SUCCESS)
		goto err;
	rhost = (const char *)vrhost;

	pam_err = pam_get_item(pamh, PAM_SOCKADDR, &vss);
	if (pam_err != PAM_SUCCESS)
		goto err;
	ss = (const struct sockaddr_storage *)vss;

	pam_err = pam_get_item(pamh, PAM_TTY, &vtty);
	if (pam_err != PAM_SUCCESS)
		goto err;
	tty = (const char *)vtty;

	if (tty == NULL) {
		pam_err = PAM_SERVICE_ERR;
		goto err;
	}

	if (pam_get_item(pamh, PAM_NUSER, &vnuser) != PAM_SUCCESS)
		nuser = NULL;
	else
		nuser = (const char *)vnuser;

	if (strncmp(tty, _PATH_DEV, strlen(_PATH_DEV)) == 0)
		tty = tty + strlen(_PATH_DEV);

	if (*tty == '\0') {
		pam_err = PAM_SERVICE_ERR;
		goto err;
	}

	(void)gettimeofday(&now, NULL);

	if (openpam_get_option(pamh, "no_nested") == NULL || nuser == NULL) {
		int quiet;
		if ((flags & PAM_SILENT) != 0)
			quiet = 1;
		else {
#ifdef LOGIN_CAP
			lc = login_getpwclass(pwd);
			quiet = login_getcapbool(lc, "hushlogin", 0);
			login_close(lc);
#else
			quiet = 0;
#endif
		}
#ifdef SUPPORT_UTMPX
		doutmpx(user, rhost, tty, ss, &now);
		dolastlogx(pamh, quiet, pwd, rhost, tty, ss, &now);
		quiet = 1;
#endif
#ifdef SUPPORT_UTMP
		doutmp(user, rhost, tty, &now);
		dolastlog(pamh, quiet, pwd, rhost, tty, &now);
#endif
	}
err:
	if (openpam_get_option(pamh, "no_fail"))
		return PAM_SUCCESS;
	return pam_err;
}

PAM_EXTERN int
pam_sm_close_session(pam_handle_t *pamh __unused, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	const void *vtty, *vnuser;
	const char *tty, *nuser;

	if (pam_get_item(pamh, PAM_NUSER, &vnuser) != PAM_SUCCESS)
		nuser = NULL;
	else
		nuser = (const char *)vnuser;

	pam_get_item(pamh, PAM_TTY, &vtty);
	if (vtty == NULL)
		return PAM_SERVICE_ERR;
	tty = (const char *)vtty;

	if (strncmp(tty, _PATH_DEV, strlen(_PATH_DEV)) == 0)
		tty = tty + strlen(_PATH_DEV);

	if (*tty == '\0')
		return PAM_SERVICE_ERR;

	if (openpam_get_option(pamh, "no_nested") == NULL || nuser == NULL) {

#ifdef SUPPORT_UTMPX
		if (logoutx(tty, 0, DEAD_PROCESS))
			logwtmpx(tty, "", "", 0, DEAD_PROCESS);
		else
			logit(LOG_NOTICE, "%s(): no utmpx record for %s",
			    __func__, tty);
#endif

#ifdef SUPPORT_UTMP
		if (logout(tty))
			logwtmp(tty, "", "");
		else
			logit(LOG_NOTICE, "%s(): no utmp record for %s",
			    __func__, tty);
#endif
	}
        return PAM_SUCCESS;
}

#if defined(SUPPORT_UTMPX) || defined(SUPPORT_UTMP)
static void
domsg(pam_handle_t *pamh, time_t t, const char *host, size_t hsize,
    const char *line, size_t lsize)
{
	char buf[MAXHOSTNAMELEN + 32], *promptresp = NULL;
	int pam_err;

	if (*host) {
		(void)snprintf(buf, sizeof(buf), "from %.*s ",
		    (int)hsize, host);
		host = buf;
	}

	pam_err = pam_prompt(pamh, PAM_TEXT_INFO, &promptresp,
	    "Last login: %.24s %son %.*s\n", ctime(&t), host, (int)lsize, line);

	if (pam_err == PAM_SUCCESS && promptresp)
		free(promptresp);
}
#endif

#ifdef SUPPORT_UTMPX
static void
doutmpx(const char *username, const char *hostname, const char *tty,
    const struct sockaddr_storage *ss, const struct timeval *now)
{
	struct utmpx utmpx;
	const char *t;

	memset((void *)&utmpx, 0, sizeof(utmpx));
	utmpx.ut_tv = *now;
	(void)strncpy(utmpx.ut_name, username, sizeof(utmpx.ut_name));
	if (hostname) {
		(void)strncpy(utmpx.ut_host, hostname, sizeof(utmpx.ut_host));
		if (ss)
			utmpx.ut_ss = *ss;
	}
	(void)strncpy(utmpx.ut_line, tty, sizeof(utmpx.ut_line));
	utmpx.ut_type = USER_PROCESS;
	utmpx.ut_pid = getpid();
	t = tty + strlen(tty);
	if ((size_t)(t - tty) >= sizeof(utmpx.ut_id)) {
		(void)strncpy(utmpx.ut_id, t - sizeof(utmpx.ut_id),
		    sizeof(utmpx.ut_id));
	} else {
		(void)strncpy(utmpx.ut_id, tty, sizeof(utmpx.ut_id));
	}
	if (pututxline(&utmpx) == NULL)
		logit(LOG_NOTICE, "Cannot update utmpx: %s", strerror(errno));
	endutxent();
	if (updwtmpx(_PATH_WTMPX, &utmpx) != 0)
		logit(LOG_NOTICE, "Cannot update wtmpx: %s", strerror(errno));
}

static void
dolastlogx(pam_handle_t *pamh, int quiet, const struct passwd *pwd,
    const char *hostname, const char *tty, const struct sockaddr_storage *ss,
    const struct timeval *now)
{
	struct lastlogx ll;
	if (!quiet) {
	    if (getlastlogx(_PATH_LASTLOGX, pwd->pw_uid, &ll) != NULL) 
		    domsg(pamh, (time_t)ll.ll_tv.tv_sec, ll.ll_host,
			sizeof(ll.ll_host), ll.ll_line,
			sizeof(ll.ll_line));
	}
	ll.ll_tv = *now;
	(void)strncpy(ll.ll_line, tty, sizeof(ll.ll_line));

	if (hostname)
		(void)strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
	else
		(void)memset(ll.ll_host, 0, sizeof(ll.ll_host));

	if (ss)
		ll.ll_ss = *ss;
	else
		(void)memset(&ll.ll_ss, 0, sizeof(ll.ll_ss));

	if (updlastlogx(_PATH_LASTLOGX, pwd->pw_uid, &ll) != 0)
		logit(LOG_NOTICE, "Cannot update lastlogx: %s", strerror(errno));
	PAM_LOG("Login recorded in %s", _PATH_LASTLOGX);
}
#endif

#ifdef SUPPORT_UTMP
static void
doutmp(const char *username, const char *hostname, const char *tty,
    const struct timeval *now)
{
	struct utmp utmp;

	(void)memset((void *)&utmp, 0, sizeof(utmp));
	utmp.ut_time = now->tv_sec;
	(void)strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
	if (hostname)
		(void)strncpy(utmp.ut_host, hostname, sizeof(utmp.ut_host));
	(void)strncpy(utmp.ut_line, tty, sizeof(utmp.ut_line));
	login(&utmp);
}

static void
dolastlog(pam_handle_t *pamh, int quiet, const struct passwd *pwd,
    const char *hostname, const char *tty, const struct timeval *now)
{
	struct lastlog ll;
	int fd;

	if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) == -1) {
		logit(LOG_NOTICE, "Cannot open `%s': %s", _PATH_LASTLOG,
		    strerror(errno));
		return;
	}
	(void)lseek(fd, (off_t)(pwd->pw_uid * sizeof(ll)), SEEK_SET);

	if (!quiet) {
		if (read(fd, (char *)&ll, sizeof(ll)) == sizeof(ll) &&
		    ll.ll_time != 0)
			domsg(pamh, ll.ll_time, ll.ll_host,
			    sizeof(ll.ll_host), ll.ll_line,
			    sizeof(ll.ll_line));
		(void)lseek(fd, (off_t)(pwd->pw_uid * sizeof(ll)), SEEK_SET);
	}

	ll.ll_time = now->tv_sec;
	(void)strncpy(ll.ll_line, tty, sizeof(ll.ll_line));

	if (hostname)
		(void)strncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
	else
		(void)memset(ll.ll_host, 0, sizeof(ll.ll_host));

	(void)write(fd, &ll, sizeof(ll));
	(void)close(fd);

	PAM_LOG("Login recorded in %s", _PATH_LASTLOG);
}
#endif

PAM_MODULE_ENTRY("pam_lastlog");
