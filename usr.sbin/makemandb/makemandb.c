/*	$NetBSD: makemandb.c,v 1.14.2.2 2013/02/25 00:30:45 tls Exp $	*/
/*
 * Copyright (c) 2011 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * Copyright (c) 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: makemandb.c,v 1.14.2.2 2013/02/25 00:30:45 tls Exp $");

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <archive.h>
#include <libgen.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "apropos-utils.h"
#include "dist/man.h"
#include "dist/mandoc.h"
#include "dist/mdoc.h"
#include "sqlite3.h"

#define BUFLEN 1024
#define MDOC 0	//If the page is of mdoc(7) type
#define MAN 1	//If the page  is of man(7) type

/*
 * A data structure for holding section specific data.
 */
typedef struct secbuff {
	char *data;
	size_t buflen;	//Total length of buffer allocated initially
	size_t offset;	// Current offset in the buffer.
} secbuff;

typedef struct makemandb_flags {
	int optimize;
	int limit;	// limit the indexing to only NAME section
	int recreate;	// Database was created from scratch
	int verbosity;	// 0: quiet, 1: default, 2: verbose
} makemandb_flags;

typedef struct mandb_rec {
	/* Fields for mandb table */
	char *name;	// for storing the name of the man page
	char *name_desc; // for storing the one line description (.Nd)
	secbuff desc; // for storing the DESCRIPTION section
	secbuff lib; // for the LIBRARY section
	secbuff return_vals; // RETURN VALUES
	secbuff env; // ENVIRONMENT
	secbuff files; // FILES
	secbuff exit_status; // EXIT STATUS
	secbuff diagnostics; // DIAGNOSTICS
	secbuff errors; // ERRORS
	char section[2];

	int xr_found;

	/* Fields for mandb_meta table */
	char *md5_hash;
	dev_t device;
	ino_t inode;
	time_t mtime;

	/* Fields for mandb_links table */
	char *machine;
	char *links; //all the links to a page in a space separated form
	char *file_path;

	/* Non-db fields */
	int page_type; //Indicates the type of page: mdoc or man
} mandb_rec;

static void append(secbuff *sbuff, const char *src);
static void init_secbuffs(mandb_rec *);
static void free_secbuffs(mandb_rec *);
static int check_md5(const char *, sqlite3 *, const char *, char **, void *, size_t);
static void cleanup(mandb_rec *);
static void set_section(const struct mdoc *, const struct man *, mandb_rec *);
static void set_machine(const struct mdoc *, mandb_rec *);
static int insert_into_db(sqlite3 *, mandb_rec *);
static	void begin_parse(const char *, struct mparse *, mandb_rec *,
			 const void *, size_t len);
static void pmdoc_node(const struct mdoc_node *, mandb_rec *);
static void pmdoc_Nm(const struct mdoc_node *, mandb_rec *);
static void pmdoc_Nd(const struct mdoc_node *, mandb_rec *);
static void pmdoc_Sh(const struct mdoc_node *, mandb_rec *);
static void pmdoc_Xr(const struct mdoc_node *, mandb_rec *);
static void pmdoc_Pp(const struct mdoc_node *, mandb_rec *);
static void pmdoc_macro_handler(const struct mdoc_node *, mandb_rec *,
				enum mdoct);
static void pman_node(const struct man_node *n, mandb_rec *);
static void pman_parse_node(const struct man_node *, secbuff *);
static void pman_parse_name(const struct man_node *, mandb_rec *);
static void pman_sh(const struct man_node *, mandb_rec *);
static void pman_block(const struct man_node *, mandb_rec *);
static void traversedir(const char *, const char *, sqlite3 *, struct mparse *);
static void mdoc_parse_section(enum mdoc_sec, const char *, mandb_rec *);
static void man_parse_section(enum man_sec, const struct man_node *, mandb_rec *);
static void build_file_cache(sqlite3 *, const char *, const char *,
			     struct stat *);
static void update_db(sqlite3 *, struct mparse *, mandb_rec *);
__dead static void usage(void);
static void optimize(sqlite3 *);
static char *parse_escape(const char *);
static makemandb_flags mflags = { .verbosity = 1 };

typedef	void (*pman_nf)(const struct man_node *n, mandb_rec *);
typedef	void (*pmdoc_nf)(const struct mdoc_node *n, mandb_rec *);
static	const pmdoc_nf mdocs[MDOC_MAX] = {
	NULL, /* Ap */
	NULL, /* Dd */
	NULL, /* Dt */
	NULL, /* Os */
	pmdoc_Sh, /* Sh */
	NULL, /* Ss */
	pmdoc_Pp, /* Pp */
	NULL, /* D1 */
	NULL, /* Dl */
	NULL, /* Bd */
	NULL, /* Ed */
	NULL, /* Bl */
	NULL, /* El */
	NULL, /* It */
	NULL, /* Ad */
	NULL, /* An */
	NULL, /* Ar */
	NULL, /* Cd */
	NULL, /* Cm */
	NULL, /* Dv */
	NULL, /* Er */
	NULL, /* Ev */
	NULL, /* Ex */
	NULL, /* Fa */
	NULL, /* Fd */
	NULL, /* Fl */
	NULL, /* Fn */
	NULL, /* Ft */
	NULL, /* Ic */
	NULL, /* In */
	NULL, /* Li */
	pmdoc_Nd, /* Nd */
	pmdoc_Nm, /* Nm */
	NULL, /* Op */
	NULL, /* Ot */
	NULL, /* Pa */
	NULL, /* Rv */
	NULL, /* St */
	NULL, /* Va */
	NULL, /* Vt */
	pmdoc_Xr, /* Xr */
	NULL, /* %A */
	NULL, /* %B */
	NULL, /* %D */
	NULL, /* %I */
	NULL, /* %J */
	NULL, /* %N */
	NULL, /* %O */
	NULL, /* %P */
	NULL, /* %R */
	NULL, /* %T */
	NULL, /* %V */
	NULL, /* Ac */
	NULL, /* Ao */
	NULL, /* Aq */
	NULL, /* At */
	NULL, /* Bc */
	NULL, /* Bf */
	NULL, /* Bo */
	NULL, /* Bq */
	NULL, /* Bsx */
	NULL, /* Bx */
	NULL, /* Db */
	NULL, /* Dc */
	NULL, /* Do */
	NULL, /* Dq */
	NULL, /* Ec */
	NULL, /* Ef */
	NULL, /* Em */
	NULL, /* Eo */
	NULL, /* Fx */
	NULL, /* Ms */
	NULL, /* No */
	NULL, /* Ns */
	NULL, /* Nx */
	NULL, /* Ox */
	NULL, /* Pc */
	NULL, /* Pf */
	NULL, /* Po */
	NULL, /* Pq */
	NULL, /* Qc */
	NULL, /* Ql */
	NULL, /* Qo */
	NULL, /* Qq */
	NULL, /* Re */
	NULL, /* Rs */
	NULL, /* Sc */
	NULL, /* So */
	NULL, /* Sq */
	NULL, /* Sm */
	NULL, /* Sx */
	NULL, /* Sy */
	NULL, /* Tn */
	NULL, /* Ux */
	NULL, /* Xc */
	NULL, /* Xo */
	NULL, /* Fo */
	NULL, /* Fc */
	NULL, /* Oo */
	NULL, /* Oc */
	NULL, /* Bk */
	NULL, /* Ek */
	NULL, /* Bt */
	NULL, /* Hf */
	NULL, /* Fr */
	NULL, /* Ud */
	NULL, /* Lb */
	NULL, /* Lp */
	NULL, /* Lk */
	NULL, /* Mt */
	NULL, /* Brq */
	NULL, /* Bro */
	NULL, /* Brc */
	NULL, /* %C */
	NULL, /* Es */
	NULL, /* En */
	NULL, /* Dx */
	NULL, /* %Q */
	NULL, /* br */
	NULL, /* sp */
	NULL, /* %U */
	NULL, /* Ta */
};

