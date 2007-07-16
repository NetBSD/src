/*	$NetBSD: perform.c,v 1.1.1.1 2007/07/16 13:01:47 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.23 1997/10/13 15:03:53 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.1.1.1 2007/07/16 13:01:47 joerg Exp $");
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

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif

static char *Home;

static lfile_head_t files;

static int
pkg_do(char *pkg)
{
	Boolean installed = FALSE, isTMP = FALSE;
	char    log_dir[MaxPathSize];
	char    fname[MaxPathSize];
	struct stat sb;
	char   *cp = NULL;
	int     code = 0;
	lfile_t	*lfp;
	int	result;
	char   *binpkgfile = NULL;

	if (IS_URL(pkg)) {
		if ((cp = fileGetURL(pkg)) != NULL) {
			strlcpy(fname, cp, sizeof(fname));
			isTMP = TRUE;
		}
	} else if (fexists(pkg) && isfile(pkg)) {
		int     len;

		if (*pkg != '/') {
			if (!getcwd(fname, MaxPathSize)) {
				cleanup(0);
				err(EXIT_FAILURE, "fatal error during execution: getcwd");
			}
			len = strlen(fname);
			(void) snprintf(&fname[len], sizeof(fname) - len, "/%s", pkg);
		} else {
			strlcpy(fname, pkg, sizeof(fname));
		}
		cp = fname;
		binpkgfile = fname;
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

				binpkgfile = NULL;

				if ((cp2 = fileGetURL(cp)) != NULL) {
					strlcpy(fname, cp2, sizeof(fname));
					isTMP = TRUE;
					
					strcpy(PlayPen, cp2);
				}
			} else {
				/*
				 * Apply a crude heuristic to see how much space the package will
				 * take up once it's unpacked.  I've noticed that most packages
				 * compress an average of 75%, but we're only unpacking the + files
				 * needed so be very optimistic.
				 */

				/* Determine which +-files to unpack - not all may be present! */
				LFILE_ADD(&files, lfp, CONTENTS_FNAME);
				LFILE_ADD(&files, lfp, COMMENT_FNAME);
				LFILE_ADD(&files, lfp, DESC_FNAME);
				if (Flags & SHOW_MTREE)
					LFILE_ADD(&files, lfp, MTREE_FNAME);
				if (Flags & SHOW_BUILD_VERSION)
					LFILE_ADD(&files, lfp, BUILD_VERSION_FNAME);
				if (Flags & (SHOW_BUILD_INFO|SHOW_SUMMARY|SHOW_BI_VAR))
					LFILE_ADD(&files, lfp, BUILD_INFO_FNAME);
				if (Flags & (SHOW_PKG_SIZE|SHOW_SUMMARY))
					LFILE_ADD(&files, lfp, SIZE_PKG_FNAME);
				if (Flags & SHOW_ALL_SIZE)
					LFILE_ADD(&files, lfp, SIZE_ALL_FNAME);
#if 0
				if (Flags & SHOW_REQBY)
					LFILE_ADD(&files, lfp, REQUIRED_BY_FNAME);
				if (Flags & SHOW_DISPLAY)
					LFILE_ADD(&files, lfp, DISPLAY_FNAME);
				if (Flags & SHOW_INSTALL)
					LFILE_ADD(&files, lfp, INSTALL_FNAME);
				if (Flags & SHOW_DEINSTALL)
					LFILE_ADD(&files, lfp, DEINSTALL_FNAME);
				if (Flags & SHOW_REQUIRE)
					LFILE_ADD(&files, lfp, REQUIRE_FNAME);
				/* PRESERVE_FNAME? */
