/*	$NetBSD: makedbm.c,v 1.3 1997/07/18 21:57:05 thorpej Exp $	*/

/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
 * All rights reserved.
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
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>

#include "protos.h"
#include "ypdb.h"
#include "ypdef.h"

extern	char *__progname;		/* from crt0.s */
extern	char *optarg;
extern	int optind;

int	main __P((int, char *[]));
void	usage __P((void));
void	add_record __P((DBM *, char *, char *, int));
char	*file_date __P((char *));
void	list_database __P((char *));
void	create_database __P((char *, char *, char *, char *,
	    char *, char *, int, int, int));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int aflag, uflag, bflag, lflag, sflag;
	char *yp_input_file, *yp_output_file;
	char *yp_master_name, *yp_domain_name;
	char *infile, *outfile;
	int ch;

	yp_input_file = yp_output_file = NULL;
	yp_master_name = yp_domain_name = NULL;
	aflag = uflag = bflag = lflag = sflag = 0;
	infile = outfile = NULL;

	while ((ch = getopt(argc, argv, "blsui:o:m:d:")) != -1) {
		switch (ch) {
		case 'b':
			bflag = aflag = 1;
			break;

		case 'l':
			lflag = aflag = 1;
			break;

		case 's':
			sflag = aflag = 1;
			break;

		case 'i':
			yp_input_file = optarg;
			aflag = 1;
			break;

		case 'o':
			yp_output_file = optarg;
			aflag = 1;
			break;

		case 'm':
			yp_master_name = optarg;
			aflag = 1;
			break;

		case 'd':
			yp_domain_name = optarg;
			aflag = 1;
			break;

		case 'u':
			uflag = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind; argv += optind;

	if ((uflag != 0) && (aflag != 0))
		usage();
	else {
		if (uflag != 0) {
			if (argc == 1)
				infile = argv[0];
			else
				usage();
		} else {
			if (argc == 2) {
				infile = argv[0];
				outfile = argv[1];
			} else
				usage();
		}
	}

	if (uflag != 0)
		list_database(infile);
	else
		create_database(infile, outfile,
		    yp_input_file, yp_output_file, yp_master_name,
		    yp_domain_name, bflag, lflag, sflag);

	exit(0);
}

void
add_record(db, str1, str2, check)
	DBM *db;
	char *str1, *str2;
	int check;
{
	datum key, val;
	int status;

	key.dptr = str1;
	key.dsize = strlen(str1);

	if (check) {
		val = ypdb_fetch(db, key);

		if (val.dptr != NULL)
			return;	/* already there */
	}
	val.dptr = str2;
	val.dsize = strlen(str2);
	status = ypdb_store(db, key, val, YPDB_INSERT);

	if (status != 0)
		errx(1, "can't store `%s %s'", str1, str2);
}

char *
file_date(filename)
	char *filename;
{
	struct stat finfo;
	static char datestr[11];

	memset(datestr, 0, sizeof(datestr));

	if (strcmp(filename, "-") == 0)
		snprintf(datestr, sizeof(datestr), "%010d",
		    (int)time(NULL));
	else {
		if (stat(filename, &finfo) != 0)
			err(1, "can't stat %s", filename);
		snprintf(datestr, sizeof(datestr), "%010d",
		    (int)finfo.st_mtime);
	}

	return datestr;
}

void
list_database(database)
	char *database;
{
	DBM *db;
	datum key, val;

	db = ypdb_open(database, O_RDONLY, 0444);
	if (db == NULL)
		errx(1, "can't open database `%s'", database);

	key = ypdb_firstkey(db);

	while (key.dptr != NULL) {
		val = ypdb_fetch(db, key);
		printf("%*.*s %*.*s\n",
		    key.dsize, key.dsize, key.dptr,
		    val.dsize, val.dsize, val.dptr);
		key = ypdb_nextkey(db);
	}

	ypdb_close(db);
}

void
create_database(infile, database, yp_input_file, yp_output_file,
    yp_master_name, yp_domain_name, bflag, lflag, sflag)
	char *infile, *database, *yp_input_file, *yp_output_file;
	char *yp_master_name, *yp_domain_name;
	int bflag, lflag, sflag;
{
	FILE *data_file;
	char data_line[4096];	/* XXX: DB bsize = 4096 in ypdb.c */
	char myname[MAXHOSTNAMELEN];
	int line_no = 0;
	int len;
	char *p, *k, *v, *slash;
	DBM *new_db;
	static char mapname[] = "ypdbXXXXXX";
	char db_mapname[MAXPATHLEN + 1], db_outfile[MAXPATHLEN + 1];
	char db_tempname[MAXPATHLEN + 1];
	char empty_str[] = "";

	memset(db_mapname, 0, sizeof(db_mapname));
	memset(db_outfile, 0, sizeof(db_outfile));
	memset(db_tempname, 0, sizeof(db_tempname));

	if (strcmp(infile, "-") == 0)
		data_file = stdin;
	else {
		data_file = fopen(infile, "r");
		if (data_file == NULL)
			err(1, "can't open `%s'", infile);
	}

	if (strlen(database) + strlen(YPDB_SUFFIX) > MAXPATHLEN)
		errx(1, "%s: file name too long\n", database);

	snprintf(db_outfile, sizeof(db_outfile), "%s%s", database, YPDB_SUFFIX);

	slash = strrchr(database, '/');
	if (slash != NULL)
		slash[1] = '\0';	/* truncate to dir */
	else
		*database = '\0';	/* elminate */

	/* NOTE: database is now directory where map goes ! */

	if (strlen(database) + strlen(mapname) +
	    strlen(YPDB_SUFFIX) > MAXPATHLEN)
		errx(1, "%s: directory name too long", database);

	snprintf(db_tempname, sizeof(db_tempname), "%s%s",
	    database, mapname);
	mktemp(db_tempname);
	snprintf(db_mapname, sizeof(db_mapname), "%s%s",
	    db_tempname, YPDB_SUFFIX);

	new_db = ypdb_open(db_tempname, O_RDWR | O_CREAT | O_EXCL, 0644);
	if (new_db == NULL)
		errx(1, "can't create temp database `%s'", db_tempname);

	while (read_line(data_file, data_line, sizeof(data_line))) {
		line_no++;
		len = strlen(data_line);

		/* Check if we have the whole line */

		if (data_line[len - 1] != '\n')
			warnx("%s: line %d too long", infile, line_no);
		else
			data_line[len - 1] = '\0';

		p = (char *)&data_line;

		k = p;		/* save start of key */
		while (!isspace(*p)) {	/* find first "space" */
			/*
			 * Convert to lower case if forcing.
			 */
			if (lflag && isupper(*p))
				*p = tolower(*p);
			p++;
		}
		while (isspace(*p))	/* replace space with <NUL> */
			*p++ = '\0';

		v = p;			/* save start of value */

		while (*p != '\0')	/* find end of string */
			p++;

		add_record(new_db, k, v, TRUE);	/* save record */
	}

	if (strcmp(infile, "-") != 0)
		(void) fclose(data_file);

	add_record(new_db, YP_LAST_KEY, file_date(infile), FALSE);

	if (yp_input_file)
		add_record(new_db, YP_INPUT_KEY, yp_input_file, FALSE);

	if (yp_output_file)
		add_record(new_db, YP_OUTPUT_KEY, yp_output_file, FALSE);

	if (yp_master_name)
		add_record(new_db, YP_MASTER_KEY, yp_master_name, FALSE);
	else {
		localhostname(myname, sizeof(myname) - 1);
		add_record(new_db, YP_MASTER_KEY, myname, FALSE);
	}

	if (yp_domain_name)
		add_record(new_db, YP_DOMAIN_KEY, yp_domain_name, FALSE);

	if (bflag)
		add_record(new_db, YP_INTERDOMAIN_KEY, empty_str, FALSE);

	if (sflag)
		add_record(new_db, YP_SECURE_KEY, empty_str, FALSE);

	ypdb_close(new_db);
	if (rename(db_mapname, db_outfile) < 0)
		err(1, "rename `%s' -> `%s'", db_mapname, db_outfile);
}

void
usage()
{

	fprintf(stderr, "usage: %s -u file", __progname);
	fprintf(stderr, "       %s [-lbs] %s\n", __progname,
	    "[-i YP_INPUT_FILE] [-o YP_OUTPUT_FILE]");
	fprintf(stderr, "          %s infile outfile\n",
	    "[-d YP_DOMAIN_NAME] [-m YP_MASTER_NAME]");
	exit(1);
}
