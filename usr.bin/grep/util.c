/*	$NetBSD: util.c,v 1.13.4.1 2012/05/23 10:08:24 yamt Exp $	*/
/*	$FreeBSD: head/usr.bin/grep/util.c 211496 2010-08-19 09:28:59Z des $	*/
/*	$OpenBSD: util.c,v 1.39 2010/07/02 22:18:03 tedu Exp $	*/

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coïdan Smørgrav
 * Copyright (C) 2008-2010 Gabor Kovesdan <gabor@FreeBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: util.c,v 1.13.4.1 2012/05/23 10:08:24 yamt Exp $");

#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <fts.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "grep.h"

static bool	 first, first_global = true;
static unsigned long long since_printed;

static int	 procline(struct str *l, int);

bool
file_matching(const char *fname)
{
	char *fname_base, *fname_copy;
	unsigned int i;
	bool ret;

	ret = finclude ? false : true;
	fname_copy = grep_strdup(fname);
	fname_base = basename(fname_copy);

	for (i = 0; i < fpatterns; ++i) {
		if (fnmatch(fpattern[i].pat, fname, 0) == 0 ||
		    fnmatch(fpattern[i].pat, fname_base, 0) == 0) {
			if (fpattern[i].mode == EXCL_PAT)
				return (false);
			else
				ret = true;
		}
	}
	free(fname_copy);
	return (ret);
}

static inline bool
dir_matching(const char *dname)
{
	unsigned int i;
	bool ret;

	ret = dinclude ? false : true;

	for (i = 0; i < dpatterns; ++i) {
		if (dname != NULL &&
		    fnmatch(dname, dpattern[i].pat, 0) == 0) {
			if (dpattern[i].mode == EXCL_PAT)
				return (false);
			else
				ret = true;
		}
	}
	return (ret);
}

/*
 * Processes a directory when a recursive search is performed with
 * the -R option.  Each appropriate file is passed to procfile().
 */
int
grep_tree(char **argv)
{
	FTS *fts;
	FTSENT *p;
	char *d, *dir = NULL;
	int c, fts_flags;
	bool ok;

	c = fts_flags = 0;

	switch(linkbehave) {
	case LINK_EXPLICIT:
		fts_flags = FTS_COMFOLLOW;
		break;
	case LINK_SKIP:
		fts_flags = FTS_PHYSICAL;
		break;
	default:
		fts_flags = FTS_LOGICAL;
			
	}

	fts_flags |= FTS_NOSTAT | FTS_NOCHDIR;

	if (!(fts = fts_open(argv, fts_flags, NULL)))
		err(2, "fts_open");
	while ((p = fts_read(fts)) != NULL) {
		switch (p->fts_info) {
		case FTS_DNR:
			/* FALLTHROUGH */
		case FTS_ERR:
			errx(2, "%s: %s", p->fts_path, strerror(p->fts_errno));
			break;
		case FTS_D:
			/* FALLTHROUGH */
		case FTS_DP:
			break;
		case FTS_DC:
			/* Print a warning for recursive directory loop */
			warnx("warning: %s: recursive directory loop",
				p->fts_path);
			break;
		default:
			/* Check for file exclusion/inclusion */
			ok = true;
			if (dexclude || dinclude) {
				if ((d = strrchr(p->fts_path, '/')) != NULL) {
					dir = grep_malloc(sizeof(char) *
					    (d - p->fts_path + 1));
					memcpy(dir, p->fts_path,
					    d - p->fts_path);
					dir[d - p->fts_path] = '\0';
				}
				ok = dir_matching(dir);
				free(dir);
				dir = NULL;
			}
			if (fexclude || finclude)
				ok &= file_matching(p->fts_path);

			if (ok)
				c += procfile(p->fts_path);
			break;
		}
	}

	fts_close(fts);
	return (c);
}

/*
 * Opens a file and processes it.  Each file is processed line-by-line
 * passing the lines to procline().
 */
