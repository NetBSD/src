/*	$NetBSD: main.c,v 1.24.2.6 2003/09/21 10:32:44 tron Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.24.2.6 2003/09/21 10:32:44 tron Exp $");
#endif

/*
 * Copyright (c) 1999 Hubert Feyrer.  All rights reserved.
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
 *      This product includes software developed by Hubert Feyrer for
 *	the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <md5.h>
#include <limits.h>
#include <stdio.h>

#include "lib.h"

#define DEFAULT_SFX	".t[bg]z"	/* default suffix for ls{all,best} */

static const char Options[] = "K:SVbd:s:";

int     filecnt;
int     pkgcnt;

static int checkpattern_fn(const char *, void *);

/* print usage message and exit */
static void 
usage(const char *prog)
{
	(void) fprintf(stderr, "usage: %s [-b] [-d lsdir] [-V] [-s sfx] command args ...\n"
	    "Where 'commands' and 'args' are:\n"
	    " rebuild                     - rebuild pkgdb from +CONTENTS files\n"
	    " check [pkg ...]             - check md5 checksum of installed files\n"
	    " add pkg ...                 - add pkg files to database\n"
	    " delete pkg ...              - delete file entries for pkg in database\n"
#ifdef PKGDB_DEBUG
	    " addkey key value            - add key and value\n"
	    " delkey key                  - delete reference to key\n"
#endif
	    " lsall /path/to/pkgpattern   - list all pkgs matching the pattern\n"
	    " lsbest /path/to/pkgpattern  - list pkgs matching the pattern best\n"
	    " dump                        - dump database\n"
	    " pmatch pattern pkg          - returns true if pkg matches pattern, otherwise false\n",
	    prog);
	exit(EXIT_FAILURE);
}

/*
 * Assumes CWD is in /var/db/pkg/<pkg>!
 */
static void 
check1pkg(const char *pkgdir)
{
	FILE   *f;
	plist_t *p;
	package_t Plist;
	char   *PkgName, *dirp = NULL, *md5file;
	char    file[FILENAME_MAX];
	char    dir[FILENAME_MAX];

	f = fopen(CONTENTS_FNAME, "r");
	if (f == NULL)
		err(EXIT_FAILURE, "can't open %s/%s/%s", _pkgdb_getPKGDB_DIR(), pkgdir, CONTENTS_FNAME);

	Plist.head = Plist.tail = NULL;
	read_plist(&Plist, f);
	p = find_plist(&Plist, PLIST_NAME);
	if (p == NULL)
		errx(EXIT_FAILURE, "Package %s has no @name, aborting.",
		    pkgdir);
	PkgName = p->name;
	for (p = Plist.head; p; p = p->next) {
		switch (p->type) {
		case PLIST_FILE:
			if (dirp == NULL) {
				warnx("dirp not initialized, please send-pr!");
				abort();
			}
			
			(void) snprintf(file, sizeof(file), "%s/%s", dirp, p->name);

			if (!(isfile(file) || islinktodir(file)))
				warnx("%s: File %s is in %s but not on filesystem!", PkgName, file, CONTENTS_FNAME);
			else {
				if (p->next &&
				    p->next->type == PLIST_COMMENT &&
				    strncmp(p->next->name, CHECKSUM_HEADER, ChecksumHeaderLen) == 0) {	/* || PLIST_MD5 - HF */
					if ((md5file = MD5File(file, NULL)) != NULL) {
						/* Mismatch? */
#ifdef PKGDB_DEBUG
						printf("%s: md5 should=<%s>, is=<%s>\n",
						    file, p->next->name + ChecksumHeaderLen, md5file);
#endif
						if (strcmp(md5file, p->next->name + ChecksumHeaderLen) != 0)
							printf("%s fails MD5 checksum\n", file);

						free(md5file);
					}
				}
				
				filecnt++;
			}
			break;
		case PLIST_CWD:
			if (strcmp(p->name, ".") != 0)
				dirp = p->name;
			else {
				(void) snprintf(dir, sizeof(dir), "%s/%s", _pkgdb_getPKGDB_DIR(), pkgdir);
				dirp = dir;
			}
			break;
		case PLIST_IGNORE:
			p = p->next;
			break;
		case PLIST_SHOW_ALL:
		case PLIST_SRC:
		case PLIST_CMD:
		case PLIST_CHMOD:
		case PLIST_CHOWN:
		case PLIST_CHGRP:
		case PLIST_COMMENT:
		case PLIST_NAME:
		case PLIST_UNEXEC:
		case PLIST_DISPLAY:
		case PLIST_PKGDEP:
		case PLIST_MTREE:
		case PLIST_DIR_RM:
		case PLIST_IGNORE_INST:
		case PLIST_OPTION:
		case PLIST_PKGCFL:
		case PLIST_BLDDEP:
			break;
		}
	}
	free_plist(&Plist);
	fclose(f);
	pkgcnt++;
}

