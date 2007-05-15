/* $NetBSD: veriexecgen.c,v 1.15 2007/05/15 19:47:47 elad Exp $ */

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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>

#ifndef lint
#ifdef __RCSID
__RCSID("$NetBSD: veriexecgen.c,v 1.15 2007/05/15 19:47:47 elad Exp $");
#endif
#endif /* not lint */

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

/* this struct defines a hash algorithm */
typedef struct hash_t {
	const char	*hashname;	/* algorithm name */
	char		*(*filefunc)(const char *, char *); /* function */
} hash_t;

/* this struct encapsulates various diverse options and arguments */
typedef struct veriexecgen_t {
	int	 all_files;	/* scan also for non-executable files */
	int	 append_output;	/* append output to existing sigs file */
	char	*dbfile;	/* name of signatures database file */
	int	 exit_on_error;	/* exit if we can't create a hash */
	char	*prefix;	/* any prefix to be discarded on output */
	int	 recursive_scan;/* perform scan for files recursively */
	int	 scan_system_dirs;	/* just scan system directories */
	int	 verbose;	/* verbosity level */
	int	 stamp;		/* put a timestamp */
} veriexecgen_t;

/* this struct describes a directory entry to generate a hash for */
struct fentry {
	char filename[MAXPATHLEN];	/* name of entry */
	char *hash_val;			/* its associated hash value */
	int flags;			/* any associated flags */
	TAILQ_ENTRY(fentry) f;		/* its place in the queue */
};
TAILQ_HEAD(, fentry) fehead;

/* define the possible hash algorithms */
static hash_t	 hashes[] = {
	{ "MD5", MD5File },
	{ "SHA1", SHA1File },
	{ "SHA256", SHA256_File },
	{ "SHA384", SHA384_File },
	{ "SHA512", SHA512_File },
	{ "RMD160", RMD160File },
	{ NULL, NULL },
};

static int Fflag;

static int	make_immutable;	/* set immutable flag on signatures file */

/* warn about a problem - exit if exit_on_error is set */
static void
gripe(veriexecgen_t *vp, const char *fmt, const char *filename)
{
	warn(fmt, filename);
	if (vp->exit_on_error) {
		/* error out on problematic files */
		exit(EXIT_FAILURE);
	}
}

/* print usage message */
static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage:  %s [-AaDrSvW] [-d dir] [-o fingerprintdb] [-p prefix]\n"
	    "\t\t    [-t algorithm]\n"
	    "\t%s [-h]\n", getprogname(), getprogname());
}

/* tell people what we're doing - scan dirs, fingerprint etc */
static void
banner(veriexecgen_t *vp, hash_t *hash_type, char **search_path)
{
	int j;

	(void)printf("Fingerprinting ");

	for (j = 0; search_path[j] != NULL; j++)
		(void)printf("%s ", search_path[j]);

	(void)printf("(%s) (%s) using %s\n",
	    vp->all_files ? "all files" : "executables only",
	    vp->recursive_scan ? "recursive" : "non-recursive",
	    hash_type->hashname);
}

/* find a hash algorithm, given its name */
static hash_t *
find_hash(char *hash_type)
{
	hash_t *hash;

	for (hash = hashes; hash->hashname != NULL; hash++)
		if (strcasecmp(hash_type, hash->hashname) == 0)
			return hash;
	return NULL;
}

/* perform the hashing operation on `filename' */
static char *
do_hash(char *filename, hash_t * h)
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
		if (strcmp(lwalk->filename, filename) == 0)
			return 1;
	}

	return 0;
}

/* add a new entry to the list for `file' */
static void
add_new_entry(veriexecgen_t *vp, FTSENT *file, hash_t *hash)
{
	struct fentry *e;
	struct stat sb;

	if (file->fts_info == FTS_SL) {
		/* we have a symbolic link */
		if (stat(file->fts_path, &sb) == -1) {
			gripe(vp, "Cannot stat symlink `%s'", file->fts_path);
			return;
		}
	} else
		sb = *file->fts_statp;

	if (!vp->all_files && !vp->scan_system_dirs && !IS_EXEC(sb.st_mode))
		return;

	e = ecalloc(1UL, sizeof(*e));

	if (realpath(file->fts_accpath, e->filename) == NULL) {
		gripe(vp, "Cannot find absolute path `%s'", file->fts_accpath);
		return;
	}
	if (check_dup(e->filename)) {
		free(e);
		return;
	}
	if ((e->hash_val = do_hash(e->filename, hash)) == NULL) {
		gripe(vp, "Cannot calculate hash `%s'", e->filename);
		return;
	}
	e->flags = figure_flags(e->filename, sb.st_mode);

	TAILQ_INSERT_TAIL(&fehead, e, f);
}

/* walk through a directory */
static void
walk_dir(veriexecgen_t *vp, char **search_path, hash_t *hash)
{
	FTS *fh;
	FTSENT *file;

	if ((fh = fts_open(search_path, FTS_PHYSICAL, NULL)) == NULL) {
		gripe(vp, "fts_open `%s'", (const char *)search_path);
		return;
	}

	while ((file = fts_read(fh)) != NULL) {
		if (!vp->recursive_scan && file->fts_level > 1) {
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
			if (vp->exit_on_error) {
				errx(EXIT_FAILURE, "%s: %s", file->fts_path,
				    strerror(file->fts_errno));
			}
		} else {
			add_new_entry(vp, file, hash);
		}
	}

	fts_close(fh);
}

