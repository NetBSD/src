/*	$NetBSD: getgrent.c,v 1.28 1999/01/16 14:44:33 lukem Exp $	*/

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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getgrent.c	8.2 (Berkeley) 3/21/94";
#else
__RCSID("$NetBSD: getgrent.c,v 1.28 1999/01/16 14:44:33 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <grp.h>
#include <limits.h>
#include <nsswitch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#ifdef HESIOD
#include <hesiod.h>
#endif
#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#ifdef __weak_alias
__weak_alias(endgrent,_endgrent);
__weak_alias(getgrent,_getgrent);
__weak_alias(getgrgid,_getgrgid);
__weak_alias(getgrnam,_getgrnam);
__weak_alias(setgrent,_setgrent);
__weak_alias(setgroupent,_setgroupent);
#endif

static FILE		*_gr_fp;
static struct group	_gr_group;
static int		_gr_stayopen;
static int		_gr_nomore;

static int grscan	__P((int, gid_t, const char *));
static int matchline	__P((int, gid_t, const char *));
static int start_gr	__P((void));

#define	MAXGRP		200
#define	MAXLINELENGTH	1024

static __aconst char	*members[MAXGRP];
static char		line[MAXLINELENGTH];

#ifdef YP
enum _grmode { GRMODE_NONE, GRMODE_FULL, GRMODE_NAME };
static enum _grmode	 __grmode;
static char		*__ypcurrent, *__ypdomain;
static int		 __ypcurrentlen;
#endif

#ifdef HESIOD
static int	__gr_hesnum;
#endif

struct group *
getgrent()
{
	_gr_nomore = 0;
	if ((!_gr_fp && !start_gr()) || !grscan(0, 0, NULL) || _gr_nomore)
 		return(NULL);
	return &_gr_group;
}

struct group *
getgrnam(name)
	const char *name;
{
	int rval;

	if (!start_gr())
		return NULL;
	rval = grscan(1, 0, name);
	if (!_gr_stayopen)
		endgrent();
	return (rval) ? &_gr_group : NULL;
}

struct group *
getgrgid(gid)
	gid_t gid;
{
	int rval;

	if (!start_gr())
		return NULL;
	rval = grscan(1, gid, NULL);
	if (!_gr_stayopen)
		endgrent();
	return (rval) ? &_gr_group : NULL;
}

static int
start_gr()
{
#ifdef YP
	__grmode = GRMODE_NONE;
	if (__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
#endif
#ifdef HESIOD
	__gr_hesnum = 0;
#endif
	if (_gr_fp) {
		rewind(_gr_fp);
		return 1;
	}
	return (_gr_fp = fopen(_PATH_GROUP, "r")) ? 1 : 0;
}

void
setgrent()
{
	(void) setgroupent(0);
}

int
setgroupent(stayopen)
	int stayopen;
{
	if (!start_gr())
		return 0;
	_gr_stayopen = stayopen;
	return 1;
}

void
endgrent()
{
#ifdef YP
	__grmode = GRMODE_NONE;
	if (__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
#endif
#ifdef HESIOD
	__gr_hesnum = 0;
#endif
	if (_gr_fp) {
		(void)fclose(_gr_fp);
		_gr_fp = NULL;
	}
}


static int _local_grscan __P((void *, void *, va_list));

static int
_local_grscan(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

	for (;;) {
		if (!fgets(line, sizeof(line), _gr_fp)) {
			if (!search) {
				_gr_nomore = 1;
				return NS_SUCCESS;
			}
			return NS_NOTFOUND;
		}
		/* skip lines that are too big */
		if (!strchr(line, '\n')) {
			int ch;

			while ((ch = getc(_gr_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		if (matchline(search, gid, name))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
}

#ifdef HESIOD
static int _dns_grscan __P((void *, void *, va_list));

static int
_dns_grscan(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

	char		**hp;

	for (;;) {
		if (search) {
			if (name)
				strncpy(line, name, sizeof(line));
			else
				snprintf(line, sizeof(line), "%u",
				    (unsigned int)gid);
		} else {
			snprintf(line, sizeof(line), "group-%u", __gr_hesnum);
			__gr_hesnum++;
		}

		line[sizeof(line) - 1] = '\0';
		hp = hes_resolve(line, "group");
		if (hp == NULL) {
			switch (hes_error()) {
			case HES_ER_NOTFOUND:
				if (!search) {
					__gr_hesnum = 0;
					_gr_nomore = 1;
					return NS_SUCCESS;
				}
				return NS_NOTFOUND;
			case HES_ER_OK:
				abort();
			default:
				return NS_UNAVAIL;
			}
		}

						/* only check first elem */
		strncpy(line, hp[0], sizeof(line));
		line[sizeof(line) - 1] = '\0';
		hes_free(hp);
		if (matchline(search, gid, name))
			return NS_SUCCESS;
		else if (search)
			return NS_NOTFOUND;
	}
}
#endif

#ifdef YP
static int _nis_grscan __P((void *, void *, va_list));

static int
_nis_grscan(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

	char	*key, *data;
	int	 keylen, datalen;
	int	 r;

	if(__ypdomain == NULL) {
		switch (yp_get_default_domain(&__ypdomain)) {
		case 0:
			break;
		case YPERR_RESRC:
			return NS_TRYAGAIN;
		default:
			return NS_UNAVAIL;
		}
	}

	if (search) {			/* specific group or gid */
		if (name)
			strncpy(line, name, sizeof(line));
		else
			snprintf(line, sizeof(line), "%u", (unsigned int)gid);
		line[sizeof(line) - 1] = '\0';
		data = NULL;
		r = yp_match(__ypdomain,
				(name) ? "group.byname" : "group.bygid",
				line, (int)strlen(line), &data, &datalen);
		switch (r) {
		case 0:
			break;
		case YPERR_KEY:
			if (data)
				free(data);
			return NS_NOTFOUND;
		default:
			if (data)
				free(data);
			return NS_UNAVAIL;
		}
		data[datalen] = '\0';			/* clear trailing \n */
		strncpy(line, data, sizeof(line));
		line[sizeof(line) - 1] = '\0';
		free(data);
		if (matchline(search, gid, name))
			return NS_SUCCESS;
		else
			return NS_NOTFOUND;
	}

	for (;;) {			/* ! search */
		data = NULL;
		if(__ypcurrent) {
			key = NULL;
			r = yp_next(__ypdomain, "group.byname",
				__ypcurrent, __ypcurrentlen,
				&key, &keylen, &data, &datalen);
			free(__ypcurrent);
			switch (r) {
			case 0:
				break;
			case YPERR_NOMORE:
				__ypcurrent = NULL;
				if (key)
					free(key);
				if (data)
					free(data);
				_gr_nomore = 1;
				return NS_SUCCESS;
			default:
				if (key)
					free(key);
				if (data)
					free(data);
				return NS_UNAVAIL;
			}
			__ypcurrent = key;
			__ypcurrentlen = keylen;
		} else {
			if (yp_first(__ypdomain, "group.byname",
					&__ypcurrent, &__ypcurrentlen,
					&data, &datalen)) {
				if (data);
					free(data);
				return NS_UNAVAIL;
			}
		}
		data[datalen] = '\0';			/* clear trailing \n */
		strncpy(line, data, sizeof(line));
		line[sizeof(line) - 1] = '\0';
		free(data);
		if (matchline(search, gid, name))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
}
#endif

#if defined(YP) || defined(HESIOD)
/*
 * log an error if "files" or "compat" is specified in group_compat database
 */
static int _bad_grscan __P((void *, void *, va_list));

static int
_bad_grscan(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	static int warned;

	if (!warned) {
		syslog(LOG_ERR,
			"nsswitch.conf group_compat database can't use '%s'",
			(char *)cb_data);
	}
	warned = 1;
	return NS_UNAVAIL;
}

/*
 * when a name lookup in compat mode is required, look it up in group_compat
 * nsswitch database. only Hesiod and NIS is supported - it doesn't make
 * sense to lookup compat names from 'files' or 'compat'
 */

static int __grscancompat __P((int, gid_t, const char *));

static int
__grscancompat(search, gid, name)
	int		 search;
	gid_t		 gid;
	const char	*name;
{
	static ns_dtab	dtab[] = {
		NS_FILES_CB(_bad_grscan, "files"),
		NS_DNS_CB(_dns_grscan, NULL),
		NS_NIS_CB(_nis_grscan, NULL),
		NS_COMPAT_CB(_bad_grscan, "compat"),
		{ 0 }
	};

	return nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, search, gid, name);
}


static int _compat_grscan __P((void *, void *, va_list));

static int
_compat_grscan(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

	static char	*grname = NULL;

	for (;;) {
		if(__grmode != GRMODE_NONE) {
			int	 r;

			switch(__grmode) {
			case GRMODE_FULL:
				r = __grscancompat(search, gid, name);
				if (r == NS_SUCCESS)
					return r;
				__grmode = GRMODE_NONE;
				break;
			case GRMODE_NAME:
				if(grname == (char *)NULL) {
					__grmode = GRMODE_NONE;
					break;
				}
				r = __grscancompat(1, 0, grname);
				free(grname);
				grname = (char *)NULL;
				if (r != NS_SUCCESS)
					break;
				if (!search)
					return NS_SUCCESS;
				if (name) {
					if (! strcmp(_gr_group.gr_name, name))
						return NS_SUCCESS;
				} else {
					if (_gr_group.gr_gid == gid)
						return NS_SUCCESS;
				}
				break;
			case GRMODE_NONE:
				abort();
			}
			continue;
		}

		if (!fgets(line, sizeof(line), _gr_fp))
			return NS_NOTFOUND;
		/* skip lines that are too big */
		if (!strchr(line, '\n')) {
			int ch;

			while ((ch = getc(_gr_fp)) != '\n' && ch != EOF)
				;
			continue;
		}

		if (line[0] == '+') {
			char	*tptr, *bp;

			switch(line[1]) {
			case ':':
			case '\0':
			case '\n':
				__grmode = GRMODE_FULL;
				break;
			default:
				__grmode = GRMODE_NAME;
				bp = line;
				tptr = strsep(&bp, ":\n");
				grname = strdup(tptr + 1);
				break;
			}
			continue;
		}
		if (matchline(search, gid, name))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
}
#endif /* YP || HESIOD */

static int
grscan(search, gid, name)
	int		 search;
	gid_t		 gid;
	const char	*name;
{
	int		r;
	static ns_dtab	dtab[] = {
		NS_FILES_CB(_local_grscan, NULL),
		NS_DNS_CB(_dns_grscan, NULL),
		NS_NIS_CB(_nis_grscan, NULL),
		NS_COMPAT_CB(_compat_grscan, NULL),
		{ 0 }
	};

	r = nsdispatch(NULL, dtab, NSDB_GROUP, search, gid, name);
	return (r == NS_SUCCESS) ? 1 : 0;
}

static int
matchline(search, gid, name)
	int		 search;
	gid_t		 gid;
	const char	*name;
{
	unsigned long	id;
	__aconst char	**m;
	char		*cp, *bp, *ep;

	if (line[0] == '+')
		return 0;	/* sanity check to prevent recursion */
	bp = line;
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
