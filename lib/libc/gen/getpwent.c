/*	$NetBSD: getpwent.c,v 1.39 1999/01/25 01:09:34 lukem Exp $	*/

/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Portions Copyright (c) 1994, 1995, Jason Downs.  All rights reserved.
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
static char sccsid[] = "@(#)getpwent.c	8.2 (Berkeley) 4/27/95";
#else
__RCSID("$NetBSD: getpwent.c,v 1.39 1999/01/25 01:09:34 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <fcntl.h>
#include <db.h>
#include <syslog.h>
#include <pwd.h>
#include <utmp.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <netgroup.h>
#include <nsswitch.h>
#ifdef HESIOD
#include <hesiod.h>
#endif
#ifdef YP
#include <machine/param.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#include "pw_private.h"

#ifdef __weak_alias
__weak_alias(endpwent,_endpwent);
__weak_alias(getpwent,_getpwent);
__weak_alias(getpwnam,_getpwnam);
__weak_alias(getpwuid,_getpwuid);
__weak_alias(setpassent,_setpassent);
__weak_alias(setpwent,_setpwent);
#endif


/*
 * The lookup techniques and data extraction code here must be kept
 * in sync with that in `pwd_mkdb'.
 */

static struct passwd _pw_passwd;	/* password structure */
static DB *_pw_db;			/* password database */
static int _pw_keynum;			/* key counter */
static int _pw_stayopen;		/* keep fd's open */
static int _pw_flags;			/* password flags */
static int _pw_none;			/* true if getpwent got EOF */

static int __hashpw __P((DBT *));
static int __initdb __P((void));

const char __yp_token[] = "__YP!";	/* Let pwd_mkdb pull this in. */
static const ns_src compatsrc[] = {
	{ NSSRC_COMPAT, NS_SUCCESS },
	{ 0 }
};

#ifdef YP
static char     *__ypcurrent, *__ypdomain;
static int      __ypcurrentlen;
#endif

#ifdef HESIOD
static int	_pw_hesnum;
#endif

#if defined(YP) || defined(HESIOD)
enum _pwmode { PWMODE_NONE, PWMODE_FULL, PWMODE_USER, PWMODE_NETGRP };
static enum _pwmode __pwmode;

enum _ypmap { YPMAP_NONE, YPMAP_ADJUNCT, YPMAP_MASTER };

static struct passwd	*__pwproto = (struct passwd *)NULL;
static int		 __pwproto_flags;
static char		 line[1024];
static long		 prbuf[1024 / sizeof(long)];
static DB		*__pwexclude = (DB *)NULL;
 
static int	__pwexclude_add __P((const char *));
static int	__pwexclude_is __P((const char *));
static void	__pwproto_set __P((void));
static int	__ypmaptype __P((void));
static int	__pwparse __P((struct passwd *, char *));

	/* macros for deciding which YP maps to use. */
#define PASSWD_BYNAME	(__ypmaptype() == YPMAP_MASTER \
			    ? "master.passwd.byname" : "passwd.byname")
#define PASSWD_BYUID	(__ypmaptype() == YPMAP_MASTER \
			    ? "master.passwd.byuid" : "passwd.byuid")

/*
 * add a name to the compat mode exclude list
 */
static int
__pwexclude_add(name)
	const char *name;
{
	DBT key;
	DBT data;

	/* initialize the exclusion table if needed. */
	if(__pwexclude == (DB *)NULL) {
		__pwexclude = dbopen(NULL, O_RDWR, 600, DB_HASH, NULL);
		if(__pwexclude == (DB *)NULL)
			return 1;
	}

	/* set up the key */
	key.size = strlen(name);
	/* LINTED key does not get modified */
	key.data = (char *)name;

	/* data is nothing. */
	data.data = NULL;
	data.size = 0;

	/* store it */
	if((__pwexclude->put)(__pwexclude, &key, &data, 0) == -1)
		return 1;
	
	return 0;
}

/*
 * test if a name is on the compat mode exclude list
 */
