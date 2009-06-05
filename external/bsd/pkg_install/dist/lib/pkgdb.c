/*	$NetBSD: pkgdb.c,v 1.1.1.3.4.2 2009/06/05 17:19:41 snj Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
__RCSID("$NetBSD: pkgdb.c,v 1.1.1.3.4.2 2009/06/05 17:19:41 snj Exp $");

/*-
 * Copyright (c) 1999-2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Hubert Feyrer <hubert@feyrer.de>.
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

#if HAVE_DB_185_H
#include <db_185.h>
#elif HAVE_DB1_DB_H
#include <db1/db.h>
#elif HAVE_DB_H
#include <db.h>
#else
#include <nbcompat/db.h>
#endif
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#include "lib.h"

#define PKGDB_FILE	"pkgdb.byfile.db"	/* indexed by filename */

/*
 * Where we put logging information by default if PKG_DBDIR is unset.
 */
#ifndef DEF_LOG_DIR
#define DEF_LOG_DIR		"/var/db/pkg"
#endif

/* just in case we change the environment variable name */
#define PKG_DBDIR		"PKG_DBDIR"

static DB   *pkgdbp;
static char *pkgdb_dir = NULL;
static char  pkgdb_cache[MaxPathSize];

/*
 *  Open the pkg-database
 *  Return value:
 *   1: everything ok
 *   0: error
 */
int
pkgdb_open(int mode)
{
	BTREEINFO info;
	char	cachename[MaxPathSize];

	/* try our btree format first */
	info.flags = 0;
	info.cachesize = 2*1024*1024;
	info.maxkeypage = 0;
	info.minkeypage = 0;
	info.psize = 4096;
	info.compare = NULL;
	info.prefix = NULL;
	info.lorder = 0;
	pkgdbp = (DB *) dbopen(_pkgdb_getPKGDB_FILE(cachename, sizeof(cachename)),
	    (mode == ReadOnly) ? O_RDONLY : O_RDWR | O_CREAT,
	    0644, DB_BTREE, (void *) &info);
	return (pkgdbp != NULL);
}

/*
 * Close the pkg database
 */
void
pkgdb_close(void)
{
	if (pkgdbp != NULL) {
		(void) (*pkgdbp->close) (pkgdbp);
		pkgdbp = NULL;
	}
}

/*
 * Store value "val" with key "key" in database
 * Return value is as from ypdb_store:
 *  0: ok
 *  1: key already present
 * -1: some other error, see errno
 */
int
pkgdb_store(const char *key, const char *val)
{
	DBT     keyd, vald;

	if (pkgdbp == NULL)
		return -1;

	keyd.data = (void *) key;
	keyd.size = strlen(key) + 1;
	vald.data = (void *) val;
	vald.size = strlen(val) + 1;

	if (keyd.size > MaxPathSize || vald.size > MaxPathSize)
		return -1;

	return (*pkgdbp->put) (pkgdbp, &keyd, &vald, R_NOOVERWRITE);
}

/*
 * Recall value for given key
 * Return value:
 *  NULL if some error occurred or value for key not found (check errno!)
 *  String for "value" else
 */
char   *
pkgdb_retrieve(const char *key)
{
	DBT     keyd, vald;
	int     status;

	if (pkgdbp == NULL)
		return NULL;

	keyd.data = (void *) key;
	keyd.size = strlen(key) + 1;
	errno = 0;		/* to be sure it's 0 if the key doesn't match anything */

	vald.data = (void *)NULL;
	vald.size = 0;
	status = (*pkgdbp->get) (pkgdbp, &keyd, &vald, 0);
	if (status) {
		vald.data = NULL;
		vald.size = 0;
	}

	return vald.data;
}

/* dump contents of the database to stdout */
int
pkgdb_dump(void)
{
	DBT     key;
	DBT	val;
	int	type;

	if (pkgdb_open(ReadOnly)) {
		for (type = R_FIRST ; (*pkgdbp->seq)(pkgdbp, &key, &val, type) == 0 ; type = R_NEXT) {
			printf("file: %.*s pkg: %.*s\n",
				(int) key.size, (char *) key.data,
				(int) val.size, (char *) val.data);
		}
		pkgdb_close();
		return 0;
	} else
		return -1;
}

/*
 *  Remove data set from pkgdb
 *  Return value as ypdb_delete:
 *   0: everything ok
 *   1: key not present
 *  -1: some error occurred (see errno)
 */
int
pkgdb_remove(const char *key)
{
	DBT     keyd;

	if (pkgdbp == NULL)
		return -1;

	keyd.data = (char *) key;
	keyd.size = strlen(key) + 1;
	if (keyd.size > MaxPathSize)
		return -1;

	return (*pkgdbp->del) (pkgdbp, &keyd, 0);
}

/*
 *  Remove any entry from the cache which has a data field of `pkg'.
 *  Return value:
 *   1: everything ok
 *   0: error
 */
int
pkgdb_remove_pkg(const char *pkg)
{
	DBT     data;
	DBT     key;
	int	type;
	int	ret;
	int	cc;
	char	cachename[MaxPathSize];

	if (pkgdbp == NULL) {
		return 0;
	}
	(void) _pkgdb_getPKGDB_FILE(cachename, sizeof(cachename));
	cc = strlen(pkg);
	for (ret = 1, type = R_FIRST; (*pkgdbp->seq)(pkgdbp, &key, &data, type) == 0 ; type = R_NEXT) {
		if ((cc + 1) == data.size && strncmp(data.data, pkg, cc) == 0) {
			if (Verbose) {
				printf("Removing file `%s' from %s\n", (char *)key.data, cachename);
			}
			switch ((*pkgdbp->del)(pkgdbp, &key, 0)) {
			case -1:
				warn("Error removing `%s' from %s", (char *)key.data, cachename);
				ret = 0;
				break;
			case 1:
				warn("Key `%s' not present in %s", (char *)key.data, cachename);
				ret = 0;
				break;

			}
		}
	}
	return ret;
}

/*
 *  Return the location of the package reference counts database directory.
 */
char *
pkgdb_refcount_dir(void)
{
	static char buf[MaxPathSize];
	char *tmp;

	if ((tmp = getenv(PKG_REFCOUNT_DBDIR_VNAME)))
		strlcpy(buf, tmp, sizeof(buf));
	else
		snprintf(buf, sizeof(buf), "%s.refcount", _pkgdb_getPKGDB_DIR());
	return buf;
}

/*
 *  Return name of cache file in the buffer that was passed.
 */
char *
_pkgdb_getPKGDB_FILE(char *buf, unsigned size)
{
	(void) snprintf(buf, size, "%s/%s", _pkgdb_getPKGDB_DIR(), PKGDB_FILE);
	return buf;
}

/*
 *  Return directory where pkgdb is stored
 */
const char *
_pkgdb_getPKGDB_DIR(void)
{
	char *tmp;

	if (pkgdb_dir == NULL) {
		if ((tmp = getenv(PKG_DBDIR)))
			_pkgdb_setPKGDB_DIR(tmp);
		else
			_pkgdb_setPKGDB_DIR(DEF_LOG_DIR);
	}

	return pkgdb_dir;
}

/*
 *  Set the first place we look for where pkgdb is stored.
 */
void
_pkgdb_setPKGDB_DIR(const char *dir)
{
	(void) snprintf(pkgdb_cache, sizeof(pkgdb_cache), "%s", dir);
	pkgdb_dir = pkgdb_cache;
}

char *
pkgdb_pkg_file(const char *pkg, const char *file)
{
	return xasprintf("%s/%s/%s", _pkgdb_getPKGDB_DIR(), pkg, file);
}
