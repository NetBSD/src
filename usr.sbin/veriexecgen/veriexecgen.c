/* $NetBSD: veriexecgen.c,v 1.10 2006/12/04 21:22:40 agc Exp $ */

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/verified_exec.h>

#include <err.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include <md5.h>
#include <sha1.h>
#include <sha2.h>
#include <rmd160.h>

#define IS_EXEC(mode) ((mode) & (S_IXUSR | S_IXGRP | S_IXOTH))

#define DEFAULT_DBFILE  "/etc/signatures"
#define DEFAULT_HASH    "sha256"
#define DEFAULT_SYSPATHS { "/bin", "/sbin", "/usr/bin", "/usr/sbin", \
			   "/lib", "/usr/lib", "/libexec", "/usr/libexec", \
			   NULL }

/* this struct describes a directory entry to generate a hash for */
struct fentry {
	char filename[MAXPATHLEN];	/* name of entry */
	char *hash_val;			/* its associated hash value */
	int flags;			/* any associated flags */
	TAILQ_ENTRY(fentry) f;		/* its place in the queue */
};
TAILQ_HEAD(, fentry) fehead;

/* this struct defines the possible hash algorithms */
struct hash {
	const char     *hashname;
	char           *(*filefunc) (const char *, char *);
} hashes[] = {
	{ "MD5", MD5File },
	{ "SHA1", SHA1File },
	{ "SHA256", SHA256_File },
	{ "SHA384", SHA384_File },
	{ "SHA512", SHA512_File },
	{ "RMD160", RMD160File },
	{ NULL, NULL },
};

static int Fflag;

static int	exit_on_error;	/* exit if we can't create a hash */
static int	append_output;	/* append output to signatures file */
static int	all_files;	/* scan for non-executable files as well */
static int	scan_system_dirs;	/* scan system directories */
static int	recursive_scan;	/* perform the scan recursively */
static int	make_immutable;	/* set immutable flag on signatures file */
static int	verbose;	/* verbose execution */

/* warn about a problem - exit if exit_on_error is set */
static void
gripe(const char *fmt, const char *filename)
{
	warn(fmt, filename);
	if (exit_on_error) {
		/* error out on problematic files */
		exit(EXIT_FAILURE);
	}
}

/* print usage message */
static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-AaDrSvW] [-d dir] [-o fingerprintdb]"
	    " [-t algorithm]\n", getprogname());
}

/* tell people what we're doing - scan dirs, fingerprint etc */
static void
banner(struct hash *hash_type, char **search_path)
{
	int j;

	(void)printf("Fingerprinting ");

	for (j = 0; search_path[j] != NULL; j++)
		(void)printf("%s ", search_path[j]);

	(void)printf("(%s) (%s) using %s\n",
	    all_files ? "all files" : "executables only",
	    recursive_scan ? "recursive" : "non-recursive",
	    hash_type->hashname);
}

/* find a hash algorithm, given its name */
static struct hash *
find_hash(char *hash_type)
{
	struct hash *hash;

	for (hash = hashes; hash->hashname != NULL; hash++)
		if (strcasecmp(hash_type, hash->hashname) == 0)
			return hash;
	return NULL;
}

/* perform the hashing operation on `filename' */
static char *
do_hash(char *filename, struct hash * h)
{
	return h->filefunc(filename, NULL);
}

/* return flags for `path' */
static int
figure_flags(char *path, mode_t mode)
{
#ifdef notyet
	if (Fflag) {
		/* Try to figure out right flag(s). */
		return VERIEXEC_DIRECT;
	}
#endif /* notyet */

	return (IS_EXEC(mode)) ? 0 : VERIEXEC_FILE;
}

/* check to see that we don't have a duplicate entry */
static int
check_dup(char *filename)
{
	struct fentry *lwalk;

	TAILQ_FOREACH(lwalk, &fehead, f) {
		if (strncmp(lwalk->filename, filename,
			    (unsigned long) MAXPATHLEN) == 0)
			return 1;
	}

	return 0;
}

/* add a new entry to the list for `file' */
static void
add_new_entry(FTSENT *file, struct hash *hash)
{
	struct fentry *e;
	struct stat sb;

	if (file->fts_info == FTS_SL) {
		/* we have a symbolic link */
		if (stat(file->fts_path, &sb) == -1) {
			gripe("Cannot stat symlink `%s'", file->fts_path);
			return;
		}
	} else
		sb = *file->fts_statp;

	if (!all_files && !scan_system_dirs && !IS_EXEC(sb.st_mode))
		return;

	e = ecalloc(1UL, sizeof(*e));

	if (realpath(file->fts_accpath, e->filename) == NULL) {
		gripe("Cannot find absolute path `%s'", file->fts_accpath);
		return;
	}
	if (check_dup(e->filename)) {
		free(e);
		return;
	}
	if ((e->hash_val = do_hash(e->filename, hash)) == NULL) {
		gripe("Cannot calculate hash `%s'", e->filename);
		return;
	}
	e->flags = figure_flags(e->filename, sb.st_mode);

	TAILQ_INSERT_TAIL(&fehead, e, f);
}

