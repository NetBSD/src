/*	$NetBSD: utempter.c,v 1.1 2017/04/23 02:02:00 christos Exp $	*/

/*-
 * Copyright (c) 2011, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: utempter.c,v 1.1 2017/04/23 02:02:00 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>
#include <paths.h>

#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif

#ifdef SUPPORT_UTMPX
static void
doutmpx(const char *username, const char *hostname, const char *tty,
    const struct timeval *now, int type, pid_t pid)
{
	struct utmpx ut;
	const char *t;

	(void)memset(&ut, 0, sizeof(ut));
	ut.ut_tv = *now;
	(void)strncpy(ut.ut_name, username, sizeof(ut.ut_name));
	if (hostname)
		(void)strncpy(ut.ut_host, hostname, sizeof(ut.ut_host));
	(void)strncpy(ut.ut_line, tty, sizeof(ut.ut_line));
	ut.ut_type = type;
	ut.ut_pid = pid;
	t = tty + strlen(tty);
	if ((size_t)(t - tty) >= sizeof(ut.ut_id))
	    tty = t - sizeof(ut.ut_id);
	(void)strncpy(ut.ut_id, tty, sizeof(ut.ut_id));
	(void)pututxline(&ut);
	endutxent();
}
static void
login_utmpx(const char *username, const char *hostname, const char *tty,
    const struct timeval *now)
{
	doutmpx(username, hostname, tty, now, USER_PROCESS, getpid());
}

static void
logout_utmpx(const char *tty, const struct timeval *now)
{
	doutmpx("", "", tty, now, DEAD_PROCESS, 0);
}
#endif

#ifdef SUPPORT_UTMP
static void
login_utmp(const char *username, const char *hostname, const char *tty,
    const struct timeval *now)
{
	struct utmp ut;
	(void)memset(&ut, 0, sizeof(ut));
	ut.ut_time = now->tv_sec;
	(void)strncpy(ut.ut_name, username, sizeof(ut.ut_name));
	if (hostname)
		(void)strncpy(ut.ut_host, hostname, sizeof(ut.ut_host));
	(void)strncpy(ut.ut_line, tty, sizeof(ut.ut_line));
	login(&ut);
}

static void
logout_utmp(const char *tty, const struct timeval *now __unused)
{
	logout(tty);
}
#endif

static int
utmp_create(int fd, const char *host)
{
	struct timeval tv;
	char username[LOGIN_NAME_MAX];
	char tty[128], *ttyp;

	if (getlogin_r(username, sizeof(username)) == -1)
		return -1;

	if ((errno = ttyname_r(fd, tty, sizeof(tty))) != 0)
		return -1;
	ttyp = tty + sizeof(_PATH_DEV) - 1;

	(void)gettimeofday(&tv, NULL);
#ifdef SUPPORT_UTMPX
	login_utmpx(username, host, ttyp, &tv);
#endif
#ifdef SUPPORT_UTMP
	login_utmp(username, host, ttyp, &tv);
#endif
	return 0;
}

static int
utmp_destroy(int fd)
{
	struct timeval tv;
	char tty[128], *ttyp;

	if ((errno = ttyname_r(fd, tty, sizeof(tty))) != 0)
		return -1;
	ttyp = tty + sizeof(_PATH_DEV) - 1;
	(void)gettimeofday(&tv, NULL);
#ifdef SUPPORT_UTMPX
	logout_utmpx(ttyp, &tv);
#endif
#ifdef SUPPORT_UTMP
	logout_utmp(ttyp, &tv);
#endif
	return 0;
}

#include "utempter.h"

static int last_fd = -1;

int
utempter_add_record(int fd, const char *host)
{
	if (utmp_create(fd, host) == -1)
		return -1;
	last_fd = fd;
	return 0;
}

int
utempter_remove_added_record(void)
{

	if (last_fd < 0)
		return -1;
	utmp_destroy(last_fd);
	last_fd = -1;
	return 0;
}

int
utempter_remove_record(int fd)
{

	utmp_destroy(fd);
	if (last_fd == fd)
		last_fd = -1;
	return 0;
}

void
addToUtmp(const char *pty __unused, const char *host, int fd)
{

	utempter_add_record(fd, host);
}

void
removeFromUtmp(void)
{

	utempter_remove_added_record();
}

void
removeLineFromUtmp(const char *pty __unused, int fd)
{

	utempter_remove_record(fd);
}
