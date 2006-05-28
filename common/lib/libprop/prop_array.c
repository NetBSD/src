/*	$NetBSD: prop_array.c,v 1.3 2006/05/28 03:53:51 thorpej Exp $	*/

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

#include <prop/prop_array.h>
#include "prop_object_impl.h"

struct _prop_array {
	struct _prop_object	pa_obj;
	prop_object_t *		pa_array;
	unsigned int		pa_capacity;
	unsigned int		pa_count;
	int			pa_flags;

	uint32_t		pa_version;
};

#define	PA_F_IMMUTABLE		0x01	/* array is immutable */

_PROP_POOL_INIT(_prop_array_pool, sizeof(struct _prop_array), "proparay")
_PROP_MALLOC_DEFINE(M_PROP_ARRAY, "prop array",
		    "property array container object")

static void		_prop_array_free(void *);
static boolean_t	_prop_array_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_array_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_array = {
	.pot_type	=	PROP_TYPE_ARRAY,
	.pot_free	=	_prop_array_free,
	.pot_extern	=	_prop_array_externalize,
	.pot_equals	=	_prop_array_equals,
};

#define	prop_object_is_array(x) 	\
	((x)->pa_obj.po_type == &_prop_object_type_array)

#define	prop_array_is_immutable(x) (((x)->pa_flags & PA_F_IMMUTABLE) != 0)

struct _prop_array_iterator {
	struct _prop_object_iterator pai_base;
	unsigned int		pai_index;
};

#define	EXPAND_STEP		16

static void
_prop_array_free(void *v)
{
	prop_array_t pa = v;
	prop_object_t po;
	unsigned int idx;

	_PROP_ASSERT(pa->pa_count <= pa->pa_capacity);
	_PROP_ASSERT((pa->pa_capacity == 0 && pa->pa_array == NULL) ||
		     (pa->pa_capacity != 0 && pa->pa_array != NULL));

	for (idx = 0; idx < pa->pa_count; idx++) {
		po = pa->pa_array[idx];
		_PROP_ASSERT(po != NULL);
		prop_object_release(po);
	}

	if (pa->pa_array != NULL)
		_PROP_FREE(pa->pa_array, M_PROP_ARRAY);

	_PROP_POOL_PUT(_prop_array_pool, pa);
}