static	const pman_nf mans[MAN_MAX] = {
	NULL,	//br
	NULL,	//TH
	pman_sh, //SH
	NULL,	//SS
	NULL,	//TP
	NULL,	//LP
	NULL,	//PP
	NULL,	//P
	NULL,	//IP
	NULL,	//HP
	NULL,	//SM
	NULL,	//SB
	NULL,	//BI
	NULL,	//IB
	NULL,	//BR
	NULL,	//RB
	NULL,	//R
	pman_block,	//B
	NULL,	//I
	NULL,	//IR
	NULL,	//RI
	NULL,	//na
	NULL,	//sp
	NULL,	//nf
	NULL,	//fi
	NULL,	//RE
	NULL,	//RS
	NULL,	//DT
	NULL,	//UC
	NULL,	//PD
	NULL,	//AT
	NULL,	//in
	NULL,	//ft
};


int
main(int argc, char *argv[])
{
	FILE *file;
	const char *sqlstr, *manconf = NULL;
	char *line, *command, *parent;
	char *errmsg;
	int ch;
	struct mparse *mp;
	sqlite3 *db;
	ssize_t len;
	size_t linesize;
	struct mandb_rec rec;

	while ((ch = getopt(argc, argv, "C:floQqv")) != -1) {
		switch (ch) {
		case 'C':
			manconf = optarg;
			break;
		case 'f':
			mflags.recreate = 1;
			break;
		case 'l':
			mflags.limit = 1;
			break;
		case 'o':
			mflags.optimize = 1;
			break;
		case 'Q':
			mflags.verbosity = 0;
			break;
		case 'q':
			mflags.verbosity = 1;
			break;
		case 'v':
			mflags.verbosity = 2;
			break;
		default:
			usage();
		}
	}

	memset(&rec, 0, sizeof(rec));

	init_secbuffs(&rec);
	mp = mparse_alloc(MPARSE_AUTO, MANDOCLEVEL_FATAL, NULL, NULL);

	if (manconf) {
		char *arg;
		size_t command_len = shquote(manconf, NULL, 0) + 1;
		arg = emalloc(command_len);
		shquote(manconf, arg, command_len);
		easprintf(&command, "man -p -C %s", arg);
		free(arg);
	} else {
		command = estrdup("man -p");
		manconf = MANCONF;
	}

	if (mflags.recreate)
		remove(get_dbpath(manconf));

	if ((db = init_db(MANDB_CREATE, manconf)) == NULL)
		exit(EXIT_FAILURE);

	sqlite3_exec(db, "PRAGMA synchronous = 0", NULL, NULL, 	&errmsg);
	if (errmsg != NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		close_db(db);
		exit(EXIT_FAILURE);
	}

	sqlite3_exec(db, "ATTACH DATABASE \':memory:\' AS metadb", NULL, NULL,
	    &errmsg);
	if (errmsg != NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		close_db(db);
		exit(EXIT_FAILURE);
	}


	/* Call man -p to get the list of man page dirs */
	if ((file = popen(command, "r")) == NULL) {
		close_db(db);
		err(EXIT_FAILURE, "fopen failed");
	}
	free(command);

	/* Begin the transaction for indexing the pages	*/
	sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		exit(EXIT_FAILURE);
	}

	sqlstr = "CREATE TABLE metadb.file_cache(device, inode, mtime, parent,"
		 " file PRIMARY KEY);"
		 "CREATE UNIQUE INDEX metadb.index_file_cache_dev"
		 " ON file_cache (device, inode)";

	sqlite3_exec(db, sqlstr, NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		close_db(db);
		exit(EXIT_FAILURE);
	}

	if (mflags.verbosity)
		printf("Building temporary file cache\n");
	line = NULL;
	linesize = 0;
	while ((len = getline(&line, &linesize, file)) != -1) {
		/* Replace the new line character at the end of string with '\0' */
		line[len - 1] = '\0';
		parent = estrdup(line);
		char *pdir = estrdup(dirname(parent));
		free(parent);
		/* Traverse the man page directories and parse the pages */
		traversedir(pdir, line, db, mp);
		free(pdir);
	}
	free(line);

	if (pclose(file) == -1) {
		close_db(db);
		cleanup(&rec);
		free_secbuffs(&rec);
		err(EXIT_FAILURE, "pclose error");
	}

	if (mflags.verbosity)
		printf("Performing index update\n");
	update_db(db, mp, &rec);
	mparse_free(mp);
	free_secbuffs(&rec);

	/* Commit the transaction */
	sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		exit(EXIT_FAILURE);
	}

	if (mflags.optimize)
		optimize(db);

	close_db(db);
	return 0;
}

/*
 * traversedir --
 *  Traverses the given directory recursively and passes all the man page files
 *  in the way to build_file_cache()
 */
static void
traversedir(const char *parent, const char *file, sqlite3 *db,
            struct mparse *mp)
{
	struct stat sb;
	struct dirent *dirp;
	DIR *dp;
	char *buf;

	if (stat(file, &sb) < 0) {
		if (mflags.verbosity)
			warn("stat failed: %s", file);
		return;
	}

	/* If it is a regular file or a symlink, pass it to build_cache() */
	if (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode)) {
		build_file_cache(db, parent, file, &sb);
		return;
	}

	/* If it is a directory, traverse it recursively */
	if (S_ISDIR(sb.st_mode)) {
		if ((dp = opendir(file)) == NULL) {
			if (mflags.verbosity)
				warn("opendir error: %s", file);
			return;
		}

		while ((dirp = readdir(dp)) != NULL) {
			/* Avoid . and .. entries in a directory */
			if (strncmp(dirp->d_name, ".", 1)) {
				easprintf(&buf, "%s/%s", file, dirp->d_name);
				traversedir(parent, buf, db, mp);
				free(buf);
			}
		}
		closedir(dp);
	}
}

/* build_file_cache --
 *   This function generates an md5 hash of the file passed as it's 2nd parameter
 *   and stores it in a temporary table file_cache along with the full file path.
 *   This is done to support incremental updation of the database.
 *   The temporary table file_cache is dropped thereafter in the function
 *   update_db(), once the database has been updated.
 */
static void
build_file_cache(sqlite3 *db, const char *parent, const char *file,
		 struct stat *sb)
{
	const char *sqlstr;
	sqlite3_stmt *stmt = NULL;
	int rc, idx;
	assert(file != NULL);
	dev_t device_cache = sb->st_dev;
	ino_t inode_cache = sb->st_ino;
	time_t mtime_cache = sb->st_mtime;

