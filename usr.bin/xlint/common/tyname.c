/*	$NetBSD: tyname.c,v 1.10 2008/09/26 23:51:04 matt Exp $	*/

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
__RCSID("$NetBSD: tyname.c,v 1.10 2008/09/26 23:51:04 matt Exp $");
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

const char *
tyname(char *buf, size_t bufsiz, type_t *tp)
{
	tspec_t	t;
	const	char *s;
	char lbuf[64];
	char cv[20];

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
