/*	$NetBSD: sshlogin.c,v 1.12 2003/08/26 16:48:34 wiz Exp $	*/
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
RCSID("$OpenBSD: sshlogin.c,v 1.5 2002/08/29 15:57:25 stevesk Exp $");
__RCSID("$NetBSD: sshlogin.c,v 1.12 2003/08/26 16:48:34 wiz Exp $");

#include <util.h>
#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif
#include "sshlogin.h"
#include "log.h"

/*
 * Returns the time when the user last logged in.  Returns 0 if the
 * information is not available.  This must be called before record_login.
 * The host the user logged in from will be returned in buf.
 */
u_long
get_last_login_time(uid_t uid, const char *logname,
    char *buf, u_int bufsize)
{
#ifdef SUPPORT_UTMPX
	struct lastlogx llx, *llxp;
#endif
#ifdef SUPPORT_UTMP
	struct lastlog ll;
	int fd;
#endif

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
	lseek(fd, (off_t) ((long) uid * sizeof(ll)), SEEK_SET);
	if (read(fd, &ll, sizeof(ll)) != sizeof(ll)) {
		close(fd);
		return 0;
	}
	close(fd);
	if (bufsize > sizeof(ll.ll_host) + 1)
		bufsize = sizeof(ll.ll_host) + 1;
	strncpy(buf, ll.ll_host, bufsize - 1);
	buf[bufsize - 1] = 0;
	return ll.ll_time;
#else
	return 0;
#endif
}

/*
 * Records that the user has logged in.  I these parts of operating systems
 * were more standardized.
 */
void
record_login(pid_t pid, const char *ttyname, const char *user, uid_t uid,
    const char *host, struct sockaddr * addr, socklen_t addrlen)
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
#endif
	(void)gettimeofday(&tv, NULL);
	/*
	 * XXX: why do we need to handle logout cases here?
	 * Isn't the function below taking care of this?
	 */
#ifdef SUPPORT_UTMP
	/* Construct an utmp/wtmp entry. */
	memset(&u, 0, sizeof(u));
	strncpy(u.ut_line, ttyname + 5, sizeof(u.ut_line));
	u.ut_time = (time_t)tv.tv_sec;
	strncpy(u.ut_name, user, sizeof(u.ut_name));
	strncpy(u.ut_host, host, sizeof(u.ut_host));

	login(&u);

	/* Update lastlog unless actually recording a logout. */
	if (*user != '\0') {
		/*
		 * It is safer to bzero the lastlog structure first because
		 * some systems might have some extra fields in it (e.g. SGI)
		 */
		memset(&ll, 0, sizeof(ll));

		/* Update lastlog. */
		ll.ll_time = time(NULL);
		strncpy(ll.ll_line, ttyname + 5, sizeof(ll.ll_line));
		strncpy(ll.ll_host, host, sizeof(ll.ll_host));
		fd = open(_PATH_LASTLOG, O_RDWR);
		if (fd >= 0) {
			lseek(fd, (off_t) ((long) uid * sizeof(ll)), SEEK_SET);
			if (write(fd, &ll, sizeof(ll)) != sizeof(ll))
				logit("Could not write %.100s: %.100s",
				    _PATH_LASTLOG, strerror(errno));
			close(fd);
		}
	}
#endif
#ifdef SUPPORT_UTMPX
	/* Construct an utmpx/wtmpx entry. */
	memset(&ux, 0, sizeof(ux));
	strncpy(ux.ut_line, ttyname + 5, sizeof(ux.ut_line));
	if (*user) {
		ux.ut_pid = pid;
		ux.ut_type = USER_PROCESS;
		ux.ut_tv = tv;
		strncpy(ux.ut_name, user, sizeof(ux.ut_name));
		strncpy(ux.ut_host, host, sizeof(ux.ut_host));
		/* XXX: need ut_id, use last 4 char of ttyname */
		if (strlen(ttyname) > sizeof(ux.ut_id)) {
			strncpy(ux.ut_id,
			    ttyname + strlen(ttyname) - sizeof(ux.ut_id),
			    sizeof(ux.ut_id));
		} else
			strncpy(ux.ut_id, ttyname, sizeof(ux.ut_id));
		/* XXX: It would be better if we had sockaddr_storage here */
		memcpy(&ux.ut_ss, addr, sizeof(*addr));
		if (pututxline(&ux) == NULL)
			logit("could not add utmpx line: %.100s",
			    strerror(errno));
	} else {
		if ((uxp = getutxline(&ux)) == NULL)
			logit("could not find utmpx line for %.100s",
			    ttyname);
		uxp->ut_type = DEAD_PROCESS;
		uxp->ut_tv = tv;
		/* XXX: we don't record exit info yet */
		if (pututxline(&ux) == NULL)
			logit("could not replace utmpx line: %.100s",
			    strerror(errno));
	}
	endutxent();
	updwtmpx(_PATH_WTMPX, uxp);
#endif
}

/* Records that the user has logged out. */
void
record_logout(pid_t pid, const char *ttyname)
{
#if defined(SUPPORT_UTMP) || defined(SUPPORT_UTMPX)
	const char *line = ttyname + 5;	/* /dev/ttyq8 -> ttyq8 */
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
