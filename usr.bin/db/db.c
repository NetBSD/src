/*	$NetBSD: db.c,v 1.25.2.1 2012/04/17 00:09:30 yamt Exp $	*/

/*-
 * Copyright (c) 2002-2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn of Wasabi Systems.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
#ifdef __RCSID
__RCSID("$NetBSD: db.c,v 1.25.2.1 2012/04/17 00:09:30 yamt Exp $");
#endif /* __RCSID */
#endif /* not lint */

#include <ctype.h>
#include <db.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>


typedef enum {
	F_WRITE		= 1<<0,
	F_DELETE	= 1<<1,
	F_SHOW_KEY	= 1<<2,
	F_SHOW_VALUE	= 1<<3,
	F_QUIET		= 1<<10,
	F_IGNORECASE	= 1<<11,
	F_ENDIAN_BIG	= 1<<12,
	F_ENDIAN_LITTLE	= 1<<13,
	F_INCLUDE_NUL	= 1<<14,
	F_CREATENEW	= 1<<20,
	F_DUPLICATES	= 1<<21,
	F_REPLACE	= 1<<22,
	F_ENCODE_KEY	= 1<<23,
	F_ENCODE_VAL	= 1<<24,
	F_DECODE_KEY	= 1<<25,
	F_DECODE_VAL	= 1<<26,
} flags_t;

static void	db_print(DBT *, DBT *);
static int	db_dump(void);
static int	db_del(char *);
static int	db_get(char *);
static int	db_seq(char *);
static int	db_put(char *, char *);
static int	parseline(FILE *, const char *, char **, char **);
static int	encode_data(size_t, char *, char **);
static int	decode_data(char *, char **);
static void	parse_encode_decode_arg(const char *, int);
static int	parse_encode_option(char **);
__dead static void	usage(void);

static flags_t	 	 flags = 0;
static DB		*db;
static const char	*outputsep = "\t";
static int		 visflags = 0;
static const char	*extra_echars = NULL;

