/*	$NetBSD: perform.c,v 1.1.1.3 2007/08/14 22:59:51 joerg Exp $	*/

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
__RCSID("$NetBSD: perform.c,v 1.1.1.3 2007/08/14 22:59:51 joerg Exp $");
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
pkg_do(const char *pkg)
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
			switch (add_installed_pkgs_by_basename(pkg, &pkgs)) {
			case 1:
				return 0;
			case 0:
				/* No match */
				warnx("can't find package `%s'", pkg);
				return 1;
			case -1:
				errx(EXIT_FAILURE, "Error during search in pkgdb for %s", pkg);
			}
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
			show_var(pkg, INSTALLED_INFO_FNAME, BuildInfoVariable);
		else
			show_var(pkg, BUILD_INFO_FNAME, BuildInfoVariable);
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

struct print_matching_arg {
	const char *pattern;
	int got_match;
};

static int
print_matching_pkg(const char *pkgname, void *cookie)
{
	struct print_matching_arg *arg= cookie;

	if (pkg_match(arg->pattern, pkgname)) {
		if (!Quiet)
			puts(pkgname);
		arg->got_match = 1;
	}

	return 0;
}

/*
 * Returns 0 if at least one package matching pkgname.
 * Returns 1 otherwise.
 *
 * If -q was not specified, print all matching packages to stdout.
 */
int
CheckForPkg(const char *pkgname)
{
	struct print_matching_arg arg;

	arg.pattern = pkgname;
	arg.got_match = 0;

	if (iterate_pkg_db(print_matching_pkg, &arg) == -1) {
		warnx("cannot iterate pkgdb");
		return 1;
	}

	if (arg.got_match == 0 && !ispkgpattern(pkgname)) {
		char *pattern;

		if (asprintf(&pattern, "%s-[0-9]*", pkgname) == -1)
			errx(EXIT_FAILURE, "asprintf failed");

		arg.pattern = pattern;
		arg.got_match = 0;

		if (iterate_pkg_db(print_matching_pkg, &arg) == -1) {
			free(pattern);
			warnx("cannot iterate pkgdb");
			return 1;
		}
		free(pattern);
	}

	if (arg.got_match)
		return 0;
	else
		return 1;
}

/*
 * Returns 0 if at least one package matching pkgname.
 * Returns 1 otherwise.
 *
 * If -q was not specified, print best match to stdout.
 */
int
CheckForBestPkg(const char *pkgname)
{
	char *pattern, *best_match;

	best_match = find_best_matching_installed_pkg(pkgname);
	if (best_match == NULL) {
		if (ispkgpattern(pkgname))
			return 1;

		if (asprintf(&pattern, "%s-[0-9]*", pkgname) == -1)
			errx(EXIT_FAILURE, "asprintf failed");

		best_match = find_best_matching_installed_pkg(pattern);
		free(pattern);
	}

	if (best_match == NULL)
		return 1;
	if (!Quiet)
		puts(best_match);
	free(best_match);
	return 0;
}

void
cleanup(int sig)
{
	leave_playpen(Home);
	exit(1);
}

static int
perform_single_pkg(const char *pkg, void *cookie)
{
	int *err_cnt = cookie;

	if (Which == WHICH_ALL || !is_automatic_installed(pkg))
		*err_cnt += pkg_do(pkg);

	return 0;
}

int
pkg_perform(lpkg_head_t *pkghead)
{
	int     err_cnt = 0;

	signal(SIGINT, cleanup);

	TAILQ_INIT(&files);

	if (Which != WHICH_LIST) {
		if (File2Pkg) {
			/* Show all files with the package they belong to */
			if (pkgdb_dump() == -1)
				err_cnt = 1;
		} else {
			if (iterate_pkg_db(perform_single_pkg, &err_cnt) == -1)
				err_cnt = 1;
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
