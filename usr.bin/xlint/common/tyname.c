/*	$NetBSD: tyname.c,v 1.51 2022/05/20 21:18:54 rillig Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
#if defined(__RCSID)
__RCSID("$NetBSD: tyname.c,v 1.51 2022/05/20 21:18:54 rillig Exp $");
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#if defined(IS_LINT1)
#include "lint1.h"
#else
#include "lint2.h"
#endif

#ifndef INTERNAL_ERROR
#define INTERNAL_ERROR(fmt, args...) \
	do { \
		(void)warnx("%s, %d: " fmt, __FILE__, __LINE__, ##args); \
		abort(); \
	} while (false)
#endif

/* A tree of strings. */
typedef struct name_tree_node {
	const char *ntn_name;
	struct name_tree_node *ntn_less;
	struct name_tree_node *ntn_greater;
} name_tree_node;

/* A growable string buffer. */
typedef struct buffer {
	size_t	len;
	size_t	cap;
	char *	data;
} buffer;

static name_tree_node *type_names;

static name_tree_node *
new_name_tree_node(const char *name)
{
	name_tree_node *n;

	n = xmalloc(sizeof(*n));
	n->ntn_name = xstrdup(name);
	n->ntn_less = NULL;
	n->ntn_greater = NULL;
	return n;
}

/* Return the canonical instance of the string, with unlimited lifetime. */
static const char *
intern(const char *name)
{
	name_tree_node *n = type_names, **next;
	int cmp;

	if (n == NULL) {
		n = new_name_tree_node(name);
		type_names = n;
		return n->ntn_name;
	}

	while ((cmp = strcmp(name, n->ntn_name)) != 0) {
		next = cmp < 0 ? &n->ntn_less : &n->ntn_greater;
		if (*next == NULL) {
			*next = new_name_tree_node(name);
			return (*next)->ntn_name;
		}
		n = *next;
	}
	return n->ntn_name;
}

static void
buf_init(buffer *buf)
{
	buf->len = 0;
	buf->cap = 128;
	buf->data = xmalloc(buf->cap);
	buf->data[0] = '\0';
}

static void
buf_done(buffer *buf)
{
	free(buf->data);
}

static void
buf_add(buffer *buf, const char *s)
{
	size_t len = strlen(s);

	while (buf->len + len + 1 >= buf->cap) {
		buf->data = xrealloc(buf->data, 2 * buf->cap);
		buf->cap = 2 * buf->cap;
	}

	memcpy(buf->data + buf->len, s, len + 1);
	buf->len += len;
}

static void
buf_add_int(buffer *buf, int n)
{
	char num[1 + sizeof(n) * CHAR_BIT + 1];

	(void)snprintf(num, sizeof(num), "%d", n);
	buf_add(buf, num);
}

const char *
tspec_name(tspec_t t)
{
	const char *name = ttab[t].tt_name;
	if (name == NULL)
		INTERNAL_ERROR("tspec_name(%d)", t);
	return name;
}

static void
type_name_of_function(buffer *buf, const type_t *tp)
{
	const char *sep = "";

	buf_add(buf, "(");
	if (tp->t_proto) {
#ifdef IS_LINT1
		sym_t *arg;

		arg = tp->t_args;
		if (arg == NULL)
			buf_add(buf, "void");
		for (; arg != NULL; arg = arg->s_next) {
			buf_add(buf, sep), sep = ", ";
			buf_add(buf, type_name(arg->s_type));
		}
#else
		type_t **argtype;

		argtype = tp->t_args;
		if (argtype == NULL)
			buf_add(buf, "void");
		for (; *argtype != NULL; argtype++) {
			buf_add(buf, sep), sep = ", ";
			buf_add(buf, type_name(*argtype));
		}
#endif
	}
	if (tp->t_vararg) {
		buf_add(buf, sep);
		buf_add(buf, "...");
	}
	buf_add(buf, ") returning ");
	buf_add(buf, type_name(tp->t_subt));
}

static void
type_name_of_struct_or_union(buffer *buf, const type_t *tp)
{
	buf_add(buf, " ");
#ifdef IS_LINT1
	if (tp->t_str->sou_tag->s_name == unnamed &&
	    tp->t_str->sou_first_typedef != NULL) {
		buf_add(buf, "typedef ");
		buf_add(buf, tp->t_str->sou_first_typedef->s_name);
	} else {
		buf_add(buf, tp->t_str->sou_tag->s_name);
	}
#else
	buf_add(buf, tp->t_isuniqpos ? "*anonymous*" : tp->t_tag->h_name);
#endif
}

static void
type_name_of_enum(buffer *buf, const type_t *tp)
{
	buf_add(buf, " ");
#ifdef IS_LINT1
	if (tp->t_enum->en_tag->s_name == unnamed &&
	    tp->t_enum->en_first_typedef != NULL) {
		buf_add(buf, "typedef ");
		buf_add(buf, tp->t_enum->en_first_typedef->s_name);
	} else {
		buf_add(buf, tp->t_enum->en_tag->s_name);
	}
#else
	buf_add(buf, tp->t_isuniqpos ? "*anonymous*" : tp->t_tag->h_name);
#endif
}

static void
type_name_of_array(buffer *buf, const type_t *tp)
{
	buf_add(buf, "[");
#ifdef IS_LINT1
	if (tp->t_incomplete_array)
		buf_add(buf, "unknown_size");
	else
		buf_add_int(buf, tp->t_dim);
#else
	buf_add_int(buf, tp->t_dim);
#endif
	buf_add(buf, "]");
	buf_add(buf, " of ");
	buf_add(buf, type_name(tp->t_subt));
}

const char *
type_name(const type_t *tp)
{
	tspec_t t;
	buffer buf;
	const char *name;

	if (tp == NULL)
		return "(null)";

	if ((t = tp->t_tspec) == INT && tp->t_is_enum)
		t = ENUM;

	buf_init(&buf);
	if (tp->t_const)
		buf_add(&buf, "const ");
	if (tp->t_volatile)
		buf_add(&buf, "volatile ");

#ifdef IS_LINT1
	if ((t == STRUCT || t == UNION) && tp->t_str->sou_incomplete)
		buf_add(&buf, "incomplete ");
#endif
	buf_add(&buf, tspec_name(t));

	switch (t) {
	case PTR:
		buf_add(&buf, " to ");
		buf_add(&buf, type_name(tp->t_subt));
		break;
	case ENUM:
		type_name_of_enum(&buf, tp);
		break;
	case STRUCT:
	case UNION:
		type_name_of_struct_or_union(&buf, tp);
		break;
	case ARRAY:
		type_name_of_array(&buf, tp);
		break;
	case FUNC:
		type_name_of_function(&buf, tp);
		break;
	default:
		break;
	}

	name = intern(buf.data);
	buf_done(&buf);
	return name;
}
