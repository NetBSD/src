/*	$NetBSD: services_mkdb.c,v 1.3 2006/07/27 22:13:38 christos Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
__RCSID("$NetBSD: services_mkdb.c,v 1.3 2006/07/27 22:13:38 christos Exp $");
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
static void	usage(void) __attribute__((__noreturn__));

int
main(int argc, char *argv[])
{
	DB	*db;
	FILE	*fp;
	int	 ch;
	size_t	 len, line;
	char	 keyb[BUFSIZ], datab[BUFSIZ], *p;
	const char *fname = _PATH_SERVICES;
	DBT	 data, key;
	const char *dbname = _PATH_SERVICES_DB;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "o:")) != -1)
		switch (ch) {
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
	    (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), DB_HASH, NULL);
	if (!db)
		err(1, "Error opening temporary database `%s'", tname);

	key.data = (u_char *)(void *)keyb;
	data.data = (u_char *)(void *)datab;

	line = 0;
	/* XXX: change NULL to "\0\0#" when fparseln fixed */
	for (; (p = fparseln(fp, &len, &line, NULL, 0)) != NULL; free(p)) {
		char	*name, *port, *proto, *aliases, *cp, *alias;
		StringList *sl;
		size_t i;

		if (len == 0)
			continue;
		cp = p;
		while ((name = strsep(&cp, " \t")) != NULL && *name == '\0')
			continue;
		if (name == NULL) {
			warnx("%s, %zu: no name found", fname, line);
			continue;
		}
		while ((port = strsep(&cp, " \t")) != NULL && *port == '\0')
			continue;
		if (port == NULL) {
			warnx("%s, %zu: no port found", fname, line);
			continue;
		}
		proto = strchr(port, '/');
		if (proto == NULL || proto[1] == '\0') {
			warnx("%s, %zu: no protocol found", fname, line);
			continue;
		}
		*proto++ = '\0';
		for (aliases = cp; cp && isspace((unsigned char)*aliases);
		    aliases++)
			continue;

		if ((aliases = strdup(aliases ? aliases : "")) == NULL)
			err(1, "Cannot copy string");

		/* key `port/proto', data = full line */
		key.size = snprintf(keyb, sizeof(keyb), "%s/%s",
		    port, proto) + 1;
		data.size = snprintf(datab, sizeof(datab), "%s %s/%s %s",
		    name, port, proto, aliases) + 1;
		switch ((db->put)(db, &key, &data, R_NOOVERWRITE)) {
		case 0:
			break;
		case 1:
			warnx("%s, %zu: duplicate service `%s'",
			    fname, line, keyb);
			break;
		case -1:
			err(1, "put");
			break;
		default:
			abort();
			break;
		}

		/* key `\376port', data = full line */
		key.size = snprintf(keyb, sizeof(keyb), "\376%s", port) + 1;
		if ((db->put)(db, &key, &data, R_NOOVERWRITE) == -1)
			err(1, "put");


		/* build list of aliases */
		sl = sl_init();
		sl_add(sl, name);
		while ((alias = strsep(&cp, " \t")) != NULL) {
			if (alias[0] == '\0')
				continue;
			sl_add(sl, alias);
		}

		/* add references for service and all aliases */
		for (i = 0; i < sl->sl_cur; i++) {
			/* key `\377service/proto', data = `full line' */
			key.size = snprintf(keyb, sizeof(keyb), "\377%s/%s",
			    sl->sl_str[i], proto) + 1;
			data.size = snprintf(datab, sizeof(datab),
			    "%s %s/%s %s",
			    name, port, proto, aliases) + 1;
			switch ((db->put)(db, &key, &data, R_NOOVERWRITE)) {
			case 0:
				break;
			case 1:
				warnx("%s, %zu: duplicate service `%s'",
				    fname, line, keyb);
				break;
			case -1:
				err(1, "put");
				break;
			default:
				abort();
				break;
			}

			/* key `\377service', data = `full line' */
			key.size = snprintf(keyb, sizeof(keyb), "\377%s",
			    sl->sl_str[i]) + 1;
			data.size = snprintf(datab, sizeof(datab),
			    "%s %s/%s %s",
			    name, port, proto, aliases) + 1;
			
			if ((db->put)(db, &key, &data, R_NOOVERWRITE) == -1)
				err(1, "put");
		}
		sl_free(sl, 0);
		free(aliases);
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

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-o <db>] [<servicefile>]\n",
	    getprogname());
	exit(1);
}
