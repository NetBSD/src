/*	Id	*/	
/*	$NetBSD: list.c,v 1.1.1.1 2016/02/09 20:29:12 plunky Exp $	*/

/*-
 * Copyright (c) 2014 Iain Hibbert.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "driver.h"

/*
 * A list is an opaque collection of strings. We can add strings, another
 * list or an array of strings to the list. We can get a pointer to the
 * array of strings in the list and the number of strings in it, and we
 * can free the entire list when we are done.
 */

#define COUNT	64		/* element count increments */
#define BLOCK	4096		/* buffer size increments */

struct list {
	const char **	array;
	size_t		arraylen;
	size_t		arrayused;
	void *		buf;
	size_t		buflen;
	size_t		bufused;
};

/*
 * return array pointer
 */
const char **
list_array(const struct list *l)
{

	return l->array;
}

/*
 * return array count
 */
size_t
list_count(const struct list *l)
{

	return l->arrayused;
}

/*
 * allocate a new list header
 */
struct list *
list_alloc(void)
{
	struct list *l;

	l = xmalloc(sizeof(struct list));
	l->array = xmalloc(COUNT * sizeof(char *));
	l->array[0] = NULL;
	l->arraylen = COUNT;
	l->arrayused = 0;
	l->buf = NULL;
	l->buflen = 0;
	l->bufused = 0;

	return l;
}

/*
 * release a list header and associated storage
 */
void
list_free(struct list *l)
{

	free(l->array);
	free(l->buf);
	free(l);
}

/*
 * print out a list
 */
void
list_print(const struct list *l)
{
	size_t i;

	for (i = 0; i < l->arrayused; i++)
		fprintf(stderr, "%s%s\n", (i == 0 ? "" : " "), l->array[i]);
}

/*
 * add const string to list
 */
void
list_add_nocopy(struct list *l, const char *str)
{

	l->array[l->arrayused++] = str;

	if (l->arrayused == l->arraylen) {
		l->arraylen += COUNT;
		l->array = xrealloc(l->array, l->arraylen * sizeof(char *));
	}

	l->array[l->arrayused] = NULL;
}

/*
 * add formatted string to list, storing in list buffer
 */
void
list_add(struct list *l, const char *str, ...)
{
	va_list ap;
	void *p;
	size_t s;
	int n;

	for (;;) {
		p = l->buf + l->bufused;
		s = l->buflen - l->bufused;

		va_start(ap, str);
		n = vsnprintf(p, s, str, ap);
		va_end(ap);

		if (n < 0)
			error("vsnprintf");

		if ((unsigned int)n < s)
			break;

		l->buflen += BLOCK;
		l->buf = xrealloc(l->buf, l->buflen);
	}

	list_add_nocopy(l, p);
	l->bufused += n;
}

/*
 * add a NULL terminated array of const data to list
 */
void
list_add_array(struct list *l, const char **a)
{

	while (*a)
		list_add_nocopy(l, *a++);
}

/*
 * add another list to list (with copy)
 */
void
list_add_list(struct list *l1, const struct list *l2)
{
	size_t i;

	for (i = 0; i < l2->arrayused; i++)
		list_add(l1, l2->array[i]);
}
