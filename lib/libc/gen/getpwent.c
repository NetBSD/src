/*	$NetBSD: getpwent.c,v 1.30 1998/11/12 16:38:49 christos Exp $	*/

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
__RCSID("$NetBSD: getpwent.c,v 1.30 1998/11/12 16:38:49 christos Exp $");
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
static int __hashpw __P((DBT *));
static int __initdb __P((void));

const char __yp_token[] = "__YP!";	/* Let pwd_mkdb pull this in. */

#ifdef YP
enum _ypmode { YPMODE_NONE, YPMODE_FULL, YPMODE_USER, YPMODE_NETGRP };
static enum _ypmode __ypmode;

enum _ypmap { YPMAP_NONE, YPMAP_ADJUNCT, YPMAP_MASTER };

static char     *__ypcurrent, *__ypdomain;
static int      __ypcurrentlen;
static struct passwd *__ypproto = (struct passwd *)NULL;
static int	__ypflags;
static char	line[1024];
static long	prbuf[1024 / sizeof(long)];
static DB *__ypexclude = (DB *)NULL;

static int __has_yppw __P((void));
static int __ypexclude_add __P((const char *));
static int __ypexclude_is __P((const char *));
static int __ypmaptype __P((void));
static void __ypproto_set __P((void));
static int __ypparse __P((struct passwd *, char *));

	/* macros for deciding which YP maps to use. */
#define PASSWD_BYNAME	(__ypmaptype() == YPMAP_MASTER \
			    ? "master.passwd.byname" : "passwd.byname")
#define PASSWD_BYUID	(__ypmaptype() == YPMAP_MASTER \
			    ? "master.passwd.byuid" : "passwd.byuid")

static int
__ypexclude_add(name)
	const char *name;
{
	DBT key;
	DBT data;

	/* initialize the exclusion table if needed. */
	if(__ypexclude == (DB *)NULL) {
		__ypexclude = dbopen(NULL, O_RDWR, 600, DB_HASH, NULL);
		if(__ypexclude == (DB *)NULL)
			return(1);
	}

	/* set up the key */
	key.size = strlen(name);
	/* LINTED key does not get modified */
	key.data = (char *)name;

	/* data is nothing. */
	data.data = NULL;
	data.size = 0;

	/* store it */
	if((__ypexclude->put)(__ypexclude, &key, &data, 0) == -1)
		return(1);
	
	return(0);
}

static int
__ypexclude_is(name)
	const char *name;
{
	const DBT key;
	DBT data;

	if(__ypexclude == (DB *)NULL)
		return(0);	/* nothing excluded */

	/* set up the key */
	key.size = strlen(name);
	/* LINTED key does not get modified */
	key.data = (char *)name;

	if((__ypexclude->get)(__ypexclude, &key, &data, 0) == 0)
		return(1);	/* excluded */
	
	return(0);
}