int
main(int argc, char *argv[])
{
	struct {
		char		*file;
		char		*type;
		DBTYPE	 	dbtype;
		void		*info;
		int	 	dbflags;
		mode_t	 	mode;
		unsigned int	pagesize;
	} oi;
	BTREEINFO	btreeinfo;
	HASHINFO	hashinfo;
	FILE		*infp;
	const char	*infile, *fieldsep;
	char		*p, *key, *val;
	int		ch, rv;
	long		lval;

	setprogname(argv[0]);

	infile = NULL;
	fieldsep = " ";
	infp = NULL;
	memset(&oi, 0, sizeof(oi));
	oi.mode = 0644;
	oi.pagesize = 4096;

				/* parse arguments */
	while ( (ch = getopt(argc, argv,
			     "CDdE:F:f:iKm:NO:P:qRS:T:U:VwX:")) != -1) {
		switch (ch) {

		case 'C':
			flags |= F_CREATENEW;
			break;

		case 'D':
			flags |= F_DUPLICATES;
			break;

		case 'd':
			flags |= F_DELETE;
			break;

		case 'E':
			if (! optarg[0] || optarg[1])
				goto badendian;
			switch (toupper((int)optarg[0])) {
			case 'B':
				flags |= F_ENDIAN_BIG;
				break;
			case 'L':
				flags |= F_ENDIAN_LITTLE;
				break;
			case 'H':
				flags &= ~(F_ENDIAN_BIG | F_ENDIAN_LITTLE);
				break;
			default:
 badendian:
				errx(1, "Bad endian `%s'", optarg);
			}
			break;

		case 'F':
			if (! optarg[0])
				errx(1, "Invalid field separator `%s'",
				    optarg);
			fieldsep = optarg;
			break;

		case 'f':
			infile = optarg;
			break;

		case 'i':
			flags |= F_IGNORECASE;
			break;

		case 'K':
			flags |= F_SHOW_KEY;
			break;

		case 'm':
			lval = strtol(optarg, &p, 8);
			if (p == optarg || *p != '\0')
				errx(1, "Invalid octal number `%s'", optarg);
			if (lval < 0 || lval > 07777)
				errx(1, "Invalid mode `%s'", optarg);
			oi.mode = (mode_t)lval;
			break;

		case 'N':
			flags |= F_INCLUDE_NUL;
			break;

		case 'O':
			outputsep = optarg;
			break;

		case 'P':
			lval = strtol(optarg, &p, 10);
			if (p == optarg || *p != '\0')
				errx(1, "Invalid pagesize `%s'", optarg);
			if (lval < 0 || (unsigned int)lval >= UINT_MAX)
				errx(1, "Pagesize `%s' out of range", optarg);
			oi.pagesize = (unsigned int)lval;
			break;

		case 'q':
			flags |= F_QUIET;
			break;

		case 'R':
			flags |= F_REPLACE;
			break;

		case 'S':
			parse_encode_decode_arg(optarg, 1 /* encode */);
			if (! (flags & (F_ENCODE_KEY | F_ENCODE_VAL)))
				errx(1, "Invalid encoding argument `%s'",
				    optarg);
			break;

		case 'T':
			visflags = parse_encode_option(&optarg);
			if (! visflags)
				errx(1, "Invalid encoding/decoding option `%s'",
				    optarg);
			break;

		case 'U':
			parse_encode_decode_arg(optarg, 0 /* decode */);
			if (! (flags & (F_DECODE_KEY | F_DECODE_VAL)))
				errx(1, "Invalid decoding argument `%s'",
				    optarg);
			break;

		case 'V':
			flags |= F_SHOW_VALUE;
			break;

		case 'w':
			flags |= F_WRITE;
			break;

		case 'X':
			extra_echars = optarg;
			break;

		default:
			usage();

		}
	}
	argc -= optind;
	argv += optind;

				/* validate arguments */
	if (argc < 2)
		usage();
	oi.type = argv[0];
	oi.file = argv[1];
	argc -= 2;
	argv += 2;

	if (flags & F_WRITE) {
		if (flags & (F_SHOW_KEY | F_SHOW_VALUE | F_DELETE))
			usage();
		if ((!infile && argc < 2) || (argc % 2))
			usage();
		if (0 != (visflags & ~(VIS_HTTPSTYLE)))
			errx(1, "Unsupported decoding option provided to -T");
		oi.dbflags = O_RDWR | O_CREAT | O_EXLOCK;
		if (flags & F_CREATENEW)
			oi.dbflags |= O_TRUNC;
	} else if (flags & F_DELETE) {
		if (flags & (F_SHOW_KEY | F_SHOW_VALUE | F_WRITE))
			usage();
		if (!infile && argc < 1)
			usage();
		if (0 != (visflags & ~(VIS_HTTPSTYLE)))
			errx(1, "Unsupported decoding option provided to -T");
		oi.dbflags = O_RDWR | O_CREAT | O_EXLOCK;
	} else {
		if (! (flags & (F_SHOW_KEY | F_SHOW_VALUE)))
			flags |= (F_SHOW_KEY | F_SHOW_VALUE);
		oi.dbflags = O_RDONLY | O_SHLOCK;
	}

				/* validate oi.type */
	if (strcmp(oi.type, "btree") == 0) {
		memset(&btreeinfo, 0, sizeof(btreeinfo));
		if (flags & F_ENDIAN_BIG)
			btreeinfo.lorder = 4321;
		else if (flags & F_ENDIAN_LITTLE)
			btreeinfo.lorder = 1234;
		if (flags & F_DUPLICATES)
			btreeinfo.flags = R_DUP;
		btreeinfo.psize = oi.pagesize;
		btreeinfo.cachesize = 1024 * 1024;
		oi.info = &btreeinfo;
		oi.dbtype = DB_BTREE;
	} else if (strcmp(oi.type, "hash") == 0) {
		memset(&hashinfo, 0, sizeof(hashinfo));
		if (flags & F_ENDIAN_BIG)
			hashinfo.lorder = 4321;
		else if (flags & F_ENDIAN_LITTLE)
			hashinfo.lorder = 1234;
		hashinfo.bsize = oi.pagesize;
		hashinfo.cachesize = 1024 * 1024;
		oi.info = &hashinfo;
		oi.dbtype = DB_HASH;
	} else {
		warnx("Unknown database type `%s'", oi.type);
		usage();
	}

	if (infile) {
		if (strcmp(infile, "-") == 0)
			infp = stdin;
		else if ((infp = fopen(infile, "r")) == NULL)
			err(1, "Opening input file `%s'", infile);
	}

				/* open database */
	db = dbopen(oi.file, oi.dbflags, oi.mode, oi.dbtype, oi.info);
	if (db == NULL)
		err(1, "Opening database `%s'", oi.file);


				/* manipulate database */
	rv = 0;
	if (flags & F_WRITE) {			/* write entries */
		for (ch = 0; ch < argc; ch += 2)
			if ((rv = db_put(argv[ch], argv[ch+1])))
				goto cleanup;
		if (infp) {
			while (parseline(infp, fieldsep, &key, &val)) {
				if ((rv = db_put(key, val)))
					goto cleanup;
			}
			if (ferror(infp)) {
				warnx("Reading `%s'", infile);
				goto cleanup;
			}
		}
	} else if (!infp && argc == 0) {	/* read all */
		db_dump();
	} else {				/* read/delete specific */
		int	(*dbop)(char *);

		if (flags & F_DELETE)
			dbop = db_del;
		else if (DB_BTREE == oi.dbtype)
			dbop = db_seq;
		else if (DB_HASH == oi.dbtype)
			dbop = db_get;
		else
			errx(5, "internal error: unsupported dbtype %d",
			    oi.dbtype);
		for (ch = 0; ch < argc; ch++) {
			if ((rv = dbop(argv[ch])))
				goto cleanup;
		}
		if (infp) {
			while (parseline(infp, fieldsep, &key, NULL)) {
				if ((rv = dbop(key)))
					goto cleanup;
			}
			if (ferror(infp)) {
				warnx("Reading `%s'", infile);
				goto cleanup;
			}
		}
	}

				/* close database */
 cleanup:
	if (db->close(db) == -1)
		err(1, "Closing database `%s'", oi.file);
	if (infp)
		fclose(infp);
	return (rv);
}

