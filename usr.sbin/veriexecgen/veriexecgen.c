/* $NetBSD: veriexecgen.c,v 1.6 2006/09/23 19:08:48 elad Exp $ */

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
#include <crypto/sha2.h>
#include <crypto/rmd160.h>

#define IS_EXEC(mode) ((mode) & (S_IXUSR | S_IXGRP | S_IXOTH))

#define DEFAULT_DBFILE  "/etc/signatures"
#define DEFAULT_HASH    "sha256"
#define DEFAULT_SYSPATHS { "/bin", "/sbin", "/usr/bin", "/usr/sbin", \
			   "/lib", "/usr/lib", "/libexec", "/usr/libexec", \
			   NULL }

struct fentry {
	char filename[MAXPATHLEN];
	char *hash_val;
	int flags;
	TAILQ_ENTRY(fentry) f;
};
TAILQ_HEAD(, fentry) fehead;

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

int Aflag, aflag, Dflag, Fflag, rflag, Sflag, vflag;

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-AaDrSv] [-d dir] [-o fingerprintdb]"
	    " [-t algorithm]\n", getprogname());
}

static void
banner(struct hash *hash_type, char **search_path)
{
	int j;

	(void)printf("Fingerprinting ");

	for (j = 0; search_path[j] != NULL; j++)
		(void)printf("%s ", search_path[j]);

	(void)printf("(%s) (%s) using %s\n",
	    aflag ? "all files" : "executables only",
	    rflag ? "recursive" : "non-recursive",
	    hash_type->hashname);
}

static struct hash *
find_hash(char *hash_type)
{
	struct hash *hash;

	for (hash = hashes; hash->hashname != NULL; hash++)
		if (strcasecmp(hash_type, hash->hashname) == 0)
			return hash;
	return NULL;
}

static char *
do_hash(char *filename, struct hash * h)
{
	return h->filefunc(filename, NULL);
}

static int
figure_flags(char *path, mode_t mode)
{
#ifdef notyet
	if (Fflag) {
		/* Try to figure out right flag(s). */
		return VERIEXEC_DIRECT;
}
#endif /* notyet */

	if (!IS_EXEC(mode))
		return VERIEXEC_FILE;
	else
		return 0;
}

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

static void
add_new_entry(FTSENT *file, struct hash *hash)
{
	struct fentry *e;
	struct stat sb;

	if (file->fts_info == FTS_SL) {
		if (stat(file->fts_path, &sb) == -1)
			err(1, "Cannot stat symlink");
	} else
		sb = *file->fts_statp;

	if (!aflag && !Dflag && !IS_EXEC(sb.st_mode))
		return;

	e = ecalloc(1UL, sizeof(*e));

	if (realpath(file->fts_accpath, e->filename) == NULL)
		err(1, "Cannot find absolute path");
	if (check_dup(e->filename)) {
		free(e);
		return;
	}
	if ((e->hash_val = do_hash(e->filename, hash)) == NULL)
		errx(1, "Cannot calculate hash");
	e->flags = figure_flags(e->filename, sb.st_mode);

	TAILQ_INSERT_TAIL(&fehead, e, f);
}

static void
walk_dir(char **search_path, struct hash *hash)
{
	FTS *fh;
	FTSENT *file;

	if ((fh = fts_open(search_path, FTS_PHYSICAL, NULL)) == NULL)
		err(1, "fts_open");

	while ((file = fts_read(fh)) != NULL) {
		if (!rflag && file->fts_level > 1) {
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
			errx(1, "%s: %s", file->fts_path,
			    strerror(file->fts_errno));
		}

		add_new_entry(file, hash);
	}

	fts_close(fh);
}

static char *
flags2str(int flags)
{
	if (flags != 0)
		return "FILE";
	else
		return "";
}

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
			err(1, "could not stat %s", dbfile);
	}
	if (move && !Aflag) {
		if (vflag)
			(void)printf("\nBacking up existing fingerprint file "
			    "to \"%s.old\"\n\n", dbfile);

		if (snprintf(old_dbfile, MAXPATHLEN, "%s.old", dbfile) <
		    strlen(dbfile) + 4) {
			err(1, "%s", old_dbfile);
		}
		if (rename(dbfile, old_dbfile) == -1)
			err(1, "could not rename file");
	}

	fp = efopen(dbfile, Aflag ? "a" : "w+");

	time(&ct);
	(void)fprintf(fp, "# Generated by %s, %.24s\n",
		getlogin(), ctime(&ct));

	TAILQ_FOREACH(e, &fehead, f) {
		if (vflag)
			(void)printf("Adding %s.\n", e->filename);

		(void)fprintf(fp, "%s %s %s %s\n", e->filename,
			hash->hashname, e->hash_val, flags2str(e->flags));
	}

	(void)fclose(fp);

	if (!vflag)
		return;

	(void)printf("\n\n"
	   "#############################################################\n"
	   "  PLEASE VERIFY CONTENTS OF %s AND FINE-TUNE THE\n"
	   "  FLAGS WHERE APPROPRIATE AFTER READING veriexecctl(8)\n"
	   "#############################################################\n",
	   dbfile);
}

int
main(int argc, char **argv)
{
	int ch, total = 0;
	char *dbfile = NULL;
	char **search_path = NULL;
	struct hash *hash = NULL;

	Aflag = aflag = Dflag = Fflag = rflag = Sflag = vflag = 0;

	while ((ch = getopt(argc, argv, "AaDd:ho:rSt:v")) != -1) {
		switch (ch) {
		case 'A':
			Aflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'D':
			Dflag = 1;
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
			return 0;
		case 'o':
			dbfile = optarg;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'S':
			Sflag = 1;
			break;
		case 't':
			hash = find_hash(optarg);
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (dbfile == NULL)
		dbfile = DEFAULT_DBFILE;

	if (hash == NULL) {
		if ((hash = find_hash(DEFAULT_HASH)) == NULL)
			errx(1, "No hash algorithm");
	}

	TAILQ_INIT(&fehead);

	if (search_path == NULL)
		Dflag = 1;

	if (Dflag) {
		char *sys_paths[] = DEFAULT_SYSPATHS;

		if (vflag)
			banner(hash, sys_paths);
		walk_dir(sys_paths, hash);
	}

	if (search_path != NULL) {
		if (vflag)
			banner(hash, search_path);
		walk_dir(search_path, hash);
	}

	store_entries(dbfile, hash);

	if (Sflag && chflags(dbfile, SF_IMMUTABLE) != 0)
		err(1, "Can't set immutable flag");

	return 0;
}
