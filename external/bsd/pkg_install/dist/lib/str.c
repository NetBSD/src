/*	$NetBSD: str.c,v 1.1.1.1.4.2 2008/10/19 22:40:51 haad Exp $	*/

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
__RCSID("$NetBSD: str.c,v 1.1.1.1.4.2 2008/10/19 22:40:51 haad Exp $");
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
 * Return the suffix portion of a path
 */
const char *
suffix_of(const char *str)
{
	const char *dot;

	return ((dot = strrchr(basename_of(str), '.')) == NULL) ? "" : dot + 1;
}

/*
 * Return the filename portion of a path
 */
const char *
basename_of(const char *str)
{
	const char *slash;

	return ((slash = strrchr(str, '/')) == NULL) ? str : slash + 1;
}

/*
 * Return the dirname portion of a path
 */
const char *
dirname_of(const char *path)
{
	size_t  cc;
	char   *s;
	static char buf[MaxPathSize];

	if ((s = strrchr(path, '/')) == NULL) {
		return ".";
	}
	if (s == path) {
		/* "/foo" -> return "/" */
		return "/";
	}
	cc = (size_t) (s - path);
	if (cc >= sizeof(buf))
		errx(EXIT_FAILURE, "dirname_of: too long dirname: '%s'", path);
	(void) memcpy(buf, path, cc);
	buf[cc] = 0;
	return buf;
}

/*
 * Does the pkgname contain any of the special chars ("{[]?*<>")? 
 * If so, return 1, else 0
 */
int
ispkgpattern(const char *pkg)
{
	return strpbrk(pkg, "<>[]?*{") != NULL;
}

/*
 * Strip off any .tgz, .tbz or .t[bg]z suffix from fname,
 * and copy into buffer "buf", the suffix is stored in "sfx"
 * if "sfx" is not NULL. If no suffix is found, "sfx" is set
 * to an empty string. 
 */
void
strip_txz(char *buf, char *sfx, const char *fname)
{
	static const char *const suffixes[] = {
		".tgz", ".tbz", ".t[bg]z", 0};
	const char *const *suffixp;
	size_t len;

	len = strlen(fname);
	assert(len < PKG_PATTERN_MAX);

	if (sfx)
		sfx[0] = '\0';

	for (suffixp = suffixes; *suffixp; suffixp++) {
		size_t suffixlen = strlen(*suffixp);

		if (memcmp(&fname[len - suffixlen], *suffixp, suffixlen))
			continue;

		/* matched! */
		memcpy(buf, fname, len - suffixlen);
		buf[len - suffixlen] = 0;
		if (sfx) {
			if (suffixlen >= PKG_SUFFIX_MAX)
				errx(EXIT_FAILURE, "too long suffix '%s'", fname);
			memcpy(sfx, *suffixp, suffixlen+1);
			return;
		}
	}

	/* not found */
	memcpy(buf, fname, len+1);
}
