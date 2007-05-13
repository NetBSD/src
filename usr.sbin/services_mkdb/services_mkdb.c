/*	$NetBSD: services_mkdb.c,v 1.8 2007/05/13 17:43:59 christos Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn and Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: services_mkdb.c,v 1.8 2007/05/13 17:43:59 christos Exp $");
#endif /* not lint */


#include <sys/param.h>

#include <db.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <ctype.h>
#include <stringlist.h>

static char tname[MAXPATHLEN];

static void	cleanup(void);
static void	store(const char *, size_t, DB *, DBT *, DBT *, int);
static void	killproto(DBT *);
static char    *getstring(const char *, size_t, char **, const char *);
static void	usage(void) __attribute__((__noreturn__));

static const HASHINFO hinfo = {
	.bsize = 256,
	.ffactor = 4,
	.nelem = 32768,
	.cachesize = 1024,
	.hash = NULL,
	.lorder = 0
};


int
main(int argc, char *argv[])
{
	DB	*db;
	FILE	*fp;
	int	 ch;
	size_t	 len, line, cnt;
	char	 keyb[BUFSIZ], datab[BUFSIZ], *p;
	DBT	 data, key;
	const char *fname = _PATH_SERVICES;
	const char *dbname = _PATH_SERVICES_DB;
	int	 warndup = 1;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "qo:")) != -1)
		switch (ch) {
		case 'q':
			warndup = 0;
			break;
		case 'o':
			dbname = optarg;
			break;
		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();
	if (argc == 1)
		fname = argv[0];

	if ((fp = fopen(fname, "r")) == NULL)
		err(1, "Cannot open `%s'", fname);

	if (atexit(cleanup))
		err(1, "Cannot install exit handler");

	(void)snprintf(tname, sizeof(tname), "%s.tmp", dbname);
	db = dbopen(tname, O_RDWR | O_CREAT | O_EXCL,
	    (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), DB_HASH, &hinfo);
	if (!db)
		err(1, "Error opening temporary database `%s'", tname);

	key.data = keyb;
	data.data = datab;

	cnt = line = 0;
	/* XXX: change NULL to "\0\0#" when fparseln fixed */
	for (; (p = fparseln(fp, &len, &line, NULL, 0)) != NULL; free(p)) {
		char	*name, *port, *proto, *aliases, *cp, *alias;
		StringList *sl;
		size_t i;

		if (len == 0)
			continue;
		for (cp = p; *cp && isspace((unsigned char)*cp); cp++)
			continue;
		if (*cp == '\0' || *cp == '#')
			continue;

		if ((name = getstring(fname, line, &cp, "name")) == NULL)
			continue;

		if ((port = getstring(fname, line, &cp, "port")) == NULL)
			continue;

		proto = strchr(port, '/');
		if (proto == NULL || proto[1] == '\0') {
			warnx("%s, %zu: no protocol found", fname, line);
			continue;
		}
		*proto++ = '\0';

		for (aliases = cp; cp && isspace((unsigned char)*aliases);
		    aliases++)
			continue;

		/* key `indirect key', data `full line' */
		data.size = snprintf(datab, sizeof(datab), "%zu", cnt++) + 1;
		key.size = snprintf(keyb, sizeof(keyb), "%s %s/%s %s",
		    name, port, proto, aliases ? aliases : "") + 1;
		store(fname, line, db, &data, &key, warndup);

		/* key `\377port/proto', data = `indirect key' */
		key.size = snprintf(keyb, sizeof(keyb), "\377%s/%s",
		    port, proto) + 1;
		store(fname, line, db, &key, &data, warndup);

		/* key `\377port', data = `indirect key' */
		killproto(&key);
		store(fname, line, db, &key, &data, 0);

		/* build list of aliases */
		sl = sl_init();
		(void)sl_add(sl, name);
		while ((alias = strsep(&cp, " \t")) != NULL) {
			if (alias[0] == '\0')
				continue;
			(void)sl_add(sl, alias);
		}

		/* add references for service and all aliases */
		for (i = 0; i < sl->sl_cur; i++) {
			/* key `\376service/proto', data = `indirect key' */
			key.size = snprintf(keyb, sizeof(keyb), "\376%s/%s",
			    sl->sl_str[i], proto) + 1;
			store(fname, line, db, &key, &data, warndup);

			/* key `\376service', data = `indirect key' */
			killproto(&key);
			store(fname, line, db, &key, &data, 0);
		}
		sl_free(sl, 0);
	}

	if ((db->close)(db))
		err(1, "Error closing temporary database `%s'", tname);

	if (rename(tname, dbname) == -1)
		err(1, "Cannot rename `%s' to `%s'", tname, dbname);

	return 0;
}

/*
 * cleanup(): Remove temporary files upon exit
 */
static void
cleanup(void)
{
	if (tname[0])
		(void)unlink(tname);
}

static char *
getstring(const char *fname, size_t line, char **cp, const char *tag)
{
	char *str;

	while ((str = strsep(cp, " \t")) != NULL && *str == '\0')
		continue;

	if (str == NULL)
		warnx("%s, %zu: no %s found", fname, line, tag);

	return str;
}

static void
killproto(DBT *key)
{
	char *p, *d = key->data;

	if ((p = strchr(d, '/')) == NULL)
		abort();
	*p++ = '\0';
	key->size = p - d;
}

static void
store(const char *fname, size_t line, DB *db, DBT *key, DBT *data, int warndup)
{
	switch ((db->put)(db, key, data, R_NOOVERWRITE)) {
	case 0:
		break;
	case 1:
		if (warndup)
			warnx("%s, %zu: duplicate service `%s'",
			    fname, line, &((char *)key->data)[1]);
		break;
	case -1:
		err(1, "put");
		break;
	default:
		abort();
		break;
	}
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-q] [-o <db>] [<servicefile>]\n",
	    getprogname());
	exit(1);
}
