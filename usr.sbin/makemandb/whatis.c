/*	$NetBSD: whatis.c,v 1.4 2012/10/06 15:33:59 wiz Exp $	*/
/*-
 * Copyright (c) 2012 Joerg Sonnenberger <joerg@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: whatis.c,v 1.4 2012/10/06 15:33:59 wiz Exp $");

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "apropos-utils.h"

__dead static void
usage(void)
{
	fprintf(stderr, "%s ...\n", "whatis");
	exit(EXIT_FAILURE);
}

static int
whatis(sqlite3 *db, const char *cmd)
{
	static const char sqlstr[] = "SELECT name, section, name_desc"
				     " FROM mandb WHERE name MATCH ? AND name=?"
				     " ORDER BY section, name";
	sqlite3_stmt *stmt = NULL;
	int retval;

	if (sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL) != SQLITE_OK)
		errx(EXIT_FAILURE, "Unable to query database");
	if (sqlite3_bind_text(stmt, 1, cmd, -1, NULL) != SQLITE_OK)
		errx(EXIT_FAILURE, "Unable to query database");
	if (sqlite3_bind_text(stmt, 2, cmd, -1, NULL) != SQLITE_OK)
		errx(EXIT_FAILURE, "Unable to query database");
	retval = 1;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		printf("%s(%s) - %s\n", sqlite3_column_text(stmt, 0),
		    sqlite3_column_text(stmt, 1),
		    sqlite3_column_text(stmt, 2));
		retval = 0;
	}
	sqlite3_finalize(stmt);
	if (retval)
		fprintf(stderr, "%s: not found\n", cmd);
	return retval;
}

int
main(int argc, char *argv[])
{
	sqlite3 *db;
	int ch, retval;

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	if ((db = init_db(MANDB_READONLY, MANCONF)) == NULL)
		exit(EXIT_FAILURE);

	retval = 0;
	while (argc--)
		retval |= whatis(db, *argv++);

	close_db(db);
	return retval;
}
