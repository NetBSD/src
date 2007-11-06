/*	$NetBSD: auth-bozo.c,v 1.3.2.2 2007/11/06 23:12:01 matt Exp $	*/

/*	$eterna: auth-bozo.c,v 1.7 2006/05/17 08:19:10 mrg Exp $	*/

/*
 * Copyright (c) 1997-2006 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/* this code implements "http basic authorisation" for bozohttpd */

#ifdef DO_HTPASSWD

#include <sys/param.h>

#include <string.h>
#include <unistd.h>

#include "bozohttpd.h"

#ifndef AUTH_FILE
#define AUTH_FILE		".htpasswd"
#endif

static	ssize_t	base64_decode(const unsigned char *, size_t,
			    unsigned char *, size_t);

/*
 * Check if HTTP authentication is required
 */
void
auth_check(http_req *request, const char *file)
{
	struct stat sb;
	char dir[MAXPATHLEN], authfile[MAXPATHLEN], *basename;
	char user[BUFSIZ], *pass;
	FILE *fp;
	int len;

			/* get dir=dirname(file) */
	snprintf(dir, sizeof(dir), "%s", file);
	if ((basename = strrchr(dir, '/')) == NULL)
		strcpy(dir, ".");
	else {
		*basename++ = '\0';
			/* ensure basename(file) != AUTH_FILE */
		check_special_files(request, basename);
	}
	request->hr_authrealm = dir;

	snprintf(authfile, sizeof(authfile), "%s/%s", dir, AUTH_FILE);
	if (stat(authfile, &sb) < 0) {
		debug((DEBUG_NORMAL,
		    "auth_check realm `%s' dir `%s' authfile `%s' missing",
		    dir, file, authfile));
		return;
	}
	if ((fp = fopen(authfile, "r")) == NULL)
		http_error(403, request, "no permission to open authfile");
	debug((DEBUG_NORMAL,
	    "auth_check realm `%s' dir `%s' authfile `%s' open",
	    dir, file, authfile));
	if (request->hr_authuser && request->hr_authpass) {
		while (fgets(user, sizeof(user), fp) != NULL) {
			len = strlen(user);
			if (len > 0 && user[len-1] == '\n')
				user[--len] = '\0';
			if ((pass = strchr(user, ':')) == NULL)
				continue;
			*pass++ = '\0';
			debug((DEBUG_NORMAL,
			    "auth_check authfile `%s':`%s' client `%s':`%s'",
			    user, pass, request->hr_authuser,
			    request->hr_authpass));
			if (strcmp(request->hr_authuser, user) != 0)
				continue;
			if (strcmp(crypt(request->hr_authpass, pass), pass))
				break;
			fclose(fp);
			return;
		}
	}
	fclose(fp);
	http_error(401, request, "bad auth");
}

int
auth_check_headers(http_req *request, char *val, char *str, ssize_t len)
{
	if (strcasecmp(val, "authorization") == 0 &&
	    strncasecmp(str, "Basic ", 6) == 0) {
		char	authbuf[BUFSIZ];
		char	*pass = NULL;
		ssize_t	alen;

		alen = base64_decode((unsigned char *)str + 6, len - 6,
		    (unsigned char *)authbuf, sizeof(authbuf) - 1);
		if (alen != -1)
			authbuf[alen] = '\0';
		if (alen == -1 ||
		    (pass = strchr(authbuf, ':')) == NULL)
			http_error(400, request,
			    "bad authorization field");
		*pass++ = '\0';
		request->hr_authuser = bozostrdup(authbuf);
		request->hr_authpass = bozostrdup(pass);
		debug((DEBUG_FAT,
		    "decoded authorization `%s' as `%s':`%s'",
		    str, request->hr_authuser, request->hr_authpass));
			/* don't store in request->headers */
		return 1;
	}
	return 0;
}

void
auth_check_special_files(http_req *request, const char *name)
{
	if (strcmp(name, AUTH_FILE) == 0)
		http_error(403, request, "no permission to open authfile");
}

void
auth_check_401(http_req *request, int code)
{
	if (code == 401)
		bozoprintf("WWW-Authenticate: Basic realm=\"%s\"\r\n",
		    request && request->hr_authrealm ? request->hr_authrealm :
		    "default realm");
}

#ifndef NO_CGIBIN_SUPPORT
void
auth_cgi_setenv(http_req *request, char ***curenvpp)
{
	if (request->hr_authuser && *request->hr_authuser) {
		spsetenv("AUTH_TYPE", "Basic", (*curenvpp)++);
		spsetenv("REMOTEUSER", request->hr_authuser, (*curenvpp)++);
	}
}

int
auth_cgi_count(http_req *request)
{
	return (request->hr_authuser && *request->hr_authuser) ? 2 : 0;
}
#endif /* NO_CGIBIN_SUPPORT */

/*
 * Decode len bytes starting at in using base64 encoding into out.
 * Result is *not* NUL terminated.
 * Written by Luke Mewburn <lukem@NetBSD.org>
 */
const unsigned char decodetable[] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63, 
	 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255,   0, 255, 255, 
	255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14, 
	 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255, 
	255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40, 
	 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255, 
};

static ssize_t
base64_decode(const unsigned char *in, size_t ilen, unsigned char *out,
	      size_t olen)
{
	unsigned char *cp;
	size_t	 i;

	cp = out;
	for (i = 0; i < ilen; i += 4) {
		if (cp + 3 > out + olen)
			return (-1);
#define IN_CHECK(x) \
		if ((x) > sizeof(decodetable) || decodetable[(x)] == 255) \
			    return(-1)

		IN_CHECK(in[i + 0]);
		*(cp++) = decodetable[in[i + 0]] << 2
			| decodetable[in[i + 1]] >> 4;
		IN_CHECK(in[i + 1]);
		*(cp++) = decodetable[in[i + 1]] << 4
			| decodetable[in[i + 2]] >> 2;
		IN_CHECK(in[i + 2]);
		*(cp++) = decodetable[in[i + 2]] << 6
			| decodetable[in[i + 3]];
#undef IN_CHECK
	}
	while (in[i - 1] == '=')
		cp--,i--;
	return (cp - out);
}
#endif /* DO_HTPASSWD */
