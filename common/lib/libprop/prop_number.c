/*	$NetBSD: prop_number.c,v 1.7 2006/10/03 15:45:04 thorpej Exp $	*/

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
#include "prop_rb_impl.h"

#if defined(_KERNEL)
#include <sys/systm.h>
#elif defined(_STANDALONE)
#include <sys/param.h>
#include <lib/libkern/libkern.h>
#else
#include <errno.h>
#include <stdlib.h>
#endif

struct _prop_number {
	struct _prop_object	pn_obj;
	struct rb_node		pn_link;
	uint64_t		pn_number;
};

#define	RBNODE_TO_PN(n)							\
	((struct _prop_number *)					\
	 ((uintptr_t)n - offsetof(struct _prop_number, pn_link)))

_PROP_POOL_INIT(_prop_number_pool, sizeof(struct _prop_number), "propnmbr")

static void		_prop_number_free(void *);
static boolean_t	_prop_number_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_number_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_number = {
	.pot_type	=	PROP_TYPE_NUMBER,
	.pot_free	=	_prop_number_free,
	.pot_extern	=	_prop_number_externalize,
	.pot_equals	=	_prop_number_equals,
};

#define	prop_object_is_number(x)	\
	((x) != NULL && (x)->pn_obj.po_type == &_prop_object_type_number)

/*
 * Number objects are immutable, and we are likely to have many number
 * objects that have the same value.  So, to save memory, we unique'ify
 * numbers so we only have one copy of each.
 */

static int
_prop_number_rb_compare_nodes(const struct rb_node *n1,
			      const struct rb_node *n2)
{
	const prop_number_t pn1 = RBNODE_TO_PN(n1);
	const prop_number_t pn2 = RBNODE_TO_PN(n2);

	if (pn1->pn_number < pn2->pn_number)
		return (-1);
	if (pn1->pn_number > pn2->pn_number)
		return (1);
	return (0);
}

static int
_prop_number_rb_compare_key(const struct rb_node *n,
			    const void *v)
{
	const prop_number_t pn = RBNODE_TO_PN(n);
	const uint64_t *valp = v;

	if (pn->pn_number < *valp)
		return (-1);
	if (pn->pn_number > *valp)
		return (1);
	return (0);
}

static const struct rb_tree_ops _prop_number_rb_tree_ops = {
	.rbto_compare_nodes = _prop_number_rb_compare_nodes,
	.rbto_compare_key   = _prop_number_rb_compare_key,
};

static struct rb_tree _prop_number_tree;
static boolean_t _prop_number_tree_initialized;

_PROP_MUTEX_DECL_STATIC(_prop_number_tree_mutex)

static void
_prop_number_free(void *v)
{
	prop_number_t pn = v;

	_PROP_MUTEX_LOCK(_prop_number_tree_mutex);
	_prop_rb_tree_remove_node(&_prop_number_tree, &pn->pn_link);
	_PROP_MUTEX_UNLOCK(_prop_number_tree_mutex);

	_PROP_POOL_PUT(_prop_number_pool, pn);
}

static boolean_t
_prop_number_externalize(struct _prop_object_externalize_context *ctx,
			 void *v)
{
	prop_number_t pn = v;
	char tmpstr[32];

	sprintf(tmpstr, "0x%" PRIx64, pn->pn_number);

	if (_prop_object_externalize_start_tag(ctx, "integer") == FALSE ||
	    _prop_object_externalize_append_cstring(ctx, tmpstr) == FALSE ||
	    _prop_object_externalize_end_tag(ctx, "integer") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static boolean_t
_prop_number_equals(void *v1, void *v2)
{
	prop_number_t num1 = v1;
	prop_number_t num2 = v2;

	if (! (prop_object_is_number(num1) &&
	       prop_object_is_number(num2)))
		return (FALSE);

	/*
	 * There is only ever one copy of a number object at any given
	 * time, so we can reduce this to a simple pointer equality check.
	 */
	return (num1 == num2);
}

static prop_number_t
_prop_number_alloc(uint64_t val)
{
	prop_number_t opn, pn;
	struct rb_node *n;

	/*
	 * Check to see if this already exists in the tree.  If it does,
	 * we just retain it and return it.
	 */
	_PROP_MUTEX_LOCK(_prop_number_tree_mutex);
	if (! _prop_number_tree_initialized) {
		_prop_rb_tree_init(&_prop_number_tree,
				   &_prop_number_rb_tree_ops);
		_prop_number_tree_initialized = TRUE;
	} else {
		n = _prop_rb_tree_find(&_prop_number_tree, &val);
		if (n != NULL) {
			opn = RBNODE_TO_PN(n);
			prop_object_retain(opn);
			_PROP_MUTEX_UNLOCK(_prop_number_tree_mutex);
			return (opn);
		}
	}
	_PROP_MUTEX_UNLOCK(_prop_number_tree_mutex);

	/*
	 * Not in the tree.  Create it now.
	 */

	pn = _PROP_POOL_GET(_prop_number_pool);
	if (pn == NULL)
		return (NULL);

	_prop_object_init(&pn->pn_obj, &_prop_object_type_number);

	pn->pn_number = val;

	/*
	 * We dropped the mutex when we allocated the new object, so
	 * we have to check again if it is in the tree.
	 */
	_PROP_MUTEX_LOCK(_prop_number_tree_mutex);
	n = _prop_rb_tree_find(&_prop_number_tree, &val);
	if (n != NULL) {
		opn = RBNODE_TO_PN(n);
		prop_object_retain(opn);
		_PROP_MUTEX_UNLOCK(_prop_number_tree_mutex);
		_PROP_POOL_PUT(_prop_number_pool, pn);
		return (opn);
	}
	_prop_rb_tree_insert_node(&_prop_number_tree, &pn->pn_link);
	_PROP_MUTEX_UNLOCK(_prop_number_tree_mutex);
	return (pn);
}

/*
 * prop_number_create_integer --
 *	Create a prop_number_t and initialize it with the
 *	provided integer value.
 */
prop_number_t
prop_number_create_integer(uint64_t val)
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

	if (! prop_object_is_number(opn))
		return (NULL);

	/*
	 * Because we only ever allocate one object for any given
	 * value, this can be reduced to a simple retain operation.
	 */
	prop_object_retain(opn);
	return (opn);
}

/*
 * prop_number_size --
 *	Return the size, in bits, required to hold the value of
 *	the specified number.
 */
int
prop_number_size(prop_number_t pn)
{

	if (! prop_object_is_number(pn))
		return (0);

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
uint64_t
prop_number_integer_value(prop_number_t pn)
{

	/*
	 * XXX Impossible to distinguish between "not a prop_number_t"
	 * XXX and "prop_number_t has a value of 0".
	 */
	if (! prop_object_is_number(pn))
		return (0);

	return (pn->pn_number);
}

/*
 * prop_number_equals --
 *	Return TRUE if two numbers are equivalent.
 */
boolean_t
prop_number_equals(prop_number_t num1, prop_number_t num2)
{

	return (_prop_number_equals(num1, num2));
}

/*
 * prop_number_equals_integer --
 *	Return TRUE if the number is equivalent to the specified integer.
 */
boolean_t
prop_number_equals_integer(prop_number_t pn, uint64_t val)
{

	if (! prop_object_is_number(pn))
		return (FALSE);

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
	uint64_t val = 0;
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
