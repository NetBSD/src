/*	$NetBSD: main.c,v 1.1.1.2.2.2 2007/08/03 13:58:19 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static char *rcsid = "from FreeBSD Id: main.c,v 1.16 1997/10/08 07:45:43 charnier Exp";
#else
__RCSID("$NetBSD: main.c,v 1.1.1.2.2.2 2007/08/03 13:58:19 joerg Exp $");
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
#include "verify.h"

static char Options[] = "AIK:LRVW:fhnp:s:t:uvw:";

char   *Prefix = NULL;
char   *View = NULL;
char   *Viewbase = NULL;
Boolean NoView = FALSE;
Boolean NoInstall = FALSE;
Boolean NoRecord = FALSE;
Boolean Automatic = FALSE;

char   *Mode = NULL;
char   *Owner = NULL;
char   *Group = NULL;
char   *PkgName = NULL;
char   *Directory = NULL;
char    FirstPen[MaxPathSize];
int     Replace = 0;

static void
usage(void)
{
	(void) fprintf(stderr, "%s\n%s\n%s\n",
	    "usage: pkg_add [-AfhILnRuVv] [-K pkg_dbdir] [-p prefix]",
	    "               [-s verification-type] [-t template] [-W viewbase] [-w view]",
	    "               [[ftp|http]://[user[:password]@]host[:port]][/path/]pkg-name ...");
	exit(1);
}

int
main(int argc, char **argv)
{
	int     ch, error=0;
	lpkg_head_t pkgs;
	struct rlimit rlim;
	int rc;

	setprogname(argv[0]);
	while ((ch = getopt(argc, argv, Options)) != -1) {
		switch (ch) {
		case 'A':
			Automatic = TRUE;
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

		case 'n':
			Fake = TRUE;
			Verbose = TRUE;
			break;

		case 'p':
			Prefix = optarg;
			break;

		case 's':
			set_verification(optarg);
			break;

		case 't':
			strlcpy(FirstPen, optarg, sizeof(FirstPen));
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

	path_create(getenv("PKG_PATH"));
	TAILQ_INIT(&pkgs);

	/* Get all the remaining package names, if any */
	for (ch = 0; *argv; ch++, argv++) {
		lpkg_t *lpp;

		if (IS_STDIN(*argv))
			lpp = alloc_lpkg("-");
		else
			lpp = alloc_lpkg(*argv);

		TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
	}

	if (!ch) {
		/* If no packages, yelp */
		warnx("missing package name(s)"), usage();
	}
	
	/* Increase # of max. open file descriptors as high as possible */
	rc = getrlimit(RLIMIT_NOFILE, &rlim);
	if (rc == -1) {
	  	warn("cannot retrieve max. number of open files resource limit");
	} else {
	   	rlim.rlim_cur = rlim.rlim_max;
		rc = setrlimit(RLIMIT_NOFILE, &rlim);
		if (rc == -1) {
		  	warn("cannot increase max. number of open files resource limit, try 'ulimit'");
		} else {
		  	if (Verbose)
		  		printf("increasing RLIMIT_NOFILE to max. %ld open files\n", (long)rlim.rlim_cur);
		}
	}

	error += pkg_perform(&pkgs);
	if (error != 0) {
		warnx("%d package addition%s failed", error, error == 1 ? "" : "s");
		exit(1);
	}
	exit(0);
}