static boolean_t
_prop_array_externalize(struct _prop_object_externalize_context *ctx,
			void *v)
{
	prop_array_t pa = v;
	struct _prop_object *po;
	prop_object_iterator_t pi;
	unsigned int i;

	if (pa->pa_count == 0)
		return (_prop_object_externalize_empty_tag(ctx, "array"));
	
	/* XXXJRT Hint "count" for the internalize step? */
	if (_prop_object_externalize_start_tag(ctx, "array") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE)
		return (FALSE);

	pi = prop_array_iterator(pa);
	if (pi == NULL)
		return (FALSE);
	
	ctx->poec_depth++;
	_PROP_ASSERT(ctx->poec_depth != 0);

	while ((po = prop_object_iterator_next(pi)) != NULL) {
		if ((*po->po_type->pot_extern)(ctx, po) == FALSE) {
			prop_object_iterator_release(pi);
			return (FALSE);
		}
	}

	prop_object_iterator_release(pi);

	ctx->poec_depth--;
	for (i = 0; i < ctx->poec_depth; i++) {
		if (_prop_object_externalize_append_char(ctx, '\t') == FALSE)
			return (FALSE);
	}
	if (_prop_object_externalize_end_tag(ctx, "array") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static boolean_t
_prop_array_equals(void *v1, void *v2)
{
	prop_array_t array1 = v1;
	prop_array_t array2 = v2;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_array(array1));
	_PROP_ASSERT(prop_object_is_array(array2));

	if (array1 == array2)
		return (TRUE);
	if (array1->pa_count != array2->pa_count)
		return (FALSE);
	
	for (idx = 0; idx < array1->pa_count; idx++) {
		if (prop_object_equals(array1->pa_array[idx],
				       array2->pa_array[idx]) == FALSE)
			return (FALSE);
	}

	return (TRUE);
}

static prop_array_t
_prop_array_alloc(unsigned int capacity)
{
	prop_array_t pa;
	prop_object_t *array;

	if (capacity != 0) {
		array = _PROP_CALLOC(capacity * sizeof(prop_object_t),
				     M_PROP_ARRAY);
		if (array == NULL)
			return (NULL);
	} else
		array = NULL;
	

	pa = _PROP_POOL_GET(_prop_array_pool);
	if (pa != NULL) {
		_prop_object_init(&pa->pa_obj, &_prop_object_type_array);
		pa->pa_obj.po_type = &_prop_object_type_array;

		pa->pa_array = array;
		pa->pa_capacity = capacity;
		pa->pa_count = 0;
		pa->pa_flags = 0;

		pa->pa_version = 0;
	} else if (array != NULL)
		_PROP_FREE(array, M_PROP_ARRAY);

	return (pa);
}

static boolean_t
_prop_array_expand(prop_array_t pa, unsigned int capacity)
{
	prop_object_t *array, *oarray;

	oarray = pa->pa_array;

	array = _PROP_CALLOC(capacity * sizeof(*array), M_PROP_ARRAY);
	if (array == NULL)
		return (FALSE);
	if (oarray != NULL)
		memcpy(array, oarray, pa->pa_capacity * sizeof(*array));
	pa->pa_array = array;
	pa->pa_capacity = capacity;

	if (oarray != NULL)
		_PROP_FREE(oarray, M_PROP_ARRAY);

	return (TRUE);
}

static prop_object_t
_prop_array_iterator_next_object(void *v)
{
	struct _prop_array_iterator *pai = v;
	prop_array_t pa = pai->pai_base.pi_obj;
	prop_object_t po;

	_PROP_ASSERT(prop_object_is_array(pa));

	if (pa->pa_version != pai->pai_base.pi_version)
		return (NULL);	/* array changed during iteration */
	
	_PROP_ASSERT(pai->pai_index <= pa->pa_count);

	if (pai->pai_index == pa->pa_count)
		return (NULL);	/* we've iterated all objects */
	
	po = pa->pa_array[pai->pai_index];
	pai->pai_index++;

	return (po);
}

static void
_prop_array_iterator_reset(void *v)
{
	struct _prop_array_iterator *pai = v;
	prop_array_t pa = pai->pai_base.pi_obj;

	_PROP_ASSERT(prop_object_is_array(pa));

	pai->pai_index = 0;
	pai->pai_base.pi_version = pa->pa_version;
}

/*
 * prop_array_create --
 *	Create an empty array.
 */
prop_array_t
prop_array_create(void)
{

	return (_prop_array_alloc(0));
}

/*
 * prop_array_create_with_capacity --
 *	Create an array with the capacity to store N objects.
 */
prop_array_t
prop_array_create_with_capacity(unsigned int capacity)
{

	return (_prop_array_alloc(capacity));
}

/*
 * prop_array_copy --
 *	Copy an array.  The new array has an initial capacity equal to
 *	the number of objects stored in the original array.  The new
 *	array contains references to the original array's objects, not
 *	copies of those objects (i.e. a shallow copy).
 */
prop_array_t
prop_array_copy(prop_array_t opa)
{
	prop_array_t pa;
	prop_object_t po;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_array(opa));

	pa = _prop_array_alloc(opa->pa_count);
	if (pa != NULL) {
		for (idx = 0; idx < opa->pa_count; idx++) {
			po = opa->pa_array[idx];
			prop_object_retain(po);
			pa->pa_array[idx] = po;
		}
		pa->pa_count = opa->pa_count;
		pa->pa_flags = opa->pa_flags;
	}
	return (pa);
}

/*
 * prop_array_copy_mutable --
 *	Like prop_array_copy(), but the resulting array is mutable.
 */
prop_array_t
prop_array_copy_mutable(prop_array_t opa)
{
	prop_array_t pa;

	pa = prop_array_copy(opa);
	if (pa != NULL)
		pa->pa_flags &= ~PA_F_IMMUTABLE;
	
	return (pa);
}

/*
 * prop_array_capacity --
 *	Return the capacity of the array.
 */
unsigned int
prop_array_capacity(prop_array_t pa)
{

	_PROP_ASSERT(prop_object_is_array(pa));
	return (pa->pa_capacity);
}

/*
 * prop_array_count --
 *	Return the number of objects stored in the array.
 */
unsigned int
prop_array_count(prop_array_t pa)
{

	_PROP_ASSERT(prop_object_is_array(pa));
	return (pa->pa_count);
}

/*
 * prop_array_ensure_capacity --
 *	Ensure that the array has the capacity to store the specified
 *	total number of objects (inluding the objects already stored
 *	in the array).
 */
boolean_t
prop_array_ensure_capacity(prop_array_t pa, unsigned int capacity)
{

	_PROP_ASSERT(prop_object_is_array(pa));
	if (capacity > pa->pa_capacity)
		return (_prop_array_expand(pa, capacity));
	return (TRUE);
}

/*
 * prop_array_iterator --
 *	Return an iterator for the array.  The array is retained by
 *	the iterator.
 */
prop_object_iterator_t
prop_array_iterator(prop_array_t pa)
{
	struct _prop_array_iterator *pai;

	_PROP_ASSERT(prop_object_is_array(pa));

	pai = _PROP_CALLOC(sizeof(*pai), M_TEMP);
	if (pai == NULL)
		return (NULL);
	pai->pai_base.pi_next_object = _prop_array_iterator_next_object;
	pai->pai_base.pi_reset = _prop_array_iterator_reset;
	prop_object_retain(pa);
	pai->pai_base.pi_obj = pa;
	pai->pai_base.pi_version = pa->pa_version;
	
	_prop_array_iterator_reset(pai);

	return (&pai->pai_base);
}