/*
 * add1pkg(<pkg>)
 *	adds the files listed in the +CONTENTS of <pkg> into the
 *	pkgdb.byfile.db database file in the current package dbdir.  It
 *	returns the number of files added to the database file.
 */
static int
add1pkg(const char *pkgdir)
{
	FILE	       *f;
	plist_t	       *p;
	package_t	Plist;
	char 		contents[FILENAME_MAX];
	char	       *PkgDBDir, *PkgName, *dirp;
	char 		file[FILENAME_MAX];
	char		dir[FILENAME_MAX];
	int		cnt = 0;

	if (!pkgdb_open(ReadWrite))
		err(EXIT_FAILURE, "cannot open pkgdb");

	PkgDBDir = _pkgdb_getPKGDB_DIR();
	(void) snprintf(contents, sizeof(contents), "%s/%s", PkgDBDir, pkgdir);
	if (!(isdir(contents) || islinktodir(contents)))
		errx(EXIT_FAILURE, "`%s' does not exist.", contents);

	(void) strlcat(contents, "/", sizeof(contents));
	(void) strlcat(contents, CONTENTS_FNAME, sizeof(contents));
	if ((f = fopen(contents, "r")) == NULL)
		errx(EXIT_FAILURE, "%s: can't open `%s'", pkgdir, CONTENTS_FNAME);

	Plist.head = Plist.tail = NULL;
	read_plist(&Plist, f);
	if ((p = find_plist(&Plist, PLIST_NAME)) == NULL) {
		errx(EXIT_FAILURE, "Package `%s' has no @name, aborting.", pkgdir);
	}

	PkgName = p->name;
	dirp = NULL;
	for (p = Plist.head; p; p = p->next) {
		switch(p->type) {
		case PLIST_FILE:
			if (dirp == NULL) {
				errx(EXIT_FAILURE, "@cwd not yet found, please send-pr!");
			}
			(void) snprintf(file, sizeof(file), "%s/%s", dirp, p->name);
			if (!(isfile(file) || islinktodir(file))) {
				warnx("%s: File `%s' is in %s but not on filesystem!",
					PkgName, file, CONTENTS_FNAME);
			} else {
				pkgdb_store(file, PkgName);
				cnt++;
			}
			break;
		case PLIST_CWD:
			if (strcmp(p->name, ".") != 0) {
				dirp = p->name;
			} else {
				(void) snprintf(dir, sizeof(dir), "%s/%s", PkgDBDir, pkgdir);
				dirp = dir;
			}
			break;
		case PLIST_IGNORE:
			p = p->next;
			break;
		case PLIST_SHOW_ALL:
		case PLIST_SRC:
		case PLIST_CMD:
		case PLIST_CHMOD:
		case PLIST_CHOWN:
		case PLIST_CHGRP:
		case PLIST_COMMENT:
		case PLIST_NAME:
		case PLIST_UNEXEC:
		case PLIST_DISPLAY:
		case PLIST_PKGDEP:
		case PLIST_MTREE:
		case PLIST_DIR_RM:
		case PLIST_IGNORE_INST:
		case PLIST_OPTION:
		case PLIST_PKGCFL:
		case PLIST_BLDDEP:
			break;
		}
	}
	free_plist(&Plist);
	fclose(f);
	pkgdb_close();

	return cnt;
}

static void
delete1pkg(const char *pkgdir)
{
	if (!pkgdb_open(ReadWrite))
		err(EXIT_FAILURE, "cannot open pkgdb");
	(void) pkgdb_remove_pkg(pkgdir);
	pkgdb_close();
}

static void 
rebuild(void)
{
	DIR	       *dp;
	struct dirent  *de;
	char	       *PkgDBDir;
	char		cachename[FILENAME_MAX];

	pkgcnt = 0;
	filecnt = 0;

	(void) _pkgdb_getPKGDB_FILE(cachename, sizeof(cachename));
	if (unlink(cachename) != 0 && errno != ENOENT)
		err(EXIT_FAILURE, "unlink %s", cachename);

	setbuf(stdout, NULL);
	PkgDBDir = _pkgdb_getPKGDB_DIR();
	chdir(PkgDBDir);
#ifdef PKGDB_DEBUG
	printf("PkgDBDir='%s'\n", PkgDBDir);
#endif
	dp = opendir(".");
	if (dp == NULL)
		err(EXIT_FAILURE, "opendir failed");
	while ((de = readdir(dp))) {
		if (!(isdir(de->d_name) || islinktodir(de->d_name)))
			continue;

		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;

#ifdef PKGDB_DEBUG
		printf("%s\n", de->d_name);
#else
		printf(".");
#endif

		filecnt += add1pkg(de->d_name);
		pkgcnt++;
	}
	chdir("..");
	closedir(dp);

	printf("\n");
	printf("Stored %d file%s from %d package%s in %s.\n",
	    filecnt, filecnt == 1 ? "" : "s",
	    pkgcnt, pkgcnt == 1 ? "" : "s",
	    cachename);
}