static void
__ypproto_set()
{
	char *ptr;
	struct passwd *pw = &_pw_passwd;

	/* make this the new prototype */
	ptr = (char *)(void *)prbuf;

	/* first allocate the struct. */
	__ypproto = (struct passwd *)(void *)ptr;
	ptr += sizeof(struct passwd);

	/* name */
	if(pw->pw_name && (pw->pw_name)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_name, strlen(pw->pw_name) + 1);
		__ypproto->pw_name = ptr;
		ptr += (strlen(pw->pw_name) + 1);
	} else
		__ypproto->pw_name = (char *)NULL;
	
	/* password */
	if(pw->pw_passwd && (pw->pw_passwd)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_passwd, strlen(pw->pw_passwd) + 1);
		__ypproto->pw_passwd = ptr;
		ptr += (strlen(pw->pw_passwd) + 1);
	} else
		__ypproto->pw_passwd = (char *)NULL;

	/* uid */
	__ypproto->pw_uid = pw->pw_uid;

	/* gid */
	__ypproto->pw_gid = pw->pw_gid;

	/* change (ignored anyway) */
	__ypproto->pw_change = pw->pw_change;

	/* class (ignored anyway) */
	__ypproto->pw_class = "";

	/* gecos */
	if(pw->pw_gecos && (pw->pw_gecos)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_gecos, strlen(pw->pw_gecos) + 1);
		__ypproto->pw_gecos = ptr;
		ptr += (strlen(pw->pw_gecos) + 1);
	} else
		__ypproto->pw_gecos = (char *)NULL;
	
	/* dir */
	if(pw->pw_dir && (pw->pw_dir)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_dir, strlen(pw->pw_dir) + 1);
		__ypproto->pw_dir = ptr;
		ptr += (strlen(pw->pw_dir) + 1);
	} else
		__ypproto->pw_dir = (char *)NULL;

	/* shell */
	if(pw->pw_shell && (pw->pw_shell)[0]) {
		ptr = (char *)ALIGN((u_long)ptr);
		memmove(ptr, pw->pw_shell, strlen(pw->pw_shell) + 1);
		__ypproto->pw_shell = ptr;
		ptr += (strlen(pw->pw_shell) + 1);
	} else
		__ypproto->pw_shell = (char *)NULL;

	/* expire (ignored anyway) */
	__ypproto->pw_expire = pw->pw_expire;

	/* flags */
	__ypflags = _pw_flags;
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

