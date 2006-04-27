/*	$NetBSD: prop_number.c,v 1.1 2006/04/27 20:11:27 thorpej Exp $	*/

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

#include <prop/prop_number.h>
#include "prop_object_impl.h"

#if defined(_KERNEL)
#include <sys/inttypes.h>
#include <sys/systm.h>
#elif defined(_STANDALONE)
#include <sys/param.h>
#include <lib/libkern/libkern.h>
#else
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#endif

struct _prop_number {
	struct _prop_object	pn_obj;
	uintmax_t		pn_number;
};

_PROP_POOL_INIT(_prop_number_pool, sizeof(struct _prop_number), "propnmbr")

#define	prop_object_is_number(x) ((x)->pn_obj.po_type == PROP_TYPE_NUMBER)

static void
_prop_number_free(void *v)
{

	_PROP_POOL_PUT(_prop_number_pool, v);
}

static boolean_t
_prop_number_externalize(struct _prop_object_externalize_context *ctx,
			 void *v)
{
	prop_number_t pn = v;
	char tmpstr[32];

	sprintf(tmpstr, "0x%" PRIxMAX, pn->pn_number);

	if (_prop_object_externalize_start_tag(ctx, "integer") == FALSE ||
	    _prop_object_externalize_append_cstring(ctx, tmpstr) == FALSE ||
	    _prop_object_externalize_end_tag(ctx, "integer") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static prop_number_t
_prop_number_alloc(uintmax_t val)
{
	prop_number_t pn;

	pn = _PROP_POOL_GET(_prop_number_pool);
	if (pn != NULL) {
		_prop_object_init(&pn->pn_obj);
		pn->pn_obj.po_type = PROP_TYPE_NUMBER;
		pn->pn_obj.po_free = _prop_number_free;
		pn->pn_obj.po_extern = _prop_number_externalize;

		pn->pn_number = val;
	}

	return (pn);
}

/*
 * prop_number_create_integer --
 *	Create a prop_number_t and initialize it with the
 *	provided integer value.
 */
prop_number_t
prop_number_create_integer(uintmax_t val)
{

	return (_prop_number_alloc(val));
}

/*
 * prop_number_copy --
 *	Copy a prop_number_t.
 */
prop_number_t
prop_number_copy(prop_number_t opn)
{

	_PROP_ASSERT(prop_object_is_number(opn));

	return (_prop_number_alloc(opn->pn_number));
}

/*
 * prop_number_size --
 *	Return the size, in bits, required to hold the value of
 *	the specified number.
 */
int
prop_number_size(prop_number_t pn)
{

	_PROP_ASSERT(prop_object_is_number(pn));
	if (pn->pn_number > UINT32_MAX)
		return (64);
	if (pn->pn_number > UINT16_MAX)
		return (32);
	if (pn->pn_number > UINT8_MAX)
		return (16);
	return (8);
}

/*
 * prop_number_integer_value --
 *	Get the integer value of a prop_number_t.
 */
uintmax_t
prop_number_integer_value(prop_number_t pn)
{

	_PROP_ASSERT(prop_object_is_number(pn));
	return (pn->pn_number);
}

/*
 * prop_number_equals --
 *	Return TRUE if two numbers are equivalent.
 */
boolean_t
prop_number_equals(prop_number_t num1, prop_number_t num2)
{

	_PROP_ASSERT(prop_object_is_number(num1));
	_PROP_ASSERT(prop_object_is_number(num2));
	return (num1->pn_number == num2->pn_number);
}

/*
 * prop_number_equals_integer --
 *	Return TRUE if the number is equivalent to the specified integer.
 */
boolean_t
prop_number_equals_integer(prop_number_t pn, uintmax_t val)
{

	_PROP_ASSERT(prop_object_is_number(pn));
	return (pn->pn_number == val);
}

/*
 * _prop_number_internalize --
 *	Parse a <number>...</number> and return the object created from
 *	the external representation.
 */
prop_object_t
_prop_number_internalize(struct _prop_object_internalize_context *ctx)
{
	uintmax_t val = 0;
	char *cp;

	/* No attributes, no empty elements. */
	if (ctx->poic_tagattr != NULL || ctx->poic_is_empty_element)
		return (NULL);

#ifndef _KERNEL
	errno = 0;
#endif
	val = strtoumax(ctx->poic_cp, &cp, 0);
#ifndef _KERNEL		/* XXX can't check for ERANGE in the kernel */
	if (val == UINTMAX_MAX && errno == ERANGE)
		return (NULL);
#endif
	ctx->poic_cp = cp;
	
	if (_prop_object_internalize_find_tag(ctx, "integer",
					      _PROP_TAG_TYPE_END) == FALSE)
		return (NULL);

	return (_prop_number_alloc(val));
}