int
procfile(const char *fn)
{
	struct file *f;
	struct stat sb;
	struct str ln;
	mode_t s;
	int c, t;

	if (mflag && (mcount <= 0))
		return (0);

	if (strcmp(fn, "-") == 0) {
		fn = label != NULL ? label : getstr(1);
		f = grep_open(NULL);
	} else {
		if (!stat(fn, &sb)) {
			/* Check if we need to process the file */
			s = sb.st_mode & S_IFMT;
			if (s == S_IFDIR && dirbehave == DIR_SKIP)
				return (0);
			if ((s == S_IFIFO || s == S_IFCHR || s == S_IFBLK
				|| s == S_IFSOCK) && devbehave == DEV_SKIP)
					return (0);
		}
		f = grep_open(fn);
	}
	if (f == NULL) {
		if (!sflag)
			warn("%s", fn);
		if (errno == ENOENT)
			notfound = true;
		return (0);
	}

	ln.file = grep_malloc(strlen(fn) + 1);
	strcpy(ln.file, fn);
	ln.line_no = 0;
	ln.len = 0;
	tail = 0;
	ln.off = -1;

	for (first = true, c = 0;  c == 0 || !(lflag || qflag); ) {
		ln.off += ln.len + 1;
		if ((ln.dat = grep_fgetln(f, &ln.len)) == NULL || ln.len == 0)
			break;
		if (ln.len > 0 && ln.dat[ln.len - 1] == line_sep)
			--ln.len;
		ln.line_no++;

		/* Return if we need to skip a binary file */
		if (f->binary && binbehave == BINFILE_SKIP) {
			grep_close(f);
			free(ln.file);
			free(f);
			return (0);
		}
		/* Process the file line-by-line */
		t = procline(&ln, f->binary);
		c += t;

		/* Count the matches if we have a match limit */
		if (mflag) {
			mcount -= t;
			if (mcount <= 0)
				break;
		}
	}
	if (Bflag > 0)
		clearqueue();
	grep_close(f);

	if (cflag) {
		if (!hflag)
			printf("%s:", ln.file);
		printf("%u%c", c, line_sep);
	}
	if (lflag && !qflag && c != 0)
		printf("%s%c", fn, line_sep);
	if (Lflag && !qflag && c == 0)
		printf("%s%c", fn, line_sep);
	if (c && !cflag && !lflag && !Lflag &&
	    binbehave == BINFILE_BIN && f->binary && !qflag)
		printf(getstr(8), fn);

	free(ln.file);
	free(f);
	return (c);
}

#define iswword(x)	(iswalnum((x)) || (x) == L'_')

/*
 * Processes a line comparing it with the specified patterns.  Each pattern
 * is looped to be compared along with the full string, saving each and every
 * match, which is necessary to colorize the output and to count the
 * matches.  The matching lines are passed to printline() to display the
 * appropriate output.
 */