static void
db_print(DBT *key, DBT *val)
{
	int	len;
	char	*data;

#define	MINUSNUL(x) ((x) > 0  ?  (x) - (flags & F_INCLUDE_NUL ? 0 : 1)  :  0)

	if (flags & F_SHOW_KEY) {
		if (flags & F_ENCODE_KEY) {
			len = encode_data(MINUSNUL(key->size),
			    (char *)key->data, &data);
		} else {
			len = (int)MINUSNUL(key->size);
			data = (char *)key->data;
		}
		printf("%.*s", len, data);
	}
	if ((flags & F_SHOW_KEY) && (flags & F_SHOW_VALUE))
		printf("%s", outputsep);
	if (flags & F_SHOW_VALUE) {
		if (flags & F_ENCODE_VAL) {
			len = encode_data(MINUSNUL(val->size),
			    (char *)val->data, &data);
		} else {
			len = (int)MINUSNUL(val->size);
			data = (char *)val->data;
		}
		printf("%.*s", len, data);
	}
	printf("\n");
}

static int
db_dump(void)
{
	DBT	key, val;
	int	rv;

	while ((rv = db->seq(db, &key, &val, R_NEXT)) == 0)
		db_print(&key, &val);
	if (rv == -1)
		warn("Error dumping database");
	return (rv == 1 ? 0 : 1);
}

static void
db_makekey(DBT *key, char *keystr, int downcase, int decode)
{
	char	*p, *ks;
	int	klen;

	memset(key, 0, sizeof(*key));
	if (decode) {
		if ((klen = decode_data(keystr, &ks)) == -1)
			errx(1, "Invalid escape sequence in `%s'", keystr);
	} else {
		klen = strlen(keystr);
		ks = keystr;
	}
	key->data = ks;
	key->size = klen + (flags & F_INCLUDE_NUL ? 0 : 1);
	if (downcase && (flags & F_IGNORECASE)) {
		for (p = ks; *p; p++)
			if (isupper((int)*p))
				*p = tolower((int)*p);
	}
}

