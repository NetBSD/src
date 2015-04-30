/*	$NetBSD: sshlogin.c,v 1.4.22.1 2015/04/30 06:07:31 riz Exp $	*/
/* $OpenBSD: sshlogin.c,v 1.31 2015/01/20 23:14:00 deraadt Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * This file performs some of the things login(1) normally does.  We cannot
 * easily use something like login -p -h host -f user, because there are
 * several different logins around, and it is hard to determined what kind of
 * login the current system has.  Also, we want to be able to execute commands
 * on a tty.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 * Copyright (c) 1999 Theo de Raadt.  All rights reserved.
 * Copyright (c) 1999 Markus Friedl.  All rights reserved.
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
__RCSID("$NetBSD: sshlogin.c,v 1.4.22.1 2015/04/30 06:07:31 riz Exp $");
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>
#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif
#include <stdarg.h>
#include <limits.h>

#include "sshlogin.h"
#include "log.h"
#include "buffer.h"
#include "misc.h"
#include "servconf.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX MAXHOSTNAMELEN
#endif

extern Buffer loginmsg;
extern ServerOptions options;

/*
 * Returns the time when the user last logged in.  Returns 0 if the
 * information is not available.  This must be called before record_login.
 * The host the user logged in from will be returned in buf.
 */
time_t
get_last_login_time(uid_t uid, const char *logname,
    char *buf, size_t bufsize)
{
#ifdef SUPPORT_UTMPX
	struct lastlogx llx, *llxp;
#endif
#ifdef SUPPORT_UTMP
	struct lastlog ll;
	int fd;
#endif
	off_t pos, r;

	buf[0] = '\0';
#ifdef SUPPORT_UTMPX
	if ((llxp = getlastlogx(_PATH_LASTLOGX, uid, &llx)) != NULL) {
		if (bufsize > sizeof(llxp->ll_host) + 1)
			bufsize = sizeof(llxp->ll_host) + 1;
		strncpy(buf, llxp->ll_host, bufsize - 1);
		buf[bufsize - 1] = 0;
		return llxp->ll_tv.tv_sec;
	}
#endif
#ifdef SUPPORT_UTMP
	fd = open(_PATH_LASTLOG, O_RDONLY);
	if (fd < 0)
		return 0;

	pos = (long) uid * sizeof(ll);
	r = lseek(fd, pos, SEEK_SET);
	if (r == -1) {
		error("%s: lseek: %s", __func__, strerror(errno));
		close(fd);
		return (0);
	}
	if (r != pos) {
		debug("%s: truncated lastlog", __func__);
		close(fd);
		return (0);
	}
	if (read(fd, &ll, sizeof(ll)) != sizeof(ll)) {
		close(fd);
		return 0;
	}
	close(fd);
	if (bufsize > sizeof(ll.ll_host) + 1)
		bufsize = sizeof(ll.ll_host) + 1;
	strncpy(buf, ll.ll_host, bufsize - 1);
	buf[bufsize - 1] = '\0';
	return (time_t)ll.ll_time;
#else
	return 0;
#endif
}

/*
 * Generate and store last login message.  This must be done before
 * login_login() is called and lastlog is updated.
 */
static void
store_lastlog_message(const char *user, uid_t uid)
{
	char *time_string, hostname[HOST_NAME_MAX+1] = "", buf[512];
	time_t last_login_time;

	if (!options.print_lastlog)
		return;

	last_login_time = get_last_login_time(uid, user, hostname,
	    sizeof(hostname));

	if (last_login_time != 0) {
		if ((time_string = ctime(&last_login_time)) != NULL)
			time_string[strcspn(time_string, "\n")] = '\0';
		if (strcmp(hostname, "") == 0)
			snprintf(buf, sizeof(buf), "Last login: %s\r\n",
			    time_string);
		else
			snprintf(buf, sizeof(buf), "Last login: %s from %s\r\n",
			    time_string, hostname);
		buffer_append(&loginmsg, buf, strlen(buf));
	}
}

/*
 * Records that the user has logged in.  I wish these parts of operating
 * systems were more standardized.
 */
