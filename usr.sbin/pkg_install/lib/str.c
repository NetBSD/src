/*	$NetBSD: str.c,v 1.23.2.3 2001/05/01 11:01:09 he Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "Id: str.c,v 1.5 1997/10/08 07:48:21 charnier Exp";
#else
__RCSID("$NetBSD: str.c,v 1.23.2.3 2001/05/01 11:01:09 he Exp $");
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

#include <assert.h>
#include <err.h>
#include <fnmatch.h>
#include "lib.h"

/*
 * Return the filename portion of a path
 */
char   *
basename_of(char *str)
{
	char   *slash;

	return ((slash = strrchr(str, '/')) == (char *) NULL) ? str : slash + 1;
}

/*
 * Return the dirname portion of a path
 */
char   *
dirname_of(const char *path)
{
	size_t  cc;
	char   *s;
	char   *t;

	if ((s = strrchr(path, '/')) == NULL) {
		return ".";
	}
	if (s == path) {
		/* "/foo" -> return "/" */
		return "/";
	}
	cc = (size_t) (s - path);
	if ((t = (char *) malloc(cc + 1)) == (char *) NULL) {
		errx(1, "out of memory in dirname_of");
	}
	(void) memcpy(t, path, cc);
	t[cc] = 0;
	return t;
}

/*
 * Get a string parameter as a file spec or as a "contents follow -" spec
 */
char   *
get_dash_string(char **s)
{
	return *s = (**s == '-') ? strdup(*s + 1) : fileGetContents(*s);
}

/*
 * Lowercase a whole string
 */
void
str_lowercase(char *s)
{
	for (; *s; s++) {
		*s = tolower(*s);
	}
}

typedef enum deweyop_t {
	GT,
	GE,
	LT,
	LE
}       deweyop_t;

/*
 * Compare two dewey decimal numbers
 */
static int
deweycmp(char *a, deweyop_t op, char *b)
{
	int     ad;
	int     bd;
	char	*a_nb;
	char	*b_nb;
	int	in_nb = 0;
	int     cmp;

	assert(a != NULL);
	assert(b != NULL);

	/* Null out 'n' in any "nb" suffixes for initial pass */
	if ((a_nb = strstr(a, "nb")))
	    *a_nb = 0;
	if ((b_nb = strstr(b, "nb")))
	    *b_nb = 0;

	for (;;) {
		if (*a == 0 && *b == 0) {
			if (!in_nb && (a_nb || b_nb)) {
				/*
				 * If exact match on first pass, test
				 * "nb<X>" suffixes in second pass
				 */
				in_nb = 1;
				if (a_nb)
				    a = a_nb + 2;	/* Skip "nb" suffix */
				if (b_nb)
				    b = b_nb + 2;	/* Skip "nb" suffix */
			} else {
				cmp = 0;
				break;
			}
		}

		ad = bd = 0;
		for (; *a && *a != '.'; a++) {
			ad = (ad * 10) + (*a - '0');
		}
		for (; *b && *b != '.'; b++) {
			bd = (bd * 10) + (*b - '0');
		}
		if ((cmp = ad - bd) != 0) {
			break;
		}
		if (*a == '.')
			++a;
		if (*b == '.')
			++b;
	}
	/* Replace any nulled 'n' */
	if (a_nb)
		*a_nb = 'n';
	if (b_nb)
		*b_nb = 'n';
	return (op == GE) ? cmp >= 0 : (op == GT) ? cmp > 0 : (op == LE) ? cmp <= 0 : cmp < 0;
}

/*
 * Perform alternate match on "pkg" against "pattern",
 * calling pmatch (recursively) to resolve any other patterns.
 * Return 1 on match, 0 otherwise
 */
static int
alternate_match(const char *pattern, const char *pkg)
{
	char   *sep;
	char    buf[FILENAME_MAX];
	char   *last;
	char   *alt;
	char   *cp;
	int     cnt;
	int     found;

	if ((sep = strchr(pattern, '{')) == (char *) NULL) {
		errx(1, "alternate_match(): '{' expected in `%s'", pattern);
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
		errx(1, "Malformed alternate `%s'", pattern);
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
		if (pmatch(buf, pkg) == 1) {
			found = 1;
		}
	}
	return found;
}

/*
 * Perform dewey match on "pkg" against "pattern".
 * Return 1 on match, 0 otherwise
 */