static int
db_del(char *keystr)
{
	DBT	key;
	int	r;

	db_makekey(&key, keystr, 1, (flags & F_DECODE_KEY ? 1 : 0));
	r = db->del(db, &key, 0);
	switch (r) {
	case -1:
		if (! (flags & F_QUIET))
			warn("Error deleting key `%s'", keystr);
		r = 1;
		break;
	case 0:
		if (! (flags & F_QUIET))
			printf("Deleted key `%s'\n", keystr);
		break;
	case 1:
		if (! (flags & F_QUIET))
			warnx("Unknown key `%s'", keystr);
		break;
	default:
		errx(5, "%s: unexpected result %d from db", __func__, r);
	}
	if (flags & F_DECODE_KEY)
		free(key.data);
	return (r);
}

static int
db_get(char *keystr)
{
	DBT	key, val;
	int	r;

	db_makekey(&key, keystr, 1, (flags & F_DECODE_KEY ? 1 : 0));

	r = db->get(db, &key, &val, 0);
	switch (r) {
	case -1:
		warn("Error reading key `%s'", keystr);
		r = 1;
		break;
	case 0:
		db_print(&key, &val);
		break;
	case 1:
		if (! (flags & F_QUIET)) {
			warnx("Unknown key `%s'", keystr);
		}
		break;
	default:
		errx(5, "%s: unexpected result %d from db", __func__, r);
	}
	if (flags & F_DECODE_KEY)
		free(key.data);
	return (r);
}

static int
db_seq(char *keystr)
{
	DBT	key, val, want;
	int	r, found;
	u_int	seqflags;

	db_makekey(&key, keystr, 1, (flags & F_DECODE_KEY ? 1 : 0));
		/* remember key in want, since db->seq() changes key */
	want.data = key.data;
	want.size = key.size;

	found = 0;
	seqflags = R_CURSOR;
	while ((r = db->seq(db, &key, &val, seqflags)) == 0) {
		if (key.size != want.size ||
		    0 != strcmp((char *)key.data, (char *)want.data)) {
			r = 1;
			break;
		}
		seqflags = R_NEXT;
		found++;
		db_print(&key, &val);
		if (! (flags & F_DUPLICATES))
			break;
	}

	switch (r) {
	case -1:
		warn("Error reading key `%s'", keystr);
		r = 1;
		break;
	case 0:
		break;
	case 1:
		if (found) {
			r = 0;
			break;
		}
		if (! (flags & F_QUIET)) {
			warnx("Unknown key `%s'", keystr);
		}
		break;
	default:
		errx(5, "%s: unexpected result %d from db", __func__, r);
	}
	if (flags & F_DECODE_KEY)
		free(want.data);
	return (r);
}

static int
db_put(char *keystr, char *valstr)
{
	DBT	key, val;
	int	r = 0;

	db_makekey(&key, keystr, 1, (flags & F_DECODE_KEY ? 1 : 0));
	db_makekey(&val, valstr, 0, (flags & F_DECODE_VAL ? 1 : 0));
	r = db->put(db, &key, &val, (flags & F_REPLACE) ? 0 : R_NOOVERWRITE);
	switch (r) {
	case -1:
		warn("Error writing key `%s'", keystr);
		r = 1;
		break;
	case 0:
		if (! (flags & F_QUIET))
			printf("Added key `%s'\n", keystr);
		break;
	case 1:
		if (! (flags & F_QUIET))
			warnx("Key `%s' already exists", keystr);
		break;
	default:
		errx(5, "Unexpected result %d in %s", r, __func__);
	}
	if (flags & F_DECODE_KEY)
		free(key.data);
	if (flags & F_DECODE_VAL)
		free(val.data);
	return (r);
}

static int
parseline(FILE *fp, const char *sep, char **kp, char **vp)
{
	size_t	len;
	char	*key, *val;

	key = fgetln(fp, &len);
	if (key == NULL)		/* end of file, or error */
		return (0);

	if (key[len-1] == '\n')		/* check for \n at EOL */
		key[--len] = '\0';
	else
		return (0);

	*kp = key;
	if (vp == NULL)			/* don't split if don't want value */
		return (1);
	if ((val = strstr(key, sep)) == NULL)
		val = key + len;
	else {
		*val = '\0';
		val += strlen(sep);
	}
	*vp = val;
	return (1);
}

