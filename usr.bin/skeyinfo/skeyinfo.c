/*	$NetBSD: skeyinfo.c,v 1.6 2009/09/05 06:13:34 dholland Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
#ifndef lint
__RCSID("$NetBSD: skeyinfo.c,v 1.6 2009/09/05 06:13:34 dholland Exp $");
#endif

#include <stdio.h>
#include <pwd.h>
#include <err.h>
#include <errno.h>
#include <skey.h>
#include <string.h>
#include <unistd.h>

int main __P((int, char *[]));

int
main(argc, argv)
	int             argc;
	char           *argv[];
{
	struct skey     skey;
	char            name[100], prompt[1024];
	int             uid;
	struct passwd  *pw = NULL;

	argc--;
	argv++;

	if (geteuid())
		errx(1, "Must be root to read /etc/skeykeys");

	uid = getuid();

	if (!argc)
		pw = getpwuid(uid);
	else if (!uid)
		pw = getpwnam(argv[0]);
	else {
		errno = EPERM;
		err(1, "%s", argv[0]);
	}

	if (!pw) {
		if (argc)
			errx(1, "%s: no such user", argv[0]);
		else
			errx(1, "Who are you?");
	}

	(void) strlcpy(name, pw->pw_name, sizeof(name));

	if (getskeyprompt(&skey, name, prompt) == -1) {
		printf("%s %s no s/key\n",
		       argc ? name : "You",
		       argc ? "has" : "have");
	}
	else {
		if (argc)
			printf("%s's ", pw->pw_name);
		else
			printf("Your ");
		printf("next %s", prompt);
	}
	return 0;
}
