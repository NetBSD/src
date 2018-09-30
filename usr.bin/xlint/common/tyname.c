/*	$NetBSD: tyname.c,v 1.12.12.1 2018/09/30 01:45:59 pgoyette Exp $	*/

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
__RCSID("$NetBSD: tyname.c,v 1.12.12.1 2018/09/30 01:45:59 pgoyette Exp $");
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include PASS

#ifndef LERROR
#define LERROR(fmt, args...) 	do { \
    (void)warnx("%s, %d: " fmt, __FILE__, __LINE__, ##args); \
    abort(); \
} while (/*CONSTCOND*/0)
#endif

const char *
basictyname(tspec_t t)
{
	switch (t) {
	case BOOL:	return "_Bool";
	case CHAR:	return "char";
	case UCHAR:	return "unsigned char";
	case SCHAR:	return "signed char";
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
	case PTR:	return "pointer";
	case ENUM:	return "enum";
	case STRUCT:	return "struct";
	case UNION:	return "union";
	case FUNC:	return "function";
	case ARRAY:	return "array";
	case FCOMPLEX:	return "float _Complex";
	case DCOMPLEX:	return "double _Complex";
	case LCOMPLEX:	return "long double _Complex";
	case COMPLEX:	return "_Complex";
	default:
		LERROR("basictyname(%d)", t);
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

const char *
tyname(char *buf, size_t bufsiz, const type_t *tp)
{
	tspec_t	t;
	const	char *s;
	char lbuf[64];
	char cv[20];

	if (tp == NULL)
		return "(null)";
	if ((t = tp->t_tspec) == INT && tp->t_isenum)
		t = ENUM;

	s = basictyname(t);

	cv[0] = '\0';
	if (tp->t_const)
		(void)strcat(cv, "const ");
	if (tp->t_volatile)
		(void)strcat(cv, "volatile ");

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
	case FUNC:
	case COMPLEX:
	case FCOMPLEX:
	case DCOMPLEX:
	case LCOMPLEX:
		(void)snprintf(buf, bufsiz, "%s%s", cv, s);
		break;
	case PTR:
		(void)snprintf(buf, bufsiz, "%s%s to %s", cv, s,
		    tyname(lbuf, sizeof(lbuf), tp->t_subt));
		break;
	case ENUM:
		(void)snprintf(buf, bufsiz, "%s%s %s", cv, s,
#ifdef t_enum
		    tp->t_enum->etag->s_name
#else
		    tp->t_isuniqpos ? "*anonymous*" : tp->t_tag->h_name
#endif
		    );
		break;
	case STRUCT:
	case UNION:
		(void)snprintf(buf, bufsiz, "%s%s %s", cv, s,
#ifdef t_str
		    tp->t_str->stag->s_name
#else
		    tp->t_isuniqpos ? "*anonymous*" : tp->t_tag->h_name
#endif
		    );
		break;
	case ARRAY:
		(void)snprintf(buf, bufsiz, "%s%s of %s[%d]", cv, s,
		    tyname(lbuf, sizeof(lbuf), tp->t_subt), tp->t_dim);
		break;
	default:
		LERROR("tyname(%d)", t);
	}
	return (buf);
}
