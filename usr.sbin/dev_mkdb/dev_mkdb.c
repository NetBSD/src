/*	$NetBSD: dev_mkdb.c,v 1.16 2001/07/14 14:50:44 manu Exp $	*/

/*-
 * Copyright (c) 1990, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)dev_mkdb.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: dev_mkdb.c,v 1.16 2001/07/14 14:50:44 manu Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <kvm.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	main __P((int, char *[]));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat *st;
	struct {
		mode_t type;
		dev_t dev;
	} bkey;
	DB *db;
	DBT data, key;
	FTS *ftsp;
	FTSENT *p;
	int ch;
	u_char buf[MAXPATHLEN + 1];
	char dbtmp[MAXPATHLEN + 1];
	char dbname[MAXPATHLEN + 1];
	char *dbname_arg = NULL;
	char *pathv[2];
	char path_dev[MAXPATHLEN + 1] = _PATH_DEV;
	char cur_dir[MAXPATHLEN + 1];
	struct timeval tv;
	char *q;

	while ((ch = getopt(argc, argv, "o:")) != -1)
		switch (ch) {
		case 'o':
			if (strlen(optarg) <= MAXPATHLEN)
				dbname_arg = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if ((argc == 1) && (strlen(argv[0]) <= MAXPATHLEN))
		strncpy(path_dev, argv[0], MAXPATHLEN);

	if (argc > 1)
		usage();

	if (!getcwd(cur_dir, MAXPATHLEN))
		err(1, "%s", cur_dir);

	if (chdir(path_dev))
		err(1, "%s", path_dev);

	pathv[0] = path_dev;
	pathv[1] = NULL;
	ftsp = fts_open(pathv, FTS_PHYSICAL, NULL);
	if (ftsp == NULL)
		err(1, "fts_open: %s", path_dev);

	if (chdir(cur_dir))
		err(1, "%s", cur_dir);

	if (dbname_arg)
		strncpy(dbname, dbname_arg, MAXPATHLEN);
	else
		(void)snprintf(dbname, MAXPATHLEN, "%sdev.db", _PATH_VARRUN);
	/* 
	 * We use rename() to produce the dev.db file from a temporary file,
	 * and rename() is not able to move files across filesystems. Hence we 
	 * need the temporary file to be in the same directory as dev.db.
	 *
	 * Additionally, we might be working in a world writable directory, 
	 * we must ensure that we are not opening an existing file, therefore
	 * the loop on dbopen. 
	 */
	(void)strncpy(dbtmp, dbname, MAXPATHLEN);
	q = dbtmp + strlen(dbtmp);
	do {
		(void)gettimeofday(&tv, NULL);
		(void)snprintf(q, MAXPATHLEN - (long)(q - dbtmp), 
			    "%ld.tmp", tv.tv_usec);
		printf (">>%s<<\n", dbtmp);
		db = dbopen(dbtmp, O_CREAT|O_EXCL|O_EXLOCK|O_RDWR|O_TRUNC,
		    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH, DB_HASH, NULL);
	} while (!db && (errno == EEXIST));
	if (db == NULL)
		err(1, "%s", dbtmp);

	/*
	 * Keys are a mode_t followed by a dev_t.  The former is the type of
	 * the file (mode & S_IFMT), the latter is the st_rdev field.  Note
	 * that the structure may contain padding, so we have to clear it
	 * out here.
	 */
	memset(&bkey, 0, sizeof(bkey));
	key.data = &bkey;
	key.size = sizeof(bkey);
	data.data = buf;
	while ((p = fts_read(ftsp)) != NULL) {
		switch (p->fts_info) {
		case FTS_DEFAULT:
			st = p->fts_statp;
			break;
		default:
			continue;
		}

		/* Create the key. */
		if (S_ISCHR(st->st_mode))
			bkey.type = S_IFCHR;
		else if (S_ISBLK(st->st_mode))
			bkey.type = S_IFBLK;
		else
			continue;
		bkey.dev = st->st_rdev;
		printf (">>>%d\n", bkey.dev);

		/*
		 * Create the data; nul terminate the name so caller doesn't
		 * have to.  Skip path_dev and slash.
		 */
		strlcpy(buf, p->fts_path + (strlen(path_dev) + 1), sizeof(buf));
		data.size = p->fts_pathlen - (strlen(path_dev) + 1) + 1;
		if ((*db->put)(db, &key, &data, 0)) {
			err(1, "dbput %s", dbtmp);
		}
	}
	(void)(*db->close)(db);
	fts_close(ftsp);

	if (chdir(cur_dir))
		err(1, "%s", cur_dir);
	if (rename(dbtmp, dbname))
		err(1, "rename %s to %s", dbtmp, dbname);
	exit(0);
}

void
usage()
{

	(void)fprintf(stderr, "usage: dev_mkdb [-o database] [directory]\n");
	exit(1);
}
