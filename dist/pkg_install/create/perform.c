/*	$NetBSD: perform.c,v 1.1.1.2 2007/08/03 13:58:20 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.38 1997/10/13 15:03:51 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.1.1.2 2007/08/03 13:58:20 joerg Exp $");
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
 * 18 July 1993
 *
 * This is the main body of the create module.
 *
 */

#include "lib.h"
#include "create.h"

#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

static void
sanity_check(void)
{
	if (!Comment)
		errx(2, "required package comment string is missing (-c comment)");
	if (!Desc)
		errx(2, "required package description string is missing (-d desc)");
	if (!Contents)
		errx(2, "required package contents list is missing (-f [-]file)");
}

int
pkg_perform(const char *pkg)
{
	char   *cp;
	FILE   *pkg_in;
	package_t plist;
	char	installed[MaxPathSize];
	const char *full_pkg, *suffix;
	char *allocated_pkg;
	int retval;

	/* Break the package name into base and desired suffix (if any) */
	if ((cp = strrchr(pkg, '.')) != NULL) {
		if ((allocated_pkg = malloc(cp - pkg + 1)) == NULL)
			err(2, "malloc failed");
		memcpy(allocated_pkg, pkg, cp - pkg);
		allocated_pkg[cp - pkg] = '\0';
		suffix = cp + 1;
		full_pkg = pkg;
		pkg = allocated_pkg;
	} else {
		allocated_pkg = NULL;
		full_pkg = pkg;
		suffix = "tgz";
	}

	/* Preliminary setup */
	sanity_check();
	if (Verbose && !PlistOnly)
		printf("Creating package %s\n", pkg);
	get_dash_string(&Comment);
	get_dash_string(&Desc);
	if (IS_STDIN(Contents))
		pkg_in = stdin;
	else {
		pkg_in = fopen(Contents, "r");
		if (!pkg_in)
			errx(2, "unable to open contents file '%s' for input", Contents);
	}
	plist.head = plist.tail = NULL;

	/* If a SrcDir override is set, add it now */
	if (SrcDir) {
		if (Verbose && !PlistOnly)
			printf("Using SrcDir value of %s\n", (realprefix) ? realprefix : SrcDir);
		add_plist(&plist, PLIST_SRC, SrcDir);
	}

	/* Stick the dependencies, if any, at the top */
	if (Pkgdeps) {
		if (Verbose && !PlistOnly)
			printf("Registering depends:");
		while (Pkgdeps) {
			cp = strsep(&Pkgdeps, " \t\n");
			if (*cp) {
				if (findmatchingname(_pkgdb_getPKGDB_DIR(), cp, note_whats_installed, installed) > 0) {
					add_plist(&plist, PLIST_BLDDEP, installed);
				}
				add_plist(&plist, PLIST_PKGDEP, cp);
				if (Verbose && !PlistOnly)
					printf(" %s", cp);
			}
		}
		if (Verbose && !PlistOnly)
			printf(".\n");
	}

	/*
	 * Put the build dependencies after the dependencies.
	 * This works due to the evaluation order in pkg_add.
	 */
	if (BuildPkgdeps) {
		if (Verbose && !PlistOnly)
			printf("Registering build depends:");
		while (BuildPkgdeps) {
			cp = strsep(&BuildPkgdeps, " \t\n");
			if (*cp) {
				if (findmatchingname(_pkgdb_getPKGDB_DIR(), cp, note_whats_installed, installed) > 0) {
					add_plist(&plist, PLIST_BLDDEP, installed);
					if (Verbose && !PlistOnly)
						printf(" %s", cp);
				}
			}
		}
		if (Verbose && !PlistOnly)
			printf(".\n");
	}

	/* Put the conflicts directly after the dependencies, if any */
	if (Pkgcfl) {
		if (Verbose && !PlistOnly)
			printf("Registering conflicts:");
		while (Pkgcfl) {
			cp = strsep(&Pkgcfl, " \t\n");
			if (*cp) {
				add_plist(&plist, PLIST_PKGCFL, cp);
				if (Verbose && !PlistOnly)
					printf(" %s", cp);
			}
		}
		if (Verbose && !PlistOnly)
			printf(".\n");
	}

	/* Slurp in the packing list */
	read_plist(&plist, pkg_in);

	if (pkg_in != stdin)
		fclose(pkg_in);

	/* Prefix should override the packing list */
	if (Prefix) {
		delete_plist(&plist, FALSE, PLIST_CWD, NULL);
		add_plist_top(&plist, PLIST_CWD, Prefix);
	}
	/*
         * Run down the list and see if we've named it, if not stick in a name
         * at the top.
         */
	if (find_plist(&plist, PLIST_NAME) == NULL) {
		add_plist_top(&plist, PLIST_NAME, basename_of(pkg));
	}

	/* Make first "real contents" pass over it */
	check_list(&plist, basename_of(pkg));

	/*
         * We're just here for to dump out a revised plist for the FreeBSD ports
         * hack.  It's not a real create in progress.
         */
	if (PlistOnly) {
		write_plist(&plist, stdout, realprefix);
		retval = TRUE;
	} else {
#ifdef BOOTSTRAP
		warnx("Package building is not supported in bootstrap mode");
		retval = FALSE;
#else
		retval = pkg_build(pkg, full_pkg, suffix, &plist);
#endif
	}

	/* Cleanup */
	free(Comment);
	free(Desc);
	free_plist(&plist);

	free(allocated_pkg);

	return retval;
}