static void 
checkall(void)
{
	DIR    *dp;
	struct dirent *de;

	pkgcnt = 0;
	filecnt = 0;

	setbuf(stdout, NULL);
	chdir(_pkgdb_getPKGDB_DIR());

	dp = opendir(".");
	if (dp == NULL)
		err(EXIT_FAILURE, "opendir failed");
	while ((de = readdir(dp))) {
		if (!(isdir(de->d_name) || islinktodir(de->d_name)))
			continue;

		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;

		chdir(de->d_name);

		check1pkg(de->d_name);
		printf(".");

		chdir("..");
	}
	closedir(dp);
	pkgdb_close();


	printf("\n");
	printf("Checked %d file%s from %d package%s.\n",
	    filecnt, (filecnt == 1) ? "" : "s",
	    pkgcnt, (pkgcnt == 1) ? "" : "s");
}

static int
checkpattern_fn(const char *pkg, void *vp)
{
	int     rc;

	rc = chdir(pkg);
	if (rc == -1)
		err(EXIT_FAILURE, "Cannot chdir to %s/%s", _pkgdb_getPKGDB_DIR(), pkg);

	check1pkg(pkg);
	printf(".");

	chdir("..");

	return 0;
}

static int
lspattern_fn(const char *pkg, void *vp)
{
	char *data = vp;
	printf("%s/%s\n", data, pkg);
	return 0;
}

static int
lsbasepattern_fn(const char *pkg, void *vp)
{
	printf("%s\n", pkg);
	return 0;
}

