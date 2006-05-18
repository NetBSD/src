/*	$NetBSD: prop_string.c,v 1.2 2006/05/18 03:05:19 thorpej Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <prop/prop_string.h>
#include "prop_object_impl.h"

struct _prop_string {
	struct _prop_object	ps_obj;
	union {
		char *		psu_mutable;
		const char *	psu_immutable;
	} ps_un;
#define	ps_mutable		ps_un.psu_mutable
#define	ps_immutable		ps_un.psu_immutable
	size_t			ps_size;	/* not including \0 */
	int			ps_flags;
};

#define	PS_F_NOCOPY		0x01

_PROP_POOL_INIT(_prop_string_pool, sizeof(struct _prop_string), "propstng")

_PROP_MALLOC_DEFINE(M_PROP_STRING, "prop string",
		    "property string container object")

static void		_prop_string_free(void *);
static boolean_t	_prop_string_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_string_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_string = {
	.pot_type	=	PROP_TYPE_STRING,
	.pot_free	=	_prop_string_free,
	.pot_extern	=	_prop_string_externalize,
	.pot_equals	=	_prop_string_equals,
};

#define	prop_object_is_string(x)	\
	((x)->ps_obj.po_type == &_prop_object_type_string)
#define	prop_string_contents(x)  ((x)->ps_immutable ? (x)->ps_immutable : "")

static void
_prop_string_free(void *v)
{
	prop_string_t ps = v;

	if ((ps->ps_flags & PS_F_NOCOPY) == 0 && ps->ps_mutable != NULL)
	    	_PROP_FREE(ps->ps_mutable, M_PROP_STRING);
	_PROP_POOL_PUT(_prop_string_pool, v);
}

