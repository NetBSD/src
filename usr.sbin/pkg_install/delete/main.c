/*	$NetBSD: main.c,v 1.18 2002/03/12 22:36:26 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char *rcsid = "from FreeBSD Id: main.c,v 1.11 1997/10/08 07:46:48 charnier Exp";
#else
__RCSID("$NetBSD: main.c,v 1.18 2002/03/12 22:36:26 hubertf Exp $");
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
 * This is the delete module.
 *
 */

#include <err.h>
#include <errno.h>
#include "lib.h"
#include "delete.h"

static char Options[] = "hVvDdnfFp:OrR";

char   *Prefix = NULL;
char   *ProgramPath = NULL;
Boolean NoDeInstall = FALSE;
Boolean CleanDirs = FALSE;
Boolean File2Pkg = FALSE;
Boolean Recurse_up = FALSE;
Boolean Recurse_down = FALSE;
Boolean OnlyDeleteFromPkgDB = FALSE;
lpkg_head_t pkgs;

static void
usage(void)
{
	fprintf(stderr, "usage: pkg_delete [-vVDdnFfOrR] [-p prefix] pkg-name ...\n");
	exit(1);
}

int
find_fn(const char *pkg, char *data)
{
	lpkg_t *lpp;
	char fn[FILENAME_MAX];

	snprintf(fn, sizeof(fn), "%s/%s", _pkgdb_getPKGDB_DIR(), pkg);
	if (!isfile(fn)) {	/* might as well use sanity_check() */
		lpp = alloc_lpkg(pkg);
		TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
	}

	return 0;
}

int
main(int argc, char **argv)
{
	int     ch, error;
	lpkg_t *lpp;

	ProgramPath = argv[0];

	while ((ch = getopt(argc, argv, Options)) != -1)
		switch (ch) {
		case 'v':
			Verbose = TRUE;
			break;

		case 'f':
			Force = TRUE;
			break;

		case 'F':
			File2Pkg = TRUE;
			break;

		case 'p':
			Prefix = optarg;
			break;

		case 'D':
			NoDeInstall = TRUE;
			break;

		case 'd':
			CleanDirs = TRUE;
			break;

		case 'n':
			Fake = TRUE;
			Verbose = TRUE;
			break;

		case 'r':
			Recurse_up = TRUE;
			break;

		case 'R':
			Recurse_down = TRUE;
			break;

		case 'O':
			OnlyDeleteFromPkgDB = TRUE;
			break;

		case 'V':
			show_version();
			/* NOTREACHED */

		case 'h':
		case '?':
		default:
			usage();
			break;
		}

	argc -= optind;
	argv += optind;

	TAILQ_INIT(&pkgs);

	/* Get all the remaining package names, if any */
	if (File2Pkg)
		if (pkgdb_open(1) == -1) {
			err(1, "cannot open pkgdb");
		}
	/* Get all the remaining package names, if any */
	while (*argv) {
		/* pkgdb: if -F flag given, don't add pkgnames to pkgs but
		 * rather resolve the given filenames to pkgnames using
		 * pkgdb_retrieve, then add these. */
		if (File2Pkg) {
			char   *s;

			s = pkgdb_retrieve(*argv);

			if (s) {
				lpp = alloc_lpkg(s);
				TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
			} else
				errx(1, "No matching pkg for %s in pkgdb.", *argv);
		} else {
			if (ispkgpattern(*argv)) {
				int rc;
				rc = findmatchingname(_pkgdb_getPKGDB_DIR(), *argv, find_fn, NULL);
				if (rc == 0)
					errx(1, "No matching pkg for %s.", *argv);
				else if (rc == -1) 
					errx(1, "error expanding '%s' ('%s' nonexistant?)\n", *argv, _pkgdb_getPKGDB_DIR());
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
	if (TAILQ_FIRST(&pkgs) == NULL)
		warnx("missing package name(s)"), usage();
	if (!Fake && getuid() != 0)
		errx(1, "you must be root to delete packages");
	if (OnlyDeleteFromPkgDB) {
		/* Only delete the given packages' files from pkgdb, do not
		 * touch the pkg itself. Used by "make reinstall" in
		 * bsd.pkg.mk */
		char   *key, *val;

		if (pkgdb_open(0) == -1) {
			err(1, "cannot open %s", _pkgdb_getPKGDB_FILE());
		}

		error = 0;
		while ((key = pkgdb_iter())) {
			val = pkgdb_retrieve(key);
			if (val == NULL || *val == '\0')
				continue;

			lpp = TAILQ_FIRST(&pkgs);
			if (lpp != NULL) {
				do {
					if (strcmp(val, lpp->lp_name) == 0) {
						if (Verbose)
							printf("Removing file %s from pkgdb\n", key);

						errno = 0;
						if (pkgdb_remove(key)) {
							if (errno)
								printf("Error removing %s from pkgdb: %s\n", key, strerror(errno));
							else
								printf("Key %s not present in pkgdb?!\n", key);
							error = 1;
						}
					}

					lpp = TAILQ_NEXT(lpp, lp_link);
				} while (lpp != NULL);
			}
		}
		pkgdb_close();

		return error;

	} else if ((error = pkg_perform(&pkgs)) != 0) {
		if (Verbose)
			warnx("%d package deletion(s) failed", error);
		return error;
	} else
		return 0;
}
