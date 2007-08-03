/*	$NetBSD: opattern.c,v 1.1.1.1 2007/08/03 13:58:21 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "Id: str.c,v 1.5 1997/10/08 07:48:21 charnier Exp";
#else
__RCSID("$NetBSD: opattern.c,v 1.1.1.1 2007/08/03 13:58:21 joerg Exp $");
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
 * Miscellaneous string utilities.
 *
 */

#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_FNMATCH_H
#include <fnmatch.h>
#endif
#include "lib.h"
#include "dewey.h"

/* pull in definitions and macros for resizing arrays as we go */
#include "defs.h"

/*
 * Perform alternate match on "pkg" against "pattern",
 * calling pkg_match (recursively) to resolve any other patterns.
 * Return 1 on match, 0 otherwise
 */
static int
alternate_match(const char *pattern, const char *pkg)
{
	char   *sep;
	char    buf[MaxPathSize];
	char   *last;
	char   *alt;
	char   *cp;
	int     cnt;
	int     found;

	if ((sep = strchr(pattern, '{')) == (char *) NULL) {
		errx(EXIT_FAILURE, "alternate_match(): '{' expected in `%s'", pattern);
	}
	(void) strncpy(buf, pattern, (size_t) (sep - pattern));
	alt = &buf[sep - pattern];
	last = (char *) NULL;
	for (cnt = 0, cp = sep; *cp && last == (char *) NULL; cp++) {
		if (*cp == '{') {
			cnt++;
		} else if (*cp == '}' && --cnt == 0 && last == (char *) NULL) {
			last = cp + 1;
		}
	}
	if (cnt != 0) {
		errx(EXIT_FAILURE, "Malformed alternate `%s'", pattern);
	}
	for (found = 0, cp = sep + 1; *sep != '}'; cp = sep + 1) {
		for (cnt = 0, sep = cp; cnt > 0 || (cnt == 0 && *sep != '}' && *sep != ','); sep++) {
			if (*sep == '{') {
				cnt++;
			} else if (*sep == '}') {
				cnt--;
			}
		}
		(void) snprintf(alt, sizeof(buf) - (alt - buf), "%.*s%s", (int) (sep - cp), cp, last);
		if (pkg_match(buf, pkg) == 1) {
			found = 1;
		}
	}
	return found;
}

/*
 * Perform glob match on "pkg" against "pattern".
 * Return 1 on match, 0 otherwise
 */
static int
glob_match(const char *pattern, const char *pkg)
{
	return fnmatch(pattern, pkg, FNM_PERIOD) == 0;
}

/*
 * Perform simple match on "pkg" against "pattern". 
 * Return 1 on match, 0 otherwise
 */
static int
simple_match(const char *pattern, const char *pkg)
{
	return strcmp(pattern, pkg) == 0;
}

/*
 * Match pkg against pattern, return 1 if matching, 0 else
 */
int
pkg_match(const char *pattern, const char *pkg)
{
	if (strchr(pattern, '{') != (char *) NULL) {
		/* emulate csh-type alternates */
		return alternate_match(pattern, pkg);
	}
	if (strpbrk(pattern, "<>") != (char *) NULL) {
		int ret;

		/* perform relational dewey match on version number */
		ret = dewey_match(pattern, pkg);
		if (ret < 0)
			errx(EXIT_FAILURE, "dewey_match returned error");
		return ret;
	}
	if (strpbrk(pattern, "*?[]") != (char *) NULL) {
		/* glob match */
		return glob_match(pattern, pkg);
	}
	
	/* no alternate, dewey or glob match -> simple compare */
	return simple_match(pattern, pkg);
}

int
pkg_order(const char *pattern, const char *first_pkg, const char *second_pkg)
{
	const char *first_version = strrchr(first_pkg, '/');
	const char *second_version = strrchr(second_pkg, '/');

	if (first_version == NULL || !pkg_match(pattern, first_pkg))
		return pkg_match(pattern, second_pkg) ? 2 : 0;

	if (second_version == NULL || !pkg_match(pattern, second_pkg))
		return pkg_match(pattern, first_pkg) ? 1 : 0;

	if (dewey_cmp(first_version, DEWEY_GT, second_version))
		return 1;
	else
		return 2;
}
