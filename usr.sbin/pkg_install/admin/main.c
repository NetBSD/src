/*	$NetBSD: main.c,v 1.28.2.1 2003/07/13 09:45:22 jlam Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.28.2.1 2003/07/13 09:45:22 jlam Exp $");
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

void    usage(void);

int     filecnt;
int     pkgcnt;

static int checkpattern_fn(const char *, void *);

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

static void 
rebuild(void)
{
	DIR    *dp;
	struct dirent *de;
	FILE   *f;
	plist_t *p;
	char   *PkgName, dir[FILENAME_MAX], *dirp = NULL;
	char   *PkgDBDir = NULL, file[FILENAME_MAX];
	char	cachename[FILENAME_MAX];

	pkgcnt = 0;
	filecnt = 0;

	if (unlink(_pkgdb_getPKGDB_FILE(cachename, sizeof(cachename))) != 0 && errno != ENOENT)
		err(EXIT_FAILURE, "unlink %s", cachename);

	if (!pkgdb_open(ReadWrite))
		err(EXIT_FAILURE, "cannot open pkgdb");

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
		package_t Plist;

		if (!isdir(de->d_name))
			continue;

		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;

#ifdef PKGDB_DEBUG
		printf("%s\n", de->d_name);
#else
		printf(".");
#endif

		chdir(de->d_name);

		f = fopen(CONTENTS_FNAME, "r");
		if (f == NULL)
			err(EXIT_FAILURE, "can't open %s/%s", de->d_name, CONTENTS_FNAME);

		Plist.head = Plist.tail = NULL;
		read_plist(&Plist, f);
		p = find_plist(&Plist, PLIST_NAME);
		if (p == NULL)
			errx(EXIT_FAILURE, "Package %s has no @name, aborting.",
			    de->d_name);
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
					warnx("%s: File %s is in %s but not on filesystem!",
					    PkgName, file, CONTENTS_FNAME);
				else {
					pkgdb_store(file, PkgName);
					filecnt++;
				}
				break;
			case PLIST_CWD:
				if (strcmp(p->name, ".") != 0)
					dirp = p->name;
				else {
					(void) snprintf(dir, sizeof(dir), "%s/%s", PkgDBDir, de->d_name);
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

		chdir("..");
	}
	closedir(dp);
	pkgdb_close();

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
		if (!isdir(de->d_name))
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

int 
main(int argc, char *argv[])
{
	setprogname(argv[0]);

	if (argc < 2)
		usage();

	if (strcmp(argv[1], "-V") == 0) {

		show_version();
		/* NOTREACHED */

	} else if (strcasecmp(argv[1], "pmatch") == 0) {

		char *pattern, *pkg;
		
		argv++;		/* argv[0] */
		argv++;		/* "pmatch" */

		pattern = argv[0];
		pkg = argv[1];

		if (pattern == NULL || pkg == NULL) {
			usage();
		}

		if (pmatch(pattern, pkg)){
			return 0;
		} else {
			return 1;
		}
	  
	} else if (strcasecmp(argv[1], "rebuild") == 0) {

		rebuild();
		printf("Done.\n");

	} else if (strcasecmp(argv[1], "check") == 0) {

		argv++;		/* argv[0] */
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

	} else if (strcasecmp(argv[1], "lsall") == 0) {
		int saved_wd;

		argv++;		/* argv[0] */
		argv++;		/* "check" */

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

			dir = dirname_of(*argv);
			basep = basename_of(*argv);
			snprintf(base, sizeof(base), "%s.t[bg]z", basep);

			fchdir(saved_wd);
			rc = chdir(dir);
			if (rc == -1)
				err(EXIT_FAILURE, "Cannot chdir to %s", _pkgdb_getPKGDB_DIR());

			if (getcwd(cwd, sizeof(cwd)) == NULL)
				err(EXIT_FAILURE, "getcwd");
			if (findmatchingname(cwd, base, lspattern_fn, cwd) == -1)
				errx(EXIT_FAILURE, "Error in findmatchingname(\"%s\", \"%s\", ...)",
				     cwd, base);

			argv++;
		}

		close(saved_wd);

	} else if (strcasecmp(argv[1], "lsbest") == 0) {
		int saved_wd;

		argv++;		/* argv[0] */
		argv++;		/* "check" */

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

			dir = dirname_of(*argv);
			basep = basename_of(*argv);
			snprintf(base, sizeof(base), "%s.t[bg]z", basep);

			fchdir(saved_wd);
			rc = chdir(dir);
			if (rc == -1)
				err(EXIT_FAILURE, "Cannot chdir to %s", _pkgdb_getPKGDB_DIR());

			if (getcwd(cwd, sizeof(cwd)) == NULL)
				err(EXIT_FAILURE, "getcwd");
			p = findbestmatchingname(cwd, base);
			if (p) {
				printf("%s/%s\n", cwd, p);
				free(p);
			}
			
			argv++;
		}

		close(saved_wd);

	} else if (strcasecmp(argv[1], "list") == 0 ||
	    strcasecmp(argv[1], "dump") == 0) {

		pkgdb_dump();

	}
#ifdef PKGDB_DEBUG
	else if (strcasecmp(argv[1], "del") == 0 ||
	    strcasecmp(argv[1], "delete") == 0) {

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

	} else if (strcasecmp(argv[1], "add") == 0) {

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
		usage();
	}

	return 0;
}

void 
usage(void)
{
	printf("usage: pkg_admin [-V] command args ...\n"
	    "Where 'commands' and 'args' are:\n"
	    " rebuild                     - rebuild pkgdb from +CONTENTS files\n"
	    " check [pkg ...]             - check md5 checksum of installed files\n"
#ifdef PKGDB_DEBUG
	    " add key value               - add key and value\n"
	    " delete key                  - delete reference to key\n"
#endif
	    " lsall /path/to/pkgpattern   - list all pkgs matching the pattern\n"
	    " lsbest /path/to/pkgpattern  - list pkgs matching the pattern best\n"
	    " dump                        - dump database\n"
	    " pmatch pattern pkg          - returns true if pkg matches pattern, otherwise false\n");
	exit(EXIT_FAILURE);
}

void
cleanup(int signo)
{
	;
}
