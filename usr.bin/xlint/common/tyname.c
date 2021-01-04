/*	$NetBSD: tyname.c,v 1.22 2021/01/04 22:26:50 rillig Exp $	*/

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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: tyname.c,v 1.22 2021/01/04 22:26:50 rillig Exp $");
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include PASS

#ifndef LERROR
#define LERROR(fmt, args...) \
	do { \
		(void)warnx("%s, %d: " fmt, __FILE__, __LINE__, ##args); \
		abort(); \
	} while (/*CONSTCOND*/0)
#endif

/* A tree of strings. */
typedef struct name_tree_node {
	char *ntn_name;
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

/* Return the canonical instance of the string, with unlimited life time. */
static const char *
intern(const char *name)
{
	name_tree_node *n = type_names;
	int cmp;

	if (n == NULL) {
		n = new_name_tree_node(name);
		type_names = n;
		return n->ntn_name;
	}

	while ((cmp = strcmp(name, n->ntn_name)) != 0) {
		if (cmp < 0) {
			if (n->ntn_less == NULL) {
				n->ntn_less = new_name_tree_node(name);
				return n->ntn_less->ntn_name;
			}
			n = n->ntn_less;
		} else {
			if (n->ntn_greater == NULL) {
				n->ntn_greater = new_name_tree_node(name);
				return n->ntn_greater->ntn_name;
			}
			n = n->ntn_greater;
		}
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

	snprintf(num, sizeof num, "%d", n);
	buf_add(buf, num);
}

const char *
tspec_name(tspec_t t)
{
	switch (t) {
	case SIGNED:	return "signed";
	case UNSIGN:	return "unsigned";
	case BOOL:	return "_Bool";
	case CHAR:	return "char";
	case SCHAR:	return "signed char";
	case UCHAR:	return "unsigned char";
	case SHORT:	return "short";
	case USHORT:	return "unsigned short";
	case INT:	return "int";
	case UINT:	return "unsigned int";
	case LONG:	return "long";
	case ULONG:	return "unsigned long";
	case QUAD:	return "long long";
	case UQUAD:	return "unsigned long long";
#ifdef INT128_SIZE
	case INT128:	return "__int128_t";
	case UINT128:	return "__uint128_t";
#endif
	case FLOAT:	return "float";
	case DOUBLE:	return "double";
	case LDOUBLE:	return "long double";
	case VOID:	return "void";
	case STRUCT:	return "struct";
	case UNION:	return "union";
	case ENUM:	return "enum";
	case PTR:	return "pointer";
	case ARRAY:	return "array";
	case FUNC:	return "function";
	case COMPLEX:	return "_Complex";
	case FCOMPLEX:	return "float _Complex";
	case DCOMPLEX:	return "double _Complex";
	case LCOMPLEX:	return "long double _Complex";
	default:
		LERROR("tspec_name(%d)", t);
		return NULL;
	}
}

int
sametype(const type_t *t1, const type_t *t2)
{
	tspec_t	t;

	if (t1->t_tspec != t2->t_tspec)
		return 0;

	/* Ignore const/void */

	switch (t = t1->t_tspec) {
	case BOOL:
	case CHAR:
	case UCHAR:
	case SCHAR:
	case SHORT:
	case USHORT:
	case INT:
	case UINT:
	case LONG:
	case ULONG:
	case QUAD:
	case UQUAD:
#ifdef INT128_SIZE
	case INT128:
	case UINT128:
#endif
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
	case VOID:
	case FUNC:
	case COMPLEX:
	case FCOMPLEX:
	case DCOMPLEX:
	case LCOMPLEX:
		return 1;
	case ARRAY:
		if (t1->t_dim != t2->t_dim)
			return 0;
		/*FALLTHROUGH*/
	case PTR:
		return sametype(t1->t_subt, t2->t_subt);
	case ENUM:
#ifdef t_enum
		return strcmp(t1->t_enum->etag->s_name,
		    t2->t_enum->etag->s_name) == 0;
#else
		return 1;
#endif
	case STRUCT:
	case UNION:
#ifdef t_str
		return strcmp(t1->t_str->stag->s_name,
		    t2->t_str->stag->s_name) == 0;
#else
		return 1;
#endif
	default:
		LERROR("tyname(%d)", t);
		return 0;
	}
}

static void
type_name_of_function(buffer *buf, const type_t *tp)
{
	const char *sep = "";

	buf_add(buf, "(");
	if (tp->t_proto) {
#ifdef t_enum /* lint1 */
		sym_t *arg;

		for (arg = tp->t_args; arg != NULL; arg = arg->s_next) {
			buf_add(buf, sep), sep = ", ";
			buf_add(buf, type_name(arg->s_type));
		}
#else /* lint2 */
		type_t **argtype;

		for (argtype = tp->t_args; *argtype != NULL; argtype++) {
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

const char *
type_name(const type_t *tp)
{
	tspec_t t;
	buffer buf;
	const char *name;

	if (tp == NULL)
		return "(null)";

	/*
	 * XXX: Why is this necessary, and in which cases does this apply?
	 * Shouldn't the type be an ENUM from the beginning?
	 */
	if ((t = tp->t_tspec) == INT && tp->t_isenum)
		t = ENUM;

	buf_init(&buf);
	if (tp->t_const)
		buf_add(&buf, "const ");
	if (tp->t_volatile)
		buf_add(&buf, "volatile ");
	buf_add(&buf, tspec_name(t));

	switch (t) {
	case BOOL:
	case CHAR:
	case UCHAR:
	case SCHAR:
	case SHORT:
	case USHORT:
	case INT:
	case UINT:
	case LONG:
	case ULONG:
	case QUAD:
	case UQUAD:
#ifdef INT128_SIZE
	case INT128:
	case UINT128:
#endif
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
	case VOID:
	case COMPLEX:
	case FCOMPLEX:
	case DCOMPLEX:
	case LCOMPLEX:
	case SIGNED:
	case UNSIGN:
		break;
	case PTR:
		buf_add(&buf, " to ");
		buf_add(&buf, type_name(tp->t_subt));
		break;
	case ENUM:
		buf_add(&buf, " ");
#ifdef t_enum
		buf_add(&buf, tp->t_enum->etag->s_name);
#else
		buf_add(&buf,
		    tp->t_isuniqpos ? "*anonymous*" : tp->t_tag->h_name);
#endif
		break;
	case STRUCT:
	case UNION:
		buf_add(&buf, " ");
#ifdef t_str
		buf_add(&buf, tp->t_str->stag->s_name);
#else
		buf_add(&buf,
		    tp->t_isuniqpos ? "*anonymous*" : tp->t_tag->h_name);
#endif
		break;
	case ARRAY:
		buf_add(&buf, " of ");
		buf_add(&buf, type_name(tp->t_subt));
		buf_add(&buf, "[");
		buf_add_int(&buf, tp->t_dim);
		buf_add(&buf, "]");
		break;
	case FUNC:
		type_name_of_function(&buf, tp);
		break;

	default:
		LERROR("type_name(%d)", t);
	}

	name = intern(buf.data);
	buf_done(&buf);
	return name;
}
