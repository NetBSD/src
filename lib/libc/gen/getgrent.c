/*	$NetBSD: getgrent.c,v 1.40.2.2 2002/03/08 21:35:05 nathanw Exp $	*/

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
__RCSID("$NetBSD: getgrent.c,v 1.40.2.2 2002/03/08 21:35:05 nathanw Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <nsswitch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <stdarg.h>

#ifdef HESIOD
#include <hesiod.h>
#endif
#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#if defined(YP) || defined(HESIOD)
#define _GROUP_COMPAT
#endif

#ifdef __weak_alias
__weak_alias(endgrent,_endgrent)
__weak_alias(getgrent,_getgrent)
__weak_alias(getgrgid,_getgrgid)
__weak_alias(getgrnam,_getgrnam)
__weak_alias(setgrent,_setgrent)
__weak_alias(setgroupent,_setgroupent)
#endif

static FILE		*_gr_fp;
static struct group	_gr_group;
static int		_gr_stayopen;
static int		_gr_filesdone;

static void grcleanup(void);
static int grscan(int, gid_t, const char *);
static int grstart(void);
static int grmatchline(int, gid_t, const char *);

#define	MAXGRP		200
#define	MAXLINELENGTH	1024

static __aconst char	*members[MAXGRP];
static char		line[MAXLINELENGTH];

#ifdef YP
static char	*__ypcurrent, *__ypdomain;
static int	 __ypcurrentlen;
static int	 _gr_ypdone;
#endif

#ifdef HESIOD
static int	_gr_hesnum;
#endif

#ifdef _GROUP_COMPAT
enum _grmode { GRMODE_NONE, GRMODE_FULL, GRMODE_NAME };
static enum _grmode	 __grmode;
#endif

struct group *
getgrent(void)
{

	if ((!_gr_fp && !grstart()) || !grscan(0, 0, NULL))
 		return (NULL);
	return &_gr_group;
}

struct group *
getgrnam(const char *name)
{
	int rval;

	_DIAGASSERT(name != NULL);

	if (!grstart())
		return NULL;
	rval = grscan(1, 0, name);
	if (!_gr_stayopen)
		endgrent();
	return (rval) ? &_gr_group : NULL;
}

struct group *
getgrgid(gid_t gid)
{
	int rval;

	if (!grstart())
		return NULL;
	rval = grscan(1, gid, NULL);
	if (!_gr_stayopen)
		endgrent();
	return (rval) ? &_gr_group : NULL;
}

void
grcleanup(void)
{

	_gr_filesdone = 0;
#ifdef YP
	if (__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
	_gr_ypdone = 0;
#endif
#ifdef HESIOD
	_gr_hesnum = 0;
#endif
#ifdef _GROUP_COMPAT
	__grmode = GRMODE_NONE;
#endif
}

static int
grstart(void)
{

	grcleanup();
	if (_gr_fp) {
		rewind(_gr_fp);
		return 1;
	}
	return (_gr_fp = fopen(_PATH_GROUP, "r")) ? 1 : 0;
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
		return 0;
	_gr_stayopen = stayopen;
	return 1;
}

void
endgrent(void)
{

	grcleanup();
	if (_gr_fp) {
		(void)fclose(_gr_fp);
		_gr_fp = NULL;
	}
}


static int _local_grscan(void *, void *, va_list);

/*ARGSUSED*/
static int
_local_grscan(void *rv, void *cb_data, va_list ap)
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

	if (_gr_filesdone)
		return NS_NOTFOUND;
	for (;;) {
		if (!fgets(line, sizeof(line), _gr_fp)) {
			if (!search)
				_gr_filesdone = 1;
			return NS_NOTFOUND;
		}
		/* skip lines that are too big */
		if (!strchr(line, '\n')) {
			int ch;

			while ((ch = getc(_gr_fp)) != '\n' && ch != EOF)
				;
			continue;
		}
		if (grmatchline(search, gid, name))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
}

#ifdef HESIOD
static int _dns_grscan(void *, void *, va_list);

/*ARGSUSED*/
static int
_dns_grscan(void *rv, void *cb_data, va_list ap)
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

	char		**hp;
	void		 *context;
	int		  r;

	r = NS_UNAVAIL;
	if (!search && _gr_hesnum == -1)
		return NS_NOTFOUND;
	if (hesiod_init(&context) == -1)
		return (r);

	for (;;) {
		if (search) {
			if (name)
				strncpy(line, name, sizeof(line));
			else
				snprintf(line, sizeof(line), "%u",
				    (unsigned int)gid);
		} else {
			snprintf(line, sizeof(line), "group-%u", _gr_hesnum);
			_gr_hesnum++;
		}

		line[sizeof(line) - 1] = '\0';
		hp = hesiod_resolve(context, line, "group");
		if (hp == NULL) {
			if (errno == ENOENT) {
				if (!search)
					_gr_hesnum = -1;
				r = NS_NOTFOUND;
			}
			break;
		}

						/* only check first elem */
		strncpy(line, hp[0], sizeof(line));
		line[sizeof(line) - 1] = '\0';
		hesiod_free_list(context, hp);
		if (grmatchline(search, gid, name)) {
			r = NS_SUCCESS;
			break;
		} else if (search) {
			r = NS_NOTFOUND;
			break;
		}
	}
	hesiod_end(context);
	return (r);
}
#endif	/* HESIOD */

#ifdef YP
static int _nis_grscan(void *, void *, va_list);

/*ARGSUSED*/
static int
_nis_grscan(void *rv, void *cb_data, va_list ap)
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
		if (grmatchline(search, gid, name))
			return NS_SUCCESS;
		else
			return NS_NOTFOUND;
	}

						/* ! search */
	if (_gr_ypdone)		
		return NS_NOTFOUND;
	for (;;) {
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
				_gr_ypdone = 1;
				return NS_NOTFOUND;
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
				if (data)
					free(data);
				return NS_UNAVAIL;
			}
		}
		data[datalen] = '\0';			/* clear trailing \n */
		strncpy(line, data, sizeof(line));
		line[sizeof(line) - 1] = '\0';
		free(data);
		if (grmatchline(search, gid, name))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
}
#endif	/* YP */

