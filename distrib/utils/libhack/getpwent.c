/*	$NetBSD: getpwent.c,v 1.5 2002/02/02 15:57:54 lukem Exp $	*/

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

/*
 * Copied from:  lib/libc/gen/getpwent.c
 *	NetBSD: getpwent.c,v 1.48 2000/10/03 03:22:26 enami Exp
 * and then gutted, leaving only /etc/master.passwd support.
 */

#include <sys/cdefs.h>

#ifdef __weak_alias
#define endpwent		_endpwent
#define getpwent		_getpwent
#define getpwuid		_getpwuid
#define getpwnam		_getpwnam
#define setpwent		_setpwent
#define setpassent		_setpassent

__weak_alias(endpwent,_endpwent)
__weak_alias(getpwent,_getpwent)
__weak_alias(getpwuid,_getpwuid)
__weak_alias(getpwnam,_getpwnam)
__weak_alias(setpwent,_setpwent)
__weak_alias(setpassent,_setpassent)
#endif

#include <sys/param.h>

#include <limits.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static	int		pwstart(void);
static	int		pwscan(int, uid_t, const char *);
static	int		pwmatchline(int, uid_t, const char *);

static	FILE		*_pw_fp;
static	struct passwd	_pw_passwd;	/* password structure */
static	int		_pw_stayopen;	/* keep fd's open */
static	int		_pw_filesdone;

#define	MAXLINELENGTH	1024

static	char		pwline[MAXLINELENGTH];

struct passwd *
getpwent(void)
{

	if ((!_pw_fp && !pwstart()) || !pwscan(0, 0, NULL))
		return (NULL);
	return (&_pw_passwd);
}

struct passwd *
getpwnam(const char *name)
{
	int rval;

	if (!pwstart())
		return NULL;
	rval = pwscan(1, 0, name);
	if (!_pw_stayopen)
		endpwent();
	return (rval) ? &_pw_passwd : NULL;
}

struct passwd *
getpwuid(uid_t uid)
{
	int rval;

	if (!pwstart())
		return NULL;
	rval = pwscan(1, uid, NULL);
	if (!_pw_stayopen)
		endpwent();
	return (rval) ? &_pw_passwd : NULL;
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
pwscan(int search, uid_t uid, const char *name)
{

	if (_pw_filesdone)
		return 0;
	for (;;) {
		if (!fgets(pwline, sizeof(pwline), _pw_fp)) {
			if (!search)
				_pw_filesdone = 1;
			return 0;
		}
		/* skip lines that are too big */
		if (!strchr(pwline, '\n')) {
			int ch;

			while ((ch = getc(_pw_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		if (pwmatchline(search, uid, name))
			return 1;
	}
	/* NOTREACHED */
}

static int
pwmatchline(int search, uid_t uid, const char *name)
{
	unsigned long	id;
	char		*cp, *bp, *ep;

	/* name may be NULL if search is nonzero */

	bp = pwline;
	memset(&_pw_passwd, 0, sizeof(_pw_passwd));
	_pw_passwd.pw_name = strsep(&bp, ":\n");		/* name */
	if (search && name && strcmp(_pw_passwd.pw_name, name))
		return 0;

	_pw_passwd.pw_passwd = strsep(&bp, ":\n");		/* passwd */

	if (!(cp = strsep(&bp, ":\n")))				/* uid */
		return 0;
	id = strtoul(cp, &ep, 10);
	if (id > UID_MAX || *ep != '\0')
		return 0;
	_pw_passwd.pw_uid = (uid_t)id;
	if (search && name == NULL && _pw_passwd.pw_uid != uid)
		return 0;

	if (!(cp = strsep(&bp, ":\n")))				/* gid */
		return 0;
	id = strtoul(cp, &ep, 10);
	if (id > GID_MAX || *ep != '\0')
		return 0;
	_pw_passwd.pw_gid = (gid_t)id;

	if (!(ep = strsep(&bp, ":")))				/* class */
		return 0;
	if (!(ep = strsep(&bp, ":")))				/* change */
		return 0;
	if (!(ep = strsep(&bp, ":")))				/* expire */
		return 0;

	if (!(_pw_passwd.pw_gecos = strsep(&bp, ":\n")))	/* gecos */
		return 0;
	if (!(_pw_passwd.pw_dir = strsep(&bp, ":\n")))		/* directory */
		return 0;
	if (!(_pw_passwd.pw_shell = strsep(&bp, ":\n")))	/* shell */
		return 0;

	if (strchr(bp, ':') != NULL)
		return 0;

	return 1;
}
