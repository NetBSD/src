/*	$NetBSD: cap_mkdb.c,v 1.24.6.1 2009/05/13 19:19:45 jym Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#if 0
static char sccsid[] = "@(#)cap_mkdb.c	8.2 (Berkeley) 4/27/95";
#endif
__RCSID("$NetBSD: cap_mkdb.c,v 1.24.6.1 2009/05/13 19:19:45 jym Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <assert.h>
#include <db.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static void	db_build(const char **);
static void	dounlink(void);
static void	usage(void) __unused;
static int	count_records(char **);

static DB *capdbp;
static int verbose;
static char *capname, buf[8 * 1024];

static HASHINFO openinfo = {
	4096,		/* bsize */
	16,		/* ffactor */
	2048,		/* nelem */
	2048 * 1024,	/* cachesize */
	NULL,		/* hash() */
	0		/* lorder */
};

/*
 * Mkcapdb creates a capability hash database for quick retrieval of capability
 * records.  The database contains 2 types of entries: records and references
 * marked by the first byte in the data.  A record entry contains the actual
 * capability record whereas a reference contains the name (key) under which
 * the correct record is stored.
 */
int
main(int argc, char *argv[])
{
	int c, byteorder;
	char *p;

	capname = NULL;
	byteorder = 0;
	while ((c = getopt(argc, argv, "bf:lv")) != -1) {
		switch(c) {
		case 'b':
		case 'l':
			if (byteorder != 0)
				usage();
			byteorder = c == 'b' ? 4321 : 1234;
			break;
		case 'f':
			capname = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (*argv == NULL)
		usage();

	/* Set byte order */
	openinfo.lorder = byteorder;

	/*
	 * Set nelem to twice the value returned by count_record().
	 */
	openinfo.nelem = count_records(argv) << 1;

	/*
	 * The database file is the first argument if no name is specified.
	 * Make arrangements to unlink it if exit badly.
	 */
	(void)snprintf(buf, sizeof(buf), "%s.db.tmp",
	    capname ? capname : *argv);
	if ((capname = strdup(buf)) == NULL)
		err(1, "strdup");
	if ((capdbp = dbopen(capname, O_CREAT | O_TRUNC | O_RDWR,
	    DEFFILEMODE, DB_HASH, &openinfo)) == NULL)
		err(1, "%s", buf);

	if (atexit(dounlink))
		err(1, "atexit");

	db_build((const char **)argv);

	if (capdbp->close(capdbp) < 0)
		err(1, "%s", capname);
	assert((p = strrchr(buf, '.')) != NULL);
	*p = '\0';
	if (rename(capname, buf) == -1)
		err(1, "rename");
	free(capname);
	capname = NULL;
	return 0;
}

static void
dounlink(void)
{
	if (capname != NULL)
		(void)unlink(capname);
}

/*
 * Any changes to these definitions should be made also in the getcap(3)
 * library routines.
 */
#define RECOK	(char)0
#define TCERR	(char)1
#define SHADOW	(char)2

/*
 * Db_build() builds the name and capabilty databases according to the
 * details above.
 */
static void
db_build(const char **ifiles)
{
	DBT key, data;
	recno_t reccnt;
	size_t len, bplen;
	int st;
	char *bp, *p, *t, *n;

	data.data = NULL;
	key.data = NULL;
	for (reccnt = 0, bplen = 0; (st = cgetnext(&bp, ifiles)) > 0; free(bp)){

		/*
		 * Allocate enough memory to store record, terminating
		 * NULL and one extra byte.
		 */
		len = strlen(bp);
		if (bplen <= len + 2) {
			if ((n = realloc(data.data,
			    bplen + MAX(256, len + 2))) == NULL)
				err(1, "realloc");
			data.data = n;
			bplen += MAX(256, len + 2);
		}

		/* Find the end of the name field. */
		if ((p = strchr(bp, ':')) == NULL) {
			warnx("no name field: %.*s", (int)(MIN(len, 20)), bp);
			continue;
		}

		/* First byte of stored record indicates status. */
		switch(st) {
		case 1:
			((char *)(data.data))[0] = RECOK;
			break;
		case 2:
			((char *)(data.data))[0] = TCERR;
			warnx("Record not tc expanded: %.*s", (int)(p - bp),bp);
			break;
		}

		/* Create the stored record. */
		(void)memmove(&((u_char *)(data.data))[1], bp, len + 1);
		data.size = len + 2;

		/* Store the record under the name field. */
		key.data = bp;
		key.size = p - bp;

		switch(capdbp->put(capdbp, &key, &data, R_NOOVERWRITE)) {
		case -1:
			err(1, "put");
			/* NOTREACHED */
		case 1:
			warnx("ignored duplicate: %.*s",
			    (int)key.size, (char *)key.data);
			continue;
		}
		++reccnt;

		/* If only one name, ignore the rest. */
		if ((p = strchr(bp, '|')) == NULL)
			continue;

		/* The rest of the names reference the entire name. */
		((char *)(data.data))[0] = SHADOW;
		(void)memmove(&((u_char *)(data.data))[1], key.data, key.size);
		data.size = key.size + 1;

		/* Store references for other names. */
		for (p = t = bp;; ++p) {
			if (p > t && (*p == ':' || *p == '|')) {
				key.size = p - t;
				key.data = t;
				switch(capdbp->put(capdbp,
				    &key, &data, R_NOOVERWRITE)) {
				case -1:
					err(1, "put");
					/* NOTREACHED */
				case 1:
					warnx("ignored duplicate: %.*s",
					    (int)key.size, (char *)key.data);
				}
				t = p + 1;
			}
			if (*p == ':')
				break;
		}
	}

	switch(st) {
	case -1:
		err(1, "file argument");
		/* NOTREACHED */
	case -2:
		errx(1, "potential reference loop detected");
		/* NOTREACHED */
	}

	if (verbose)
		(void)printf("cap_mkdb: %d capability records\n", reccnt);
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-b|-l] [-v] [-f outfile] file1 [file2 ...]\n",
	    getprogname());
	exit(1);
}

/*
 * Count number of records in input files. This does not need
 * to be really accurate (the result is used only as a hint).
 * It seems to match number of records should a cgetnext() be used, though.
 */
static int
count_records(char **list)
{
	FILE *fp;
	char *line;
	size_t len;
	int nelem, slash;

	/* scan input files and count individual records */
	for(nelem = 0, slash = 0; *list && (fp = fopen(*list++, "r")); ) {
		while((line = fgetln(fp, &len)) != NULL) {
			if (len < 2)
				continue;
			if (!isspace((unsigned char) *line) && *line != ':'
				&& *line != '#' && !slash)
				nelem++;

			slash = (line[len - 2] == '\\');
		}
		(void)fclose(fp);
	} 

	if (nelem == 0) {
		/* no records found; pass default size hint */
		nelem = 1;
	} else if (!powerof2(nelem)) {
		/* set nelem to nearest bigger power-of-two number */
		int bt = 1;
		while(bt < nelem) bt <<= 1;
		nelem = bt;
	}

	return nelem;
}
