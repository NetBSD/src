/*	$NetBSD: main.c,v 1.19 1999/11/29 19:48:46 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char *rcsid = "from FreeBSD Id: main.c,v 1.14 1997/10/08 07:47:26 charnier Exp";
#else
__RCSID("$NetBSD: main.c,v 1.19 1999/11/29 19:48:46 hubertf Exp $");
#endif
#endif

/*
 *
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
 * 18 July 1993
 *
 * This is the add module.
 *
 */

#include <sys/ioctl.h>

#include <termios.h>
#include <err.h>

#include "lib.h"
#include "info.h"

static char Options[] = "aBbcDde:fFhIikLl:mpqRrsSv";

int     Flags = 0;
Boolean AllInstalled = FALSE;
Boolean File2Pkg = FALSE;
Boolean Quiet = FALSE;
char   *InfoPrefix = "";
char    PlayPen[FILENAME_MAX];
size_t  PlayPenSize = sizeof(PlayPen);
char   *CheckPkg = NULL;
size_t  termwidth = 0;
lpkg_head_t pkgs;

static void
usage(void)
{
	fprintf(stderr, "%s\n%s\n%s\n",
	    "usage: pkg_info [-BbcDdFfIikLmpqRrvh] [-e package] [-l prefix]",
	    "                pkg-name [pkg-name ...]",
	    "       pkg_info -a [flags]");
	exit(1);
}

int
find_fn(const char *pkg, char *data)
{
	lpkg_t *lpp;

	lpp = alloc_lpkg(pkg);
	TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);

	return 0;
}

int
main(int argc, char **argv)
{
	int     ch, rc;
	lpkg_t *lpp;

	while ((ch = getopt(argc, argv, Options)) != -1)
		switch (ch) {
		case 'a':
			AllInstalled = TRUE;
			break;

		case 'B':
			Flags |= SHOW_BUILD_INFO;
			break;

		case 'b':
			Flags |= SHOW_BUILD_VERSION;
			break;

		case 'c':
			Flags |= SHOW_COMMENT;
			break;

		case 'D':
			Flags |= SHOW_DISPLAY;
			break;

		case 'd':
			Flags |= SHOW_DESC;
			break;

		case 'e':
			CheckPkg = optarg;
			break;

		case 'f':
			Flags |= SHOW_PLIST;
			break;

		case 'F':
			File2Pkg = 1;
			break;

		case 'I':
			Flags |= SHOW_INDEX;
			break;

		case 'i':
			Flags |= SHOW_INSTALL;
			break;

		case 'k':
			Flags |= SHOW_DEINSTALL;
			break;

		case 'L':
			Flags |= SHOW_FILES;
			break;

		case 'l':
			InfoPrefix = optarg;
			break;

		case 'm':
			Flags |= SHOW_MTREE;
			break;

		case 'p':
			Flags |= SHOW_PREFIX;
			break;

		case 'q':
			Quiet = TRUE;
			break;

		case 'R':
			Flags |= SHOW_REQBY;
			break;

		case 'r':
			Flags |= SHOW_REQUIRE;
			break;

		case 's':
			Flags |= SHOW_PKG_SIZE;
			break;

		case 'S':
			Flags |= SHOW_ALL_SIZE;
			break;

		case 'v':
			Verbose = TRUE;
			/* Reasonable definition of 'everything' */
			Flags = SHOW_COMMENT | SHOW_DESC | SHOW_PLIST | SHOW_INSTALL |
			    SHOW_DEINSTALL | SHOW_REQUIRE | SHOW_DISPLAY | SHOW_MTREE |
			    SHOW_REQBY | SHOW_DEPENDS | SHOW_PKG_SIZE | SHOW_ALL_SIZE;
			break;

		case 'h':
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	if (argc == 0 && !Flags) {
		/* No argument or flags specified - assume -Ia */
		Flags = SHOW_INDEX;
		AllInstalled = TRUE;
	}

	/* Set some reasonable defaults */
	if (!Flags)
		Flags = SHOW_COMMENT | SHOW_DESC | SHOW_REQBY | SHOW_DEPENDS;

	/* -Fe /filename -> change CheckPkg to real packagename */
	if (CheckPkg && File2Pkg) {
		char   *s;

		if (pkgdb_open(1) == -1)
			err(1, "cannot open pkgdb");

		s = pkgdb_retrieve(CheckPkg);

		if (s) {
			CheckPkg = s;
		} else {
			errx(1, "No matching pkg for %s.", CheckPkg);
		}

		pkgdb_close();
	}

	TAILQ_INIT(&pkgs);

	/* Get all the remaining package names, if any */
	if (File2Pkg && !AllInstalled)
		if (pkgdb_open(1) == -1) {
			err(1, "cannot open pkgdb");
		}
	while (*argv) {
		/* pkgdb: if -F flag given, don't add pkgnames to the "pkgs"
		 * queue but rather resolve the given filenames to pkgnames
		 * using pkgdb_retrieve, then add them. */
		if (File2Pkg) {
			char   *s;

			s = pkgdb_retrieve(*argv);

			if (s) {
				lpp = alloc_lpkg(s);
				TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
			} else
				errx(1, "No matching pkg for %s.", *argv);
		} else {
			if (ispkgpattern(*argv)) {
				if (findmatchingname(_pkgdb_getPKGDB_DIR(), *argv, find_fn, NULL) == 0)
					errx(1, "No matching pkg for %s.", *argv);
			} else {
				lpp = alloc_lpkg(*argv);
				TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
			}
		}
		argv++;
	}

	if (File2Pkg)
		pkgdb_close();

	/* If no packages, yelp */
	if (TAILQ_FIRST(&pkgs) == NULL && !AllInstalled && !CheckPkg)
		warnx("missing package name(s)"), usage();

	if (isatty(STDOUT_FILENO)) {
		const char *p;
		struct winsize win;

		if ((p = getenv("COLUMNS")) != NULL)
			termwidth = atoi(p);
		else if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 &&
		    win.ws_col > 0)
			termwidth = win.ws_col;
	}

	rc = pkg_perform(&pkgs);
	exit(rc);
	/* NOTREACHED */
}