#endif				

				if (stat(fname, &sb) == FAIL) {
					warnx("can't stat package file '%s'", fname);
					code = 1;
					goto bail;
				}
				Home = make_playpen(PlayPen, PlayPenSize, sb.st_size / 2);
				result = unpack(fname, &files);
				while ((lfp = TAILQ_FIRST(&files)) != NULL) {
					TAILQ_REMOVE(&files, lfp, lf_link);
					free(lfp);
				}
				if (result) {
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
		(void) snprintf(log_dir, sizeof(log_dir), "%s/%s",
		    _pkgdb_getPKGDB_DIR(), pkg);
		if (!fexists(log_dir) || !(isdir(log_dir) || islinktodir(log_dir))) {
			{
				/* Check if the given package name matches
				 * something with 'pkg-[0-9]*' */
				char    try[MaxPathSize];
				snprintf(try, MaxPathSize, "%s-[0-9]*", pkg);
				if (findmatchingname(_pkgdb_getPKGDB_DIR(), try,
					add_to_list_fn, &pkgs) > 0) {
					return 0;	/* we've just appended some names to the pkgs list,
							 * they will be processed after this package. */
				}
			}

			/* No match */
			warnx("can't find package `%s'", pkg);
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
		char    tmp[MaxPathSize];

		(void) snprintf(tmp, sizeof(tmp), "%-19s ", pkg);
		show_index(pkg, tmp, COMMENT_FNAME);
	} else if (Flags & SHOW_BI_VAR) {
		if (strcspn(BuildInfoVariable, "ABCDEFGHIJKLMNOPQRSTUVWXYZ")
		    == strlen(BuildInfoVariable))
			show_var(INSTALLED_INFO_FNAME, BuildInfoVariable);
		else
			show_var(BUILD_INFO_FNAME, BuildInfoVariable);
	} else {
		FILE   *fp;
		package_t plist;
		
		/* Read the contents list */
		plist.head = plist.tail = NULL;
		fp = fopen(CONTENTS_FNAME, "r");
		if (!fp) {
			warn("unable to open %s file", CONTENTS_FNAME);
			code = 1;
			goto bail;
		}
		/* If we have a prefix, add it now */
		read_plist(&plist, fp);
		fclose(fp);

		/* Start showing the package contents */
		if (!Quiet && !(Flags & SHOW_SUMMARY)) {
			printf("%sInformation for %s:\n\n", InfoPrefix, pkg);
			if (fexists(PRESERVE_FNAME)) {
				printf("*** PACKAGE MAY NOT BE DELETED ***\n");
			}
		}
		if (Flags & SHOW_SUMMARY) {
			show_summary(&plist, binpkgfile);
		}
		if (Flags & SHOW_COMMENT) {
			show_file(pkg, "Comment:\n", COMMENT_FNAME, TRUE);
		}
		if (Flags & SHOW_DEPENDS) {
			show_depends("Requires:\n", &plist);
		}
		if (Flags & SHOW_BLD_DEPENDS) {
			show_bld_depends("Built using:\n", &plist);
		}
		if ((Flags & SHOW_REQBY) && !isemptyfile(REQUIRED_BY_FNAME)) {
			show_file(pkg, "Required by:\n",
				  REQUIRED_BY_FNAME, TRUE);
		}
		if (Flags & SHOW_DESC) {
			show_file(pkg, "Description:\n", DESC_FNAME, TRUE);
		}
		if ((Flags & SHOW_DISPLAY) && fexists(DISPLAY_FNAME)) {
			show_file(pkg, "Install notice:\n",
				  DISPLAY_FNAME, TRUE);
		}
		if (Flags & SHOW_PLIST) {
			show_plist("Packing list:\n", &plist, PLIST_SHOW_ALL);
		}
		if ((Flags & SHOW_INSTALL) && fexists(INSTALL_FNAME)) {
			show_file(pkg, "Install script:\n",
				  INSTALL_FNAME, TRUE);
		}
		if ((Flags & SHOW_DEINSTALL) && fexists(DEINSTALL_FNAME)) {
			show_file(pkg, "De-Install script:\n",
				  DEINSTALL_FNAME, TRUE);
		}
		if (fexists(REQUIRE_FNAME)) {
			warnx("package %s uses obsoleted require scripts", pkg);
			if (Flags & SHOW_REQUIRE) {
				show_file(pkg, "Require script:\n",
					  REQUIRE_FNAME, TRUE);
			}
		}
		if ((Flags & SHOW_MTREE) && fexists(MTREE_FNAME)) {
			show_file(pkg, "mtree file:\n", MTREE_FNAME, TRUE);
		}
		if (Flags & SHOW_PREFIX) {
			show_plist("Prefix(s):\n", &plist, PLIST_CWD);
		}
		if (Flags & SHOW_FILES) {
			show_files("Files:\n", &plist);
		}
		if ((Flags & SHOW_BUILD_VERSION) && fexists(BUILD_VERSION_FNAME)) {
			show_file(pkg, "Build version:\n",
				  BUILD_VERSION_FNAME, TRUE);
		}
		if (Flags & SHOW_BUILD_INFO) {
			if (fexists(BUILD_INFO_FNAME)) {
				show_file(pkg, "Build information:\n",
					  BUILD_INFO_FNAME,
					  !fexists(INSTALLED_INFO_FNAME));
			}
			if (fexists(INSTALLED_INFO_FNAME)) {
				show_file(pkg, "Installed information:\n",
					  INSTALLED_INFO_FNAME, TRUE);
			}
		}
		if ((Flags & SHOW_PKG_SIZE) && fexists(SIZE_PKG_FNAME)) {
			show_file(pkg, "Size of this package in bytes: ",
				  SIZE_PKG_FNAME, TRUE);
		}
		if ((Flags & SHOW_ALL_SIZE) && fexists(SIZE_ALL_FNAME)) {
			show_file(pkg, "Size in bytes including required pkgs: ",
				  SIZE_ALL_FNAME, TRUE);
		}
		if (!Quiet && !(Flags & SHOW_SUMMARY)) {
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
	char buf[MaxPathSize+1];

	/* we only want to display this if it really is a directory */
	snprintf(buf, sizeof(buf), "%s/%s", data, found);
	if (!(isdir(buf) || islinktodir(buf))) {
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
	char    buf[MaxPathSize];
	int     error;

	if (strpbrk(pkgspec, "<>[]?*{")) {
		/* expensive (pattern) match */
		error = findmatchingname(dbdir, pkgspec, foundpkg, dbdir);
		if (error == -1)
			return 1;
		else
			return !error;
	}
	/* simple match */
	(void) snprintf(buf, sizeof(buf), "%s/%s", dbdir, pkgspec);
	error = !(isdir(buf) || islinktodir(buf));
	if (!error && !Quiet) {
		printf("%s\n", pkgspec);
	}
	if (error) {
		/* found nothing - try 'pkg-[0-9]*' */
		
		char    try[MaxPathSize];
		snprintf(try, MaxPathSize, "%s-[0-9]*", pkgspec);
		if (findmatchingname(dbdir, try, foundpkg, dbdir) > 0) {
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
	char   *dbdir;
	DIR    *dirp;
	int     err_cnt = 0;

	signal(SIGINT, cleanup);

	TAILQ_INIT(&files);

	dbdir = _pkgdb_getPKGDB_DIR();

	/* Overriding action? */
	if (CheckPkg) {
		err_cnt += CheckForPkg(CheckPkg, dbdir);
	} else if (Which != WHICH_LIST) {
		if (!(isdir(dbdir) || islinktodir(dbdir)))
			return 1;

		if (File2Pkg) {
			/* Show all files with the package they belong to */
			pkgdb_dump();
		} else {
			/* Show all packages with description */
			if ((dirp = opendir(dbdir)) != (DIR *) NULL) {
				while ((dp = readdir(dirp)) != (struct dirent *) NULL) {
					char    tmp2[MaxPathSize];

					if (strcmp(dp->d_name, ".") == 0 ||
					    strcmp(dp->d_name, "..") == 0)
						continue;

					(void) snprintf(tmp2, sizeof(tmp2), "%s/%s",
					    dbdir, dp->d_name);
					if (isfile(tmp2))
						continue;

					if (Which == WHICH_ALL
					    || !is_automatic_installed(tmp2))
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