/*
 * prop_array_make_immutable --
 *	Make the array immutable.
 */
void
prop_array_make_immutable(prop_array_t pa)
{

	if (prop_array_is_immutable(pa) == FALSE)
		pa->pa_flags |= PA_F_IMMUTABLE;
}

/*
 * prop_array_mutable --
 *	Returns TRUE if the array is mutable.
 */
boolean_t
prop_array_mutable(prop_array_t pa)
{

	return (prop_array_is_immutable(pa) == FALSE);
}

/*
 * prop_array_get --
 *	Return the object stored at the specified array index.
 */
prop_object_t
prop_array_get(prop_array_t pa, unsigned int idx)
{
	prop_object_t po;

	_PROP_ASSERT(prop_object_is_array(pa));
	if (idx >= pa->pa_count)
		return (NULL);
	po = pa->pa_array[idx];
	_PROP_ASSERT(po != NULL);
	return (po);
}

/*
 * prop_array_set --
 *	Store a reference to an object at the specified array index.
 *	This method is not allowed to create holes in the array; the
 *	caller must either be setting the object just beyond the existing
 *	count or replacing an already existing object reference.
 */
boolean_t
prop_array_set(prop_array_t pa, unsigned int idx, prop_object_t po)
{
	prop_object_t opo;

	_PROP_ASSERT(prop_object_is_array(pa));

	if (prop_array_is_immutable(pa))
		return (FALSE);
	
	if (idx == pa->pa_count)
		return (prop_array_add(pa, po));
	
	_PROP_ASSERT(idx < pa->pa_count);

	opo = pa->pa_array[idx];
	_PROP_ASSERT(opo != NULL);
	
	prop_object_retain(po);
	pa->pa_array[idx] = po;
	pa->pa_version++;

	prop_object_release(opo);

	return (TRUE);
}

/*
 * prop_array_add --
 *	Add a refrerence to an object to the specified array, appending
 *	to the end and growing the array's capacity, if necessary.
 */
boolean_t
prop_array_add(prop_array_t pa, prop_object_t po)
{

	_PROP_ASSERT(prop_object_is_array(pa));
	_PROP_ASSERT(pa->pa_count <= pa->pa_capacity);

	if (prop_array_is_immutable(pa) ||
	    (pa->pa_count == pa->pa_capacity &&
	    _prop_array_expand(pa, pa->pa_capacity + EXPAND_STEP) == FALSE))
		return (FALSE);

	prop_object_retain(po);
	pa->pa_array[pa->pa_count++] = po;
	pa->pa_version++;

	return (TRUE);
}

/*
 * prop_array_remove --
 *	Remove the reference to an object from an array at the specified
 *	index.  The array will be compacted following the removal.
 */
void
prop_array_remove(prop_array_t pa, unsigned int idx)
{
	prop_object_t po;

	_PROP_ASSERT(prop_object_is_array(pa));
	_PROP_ASSERT(idx < pa->pa_count);

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_array_is_immutable(pa))
		return;

	po = pa->pa_array[idx];
	_PROP_ASSERT(po != NULL);

	for (++idx; idx < pa->pa_count; idx++)
		pa->pa_array[idx - 1] = pa->pa_array[idx];
	pa->pa_count--;
	pa->pa_version++;
	
	prop_object_release(po);
}

/*
 * prop_array_equals --
 *	Return TRUE if the two arrays are equivalent.  Note we do a
 *	by-value comparison of the objects in the array.
 */
boolean_t
prop_array_equals(prop_array_t array1, prop_array_t array2)
{

	return (_prop_array_equals(array1, array2));
}

/*
 * _prop_array_internalize --
 *	Parse an <array>...</array> and return the object created from the
 *	external representation.
 */
prop_object_t
_prop_array_internalize(struct _prop_object_internalize_context *ctx)
{
	prop_array_t array;
	prop_object_t obj;

	/* We don't currently understand any attributes. */
	if (ctx->poic_tagattr != NULL)
		return (NULL);
	
	array = prop_array_create();
	if (array == NULL)
		return (NULL);
	
	if (ctx->poic_is_empty_element)
		return (array);
	
	for (;;) {
		/* Fetch the next tag. */
		if (_prop_object_internalize_find_tag(ctx, NULL,
					_PROP_TAG_TYPE_EITHER) == FALSE)
			goto bad;

		/* Check to see if this is the end of the array. */
		if (_PROP_TAG_MATCH(ctx, "array") &&
		    ctx->poic_tag_type == _PROP_TAG_TYPE_END)
		    	break;

		/* Fetch the object. */
		obj = _prop_object_internalize_by_tag(ctx);
		if (obj == NULL)
			goto bad;

		if (prop_array_add(array, obj) == FALSE) {
			prop_object_release(obj);
			goto bad;
		}
		prop_object_release(obj);
	}

	return (array);

 bad:
	prop_object_release(array);
	return (NULL);
}