static int
encode_data(size_t len, char *data, char **edata)
{
	static char	*buf = NULL;
	char		*nbuf;
	static size_t	buflen = 0;
	size_t		elen;

	elen = 1 + (len * 4);
	if (elen > buflen) {
		if ((nbuf = realloc(buf, elen)) == NULL)
			err(1, "Cannot allocate encoding buffer");
		buf = nbuf;
		buflen = elen;
	}
	*edata = buf;
	if (extra_echars) {
		return (strsvisx(buf, data, len, visflags, extra_echars));
	} else {
		return (strvisx(buf, data, len, visflags));
	}
}

static int
decode_data(char *data, char **ddata)
{
	char	*buf;

	if ((buf = malloc(strlen(data) + 1)) == NULL)
		err(1, "Cannot allocate decoding buffer");
	*ddata = buf;
	return (strunvisx(buf, data, (visflags & VIS_HTTPSTYLE)));
}

static void
parse_encode_decode_arg(const char *arg, int encode)
{
	if (! arg[0] || arg[1])
		return;
	if (arg[0] == 'k' || arg[0] == 'b') {
		if (encode)
			flags |= F_ENCODE_KEY;
		else
			flags |= F_DECODE_KEY;
	}
	if (arg[0] == 'v' || arg[0] == 'b') {
		if (encode)
			flags |= F_ENCODE_VAL;
		else
			flags |= F_DECODE_VAL;
	}
	return;
}

static int
parse_encode_option(char **arg)
{
	int	r = 0;
	int	encmask = ~(VIS_CSTYLE | VIS_HTTPSTYLE | VIS_OCTAL);

	for(; **arg; (*arg)++) {
		switch (**arg) {
			case 'b':
				r |= VIS_NOSLASH;
				break;
			case 'c':
				r &= encmask;
				r |= VIS_CSTYLE;
				break;
			case 'h':
				r &= encmask;
				r |= VIS_HTTPSTYLE;
				break;
			case 'o':
				r &= encmask;
				r |= VIS_OCTAL;
				break;
			case 's':
				r |= VIS_SAFE;
				break;
			case 't':
				r |= VIS_TAB;
				break;
			case 'w':
				r |= VIS_WHITE;
				break;
			default:
				return (0);
				break;
		}
	}
	return (r);
}

static void
usage(void)
{
	const char *p = getprogname();

	fprintf(stderr,
"usage: %s    [-DKiNqV] [-E endian] [-f infile] [-O outsep] [-S visitem]\n"
"             [-T visspec] [-U unvisitem] [-X extravis] type dbfile [key [...]]\n"
"       %s -d [-iNq] [-E endian] [-f infile] [-T visspec] [-U unvisitem]\n"
"             type dbfile [key [...]]\n"
"       %s -w [-CDiNqR] [-E endian] [-F isep] [-f infile] [-m mode]\n"
"             [-P pagesize] [-T visspec] [-U unvisitem]\n"
"             type dbfile [key value [...]]\n"
	    ,p ,p ,p );
	fprintf(stderr,
"Supported modes:\n"
"                read keys  [default]\n"
"   -d           delete keys\n"
"   -w           write (add) keys/values\n"
"Supported options:\n"
"   -C           create empty (truncated) database\n"
"   -D           allow duplicates\n"
"   -E endian    database endian: `B'ig, `L'ittle, `H'ost  [default: H]\n"
"   -F isep      input field separator string  [default: a space]\n"
"   -f infile    file of keys (read|delete) or keys/vals (write)\n"
"   -i           ignore case of key by converting to lower case\n"
"   -K           print key\n"
"   -m mode      mode of created database  [default: 0644]\n"
"   -N           don't NUL terminate key\n"
"   -O outsep    output field separator string  [default: a tab]\n"
"   -P pagesize  database page size  [default: 4096]\n"
"   -q           quiet operation (missing keys aren't errors)\n"
"   -R           replace existing keys\n"
"   -S visitem   items to strvis(3) encode: 'k'ey, 'v'alue, 'b'oth\n"
"   -T visspec   options to control -S and -U; like vis(1) options\n"
"   -U unvisitem items to strunvis(3) decode: 'k'ey, 'v'alue, 'b'oth\n"
"   -V           print value\n"
"   -X extravis  extra characters to encode with -S\n"
	    );
	exit(1);
}
