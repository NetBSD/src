/*	$NetBSD: perform.c,v 1.44 2003/03/15 20:49:26 agc Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.23 1997/10/13 15:03:53 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.44 2003/03/15 20:49:26 agc Exp $");
#endif
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
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
 * Jordan K. Hubbard
 * 23 Aug 1993
 *
 * This is the main body of the info module.
 *
 */

#include "lib.h"
#include "info.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>

static char *Home;

static int
pkg_do(char *pkg)
{
	Boolean installed = FALSE, isTMP = FALSE;
	char    log_dir[FILENAME_MAX];
	char    fname[FILENAME_MAX];
	struct stat sb;
	char   *cp = NULL;
	int     code = 0;

	if (IS_URL(pkg)) {
		if ((cp = fileGetURL(pkg)) != NULL) {
			strcpy(fname, cp);
			isTMP = TRUE;
		}
	} else if (fexists(pkg) && isfile(pkg)) {
		int     len;

		if (*pkg != '/') {
			if (!getcwd(fname, FILENAME_MAX)) {
				cleanup(0);
				err(EXIT_FAILURE, "fatal error during execution: getcwd");
			}
			len = strlen(fname);
			(void) snprintf(&fname[len], sizeof(fname) - len, "/%s", pkg);
		} else {
			strcpy(fname, pkg);
		}
		cp = fname;
	} else {
		if ((cp = fileFindByPath(pkg)) != NULL) {
			strncpy(fname, cp, FILENAME_MAX);
		}
	}

	if (cp) {
		if (IS_URL(pkg)) {
			/* file is already unpacked by fileGetURL() */
			strcpy(PlayPen, cp);
		} else {
			if (IS_URL(cp)) {
				/* only a package name was given, and it was expanded to a
				 * full URL by fileFindByPath. Now extract...
				 */
				char *cp2;

				if ((cp2 = fileGetURL(cp)) != NULL) {
					strcpy(fname, cp2);
					isTMP = TRUE;
				}
				strcpy(PlayPen, cp2);
			} else {
				/*
				 * Apply a crude heuristic to see how much space the package will
				 * take up once it's unpacked.  I've noticed that most packages
				 * compress an average of 75%, but we're only unpacking the + files so
				 * be very optimistic.
				 */
				if (stat(fname, &sb) == FAIL) {
					warnx("can't stat package file '%s'", fname);
					code = 1;
					goto bail;
				}
				Home = make_playpen(PlayPen, PlayPenSize, sb.st_size / 2);
				if (unpack(fname, "+*")) {
					warnx("error during unpacking, no info for '%s' available", pkg);
					code = 1;
					goto bail;
				}
			}
		}
	} else {
		/*
	         * It's not an uninstalled package, try and find it among the
	         * installed
	         */
		char   *tmp;

		(void) snprintf(log_dir, sizeof(log_dir), "%s/%s",
		    (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR,
		    pkg);
		if (!fexists(log_dir) || !isdir(log_dir)) {
			{
				/* Check if the given package name matches
				 * something with 'pkg-[0-9]*' */
				char    try[FILENAME_MAX];
				snprintf(try, FILENAME_MAX, "%s-[0-9]*", pkg);
				if (findmatchingname(_pkgdb_getPKGDB_DIR(), try,
					add_to_list_fn, &pkgs) != 0) {
					return 0;	/* we've just appended some names to the pkgs list,
							 * they will be processed after this package. */
				}
			}

			/* No match */
			warnx("can't find package `%s' installed or in a file!", pkg);
			return 1;
		}
		if (chdir(log_dir) == FAIL) {
			warnx("can't change directory to '%s'!", log_dir);
			return 1;
		}
		installed = TRUE;
	}

	/*
         * Index is special info type that has to override all others to make
         * any sense.
         */
	if (Flags & SHOW_INDEX) {
		char    tmp[FILENAME_MAX];

		(void) snprintf(tmp, sizeof(tmp), "%-19s ", pkg);
		show_index(tmp, COMMENT_FNAME);
	} else {
		FILE   *fp;
		package_t plist;
		
		/* Read the contents list */
		plist.head = plist.tail = NULL;
		fp = fopen(CONTENTS_FNAME, "r");
		if (!fp) {
			warnx("unable to open %s file", CONTENTS_FNAME);
			code = 1;
			goto bail;
		}
		/* If we have a prefix, add it now */
		read_plist(&plist, fp);
		fclose(fp);

		/* Start showing the package contents */
		if (!Quiet) {
			printf("%sInformation for %s:\n\n", InfoPrefix, pkg);
			if (fexists(PRESERVE_FNAME)) {
				printf("*** PACKAGE MAY NOT BE DELETED ***\n");
			}
		}
		if (Flags & SHOW_COMMENT) {
			show_file("Comment:\n", COMMENT_FNAME);
		}
		if (Flags & SHOW_DEPENDS) {
			show_depends("Requires:\n", &plist);
		}
		if ((Flags & SHOW_REQBY) && !isemptyfile(REQUIRED_BY_FNAME)) {
			show_file("Required by:\n", REQUIRED_BY_FNAME);
		}
		if (Flags & SHOW_DESC) {
			show_file("Description:\n", DESC_FNAME);
		}
		if ((Flags & SHOW_DISPLAY) && fexists(DISPLAY_FNAME)) {
			show_file("Install notice:\n", DISPLAY_FNAME);
		}
		if (Flags & SHOW_PLIST) {
			show_plist("Packing list:\n", &plist, PLIST_SHOW_ALL);
		}
		if ((Flags & SHOW_INSTALL) && fexists(INSTALL_FNAME)) {
			show_file("Install script:\n", INSTALL_FNAME);
		}
		if ((Flags & SHOW_DEINSTALL) && fexists(DEINSTALL_FNAME)) {
			show_file("De-Install script:\n", DEINSTALL_FNAME);
		}
		if ((Flags & SHOW_REQUIRE) && fexists(REQUIRE_FNAME)) {
			show_file("Require script:\n", REQUIRE_FNAME);
		}
		if ((Flags & SHOW_MTREE) && fexists(MTREE_FNAME)) {
			show_file("mtree file:\n", MTREE_FNAME);
		}
		if (Flags & SHOW_PREFIX) {
			show_plist("Prefix(s):\n", &plist, PLIST_CWD);
		}
		if (Flags & SHOW_FILES) {
			show_files("Files:\n", &plist);
		}
		if ((Flags & SHOW_BUILD_VERSION) && fexists(BUILD_VERSION_FNAME)) {
			show_file("Build version:\n", BUILD_VERSION_FNAME);
		}
		if ((Flags & SHOW_BUILD_INFO) && fexists(BUILD_INFO_FNAME)) {
			show_file("Build information:\n", BUILD_INFO_FNAME);
		}
		if ((Flags & SHOW_PKG_SIZE) && fexists(SIZE_PKG_FNAME)) {
			show_file("Size of this package in bytes: ", SIZE_PKG_FNAME);
		}
		if ((Flags & SHOW_ALL_SIZE) && fexists(SIZE_ALL_FNAME)) {
			show_file("Size in bytes including required pkgs: ", SIZE_ALL_FNAME);
		}
		if (!Quiet) {
			if (fexists(PRESERVE_FNAME)) {
				printf("*** PACKAGE MAY NOT BE DELETED ***\n\n");
			}
			puts(InfoPrefix);
		}
		free_plist(&plist);
	}
bail:
	leave_playpen(Home);
	if (isTMP)
		unlink(fname);
	return code;
}

/*
 * Function to be called for pkgs found
 */
static int
foundpkg(const char *found, void *vp)
{
	char *data = vp;
	char buf[FILENAME_MAX+1];

	/* we only want to display this if it really is a directory */
	snprintf(buf, sizeof(buf), "%s/%s", data, found);
	if (!isdir(buf)) {
		/* return value seems to be ignored for now */
		return -1;
	}

	if (!Quiet) {
		printf("%s\n", found);
	}

	return 0;
}

/*
 * Check if a package "pkgspec" (which can be a pattern) is installed.
 * dbdir contains the return value of _pkgdb_getPKGDB_DIR(), for reading only.
 * Return 0 if found, 1 otherwise (indicating an error).
 */
static int
CheckForPkg(char *pkgspec, char *dbdir)
{
	char    buf[FILENAME_MAX];
	int     error;

	if (strpbrk(pkgspec, "<>[]?*{")) {
		/* expensive (pattern) match */
		return !findmatchingname(dbdir, pkgspec, foundpkg, dbdir);
	}
	/* simple match */
	(void) snprintf(buf, sizeof(buf), "%s/%s", dbdir, pkgspec);
	error = !isdir(buf);
	if (!error && !Quiet) {
		printf("%s\n", pkgspec);
	}
	if (error) {
		/* found nothing - try 'pkg-[0-9]*' */
		
		char    try[FILENAME_MAX];
		snprintf(try, FILENAME_MAX, "%s-[0-9]*", pkgspec);
		if (findmatchingname(dbdir, try, foundpkg, dbdir) != 0) {
			error = 0;
		}
	}
	return error;
}

void
cleanup(int sig)
{
	leave_playpen(Home);
	exit(1);
}

int
pkg_perform(lpkg_head_t *pkghead)
{
	struct dirent *dp;
	char   *tmp;
	DIR    *dirp;
	int     err_cnt = 0;

	signal(SIGINT, cleanup);

	tmp = _pkgdb_getPKGDB_DIR();

	/* Overriding action? */
	if (CheckPkg) {
		err_cnt += CheckForPkg(CheckPkg, tmp);
	} else if (AllInstalled) {
		if (!(isdir(tmp) || islinktodir(tmp)))
			return 1;

		if (File2Pkg) {

			/* Show all files with the package they belong to */
			pkgdb_dump();

		} else {
			/* Show all packges with description */
			if ((dirp = opendir(tmp)) != (DIR *) NULL) {
				while ((dp = readdir(dirp)) != (struct dirent *) NULL) {
					char    tmp2[FILENAME_MAX];

					if (strcmp(dp->d_name, ".") == 0 ||
					    strcmp(dp->d_name, "..") == 0)
						continue;

					(void) snprintf(tmp2, sizeof(tmp2), "%s/%s",
					    tmp, dp->d_name);
					if (isfile(tmp2))
						continue;

					err_cnt += pkg_do(dp->d_name);
				}
				(void) closedir(dirp);
			}
		}
	} else {
		/* Show info on individual pkg(s) */
		lpkg_t *lpp;

		while ((lpp = TAILQ_FIRST(pkghead))) {
			TAILQ_REMOVE(pkghead, lpp, lp_link);
			err_cnt += pkg_do(lpp->lp_name);
			free_lpkg(lpp);
		}
	}
	ftp_stop();
	return err_cnt;
}
