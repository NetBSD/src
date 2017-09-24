/*	$NetBSD: cvslatest.c,v 1.6 2017/09/24 09:43:27 joerg Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
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

#ifdef __linux__
#define _GNU_SOURCE
#endif

#ifdef HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: cvslatest.c,v 1.6 2017/09/24 09:43:27 joerg Exp $");

/*
 * Find the latest timestamp in a set of CVS trees, by examining the
 * Entries files
 */

#include <sys/param.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fts.h>
#include <time.h>

static int debug = 0;
static int ignore = 0;

struct latest {
	time_t time;
	char path[MAXPATHLEN];
};

static void
printlat(const struct latest *lat)
{
	fprintf(stderr, "%s %s", lat->path, ctime(&lat->time));
}

static void
getrepo(const FTSENT *e, char *repo, size_t maxrepo)
{
	FILE *fp;
	char name[MAXPATHLEN], ename[MAXPATHLEN];
	char *ptr;

	snprintf(name, sizeof(name), "%s/Repository", e->fts_accpath);
	snprintf(ename, sizeof(ename), "%s/Repository", e->fts_path);
	if ((fp = fopen(name, "r")) == NULL)
		err(EXIT_FAILURE, "Can't open `%s'", ename);
	if (fgets(repo, (int)maxrepo, fp) == NULL)
		err(EXIT_FAILURE, "Can't read `%s'", ename);
	if ((ptr = strchr(repo, '\n')) == NULL)
		errx(EXIT_FAILURE, "Malformed line in `%s'", ename);
	*ptr = '\0';
	fclose(fp);
}

static void
getlatest(const FTSENT *e, const char *repo, struct latest *lat)
{
	static const char fmt[] = "%a %b %d %H:%M:%S %Y";
	char name[MAXPATHLEN], ename[MAXPATHLEN];
	char entry[MAXPATHLEN * 2];
	char *fn, *dt, *p;
	time_t t;
	struct tm tm;
	FILE *fp;

	snprintf(name, sizeof(name), "%s/Entries", e->fts_accpath);
	snprintf(ename, sizeof(ename), "%s/Entries", e->fts_path);
	if ((fp = fopen(name, "r")) == NULL)
		err(EXIT_FAILURE, "Can't open `%s'", ename);

	while (fgets(entry, (int)sizeof(entry), fp) != NULL) {
		if (entry[0] != '/')
		    continue;
		if ((fn = strtok(entry, "/")) == NULL)
			goto mal;
		if (strtok(NULL, "/") == NULL)
			goto mal;
		if ((dt = strtok(NULL, "/")) == NULL)
			goto mal;
		if ((p = strptime(dt, fmt, &tm)) == NULL || *p) {
			warnx("Malformed time `%s' in `%s'", dt, ename);
			if (!ignore)
				exit(EXIT_FAILURE);
		}
		tm.tm_isdst = 0;	// We are in GMT anyway
		if ((t = mktime(&tm)) == (time_t)-1)
			errx(EXIT_FAILURE, "Time conversion `%s' in `%s'",
			    dt, ename);
		if (lat->time == 0 || lat->time < t) {
			lat->time = t;
			snprintf(lat->path, sizeof(lat->path),
			    "%s/%s", repo, fn);
			if (debug > 1)
				printlat(lat);
		}
	}

	fclose(fp);
	return;
			
mal:
	errx(EXIT_FAILURE, "Malformed line in `%s'", ename);
}

static void
cvsscan(char **pathv, const char *name, struct latest *lat)
{
        FTS *dh;
	char repo[MAXPATHLEN];
        FTSENT *entry;

	lat->time = 0;

        dh = fts_open(pathv, FTS_PHYSICAL, NULL);
        if (dh == NULL)
		err(EXIT_FAILURE, "fts_open `%s'", pathv[0]);

        while ((entry = fts_read(dh)) != NULL) {
                if (entry->fts_info != FTS_D)
			continue;

		if (strcmp(entry->fts_name, name) != 0)
                        continue;

		getrepo(entry, repo, sizeof(repo));
		getlatest(entry, repo, lat);
        }

        (void)fts_close(dh);
}

static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s [-di] [-n <name>] <path> ...\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct latest lat;
	const char *name = "CVS";
	int c;

	while ((c = getopt(argc, argv, "din:")) != -1)
		switch (c) {
		case 'i':
			ignore++;
			break;
		case 'd':
			debug++;
			break;
		case 'n':
			name = optarg;
			break;
		default:
			usage();
		}

	if (argc == optind)
		usage();

	// So that mktime behaves consistently
	setenv("TZ", "UTC", 1);

	cvsscan(argv + optind, name, &lat);
	if (debug)
		printlat(&lat);
	printf("%jd\n", (intmax_t)lat.time);
	return 0;
}
