/*	$NetBSD: utmp_update.c,v 1.6 2003/02/26 18:16:50 christos Exp $	 */

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

__RCSID("$NetBSD: utmp_update.c,v 1.6 2003/02/26 18:16:50 christos Exp $");

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

int main(int, char *[]);

int
main(int argc, char *argv[])
{
	struct utmpx *utx;
	size_t len;
	struct passwd *pwd;
	struct stat st;
	int fd;
	uid_t euid, ruid;
	char tty[MAXPATHLEN];

	euid = geteuid();
	ruid = getuid();
	if (seteuid(ruid) == -1)
		err(1, "seteuid");

	if (argc != 2) {
		(void)fprintf(stderr, "Usage: %s <vis-utmpx-entry>\n",
			getprogname());
		exit(1);
	}

	len = strlen(argv[1]);

	if (len > sizeof(*utx) * 4 + 1 || len < sizeof(*utx))
		errx(1, "Bad argument");

	if ((utx = malloc(len)) == NULL)
		err(1, NULL);

	if (strunvis((char *)utx, argv[1]) != sizeof(*utx))
		errx(1, "Decoding error");

	switch (utx->ut_type) {
	case USER_PROCESS:
	case DEAD_PROCESS:
		break;
	default:
		errx(1, "Invalid utmpx type %d", (int)utx->ut_type);
	}

	if (ruid != 0) {
		if ((pwd = getpwuid(ruid)) == NULL)
			errx(1, "User %lu does not exist in password database",
			    (long)ruid);

		if (strcmp(pwd->pw_name, utx->ut_name) != 0)
			errx(1, "Current user `%s' does not match "
			    "`%s' in utmpx entry", pwd->pw_name, utx->ut_name);
	}

	(void)snprintf(tty, sizeof(tty), "%s%s", _PATH_DEV, utx->ut_line);
	fd = open(tty, O_RDONLY, 0);
	if (fd != -1) {
		if (fstat(fd, &st) == -1)
			err(1, "Cannot stat `%s'", tty);
		if (ruid != 0 && st.st_uid != ruid)
			errx(1, "%s: Is not owned by you", tty);
		if (!isatty(fd))
			errx(1, "%s: Not a tty device", tty);
		(void)close(fd);
		if (access(tty, W_OK|R_OK) == -1)
			err(1, "%s", tty);
	} else {
		struct utmpx utold, *utoldp;
		/*
		 * A daemon like ftpd that does not use a tty line? 
		 * We only allow it to kill its own existing entries 
		 */
		if (utx->ut_type != DEAD_PROCESS)
			err(1, "Cannot open `%s'", tty);

		(void)memcpy(utold.ut_line, utx->ut_line, sizeof(utx->ut_line));
		if ((utoldp = getutxline(&utold)) == NULL)
			err(1, "Cannot find existing entry for `%s'",
			    utx->ut_line);
		if (utoldp->ut_pid != getppid())
			err(1, "Cannot modify entry for `%s'", tty);
	}

	(void)seteuid(euid);
	if (pututxline(utx) == NULL)
		err(1, "Cannot update utmp entry");

	return 0;
}
