/*	$NetBSD: local_passwd.c,v 1.13 1997/07/24 08:53:48 phil Exp $	*/

/*-
 * Copyright (c) 1990, 1993, 1994
 * 	The Regents of the University of California.  All rights reserved.
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

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)local_passwd.c    8.3 (Berkeley) 4/2/94";
#else
static char rcsid[] = "$NetBSD: local_passwd.c,v 1.13 1997/07/24 08:53:48 phil Exp $";
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include "extern.h"

static uid_t uid;

char *tempname;

static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void
to64(s, v, n)
	char *s;
	long v;
	int n;
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}

char *
getnewpasswd(pw)
	struct passwd *pw;
{
	int tries;
	char *p, *t;
	char buf[_PASSWORD_LEN+1], salt[9], *crypt(), *getpass();

	(void)printf("Changing local password for %s.\n", pw->pw_name);

	if (uid && pw->pw_passwd[0] &&
	    strcmp(crypt(getpass("Old password:"), pw->pw_passwd),
	    pw->pw_passwd)) {
		errno = EACCES;
		pw_error(NULL, 1, 1);
	}

	for (buf[0] = '\0', tries = 0;;) {
		p = getpass("New password:");
		if (!*p) {
			(void)printf("Password unchanged.\n");
			pw_error(NULL, 0, 0);
		}
		if (strlen(p) <= 5 && ++tries < 2) {
			(void)printf("Please enter a longer password.\n");
			continue;
		}
		for (t = p; *t && islower(*t); ++t);
		if (!*t && ++tries < 2) {
			(void)printf("Please don't use an all-lower case "
				     "password.\nUnusual capitalization, "
				     "control characters or digits are "
				     "suggested.\n");
			continue;
		}
		(void)strncpy(buf, p, sizeof(buf) - 1);
		if (!strcmp(buf, getpass("Retype new password:")))
			break;
		(void)printf("Mismatch; try again, EOF to quit.\n");
	}
	/* grab a random printable character that isn't a colon */
	(void)srandom((int)time((time_t *)NULL));
#ifdef NEWSALT
	salt[0] = _PASSWORD_EFMT1;
	to64(&salt[1], (long)(29 * 25), 4);
	to64(&salt[5], random(), 4);
#else
	to64(&salt[0], random(), 2);
#endif
	return(crypt(buf, salt));
}

int
local_passwd(uname)
	char *uname;
{
	struct passwd *pw;
	struct passwd old_pw;
	int pfd, tfd;
	char *getnewpasswd();

	if (!(pw = getpwnam(uname))) {
		warnx("unknown user %s", uname);
		return (1);
	}

	uid = getuid();
	if (uid && uid != pw->pw_uid) {
		warnx("%s", strerror(EACCES));
		return (1);
	}

	/* Save the old pw information for comparing on pw_copy(). */
	old_pw = *pw;

	/*
	 * Get the new password.  Reset passwd change time to zero; when
	 * classes are implemented, go and get the "offset" value for this
	 * class and reset the timer.
	 */
	pw->pw_passwd = getnewpasswd(pw);
	pw->pw_change = 0;

	/* Now that the user has given us a new password, let us
	 * change the database.
	 */

	pw_init();
	tfd = pw_lock(0);
	if (tfd < 0) {
		warnx ("The passwd file is busy, waiting...");
		tfd = pw_lock(10);
		if (tfd < 0)
			errx(1, "The passwd file is still busy, "
			     "try again later.");
	}

	pfd = open(_PATH_MASTERPASSWD, O_RDONLY, 0);
	if (pfd < 0)
		pw_error(_PATH_MASTERPASSWD, 1, 1);

	pw_copy(pfd, tfd, pw, &old_pw);

	if (pw_mkdb() < 0)
		pw_error((char *)NULL, 0, 1);
	return (0);
}