	sqlstr = "INSERT INTO metadb.file_cache VALUES (:device, :inode,"
		 " :mtime, :parent, :file)";
	rc = sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		return;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":device");
	rc = sqlite3_bind_int64(stmt, idx, device_cache);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":inode");
	rc = sqlite3_bind_int64(stmt, idx, inode_cache);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":mtime");
	rc = sqlite3_bind_int64(stmt, idx, mtime_cache);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":parent");
	rc = sqlite3_bind_text(stmt, idx, parent, -1, NULL);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":file");
	rc = sqlite3_bind_text(stmt, idx, file, -1, NULL);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

static void
update_existing_entry(sqlite3 *db, const char *file, const char *hash,
    mandb_rec *rec, int *new_count, int *link_count, int *err_count)
{
	int update_count, rc, idx;
	const char *inner_sqlstr;
	sqlite3_stmt *inner_stmt;

	update_count = sqlite3_total_changes(db);
	inner_sqlstr = "UPDATE mandb_meta SET device = :device,"
		       " inode = :inode, mtime = :mtime WHERE"
		       " md5_hash = :md5 AND file = :file AND"
		       " (device <> :device2 OR inode <> "
		       "  :inode2 OR mtime <> :mtime2)";
	rc = sqlite3_prepare_v2(db, inner_sqlstr, -1, &inner_stmt, NULL);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		return;
	}
	idx = sqlite3_bind_parameter_index(inner_stmt, ":device");
	sqlite3_bind_int64(inner_stmt, idx, rec->device);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":inode");
	sqlite3_bind_int64(inner_stmt, idx, rec->inode);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":mtime");
	sqlite3_bind_int64(inner_stmt, idx, rec->mtime);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":md5");
	sqlite3_bind_text(inner_stmt, idx, hash, -1, NULL);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":file");
	sqlite3_bind_text(inner_stmt, idx, file, -1, NULL);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":device2");
	sqlite3_bind_int64(inner_stmt, idx, rec->device);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":inode2");
	sqlite3_bind_int64(inner_stmt, idx, rec->inode);
	idx = sqlite3_bind_parameter_index(inner_stmt, ":mtime2");
	sqlite3_bind_int64(inner_stmt, idx, rec->mtime);

	rc = sqlite3_step(inner_stmt);
	if (rc == SQLITE_DONE) {
		/* Check if an update has been performed. */
		if (update_count != sqlite3_total_changes(db)) {
			if (mflags.verbosity == 2)
				printf("Updated %s\n", file);
			(*new_count)++;
		} else {
			/* Otherwise it was a hardlink. */
			(*link_count)++;
		}
	} else {
		if (mflags.verbosity == 2)
			warnx("Could not update the meta data for %s", file);
		(*err_count)++;
	}
	sqlite3_finalize(inner_stmt);
}

/* read_and_decompress --
 *	Reads the given file into memory. If it is compressed, decompres
 *	it before returning to the caller.
 */
static int
read_and_decompress(const char *file, void **buf, size_t *len)
{
	size_t off;
	ssize_t r;
	struct archive *a;
	struct archive_entry *ae;

	if ((a = archive_read_new()) == NULL)
		errx(EXIT_FAILURE, "memory allocation failed");

	if (archive_read_support_compression_all(a) != ARCHIVE_OK ||
	    archive_read_support_format_raw(a) != ARCHIVE_OK ||
	    archive_read_open_filename(a, file, 65536) != ARCHIVE_OK ||
	    archive_read_next_header(a, &ae) != ARCHIVE_OK)
		goto archive_error;
	*len = 65536;
	*buf = emalloc(*len);
	off = 0;
	for (;;) {
		r = archive_read_data(a, (char *)*buf + off, *len - off);
		if (r == ARCHIVE_OK) {
			archive_read_close(a);
			*len = off;
			return 0;
		}
		if (r <= 0) {
			free(*buf);
			break;
		}
		off += r;
		if (off == *len) {
			*len *= 2;
			if (*len < off) {
				if (mflags.verbosity)
					warnx("File too large: %s", file);
				free(*buf);
				archive_read_close(a);
				return -1;
			}
			*buf = erealloc(*buf, *len);
		}
	}

archive_error:
	warnx("Error while reading `%s': %s", file, archive_error_string(a));
	archive_read_close(a);
	return -1;
}

/* update_db --
 *	Does an incremental updation of the database by checking the file_cache.
 *	It parses and adds the pages which are present in file_cache,
 *	but not in the database.
 *	It also removes the pages which are present in the databse,
 *	but not in the file_cache.
 */
static void
update_db(sqlite3 *db, struct mparse *mp, mandb_rec *rec)
{
	const char *sqlstr;
	sqlite3_stmt *stmt = NULL;
	const char *file;
	const char *parent;
	char *errmsg = NULL;
	char *md5sum;
	void *buf;
	size_t buflen;
	int new_count = 0;	/* Counter for newly indexed/updated pages */
	int total_count = 0;	/* Counter for total number of pages */
	int err_count = 0;	/* Counter for number of failed pages */
	int link_count = 0;	/* Counter for number of hard/sym links */
	int md5_status;
	int rc;

	sqlstr = "SELECT device, inode, mtime, parent, file"
	         " FROM metadb.file_cache fc"
	         " WHERE NOT EXISTS(SELECT 1 FROM mandb_meta WHERE"
	         "  device = fc.device AND inode = fc.inode AND "
	         "  mtime = fc.mtime AND file = fc.file)";

	rc = sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
		warnx("%s", sqlite3_errmsg(db));
		close_db(db);
		errx(EXIT_FAILURE, "Could not query file cache");
	}

	buf = NULL;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		free(buf);
		total_count++;
		rec->device = sqlite3_column_int64(stmt, 0);
		rec->inode = sqlite3_column_int64(stmt, 1);
		rec->mtime = sqlite3_column_int64(stmt, 2);
		parent = (const char *) sqlite3_column_text(stmt, 3);
		file = (const char *) sqlite3_column_text(stmt, 4);
		if (read_and_decompress(file, &buf, &buflen)) {
			err_count++;
			buf = NULL;
			continue;
		}
		md5_status = check_md5(file, db, "mandb_meta", &md5sum, buf, buflen);
		assert(md5sum != NULL);
		if (md5_status == -1) {
			if (mflags.verbosity)
				warnx("An error occurred in checking md5 value"
			      " for file %s", file);
			err_count++;
			continue;
		}

		if (md5_status == 0) {
			/*
			 * The MD5 hash is already present in the database,
			 * so simply update the metadata, ignoring symlinks.
			 */
			struct stat sb;
			stat(file, &sb);
			if (S_ISLNK(sb.st_mode)) {
				free(md5sum);
				link_count++;
				continue;
			}
			update_existing_entry(db, file, md5sum, rec,
			    &new_count, &link_count, &err_count);
			free(md5sum);
			continue;
		}

		if (md5_status == 1) {
			/*
			 * The MD5 hash was not present in the database.
			 * This means is either a new file or an updated file.
			 * We should go ahead with parsing.
			 */
			if (mflags.verbosity == 2)
				printf("Parsing: %s\n", file);
			rec->md5_hash = md5sum;
			rec->file_path = estrdup(file);
			// file_path is freed by insert_into_db itself.
			chdir(parent);
			begin_parse(file, mp, rec, buf, buflen);
			if (insert_into_db(db, rec) < 0) {
				if (mflags.verbosity)
					warnx("Error in indexing %s", file);
				err_count++;
			} else {
				new_count++;
			}
		}
	}
	free(buf);

	sqlite3_finalize(stmt);

	if (mflags.verbosity == 2) {
		printf("Total Number of new or updated pages encountered = %d\n"
			"Total number of (hard or symbolic) links found = %d\n"
			"Total number of pages that were successfully"
			" indexed/updated = %d\n"
			"Total number of pages that could not be indexed"
			" due to errors = %d\n",
			total_count - link_count, link_count, new_count, err_count);
	}

	if (mflags.recreate)
		return;

	if (mflags.verbosity == 2)
		printf("Deleting stale index entries\n");

	sqlstr = "DELETE FROM mandb_meta WHERE file NOT IN"
		 " (SELECT file FROM metadb.file_cache);"
		 "DELETE FROM mandb_links WHERE md5_hash NOT IN"
		 " (SELECT md5_hash from mandb_meta);"
		 "DROP TABLE metadb.file_cache;"
		 "DELETE FROM mandb WHERE rowid NOT IN"
		 " (SELECT id FROM mandb_meta);";

	sqlite3_exec(db, sqlstr, NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		warnx("Removing old entries failed: %s", errmsg);
		warnx("Please rebuild database from scratch with -f.");
		free(errmsg);
		return;
	}
}

