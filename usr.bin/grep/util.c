/*	$NetBSD: util.c,v 1.3 2006/01/13 20:30:40 wiz Exp $	*/

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coïdan Smørgrav
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
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: util.c,v 1.3 2006/01/13 20:30:40 wiz Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fts.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "grep.h"

/*
 * Process a file line by line...
 */

static int linesqueued, newfile;
static int procline(str_t *l, int nottext);

int 
grep_tree(char **argv)
{
	FTS *fts;
	FTSENT *p;
	int c, fts_flags;

	c = fts_flags = 0;

/* 	if (linkbehave == LINK_EXPLICIT)
		fts_flags = FTS_COMFOLLOW;
	if (linkbehave == LINK_SKIP)
		fts_flags = FTS_PHYSICAL;
	if (linkbehave == LINK_FOLLOW)
		fts_flags = FTS_LOGICAL;*/

	fts_flags |= FTS_NOSTAT | FTS_NOCHDIR | FTS_LOGICAL;

	if ((fts = fts_open(argv, fts_flags, NULL)) == NULL)
		err(2, NULL);
	while ((p = fts_read(fts)) != NULL) {
		switch (p->fts_info) {
		case FTS_DNR:
			break;
		case FTS_ERR:
			errx(2, "%s: %s", p->fts_path, strerror(p->fts_errno));
			break;
		case FTS_DP:
		case FTS_D:
			break;
		case FTS_DC:
			warnx("warning: %s: recursive directory loop",
				p->fts_path);
			break;
		default:
			c += procfile(p->fts_path);
			break;
		}
	}

	return c;
}

int
procfile(char *fn)
{
	str_t ln;
	file_t *f;
	struct stat sb;
	mode_t s;
	int c, t, z, nottext, skip;
	
	tail = 0;
	newfile = 1;

	if (fn == NULL) {
		fn = stdin_label;
		f = grep_fdopen(STDIN_FILENO, "r");
	} else {
		skip = 1;
		if (dirbehave == GREP_SKIP || devbehave == GREP_SKIP) {
			if (stat(fn, &sb)) {
				fprintf(stderr, "Cannot stat %s %d\n",
					fn, errno);
				/* XXX record error variable */
			} else {
				s = sb.st_mode & S_IFMT;
				if (s == S_IFDIR && dirbehave == GREP_SKIP)
					skip = 0;
				if (   (s == S_IFIFO || s == S_IFCHR ||
					s == S_IFBLK || s == S_IFSOCK)
					&& devbehave == GREP_SKIP)
							skip = 0;
			}
		}
		if (skip == 0)
			return 0;
		
		f = grep_open(fn, "r");
	}
	if (f == NULL) {
		if (!sflag)
			warn("%s", fn);
		return 0;
	}

	nottext = grep_bin_file(f);

	if (nottext && binbehave == BIN_FILE_SKIP) {
		/* Skip this file as it is binary */
		grep_close(f);
		return 0;
	}

	ln.file = fn;
	ln.line_no = 0;
	linesqueued = 0;
	ln.off = -1;

	if (Bflag > 0)
		initqueue();
	for (c = 0; !(lflag && c);) {
		ln.off += ln.len + 1;
		if ((ln.dat = grep_fgetln(f, &ln.len)) == NULL)
			break;
		if (ln.len > 0 && ln.dat[ln.len - 1] == line_endchar)
			--ln.len;
		ln.line_no++;

		z = tail;
		
		if ((t = procline(&ln, nottext)) == 0 && Bflag > 0 && z == 0) {
			enqueue(&ln);
			linesqueued++;
		}
		c += t;
		
		/* If we have a maximum number of matches, stop processing */
		if (mflag && c >= maxcount)
			break;
	}
	if (Bflag > 0)
		clearqueue();
	grep_close(f);

	if (cflag) {
		if (output_filenames)
			printf("%s%c", ln.file, fn_colonchar);
		printf("%u\n", c);
	} 
		
	if (lflag && c != 0)
		printf("%s%c", fn, fn_endchar);
	if (Lflag && c == 0)
		printf("%s%c", fn, fn_endchar);
	if (c && !cflag && !lflag && !Lflag && 
		binbehave == BIN_FILE_BIN && nottext && !qflag)
			printf("Binary file %s matches\n", fn); 
		
	return c;
}


