/*	$NetBSD: pkgdb.c,v 1.7.2.3 2003/03/15 20:12:23 he Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pkgdb.c,v 1.7.2.3 2003/03/15 20:12:23 he Exp $");
#endif

/*
 * Copyright (c) 1999 Hubert Feyrer.  All rights reserved.
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
 *      This product includes software developed by Hubert Feyrer for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>

#include "lib.h"

#define PKGDB_FILE	"pkgdb.byfile.db"	/* indexed by filename */

static DB *pkgdbp;
static int pkgdb_iter_flag;

/*
 *  Open the pkg-database
 *  Return value:
 *   0: everything ok
 *  -1: error, see errno
 */
int
pkgdb_open(int mode)
{
	BTREEINFO info;
	char	cachename[FILENAME_MAX];

	pkgdb_iter_flag = 0;	/* used in pkgdb_iter() */

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

	if (keyd.size > FILENAME_MAX || vald.size > FILENAME_MAX)
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
	if (keyd.size > FILENAME_MAX)
		return -1;

	return (*pkgdbp->del) (pkgdbp, &keyd, 0);
}

/* remove any entry from the cache which has a data field of `pkg' */
int
pkgdb_remove_pkg(const char *pkg)
{
	DBT     data;
	DBT     key;
	int	type;
	int	ret;
	int	cc;

	if (pkgdbp == NULL) {
		return 0;
	}
	cc = strlen(pkg);
	for (ret = 1, type = R_FIRST; (*pkgdbp->seq)(pkgdbp, &key, &data, type) == 0 ; type = R_NEXT) {
		if (cc == data.size && strncmp(data.data, pkg, cc) == 0) {
			if (Verbose) {
				printf("Removing file %s from pkgdb\n", (char *)key.data);
			}
			switch ((*pkgdbp->del)(pkgdbp, &key, 0)) {
			case -1:
				warn("Error removing %s from pkgdb", (char *) key.data);
				ret = 0;
				break;
			case 1:
				warn("Key %s not present in pkgdb", (char *) key.data);
				ret = 0;
				break;

			}
		}
	}
	return ret;
}

/*
 *  Iterate all pkgdb keys (which can then be handled to pkgdb_retrieve())
 *  Return value:
 *    NULL if no more data is available
 *   !NULL else
 */
char   *
pkgdb_iter(void)
{
	DBT     key, val;
	int	type;

	if (pkgdb_iter_flag == 0) {
		pkgdb_iter_flag = 1;
		type = R_FIRST;
	} else
		type = R_NEXT;

	if ((*pkgdbp->seq)(pkgdbp, &key, &val, type) != 0) {
		key.data = NULL;
	}

	return (char *) key.data;
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
char *
_pkgdb_getPKGDB_DIR(void)
{
	char   *tmp;
	static char *cache = NULL;

	if (cache == NULL)
		cache = (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR;

	return cache;
}