int 
main(int argc, char *argv[])
{
	const char	*prog;
	Boolean		 use_default_sfx = TRUE;
	Boolean 	 show_basename_only = FALSE;
	char		 lsdir[FILENAME_MAX];
	char		 sfx[FILENAME_MAX];
	char		*lsdirp = NULL;
	int		 ch;

	setprogname(prog = argv[0]);

	if (argc < 2)
		usage(prog);

	while ((ch = getopt(argc, argv, Options)) != -1)
		switch (ch) {
		case 'K':
			_pkgdb_setPKGDB_DIR(optarg);
			break;

		case 'S':
			sfx[0] = 0x0;
			use_default_sfx = FALSE;
			break;

		case 'V':
			show_version();
			/* NOTREACHED */

		case 'b':
			show_basename_only = TRUE;
			break;

		case 'd':
			(void) strlcpy(lsdir, optarg, sizeof(lsdir));
			lsdirp = lsdir;
			break;

		case 's':
			(void) strlcpy(sfx, optarg, sizeof(sfx));
			use_default_sfx = FALSE;
			break;

		default:
			usage(prog);
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	if (argc <= 0) {
		usage(prog);
	}

	if (use_default_sfx)
		(void) snprintf(sfx, sizeof(sfx), "%s", DEFAULT_SFX);

	if (strcasecmp(argv[0], "pmatch") == 0) {

		char *pattern, *pkg;
		
		argv++;		/* "pmatch" */

		pattern = argv[0];
		pkg = argv[1];

		if (pattern == NULL || pkg == NULL) {
			usage(prog);
		}

		if (pmatch(pattern, pkg)){
			return 0;
		} else {
			return 1;
		}
	  
	} else if (strcasecmp(argv[0], "rebuild") == 0) {

		rebuild();
		printf("Done.\n");

	} else if (strcasecmp(argv[0], "check") == 0) {

		argv++;		/* "check" */

		if (*argv != NULL) {
			/* args specified */
			int     rc;

			filecnt = 0;

			setbuf(stdout, NULL);

			rc = chdir(_pkgdb_getPKGDB_DIR());
			if (rc == -1)
				err(EXIT_FAILURE, "Cannot chdir to %s", _pkgdb_getPKGDB_DIR());

			while (*argv != NULL) {
				if (ispkgpattern(*argv)) {
					if (findmatchingname(_pkgdb_getPKGDB_DIR(), *argv, checkpattern_fn, NULL) <= 0)
						errx(EXIT_FAILURE, "No matching pkg for %s.", *argv);
				} else {
					rc = chdir(*argv);
					if (rc == -1) {
						/* found nothing - try 'pkg-[0-9]*' */
						char try[FILENAME_MAX];
					
						snprintf(try, sizeof(try), "%s-[0-9]*", *argv);
						if (findmatchingname(_pkgdb_getPKGDB_DIR(), try,
								     checkpattern_fn, NULL) <= 0) {

							errx(EXIT_FAILURE, "cannot find package %s", *argv);
						} else {
							/* nothing to do - all the work is/was
							 * done in checkpattern_fn() */
						}
					} else {
						check1pkg(*argv);
						printf(".");

						chdir("..");
					}
				}

				argv++;
			}

			printf("\n");
			printf("Checked %d file%s from %d package%s.\n",
			    filecnt, (filecnt == 1) ? "" : "s",
			    pkgcnt, (pkgcnt == 1) ? "" : "s");
		} else {
			checkall();
		}
		printf("Done.\n");

	} else if (strcasecmp(argv[0], "lsall") == 0) {
		int saved_wd;

		argv++;		/* "lsall" */

		/* preserve cwd */
		saved_wd=open(".", O_RDONLY);
		if (saved_wd == -1)
			err(EXIT_FAILURE, "Cannot save working dir");

		while (*argv != NULL) {
			/* args specified */
			int     rc;
			const char *basep, *dir;
			char cwd[MAXPATHLEN];
			char base[FILENAME_MAX];

			dir = lsdirp ? lsdirp : dirname_of(*argv);
			basep = basename_of(*argv);
			snprintf(base, sizeof(base), "%s%s", basep, sfx);

			fchdir(saved_wd);
			rc = chdir(dir);
			if (rc == -1)
				err(EXIT_FAILURE, "Cannot chdir to %s", _pkgdb_getPKGDB_DIR());

			if (getcwd(cwd, sizeof(cwd)) == NULL)
				err(EXIT_FAILURE, "getcwd");

			if (show_basename_only)
				rc = findmatchingname(cwd, base, lsbasepattern_fn, cwd);
			else
				rc = findmatchingname(cwd, base, lspattern_fn, cwd);
			if (rc == -1)
				errx(EXIT_FAILURE, "Error in findmatchingname(\"%s\", \"%s\", ...)",
				     cwd, base);

			argv++;
		}

		close(saved_wd);

	} else if (strcasecmp(argv[0], "lsbest") == 0) {
		int saved_wd;

		argv++;		/* "lsbest" */

		/* preserve cwd */
		saved_wd=open(".", O_RDONLY);
		if (saved_wd == -1)
			err(EXIT_FAILURE, "Cannot save working dir");

		while (*argv != NULL) {
			/* args specified */
			int     rc;
			const char *basep, *dir;
			char cwd[MAXPATHLEN];
			char base[FILENAME_MAX];
			char *p;

			dir = lsdirp ? lsdirp : dirname_of(*argv);
			basep = basename_of(*argv);
			snprintf(base, sizeof(base), "%s%s", basep, sfx);

			fchdir(saved_wd);
			rc = chdir(dir);
			if (rc == -1)
				err(EXIT_FAILURE, "Cannot chdir to %s", _pkgdb_getPKGDB_DIR());

			if (getcwd(cwd, sizeof(cwd)) == NULL)
				err(EXIT_FAILURE, "getcwd");
			p = findbestmatchingname(cwd, base);
			if (p) {
				if (show_basename_only)
					printf("%s\n", p);
				else
					printf("%s/%s\n", cwd, p);
				free(p);
			}
			
			argv++;
		}

		close(saved_wd);

	} else if (strcasecmp(argv[0], "list") == 0 ||
	    strcasecmp(argv[0], "dump") == 0) {

		pkgdb_dump();

	} else if (strcasecmp(argv[0], "add") == 0) {
		argv++;		/* "add" */
		while (*argv != NULL) {
			add1pkg(*argv);
			argv++;
		}
	} else if (strcasecmp(argv[0], "delete") == 0) {
		argv++;		/* "delete" */
		while (*argv != NULL) {
			delete1pkg(*argv);
			argv++;
		}
	}
#ifdef PKGDB_DEBUG
	else if (strcasecmp(argv[0], "delkey") == 0) {
		int     rc;

		if (!pkgdb_open(ReadWrite))
			err(EXIT_FAILURE, "cannot open pkgdb");

		rc = pkgdb_remove(argv[2]);
		if (rc) {
			if (errno)
				perror("pkgdb_remove");
			else
				printf("Key not present in pkgdb.\n");
		}
		
		pkgdb_close();

	} else if (strcasecmp(argv[0], "addkey") == 0) {

		int     rc;

		if (!pkgdb_open(ReadWrite)) {
			err(EXIT_FAILURE, "cannot open pkgdb");
		}

		rc = pkgdb_store(argv[2], argv[3]);
		switch (rc) {
		case -1:
			perror("pkgdb_store");
			break;
		case 1:
			printf("Key already present.\n");
			break;
		default:
			/* 0: everything ok */
			break;
		}

		pkgdb_close();

	}
#endif
	else {
		usage(prog);
	}

	return 0;
}

void
cleanup(int signo)
{
}
