/*	$NetBSD: main.c,v 1.23.2.1 2002/06/28 12:42:49 lukem Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char *rcsid = "from FreeBSD Id: main.c,v 1.16 1997/10/08 07:45:43 charnier Exp";
#else
__RCSID("$NetBSD: main.c,v 1.23.2.1 2002/06/28 12:42:49 lukem Exp $");
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

#include <err.h>
#include <sys/param.h>
#include "lib.h"
#include "add.h"
#include "verify.h"

static char Options[] = "hVvIRfnp:SMs:t:u";

char   *Prefix = NULL;
Boolean NoInstall = FALSE;
Boolean NoRecord = FALSE;

char   *Mode = NULL;
char   *Owner = NULL;
char   *Group = NULL;
char   *PkgName = NULL;
char   *Directory = NULL;
char    FirstPen[FILENAME_MAX];
add_mode_t AddMode = NORMAL;
int	upgrade = 0;

static void
usage(void)
{
	(void) fprintf(stderr, "%s\n%s\n",
	    "usage: pkg_add [-hVvInfRMSu] [-t template] [-p prefix]",
	    "               [-s verification-type] pkg-name [pkg-name ...]");
	exit(1);
}

int
main(int argc, char **argv)
{
	int     ch, error=0;
	lpkg_head_t pkgs;
	lpkg_t *lpp;
	char   *cp;
	char    pkgname[MAXPATHLEN];

	while ((ch = getopt(argc, argv, Options)) != -1) {
		switch (ch) {
		case 'v':
			Verbose = TRUE;
			break;

		case 'p':
			Prefix = optarg;
			break;

		case 'I':
			NoInstall = TRUE;
			break;

		case 'R':
			NoRecord = TRUE;
			break;

		case 'f':
			Force = TRUE;
			break;

		case 'n':
			Fake = TRUE;
			Verbose = TRUE;
			break;

		case 's':
			set_verification(optarg);
			break;

		case 't':
			strcpy(FirstPen, optarg);
			break;

		case 'S':
			AddMode = SLAVE;
			break;

		case 'M':
			AddMode = MASTER;
			break;

		case 'V':
			show_version();
			/* NOTREACHED */

		case 'u':
			upgrade = 1;
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

	TAILQ_INIT(&pkgs);

	if (AddMode != SLAVE) {
		/* Get all the remaining package names, if any */
		for (ch = 0; *argv; ch++, argv++) {
			if (!strcmp(*argv, "-"))	/* stdin? */
				lpp = alloc_lpkg("-");
			else if (IS_URL(*argv))	/* preserve URLs */
				lpp = alloc_lpkg(*argv);
			else {	/* expand all pathnames to fullnames */
				char   *s;

				if (fexists(*argv)) {	/* refers to a file directly */
					if (!(cp = realpath(*argv, pkgname))) {
						lpp = NULL;
						warn("realpath failed for '%s'", *argv);
					} else
						lpp = alloc_lpkg(cp);
				} else if (ispkgpattern(*argv)) {
					    if ((s = findbestmatchingname(dirname_of(*argv),
					    basename_of(*argv))) != NULL) {
						    char tmp[FILENAME_MAX];
						    
						    snprintf(tmp, sizeof(tmp), "%s/%s", dirname_of(*argv), s);
							free(s);
						    
						    if (Verbose)
							    printf("Using %s for %s\n", tmp, *argv);
						    
						    if (!(cp = realpath(tmp, pkgname))) {
							    lpp = NULL;
							    warn("realpath failed for '%s'", tmp);
							    error++;
						    } else
							    lpp = alloc_lpkg(cp);
					    } else {
						    lpp = NULL;
						    warnx("can't find package pattern '%s'", *argv);
						    error++;
					    }
				} else {
					/* Maybe just a pkg name w/o pattern was given */
					char tmp[FILENAME_MAX];
						
					if (*argv[0] == '/'){
						/* given arg has path - ignore PKG_PATH */
					snprintf(tmp, sizeof(tmp), "%s-[0-9]*.t[bg]z", *argv);
						s = findbestmatchingname(dirname_of(tmp),
									 basename_of(tmp));
					} else {
						/* try all elements of PKG_PATH, see also loop
						 * in file.c::fileFindByPath() */
						/* This code prefers local pkgs over remote
						 * pkgs - feature? */
						char *e;
						
						if (getenv("PKG_PATH"))
							e = strdup(getenv("PKG_PATH"));
						else
							e = NULL;
						s = NULL;
						while(s==NULL && e && *e) {
							char *e2 = strsep(&e, ";");
							
							if (IS_URL(e2)) {
								/* Let some other code do this */
								;
							} else {
								snprintf(tmp, sizeof(tmp), "%s/%s-[0-9]*.t[bg]z",
									 e2, *argv);
								s = findbestmatchingname(dirname_of(tmp),
							       basename_of(tmp));
							}
						}
					}
					if (s) {
						char tmp2[FILENAME_MAX];
						
						snprintf(tmp2, sizeof(tmp2), "%s/%s", dirname_of(tmp), s);
						free(s);
						
						if (Verbose)
							printf("Using %s for %s\n", tmp2, *argv);

						if (!(cp = realpath(tmp2, pkgname))) {
							lpp = NULL;
							warn("realpath failed for '%s'", tmp2);
							error++;
						} else
							lpp = alloc_lpkg(cp);
					} else {
						/* No go there... */
						/* look for the file(pattern) in the expected places */
						if (!(cp = fileFindByPath(NULL, *argv))) {
							lpp = NULL;
							warnx("can't find package '%s'", *argv);
							error++;
						} else
							lpp = alloc_lpkg(cp);
					}
				}
			}
			if (lpp)
				TAILQ_INSERT_TAIL(&pkgs, lpp, lp_link);
		}
	}
	/* If no packages, yelp */
	else if (!ch)
		warnx("missing package name(s)"), usage();
	else if (ch > 1 && AddMode == MASTER)
		warnx("only one package name may be specified with master mode"),
		    usage();
	error += pkg_perform(&pkgs);
	if (error  != 0) {
		if (Verbose)
			warnx("%d package addition(s) failed", error);
		exit(1);
	}
	exit(0);
}
