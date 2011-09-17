/*	$NetBSD: utmp_update.c,v 1.10 2011/09/17 01:50:54 christos Exp $	 */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

__RCSID("$NetBSD: utmp_update.c,v 1.10 2011/09/17 01:50:54 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#include <vis.h>
#include <err.h>
#include <fcntl.h>
#include <pwd.h>
#include <utmpx.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>

static __dead void
logerr(int e, const char *fmt, ...)
{
	va_list sap, eap;
	char *s = NULL;
	
	if (e)
		(void)asprintf(&s, "%s (%s)", fmt, strerror(e));
	if (s)
		fmt = s;

	va_start(sap, fmt);
	va_copy(eap, sap);
	vsyslog(LOG_ERR, fmt, sap);
	va_end(sap);
	errx(1, fmt, eap);
	va_end(eap);
}

int
main(int argc, char *argv[])
{
	struct utmpx *utx;
	size_t len;
	struct passwd *pwd;
	struct stat st;
	int fd;
	int res;
	uid_t euid, ruid;
	char tty[MAXPATHLEN];

	euid = geteuid();
	ruid = getuid();

	if (argc != 2) {
		(void)fprintf(stderr, "Usage: %s <vis-utmpx-entry>\n",
		    getprogname());
		return 1;
	}

	openlog(getprogname(), LOG_PID | LOG_NDELAY, LOG_AUTH);
	if (seteuid(ruid) == -1)
		logerr(errno, "Can't setuid %ld", (long)ruid);

	len = strlen(argv[1]);

	if (len > sizeof(*utx) * 4 + 1 || len < sizeof(*utx))
		logerr(0, "Bad argument size %zu", len);

	if ((utx = malloc(len)) == NULL)
		logerr(errno, "Can't allocate %zu", len);

	res = strunvis((char *)utx, argv[1]);
	if (res != (int)sizeof(*utx))
		logerr(0, "Decoding error %s %d != %zu", argv[1], res,
		    sizeof(*utx));

	switch (utx->ut_type) {
	case USER_PROCESS:
	case DEAD_PROCESS:
		break;
	default:
		logerr(0, "Invalid utmpx type %d", (int)utx->ut_type);
	}

	if (ruid != 0) {
		if ((pwd = getpwuid(ruid)) == NULL)
			logerr(0, "User %ld does not exist in password"
			    " database", (long)ruid);

		if (strcmp(pwd->pw_name, utx->ut_name) != 0)
			logerr(0, "Current user `%s' does not match "
			    "`%s' in utmpx entry", pwd->pw_name, utx->ut_name);
	}

	(void)snprintf(tty, sizeof(tty), "%s%s", _PATH_DEV, utx->ut_line);
	fd = open(tty, O_RDONLY|O_NONBLOCK, 0);
	if (fd != -1) {
		if (fstat(fd, &st) == -1)
			logerr(errno, "Cannot stat `%s'", tty);
		if (ruid != 0 && st.st_uid != ruid)
			logerr(0, "%s: Is not owned by you", tty);
		if (!isatty(fd))
			logerr(0, "%s: Not a tty device", tty);
		(void)close(fd);
		if (access(tty, W_OK|R_OK) == -1)
			logerr(errno, "Can't access `%s'", tty);
	} else {
		struct utmpx utold, *utoldp;
		pid_t ppid;

		/*
		 * A daemon like ftpd that does not use a tty line? 
		 * We only allow it to kill its own existing entries 
		 */
		if (utx->ut_type != DEAD_PROCESS)
			logerr(errno, "Cannot open `%s'", tty);

		(void)memcpy(utold.ut_line, utx->ut_line, sizeof(utx->ut_line));
		if ((utoldp = getutxline(&utold)) == NULL)
			logerr(0, "Cannot find existing entry for `%s'",
			    utx->ut_line);
		if (utoldp->ut_pid != (ppid = getppid()))
			logerr(0, "Cannot modify entry for `%s' "
			    "utmp pid %ld != parent %ld", tty,
			    (long)utoldp->ut_pid, (long)ppid);
	}

	if (seteuid(euid) == 1)
		logerr(errno, "Can't setuid %ld", (long)euid);
	if (pututxline(utx) == NULL)
		logerr(errno, "Cannot update utmp entry");

	return 0;
}