static int
__ypparse(pw, s)
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
	if(__ypproto != (struct passwd *)NULL) {
#ifdef YP_OVERRIDE_PASSWD
		if(__ypproto->pw_passwd != (char *)NULL)
			pw->pw_passwd = __ypproto->pw_passwd;
#endif
		if(!(__ypflags & _PASSWORD_NOUID))
			pw->pw_uid = __ypproto->pw_uid;
		if(!(__ypflags & _PASSWORD_NOGID))
			pw->pw_gid = __ypproto->pw_gid;
		if(__ypproto->pw_gecos != (char *)NULL)
			pw->pw_gecos = __ypproto->pw_gecos;
		if(__ypproto->pw_dir != (char *)NULL)
			pw->pw_dir = __ypproto->pw_dir;
		if(__ypproto->pw_shell != (char *)NULL)
			pw->pw_shell = __ypproto->pw_shell;
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
#endif

struct passwd *
getpwent()
{
	DBT dbt;
	char bf[sizeof(_pw_keynum) + 1];
#ifdef YP
	static char *name = (char *)NULL;
	const char *user, *host, *dom;
	int has_yppw;
#endif

	if (!_pw_db && !__initdb())
		return((struct passwd *)NULL);

#ifdef YP
	has_yppw = __has_yppw();

again:
	if(has_yppw && (__ypmode != YPMODE_NONE)) {
		char *key, *data;
		int keylen, datalen;
		int r, s;

		if(!__ypdomain) {
			if( _yp_check(&__ypdomain) == 0) {
				__ypmode = YPMODE_NONE;
				goto again;
			}
		}
		switch(__ypmode) {
		case YPMODE_FULL:
			data = NULL;
			if(__ypcurrent) {
				key = NULL;
				r = yp_next(__ypdomain, PASSWD_BYNAME,
					__ypcurrent, __ypcurrentlen,
					&key, &keylen, &data, &datalen);
				free(__ypcurrent);
				if(r != 0) {
					__ypcurrent = NULL;
					if (key)
						free(key);
				}
				else {
					__ypcurrent = key;
					__ypcurrentlen = keylen;
				}
			} else {
				r = yp_first(__ypdomain, PASSWD_BYNAME,
					&__ypcurrent, &__ypcurrentlen,
					&data, &datalen);
			}
			if(r != 0) {
				__ypmode = YPMODE_NONE;
				if(data)
					free(data);
				data = NULL;
				goto again;
			}
			memmove(line, data, (size_t)datalen);
			free(data);
			data = NULL;
			break;
		case YPMODE_NETGRP:
			s = getnetgrent(&host, &user, &dom);
			if(s == 0) {	/* end of group */
				endnetgrent();
				__ypmode = YPMODE_NONE;
				goto again;
			}
			if(user && *user) {
				data = NULL;
				r = yp_match(__ypdomain, PASSWD_BYNAME,
					user, (int)strlen(user),
					&data, &datalen);
			} else
				goto again;
			if(r != 0) {
				/*
				 * if the netgroup is invalid, keep looking
				 * as there may be valid users later on.
				 */
				if(data)
					free(data);
				goto again;
			}
			memmove(line, data, (size_t)datalen);
			free(data);
			data = NULL;
			break;
		case YPMODE_USER:
			if(name != (char *)NULL) {
				data = NULL;
				r = yp_match(__ypdomain, PASSWD_BYNAME,
					name, (int)strlen(name),
					&data, &datalen);
				__ypmode = YPMODE_NONE;
				free(name);
				name = NULL;
				if(r != 0) {
					if(data)
						free(data);
					goto again;
				}
				memmove(line, data, (size_t)datalen);
				free(data);
				data = (char *)NULL;
			} else {		/* XXX */
				__ypmode = YPMODE_NONE;
				goto again;
			}
			break;
		case YPMODE_NONE:
			abort();	/* cannot happen */
			break;
		}

		line[datalen] = '\0';
		if (__ypparse(&_pw_passwd, line))
			goto again;
		return &_pw_passwd;
	}
#endif

	++_pw_keynum;
	bf[0] = _PW_KEYBYNUM;
	memmove(bf + 1, &_pw_keynum, sizeof(_pw_keynum));
	dbt.data = (u_char *)bf;
	dbt.size = sizeof(_pw_keynum) + 1;
	if(__hashpw(&dbt)) {
#ifdef YP
		/* if we don't have YP at all, don't bother. */
		if(has_yppw) {
			if(_pw_passwd.pw_name[0] == '+') {
				/* set the mode */
				switch(_pw_passwd.pw_name[1]) {
				case '\0':
					__ypmode = YPMODE_FULL;
					break;
				case '@':
					__ypmode = YPMODE_NETGRP;
					setnetgrent(_pw_passwd.pw_name + 2);
					break;
				default:
					__ypmode = YPMODE_USER;
					name = strdup(_pw_passwd.pw_name + 1);
					break;
				}

				/* save the prototype */
				__ypproto_set();
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
							__ypexclude_add(user);
					}
					endnetgrent();
					break;
				default:
					__ypexclude_add(_pw_passwd.pw_name + 1);
					break;
				}
				goto again;
			}
		}
#endif
		return &_pw_passwd;
	}
	return (struct passwd *)NULL;
}

#ifdef YP

/*
 * See if the YP token is in the database.  Only works if pwd_mkdb knows
 * about the token.
 */
static int
__has_yppw()
{
	DBT key, data;
	DBT pkey, pdata;
	char bf[MAXLOGNAME];

	key.size = strlen(__yp_token);
	/* LINTED key does not get modified */
	key.data = (u_char *)__yp_token;

	/* Pre-token database support. */
	bf[0] = _PW_KEYBYNAME;
	bf[1] = '+';
	pkey.data = (u_char *)bf;
	pkey.size = 2;

	if ((_pw_db->get)(_pw_db, &key, &data, 0)
	    && (_pw_db->get)(_pw_db, &pkey, &pdata, 0))
		return(0);	/* No YP. */
	return(1);
}
#endif