static int
__pwexclude_is(name)
	const char *name;
{
	DBT key;
	DBT data;

	if(__pwexclude == (DB *)NULL)
		return 0;	/* nothing excluded */

	/* set up the key */
	key.size = strlen(name);
	/* LINTED key does not get modified */
	key.data = (char *)name;

	if((__pwexclude->get)(__pwexclude, &key, &data, 0) == 0)
		return 1;	/* excluded */
	
	return 0;
}

/*
 * setup the compat mode prototype template
 */
static void
__pwproto_set()
{
	char *ptr;
	struct passwd *pw = &_pw_passwd;

	/* make this the new prototype */
	ptr = (char *)(void *)prbuf;

	/* first allocate the struct. */
	__pwproto = (struct passwd *)(void *)ptr;
	ptr += sizeof(struct passwd);

	/* name */
	if(pw->pw_name && (pw->pw_name)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_name, strlen(pw->pw_name) + 1);
		__pwproto->pw_name = ptr;
		ptr += (strlen(pw->pw_name) + 1);
	} else
		__pwproto->pw_name = (char *)NULL;
	
	/* password */
	if(pw->pw_passwd && (pw->pw_passwd)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_passwd, strlen(pw->pw_passwd) + 1);
		__pwproto->pw_passwd = ptr;
		ptr += (strlen(pw->pw_passwd) + 1);
	} else
		__pwproto->pw_passwd = (char *)NULL;

	/* uid */
	__pwproto->pw_uid = pw->pw_uid;

	/* gid */
	__pwproto->pw_gid = pw->pw_gid;

	/* change (ignored anyway) */
	__pwproto->pw_change = pw->pw_change;

	/* class (ignored anyway) */
	__pwproto->pw_class = "";

	/* gecos */
	if(pw->pw_gecos && (pw->pw_gecos)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_gecos, strlen(pw->pw_gecos) + 1);
		__pwproto->pw_gecos = ptr;
		ptr += (strlen(pw->pw_gecos) + 1);
	} else
		__pwproto->pw_gecos = (char *)NULL;
	
	/* dir */
	if(pw->pw_dir && (pw->pw_dir)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_dir, strlen(pw->pw_dir) + 1);
		__pwproto->pw_dir = ptr;
		ptr += (strlen(pw->pw_dir) + 1);
	} else
		__pwproto->pw_dir = (char *)NULL;

	/* shell */
	if(pw->pw_shell && (pw->pw_shell)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_shell, strlen(pw->pw_shell) + 1);
		__pwproto->pw_shell = ptr;
		ptr += (strlen(pw->pw_shell) + 1);
	} else
		__pwproto->pw_shell = (char *)NULL;

	/* expire (ignored anyway) */
	__pwproto->pw_expire = pw->pw_expire;

	/* flags */
	__pwproto_flags = _pw_flags;
}

static int
__ypmaptype()
{
	static int maptype = -1;
	int order, r;

	if (maptype != -1)
		return (maptype);

	maptype = YPMAP_NONE;
	if (geteuid() != 0)
		return (maptype);

	if (!__ypdomain) {
		if( _yp_check(&__ypdomain) == 0)
			return (maptype);
	}

	r = yp_order(__ypdomain, "master.passwd.byname", &order);
	if (r == 0) {
		maptype = YPMAP_MASTER;
		return (maptype);
	}

	/*
	 * NIS+ in YP compat mode doesn't support
	 * YPPROC_ORDER -- no point in continuing.
	 */
	if (r == YPERR_YPERR)
		return (maptype);

	/* master.passwd doesn't exist -- try passwd.adjunct */
	if (r == YPERR_MAP) {
		r = yp_order(__ypdomain, "passwd.adjunct.byname", &order);
		if (r == 0)
			maptype = YPMAP_ADJUNCT;
		return (maptype);
	}

	return (maptype);
}

/*
 * parse an old-style passwd file line (from NIS or HESIOD)
 */