/* return a string representation of the flags */
static char *
flags2str(int flags)
{
	return (flags == 0) ? "" : "file, indirect";
}

static char *
escape(const char *s)
{
	char *q, *p;
	size_t len;

	len = strlen(s);
	if (len >= MAXPATHLEN)
		return (NULL);

	len *= 2;
	q = p = calloc(1, len + 1);

	while (*s) {
		if (*s == ' ' || *s == '\t')
			*p++ = '\\';

		*p++ = *s++;
	}

	return (q);
}

/* store the list in the signatures file */
static void
store_entries(veriexecgen_t *vp, hash_t *hash)
{
	FILE *fp;
	int move = 1;
	char old_dbfile[MAXPATHLEN];
	time_t ct;
	struct stat sb;
	struct fentry  *e;
	int	prefixc;

	if (stat(vp->dbfile, &sb) != 0) {
		if (errno == ENOENT)
			move = 0;
		else
			err(EXIT_FAILURE, "could not stat %s", vp->dbfile);
	}
	if (move && !vp->append_output) {
		if (vp->verbose)
			(void)printf("\nBacking up existing fingerprint file "
			    "to \"%s.old\"\n\n", vp->dbfile);

		if (snprintf(old_dbfile, sizeof(old_dbfile), "%s.old",
		    vp->dbfile) < strlen(vp->dbfile) + 4) {
			err(EXIT_FAILURE, "%s", old_dbfile);
		}
		if (rename(vp->dbfile, old_dbfile) == -1)
			err(EXIT_FAILURE, "could not rename file");
	}

	prefixc = (vp->prefix == NULL) ? -1 : strlen(vp->prefix);

	fp = efopen(vp->dbfile, vp->append_output ? "a" : "w+");

	if (vp->stamp) {
		time(&ct);
		(void)fprintf(fp, "# Generated by %s, %.24s\n",
			getlogin(), ctime(&ct));
	}

	TAILQ_FOREACH(e, &fehead, f) {
		char *file;

		if (vp->verbose)
			(void)printf("Adding %s.\n", e->filename);

		file = (prefixc < 0) ? e->filename : &e->filename[prefixc];
		file = escape(file);

		(void)fprintf(fp, "%s %s %s %s\n", file, hash->hashname,
		    e->hash_val, flags2str(e->flags));

		free(file);
	}

	(void)fclose(fp);

	if (vp->verbose) {
		(void)printf("\n\n"
"#############################################################\n"
"  PLEASE VERIFY CONTENTS OF %s AND FINE-TUNE THE\n"
"  FLAGS WHERE APPROPRIATE AFTER READING veriexecctl(8)\n"
"#############################################################\n",
			vp->dbfile);
	}
}

int
main(int argc, char **argv)
{
	int ch, total = 0;
	char **search_path = NULL;
	hash_t *hash = NULL;
	veriexecgen_t	v;

	(void) memset(&v, 0x0, sizeof(v));
	make_immutable = 0;
	Fflag = 0;

	/* error out if we have a dangling symlink or other fs problem */
	v.exit_on_error = 1;

	while ((ch = getopt(argc, argv, "AaDd:ho:p:rSt:vW")) != -1) {
		switch (ch) {
		case 'A':
			v.append_output = 1;
			break;
		case 'a':
			v.all_files = 1;
			break;
		case 'D':
			v.scan_system_dirs = 1;
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
			v.dbfile = optarg;
			break;
		case 'p':
			v.prefix = optarg;
			break;
		case 'r':
			v.recursive_scan = 1;
			break;
		case 'S':
			make_immutable = 1;
			break;
		case 'T':
			v.stamp = 1;
			break;
		case 't':
			if ((hash = find_hash(optarg)) == NULL) {
				errx(EXIT_FAILURE,
					"No such hash algorithm (%s)",
					optarg);
			}
			break;
		case 'v':
			v.verbose = 1;
			break;
		case 'W':
			v.exit_on_error = 0;
			break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	if (v.dbfile == NULL)
		v.dbfile = DEFAULT_DBFILE;

	if (hash == NULL) {
		if ((hash = find_hash(DEFAULT_HASH)) == NULL)
			errx(EXIT_FAILURE, "No hash algorithm");
	}

	TAILQ_INIT(&fehead);

	if (search_path == NULL)
		v.scan_system_dirs = 1;

	if (v.scan_system_dirs) {
		char *sys_paths[] = DEFAULT_SYSPATHS;

		if (v.verbose)
			banner(&v, hash, sys_paths);
		walk_dir(&v, sys_paths, hash);
	}

	if (search_path != NULL) {
		if (v.verbose)
			banner(&v, hash, search_path);
		walk_dir(&v, search_path, hash);
	}

	store_entries(&v, hash);

	if (make_immutable && chflags(v.dbfile, SF_IMMUTABLE) != 0)
		err(EXIT_FAILURE, "Can't set immutable flag");

	return EXIT_SUCCESS;
}
