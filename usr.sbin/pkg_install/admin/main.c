/*	$NetBSD: main.c,v 1.8.4.1 1999/12/27 18:37:59 wrstuden Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.8.4.1 1999/12/27 18:37:59 wrstuden Exp $");
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
 *      This product includes software developed by Hubert Feyrer.
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
#include <db.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <md5.h>
#include <limits.h>
#include <stdio.h>

#include "lib.h"

void    usage(void);

extern const char *__progname;	/* from crt0.o */

int     filecnt;

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
		err(1, "can't open %s/%s/%s", _pkgdb_getPKGDB_DIR(), pkgdir, CONTENTS_FNAME);

	Plist.head = Plist.tail = NULL;
	read_plist(&Plist, f);
	p = find_plist(&Plist, PLIST_NAME);
	if (p == NULL)
		errx(1, "Package %s has no @name, aborting.\n",
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
		case PLIST_SRC:
			warnx("@src is deprecated - please send-pr for %s!\n", PkgName);
			break;
		case PLIST_IGNORE:
			p = p->next;
			break;
		case PLIST_SHOW_ALL:
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
			break;
		}
	}
	free_plist(&Plist);
	fclose(f);
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
	int     pkgcnt = 0;

	filecnt = 0;

	if (unlink(_pkgdb_getPKGDB_FILE()) != 0 && errno != ENOENT)
		err(1, "unlink %s", _pkgdb_getPKGDB_FILE());

	if (pkgdb_open(0) == -1)
		err(1, "cannot open pkgdb");

	setbuf(stdout, NULL);
	PkgDBDir = _pkgdb_getPKGDB_DIR();
	chdir(PkgDBDir);
#ifdef PKGDB_DEBUG
	printf("PkgDBDir='%s'\n", PkgDBDir);
#endif
	dp = opendir(".");
	if (dp == NULL)
		err(1, "opendir failed");
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
			err(1, "can't open %s/%s", de->d_name, CONTENTS_FNAME);

		Plist.head = Plist.tail = NULL;
		read_plist(&Plist, f);
		p = find_plist(&Plist, PLIST_NAME);
		if (p == NULL)
			errx(1, "Package %s has no @name, aborting.\n",
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
			case PLIST_SRC:
				warnx("@src is deprecated - please send-pr for %s!\n", PkgName);
				break;
			case PLIST_IGNORE:
				p = p->next;
				break;
			case PLIST_SHOW_ALL:
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
				break;
			}
		}
		free_plist(&Plist);
		fclose(f);

		chdir("..");

		pkgcnt++;
	}
	closedir(dp);
	pkgdb_close();

	printf("\n");
	printf("Stored %d file%s from %d package%s in %s.\n",
	    filecnt, filecnt == 1 ? "" : "s",
	    pkgcnt, pkgcnt == 1 ? "" : "s",
	    _pkgdb_getPKGDB_FILE());
}

static void 
checkall(void)
{
	DIR    *dp;
	struct dirent *de;
	int     pkgcnt = 0;

	filecnt = 0;

	setbuf(stdout, NULL);
	chdir(_pkgdb_getPKGDB_DIR());

	dp = opendir(".");
	if (dp == NULL)
		err(1, "opendir failed");
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

		pkgcnt++;
	}
	closedir(dp);
	pkgdb_close();


	printf("\n");
	printf("Checked %d file%s from %d package%s.\n",
	    filecnt, (filecnt == 1) ? "" : "s",
	    pkgcnt, (pkgcnt == 1) ? "" : "s");
}

static int
checkpattern_fn(const char *pkg, char *data)
{
	int     rc;

	rc = chdir(pkg);
	if (rc == -1)
		err(1, "Cannot chdir to %s/%s", _pkgdb_getPKGDB_DIR(), pkg);

	check1pkg(pkg);
	printf(".");

	chdir("..");

	return 0;
}

int 
main(int argc, char *argv[])
{
	if (argc < 2)
		usage();

	if (strcasecmp(argv[1], "rebuild") == 0) {

		rebuild();

	} else if (strcasecmp(argv[1], "check") == 0) {

		argv++;		/* argv[0] */
		argv++;		/* "check" */

		if (*argv != NULL) {
			/* args specified */
			int     pkgcnt = 0;
			int     rc;

			filecnt = 0;

			setbuf(stdout, NULL);

			rc = chdir(_pkgdb_getPKGDB_DIR());
			if (rc == -1)
				err(1, "Cannot chdir to %s", _pkgdb_getPKGDB_DIR());

			while (*argv != NULL) {
				if (ispkgpattern(*argv)) {
					if (findmatchingname(_pkgdb_getPKGDB_DIR(), *argv, checkpattern_fn, NULL) == 0)
						errx(1, "No matching pkg for %s.", *argv);
				} else {
					rc = chdir(*argv);
					if (rc == -1)
						err(1, "Cannot chdir to %s/%s", _pkgdb_getPKGDB_DIR(), *argv);

					check1pkg(*argv);
					printf(".");

					chdir("..");
				}

				pkgcnt++;
				argv++;
			}

			printf("\n");
			printf("Checked %d file%s from %d package%s.\n",
			    filecnt, (filecnt == 1) ? "" : "s",
			    pkgcnt, (pkgcnt == 1) ? "" : "s");
		} else {
			checkall();
		}

	} else if (strcasecmp(argv[1], "list") == 0 ||
	    strcasecmp(argv[1], "dump") == 0) {

		char   *key, *val;

		printf("Dumping pkgdb %s:\n", _pkgdb_getPKGDB_FILE());

		if (pkgdb_open(1) == -1) {
			err(1, "cannot open %s", _pkgdb_getPKGDB_FILE());
		}
		while ((key = pkgdb_iter())) {
			val = pkgdb_retrieve(key);

			printf("file: %-50s pkg: %s\n", key, val);
		}
		pkgdb_close();

	}
#ifdef PKGDB_DEBUG
	else if (strcasecmp(argv[1], "del") == 0 ||
	    strcasecmp(argv[1], "delete") == 0) {

		int     rc;

		if (pkgdb_open(0) == -1)
			err(1, "cannot open pkgdb");

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

		if (pkgdb_open(0) == -1) {
			err(1, "cannot open pkgdb");
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

	printf("Done.\n");

	return 0;
}

void 
usage(void)
{
	printf("Usage: %s command args ...\n"
	    "Where 'commands' and 'args' are:\n"
	    " rebuild          - rebuild pkgdb from +CONTENTS files\n"
	    " check [pkg ...]  - check md5 checksum of installed files\n"
#ifdef PKGDB_DEBUG
	    " add key value    - add key & value\n"
	    " delete key       - delete reference to key\n"
#endif
	    " dump             - dump database\n"
	    ,__progname);
	exit(1);
}

void
cleanup(int signo)
{
	;
}
