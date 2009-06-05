/*	$NetBSD: main.c,v 1.1.1.3.2.2 2009/06/05 17:01:57 snj Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
__RCSID("$NetBSD: main.c,v 1.1.1.3.2.2 2009/06/05 17:01:57 snj Exp $");

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

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include "lib.h"
#include "add.h"

static char Options[] = "AIK:LP:RVW:fhm:np:t:uvw:";

char   *Destdir = NULL;
char   *OverrideMachine = NULL;
char   *Prefix = NULL;
char   *View = NULL;
char   *Viewbase = NULL;
Boolean NoView = FALSE;
Boolean NoInstall = FALSE;
Boolean NoRecord = FALSE;
Boolean Automatic = FALSE;

int     Replace = 0;

static void
usage(void)
{
	(void) fprintf(stderr, "%s\n%s\n%s\n%s\n",
	    "usage: pkg_add [-AfhILnRuVv] [-C config] [-P destdir] [-K pkg_dbdir]",
	    "               [-m machine] [-p prefix] [-s verification-type",
	    "               [-W viewbase] [-w view]\n",
	    "               [[ftp|http]://[user[:password]@]host[:port]][/path/]pkg-name ...");
	exit(1);
}

int
main(int argc, char **argv)
{
	int     ch, error=0;
	lpkg_head_t pkgs;

	setprogname(argv[0]);
	while ((ch = getopt(argc, argv, Options)) != -1) {
		switch (ch) {
		case 'A':
			Automatic = TRUE;
			break;

		case 'C':
			config_file = optarg;

		case 'P':
			Destdir = optarg;
			break;

		case 'f':
			Force = TRUE;
			break;

		case 'I':
			NoInstall = TRUE;
			break;

		case 'K':
			_pkgdb_setPKGDB_DIR(optarg);
			break;

		case 'L':
			NoView = TRUE;
			break;

		case 'R':
			NoRecord = TRUE;
			break;

		case 'm':
			OverrideMachine = optarg;
			break;

		case 'n':
			Fake = TRUE;
			Verbose = TRUE;
			break;

		case 'p':
			Prefix = optarg;
			break;

		case 'u':
			Replace++;
			break;

		case 'V':
			show_version();
			/* NOTREACHED */

		case 'v':
			Verbose = TRUE;
			break;

		case 'W':
			Viewbase = optarg;
			break;

		case 'w':
			View = optarg;
			break;

		case 'h':
		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	pkg_install_config();

	process_pkg_path();
	TAILQ_INIT(&pkgs);

	if (argc == 0) {
		/* If no packages, yelp */
		warnx("missing package name(s)");
		usage();
	}

	/* Get all the remaining package names, if any */
	for (; argc > 0; --argc, ++argv) {
		lpkg_t *lpp;

		if (IS_STDIN(*argv))
			lpp = alloc_lpkg("-");
		else
			lpp = alloc_lpkg(*argv);

		TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
	}

	error += pkg_perform(&pkgs);
	if (error != 0) {
		warnx("%d package addition%s failed", error, error == 1 ? "" : "s");
		exit(1);
	}
	exit(0);
}