static int
__pwparse(pw, s)
	struct passwd *pw;
	char *s;
{
	static char adjunctpw[YPMAXRECORD + 2];
	int flags, maptype;

	maptype = __ypmaptype();
	flags = _PASSWORD_NOWARN;
	if (maptype != YPMAP_MASTER)
		flags |= _PASSWORD_OLDFMT;
	if (! __pw_scan(s, pw, &flags))
		return 1;

	/* now let the prototype override, if set. */
	if(__pwproto != (struct passwd *)NULL) {
#ifdef PW_OVERRIDE_PASSWD
		if(__pwproto->pw_passwd != (char *)NULL)
			pw->pw_passwd = __pwproto->pw_passwd;
#endif
		if(!(__pwproto_flags & _PASSWORD_NOUID))
			pw->pw_uid = __pwproto->pw_uid;
		if(!(__pwproto_flags & _PASSWORD_NOGID))
			pw->pw_gid = __pwproto->pw_gid;
		if(__pwproto->pw_gecos != (char *)NULL)
			pw->pw_gecos = __pwproto->pw_gecos;
		if(__pwproto->pw_dir != (char *)NULL)
			pw->pw_dir = __pwproto->pw_dir;
		if(__pwproto->pw_shell != (char *)NULL)
			pw->pw_shell = __pwproto->pw_shell;
	}
	if ((maptype == YPMAP_ADJUNCT) &&
	    (strstr(pw->pw_passwd, "##") != NULL)) {
		char *data, *bp;
		int datalen;

		if (yp_match(__ypdomain, "passwd.adjunct.byname", pw->pw_name,
		    (int)strlen(pw->pw_name), &data, &datalen) == 0) {
			if (datalen > sizeof(adjunctpw) - 1)
				datalen = sizeof(adjunctpw) - 1;
			strncpy(adjunctpw, data, (size_t)datalen);

				/* skip name to get password */
			if ((bp = strsep(&data, ":")) != NULL &&
			    (bp = strsep(&data, ":")) != NULL)
				pw->pw_passwd = bp;
		}
	}
	return 0;
}
#endif /* YP || HESIOD */

/*
 * local files implementation of getpw*()
 * varargs: type, [ uid (type == _PW_KEYBYUID) | name (type == _PW_KEYBYNAME) ]
 */
static int	_local_getpw __P((void *, void *, va_list));

/*ARGSUSED*/
static int
_local_getpw(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	DBT		 key;
	char		 bf[/*CONSTCOND*/ MAX(MAXLOGNAME, sizeof(_pw_keynum)) + 1];
	uid_t		 uid;
	int		 search, len, rval;
	const char	*name;

	if (!_pw_db && !__initdb())
		return NS_UNAVAIL;

	search = va_arg(ap, int);
	bf[0] = search;
	switch (search) {
	case _PW_KEYBYNUM:
		++_pw_keynum;
		memmove(bf + 1, &_pw_keynum, sizeof(_pw_keynum));
		key.size = sizeof(_pw_keynum) + 1;
		break;
	case _PW_KEYBYNAME:
		name = va_arg(ap, const char *);
		len = strlen(name);
		memmove(bf + 1, name, (size_t)MIN(len, MAXLOGNAME));
		key.size = len + 1;
		break;
	case _PW_KEYBYUID:
		uid = va_arg(ap, uid_t);
		memmove(bf + 1, &uid, sizeof(len));
		key.size = sizeof(uid) + 1;
		break;
	default:
		abort();
	}

	key.data = (u_char *)bf;
	rval = __hashpw(&key);
	if (rval == NS_NOTFOUND && search == _PW_KEYBYNUM) {
		_pw_none = 1;
		rval = NS_SUCCESS;
	}

	if (!_pw_stayopen && (search != _PW_KEYBYNUM)) {
		(void)(_pw_db->close)(_pw_db);
		_pw_db = (DB *)NULL;
	}
	return (rval);
}

#ifdef HESIOD
/*
 * hesiod implementation of getpw*()
 * varargs: type, [ uid (type == _PW_KEYBYUID) | name (type == _PW_KEYBYNAME) ]
 */
static int	_dns_getpw __P((void *, void *, va_list));

