/*	$NetBSD: utmp_update.c,v 1.2 2002/08/05 15:36:51 jmcneill Exp $	 */

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

#include <sys/stat.h>

#include <stdio.h>
#include <vis.h>
#include <err.h>
#include <pwd.h>
#include <utmpx.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int, char *[]);

int
main(int argc, char *argv[])
{
	struct utmpx *utx;
	size_t len;
	struct passwd *pwd;
	struct stat st;

	if (argc != 1) {
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
		errx(1, "Invalid utmpx type %d\n", (int)utx->ut_type);
	}

	if ((pwd = getpwuid(getuid())) == NULL)
		errx(1, "User %lu does not exist in password database",
		    (long)getuid());

	if (strcmp(pwd->pw_name, utx->ut_name) != 0)
		errx(1, "Current user `%s' does not match `%s' in utmpx entry",
		    pwd->pw_name, utx->ut_name);

	if (stat(utx->ut_line, &st) == -1)
		err(1, "Cannot stat `%s'", utx->ut_line);

	if (!S_ISCHR(st.st_mode))
		errx(1, "%s: Not a character device", utx->ut_line);

	/*
	 * to check if the tty is writable here is problematic. First we
	 * don't even know if it is a tty, secondly we are setuid so it
	 * is not trivial to use access
	 */

	pututxline(utx);

	return 0;
}
