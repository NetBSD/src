/*	$NetBSD: tilde-luzah-bozo.c,v 1.4.2.1 2009/05/13 19:18:38 jym Exp $	*/

/*	$eterna: tilde-luzah-bozo.c,v 1.10 2009/04/18 05:36:04 mrg Exp $	*/

/*
 * Copyright (c) 1997-2009 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer and
 *    dedication in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* this code implements ~user support for bozohttpd */

#ifndef NO_USER_SUPPORT

#include <sys/param.h>

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bozohttpd.h"

#ifndef PUBLIC_HTML
#define PUBLIC_HTML		"public_html"
#endif

	int	uflag;		/* allow /~user/ translation */
	const char *public_html	= PUBLIC_HTML;

/*
 * user_transform does this:
 *	- chdir's /~user/public_html
 *	- returns the rest of the file, index.html appended if required
 *	- returned malloced file to serve in request->hr_file,
 *        ala transform_request().
 *
 * transform_request() is supposed to check that we have user support
 * enabled.
 */
int
user_transform(request, isindex)
	http_req *request;
	int *isindex;
{
	char	c, *s, *file = NULL;
	struct	passwd *pw;

	*isindex = 0;

	if ((s = strchr(request->hr_file + 2, '/')) != NULL) {
		*s++ = '\0';
		c = s[strlen(s)-1];
		*isindex = (c == '/' || c == '\0');
	}

	debug((DEBUG_OBESE, "looking for user %s", request->hr_file + 2));
	pw = getpwnam(request->hr_file + 2);
	/* fix this up immediately */
	if (s)
		s[-1] = '/';
	if (pw == NULL) {
		(void)http_error(404, request, "no such user");
		return 0;
	}

	debug((DEBUG_OBESE, "user %s home dir %s uid %d gid %d", pw->pw_name,
	    pw->pw_dir, pw->pw_uid, pw->pw_gid));

	if (chdir(pw->pw_dir) < 0) {
		warning("chdir1 error: %s: %s", pw->pw_dir, strerror(errno));
		(void)http_error(403, request, "can't chdir to homedir");
		return 0;
	}
	if (chdir(public_html) < 0) {
		warning("chdir2 error: %s: %s", public_html, strerror(errno));
		(void)http_error(403, request, "can't chdir to public_html");
		return 0;
	}
	if (s == NULL || *s == '\0') {
		file = bozostrdup(index_html);
	} else {
		file = bozomalloc(strlen(s) +
		    (*isindex ? strlen(index_html) + 1 : 1));
		strcpy(file, s);
		if (*isindex)
			strcat(file, index_html);
	}

	/* see transform_request() */
	if (*file == '/' || strcmp(file, "..") == 0 ||
	    strstr(file, "/..") || strstr(file, "../")) {
		(void)http_error(403, request, "illegal request");
		free(file);
		return 0;
	}

	if (auth_check(request, file)) {
		free(file);
		return 0;
	}

	free(request->hr_file);
	request->hr_file = file;

	debug((DEBUG_FAT, "transform_user returning %s under %s", file,
	    pw->pw_dir));
	return 1;
}
#endif /* NO_USER_SUPPORT */
