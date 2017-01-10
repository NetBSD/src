/*	$NetBSD: ypdb.c,v 1.12 2017/01/10 21:06:17 christos Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * This code is derived from ndbm module of BSD4.4 db (hash) by
 * Mats O Jansson
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypdb.c,v 1.12 2017/01/10 21:06:17 christos Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpcsvc/yp.h>

#include "ypdb.h"

static DBM	*_ypdb_dbopen(const char *, int, mode_t);

/*
 * ypdb_open --
 *	dbopen(3) file, read-only.
 *	First ensure that file has a suffix of YPDB_SUFFIX.
 *	Try opening as a DB_BTREE first, then DB_HASH.
 *
 * Returns:
 *	*DBM on success
 *	 NULL on failure
 */

DBM *
ypdb_open(const char *file)
{
	char path[MAXPATHLEN];
	const char *cp, *suffix;

	cp = strrchr(file, '.');
	if (cp != NULL && strcmp(cp, YPDB_SUFFIX) == 0)
		suffix = "";
	else
		suffix = YPDB_SUFFIX;
	if (strlen(file) + strlen(suffix) > (sizeof(path) - 1)) {
		warnx("File name `%s' is too long", file);
		return (NULL);
	}
	snprintf(path, sizeof(path), "%s%s", file, suffix);
	return _ypdb_dbopen(path, O_RDONLY, 0444);
}

/*
 * ypdb_mktemp --
 *	Create a temporary file using mkstemp(3) based on the
 *	template provided in file.
 *	dbopen(3) file, read-write, 0644 (modified by umask(2)).
 *	Try opening as a DB_BTREE first, then DB_HASH.
 *	file won't have YPDB_SUFFIX.
 *
 * Returns:
 *	*DBM on success; file now exists.
 *	 NULL on failure
 */

DBM *
ypdb_mktemp(char *file)
{
	int fd = -1;
	DBM *db = NULL;
	mode_t myumask;
	int save_errno;

	if ((fd = mkstemp(file)) == -1)
		return NULL;

	myumask = umask(0);
	(void)umask(myumask);
	if (fchmod(fd, 0644 & ~myumask) == -1)
		goto bad;

	(void) close(fd);
	fd = -1;

	if ((db = _ypdb_dbopen(file, O_RDWR, 0644)) == NULL)
		goto bad;

	return db;

 bad:
	save_errno = errno;
	if (fd != 1)
		(void) close(fd);
	(void) unlink(file);
	errno = save_errno;
	return NULL;
}

/*
 * _ypdb_dbopen --
 *	dbopen(3) path with the flags & mode.
 *	Try opening as a DB_BTREE first, then DB_HASH.
 */

static DBM *
_ypdb_dbopen(const char *path, int flags, mode_t mode)
{
	DBM *db;
	BTREEINFO info;

		/* try our btree format first */
	info.flags = 0;
	info.cachesize = 0;
	info.maxkeypage = 0;
	info.minkeypage = 0;
	info.psize = 0;
	info.compare = NULL;
	info.prefix = NULL;
	info.lorder = 0;
	db = (DBM *)dbopen(path, flags, mode, DB_BTREE, (void *)&info);
	if (db != NULL || errno != EFTYPE)
		return (db);

		/* fallback to standard hash (for sendmail's aliases.db) */
	db = (DBM *)dbopen(path, flags, mode, DB_HASH, NULL);
	return (db);
}

/*
 * ypdb_close --
 *	Close the db
 */

void
ypdb_close(DBM *db)
{
	(void)(db->close)(db);
}

/*
 * Returns:
 *	DATUM on success
 *	NULL on failure
 */

datum
ypdb_fetch(DBM *db, datum key)
{
	datum retkey;
	DBT nk, nd;
	int status;

	nk.data = key.dptr;
	nk.size = key.dsize;
	status = (db->get)(db, &nk, &nd, 0);
	if (status) {
		retkey.dptr = NULL;
		retkey.dsize = 0;
	} else {
		retkey.dptr = nd.data;
		retkey.dsize = nd.size;
	}
	return (retkey);
}

/*
 * Returns:
 *	DATUM on success
 *	NULL on failure
 */

datum
ypdb_firstkey(DBM *db)
{
	int status;
	datum retkey;
	DBT nk, nd;

	status = (db->seq)(db, &nk, &nd, R_FIRST);
	if (status) {
		retkey.dptr = NULL;
		retkey.dsize = 0;
	} else {
		retkey.dptr = nk.data;
		retkey.dsize = nk.size;
	}
	return (retkey);
}

/*
 * Returns:
 *	DATUM on success
 *	NULL on failure
 */

datum
ypdb_nextkey(DBM *db)
{
	int status;
	datum retkey;
	DBT nk, nd;

	status = (db->seq)(db, &nk, &nd, R_NEXT);
	if (status) {
		retkey.dptr = NULL;
		retkey.dsize = 0;
	} else {
		retkey.dptr = nk.data;
		retkey.dsize = nk.size;
	}
	return (retkey);
}

/*
 * Returns:
 *	DATUM on success
 *	NULL on failure
 */

datum
ypdb_setkey(DBM *db, datum key)
{
	int status;
	DBT nk, nd;

	nk.data = key.dptr;
	nk.size = key.dsize;
	status = (db->seq)(db, &nk, &nd, R_CURSOR);
	if (status) {
		key.dptr = NULL;
		key.dsize = 0;
	}
	return (key);
}

/*
 * Returns:
 *	 0 on success
 *	<0 failure
 */

int
ypdb_delete(DBM *db, datum key)
{
	int status;
	DBT nk;

	nk.data = key.dptr;
	nk.size = key.dsize;
	status = (db->del)(db, &nk, 0);
	if (status)
		return (-1);
	else
		return (0);
}

/*
 * Returns:
 *	 0 on success
 *	<0 failure
 *	 1 if YPDB_INSERT and entry exists
 */

int
ypdb_store(DBM *db, datum key, datum content, int flags)
{
	DBT nk, nd;

	if (key.dsize > YPMAXRECORD || content.dsize > YPMAXRECORD)
		return -1;
	nk.data = key.dptr;
	nk.size = key.dsize;
	nd.data = content.dptr;
	nd.size = content.dsize;
	return ((db->put)(db, &nk, &nd,
	    (flags == YPDB_INSERT) ? R_NOOVERWRITE : 0));
}