/*
 * begin_parse --
 *  parses the man page using libmandoc
 */
static void
begin_parse(const char *file, struct mparse *mp, mandb_rec *rec,
    const void *buf, size_t len)
{
	struct mdoc *mdoc;
	struct man *man;
	mparse_reset(mp);

	rec->xr_found = 0;

	if (mparse_readmem(mp, buf, len, file) >= MANDOCLEVEL_FATAL) {
		/* Printing this warning at verbosity level 2
		 * because some packages from pkgsrc might trigger several
		 * of such warnings.
		 */
		if (mflags.verbosity == 2)
			warnx("%s: Parse failure", file);
		return;
	}

	mparse_result(mp, &mdoc, &man);
	if (mdoc == NULL && man == NULL) {
		if (mflags.verbosity == 2)
			warnx("Not a man(7) or mdoc(7) page");
		return;
	}

	set_machine(mdoc, rec);
	set_section(mdoc, man, rec);
	if (mdoc) {
		rec->page_type = MDOC;
		pmdoc_node(mdoc_node(mdoc), rec);
	} else {
		rec->page_type = MAN;
		pman_node(man_node(man), rec);
	}
}

/*
 * set_section --
 *  Extracts the section number and normalizes it to only the numeric part
 *  (Which should be the first character of the string).
 */
static void
set_section(const struct mdoc *md, const struct man *m, mandb_rec *rec)
{
	if (md) {
		const struct mdoc_meta *md_meta = mdoc_meta(md);
		rec->section[0] = md_meta->msec[0];
	} else if (m) {
		const struct man_meta *m_meta = man_meta(m);
		rec->section[0] = m_meta->msec[0];
	}
}

/*
 * get_machine --
 *  Extracts the machine architecture information if available.
 */
static void
set_machine(const struct mdoc *md, mandb_rec *rec)
{
	if (md == NULL)
		return;
	const struct mdoc_meta *md_meta = mdoc_meta(md);
	if (md_meta->arch)
		rec->machine = estrdup(md_meta->arch);
}

static void
pmdoc_node(const struct mdoc_node *n, mandb_rec *rec)
{

	if (n == NULL)
		return;

	switch (n->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_TAIL):
		/* FALLTHROUGH */
	case (MDOC_ELEM):
		if (mdocs[n->tok] == NULL)
			break;
		(*mdocs[n->tok])(n, rec);
		break;
	default:
		break;
	}

	pmdoc_node(n->child, rec);
	pmdoc_node(n->next, rec);
}

/*
 * pmdoc_Nm --
 *  Extracts the Name of the manual page from the .Nm macro
 */
static void
pmdoc_Nm(const struct mdoc_node *n, mandb_rec *rec)
{
	if (n->sec != SEC_NAME)
		return;

	for (n = n->child; n; n = n->next) {
		if (n->type == MDOC_TEXT) {
			concat(&rec->name, n->string);
		}
	}
}

/*
 * pmdoc_Nd --
 *  Extracts the one line description of the man page from the .Nd macro
 */
static void
pmdoc_Nd(const struct mdoc_node *n, mandb_rec *rec)
{
	/*
	 * A static variable for keeping track of whether a Xr macro was seen
	 * previously.
	 */
	char *buf = NULL;
	char *temp;

	if (n == NULL)
		return;

	if (n->type == MDOC_TEXT) {
		if (rec->xr_found && n->next) {
			/*
			 * An Xr macro was seen previously, so parse this
			 * and the next node.
			 */
			temp = estrdup(n->string);
			n = n->next;
			easprintf(&buf, "%s(%s)", temp, n->string);
			concat(&rec->name_desc, buf);
			free(buf);
			free(temp);
		} else {
			concat(&rec->name_desc, n->string);
		}
		rec->xr_found = 0;
	} else if (mdocs[n->tok] == pmdoc_Xr) {
		/* Remember that we have encountered an Xr macro */
		rec->xr_found = 1;
	}

	if (n->child)
		pmdoc_Nd(n->child, rec);

	if(n->next)
		pmdoc_Nd(n->next, rec);
}

/*
 * pmdoc_macro_handler--
 *  This function is a single point of handling all the special macros that we
 *  want to handle especially. For example the .Xr macro for properly parsing
 *  the referenced page name along with the section number, or the .Pp macro
 *  for adding a new line whenever we encounter it.
 */
static void
pmdoc_macro_handler(const struct mdoc_node *n, mandb_rec *rec, enum mdoct doct)
{
	const struct mdoc_node *sn;
	assert(n);

	switch (doct) {
	/*  Parse the man page references.
	 * Basically the .Xr macros are used like:
	 *  .Xr ls 1
 	 *  and formatted like this:
	 *  ls(1)
	 *  Prepare a buffer to format the data like the above example and call
	 *  pmdoc_parse_section to append it.
	 */
	case MDOC_Xr:
		n = n->child;
		while (n->type != MDOC_TEXT && n->next)
			n = n->next;

		if (n && n->type != MDOC_TEXT)
			return;
		sn = n;
		if (n->next)
			n = n->next;

		while (n->type != MDOC_TEXT && n->next)
			n = n->next;

		if (n && n->type == MDOC_TEXT) {
			size_t len = strlen(sn->string);
			char *buf = emalloc(len + 4);
			memcpy(buf, sn->string, len);
			buf[len] = '(';
			buf[len + 1] = n->string[0];
			buf[len + 2] = ')';
			buf[len + 3] = 0;
			mdoc_parse_section(n->sec, buf, rec);
			free(buf);
		}

		break;

	/* Parse the .Pp macro to add a new line */
	case MDOC_Pp:
		if (n->type == MDOC_TEXT)
			mdoc_parse_section(n->sec, "\n", rec);
		break;
	default:
		break;
	}

}

/*
 * pmdoc_Xr, pmdoc_Pp--
 *  Empty stubs.
 *  The parser calls these functions each time it encounters
 *  a .Xr or .Pp macro. We are parsing all the data from
 *  the pmdoc_Sh function, so don't do anything here.
 *  (See if else blocks in pmdoc_Sh.)
 */
