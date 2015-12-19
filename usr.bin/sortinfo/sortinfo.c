/*	$NetBSD: sortinfo.c,v 1.2 2015/12/19 17:30:00 joerg Exp $	*/

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
__RCSID("$NetBSD: sortinfo.c,v 1.2 2015/12/19 17:30:00 joerg Exp $");

/*
 * Sort a texinfo(1) directory file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <util.h>

struct line {
	char *data;
	struct line *next;
};

struct section {
	const char *name;
	struct line *line;
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
	slist[nsections].line = NULL;
	return &slist[nsections++];
}

static struct line *
addline(struct section *s, struct line *l, const char *line)
{
	if (l == NULL)
		s->line = l = emalloc(sizeof(*l));
	else {
		l->next = emalloc(sizeof(*l));
		l = l->next;
	}
	l->next = NULL;
	l->data = estrdup(line);
	return l;
}

static int
compsection(const void *a, const void *b)
{
	const struct section *sa = a, *sb = b;
	return strcmp(sa->name, sb->name);
}

static void
printsection(const struct section *s)
{
	struct line *l;
	fputc('\n', stdout);
	printf("%s", s->name);
	for (l = s->line; l; l = l->next)
		printf("%s", l->data);
}
	
int
main(int argc, char *argv[])
{
	size_t i;
	char *line;
	int needsection;
	struct section *s = NULL;
	struct line *l = NULL;

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
			l = addline(s, l, line);
			continue;
		default:
			if (needsection == 0)
				errx(EXIT_FAILURE, "Already in section");
			s = addsection(line);
			l = NULL;
			needsection = 0;
			continue;
		}

	qsort(slist, nsections, sizeof(*slist), compsection);
	for (i = 0; i < nsections; i++)
		printsection(&slist[i]);

	return EXIT_SUCCESS;
}