/*ARGSUSED*/
static int
_dns_getpw(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	const char	 *name;
	uid_t		  uid;
	int		  search;

	char		 *map;
	char		**hp;
	void		 *context;
	int		  r;

	search = va_arg(ap, int);
	switch (search) {
	case _PW_KEYBYNUM:
		snprintf(line, sizeof(line) - 1, "passwd-%u", _pw_hesnum);
		_pw_hesnum++;
		map = "passwd";
		break;
	case _PW_KEYBYNAME:
		name = va_arg(ap, const char *);
		strncpy(line, name, sizeof(line));
		map = "passwd";
		break;
	case _PW_KEYBYUID:
		uid = va_arg(ap, uid_t);
		snprintf(line, sizeof(line), "%u", (unsigned int)uid);
		map = "uid";
		break;
	default:
		abort();
	}
	line[sizeof(line) - 1] = '\0';

	r = NS_UNAVAIL;
	if (hesiod_init(&context) == -1)
		return (r);

	hp = hesiod_resolve(context, line, map);
	if (hp == NULL) {
		if (errno == ENOENT) {
			if (search == _PW_KEYBYNUM) {
				_pw_hesnum = 0;
				_pw_none = 1;
				r = NS_SUCCESS;
			} else
				r = NS_NOTFOUND;
		}
		goto cleanup_dns_getpw;
	}

	strncpy(line, hp[0], sizeof(line));	/* only check first elem */
	line[sizeof(line) - 1] = '\0';
	hesiod_free_list(context, hp);
	if (__pwparse(&_pw_passwd, line))
		r = NS_UNAVAIL;
	else
		r = NS_SUCCESS;
 cleanup_dns_getpw:
	hesiod_end(context);
	return (r);
}
#endif

#ifdef YP
/*
 * nis implementation of getpw*()
 * varargs: type, [ uid (type == _PW_KEYBYUID) | name (type == _PW_KEYBYNAME) ]
 */
static int	_nis_getpw __P((void *, void *, va_list));

/*ARGSUSED*/
static int
_nis_getpw(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	const char	*name;
	uid_t		 uid;
	int		 search;
	char		*key, *data;
	char		*map = PASSWD_BYNAME;
	int		 keylen, datalen, r;

	if(__ypdomain == NULL) {
		if(_yp_check(&__ypdomain) == 0)
			return NS_UNAVAIL;
	}

	search = va_arg(ap, int);
	switch (search) {
	case _PW_KEYBYNUM:
		break;
	case _PW_KEYBYNAME:
		name = va_arg(ap, const char *);
		strncpy(line, name, sizeof(line));
		break;
	case _PW_KEYBYUID:
		uid = va_arg(ap, uid_t);
		snprintf(line, sizeof(line), "%u", (unsigned int)uid);
		map = PASSWD_BYUID;
		break;
	default:
		abort();
	}
	line[sizeof(line) - 1] = '\0';
	if (search != _PW_KEYBYNUM) {
		data = NULL;
		r = yp_match(__ypdomain, map, line, (int)strlen(line),
				&data, &datalen);
		switch (r) {
		case 0:
			break;
		case YPERR_KEY:
			r =  NS_NOTFOUND;
			break;
		default:
			r = NS_UNAVAIL;
			break;
		}
		if (r != 0) {
			if (data)
				free(data);
			return r;
		}
		data[datalen] = '\0';		/* clear trailing \n */
		strncpy(line, data, sizeof(line));
		line[sizeof(line) - 1] = '\0';
		free(data);
		if (__pwparse(&_pw_passwd, line))
			return NS_UNAVAIL;
		return NS_SUCCESS;
	}

	for (;;) {
		data = key = NULL;
		if (__ypcurrent) {
			r = yp_next(__ypdomain, map,
					__ypcurrent, __ypcurrentlen,
					&key, &keylen, &data, &datalen);
			free(__ypcurrent);
			switch (r) {
			case 0:
				__ypcurrent = key;
				__ypcurrentlen = keylen;
				break;
			case YPERR_NOMORE:
				__ypcurrent = NULL;
				_pw_none = 1;
				if (key)
					free(key);
				return NS_SUCCESS;
			default:
				r = NS_UNAVAIL;
				break;
			}
		} else {
			r = 0;
			if (yp_first(__ypdomain, map, &__ypcurrent,
					&__ypcurrentlen, &data, &datalen))
				r = NS_UNAVAIL;
		}
		if (r != 0) {
			if (key)
				free(key);
			if (data)
				free(data);
			return r;
		}
		data[datalen] = '\0';		/* clear trailing \n */
		strncpy(line, data, sizeof(line));
		line[sizeof(line) - 1] = '\0';
				free(data);
		if (! __pwparse(&_pw_passwd, line))
			return NS_SUCCESS;
	}
	/* NOTREACHED */
} /* _nis_getpw */
#endif

