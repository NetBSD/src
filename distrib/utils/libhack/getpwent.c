/*	$NetBSD: getpwent.c,v 1.10 2008/11/28 19:39:00 sborrill Exp $	*/

/*
 * Copyright (c) 1987, 1988, 1989, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

/*
 * Copied from:  lib/libc/gen/getpwent.c
 *	NetBSD: getpwent.c,v 1.48 2000/10/03 03:22:26 enami Exp
 * and then gutted, leaving only /etc/master.passwd support.
 */

#include <sys/cdefs.h>

#ifdef __weak_alias
#define endpwent		_endpwent
#define getpwent		_getpwent
#define getpwent_r		_getpwent_r
#define getpwuid		_getpwuid
#define getpwnam		_getpwnam
#define setpwent		_setpwent
#define setpassent		_setpassent
#define getpwuid_r		_getpwuid_r
#define getpwnam_r		_getpwnam_r

__weak_alias(endpwent,_endpwent)
__weak_alias(getpwent,_getpwent)
__weak_alias(getpwent_r,_getpwent_r)
__weak_alias(getpwuid,_getpwuid)
__weak_alias(getpwnam,_getpwnam)
__weak_alias(setpwent,_setpwent)
__weak_alias(setpassent,_setpassent)
__weak_alias(getpwuid_r,_getpwuid_r)
__weak_alias(getpwnam_r,_getpwnam_r)
#endif

#include <sys/param.h>

#include <limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static	int		pwstart(void);
static	int		pwscan(int, uid_t, const char *, struct passwd *,
    char *, size_t);
static	int		pwmatchline(int, uid_t, const char *, struct passwd *,
    char *);

static	FILE		*_pw_fp;
static	struct passwd	_pw_passwd;	/* password structure */
static	int		_pw_stayopen;	/* keep fd's open */
static	int		_pw_filesdone;

#define	MAXLINELENGTH	1024

static	char		pwline[MAXLINELENGTH];

struct passwd *
getpwent(void)
{

	if ((!_pw_fp && !pwstart()) ||
	    !pwscan(0, 0, NULL, &_pw_passwd, pwline, sizeof(pwline)))
		return (NULL);
	return (&_pw_passwd);
}

int
getpwent_r(struct passwd *pwres, char *buf, size_t bufsiz,
    struct passwd **pwd)
{
	int rval;

	if (!_pw_fp && !pwstart())
		return 1;
	rval = !pwscan(0, 0, NULL, pwres, buf, bufsiz);
	if (rval)
		*pwd = NULL;
	else
		*pwd = pwres;
	return rval;
}

struct passwd *
getpwnam(const char *name)
{
	struct passwd *pwd;
	return getpwnam_r(name, &_pw_passwd, pwline, sizeof(pwline),
	    &pwd) == 0 ? pwd : NULL;
}

int
getpwnam_r(const char *name, struct passwd *pwres, char *buf, size_t bufsiz,
    struct passwd **pwd)
{
	int rval;

	if (!pwstart())
		return 1;
	rval = !pwscan(1, 0, name, pwres, buf, bufsiz);
	if (!_pw_stayopen)
		endpwent();
	if (rval)
		*pwd = NULL;
	else
		*pwd = pwres;
	return rval;
}

struct passwd *
getpwuid(uid_t uid)
{
	struct passwd *pwd;
	return getpwuid_r(uid, &_pw_passwd, pwline, sizeof(pwline),
	    &pwd) == 0 ? pwd : NULL;
}

int
getpwuid_r(uid_t uid, struct passwd *pwres, char *buf, size_t bufsiz,
    struct passwd **pwd)
{
	int rval;

	if (!pwstart())
		return 1;
	rval = !pwscan(1, uid, NULL, pwres, buf, bufsiz);
	if (!_pw_stayopen)
		endpwent();
	if (rval)
		*pwd = NULL;
	else
		*pwd = pwres;
	return rval;
}

void
setpwent(void)
{

	(void) setpassent(0);
}

int
setpassent(int stayopen)
{

	if (!pwstart())
		return 0;
	_pw_stayopen = stayopen;
	return 1;
}

void
endpwent(void)
{

	_pw_filesdone = 0;
	if (_pw_fp) {
		(void)fclose(_pw_fp);
		_pw_fp = NULL;
	}
}

static int
pwstart(void)
{

	_pw_filesdone = 0;
	if (_pw_fp) {
		rewind(_pw_fp);
		return 1;
	}
	return (_pw_fp = fopen(_PATH_MASTERPASSWD, "r")) ? 1 : 0;
}


static int
pwscan(int search, uid_t uid, const char *name, struct passwd *pwd, char *buf,
    size_t bufsiz)
{

	if (_pw_filesdone)
		return 0;
	for (;;) {
		if (!fgets(buf, bufsiz, _pw_fp)) {
			if (!search)
				_pw_filesdone = 1;
			return 0;
		}
		/* skip lines that are too big */
		if (!strchr(buf, '\n')) {
			int ch;

			while ((ch = getc(_pw_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		if (pwmatchline(search, uid, name, pwd, buf))
			return 1;
	}
	/* NOTREACHED */
}

static int
pwmatchline(int search, uid_t uid, const char *name, struct passwd *pwd,
    char *buf)
{
	unsigned long	id;
	char		*cp, *bp, *ep;

	/* name may be NULL if search is nonzero */

	bp = buf;
	memset(pwd, 0, sizeof(*pwd));
	pwd->pw_name = strsep(&bp, ":\n");		/* name */
	if (search && name && strcmp(pwd->pw_name, name))
		return 0;

	pwd->pw_passwd = strsep(&bp, ":\n");		/* passwd */

	if (!(cp = strsep(&bp, ":\n")))				/* uid */
		return 0;
	id = strtoul(cp, &ep, 10);
	if (id > UID_MAX || *ep != '\0')
		return 0;
	pwd->pw_uid = (uid_t)id;
	if (search && name == NULL && pwd->pw_uid != uid)
		return 0;

	if (!(cp = strsep(&bp, ":\n")))				/* gid */
		return 0;
	id = strtoul(cp, &ep, 10);
	if (id > GID_MAX || *ep != '\0')
		return 0;
	pwd->pw_gid = (gid_t)id;

	if (!(pwd->pw_class = strsep(&bp, ":")))		/* class */
		return 0;
	if (!(ep = strsep(&bp, ":")))				/* change */
		return 0;
	if (!(ep = strsep(&bp, ":")))				/* expire */
		return 0;

	if (!(pwd->pw_gecos = strsep(&bp, ":\n")))		/* gecos */
		return 0;
	if (!(pwd->pw_dir = strsep(&bp, ":\n")))		/* directory */
		return 0;
	if (!(pwd->pw_shell = strsep(&bp, ":\n")))		/* shell */
		return 0;

	if (strchr(bp, ':') != NULL)
		return 0;

	return 1;
}
