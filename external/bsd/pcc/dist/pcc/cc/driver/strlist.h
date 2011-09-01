/*	Id: strlist.h,v 1.2 2011/05/26 16:48:40 plunky Exp 	*/	
/*	$NetBSD: strlist.h,v 1.1.1.1 2011/09/01 12:47:05 plunky Exp $	*/

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

#ifndef STRLIST_H
#define STRLIST_H

#include <stddef.h>
#include <stdio.h>

struct string {
	struct string *next;
	char *value;
};

struct strlist {
	struct string *first;
	struct string *last;
};

void strlist_init(struct strlist *);
void strlist_free(struct strlist *);
void strlist_make_array(const struct strlist *, char ***, size_t *);
void strlist_print(const struct strlist *, FILE *);

void strlist_prepend(struct strlist *, const char *);
void strlist_prepend_nocopy(struct strlist *, char *);
void strlist_prepend_list(struct strlist *, const struct strlist *);
void strlist_append(struct strlist *, const char *);
void strlist_append_nocopy(struct strlist *, char *);
void strlist_append_list(struct strlist *, const struct strlist *);
void strlist_append_array(struct strlist *, const char * const *);

#define	STRLIST_FIRST(head)	((head)->first)
#define	STRLIST_NEXT(elem)	((elem)->next)
#define	STRLIST_FOREACH(var, head) \
	for ((var) = STRLIST_FIRST(head); \
	     (var) != NULL; \
	     (var) = STRLIST_NEXT(var))
#define	STRLIST_FOREACH_MUTABLE(var, head, var2) \
	for ((var) = STRLIST_FIRST(head); \
	     (var) != NULL && ((var2) = STRLIST_NEXT(var), 1); \
	     (var) = (var2))
#define	STRLIST_EMPTY(head)	(STRLIST_FIRST(head) == NULL)

#endif /* STRLIST_H */