#ifdef _GROUP_COMPAT
/*
 * log an error if "files" or "compat" is specified in group_compat database
 */
static int _bad_grscan(void *, void *, va_list);

/*ARGSUSED*/
static int
_bad_grscan(void *rv, void *cb_data, va_list ap)
{
	static int warned;

	_DIAGASSERT(cb_data != NULL);

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

static int __grscancompat(int, gid_t, const char *);

static int
__grscancompat(int search, gid_t gid, const char *name)
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_bad_grscan, "files")
		NS_DNS_CB(_dns_grscan, NULL)
		NS_NIS_CB(_nis_grscan, NULL)
		NS_COMPAT_CB(_bad_grscan, "compat")
		{ 0 }
	};
	static const ns_src defaultnis[] = {
		{ NSSRC_NIS, 	NS_SUCCESS },
		{ 0 }
	};

	_DIAGASSERT(name != NULL);

	return (nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "grscancompat",
	    defaultnis, search, gid, name));
}
#endif	/* GROUP_COMPAT */


static int _compat_grscan(void *, void *, va_list);

/*ARGSUSED*/
static int
_compat_grscan(void *rv, void *cb_data, va_list ap)
{
	int		 search = va_arg(ap, int);
	gid_t		 gid = va_arg(ap, gid_t);
	const char	*name = va_arg(ap, const char *);

#ifdef _GROUP_COMPAT
	static char	*grname = NULL;
#endif

	for (;;) {
#ifdef _GROUP_COMPAT
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
#endif	/* _GROUP_COMPAT */

		if (!fgets(line, sizeof(line), _gr_fp))
			return NS_NOTFOUND;
		/* skip lines that are too big */
		if (!strchr(line, '\n')) {
			int ch;

			while ((ch = getc(_gr_fp)) != '\n' && ch != EOF)
				;
			continue;
		}

#ifdef _GROUP_COMPAT
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
#endif	/* _GROUP_COMPAT */
		if (grmatchline(search, gid, name))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
}

static int
grscan(int search, gid_t gid, const char *name)
{
	int		r;
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_local_grscan, NULL)
		NS_DNS_CB(_dns_grscan, NULL)
		NS_NIS_CB(_nis_grscan, NULL)
		NS_COMPAT_CB(_compat_grscan, NULL)
		{ 0 }
	};
	static const ns_src compatsrc[] = {
		{ NSSRC_COMPAT, NS_SUCCESS },
		{ 0 }
	};

	/* name may be NULL if search is nonzero */

	r = nsdispatch(NULL, dtab, NSDB_GROUP, "grscan", compatsrc,
	    search, gid, name);
	return (r == NS_SUCCESS) ? 1 : 0;
}

static int
grmatchline(int search, gid_t gid, const char *name)
{
	unsigned long	id;
	__aconst char	**m;
	char		*cp, *bp, *ep;

	/* name may be NULL if search is nonzero */

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
