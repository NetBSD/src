/*	$NetBSD: services_mkdb.c,v 1.1 1999/02/08 03:57:05 lukem Exp $	*/

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
__RCSID("$NetBSD: services_mkdb.c,v 1.1 1999/02/08 03:57:05 lukem Exp $");
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

#ifndef _PATH_SERVICES_DB
#define _PATH_SERVICES_DB "/etc/services.db";
#endif

static	char	*dbname = _PATH_SERVICES_DB;

void	cleanup __P((void));
int	main __P((int, char *[]));
void	usage __P((void));

int
main(argc, argv)
	int	 argc;
	char	*argv[];
{
	DB	*db;
	FILE	*fp;
	int	 ch, len, line;
	char	 tname[MAXPATHLEN], *p;
	char	 keyb[BUFSIZ], datab[BUFSIZ];
	char	*fname = _PATH_SERVICES;
	DBT	 data, key;

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

	if (argc == 1)
		fname = argv[0];
	else if (argc > 1)
		usage();
	if ((fp = fopen(fname, "r")) == NULL)
		err(1, "Cannot open `%s'", fname);
	if (atexit(cleanup))
		err(1, "Cannot install exit handler");
	(void) snprintf(tname, sizeof(tname), "%s.tmp", dbname);
	db = dbopen(tname, O_RDWR | O_CREAT | O_EXCL,
		    (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), DB_HASH, NULL);
	if (!db)
		err(1, "Error opening temporary database `%s'", tname);

	key.data = (u_char *)keyb;
	data.data = (u_char *)datab;

			/* XXX: change NULL to "\0\0#" when fparseln fixed */
	for (; (p = fparseln(fp, &len, &line, NULL, 0)) != NULL; free(p)) {
		char	*name, *port, *proto, *aliases, *cp;
#define MAXALIASES 35
		char	*aliasv[MAXALIASES];
		int	 aliasc, i;

		if (len == 0)
			continue;
		cp = p;
		while ((name = strsep(&cp, " \t")) != NULL && *name == '\0')
			;
		if (name == NULL) {
			warnx("%s line %d: no name found", fname, line);
			continue;
		}
		while ((port = strsep(&cp, " \t")) != NULL && *port == '\0')
			;
		if (port == NULL) {
			warnx("%s line %d: no port found", fname, line);
			continue;
		}
		proto = strchr(port, '/');
		if (proto == NULL || proto[1] == '\0') {
			warnx("%s line %d: no protocol found", fname, line);
			continue;
		}
		*proto++ = '\0';
		while ((aliases = strsep(&cp, " \t")) != NULL &&
		    *aliases == '\0')
			;

				/* key `port/proto', data = full line */
		key.size = snprintf(keyb, sizeof(keyb), "%s/%s",
		    port, proto) + 1;
		data.size = snprintf(datab, sizeof(datab), "%s %s/%s %s",
		    name, port, proto, aliases == NULL ? "" : aliases) + 1;
		switch ((db->put)(db, &key, &data, R_NOOVERWRITE)) {
		case 0:
			break;
		case 1:
			warnx("%s line %d: duplicate service `%s'",
			    fname, line, keyb);
			break;
		case -1:
			err(1, "put");
			break;
		default:
			abort();
			break;
		}

				/* key `port/.', data = full line */
		key.size = snprintf(keyb, sizeof(keyb), "%s/.", port) + 1;
		if ((db->put)(db, &key, &data, R_NOOVERWRITE) == -1)
			err(1, "put");


				/* build list of aliases */
		cp = aliases;
		aliasv[0] = name;
		for (aliasc = 1;
		    (aliasv[aliasc] = strsep(&cp, " \t")) != NULL;) {
			if (aliasv[aliasc][0] == '\0')
				continue;
			if (++aliasc > MAXALIASES)
				break;
		}
		aliasv[aliasc] = NULL;

				/* add references for service and all aliases */
		for (i = 0; i < aliasc; i++) {

				/* key `service/proto', data = `.port/proto' */
			key.size = snprintf(keyb, sizeof(keyb), "%s/%s",
			    aliasv[i], proto) + 1;
			data.size = snprintf(datab, sizeof(datab), ".%s/%s",
			    port, proto) + 1;
			switch ((db->put)(db, &key, &data, R_NOOVERWRITE)) {
			case 0:
				break;
			case 1:
				warnx("%s line %d: duplicate service `%s'",
				    fname, line, keyb);
				break;
			case -1:
				err(1, "put");
				break;
			default:
				abort();
				break;
			}

					/* key `service/.', data = `.port/.' */
			key.size = snprintf(keyb, sizeof(keyb), "%s/.",
			    aliasv[i]) + 1;
			data.size = snprintf(datab, sizeof(datab), ".%s/.",
			    port) + 1;
			if ((db->put)(db, &key, &data, R_NOOVERWRITE) == -1)
				err(1, "put");
		}
	}

	if ((db->close)(db))
		err(1, "Error closing temporary database `%s'", tname);
	if (rename(tname, dbname) == -1)
		err(1, "Cannot rename `%s' to `%s'", tname, dbname);

	return (0);
}

/*
 * cleanup(): Remove temporary files upon exit
 */
void
cleanup()
{
	char tname[MAXPATHLEN];
	(void) snprintf(tname, sizeof(tname), "%s.tmp", dbname);
	(void) unlink(tname);
}

void
usage()
{
	extern const char *__progname;

	fprintf(stderr, "usage: %s [-o db]\n", __progname);
	exit(1);
}