static void
pmdoc_Xr(const struct mdoc_node *n, mandb_rec *rec)
{
}

static void
pmdoc_Pp(const struct mdoc_node *n, mandb_rec *rec)
{
}

/*
 * pmdoc_Sh --
 *  Called when a .Sh macro is encountered and loops through its body, calling
 *  mdoc_parse_section to append the data to the section specific buffer.
 *  Two special macros which may occur inside the body of Sh are .Nm and .Xr and
 *  they need special handling, thus the separate if branches for them.
 */
static void
pmdoc_Sh(const struct mdoc_node *n, mandb_rec *rec)
{
	if (n == NULL)
		return;
	int xr_found = 0;

	if (n->type == MDOC_TEXT) {
		mdoc_parse_section(n->sec, n->string, rec);
	} else if (mdocs[n->tok] == pmdoc_Nm && rec->name != NULL) {
		/*
		 * When encountering a .Nm macro, substitute it
		 * with its previously cached value of the argument.
		 */
		mdoc_parse_section(n->sec, rec->name, rec);
	} else if (mdocs[n->tok] == pmdoc_Xr) {
		/*
		 * When encountering other inline macros,
		 * call pmdoc_macro_handler.
		 */
		pmdoc_macro_handler(n, rec, MDOC_Xr);
		xr_found = 1;
	} else if (mdocs[n->tok] == pmdoc_Pp) {
		pmdoc_macro_handler(n, rec, MDOC_Pp);
	}

	/*
	 * If an Xr macro was encountered then the child node has
	 * already been explored by pmdoc_macro_handler.
	 */
	if (xr_found == 0)
		pmdoc_Sh(n->child, rec);
	pmdoc_Sh(n->next, rec);
}

/*
 * mdoc_parse_section--
 *  Utility function for parsing sections of the mdoc type pages.
 *  Takes two params:
 *   1. sec is an enum which indicates the section in which we are present
 *   2. string is the string which we need to append to the secbuff for this
 *      particular section.
 *  The function appends string to the global section buffer and returns.
 */
static void
mdoc_parse_section(enum mdoc_sec sec, const char *string, mandb_rec *rec)
{
	/*
	 * If the user specified the 'l' flag, then parse and store only the
	 * NAME section. Ignore the rest.
	 */
	if (mflags.limit)
		return;

	switch (sec) {
	case SEC_LIBRARY:
		append(&rec->lib, string);
		break;
	case SEC_RETURN_VALUES:
		append(&rec->return_vals, string);
		break;
	case SEC_ENVIRONMENT:
		append(&rec->env, string);
		break;
	case SEC_FILES:
		append(&rec->files, string);
		break;
	case SEC_EXIT_STATUS:
		append(&rec->exit_status, string);
		break;
	case SEC_DIAGNOSTICS:
		append(&rec->diagnostics, string);
		break;
	case SEC_ERRORS:
		append(&rec->errors, string);
		break;
	case SEC_NAME:
	case SEC_SYNOPSIS:
	case SEC_EXAMPLES:
	case SEC_STANDARDS:
	case SEC_HISTORY:
	case SEC_AUTHORS:
	case SEC_BUGS:
		break;
	default:
		append(&rec->desc, string);
		break;
	}
}

static void
pman_node(const struct man_node *n, mandb_rec *rec)
{
	if (n == NULL)
		return;

	switch (n->type) {
	case (MAN_BODY):
		/* FALLTHROUGH */
	case (MAN_TAIL):
		/* FALLTHROUGH */
	case (MAN_BLOCK):
		/* FALLTHROUGH */
	case (MAN_ELEM):
		if (mans[n->tok] != NULL)
			(*mans[n->tok])(n, rec);
		break;
	default:
		break;
	}

	pman_node(n->child, rec);
	pman_node(n->next, rec);
}

/*
 * pman_parse_name --
 *  Parses the NAME section and puts the complete content in the name_desc
 *  variable.
 */
static void
pman_parse_name(const struct man_node *n, mandb_rec *rec)
{
	if (n == NULL)
		return;

	if (n->type == MAN_TEXT) {
		char *tmp = parse_escape(n->string);
		concat(&rec->name_desc, tmp);
		free(tmp);
	}

	if (n->child)
		pman_parse_name(n->child, rec);

	if(n->next)
		pman_parse_name(n->next, rec);
}

/*
 * A stub function to be able to parse the macros like .B embedded inside
 * a section.
 */
static void
pman_block(const struct man_node *n, mandb_rec *rec)
{
}

/*
 * pman_sh --
 * This function does one of the two things:
 *  1. If the present section is NAME, then it will:
 *    (a) Extract the name of the page (in case of multiple comma separated
 *        names, it will pick up the first one).
 *    (b) Build a space spearated list of all the symlinks/hardlinks to
 *        this page and store in the buffer 'links'. These are extracted from
 *        the comma separated list of names in the NAME section as well.
 *    (c) Move on to the one line description section, which is after the list
 *        of names in the NAME section.
 *  2. Otherwise, it will check the section name and call the man_parse_section
 *     function, passing the enum corresponding that section.
 */