static boolean_t
_prop_string_externalize(struct _prop_object_externalize_context *ctx,
			 void *v)
{
	prop_string_t ps = v;

	if (ps->ps_size == 0)
		return (_prop_object_externalize_empty_tag(ctx, "string"));

	if (_prop_object_externalize_start_tag(ctx, "string") == FALSE ||
	    _prop_object_externalize_append_encoded_cstring(ctx,
	    					ps->ps_immutable) == FALSE ||
	    _prop_object_externalize_end_tag(ctx, "string") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static boolean_t
_prop_string_equals(void *v1, void *v2)
{
	prop_string_t str1 = v1;
	prop_string_t str2 = v2;

	_PROP_ASSERT(prop_object_is_string(str1));
	_PROP_ASSERT(prop_object_is_string(str2));
	if (str1 == str2)
		return (TRUE);
	if (str1->ps_size != str2->ps_size)
		return (FALSE);
	return (strcmp(prop_string_contents(str1),
		       prop_string_contents(str2)) == 0);
}

static prop_string_t
_prop_string_alloc(void)
{
	prop_string_t ps;

	ps = _PROP_POOL_GET(_prop_string_pool);
	if (ps != NULL) {
		_prop_object_init(&ps->ps_obj, &_prop_object_type_string);

		ps->ps_mutable = NULL;
		ps->ps_size = 0;
		ps->ps_flags = 0;
	}

	return (ps);
}

/*
 * prop_string_create --
 *	Create an empty mutable string.
 */
prop_string_t
prop_string_create(void)
{

	return (_prop_string_alloc());
}

/*
 * prop_string_create_cstring --
 *	Create a string that contains a copy of the provided C string.
 */
prop_string_t
prop_string_create_cstring(const char *str)
{
	prop_string_t ps;
	char *cp;
	size_t len;

	ps = _prop_string_alloc();
	if (ps != NULL) {
		len = strlen(str);
		cp = _PROP_MALLOC(len + 1, M_PROP_STRING);
		if (cp == NULL) {
			_prop_string_free(ps);
			return (NULL);
		}
		strcpy(cp, str);
		ps->ps_mutable = cp;
		ps->ps_size = len;
	}
	return (ps);
}

/*
 * prop_string_create_cstring_nocopy --
 *	Create an immutable string that contains a refrence to the
 *	provided C string.
 */
prop_string_t
prop_string_create_cstring_nocopy(const char *str)
{
	prop_string_t ps;
	
	ps = _prop_string_alloc();
	if (ps != NULL) {
		ps->ps_immutable = str;
		ps->ps_size = strlen(str);
		ps->ps_flags |= PS_F_NOCOPY;
	}
	return (ps);
}

/*
 * prop_string_copy --
 *	Copy a string.  If the original string is immutable, then the
 *	copy is also immutable and references the same external data.
 */
prop_string_t
prop_string_copy(prop_string_t ops)
{
	prop_string_t ps;

	_PROP_ASSERT(prop_object_is_string(ops));

	ps = _prop_string_alloc();
	if (ps != NULL) {
		ps->ps_size = ops->ps_size;
		ps->ps_flags = ops->ps_flags;
		if (ops->ps_flags & PS_F_NOCOPY)
			ps->ps_immutable = ops->ps_immutable;
		else {
			char *cp = _PROP_MALLOC(ps->ps_size + 1, M_PROP_STRING);
			if (cp == NULL) {
				_prop_string_free(ps);
				return (NULL);
			}
			strcpy(cp, prop_string_contents(ops));
			ps->ps_mutable = cp;
		}
	}
	return (ps);
}

/*
 * prop_string_copy_mutable --
 *	Copy a string, always returning a mutable copy.
 */
prop_string_t
prop_string_copy_mutable(prop_string_t ops)
{
	prop_string_t ps;
	char *cp;

	_PROP_ASSERT(prop_object_is_string(ops));

	ps = _prop_string_alloc();
	if (ps != NULL) {
		ps->ps_size = ops->ps_size;
		cp = _PROP_MALLOC(ps->ps_size + 1, M_PROP_STRING);
		if (cp == NULL) {
			_prop_string_free(ps);
			return (NULL);
		}
		strcpy(cp, prop_string_contents(ops));
		ps->ps_mutable = cp;
	}
	return (ps);
}

/*
 * prop_string_size --
 *	Return the size of the string, no including the terminating NUL.
 */
size_t
prop_string_size(prop_string_t ps)
{

	_PROP_ASSERT(prop_object_is_string(ps));
	return (ps->ps_size);
}

/*
 * prop_string_mutable --
 *	Return TRUE if the string is a mutable string.
 */
boolean_t
prop_string_mutable(prop_string_t ps)
{

	_PROP_ASSERT(prop_object_is_string(ps));
	return ((ps->ps_flags & PS_F_NOCOPY) == 0);
}

/*
 * prop_string_cstring --
 *	Return a copy of the contents of the string as a C string.
 *	The string is allocated with the M_TEMP malloc type.
 */
char *
prop_string_cstring(prop_string_t ps)
{
	char *cp;

	_PROP_ASSERT(prop_object_is_string(ps));
	cp = _PROP_MALLOC(ps->ps_size + 1, M_TEMP);
	if (cp != NULL)
		strcpy(cp, prop_string_contents(ps));
	
	return (cp);
}

/*
 * prop_string_cstring_nocopy --
 *	Return an immutable reference to the contents of the string
 *	as a C string.
 */
const char *
prop_string_cstring_nocopy(prop_string_t ps)
{

	_PROP_ASSERT(prop_object_is_string(ps));
	return (prop_string_contents(ps));
}

/*
 * prop_string_append --
 *	Append the contents of one string to another.  Returns TRUE
 *	upon success.  The destination string must be mutable.
 */
boolean_t
prop_string_append(prop_string_t dst, prop_string_t src)
{
	char *ocp, *cp;
	size_t len;

	_PROP_ASSERT(prop_object_is_string(dst));
	_PROP_ASSERT(prop_object_is_string(src));

	if (dst->ps_flags & PS_F_NOCOPY)
		return (FALSE);

	len = dst->ps_size + src->ps_size;
	cp = _PROP_MALLOC(len + 1, M_PROP_STRING);
	if (cp == NULL)
		return (FALSE);
	sprintf(cp, "%s%s", prop_string_contents(dst),
		prop_string_contents(src));
	ocp = dst->ps_mutable;
	dst->ps_mutable = cp;
	dst->ps_size = len;
	if (ocp != NULL)
		_PROP_FREE(ocp, M_PROP_STRING);
	
	return (TRUE);
}

/*
 * prop_string_append_cstring --
 *	Append a C string to a string.  Returns TRUE upon success.
 *	The destination string must be mutable.
 */
boolean_t
prop_string_append_cstring(prop_string_t dst, const char *src)
{
	char *ocp, *cp;
	size_t len;

	_PROP_ASSERT(src != NULL);
	_PROP_ASSERT(prop_object_is_string(dst));

	if (dst->ps_flags & PS_F_NOCOPY)
		return (FALSE);
	
	len = dst->ps_size + strlen(src);
	cp = _PROP_MALLOC(len + 1, M_PROP_STRING);
	if (cp == NULL)
		return (FALSE);
	sprintf(cp, "%s%s", prop_string_contents(dst), src);
	ocp = dst->ps_mutable;
	dst->ps_mutable = cp;
	dst->ps_size = len;
	if (ocp != NULL)
		_PROP_FREE(ocp, M_PROP_STRING);
	
	return (TRUE);
}

/*
 * prop_string_equals --
 *	Return TRUE if two strings are equivalent.
 */
boolean_t
prop_string_equals(prop_string_t str1, prop_string_t str2)
{

	return (_prop_string_equals(str1, str2));
}

/*
 * prop_string_equals_cstring --
 *	Return TRUE if the string is equivalent to the specified
 *	C string.
 */
boolean_t
prop_string_equals_cstring(prop_string_t ps, const char *cp)
{

	_PROP_ASSERT(prop_object_is_string(ps));
	return (strcmp(prop_string_contents(ps), cp) == 0);
}

/*
 * _prop_string_internalize --
 *	Parse a <string>...</string> and return the object created from the
 *	external representation.
 */
prop_object_t
_prop_string_internalize(struct _prop_object_internalize_context *ctx)
{
	prop_string_t string;
	char *str;
	size_t len, alen;

	if (ctx->poic_is_empty_element)
		return (prop_string_create());
	
	/* No attributes recognized here. */
	if (ctx->poic_tagattr != NULL)
		return (NULL);

	/* Compute the length of the result. */
	if (_prop_object_internalize_decode_string(ctx, NULL, 0, &len,
						   NULL) == FALSE)
		return (NULL);
	
	str = _PROP_MALLOC(len + 1, M_PROP_STRING);
	if (str == NULL)
		return (NULL);
	
	if (_prop_object_internalize_decode_string(ctx, str, len, &alen,
						   &ctx->poic_cp) == FALSE ||
	    alen != len) {
		_PROP_FREE(str, M_PROP_STRING);
		return (NULL);
	}
	str[len] = '\0';

	if (_prop_object_internalize_find_tag(ctx, "string",
					      _PROP_TAG_TYPE_END) == FALSE) {
		_PROP_FREE(str, M_PROP_STRING);
		return (NULL);
	}

	string = _prop_string_alloc();
	if (string == NULL) {
		_PROP_FREE(str, M_PROP_STRING);
		return (NULL);
	}

	string->ps_mutable = str;
	string->ps_size = len;

	return (string);
}