struct passwd *
getpwnam(name)
	const char *name;
{
	DBT key;
	int len, rval;
	char bf[MAXLOGNAME + 1];

	if (!_pw_db && !__initdb())
		return((struct passwd *)NULL);

#ifdef YP
	/*
	 * If YP is active, we must sequence through the passwd file
	 * in sequence.
	 */
	if (__has_yppw()) {
		int r;
		int s = -1;
		const char *host, *user, *dom;

		for(_pw_keynum=1; _pw_keynum; _pw_keynum++) {
			bf[0] = _PW_KEYBYNUM;
			memmove(bf + 1, &_pw_keynum, sizeof(_pw_keynum));
			key.data = (u_char *)bf;
			key.size = sizeof(_pw_keynum) + 1;
			if(__hashpw(&key) == 0)
				break;
			switch(_pw_passwd.pw_name[0]) {
			case '+':
				if(!__ypdomain) {
					if(_yp_check(&__ypdomain) == 0) {
						continue;
					}
				}
				/* save the prototype */
				__ypproto_set();

				switch(_pw_passwd.pw_name[1]) {
				case '\0':
					if(__ypcurrent) {
						free(__ypcurrent);
						__ypcurrent = NULL;
					}
					r = yp_match(__ypdomain,
						PASSWD_BYNAME,
						name, (int)strlen(name),
						&__ypcurrent, &__ypcurrentlen);
					if(r != 0) {
						if(__ypcurrent)
							free(__ypcurrent);
						__ypcurrent = NULL;
						continue;
					}
					break;
				case '@':
pwnam_netgrp:
					if(__ypcurrent) {
						free(__ypcurrent);
						__ypcurrent = NULL;
					}
					if(s == -1)	/* first time */
						setnetgrent(_pw_passwd.pw_name + 2);
					s = getnetgrent(&host, &user, &dom);
					if(s == 0) {	/* end of group */
						endnetgrent();
						s = -1;
						continue;
					} else {
						if(user && *user) {
							r = yp_match(__ypdomain,
							    PASSWD_BYNAME,
							    user,
							    (int)strlen(user),
							    &__ypcurrent,
							    &__ypcurrentlen);
						} else
							goto pwnam_netgrp;
						if(r != 0) {
							if(__ypcurrent)
							    free(__ypcurrent);
							__ypcurrent = NULL;
							/*
							 * just because this
							 * user is bad, doesn't
							 * mean they all are.
							 */
							goto pwnam_netgrp;
						}
					}
					break;
				default:
					if(__ypcurrent) {
						free(__ypcurrent);
						__ypcurrent = NULL;
					}
					user = _pw_passwd.pw_name + 1;
					r = yp_match(__ypdomain,
						PASSWD_BYNAME,
						user, (int)strlen(user),
						&__ypcurrent,
						&__ypcurrentlen);
					if(r != 0) {
						if(__ypcurrent)
							free(__ypcurrent);
						__ypcurrent = NULL;
						continue;
					}
					break;
				}
				memmove(line, __ypcurrent,
				    (size_t)__ypcurrentlen);
				line[__ypcurrentlen] = '\0';
				if(__ypparse(&_pw_passwd, line)
				   || __ypexclude_is(_pw_passwd.pw_name)) {
					if(s == 1)	/* inside netgrp */
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
							__ypexclude_add(user);
					}
					endnetgrent();
					break;
				default:
					__ypexclude_add(_pw_passwd.pw_name + 1);
					break;
				}
				break;
			}
			if(strcmp(_pw_passwd.pw_name, name) == 0) {
				if (!_pw_stayopen) {
					(void)(_pw_db->close)(_pw_db);
					_pw_db = (DB *)NULL;
				}
				if(__ypexclude != (DB *)NULL) {
					(void)(__ypexclude->close)(__ypexclude);
					__ypexclude = (DB *)NULL;
				}
				__ypproto = (struct passwd *)NULL;
				return &_pw_passwd;
			}
			if(s == 1)	/* inside netgrp */
				goto pwnam_netgrp;
			continue;
		}
		if (!_pw_stayopen) {
			(void)(_pw_db->close)(_pw_db);
			_pw_db = (DB *)NULL;
		}
		if(__ypexclude != (DB *)NULL) {
			(void)(__ypexclude->close)(__ypexclude);
			__ypexclude = (DB *)NULL;
		}
		__ypproto = (struct passwd *)NULL;
		return (struct passwd *)NULL;
	}