static int
procline(struct str *l, int nottext)
{
	regmatch_t matches[MAX_LINE_MATCHES];
	regmatch_t pmatch;
	size_t st = 0;
	unsigned int i;
	int c = 0, m = 0, r = 0;

	/* Loop to process the whole line */
	while (st <= l->len) {
		pmatch.rm_so = st;
		pmatch.rm_eo = l->len;

		/* Loop to compare with all the patterns */
		for (i = 0; i < patterns; i++) {
/*
 * XXX: grep_search() is a workaround for speed up and should be
 * removed in the future.  See fastgrep.c.
 */
			if (fg_pattern[i].pattern) {
				r = grep_search(&fg_pattern[i],
				    (unsigned char *)l->dat,
				    l->len, &pmatch);
				r = (r == 0) ? 0 : REG_NOMATCH;
				st = pmatch.rm_eo;
			} else {
				r = regexec(&r_pattern[i], l->dat, 1,
				    &pmatch, eflags);
				r = (r == 0) ? 0 : REG_NOMATCH;
				st = pmatch.rm_eo;
			}
			if (r == REG_NOMATCH)
				continue;
			/* Check for full match */
			if (xflag &&
			    (pmatch.rm_so != 0 ||
			     (size_t)pmatch.rm_eo != l->len))
				continue;
			/* Check for whole word match */
			if (fg_pattern[i].word && pmatch.rm_so != 0) {
				wint_t wbegin, wend;

				wbegin = wend = L' ';
				if (pmatch.rm_so != 0 &&
				    sscanf(&l->dat[pmatch.rm_so - 1],
				    "%lc", &wbegin) != 1)
					continue;
				if ((size_t)pmatch.rm_eo != l->len &&
				    sscanf(&l->dat[pmatch.rm_eo],
				    "%lc", &wend) != 1)
					continue;
				if (iswword(wbegin) || iswword(wend))
					continue;
			}
			c = 1;
			if (m < MAX_LINE_MATCHES)
				matches[m++] = pmatch;
			/* matches - skip further patterns */
			if ((color != NULL && !oflag) || qflag || lflag)
				break;
		}

		if (vflag) {
			c = !c;
			break;
		}
		/* One pass if we are not recording matches */
		if ((color != NULL && !oflag) || qflag || lflag)
			break;

		if (st == (size_t)pmatch.rm_so)
			break; 	/* No matches */
	}

	if (c && binbehave == BINFILE_BIN && nottext)
		return (c); /* Binary file */

	/* Dealing with the context */
	if ((tail || c) && !cflag && !qflag && !lflag && !Lflag) {
		if (c) {
			if ((Aflag || Bflag) && !first_global &&
			    (first || since_printed > Bflag))
				printf("--\n");
			tail = Aflag;
			if (Bflag > 0)
				printqueue();
			printline(l, ':', matches, m);
		} else {
			printline(l, '-', matches, m);
			tail--;
		}
		first = false;
		first_global = false;
		since_printed = 0;
	} else {
		if (Bflag)
			enqueue(l);
		since_printed++;
	}
	return (c);
}

/*
 * Safe malloc() for internal use.
 */
void *
grep_malloc(size_t size)
{
	void *ptr;

	if ((ptr = malloc(size)) == NULL)
		err(2, "malloc");
	return (ptr);
}

/*
 * Safe calloc() for internal use.
 */
void *
grep_calloc(size_t nmemb, size_t size)
{
	void *ptr;

	if ((ptr = calloc(nmemb, size)) == NULL)
		err(2, "calloc");
	return (ptr);
}

/*
 * Safe realloc() for internal use.
 */
void *
grep_realloc(void *ptr, size_t size)
{

	if ((ptr = realloc(ptr, size)) == NULL)
		err(2, "realloc");
	return (ptr);
}

/*
 * Safe strdup() for internal use.
 */
char *
grep_strdup(const char *str)
{
	char *ret;

	if ((ret = strdup(str)) == NULL)
		err(2, "strdup");
	return (ret);
}

/*
 * Prints a matching line according to the command line options.
 */
void
printline(struct str *line, int sep, regmatch_t *matches, int m)
{
	size_t a = 0;
	int i, n = 0;

	if (!hflag) {
		if (nullflag == 0)
			fputs(line->file, stdout);
		else {
			printf("%s", line->file);
			putchar(0);
		}
		++n;
	}
	if (nflag) {
		if (n > 0)
			putchar(sep);
		printf("%d", line->line_no);
		++n;
	}
	if (bflag) {
		if (n > 0)
			putchar(sep);
		printf("%lld", (long long)line->off);
		++n;
	}
	if (n)
		putchar(sep);
	/* --color and -o */
	if ((oflag || color) && m > 0) {
		for (i = 0; i < m; i++) {
			if (!oflag)
				fwrite(line->dat + a, matches[i].rm_so - a, 1,
				    stdout);
			if (color) 
				fprintf(stdout, "\33[%sm\33[K", color);

				fwrite(line->dat + matches[i].rm_so, 
				    matches[i].rm_eo - matches[i].rm_so, 1,
				    stdout);
			if (color) 
				fprintf(stdout, "\33[m\33[K");
			a = matches[i].rm_eo;
			if (oflag)
				putchar('\n');
		}
		if (!oflag) {
			if (line->len - a > 0)
				fwrite(line->dat + a, line->len - a, 1, stdout);
			putchar(line_sep);
		}
	} else {
		fwrite(line->dat, line->len, 1, stdout);
		putchar(line_sep);
	}
}
