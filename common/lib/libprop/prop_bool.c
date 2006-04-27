/*	$NetBSD: prop_bool.c,v 1.1 2006/04/27 20:11:27 thorpej Exp $	*/

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

_PROP_POOL_INIT(_prop_bool_pool, sizeof(struct _prop_bool), "propbool")

#define	prop_object_is_bool(x)	((x)->pb_obj.po_type == PROP_TYPE_BOOL)

static void
_prop_bool_free(void *v)
{

	_PROP_POOL_PUT(_prop_bool_pool, v);
}

static boolean_t
_prop_bool_externalize(struct _prop_object_externalize_context *ctx,
		       void *v)
{
	prop_bool_t pb = v;

	return (_prop_object_externalize_empty_tag(ctx,
	    pb->pb_value ? "true" : "false"));
}

static prop_bool_t
_prop_bool_alloc(void)
{
	prop_bool_t pb;

	pb = _PROP_POOL_GET(_prop_bool_pool);
	if (pb != NULL) {
		_prop_object_init(&pb->pb_obj);
		pb->pb_obj.po_type = PROP_TYPE_BOOL;
		pb->pb_obj.po_free = _prop_bool_free;
		pb->pb_obj.po_extern = _prop_bool_externalize;
	}

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
	prop_bool_t pb;

	pb = _prop_bool_alloc();
	if (pb != NULL)
		pb->pb_value = val;
	return (pb);
}

/*
 * prop_bool_copy --
 *	Copy a prop_bool_t.
 */
prop_bool_t
prop_bool_copy(prop_bool_t opb)
{
	prop_bool_t pb;

	_PROP_ASSERT(prop_object_is_bool(opb));

	pb = _prop_bool_alloc();
	if (pb != NULL)
		pb->pb_value = opb->pb_value;
	return (pb);
}

/*
 * prop_bool_true --
 *	Get the value of a prop_bool_t.
 */
boolean_t
prop_bool_true(prop_bool_t pb)
{

	_PROP_ASSERT(prop_object_is_bool(pb));
	return (pb->pb_value);
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
