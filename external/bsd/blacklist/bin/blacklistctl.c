/*	$NetBSD: blacklistctl.c,v 1.9 2015/01/22 15:29:27 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: blacklistctl.c,v 1.9 2015/01/22 15:29:27 christos Exp $");

#include <stdio.h>
#include <time.h>
#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#include <fcntl.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "conf.h"
#include "state.h"
#include "internal.h"
#include "support.h"

static __dead void
usage(int c)
{
	warnx("Unknown option `%c'", (char)c);
	fprintf(stderr, "Usage: %s [-d]\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	const char *dbname = _PATH_BLSTATE;
	DB *db;
	struct conf c;
	struct sockaddr_storage ss;
	struct dbinfo dbi;
	unsigned int i;
	int o;

	while ((o = getopt(argc, argv, "d")) != -1)
		switch (o) {
		case 'd':
			debug++;
			break;
		default:
			usage(o);
			break;
		}

	db = state_open(dbname, O_RDONLY, 0);
	if (db == NULL)
		err(EXIT_FAILURE, "Can't open `%s'", dbname);

	for (i = 1; state_iterate(db, &ss, &c, &dbi, i) != 0; i = 0) {
		char buf[BUFSIZ];
		(*lfun)(LOG_DEBUG, "conf: %s\n", conf_print(buf, sizeof(buf), "",
		    ":", &c));
		sockaddr_snprintf(buf, sizeof(buf), "%a", (void *)&ss);
		(*lfun)(LOG_DEBUG, "addr: %s\n", buf);
		(*lfun)(LOG_DEBUG, "data: count=%d id=%s time=%s\n", dbi.count,
		    dbi.id, fmttime(buf, sizeof(buf), dbi.last));
	}
	state_close(db);
	return EXIT_SUCCESS;
}
