/*	$NetBSD: main.c,v 1.7 1999/01/19 17:01:59 hubertf Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char *rcsid = "from FreeBSD Id: main.c,v 1.11 1997/10/08 07:46:48 charnier Exp";
#else
__RCSID("$NetBSD: main.c,v 1.7 1999/01/19 17:01:59 hubertf Exp $");
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
#include "lib.h"
#include "delete.h"

static char Options[] = "hvDdnfFp:";

char	*Prefix		= NULL;
Boolean	NoDeInstall	= FALSE;
Boolean	CleanDirs	= FALSE;
Boolean File2Pkg	= FALSE;

static void
usage(void)
{
    fprintf(stderr, "usage: pkg_delete [-vDdnFf] [-p prefix] pkg-name ...\n");
    exit(1);
}

int
main(int argc, char **argv)
{
    int ch, error;
    char **pkgs, **start;

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
    if ((error = pkg_perform(start)) != 0) {
	if (Verbose)
	    warnx("%d package deletion(s) failed", error);
	return error;
    }
    else
	return 0;
}