void
record_login(pid_t pid, const char *tty, const char *user, uid_t uid,
    const char *host, struct sockaddr *addr, socklen_t addrlen)
{
#if defined(SUPPORT_UTMP) || defined(SUPPORT_UTMPX)
	int fd;
#endif
	struct timeval tv;
#ifdef SUPPORT_UTMP
	struct utmp u;
	struct lastlog ll;
#endif
#ifdef SUPPORT_UTMPX
	struct utmpx ux, *uxp = &ux;
	struct lastlogx llx;
#endif
	(void)gettimeofday(&tv, NULL);
	/*
	 * XXX: why do we need to handle logout cases here?
	 * Isn't the function below taking care of this?
	 */
	/* save previous login details before writing new */
	store_lastlog_message(user, uid);

#ifdef SUPPORT_UTMP
	/* Construct an utmp/wtmp entry. */
	memset(&u, 0, sizeof(u));
	strncpy(u.ut_line, tty + 5, sizeof(u.ut_line));
	u.ut_time = (time_t)tv.tv_sec;
	strncpy(u.ut_name, user, sizeof(u.ut_name));
	strncpy(u.ut_host, host, sizeof(u.ut_host));

	login(&u);

	/* Update lastlog unless actually recording a logout. */
	if (*user != '\0') {
		/*
		 * It is safer to memset the lastlog structure first because
		 * some systems might have some extra fields in it (e.g. SGI)
		 */
		memset(&ll, 0, sizeof(ll));

		/* Update lastlog. */
		ll.ll_time = time(NULL);
		strncpy(ll.ll_line, tty + 5, sizeof(ll.ll_line));
		strncpy(ll.ll_host, host, sizeof(ll.ll_host));
		fd = open(_PATH_LASTLOG, O_RDWR);
		if (fd >= 0) {
			lseek(fd, (off_t) ((long) uid * sizeof(ll)), SEEK_SET);
			if (write(fd, &ll, sizeof(ll)) != sizeof(ll))
				logit("Could not write %.100s: %.100s", _PATH_LASTLOG, strerror(errno));
			close(fd);
		}
	}
#endif
#ifdef SUPPORT_UTMPX
	/* Construct an utmpx/wtmpx entry. */
	memset(&ux, 0, sizeof(ux));
	strncpy(ux.ut_line, tty + 5, sizeof(ux.ut_line));
	if (*user) {
		ux.ut_pid = pid;
		ux.ut_type = USER_PROCESS;
		ux.ut_tv = tv;
		strncpy(ux.ut_name, user, sizeof(ux.ut_name));
		strncpy(ux.ut_host, host, sizeof(ux.ut_host));
		/* XXX: need ut_id, use last 4 char of tty */
		if (strlen(tty) > sizeof(ux.ut_id)) {
			strncpy(ux.ut_id,
			    tty + strlen(tty) - sizeof(ux.ut_id),
			    sizeof(ux.ut_id));
		} else
			strncpy(ux.ut_id, tty, sizeof(ux.ut_id));
		/* XXX: It would be better if we had sockaddr_storage here */
		if (addrlen > sizeof(ux.ut_ss))
			addrlen = sizeof(ux.ut_ss);
		(void)memcpy(&ux.ut_ss, addr, addrlen);
		if (pututxline(&ux) == NULL)
			logit("could not add utmpx line: %.100s",
			    strerror(errno));
		/* Update lastlog. */
		(void)gettimeofday(&llx.ll_tv, NULL);
		strncpy(llx.ll_line, tty + 5, sizeof(llx.ll_line));
		strncpy(llx.ll_host, host, sizeof(llx.ll_host));
		(void)memcpy(&llx.ll_ss, addr, addrlen);
		if (updlastlogx(_PATH_LASTLOGX, uid, &llx) == -1)
			logit("Could not update %.100s: %.100s",
			    _PATH_LASTLOGX, strerror(errno));
	} else {
		if ((uxp = getutxline(&ux)) == NULL)
			logit("could not find utmpx line for %.100s", tty);
		else {
			uxp->ut_type = DEAD_PROCESS;
			uxp->ut_tv = tv;
			/* XXX: we don't record exit info yet */
			if (pututxline(&ux) == NULL)
				logit("could not replace utmpx line: %.100s",
				    strerror(errno));
		}
	}
	endutxent();
	updwtmpx(_PATH_WTMPX, uxp);
#endif
}

/* Records that the user has logged out. */
void
record_logout(pid_t pid, const char *tty)
{
#if defined(SUPPORT_UTMP) || defined(SUPPORT_UTMPX)
	const char *line = tty + 5;	/* /dev/ttyq8 -> ttyq8 */
#endif
#ifdef SUPPORT_UTMP
	if (logout(line))
		logwtmp(line, "", "");
#endif
#ifdef SUPPORT_UTMPX
	/* XXX: no exit info yet */
	if (logoutx(line, 0, DEAD_PROCESS))
		logwtmpx(line, "", "", 0, DEAD_PROCESS);
#endif
}