#endif /* YP */

	bf[0] = _PW_KEYBYNAME;
	len = strlen(name);
	len = MIN(len, MAXLOGNAME);
	memmove(bf + 1, name, (size_t)len);
	key.data = (u_char *)bf;
	key.size = len + 1;
	rval = __hashpw(&key);

	if (!_pw_stayopen) {
		(void)(_pw_db->close)(_pw_db);
		_pw_db = (DB *)NULL;
	}
	return(rval ? &_pw_passwd : (struct passwd *)NULL);
}

struct passwd *
getpwuid(uid)
	uid_t uid;
{
	DBT key;
	char bf[sizeof(_pw_keynum) + 1];
	uid_t keyuid;
	int rval;

	if (!_pw_db && !__initdb())
		return((struct passwd *)NULL);

#ifdef YP
	/*
	 * If YP is active, we must sequence through the passwd file
	 * in sequence.
	 */
	if (__has_yppw()) {
		char uidbuf[20];
		int r;
		int s = -1;
		const char *host, *user, *dom;

		snprintf(uidbuf, sizeof(uidbuf), "%u", uid);
		for(_pw_keynum=1; _pw_keynum; _pw_keynum++) {
			bf[0] = _PW_KEYBYNUM;
			memmove(bf + 1, &_pw_keynum, sizeof(_pw_keynum));
			key.data = (u_char *)bf;
			key.size = sizeof(_pw_keynum) + 1;
			if(__hashpw(&key) == 0)
				break;
			switch(_pw_passwd.pw_name[0]) {
			case '+':
				if(!__ypdomain) {
					if(_yp_check(&__ypdomain) == 0) {
						continue;
					}
				}
				/* save the prototype */
				__ypproto_set();

				switch(_pw_passwd.pw_name[1]) {
				case '\0':
					if(__ypcurrent) {
						free(__ypcurrent);
						__ypcurrent = NULL;
					}
					r = yp_match(__ypdomain, PASSWD_BYUID,
						uidbuf, (int)strlen(uidbuf),
						&__ypcurrent, &__ypcurrentlen);
					if(r != 0) {
						if(__ypcurrent)
							free(__ypcurrent);
						__ypcurrent = NULL;
						continue;
					}
					break;
				case '@':
pwuid_netgrp:
					if(__ypcurrent) {
						free(__ypcurrent);
						__ypcurrent = NULL;
					}
					if(s == -1)	/* first time */
						setnetgrent(_pw_passwd.pw_name + 2);
					s = getnetgrent(&host, &user, &dom);
					if(s == 0) {	/* end of group */
						endnetgrent();
						s = -1;
						continue;
					} else {
						if(user && *user) {
							r = yp_match(__ypdomain,
							    PASSWD_BYNAME,
							    user,
							    (int)strlen(user),
							    &__ypcurrent,
							    &__ypcurrentlen);
						} else
							goto pwuid_netgrp;
						if(r != 0) {
							if(__ypcurrent)
							    free(__ypcurrent);
							__ypcurrent = NULL;
							/*
                                                         * just because this
							 * user is bad, doesn't
							 * mean they all are.
							 */
							goto pwuid_netgrp;
						}
					}
					break;
				default:
					if(__ypcurrent) {
						free(__ypcurrent);
						__ypcurrent = NULL;
					}
					user = _pw_passwd.pw_name + 1;
					r = yp_match(__ypdomain,
						PASSWD_BYNAME,
						user, (int)strlen(user),
						&__ypcurrent,
						&__ypcurrentlen);
					if(r != 0) {
						if(__ypcurrent)
							free(__ypcurrent);
						__ypcurrent = NULL;
						continue;
					}
					break;
				}
				memmove(line, __ypcurrent,
				    (size_t)__ypcurrentlen);
				line[__ypcurrentlen] = '\0';
				if(__ypparse(&_pw_passwd, line)
				   || __ypexclude_is(_pw_passwd.pw_name)) {
					if(s == 1)	/* inside netgroup */
						goto pwuid_netgrp;
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
							__ypexclude_add(user);
					}
					endnetgrent();
					break;
				default:
					__ypexclude_add(_pw_passwd.pw_name + 1);
					break;
				}
				break;
			}
			if( _pw_passwd.pw_uid == uid) {
				if (!_pw_stayopen) {
					(void)(_pw_db->close)(_pw_db);
					_pw_db = (DB *)NULL;
				}
				if (__ypexclude != (DB *)NULL) {
					(void)(__ypexclude->close)(__ypexclude);
					__ypexclude = (DB *)NULL;
				}
				__ypproto = NULL;
				return &_pw_passwd;
			}
			if(s == 1)	/* inside netgroup */
				goto pwuid_netgrp;
			continue;
		}
		if (!_pw_stayopen) {
			(void)(_pw_db->close)(_pw_db);
			_pw_db = (DB *)NULL;
		}
		if(__ypexclude != (DB *)NULL) {
			(void)(__ypexclude->close)(__ypexclude);
			__ypexclude = (DB *)NULL;
		}
		__ypproto = (struct passwd *)NULL;
		return (struct passwd *)NULL;
	}