static void
pman_sh(const struct man_node *n, mandb_rec *rec)
{
	static const struct {
		enum man_sec section;
		const char *header;
	} mapping[] = {
	    { MANSEC_DESCRIPTION, "DESCRIPTION" },
	    { MANSEC_SYNOPSIS, "SYNOPSIS" },
	    { MANSEC_LIBRARY, "LIBRARY" },
	    { MANSEC_ERRORS, "ERRORS" },
	    { MANSEC_FILES, "FILES" },
	    { MANSEC_RETURN_VALUES, "RETURN VALUE" },
	    { MANSEC_RETURN_VALUES, "RETURN VALUES" },
	    { MANSEC_EXIT_STATUS, "EXIT STATUS" },
	    { MANSEC_EXAMPLES, "EXAMPLES" },
	    { MANSEC_EXAMPLES, "EXAMPLE" },
	    { MANSEC_STANDARDS, "STANDARDS" },
	    { MANSEC_HISTORY, "HISTORY" },
	    { MANSEC_BUGS, "BUGS" },
	    { MANSEC_AUTHORS, "AUTHORS" },
	    { MANSEC_COPYRIGHT, "COPYRIGHT" },
	};
	const struct man_node *head;
	char *name_desc;
	int sz;
	size_t i;

	if ((head = n->parent->head) == NULL || (head = head->child) == NULL ||
	    head->type != MAN_TEXT)
		return;

	/*
	 * Check if this section should be extracted and
	 * where it should be stored. Handled the trival cases first.
	 */
	for (i = 0; i < sizeof(mapping) / sizeof(mapping[0]); ++i) {
		if (strcmp(head->string, mapping[i].header) == 0) {
			man_parse_section(mapping[i].section, n, rec);
			return;
		}
	}

	if (strcmp(head->string, "NAME") == 0) {
		/*
		 * We are in the NAME section.
		 * pman_parse_name will put the complete content in name_desc.
		 */
		pman_parse_name(n, rec);

		name_desc = rec->name_desc;
		if (name_desc == NULL)
			return;

		/* Remove any leading spaces. */
		while (name_desc[0] == ' ')
			name_desc++;

		/* If the line begins with a "\&", avoid those */
		if (name_desc[0] == '\\' && name_desc[1] == '&')
			name_desc += 2;

		/* Now name_desc should be left with a comma-space
		 * separated list of names and the one line description
		 * of the page:
		 *     "a, b, c \- sample description"
		 * Take out the first name, before the first comma
		 * (or space) and store it in rec->name.
		 * If the page has aliases then they should be
		 * in the form of a comma separated list.
		 * Keep looping while there is a comma in name_desc,
		 * extract the alias name and store in rec->links.
		 * When there are no more commas left, break out.
		 */
		int has_alias = 0;	// Any more aliases left?
		while (*name_desc) {
			/* Remove any leading spaces or hyphens. */
			if (name_desc[0] == ' ' || name_desc[0] =='-') {
				name_desc++;
				continue;
			}
			sz = strcspn(name_desc, ", ");

			/* Extract the first term and store it in rec->name. */
			if (rec->name == NULL) {
				if (name_desc[sz] == ',')
					has_alias = 1;
				name_desc[sz] = 0;
				rec->name = emalloc(sz + 1);
				memcpy(rec->name, name_desc, sz + 1);
				name_desc += sz + 1;
				continue;
			}

			/*
			 * Once rec->name is set, rest of the names
			 * are to be treated as links or aliases.
			 */
			if (rec->name && has_alias) {
				if (name_desc[sz] != ',') {
					/* No more commas left -->
					 * no more aliases to take out
					 */
					has_alias = 0;
				}
				name_desc[sz] = 0;
				concat2(&rec->links, name_desc, sz);
				name_desc += sz + 1;
				continue;
			}
			break;
		}

		/* Parse any escape sequences that might be there */
		char *temp = parse_escape(name_desc);
		free(rec->name_desc);
		rec->name_desc = temp;
		temp = parse_escape(rec->name);
		free(rec->name);
		rec->name = temp;
		return;
	}

	/* The RETURN VALUE section might be specified in multiple ways */
	if (strcmp(head->string, "RETURN") == 0 &&
	    head->next != NULL && head->next->type == MAN_TEXT &&
	    (strcmp(head->next->string, "VALUE") == 0 ||
	    strcmp(head->next->string, "VALUES") == 0)) {
		man_parse_section(MANSEC_RETURN_VALUES, n, rec);
		return;
	}

	/*
	 * EXIT STATUS section can also be specified all on one line or on two
	 * separate lines.
	 */
	if (strcmp(head->string, "EXIT") == 0 &&
	    head->next != NULL && head->next->type == MAN_TEXT &&
	    strcmp(head->next->string, "STATUS") == 0) {
		man_parse_section(MANSEC_EXIT_STATUS, n, rec);
		return;
	}

	/* Store the rest of the content in desc. */
	man_parse_section(MANSEC_NONE, n, rec);
}

/*
 * pman_parse_node --
 *  Generic function to iterate through a node. Usually called from
 *  man_parse_section to parse a particular section of the man page.
 */
static void
pman_parse_node(const struct man_node *n, secbuff *s)
{
	if (n == NULL)
		return;

	if (n->type == MAN_TEXT)
		append(s, n->string);

	pman_parse_node(n->child, s);
	pman_parse_node(n->next, s);
}

/*
 * man_parse_section --
 *  Takes two parameters:
 *   sec: Tells which section we are present in
 *   n: Is the present node of the AST.
 * Depending on the section, we call pman_parse_node to parse that section and
 * concatenate the content from that section into the buffer for that section.
 */
static void
man_parse_section(enum man_sec sec, const struct man_node *n, mandb_rec *rec)
{
	/*
	 * If the user sepecified the 'l' flag then just parse
	 * the NAME section, ignore the rest.
	 */
	if (mflags.limit)
		return;

	switch (sec) {
	case MANSEC_LIBRARY:
		pman_parse_node(n, &rec->lib);
		break;
	case MANSEC_RETURN_VALUES:
		pman_parse_node(n, &rec->return_vals);
		break;
	case MANSEC_ENVIRONMENT:
		pman_parse_node(n, &rec->env);
		break;
	case MANSEC_FILES:
		pman_parse_node(n, &rec->files);
		break;
	case MANSEC_EXIT_STATUS:
		pman_parse_node(n, &rec->exit_status);
		break;
	case MANSEC_DIAGNOSTICS:
		pman_parse_node(n, &rec->diagnostics);
		break;
	case MANSEC_ERRORS:
		pman_parse_node(n, &rec->errors);
		break;
	case MANSEC_NAME:
	case MANSEC_SYNOPSIS:
	case MANSEC_EXAMPLES:
	case MANSEC_STANDARDS:
	case MANSEC_HISTORY:
	case MANSEC_BUGS:
	case MANSEC_AUTHORS:
	case MANSEC_COPYRIGHT:
		break;
	default:
		pman_parse_node(n, &rec->desc);
		break;
	}

}

/*
 * insert_into_db --
 *  Inserts the parsed data of the man page in the Sqlite databse.
 *  If any of the values is NULL, then we cleanup and return -1 indicating
 *  an error.
 *  Otherwise, store the data in the database and return 0.
 */