#if defined(YP) || defined(HESIOD)
/*
 * See if the compat token is in the database.  Only works if pwd_mkdb knows
 * about the token.
 */
static int	__has_compatpw __P((void));

static int
__has_compatpw()
{
	DBT key, data;
	DBT pkey, pdata;
	char bf[MAXLOGNAME];

	/*LINTED*/
	key.data = (u_char *)__yp_token;
	key.size = strlen(__yp_token);

	/* Pre-token database support. */
	bf[0] = _PW_KEYBYNAME;
	bf[1] = '+';
	pkey.data = (u_char *)bf;
	pkey.size = 2;

	if ((_pw_db->get)(_pw_db, &key, &data, 0)
	    && (_pw_db->get)(_pw_db, &pkey, &pdata, 0))
		return 0;		/* No compat token */
	return 1;
}

/*
 * log an error if "files" or "compat" is specified in passwd_compat database
 */
static int	_bad_getpw __P((void *, void *, va_list));

/*ARGSUSED*/
static int
_bad_getpw(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	static int warned;
	if (!warned) {
		syslog(LOG_ERR,
			"nsswitch.conf passwd_compat database can't use '%s'",
			(char *)cb_data);
	}
	warned = 1;
	return NS_UNAVAIL;
}

/*
 * when a name lookup in compat mode is required (e.g., '+name', or a name in
 * '+@netgroup'), look it up in the 'passwd_compat' nsswitch database.
 * only Hesiod and NIS is supported - it doesn't make sense to lookup
 * compat names from 'files' or 'compat'.
 */
static int	__getpwcompat __P((int, uid_t, const char *));

static int
__getpwcompat(type, uid, name)
	int		 type;
	uid_t		 uid;
	const char	*name;
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_bad_getpw, "files")
		NS_DNS_CB(_dns_getpw, NULL)
		NS_NIS_CB(_nis_getpw, NULL)
		NS_COMPAT_CB(_bad_getpw, "compat")
		{ 0 }
	};
	static const ns_src defaultnis[] = {
		{ NSSRC_NIS, 	NS_SUCCESS },
		{ 0 }
	};

	switch (type) {
	case _PW_KEYBYNUM:
		return nsdispatch(NULL, dtab, NSDB_PASSWD_COMPAT, "getpwcompat",
		    defaultnis, type);
	case _PW_KEYBYNAME:
		return nsdispatch(NULL, dtab, NSDB_PASSWD_COMPAT, "getpwcompat",
		    defaultnis, type, name);
	case _PW_KEYBYUID:
		return nsdispatch(NULL, dtab, NSDB_PASSWD_COMPAT, "getpwcompat",
		    defaultnis, type, uid);
	default:
		abort();
		/*NOTREACHED*/
	}
}

/*
 * compat implementation of getpwent()
 * varargs (ignored):
 *	type, [ uid (type == _PW_KEYBYUID) | name (type == _PW_KEYBYNAME) ]
 */
static int	_compat_getpwent __P((void *, void *, va_list));

