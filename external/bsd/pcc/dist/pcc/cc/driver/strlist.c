/*	Id: strlist.c,v 1.2 2011/05/26 16:48:40 plunky Exp 	*/	
/*	$NetBSD: strlist.c,v 1.1.1.1 2011/09/01 12:47:05 plunky Exp $	*/

/*-
 * Copyright (c) 2011 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include "strlist.h"
#include "xalloc.h"

void
strlist_init(struct strlist *l)
{
	l->first = l->last = NULL;
}

void
strlist_free(struct strlist *l)
{
	struct string *s1, *s2;

	STRLIST_FOREACH_MUTABLE(s1, l, s2) {
		free(s1->value);
		free(s1);
	}
	l->first = l->last = NULL;
}

void
strlist_make_array(const struct strlist *l, char ***a, size_t *len)
{
	const struct string *s;
	char **i;

	*len = 0;

	STRLIST_FOREACH(s, l)
		++*len;

	*a = xcalloc(*len + 1, sizeof(*i));
	i = *a;

	STRLIST_FOREACH(s, l)
		*i++ = xstrdup(s->value);
	*i = NULL;
}

void
strlist_print(const struct strlist *l, FILE *f)
{
	const struct string *s;
	int first = 1;

	STRLIST_FOREACH(s, l) {
		if (!first)
			putc(' ', f);
		fputs(s->value, f);
		first = 0;
	}
}

void
strlist_append_nocopy(struct strlist *l, char *val)
{
	struct string *s;

	s = xmalloc(sizeof(*s));
	s->next = NULL;
	s->value = val;
	if (l->last != NULL) {
		l->last->next = s;
		l->last = s;
	} else {
		l->last = s;
		l->first = s;
	}
}

void
strlist_append(struct strlist *l, const char *val)
{
	strlist_append_nocopy(l, xstrdup(val));
}

void
strlist_append_list(struct strlist *l, const struct strlist *l2)
{
	struct string *s;

	STRLIST_FOREACH(s, l2)
		strlist_append(l, s->value);
}

void
strlist_append_array(struct strlist *l, const char * const *strings)
{
	for (; *strings != NULL; ++strings)
		strlist_append(l, *strings);
}

void
strlist_prepend_nocopy(struct strlist *l, char *val)
{
	struct string *s;

	s = xmalloc(sizeof(*s));
	s->next = l->first;
	s->value = val;
	l->first = s;
	if (l->last == NULL) {
		l->last = s;
	}
}

void
strlist_prepend(struct strlist *l, const char *val)
{
	strlist_prepend_nocopy(l, xstrdup(val));
}

void
strlist_prepend_list(struct strlist *l, const struct strlist *l2)
{
	struct string *s, *s2, *s3, *s4;

	if (STRLIST_EMPTY(l2))
		return;

	if (STRLIST_EMPTY(l)) {
		strlist_append_list(l, l2);
		return;
	}

	s2 = NULL;
	s4 = l->first;
	STRLIST_FOREACH(s, l2) {
		s3 = xmalloc(sizeof(*s3));
		s3->value = xstrdup(s->value);
		s3->next = s4;
		if (s2 == NULL)
			l->first = s3;
		else
			s2->next = s3;
	}
}