static int
insert_into_db(sqlite3 *db, mandb_rec *rec)
{
	int rc = 0;
	int idx = -1;
	const char *sqlstr = NULL;
	sqlite3_stmt *stmt = NULL;
	char *ln = NULL;
	char *errmsg = NULL;
	long int mandb_rowid;

	/*
	 * At the very minimum we want to make sure that we store
	 * the following data:
	 *   Name, one line description, and the MD5 hash
	 */
	if (rec->name == NULL || rec->name_desc == NULL ||
	    rec->md5_hash == NULL) {
		cleanup(rec);
		return -1;
	}

	/* Write null byte at the end of all the sec_buffs */
	rec->desc.data[rec->desc.offset] = 0;
	rec->lib.data[rec->lib.offset] = 0;
	rec->env.data[rec->env.offset] = 0;
	rec->return_vals.data[rec->return_vals.offset] = 0;
	rec->exit_status.data[rec->exit_status.offset] = 0;
	rec->files.data[rec->files.offset] = 0;
	rec->diagnostics.data[rec->diagnostics.offset] = 0;
	rec->errors.data[rec->errors.offset] = 0;

	/*
	 * In case of a mdoc page: (sorry, no better place to put this code)
	 * parse the comma separated list of names of man pages,
	 * the first name will be stored in the mandb table, rest will be
	 * treated as links and put in the mandb_links table.
	 */
	if (rec->page_type == MDOC) {
		char *tmp;
		rec->links = estrdup(rec->name);
		free(rec->name);
		int sz = strcspn(rec->links, " \0");
		rec->name = emalloc(sz + 1);
		memcpy(rec->name, rec->links, sz);
		if(rec->name[sz - 1] == ',')
			rec->name[sz - 1] = 0;
		else
			rec->name[sz] = 0;
		while (rec->links[sz] == ' ')
			++sz;
		tmp = estrdup(rec->links + sz);
		free(rec->links);
		rec->links = tmp;
	}

/*------------------------ Populate the mandb table---------------------------*/
	sqlstr = "INSERT INTO mandb VALUES (:section, :name, :name_desc, :desc,"
		 " :lib, :return_vals, :env, :files, :exit_status,"
		 " :diagnostics, :errors, :md5_hash, :machine)";

	rc = sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		goto Out;

	idx = sqlite3_bind_parameter_index(stmt, ":name");
	rc = sqlite3_bind_text(stmt, idx, rec->name, -1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":section");
	rc = sqlite3_bind_text(stmt, idx, rec->section, -1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":name_desc");
	rc = sqlite3_bind_text(stmt, idx, rec->name_desc, -1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":desc");
	rc = sqlite3_bind_text(stmt, idx, rec->desc.data,
	                       rec->desc.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":lib");
	rc = sqlite3_bind_text(stmt, idx, rec->lib.data, rec->lib.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":return_vals");
	rc = sqlite3_bind_text(stmt, idx, rec->return_vals.data,
	                      rec->return_vals.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":env");
	rc = sqlite3_bind_text(stmt, idx, rec->env.data, rec->env.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":files");
	rc = sqlite3_bind_text(stmt, idx, rec->files.data,
	                       rec->files.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":exit_status");
	rc = sqlite3_bind_text(stmt, idx, rec->exit_status.data,
	                       rec->exit_status.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":diagnostics");
	rc = sqlite3_bind_text(stmt, idx, rec->diagnostics.data,
	                       rec->diagnostics.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":errors");
	rc = sqlite3_bind_text(stmt, idx, rec->errors.data,
	                       rec->errors.offset + 1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":md5_hash");
	rc = sqlite3_bind_text(stmt, idx, rec->md5_hash, -1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":machine");
	if (rec->machine)
		rc = sqlite3_bind_text(stmt, idx, rec->machine, -1, NULL);
	else
		rc = sqlite3_bind_null(stmt, idx);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	sqlite3_finalize(stmt);

	/* Get the row id of the last inserted row */
	mandb_rowid = sqlite3_last_insert_rowid(db);

/*------------------------Populate the mandb_meta table-----------------------*/
	sqlstr = "INSERT INTO mandb_meta VALUES (:device, :inode, :mtime,"
		 " :file, :md5_hash, :id)";
	rc = sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
		goto Out;

	idx = sqlite3_bind_parameter_index(stmt, ":device");
	rc = sqlite3_bind_int64(stmt, idx, rec->device);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":inode");
	rc = sqlite3_bind_int64(stmt, idx, rec->inode);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":mtime");
	rc = sqlite3_bind_int64(stmt, idx, rec->mtime);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":file");
	rc = sqlite3_bind_text(stmt, idx, rec->file_path, -1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":md5_hash");
	rc = sqlite3_bind_text(stmt, idx, rec->md5_hash, -1, NULL);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":id");
	rc = sqlite3_bind_int64(stmt, idx, mandb_rowid);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		goto Out;
	}

	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc == SQLITE_CONSTRAINT) {
		/* The *most* probable reason for reaching here is that
		 * the UNIQUE contraint on the file column of the mandb_meta
		 * table was violated.
		 * This can happen when a file was updated/modified.
		 * To fix this we need to do two things:
		 * 1. Delete the row for the older version of this file
		 *    from mandb table.
		 * 2. Run an UPDATE query to update the row for this file
		 *    in the mandb_meta table.
		 */
		warnx("Trying to update index for %s", rec->file_path);
		char *sql = sqlite3_mprintf("DELETE FROM mandb "
					    "WHERE rowid = (SELECT id"
					    "  FROM mandb_meta"
					    "  WHERE file = %Q)",
					    rec->file_path);
		sqlite3_exec(db, sql, NULL, NULL, &errmsg);
		sqlite3_free(sql);
		if (errmsg != NULL) {
			if (mflags.verbosity)
				warnx("%s", errmsg);
			free(errmsg);
		}
		sqlstr = "UPDATE mandb_meta SET device = :device,"
			 " inode = :inode, mtime = :mtime, id = :id,"
			 " md5_hash = :md5 WHERE file = :file";
		rc = sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
		if (rc != SQLITE_OK) {
			if (mflags.verbosity)
				warnx("Update failed with error: %s",
			    sqlite3_errmsg(db));
			close_db(db);
			cleanup(rec);
			errx(EXIT_FAILURE,
			    "Consider running makemandb with -f option");
		}

		idx = sqlite3_bind_parameter_index(stmt, ":device");
		sqlite3_bind_int64(stmt, idx, rec->device);
		idx = sqlite3_bind_parameter_index(stmt, ":inode");
		sqlite3_bind_int64(stmt, idx, rec->inode);
		idx = sqlite3_bind_parameter_index(stmt, ":mtime");
		sqlite3_bind_int64(stmt, idx, rec->mtime);
		idx = sqlite3_bind_parameter_index(stmt, ":id");
		sqlite3_bind_int64(stmt, idx, mandb_rowid);
		idx = sqlite3_bind_parameter_index(stmt, ":md5");
		sqlite3_bind_text(stmt, idx, rec->md5_hash, -1, NULL);
		idx = sqlite3_bind_parameter_index(stmt, ":file");
		sqlite3_bind_text(stmt, idx, rec->file_path, -1, NULL);
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);

		if (rc != SQLITE_DONE) {
			if (mflags.verbosity)
				warnx("%s", sqlite3_errmsg(db));
			close_db(db);
			cleanup(rec);
			errx(EXIT_FAILURE,
			    "Consider running makemandb with -f option");
		}
	} else if (rc != SQLITE_DONE) {
		/* Otherwise make this error fatal */
		warnx("Failed at %s\n%s", rec->file_path, sqlite3_errmsg(db));
		cleanup(rec);
		close_db(db);
		exit(EXIT_FAILURE);
	}

/*------------------------ Populate the mandb_links table---------------------*/
	char *str = NULL;
	char *links;
	if (rec->links && strlen(rec->links)) {
		links = rec->links;
		for(ln = strtok(links, " "); ln; ln = strtok(NULL, " ")) {
			if (ln[0] == ',')
				ln++;
			if(ln[strlen(ln) - 1] == ',')
				ln[strlen(ln) - 1] = 0;

			str = sqlite3_mprintf("INSERT INTO mandb_links"
					      " VALUES (%Q, %Q, %Q, %Q, %Q)",
					      ln, rec->name, rec->section,
					      rec->machine, rec->md5_hash);
			sqlite3_exec(db, str, NULL, NULL, &errmsg);
			sqlite3_free(str);
			if (errmsg != NULL) {
				warnx("%s", errmsg);
				cleanup(rec);
				free(errmsg);
				return -1;
			}
		}
	}

	cleanup(rec);
	return 0;

  Out:
	if (mflags.verbosity)
		warnx("%s", sqlite3_errmsg(db));
	cleanup(rec);
	return -1;
}

/*
 * check_md5--
 *  Generates the md5 hash of the file and checks if it already doesn't exist
 *  in the table (passed as the 3rd parameter).
 *  This function is being used to avoid hardlinks.
 *  On successful completion it will also set the value of the fourth parameter
 *  to the md5 hash of the file (computed previously). It is the responsibility
 *  of the caller to free this buffer.
 *  Return values:
 *  -1: If an error occurs somewhere and sets the md5 return buffer to NULL.
 *  0: If the md5 hash does not exist in the table.
 *  1: If the hash exists in the database.
 */