/*ARGSUSED*/
static int
_compat_getpwent(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	DBT		 key;
	char		 bf[sizeof(_pw_keynum) + 1];
	static char	*name = NULL;
	const char	*user, *host, *dom;
	int		 has_compatpw;

	if (!_pw_db && !__initdb())
		return NS_UNAVAIL;

	has_compatpw = __has_compatpw();

again:
	if (has_compatpw && (__pwmode != PWMODE_NONE)) {
		int r;

		switch (__pwmode) {
		case PWMODE_FULL:
			r = __getpwcompat(_PW_KEYBYNUM, 0, NULL);
			if (r == NS_SUCCESS)
				return r;
			__pwmode = PWMODE_NONE;
			break;

		case PWMODE_NETGRP:
			r = getnetgrent(&host, &user, &dom);
			if (r == 0) {	/* end of group */
				endnetgrent();
				__pwmode = PWMODE_NONE;
				break;
			}
			if (!user || !*user)
				break;
			r = __getpwcompat(_PW_KEYBYNAME, 0, user);
			if (r == NS_SUCCESS)
				return r;
			break;

		case PWMODE_USER:
			if (name == NULL) {
				__pwmode = PWMODE_NONE;
				break;
			}
			r = __getpwcompat(_PW_KEYBYNAME, 0, name);
			free(name);
			name = NULL;
			if (r == NS_SUCCESS)
				return r;
			break;

		case PWMODE_NONE:
			abort();
		}
		goto again;
	}

	++_pw_keynum;
	bf[0] = _PW_KEYBYNUM;
	memmove(bf + 1, &_pw_keynum, sizeof(_pw_keynum));
	key.data = (u_char *)bf;
	key.size = sizeof(_pw_keynum) + 1;
	if(__hashpw(&key) == NS_SUCCESS) {
		/* if we don't have YP at all, don't bother. */
		if (has_compatpw) {
			if(_pw_passwd.pw_name[0] == '+') {
				/* set the mode */
				switch(_pw_passwd.pw_name[1]) {
				case '\0':
					__pwmode = PWMODE_FULL;
					break;
				case '@':
					__pwmode = PWMODE_NETGRP;
					setnetgrent(_pw_passwd.pw_name + 2);
					break;
				default:
					__pwmode = PWMODE_USER;
					name = strdup(_pw_passwd.pw_name + 1);
					break;
				}

				/* save the prototype */
				__pwproto_set();
				goto again;
			} else if(_pw_passwd.pw_name[0] == '-') {
				/* an attempted exclusion */
				switch(_pw_passwd.pw_name[1]) {
				case '\0':
					break;
				case '@':
					setnetgrent(_pw_passwd.pw_name + 2);
					while(getnetgrent(&host, &user, &dom)) {
						if(user && *user)
							__pwexclude_add(user);
					}
					endnetgrent();
					break;
				default:
					__pwexclude_add(_pw_passwd.pw_name + 1);
					break;
				}
				goto again;
			}
		}
		return NS_SUCCESS;
	}
	return NS_NOTFOUND;
}

/*
 * compat implementation of getpwnam() and getpwuid()
 * varargs: type, [ uid (type == _PW_KEYBYUID) | name (type == _PW_KEYBYNAME) ]
 */
static int	_compat_getpw __P((void *, void *, va_list));