static int
dewey_match(const char *pattern, const char *pkg)
{
	deweyop_t op;
	char   *cp;
	char   *sep;
	char   *ver;
	char    name[FILENAME_MAX];
	int     n;

	if ((sep = strpbrk(pattern, "<>")) == NULL) {
		errx(1, "dewey_match(): '<' or '>' expected in `%s'", pattern);
	}
	(void) snprintf(name, sizeof(name), "%.*s", (int) (sep - pattern), pattern);
	op = (*sep == '>') ? (*(sep + 1) == '=') ? GE : GT : (*(sep + 1) == '=') ? LE : LT;
	ver = (op == GE || op == LE) ? sep + 2 : sep + 1;
	n = (int) (sep - pattern);
	if ((cp = strrchr(pkg, '-')) != (char *) NULL) {
		if (strncmp(pkg, name, (size_t) (cp - pkg)) == 0 && n == cp - pkg) {
			if (deweycmp(cp + 1, op, ver)) {
				return 1;
			}
		}
	}
	return 0;
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
pmatch(const char *pattern, const char *pkg)
{
	if (strchr(pattern, '{') != (char *) NULL) {
		/* emulate csh-type alternates */
		return alternate_match(pattern, pkg);
	}
	if (strpbrk(pattern, "<>") != (char *) NULL) {
		/* perform relational dewey match on version number */
		return dewey_match(pattern, pkg);
	}
	if (strpbrk(pattern, "*?[]") != (char *) NULL) {
		/* glob match */
		return glob_match(pattern, pkg);
	}
	
	/* no alternate, dewey or glob match -> simple compare */
	return simple_match(pattern, pkg);
}

/*
 * Search dir for pattern, calling match(pkg_found, data) for every match.
 * Returns -1 on error, 1 if found, 0 otherwise.
 */
int
findmatchingname(const char *dir, const char *pattern, matchfn match, char *data)
{
	struct dirent *dp;
	char tmp_pattern[256];
	DIR    *dirp;
	int     found;
	char *pat_tgz, *file_tgz;		/* ptr to .tgz */
	char *pat_tbz, *file_tbz;		/* ptr to .tbz */
	char *pat_tbgz, *file_tbgz;		/* ptr to .t[bg]z */
	char pat_sfx[256], file_sfx[256];	/* suffixes */

	found = 0;
	if ((dirp = opendir(dir)) == (DIR *) NULL) {
		/* warnx("can't opendir dir '%s'", dir); */
		return -1;
	}

	/* chop any possible suffix off of 'pattern' and
	 * store it in pat_sfx
	 */
	strcpy(tmp_pattern, pattern);
	pat_sfx[0] = '\0';
	pat_tgz = strstr(tmp_pattern, ".tgz");
	if (pat_tgz) {
		/* strip off any ".tgz" */
		strcpy(pat_sfx, pat_tgz);
		*pat_tgz = '\0';
	}
	pat_tbz = strstr(tmp_pattern, ".tbz");
	if (pat_tbz) {
		/* strip off any ".tbz" */
		strcpy(pat_sfx, pat_tbz);
		*pat_tbz = '\0';
	}
	pat_tbgz = strstr(tmp_pattern, ".t[bg]z");
	if (pat_tbgz) {
		/* strip off any ".t[bg]z" */
		strcpy(pat_sfx, pat_tbgz);
		*pat_tbgz = '\0';
	}

	
	while ((dp = readdir(dirp)) != (struct dirent *) NULL) {
		char    tmp_file[FILENAME_MAX];
		
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		/* chop any possible suffix off of 'tmp_file' and
		 * store it in file_sfx
		 */
		strcpy(tmp_file, dp->d_name);
		file_sfx[0] = '\0';
		file_tgz = strstr(tmp_file, ".tgz");
		if (file_tgz) {
			/* strip off any ".tgz" */
			strcpy(file_sfx, file_tgz);
			*file_tgz = '\0';
		}
		file_tbz = strstr(tmp_file, ".tbz");
		if (file_tbz) {
			/* strip off any ".tbz" */
			strcpy(file_sfx, file_tbz);
			*file_tbz = '\0';
		}
		file_tbgz = strstr(tmp_file, ".t[bg]z");
		if (file_tbgz) {
			/* strip off any ".t[bg]z" */
			strcpy(file_sfx, file_tbgz);
			*file_tbgz = '\0';
		}

		/* we need to match pattern and suffix separately, in case
		 * each is a different pattern class (e.g. dewey and
		 * character class (.t[bg]z)) */
		if (pmatch(tmp_pattern, tmp_file)
		    && pmatch(pat_sfx, file_sfx)) {
			if (match) {
				match(dp->d_name, data);
				/* return value ignored for now */
			}
			found = 1;
		}
	}
	(void) closedir(dirp);
	return found;
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
 * Auxiliary function called by findbestmatchingname() if pkg > data
 * Also called for FTP matching
 */
int
findbestmatchingname_fn(const char *found, char *best)
{
	char *found_version, *best_version;
	char *found_tgz, *best_tgz;
	char *found_tbz, *best_tbz;
	char *found_tbgz, *best_tbgz;
	char found_no_sfx[255];
	char best_no_sfx[255];

	/* The same suffix-hack-off again, but we can't do it
	 * otherwise without changing the function call interface
	 */
	found_version = strrchr(found, '-');
	if (found_version) {
		/* skip '-', if any version found */
		found_version++;
	}
	found_tgz = strstr(found, ".tgz");
	if (found_tgz) {
		/* strip off any ".tgz" */
		strncpy(found_no_sfx, found_version, found_tgz-found_version);
		found_no_sfx[found_tgz-found_version] = '\0';
		found_version = found_no_sfx;
	}
	found_tbz = strstr(found, ".tbz");
	if (found_tbz) {
		/* strip off any ".tbz" */
		strncpy(found_no_sfx, found_version, found_tbz-found_version);
		found_no_sfx[found_tbz-found_version] = '\0';
		found_version = found_no_sfx;
	}
	found_tbgz = strstr(found, ".t[bg]z");
	if (found_tbgz) {
		/* strip off any ".t[bg]z" */
		strncpy(found_no_sfx, found_version, found_tbgz-found_version);
		found_no_sfx[found_tbgz-found_version] = '\0';
		found_version = found_no_sfx;
	}

	best_version=NULL;
	if (best && best[0] != '\0') {
		best_version = strrchr(best, '-');
		if (best_version) {
			/* skip '-' if any version found */
			best_version++;
		}
		best_tgz = strstr(best, ".tgz");
		if (best_tgz) {
			/* strip off any ".tgz" */
			strncpy(best_no_sfx, best_version, best_tgz-best_version);
			best_no_sfx[best_tgz-best_version] = '\0';
			best_version = best_no_sfx;
		}
		best_tbz = strstr(best, ".tbz");
		if (best_tbz) {
			/* strip off any ".tbz" */
			strncpy(best_no_sfx, best_version, best_tbz-best_version);
			best_no_sfx[best_tbz-best_version] = '\0';
			best_version = best_no_sfx;
		}
		best_tbgz = strstr(best, ".t[bg]z");
		if (best_tbgz) {
			/* strip off any ".t[bg]z" */
			strncpy(best_no_sfx, best_version, best_tbgz-best_version);
			best_no_sfx[best_tbgz-best_version] = '\0';
			best_version = best_no_sfx;
		}
	}

	if (found_version == NULL) {
		fprintf(stderr, "'%s' is not a usable package(version)\n",
			found);
	} else {
		/* if best_version==NULL only if best==NULL
		 * (or best[0]='\0') */
		if (best == NULL || best[0] == '\0' || deweycmp(found_version, GT, best_version)) {
			/* found pkg(version) is bigger than current "best"
			 * version - remember! */
			strcpy(best, found);
		}
	}

	return 0;
}

/*
 * Find best matching filename, i.e. the pkg with the highest
 * matching(!) version. 
 * Returns pointer to pkg name (which can be free(3)ed),
 * or NULL if no match is available.
 */
char *
findbestmatchingname(const char *dir, const char *pattern)
{
	char    buf[FILENAME_MAX];

	buf[0] = '\0';
	if (findmatchingname(dir, pattern, findbestmatchingname_fn, buf) > 0
	    && buf[0] != '\0') {
		return strdup(buf);
	}
	return NULL;
}

/*
 * Bounds-checking strncpy()
 */
char   *
strnncpy(char *to, size_t tosize, char *from, size_t cc)
{
	size_t  len;

	if ((len = cc) >= tosize - 1) {
		len = tosize - 1;
	}
	(void) strncpy(to, from, len);
	to[len] = 0;
	return to;
}

/*
 * Strip off any .tgz, .tbz or .t[bg]z suffix from fname,
 * and copy into buffer "buf"
 */
void
strip_txz(char *buf, char *fname)
{
	char *s;

	strcpy(buf, fname);
	
	s = strstr(buf, ".tgz");
	if (s) { *s = '\0'; }		/* strip off any ".tgz" */
	
	s = strstr(buf, ".tbz");
	if (s) { *s = '\0'; }		/* strip off any ".tbz" */
	
	s = strstr(buf, ".t[bg]z");
	if (s) { *s = '\0'; }		/* strip off any ".t[bg]z" */
}
