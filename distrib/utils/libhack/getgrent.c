/*	$NetBSD: getgrent.c,v 1.5 2002/02/02 15:31:58 lukem Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Portions Copyright (c) 1994, Jason Downs. All Rights Reserved.
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
 * Copied from:  lib/libc/gen/getgrent.c
 *	NetBSD: getgrent.c,v 1.42 2002/02/02 15:21:29 lukem Exp
 * and then gutted, leaving only /etc/group support.
 */

#include <sys/cdefs.h>

#ifdef __weak_alias
#define endgrent		_endgrent
#define getgrent		_getgrent
#define getgrgid		_getgrgid
#define getgrnam		_getgrnam
#define setgrent		_setgrent
#define setgroupent		_setgroupent

__weak_alias(endgrent,_endgrent)
__weak_alias(getgrent,_getgrent)
__weak_alias(getgrgid,_getgrgid)
__weak_alias(getgrnam,_getgrnam)
__weak_alias(setgrent,_setgrent)
__weak_alias(setgroupent,_setgroupent)
#endif

#include <sys/types.h>

#include <grp.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE		*_gr_fp;
static struct group	_gr_group;
static int		_gr_filesdone;
static int		_gr_stayopen;

static int grscan(int, gid_t, const char *);
static int grstart(void);
static int grmatchline(int, gid_t, const char *);

#define	MAXGRP		200
#define	MAXLINELENGTH	1024
static char		*members[MAXGRP];
static char		grline[MAXLINELENGTH];

struct group *
getgrent(void)
{
	if ((!_gr_fp && !grstart()) || !grscan(0, 0, NULL))
		return (NULL);
	return (&_gr_group);
}

struct group *
getgrnam(const char *name)
{
	int rval;

	if (!grstart())
		return(NULL);
	rval = grscan(1, 0, name);
	if (!_gr_stayopen)
		endgrent();
	return(rval ? &_gr_group : NULL);
}

struct group *
getgrgid(gid_t gid)
{
	int rval;

	if (!grstart())
		return(NULL);
	rval = grscan(1, gid, NULL);
	if (!_gr_stayopen)
		endgrent();
	return(rval ? &_gr_group : NULL);
}

static int
grstart(void)
{
	_gr_filesdone = 0;
	if (_gr_fp) {
		rewind(_gr_fp);
		return(1);
	}
	return((_gr_fp = fopen(_PATH_GROUP, "r")) ? 1 : 0);
}

void
setgrent(void)
{
	(void) setgroupent(0);
}

int
setgroupent(int stayopen)
{
	if (!grstart())
		return(0);
	_gr_stayopen = stayopen;
	return(1);
}

void
endgrent(void)
{
	_gr_filesdone = 0;
	if (_gr_fp) {
		(void)fclose(_gr_fp);
		_gr_fp = NULL;
	}
}

static int
grscan(int search, gid_t gid, const char *name)
{
	if (_gr_filesdone)
		return 0;
	for (;;) {
		if (!fgets(grline, sizeof(grline), _gr_fp)) {
			if (!search)
				_gr_filesdone = 1;
			return 0;
		}
		/* skip lines that are too big */
		if (!strchr(grline, '\n')) {
			int ch;

			while ((ch = getc(_gr_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		if (grmatchline(search, gid, name))
			return 1;
	}
	/* NOTREACHED */
}

static int
grmatchline(int search, gid_t gid, const char *name)
{
	unsigned long	id;
	char		**m;
	char		*cp, *bp, *ep;

	/* name may be NULL if search is nonzero */

	bp = grline;
	memset(&_gr_group, 0, sizeof(_gr_group));
	_gr_group.gr_name = strsep(&bp, ":\n");
	if (search && name && strcmp(_gr_group.gr_name, name))
		return 0;
	_gr_group.gr_passwd = strsep(&bp, ":\n");
	if (!(cp = strsep(&bp, ":\n")))
		return 0;
	id = strtoul(cp, &ep, 10);
	if (id > GID_MAX || *ep != '\0')
		return 0;
	_gr_group.gr_gid = (gid_t)id;
	if (search && name == NULL && _gr_group.gr_gid != gid)
		return 0;
	cp = NULL;
	if (bp == NULL)
		return 0;
	for (_gr_group.gr_mem = m = members;; bp++) {
		if (m == &members[MAXGRP - 1])
			break;
		if (*bp == ',') {
			if (cp) {
				*bp = '\0';
				*m++ = cp;
				cp = NULL;
			}
		} else if (*bp == '\0' || *bp == '\n' || *bp == ' ') {
			if (cp) {
				*bp = '\0';
				*m++ = cp;
			}
			break;
		} else if (cp == NULL)
			cp = bp;
	}
	*m = NULL;
	return 1;
}