/*
 * Process an individual line in a file. Return non-zero if it matches.
 */

#define isword(x) (isalnum((unsigned char)(x)) || (x) == '_')

static int
procline(str_t *l, int nottext)
{
	regmatch_t pmatch;
	regmatch_t matches[MAX_LINE_MATCHES];
	int c = 0, i, r, t, m = 0;
	regoff_t st = 0;

	if (matchall) {
		c = !vflag;
		goto print;
	}
	
	t = vflag ? REG_NOMATCH : 0;

	while (st <= l->len) {
		pmatch.rm_so = st;
		pmatch.rm_eo = l->len;
		for (i = 0; i < patterns; i++) {
			r = regexec(&r_pattern[i], l->dat, 1, &pmatch, eflags);
			if (r == REG_NOMATCH && t == 0)
				continue;
			if (r == 0) {
				if (wflag) {
					if ((pmatch.rm_so != 0 && isword(l->dat[pmatch.rm_so - 1]))
					    || (pmatch.rm_eo != l->len && isword(l->dat[pmatch.rm_eo])))
						r = REG_NOMATCH;
				}
				if (xflag) {
					if (pmatch.rm_so != 0 || pmatch.rm_eo != l->len)
						r = REG_NOMATCH;
				}
			}
			if (r == t) {
				if (m == 0)
					c++;
				if (m < MAX_LINE_MATCHES) {
					matches[m] = pmatch;
					m++;
				}
				st = pmatch.rm_eo;
				break;
			}
		}

		/* One pass if we are not recording matches */
		if (!oflag && !colours)
			break;

		if (st == pmatch.rm_so)
			break; 	/* No matches */
		
	}
	
print:

	if (c && binbehave == BIN_FILE_BIN && nottext) 
		return c;	/* Binary file */
	
	if ((tail > 0 || c) && !cflag && !qflag) {
		if (c) {
						
			if ( (Aflag || Bflag) && first > 0 && 
			   ( (Bflag <= linesqueued && tail == 0) || newfile) ) 
						printf("--\n");
									
			first = 1;
			newfile = 0;
			tail = Aflag;
			if (Bflag > 0)
				printqueue();
			linesqueued = 0;
			printline(l, fn_colonchar, matches, m);
		} else {
			printline(l, fn_dashchar, matches, m);
			tail--;
		}

	}
	return c;
}

void *
grep_malloc(size_t size)
{
	void *ptr;

	if ((ptr = malloc(size)) == NULL)
		err(2, "malloc");
	return ptr;
}

void *
grep_realloc(void *ptr, size_t size)
{
	if ((ptr = realloc(ptr, size)) == NULL)
		err(2, "realloc");
	return ptr;
}

void
printline(str_t *line, int sep, regmatch_t *matches, int m)
{
	int i, n = 0;
	size_t a = 0;

	if (output_filenames) {
		fputs(line->file, stdout);
		++n;
	}
	if (nflag) {
		if (n)
			putchar(sep);
		printf("%d", line->line_no);
		++n;
	}
	if (bflag) {
		if (n)
			putchar(sep);
		printf("%lu", (unsigned long)line->off);
	}
	if (n)
		putchar(sep);

	if ((oflag || colours) && m > 0) {

		for (i = 0; i < m; i++) {
			
			if (!oflag)
				fwrite(line->dat + a, matches[i].rm_so - a, 1, stdout);
			
			if (colours) 
				fprintf(stdout, "\33[%sm", grep_colour);
			fwrite(line->dat + matches[i].rm_so, 
				matches[i].rm_eo - matches[i].rm_so, 1, stdout);
			
			if (colours) 
				fprintf(stdout, "\33[00m");
			a = matches[i].rm_eo;
			if (oflag)
				putchar('\n');
		}
		if (!oflag) {
			if (line->len - a > 0)
				fwrite(line->dat + a, line->len - a, 1, stdout);
			putchar('\n');
		}
		
		
	} else {
		fwrite(line->dat, line->len, 1, stdout);
		putchar(line_endchar);
	}
			
}