static int
_compat_getpw(rv, cb_data, ap)
	void	*rv;
	void	*cb_data;
	va_list	 ap;
{
	DBT		key;
	int		search, rval, r, s;
	uid_t		uid;
	char		bf[MAXLOGNAME + 1];
	const char	*name, *host, *user, *dom;

	if (!_pw_db && !__initdb())
		return NS_UNAVAIL;

		/*
		 * If there isn't a compat token in the database, use files.
		 */
	if (! __has_compatpw())
		return (_local_getpw(rv, cb_data, ap));

	search = va_arg(ap, int);
	uid = 0;
	name = NULL;
	rval = NS_NOTFOUND;
	switch (search) {
	case _PW_KEYBYNAME:
		name = va_arg(ap, const char *);
		break;
	case _PW_KEYBYUID:
		uid = va_arg(ap, uid_t);
		break;
	default:
		abort();
	}

	for(s = -1, _pw_keynum=1; _pw_keynum; _pw_keynum++) {
		bf[0] = _PW_KEYBYNUM;
		memmove(bf + 1, &_pw_keynum, sizeof(_pw_keynum));
		key.data = (u_char *)bf;
		key.size = sizeof(_pw_keynum) + 1;
		if(__hashpw(&key) != NS_SUCCESS)
			break;
		switch(_pw_passwd.pw_name[0]) {
		case '+':
			/* save the prototype */
			__pwproto_set();

			switch(_pw_passwd.pw_name[1]) {
			case '\0':
				r = __getpwcompat(search, uid, name);
				if (r != NS_SUCCESS)
					continue;
				break;
			case '@':
pwnam_netgrp:
				if(__ypcurrent) {
					free(__ypcurrent);
					__ypcurrent = NULL;
				}
				if (s == -1)		/* first time */
					setnetgrent(_pw_passwd.pw_name + 2);
				s = getnetgrent(&host, &user, &dom);
				if (s == 0) {		/* end of group */
					endnetgrent();
					s = -1;
					continue;
				}
				if (!user || !*user)
					goto pwnam_netgrp;

				r = __getpwcompat(_PW_KEYBYNAME, 0, user);

				if (r == NS_UNAVAIL)
					return r;
				if (r == NS_NOTFOUND) {
					/*
					 * just because this user is bad
					 * it doesn't mean they all are.
					 */
					goto pwnam_netgrp;
				}
				break;
			default:
				user = _pw_passwd.pw_name + 1;
				r = __getpwcompat(_PW_KEYBYNAME, 0, user);

				if (r == NS_UNAVAIL)
					return r;
				if (r == NS_NOTFOUND)
					continue;
				break;
			}
			if(__pwexclude_is(_pw_passwd.pw_name)) {
				if(s == 1)		/* inside netgroup */
					goto pwnam_netgrp;
				continue;
			}
			break;
		case '-':
			/* attempted exclusion */
			switch(_pw_passwd.pw_name[1]) {
			case '\0':
				break;
			case '@':
				setnetgrent(_pw_passwd.pw_name + 2);
				while(getnetgrent(&host, &user, &dom)) {
					if(user && *user)
						__pwexclude_add(user);
				}
				endnetgrent();
				break;
			default:
				__pwexclude_add(_pw_passwd.pw_name + 1);
				break;
			}
			break;
		}
		if ((search == _PW_KEYBYNAME &&
			    strcmp(_pw_passwd.pw_name, name) == 0)
		 || (search == _PW_KEYBYUID && _pw_passwd.pw_uid == uid)) {
			rval = NS_SUCCESS;
			break;
		}
		if(s == 1)				/* inside netgroup */
			goto pwnam_netgrp;
		continue;
	}
	__pwproto = (struct passwd *)NULL;

	if (!_pw_stayopen) {
		(void)(_pw_db->close)(_pw_db);
		_pw_db = (DB *)NULL;
	}
	if(__pwexclude != (DB *)NULL) {
		(void)(__pwexclude->close)(__pwexclude);
			__pwexclude = (DB *)NULL;
	}
	return rval;
}
#endif /* YP || HESIOD */

struct passwd *
getpwent()
{
	int		r;
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_local_getpw, NULL)
		NS_DNS_CB(_dns_getpw, NULL)
		NS_NIS_CB(_nis_getpw, NULL)
		NS_COMPAT_CB(_compat_getpwent, NULL)
		{ 0 }
	};

	_pw_none = 0;
	r = nsdispatch(NULL, dtab, NSDB_PASSWD, "getpwent", compatsrc,
	    _PW_KEYBYNUM);
	if (_pw_none || r != NS_SUCCESS)
		return (struct passwd *)NULL;
	return &_pw_passwd;
}

struct passwd *
getpwnam(name)
	const char *name;
{
	int		r;
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_local_getpw, NULL)
		NS_DNS_CB(_dns_getpw, NULL)
		NS_NIS_CB(_nis_getpw, NULL)
		NS_COMPAT_CB(_compat_getpw, NULL)
		{ 0 }
	};

	if (name == NULL || name[0] == '\0')
		return (struct passwd *)NULL;

	r = nsdispatch(NULL, dtab, NSDB_PASSWD, "getpwnam", compatsrc,
	    _PW_KEYBYNAME, name);
	return (r == NS_SUCCESS ? &_pw_passwd : (struct passwd *)NULL);
}