static int
check_md5(const char *file, sqlite3 *db, const char *table, char **md5sum,
    void *buf, size_t buflen)
{
	int rc = 0;
	int idx = -1;
	char *sqlstr = NULL;
	sqlite3_stmt *stmt = NULL;

	assert(file != NULL);
	*md5sum = MD5Data(buf, buflen, NULL);
	if (*md5sum == NULL) {
		if (mflags.verbosity)
			warn("md5 failed: %s", file);
		return -1;
	}

	easprintf(&sqlstr, "SELECT * FROM %s WHERE md5_hash = :md5_hash",
	    table);
	rc = sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		free(sqlstr);
		free(*md5sum);
		*md5sum = NULL;
		return -1;
	}

	idx = sqlite3_bind_parameter_index(stmt, ":md5_hash");
	rc = sqlite3_bind_text(stmt, idx, *md5sum, -1, NULL);
	if (rc != SQLITE_OK) {
		if (mflags.verbosity)
			warnx("%s", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		free(sqlstr);
		free(*md5sum);
		*md5sum = NULL;
		return -1;
	}

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		sqlite3_finalize(stmt);
		free(sqlstr);
		return 0;
	}

	sqlite3_finalize(stmt);
	free(sqlstr);
	return 1;
}

/* Optimize the index for faster search */
static void
optimize(sqlite3 *db)
{
	const char *sqlstr;
	char *errmsg = NULL;

	if (mflags.verbosity == 2)
		printf("Optimizing the database index\n");
	sqlstr = "INSERT INTO mandb(mandb) VALUES (\'optimize\');"
		 "VACUUM";
	sqlite3_exec(db, sqlstr, NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		if (mflags.verbosity)
			warnx("%s", errmsg);
		free(errmsg);
		return;
	}
}

/*
 * cleanup --
 *  cleans up the global buffers
 */
static void
cleanup(mandb_rec *rec)
{
	rec->desc.offset = 0;
	rec->lib.offset = 0;
	rec->return_vals.offset = 0;
	rec->env.offset = 0;
	rec->exit_status.offset = 0;
	rec->diagnostics.offset = 0;
	rec->errors.offset = 0;
	rec->files.offset = 0;

	free(rec->machine);
	rec->machine = NULL;

	free(rec->links);
	rec->links = NULL;

	free(rec->file_path);
	rec->file_path = NULL;

	free(rec->name);
	rec->name = NULL;

	free(rec->name_desc);
	rec->name_desc = NULL;

	free(rec->md5_hash);
	rec->md5_hash = NULL;
}

/*
 * init_secbuffs--
 *  Sets the value of buflen for all the sec_buff field of rec. And then
 *  allocate memory to each sec_buff member of rec.
 */
static void
init_secbuffs(mandb_rec *rec)
{
	/*
	 * Some sec_buff might need more memory, for example desc,
	 * which stores the data of the DESCRIPTION section,
	 * while some might need very small amount of memory.
	 * Therefore explicitly setting the value of buflen field for
	 * each sec_buff.
	 */
	rec->desc.buflen = 10 * BUFLEN;
	rec->desc.data = emalloc(rec->desc.buflen);
	rec->desc.offset = 0;

	rec->lib.buflen = BUFLEN / 2;
	rec->lib.data = emalloc(rec->lib.buflen);
	rec->lib.offset = 0;

	rec->return_vals.buflen = BUFLEN;
	rec->return_vals.data = emalloc(rec->return_vals.buflen);
	rec->return_vals.offset = 0;

	rec->exit_status.buflen = BUFLEN;
	rec->exit_status.data = emalloc(rec->exit_status.buflen);
	rec->exit_status.offset = 0;

	rec->env.buflen = BUFLEN;
	rec->env.data = emalloc(rec->env.buflen);
	rec->env.offset = 0;

	rec->files.buflen = BUFLEN;
	rec->files.data = emalloc(rec->files.buflen);
	rec->files.offset = 0;

	rec->diagnostics.buflen = BUFLEN;
	rec->diagnostics.data = emalloc(rec->diagnostics.buflen);
	rec->diagnostics.offset = 0;

	rec->errors.buflen = BUFLEN;
	rec->errors.data = emalloc(rec->errors.buflen);
	rec->errors.offset = 0;
}

/*
 * free_secbuffs--
 *  This function should be called at the end, when all the pages have been
 *  parsed.
 *  It frees the memory allocated to sec_buffs by init_secbuffs in the starting.
 */
static void
free_secbuffs(mandb_rec *rec)
{
	free(rec->desc.data);
	free(rec->lib.data);
	free(rec->return_vals.data);
	free(rec->exit_status.data);
	free(rec->env.data);
	free(rec->files.data);
	free(rec->diagnostics.data);
	free(rec->errors.data);
}

static void
replace_hyph(char *str)
{
	char *iter = str;
	while ((iter = strchr(iter, ASCII_HYPH)) != NULL)
		*iter = '-';
}

static char *
parse_escape(const char *str)
{
	const char *backslash, *last_backslash;
	char *result, *iter;
	size_t len;

	assert(str);

	last_backslash = str;
	backslash = strchr(str, '\\');
	if (backslash == NULL) {
		result = estrdup(str);
		replace_hyph(result);
		return result;
	}

	result = emalloc(strlen(str) + 1);
	iter = result;

	do {
		len = backslash - last_backslash;
		memcpy(iter, last_backslash, len);
		iter += len;
		if (backslash[1] == '-' || backslash[1] == ' ') {
			*iter++ = backslash[1];
			last_backslash = backslash + 2;
			backslash = strchr(backslash + 2, '\\');
		} else {
			++backslash;
			mandoc_escape(&backslash, NULL, NULL);
			last_backslash = backslash;
			if (backslash == NULL)
				break;
			backslash = strchr(last_backslash, '\\');
		}
	} while (backslash != NULL);
	if (last_backslash != NULL)
		strcpy(iter, last_backslash);

	replace_hyph(result);
	return result;
}

/*
 * append--
 *  Concatenates a space and src at the end of sbuff->data (much like concat in
 *  apropos-utils.c).
 *  Rather than reallocating space for writing data, it uses the value of the
 *  offset field of sec_buff to write new data at the free space left in the
 *  buffer.
 *  In case the size of the data to be appended exceeds the number of bytes left
 *  in the buffer, it reallocates buflen number of bytes and then continues.
 *  Value of offset field should be adjusted as new data is written.
 *
 *  NOTE: This function does not write the null byte at the end of the buffers,
 *  write a null byte at the position pointed to by offset before inserting data
 *  in the db.
 */
static void
append(secbuff *sbuff, const char *src)
{
	short flag = 0;
	size_t srclen, newlen;
	char *temp;

	assert(src != NULL);
	temp = parse_escape(src);
	srclen = strlen(temp);

	if (sbuff->data == NULL) {
		sbuff->data = emalloc(sbuff->buflen);
		sbuff->offset = 0;
	}

	newlen = sbuff->offset + srclen + 2;
	if (newlen >= sbuff->buflen) {
		while (sbuff->buflen < newlen)
			sbuff->buflen += sbuff->buflen;
		sbuff->data = erealloc(sbuff->data, sbuff->buflen);
		flag = 1;
	}

	/* Append a space at the end of the buffer. */
	if (sbuff->offset || flag)
		sbuff->data[sbuff->offset++] = ' ';
	/* Now, copy src at the end of the buffer. */
	memcpy(sbuff->data + sbuff->offset, temp, srclen);
	sbuff->offset += srclen;
	free(temp);
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-floQqv] [-C path]\n", getprogname());
	exit(1);
}
