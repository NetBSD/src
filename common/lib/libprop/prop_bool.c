/*	$NetBSD: prop_bool.c,v 1.9 2006/10/16 03:21:07 thorpej Exp $	*/

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

#include <prop/prop_bool.h>
#include "prop_object_impl.h"

struct _prop_bool {
	struct _prop_object	pb_obj;
	boolean_t		pb_value;
};

static struct _prop_bool _prop_bool_true;
static struct _prop_bool _prop_bool_false;

_PROP_MUTEX_DECL_STATIC(_prop_bool_initialized_mutex)
static boolean_t	_prop_bool_initialized;

static void		_prop_bool_free(void *);
static boolean_t	_prop_bool_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_bool_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_bool = {
	.pot_type	=	PROP_TYPE_BOOL,
	.pot_free	=	_prop_bool_free,
	.pot_extern	=	_prop_bool_externalize,
	.pot_equals	=	_prop_bool_equals,
};

#define	prop_object_is_bool(x)		\
	((x) != NULL && (x)->pb_obj.po_type == &_prop_object_type_bool)

static void
/*ARGSUSED*/
_prop_bool_free(void *v _PROP_ARG_UNUSED)
{

	/*
	 * This should never happen as we "leak" our initial reference
	 * count.
	 */
	/* XXX forced assertion failure? */
}

static boolean_t
_prop_bool_externalize(struct _prop_object_externalize_context *ctx,
		       void *v)
{
	prop_bool_t pb = v;

	return (_prop_object_externalize_empty_tag(ctx,
	    pb->pb_value ? "true" : "false"));
}

static boolean_t
_prop_bool_equals(void *v1, void *v2)
{
	prop_bool_t b1 = v1;
	prop_bool_t b2 = v2;

	if (! (prop_object_is_bool(b1) &&
	       prop_object_is_bool(b2)))
		return (FALSE);

	/*
	 * Since we only ever allocate one true and one false,
	 * save ourselves a couple of memory operations.
	 */
	return (b1 == b2);
}

static prop_bool_t
_prop_bool_alloc(boolean_t val)
{
	prop_bool_t pb;

	if (! _prop_bool_initialized) {
		_PROP_MUTEX_LOCK(_prop_bool_initialized_mutex);
		if (! _prop_bool_initialized) {
			_prop_object_init(&_prop_bool_true.pb_obj,
					  &_prop_object_type_bool);
			_prop_bool_true.pb_value = TRUE;

			_prop_object_init(&_prop_bool_false.pb_obj,
					  &_prop_object_type_bool);
			_prop_bool_false.pb_value = FALSE;

			_prop_bool_initialized = TRUE;
		}
		_PROP_MUTEX_UNLOCK(_prop_bool_initialized_mutex);
	}

	pb = val ? &_prop_bool_true : &_prop_bool_false;
	prop_object_retain(pb);

	return (pb);
}

/*
 * prop_bool_create --
 *	Create a prop_bool_t and initialize it with the
 *	provided boolean value.
 */
prop_bool_t
prop_bool_create(boolean_t val)
{

	return (_prop_bool_alloc(val));
}

/*
 * prop_bool_copy --
 *	Copy a prop_bool_t.
 */
prop_bool_t
prop_bool_copy(prop_bool_t opb)
{

	if (! prop_object_is_bool(opb))
		return (NULL);

	/*
	 * Because we only ever allocate one true and one false, this
	 * can be reduced to a simple retain operation.
	 */
	prop_object_retain(opb);
	return (opb);
}

/*
 * prop_bool_true --
 *	Get the value of a prop_bool_t.
 */
boolean_t
prop_bool_true(prop_bool_t pb)
{

	if (! prop_object_is_bool(pb))
		return (FALSE);

	return (pb->pb_value);
}

/*
 * prop_bool_equals --
 *	Return TRUE if the boolean values are equivalent.
 */
boolean_t
prop_bool_equals(prop_bool_t b1, prop_bool_t b2)
{

	return (_prop_bool_equals(b1, b2));
}

/*
 * _prop_bool_internalize --
 *	Parse a <true/> or <false/> and return the object created from
 *	the external representation.
 */
prop_object_t
_prop_bool_internalize(struct _prop_object_internalize_context *ctx)
{
	boolean_t val;

	/* No attributes, and it must be an empty element. */
	if (ctx->poic_tagattr != NULL ||
	    ctx->poic_is_empty_element == FALSE)
	    	return (NULL);

	if (_PROP_TAG_MATCH(ctx, "true"))
		val = TRUE;
	else {
		_PROP_ASSERT(_PROP_TAG_MATCH(ctx, "false"));
		val = FALSE;
	}

	return (prop_bool_create(val));
}
