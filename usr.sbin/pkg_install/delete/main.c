/*	$NetBSD: main.c,v 1.10 1999/03/08 00:20:21 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char *rcsid = "from FreeBSD Id: main.c,v 1.11 1997/10/08 07:46:48 charnier Exp";
#else
__RCSID("$NetBSD: main.c,v 1.10 1999/03/08 00:20:21 hubertf Exp $");
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

static char Options[] = "hvDdnfFp:OrR";

char	*Prefix		= NULL;
char    *ProgramPath	= NULL;
Boolean	NoDeInstall	= FALSE;
Boolean	CleanDirs	= FALSE;
Boolean File2Pkg	= FALSE;
Boolean Recurse_up	= FALSE;
Boolean Recurse_down	= FALSE;
Boolean OnlyDeleteFromPkgDB = FALSE;

static void
usage(void)
{
    fprintf(stderr, "usage: pkg_delete [-vDdnFfOrR] [-p prefix] pkg-name ...\n");
    exit(1);
}

int
main(int argc, char **argv)
{
    int ch, error;
    char **pkgs, **start;

    ProgramPath = argv[0];

    pkgs = start = argv;
    while ((ch = getopt(argc, argv, Options)) != -1)
	switch(ch) {
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
	  
	case 'h':
	case '?':
	default:
	    usage();
	    break;
	}

    argc -= optind;
    argv += optind;

    /* Get all the remaining package names, if any */
    if (File2Pkg)
	if (pkgdb_open(1)==-1) {
	    err(1, "cannot open pkgdb");
	}
    /* Get all the remaining package names, if any */
    while (*argv) {
	/* pkgdb: if -F flag given, don't add pkgnames to *pkgs but
	 * rather resolve the given filenames to pkgnames using
	 * pkgdb_retrieve, then add them. 
	 */
	if (File2Pkg) {
	    char *s;

	    s = pkgdb_retrieve(*argv);

	    if (s)
		*pkgs++ = s;
	    else
		warnx("No matching pkg for %s.", *argv);
	} else {
	    *pkgs++ = *argv;
	}
	argv++;
    }

    if (File2Pkg)
	pkgdb_close();

    /* If no packages, yelp */
    if (pkgs == start)
	warnx("missing package name(s)"), usage();
    *pkgs = NULL;
    if (!Fake && getuid() != 0)
	errx(1, "you must be root to delete packages");
    if (OnlyDeleteFromPkgDB) {
	/* Only delete the given packages' files from pkgdb, do not touch
	 * the pkg itself. Used by "make reinstall" in bsd.pkg.mk
	 */
	char *key, *val;
	char **s;
	
	if (pkgdb_open(0)==-1) {
	    err(1, "cannot open %s", _pkgdb_getPKGDB_FILE());
	}

	error = 0;
	while ((key=pkgdb_iter())) {
	    val=pkgdb_retrieve(key);

	    for (s=start; *s; s++) {
		if (strcmp(val, *s) == 0) {
		    if (Verbose)
			printf("Removing file %s from pkgdb\n", key);

		    errno=0;
		    if (pkgdb_remove(key)) {
			if (errno)
			    printf("Error removing %s from pkgdb: %s\n", key, strerror(errno));
			else
			    printf("Key %s not present in pkgdb?!\n", key);
			error = 1;
		    }
		}
	    }
	}
	pkgdb_close();

	return error;
	
    } else if ((error = pkg_perform(start)) != 0) {
	if (Verbose)
	    warnx("%d package deletion(s) failed", error);
	return error;
    }
    else
	return 0;
}