struct passwd *
getpwuid(uid)
	uid_t uid;
{
	int		r;
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_local_getpw, NULL)
		NS_DNS_CB(_dns_getpw, NULL)
		NS_NIS_CB(_nis_getpw, NULL)
		NS_COMPAT_CB(_compat_getpw, NULL)
		{ 0 }
	};

	r = nsdispatch(NULL, dtab, NSDB_PASSWD, "getpwuid", compatsrc,
	    _PW_KEYBYUID, uid);
	return (r == NS_SUCCESS ? &_pw_passwd : (struct passwd *)NULL);
}

int
setpassent(stayopen)
	int stayopen;
{
	_pw_keynum = 0;
	_pw_stayopen = stayopen;
#ifdef YP
	__pwmode = PWMODE_NONE;
	if(__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
#endif
#ifdef HESIOD
	_pw_hesnum = 0;
#endif
#if defined(YP) || defined(HESIOD)
	if(__pwexclude != (DB *)NULL) {
		(void)(__pwexclude->close)(__pwexclude);
		__pwexclude = (DB *)NULL;
	}
	__pwproto = (struct passwd *)NULL;
#endif
	return 1;
}

void
setpwent()
{
	(void) setpassent(0);
}

void
endpwent()
{
	_pw_keynum = 0;
	if (_pw_db) {
		(void)(_pw_db->close)(_pw_db);
		_pw_db = (DB *)NULL;
	}
#if defined(YP) || defined(HESIOD)
	__pwmode = PWMODE_NONE;
#endif
#ifdef YP
	if(__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
#endif
#ifdef HESIOD
	_pw_hesnum = 0;
#endif
#if defined(YP) || defined(HESIOD)
	if(__pwexclude != (DB *)NULL) {
		(void)(__pwexclude->close)(__pwexclude);
		__pwexclude = (DB *)NULL;
	}
	__pwproto = (struct passwd *)NULL;
#endif
}

static int
__initdb()
{
	static int warned;
	char *p;

#if defined(YP) || defined(HESIOD)
	__pwmode = PWMODE_NONE;
#endif
	if (geteuid() == 0) {
		_pw_db = dbopen((p = _PATH_SMP_DB), O_RDONLY, 0, DB_HASH, NULL);
		if (_pw_db)
			return(1);
	}
	_pw_db = dbopen((p = _PATH_MP_DB), O_RDONLY, 0, DB_HASH, NULL);
	if (_pw_db)
		return 1;
	if (!warned)
		syslog(LOG_ERR, "%s: %m", p);
	warned = 1;
	return 0;
}

static int
__hashpw(key)
	DBT *key;
{
	char *p, *t;
	static u_int max;
	static char *buf;
	DBT data;

	switch ((_pw_db->get)(_pw_db, key, &data, 0)) {
	case 0:
		break;			/* found */
	case 1:
		return NS_NOTFOUND;
	case -1:			
		return NS_UNAVAIL;	/* error in db routines */
	default:
		abort();
	}

	p = (char *)data.data;
	if (data.size > max && !(buf = realloc(buf, (max += 1024))))
		return NS_UNAVAIL;

	/* THIS CODE MUST MATCH THAT IN pwd_mkdb. */
	t = buf;
#define	EXPAND(e)	e = t; while ((*t++ = *p++));
#define	SCALAR(v)	memmove(&(v), p, sizeof v); p += sizeof v
	EXPAND(_pw_passwd.pw_name);
	EXPAND(_pw_passwd.pw_passwd);
	SCALAR(_pw_passwd.pw_uid);
	SCALAR(_pw_passwd.pw_gid);
	SCALAR(_pw_passwd.pw_change);
	EXPAND(_pw_passwd.pw_class);
	EXPAND(_pw_passwd.pw_gecos);
	EXPAND(_pw_passwd.pw_dir);
	EXPAND(_pw_passwd.pw_shell);
	SCALAR(_pw_passwd.pw_expire);

	/* See if there's any data left.  If so, read in flags. */
	if (data.size > (p - (char *)data.data)) {
		SCALAR(_pw_flags);
	} else
		_pw_flags = _PASSWORD_NOUID|_PASSWORD_NOGID;	/* default */

	return NS_SUCCESS;
}
