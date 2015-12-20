/*	$NetBSD: sortinfo.c,v 1.4 2015/12/20 00:48:36 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: sortinfo.c,v 1.4 2015/12/20 00:48:36 christos Exp $");

/*
 * Sort a texinfo(1) directory file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <util.h>

struct section {
	const char *name;
	char **lines;
	size_t nlines;
	size_t maxlines;
};

static struct section *slist;
static size_t nsections, maxsections;

static struct section *
addsection(const char *line)
{
	if (nsections >= maxsections) {
		maxsections += 20;
		slist = erealloc(slist, maxsections * sizeof(*slist));
	}
	slist[nsections].name = estrdup(line);
	slist[nsections].nlines = 0;
	slist[nsections].maxlines = 20;
	slist[nsections].lines = ecalloc(slist[nsections].maxlines,
	    sizeof(*slist[nsections].lines));
	return &slist[nsections++];
}

static void
addline(struct section *s, const char *line)
{
	if (s->nlines == s->maxlines) {
		s->maxlines += 20;
		s->lines = erealloc(s->lines, s->maxlines * sizeof(*s->lines));
	}
	s->lines[s->nlines++] = estrdup(line);
}

static int
compsection(const void *a, const void *b)
{
	const struct section *sa = a, *sb = b;
	return strcmp(sa->name, sb->name);
}

static int
strptrcmp(const void *a, const void *b)
{
	const char *sa = *(const char * const *)a;
	const char *sb = *(const char * const *)b;
	return strcmp(sa, sb);
}

static void
printsection(const struct section *s)
{
	size_t i;

	fputc('\n', stdout);
	printf("%s", s->name);
	for (i = 0; i < s->nlines; i++)
		printf("%s", s->lines[i]);
}
	
int
main(int argc, char *argv[])
{
	size_t i;
	char *line;
	int needsection;
	struct section *s = NULL;

	while ((line = fgetln(stdin, &i)) != NULL) {
		fputs(line, stdout);
		if (strcmp(line, "* Menu:\n") == 0)
			break;
	}

	if (line == NULL)
		errx(EXIT_FAILURE, "Did not find menu line");

	needsection = 0;
	while ((line = fgetln(stdin, &i)) != NULL)
		switch (*line) {
		case '\n':
			needsection = 1;
			continue;
		case '*':
			if (s == NULL)
				errx(EXIT_FAILURE, "No current section");
			addline(s, line);
			continue;
		default:
			if (needsection == 0)
				errx(EXIT_FAILURE, "Already in section");
			s = addsection(line);
			needsection = 0;
			continue;
		}

	qsort(slist, nsections, sizeof(*slist), compsection);
	for (i = 0; i < nsections; i++) {
		s = &slist[i];
		qsort(s->lines, s->nlines, sizeof(*s->lines), strptrcmp);
		printsection(&slist[i]);
	}

	return EXIT_SUCCESS;
}