#endif /* YP */

	bf[0] = _PW_KEYBYUID;
	keyuid = uid;
	memmove(bf + 1, &keyuid, sizeof(keyuid));
	key.data = (u_char *)bf;
	key.size = sizeof(keyuid) + 1;
	rval = __hashpw(&key);

	if (!_pw_stayopen) {
		(void)(_pw_db->close)(_pw_db);
		_pw_db = (DB *)NULL;
	}
	return(rval ? &_pw_passwd : (struct passwd *)NULL);
}

int
setpassent(stayopen)
	int stayopen;
{
	_pw_keynum = 0;
	_pw_stayopen = stayopen;
#ifdef YP
	__ypmode = YPMODE_NONE;
	if(__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
	if(__ypexclude != (DB *)NULL) {
		(void)(__ypexclude->close)(__ypexclude);
		__ypexclude = (DB *)NULL;
	}
	__ypproto = (struct passwd *)NULL;
#endif
	return(1);
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
#ifdef YP
	__ypmode = YPMODE_NONE;
	if(__ypcurrent)
		free(__ypcurrent);
	__ypcurrent = NULL;
	if(__ypexclude != (DB *)NULL) {
		(void)(__ypexclude->close)(__ypexclude);
		__ypexclude = (DB *)NULL;
	}
	__ypproto = (struct passwd *)NULL;
#endif
}

static int
__initdb()
{
	static int warned;
	char *p;

#ifdef YP
	__ypmode = YPMODE_NONE;
#endif
	if (geteuid() == 0) {
		_pw_db = dbopen((p = _PATH_SMP_DB), O_RDONLY, 0, DB_HASH, NULL);
		if (_pw_db)
			return(1);
	}
	_pw_db = dbopen((p = _PATH_MP_DB), O_RDONLY, 0, DB_HASH, NULL);
	if (_pw_db)
		return(1);
	if (!warned)
		syslog(LOG_ERR, "%s: %m", p);
	warned = 1;
	return(0);
}

static int
__hashpw(key)
	DBT *key;
{
	char *p, *t;
	static u_int max;
	static char *buf;
	DBT data;

	if ((_pw_db->get)(_pw_db, key, &data, 0))
		return(0);
	p = (char *)data.data;
	if (data.size > max && !(buf = realloc(buf, (max += 1024))))
		return(0);

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

	return(1);
}