/* walk through a directory */
static void
walk_dir(char **search_path, struct hash *hash)
{
	FTS *fh;
	FTSENT *file;

	if ((fh = fts_open(search_path, FTS_PHYSICAL, NULL)) == NULL) {
		gripe("fts_open `%s'", (const char *)search_path);
		return;
	}

	while ((file = fts_read(fh)) != NULL) {
		if (!recursive_scan && file->fts_level > 1) {
			fts_set(fh, file, FTS_SKIP);
			continue;
		}

		switch (file->fts_info) {
		case FTS_D:
		case FTS_DC:
		case FTS_DP:
			continue;
		default:
			break;
		}

		if (file->fts_errno) {
			if (exit_on_error) {
				errx(EXIT_FAILURE, "%s: %s", file->fts_path,
				    strerror(file->fts_errno));
			}
		} else {
			add_new_entry(file, hash);
		}
	}

	fts_close(fh);
}

/* return a string representation of the flags */
static char *
flags2str(int flags)
{
	return (flags == 0) ? "" : "FILE, INDIRECT";
}

/* store the list in the signatures file */
static void
store_entries(char *dbfile, struct hash *hash)
{
	FILE *fp;
	int move = 1;
	char old_dbfile[MAXPATHLEN];
	time_t ct;
	struct stat sb;
	struct fentry  *e;

	if (stat(dbfile, &sb) != 0) {
		if (errno == ENOENT)
			move = 0;
		else
			err(EXIT_FAILURE, "could not stat %s", dbfile);
	}
	if (move && !append_output) {
		if (verbose)
			(void)printf("\nBacking up existing fingerprint file "
			    "to \"%s.old\"\n\n", dbfile);

		if (snprintf(old_dbfile, MAXPATHLEN, "%s.old", dbfile) <
		    strlen(dbfile) + 4) {
			err(EXIT_FAILURE, "%s", old_dbfile);
		}
		if (rename(dbfile, old_dbfile) == -1)
			err(EXIT_FAILURE, "could not rename file");
	}

	fp = efopen(dbfile, append_output ? "a" : "w+");

	time(&ct);
	(void)fprintf(fp, "# Generated by %s, %.24s\n",
		getlogin(), ctime(&ct));

	TAILQ_FOREACH(e, &fehead, f) {
		if (verbose)
			(void)printf("Adding %s.\n", e->filename);

		(void)fprintf(fp, "%s %s %s %s\n", e->filename,
			hash->hashname, e->hash_val, flags2str(e->flags));
	}

	(void)fclose(fp);

	if (verbose) {
		(void)printf("\n\n"
"#############################################################\n"
"  PLEASE VERIFY CONTENTS OF %s AND FINE-TUNE THE\n"
"  FLAGS WHERE APPROPRIATE AFTER READING veriexecctl(8)\n"
"#############################################################\n",
			dbfile);
	}
}

int
main(int argc, char **argv)
{
	int ch, total = 0;
	char *dbfile = NULL;
	char **search_path = NULL;
	struct hash *hash = NULL;

	append_output = 0;
	all_files = 0;
	scan_system_dirs = 0;
	recursive_scan = 0;
	make_immutable = 0;
	verbose = 0;
	Fflag = 0;

	/* error out if we have a dangling symlink or other fs problem */
	exit_on_error = 1;

	while ((ch = getopt(argc, argv, "AaDd:ho:rSt:vW")) != -1) {
		switch (ch) {
		case 'A':
			append_output = 1;
			break;
		case 'a':
			all_files = 1;
			break;
		case 'D':
			scan_system_dirs = 1;
			break;
		case 'd':
			search_path = erealloc(search_path, sizeof(char *) *
			    (total + 1));
			search_path[total] = optarg;
			search_path[++total] = NULL;
			break;
#ifdef notyet
		case 'F':
			Fflag = 1;
			break;
#endif /* notyet */
		case 'h':
			usage();
			return EXIT_SUCCESS;
		case 'o':
			dbfile = optarg;
			break;
		case 'r':
			recursive_scan = 1;
			break;
		case 'S':
			make_immutable = 1;
			break;
		case 't':
			if ((hash = find_hash(optarg)) == NULL) {
				errx(EXIT_FAILURE,
					"No such hash algorithm (%s)",
					optarg);
			}
			break;
		case 'v':
			verbose = 1;
			break;
		case 'W':
			exit_on_error = 0;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	if (dbfile == NULL)
		dbfile = DEFAULT_DBFILE;

	if (hash == NULL) {
		if ((hash = find_hash(DEFAULT_HASH)) == NULL)
			errx(EXIT_FAILURE, "No hash algorithm");
	}

	TAILQ_INIT(&fehead);

	if (search_path == NULL)
		scan_system_dirs = 1;

	if (scan_system_dirs) {
		char *sys_paths[] = DEFAULT_SYSPATHS;

		if (verbose)
			banner(hash, sys_paths);
		walk_dir(sys_paths, hash);
	}

	if (search_path != NULL) {
		if (verbose)
			banner(hash, search_path);
		walk_dir(search_path, hash);
	}

	store_entries(dbfile, hash);

	if (make_immutable && chflags(dbfile, SF_IMMUTABLE) != 0)
		err(EXIT_FAILURE, "Can't set immutable flag");

	return EXIT_SUCCESS;
}
