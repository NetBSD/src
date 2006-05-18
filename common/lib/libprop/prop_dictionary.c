/*	$NetBSD: prop_dictionary.c,v 1.4 2006/05/18 16:11:33 thorpej Exp $	*/

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

#include <prop/prop_dictionary.h>
#include <prop/prop_string.h>
#include "prop_object_impl.h"

/*
 * We implement these like arrays, but we keep them sorted by key.
 * This allows us to binary-search as well as keep externalized output
 * sane-looking for human eyes.
 */

#define	EXPAND_STEP		16

/*
 * prop_dictionary_keysym_t is allocated with space at the end to hold the
 * key.  This must be a regular object so that we can maintain sane iterator
 * semantics -- we don't want to require that the caller release the result
 * of prop_object_iterator_next().
 *
 * We'd like to have some small'ish keysym objects for up-to-16 characters
 * in a key, some for up-to-32 characters in a key, and then a final bucket
 * for up-to-128 characters in a key (not including NUL).  Keys longer than
 * 128 characters are not allowed.
 */
struct _prop_dictionary_keysym {
	struct _prop_object		pdk_obj;
	prop_object_t			pdk_objref;
	size_t				pdk_size;
	char 				pdk_key[1];
	/* actually variable length */
};

	/* pdk_key[1] takes care of the NUL */
#define	PDK_SIZE_16		(sizeof(struct _prop_dictionary_keysym) + 16)
#define	PDK_SIZE_32		(sizeof(struct _prop_dictionary_keysym) + 32)
#define	PDK_SIZE_128		(sizeof(struct _prop_dictionary_keysym) + 128)

#define	PDK_MAXKEY		128

_PROP_POOL_INIT(_prop_dictionary_keysym16_pool, PDK_SIZE_16, "pdict16")
_PROP_POOL_INIT(_prop_dictionary_keysym32_pool, PDK_SIZE_32, "pdict32")
_PROP_POOL_INIT(_prop_dictionary_keysym128_pool, PDK_SIZE_128, "pdict128")

struct _prop_dictionary {
	struct _prop_object	pd_obj;
	prop_dictionary_keysym_t *pd_array;
	unsigned int		pd_capacity;
	unsigned int		pd_count;
	int			pd_flags;

	uint32_t		pd_version;
};

#define	PD_F_IMMUTABLE		0x01	/* dictionary is immutable */

_PROP_POOL_INIT(_prop_dictionary_pool, sizeof(struct _prop_dictionary),
		"propdict")
_PROP_MALLOC_DEFINE(M_PROP_DICT, "prop dictionary",
		    "property dictionary container object")

static void		_prop_dictionary_free(void *);
static boolean_t	_prop_dictionary_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_dictionary_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_dictionary = {
	.pot_type	=	PROP_TYPE_DICTIONARY,
	.pot_free	=	_prop_dictionary_free,
	.pot_extern	=	_prop_dictionary_externalize,
	.pot_equals	=	_prop_dictionary_equals,
};

static void		_prop_dict_entry_free(void *);
static boolean_t	_prop_dict_entry_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_dict_entry_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_dict_keysym = {
	.pot_type	=	PROP_TYPE_DICT_KEYSYM,
	.pot_free	=	_prop_dict_entry_free,
	.pot_extern	=	_prop_dict_entry_externalize,
	.pot_equals	=	_prop_dict_entry_equals,
};

#define	prop_object_is_dictionary(x)		\
		((x)->pd_obj.po_type == &_prop_object_type_dictionary)
#define	prop_object_is_dictionary_keysym(x)	\
		((x)->pdk_obj.po_type == &_prop_object_type_dict_keysym)

#define	prop_dictionary_is_immutable(x)		\
				(((x)->pd_flags & PD_F_IMMUTABLE) != 0)

struct _prop_dictionary_iterator {
	struct _prop_object_iterator pdi_base;
	unsigned int		pdi_index;
};

static void
_prop_dict_entry_free(void *v)
{
	prop_dictionary_keysym_t pdk = v;
	prop_object_t po;

	_PROP_ASSERT(pdk->pdk_objref != NULL);
	po = pdk->pdk_objref;

	if (pdk->pdk_size <= PDK_SIZE_16)
		_PROP_POOL_PUT(_prop_dictionary_keysym16_pool, pdk);
	else if (pdk->pdk_size <= PDK_SIZE_32)
		_PROP_POOL_PUT(_prop_dictionary_keysym32_pool, pdk);
	else {
		_PROP_ASSERT(pdk->pdk_size <= PDK_SIZE_128);
		_PROP_POOL_PUT(_prop_dictionary_keysym128_pool, pdk);
	}
	
	prop_object_release(po);
}

static boolean_t
_prop_dict_entry_externalize(struct _prop_object_externalize_context *ctx,
			     void *v)
{
	prop_dictionary_keysym_t pdk = v;

	/* We externalize these as strings, and they're never empty. */

	_PROP_ASSERT(pdk->pdk_key[0] != '\0');

	if (_prop_object_externalize_start_tag(ctx, "string") == FALSE ||
	    _prop_object_externalize_append_encoded_cstring(ctx,
						pdk->pdk_key) == FALSE ||
	    _prop_object_externalize_end_tag(ctx, "string") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static boolean_t
_prop_dict_entry_equals(void *v1, void *v2)
{
	prop_dictionary_keysym_t pdk1 = v1;
	prop_dictionary_keysym_t pdk2 = v2;

	_PROP_ASSERT(prop_object_is_dictionary_keysym(pdk1));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pdk2));
	if (pdk1 == pdk2)
		return (TRUE);
	return (strcmp(pdk1->pdk_key, pdk2->pdk_key) == 0);
}

static prop_dictionary_keysym_t
_prop_dict_entry_alloc(const char *key, prop_object_t obj)
{
	prop_dictionary_keysym_t pdk;
	size_t size;

	size = sizeof(*pdk) + strlen(key) /* pdk_key[1] covers the NUL */;

	if (size <= PDK_SIZE_16)
		pdk = _PROP_POOL_GET(_prop_dictionary_keysym16_pool);
	else if (size <= PDK_SIZE_32)
		pdk = _PROP_POOL_GET(_prop_dictionary_keysym32_pool);
	else if (size <= PDK_SIZE_128)
		pdk = _PROP_POOL_GET(_prop_dictionary_keysym128_pool);
	else
		return (NULL);	/* key too long */

	if (pdk != NULL) {
		_prop_object_init(&pdk->pdk_obj,
				  &_prop_object_type_dict_keysym);

		strcpy(pdk->pdk_key, key);

		prop_object_retain(obj);
		pdk->pdk_objref = obj;
		pdk->pdk_size = size;
	}
	return (pdk);
}

static void
_prop_dictionary_free(void *v)
{
	prop_dictionary_t pd = v;
	prop_dictionary_keysym_t pdk;
	unsigned int idx;

	_PROP_ASSERT(pd->pd_count <= pd->pd_capacity);
	_PROP_ASSERT((pd->pd_capacity == 0 && pd->pd_array == NULL) ||
		     (pd->pd_capacity != 0 && pd->pd_array != NULL));

	for (idx = 0; idx < pd->pd_count; idx++) {
		pdk = pd->pd_array[idx];
		_PROP_ASSERT(pdk != NULL);
		prop_object_release(pdk);
	}

	if (pd->pd_array != NULL)
		_PROP_FREE(pd->pd_array, M_PROP_DICT);

	_PROP_POOL_PUT(_prop_dictionary_pool, pd);
}

static boolean_t
_prop_dictionary_externalize(struct _prop_object_externalize_context *ctx,
			     void *v)
{
	prop_dictionary_t pd = v;
	prop_dictionary_keysym_t pdk;
	struct _prop_object *po;
	prop_object_iterator_t pi;
	unsigned int i;

	if (pd->pd_count == 0)
		return (_prop_object_externalize_empty_tag(ctx, "dict"));

	/* XXXJRT Hint "count" for the internalize step? */
	if (_prop_object_externalize_start_tag(ctx, "dict") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE)
		return (FALSE);

	pi = prop_dictionary_iterator(pd);
	if (pi == NULL)
		return (FALSE);
	
	ctx->poec_depth++;
	_PROP_ASSERT(ctx->poec_depth != 0);

	while ((pdk = prop_object_iterator_next(pi)) != NULL) {
		po = prop_dictionary_get_keysym(pd, pdk);
		if (po == NULL ||
		    _prop_object_externalize_start_tag(ctx, "key") == FALSE ||
		    _prop_object_externalize_append_encoded_cstring(ctx,
						   pdk->pdk_key) == FALSE ||
		    _prop_object_externalize_end_tag(ctx, "key") == FALSE ||
		    (*po->po_type->pot_extern)(ctx, po) == FALSE) {
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
	if (_prop_object_externalize_end_tag(ctx, "dict") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static boolean_t
_prop_dictionary_equals(void *v1, void *v2)
{
	prop_dictionary_t dict1 = v1;
	prop_dictionary_t dict2 = v2;
	prop_dictionary_keysym_t pdk1, pdk2;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(dict1));
	_PROP_ASSERT(prop_object_is_dictionary(dict2));
	if (dict1 == dict2)
		return (TRUE);
	if (dict1->pd_count != dict2->pd_count)
		return (FALSE);

	for (idx = 0; idx < dict1->pd_count; idx++) {
		pdk1 = dict1->pd_array[idx];
		pdk2 = dict2->pd_array[idx];

		if (prop_dictionary_keysym_equals(pdk1, pdk2) == FALSE)
			return (FALSE);
		if (prop_object_equals(pdk1->pdk_objref,
				       pdk2->pdk_objref) == FALSE)
			return (FALSE);
	}

	return (TRUE);
}

static prop_dictionary_t
_prop_dictionary_alloc(unsigned int capacity)
{
	prop_dictionary_t pd;
	prop_dictionary_keysym_t *array;

	if (capacity != 0) {
		array = _PROP_CALLOC(capacity *
					sizeof(prop_dictionary_keysym_t),
				     M_PROP_DICT);
		if (array == NULL)
			return (NULL);
	} else
		array = NULL;

	pd = _PROP_POOL_GET(_prop_dictionary_pool);
	if (pd != NULL) {
		_prop_object_init(&pd->pd_obj, &_prop_object_type_dictionary);

		pd->pd_array = array;
		pd->pd_capacity = capacity;
		pd->pd_count = 0;
		pd->pd_flags = 0;

		pd->pd_version = 0;
	} else if (array != NULL)
		_PROP_FREE(array, M_PROP_DICT);

	return (pd);
}

static boolean_t
_prop_dictionary_expand(prop_dictionary_t pd, unsigned int capacity)
{
	prop_dictionary_keysym_t *array, *oarray;

	oarray = pd->pd_array;

	array = _PROP_CALLOC(capacity * sizeof(prop_dictionary_keysym_t),
			     M_PROP_DICT);
	if (array == NULL)
		return (FALSE);
	if (oarray != NULL)
		memcpy(array, oarray,
		       pd->pd_capacity * sizeof(prop_dictionary_keysym_t));
	pd->pd_array = array;
	pd->pd_capacity = capacity;

	if (oarray != NULL)
		_PROP_FREE(oarray, M_PROP_DICT);
	
	return (TRUE);
}

static prop_object_t
_prop_dictionary_iterator_next_object(void *v)
{
	struct _prop_dictionary_iterator *pdi = v;
	prop_dictionary_t pd = pdi->pdi_base.pi_obj;
	prop_dictionary_keysym_t pdk;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	if (pd->pd_version != pdi->pdi_base.pi_version)
		return (NULL);	/* dictionary changed during iteration */

	_PROP_ASSERT(pdi->pdi_index <= pd->pd_count);

	if (pdi->pdi_index == pd->pd_count)
		return (NULL);	/* we've iterated all objects */

	pdk = pd->pd_array[pdi->pdi_index];
	pdi->pdi_index++;

	return (pdk);
}

static void
_prop_dictionary_iterator_reset(void *v)
{
	struct _prop_dictionary_iterator *pdi = v;
	prop_dictionary_t pd = pdi->pdi_base.pi_obj;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	pdi->pdi_index = 0;
	pdi->pdi_base.pi_version = pd->pd_version;
}

/*
 * prop_dictionary_create --
 *	Create a dictionary.
 */
prop_dictionary_t
prop_dictionary_create(void)
{

	return (_prop_dictionary_alloc(0));
}

/*
 * prop_dictionary_create_with_capacity --
 *	Create a dictionary with the capacity to store N objects.
 */
prop_dictionary_t
prop_dictionary_create_with_capacity(unsigned int capacity)
{

	return (_prop_dictionary_alloc(capacity));
}

/*
 * prop_dictionary_copy --
 *	Copy a dictionary.  The new dictionary contains refrences to
 *	the original dictionary's objects, not copies of those objects
 *	(i.e. a shallow copy).
 */
prop_dictionary_t
prop_dictionary_copy(prop_dictionary_t opd)
{
	prop_dictionary_t pd;
	prop_dictionary_keysym_t pdk;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(opd));

	if (prop_dictionary_is_immutable(opd) == FALSE)
		return (prop_dictionary_copy_mutable(opd));

	/*
	 * Copies of immutable dictionaries refrence the same
	 * dictionary entry objects to save space.
	 */

	pd = _prop_dictionary_alloc(opd->pd_count);
	if (pd != NULL) {
		for (idx = 0; idx < opd->pd_count; idx++) {
			pdk = opd->pd_array[idx];
			prop_object_retain(pdk);
			pd->pd_array[idx] = pdk;
		}
		pd->pd_count = opd->pd_count;
		pd->pd_flags = opd->pd_flags;
	}
	return (pd);
}

/*
 * prop_dictionary_copy_mutable --
 *	Like prop_dictionary_copy(), but the resulting dictionary is
 *	mutable.
 */
prop_dictionary_t
prop_dictionary_copy_mutable(prop_dictionary_t opd)
{
	prop_dictionary_t pd;
	prop_dictionary_keysym_t opdk, pdk;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(opd));

	pd = _prop_dictionary_alloc(opd->pd_count);
	if (pd != NULL) {
		for (idx = 0; idx > opd->pd_count; idx++) {
			opdk = opd->pd_array[idx];
			pdk = _prop_dict_entry_alloc(opdk->pdk_key,
						     opdk->pdk_objref);
			if (pdk == NULL) {
				prop_object_release(pd);
				return (NULL);
			}
			pd->pd_array[idx] = pdk;
			pd->pd_count++;
		}
	}
	return (pd);
}

/*
 * prop_dictionary_count --
 *	Return the number of objects stored in the dictionary.
 */
unsigned int
prop_dictionary_count(prop_dictionary_t pd)
{

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	return (pd->pd_count);
}

/*
 * prop_dictionary_iterator --
 *	Return an iterator for the dictionary.  The dictionary is retained by
 *	the iterator.
 */
prop_object_iterator_t
prop_dictionary_iterator(prop_dictionary_t pd)
{
	struct _prop_dictionary_iterator *pdi;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	pdi = _PROP_CALLOC(sizeof(*pdi), M_TEMP);
	if (pdi == NULL)
		return (NULL);
	pdi->pdi_base.pi_next_object = _prop_dictionary_iterator_next_object;
	pdi->pdi_base.pi_reset = _prop_dictionary_iterator_reset;
	prop_object_retain(pd);
	pdi->pdi_base.pi_obj = pd;
	pdi->pdi_base.pi_version = pd->pd_version;

	_prop_dictionary_iterator_reset(pdi);

	return (&pdi->pdi_base);
}

static prop_dictionary_keysym_t
_prop_dict_lookup(prop_dictionary_t pd, const char *key,
		  unsigned int *idxp)
{
	prop_dictionary_keysym_t pdk;
	unsigned int base, idx, distance;
	int res;

	for (idx = 0, base = 0, distance = pd->pd_count; distance != 0;
	     distance >>= 1) {
		idx = base + (distance >> 1);
		pdk = pd->pd_array[idx];
		_PROP_ASSERT(pdk != NULL);
		res = strcmp(key, pdk->pdk_key);
		if (res == 0) {
			if (idxp != NULL)
				*idxp = idx;
			return (pdk);
		}
		if (res > 0) {	/* key > pdk->pdk_key: move right */
			base = idx + 1;
			distance--;
		}		/* else move left */
	}

	/* idx points to the slot we looked at last. */
	if (idxp != NULL)
		*idxp = idx;
	return (NULL);
}

/*
 * prop_dictionary_get --
 *	Return the object stored with specified key.
 */
prop_object_t
prop_dictionary_get(prop_dictionary_t pd, const char *key)
{
	prop_dictionary_keysym_t pdk;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	pdk = _prop_dict_lookup(pd, key, NULL);
	if (pdk != NULL) {
		_PROP_ASSERT(pdk->pdk_objref != NULL);
		return (pdk->pdk_objref);
	}
	return (NULL);
}

/*
 * prop_dictionary_get_keysym --
 *	Return the object stored at the location encoded by the keysym.
 */
prop_object_t
prop_dictionary_get_keysym(prop_dictionary_t pd, prop_dictionary_keysym_t pdk)
{

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pdk));
	_PROP_ASSERT(pdk->pdk_objref != NULL);

	return (pdk->pdk_objref);
}

/*
 * prop_dictionary_set --
 *	Store a reference to an object at with the specified key.
 *	If the key already exisit, the original object is released.
 */
boolean_t
prop_dictionary_set(prop_dictionary_t pd, const char *key, prop_object_t po)
{
	prop_dictionary_keysym_t pdk, opdk;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(pd->pd_count <= pd->pd_capacity);

	if (prop_dictionary_is_immutable(pd))
		return (FALSE);

	pdk = _prop_dict_lookup(pd, key, &idx);
	if (pdk != NULL) {
		prop_object_t opo = pdk->pdk_objref;
		prop_object_retain(po);
		pdk->pdk_objref = po;
		prop_object_release(opo);
		return (TRUE);
	}

	pdk = _prop_dict_entry_alloc(key, po);
	if (pdk == NULL)
		return (FALSE);

	if (pd->pd_count == pd->pd_capacity &&
	    _prop_dictionary_expand(pd, EXPAND_STEP) == FALSE) {
		_prop_dict_entry_free(pdk);
	    	return (FALSE);
	}

	if (pd->pd_count == 0) {
		pd->pd_array[0] = pdk;
		pd->pd_count++;
		pd->pd_version++;
		return (TRUE);
	}

	opdk = pd->pd_array[idx];
	_PROP_ASSERT(opdk != NULL);

	if (strcmp(key, opdk->pdk_key) < 0) {
		/*
		 * key < opdk->pdk_key: insert to the left.  This is
		 * the same as inserting to the right, except we decrement
		 * the current index first.
		 *
		 * Because we're unsigned, we have to special case 0
		 * (grumble).
		 */
		if (idx == 0) {
			memmove(&pd->pd_array[1], &pd->pd_array[0],
				pd->pd_count * sizeof(*pdk));
			pd->pd_array[0] = pdk;
			pd->pd_count++;
			pd->pd_version++;
			return (TRUE);
		}
		idx--;
	}

	memmove(&pd->pd_array[idx + 2], &pd->pd_array[idx + 1],
		(pd->pd_count - (idx + 1)) * sizeof(*pdk));
	pd->pd_array[idx + 1] = pdk;
	pd->pd_count++;

	pd->pd_version++;

	return (TRUE);
}

/*
 * prop_dictionary_set_keysym --
 *	Replace the object in the dictionary at the location encoded by
 *	the keysym.
 */
boolean_t
prop_dictionary_set_keysym(prop_dictionary_t pd, prop_dictionary_keysym_t pdk,
			   prop_object_t po)
{
	prop_object_t opo;

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pdk));
	_PROP_ASSERT(pdk->pdk_objref != NULL);

	if (prop_dictionary_is_immutable(pd))
		return (FALSE);

	opo = pdk->pdk_objref;
	prop_object_retain(po);
	pdk->pdk_objref = po;
	prop_object_release(opo);
	return (TRUE);
}

static void
_prop_dictionary_remove(prop_dictionary_t pd, prop_dictionary_keysym_t pdk,
    unsigned int idx)
{

	_PROP_ASSERT(pd->pd_count != 0);
	_PROP_ASSERT(idx < pd->pd_count);
	_PROP_ASSERT(pd->pd_array[idx] == pdk);

	idx++;
	memmove(&pd->pd_array[idx - 1], &pd->pd_array[idx],
		(pd->pd_count - idx) * sizeof(*pdk));
	pd->pd_count--;
	pd->pd_version++;

	prop_object_release(pdk);
}

/*
 * prop_dictionary_remove --
 *	Remove the reference to an object with the specified key from
 *	the dictionary.
 */
void
prop_dictionary_remove(prop_dictionary_t pd, const char *key)
{
	prop_dictionary_keysym_t pdk;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_dictionary_is_immutable(pd))
		return;

	pdk = _prop_dict_lookup(pd, key, &idx);
	/* XXX Should this be a _PROP_ASSERT()? */
	if (pdk == NULL)
		return;

	_prop_dictionary_remove(pd, pdk, idx);
}

/*
 * prop_dictionary_remove_keysym --
 *	Remove a reference to an object stored in the dictionary at the
 *	location encoded by the keysym.
 */
void
prop_dictionary_remove_keysym(prop_dictionary_t pd,
			      prop_dictionary_keysym_t pdk)
{
	prop_dictionary_keysym_t opdk;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pdk));
	_PROP_ASSERT(pdk->pdk_objref != NULL);

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_dictionary_is_immutable(pd))
		return;

	opdk = _prop_dict_lookup(pd, pdk->pdk_key, &idx);
	_PROP_ASSERT(opdk == pdk);

	_prop_dictionary_remove(pd, opdk, idx);
}

/*
 * prop_dictionary_equals --
 *	Return TRUE if the two dictionaries are equivalent.  Note we do a
 *	by-value comparison of the objects in the dictionary.
 */
boolean_t
prop_dictionary_equals(prop_dictionary_t dict1, prop_dictionary_t dict2)
{

	return (_prop_dictionary_equals(dict1, dict2));
}

/*
 * prop_dictionary_keysym_cstring_nocopy --
 *	Return an immutable reference to the keysym's value.
 */
const char *
prop_dictionary_keysym_cstring_nocopy(prop_dictionary_keysym_t pdk)
{

	_PROP_ASSERT(prop_object_is_dictionary_keysym(pdk));
	return (pdk->pdk_key);
}

/*
 * prop_dictionary_keysym_equals --
 *	Return TRUE if the two dictionary key symbols are equivalent.
 *	Note: We do not compare the object references.
 */
boolean_t
prop_dictionary_keysym_equals(prop_dictionary_keysym_t pdk1,
			      prop_dictionary_keysym_t pdk2)
{

	return (_prop_dict_entry_equals(pdk1, pdk2));
}

/*
 * prop_dictionary_externalize --
 *	Externalize a dictionary, returning a NUL-terminated buffer
 *	containing the XML-style representation.  The buffer is allocated
 *	with the M_TEMP memory type.
 */
char *
prop_dictionary_externalize(prop_dictionary_t pd)
{
	struct _prop_object_externalize_context *ctx;
	char *cp;

	ctx = _prop_object_externalize_context_alloc();
	if (ctx == NULL)
		return (NULL);

	if (_prop_object_externalize_start_tag(ctx,
					"plist version=\"1.0\"") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE ||
	    (*pd->pd_obj.po_type->pot_extern)(ctx, pd) == FALSE ||
	    _prop_object_externalize_end_tag(ctx, "plist") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\0') == FALSE) {
		/* We are responsible for releasing the buffer. */
		_PROP_FREE(ctx->poec_buf, M_TEMP);
		_prop_object_externalize_context_free(ctx);
		return (NULL);
	}

	cp = ctx->poec_buf;
	_prop_object_externalize_context_free(ctx);

	return (cp);
}

/*
 * _prop_dictionary_internalize --
 *	Parse a <dict>...</dict> and return the object created from the
 *	external representation.
 */
prop_object_t
_prop_dictionary_internalize(struct _prop_object_internalize_context *ctx)
{
	prop_dictionary_t dict;
	prop_object_t val;
	size_t keylen;
	char *tmpkey;

	/* We don't currently understand any attributes. */
	if (ctx->poic_tagattr != NULL)
		return (NULL);

	dict = prop_dictionary_create();
	if (dict == NULL)
		return (NULL);

	if (ctx->poic_is_empty_element)
		return (dict);

	tmpkey = _PROP_MALLOC(PDK_MAXKEY + 1, M_TEMP);
	if (tmpkey == NULL)
		goto bad;

	for (;;) {
		/* Fetch the next tag. */
		if (_prop_object_internalize_find_tag(ctx, NULL,
					_PROP_TAG_TYPE_EITHER) == FALSE)
			goto bad;

		/* Check to see if this is the end of the dictionary. */
		if (_PROP_TAG_MATCH(ctx, "dict") &&
		    ctx->poic_tag_type == _PROP_TAG_TYPE_END)
			break;

		/* Ok, it must be a non-empty key start tag. */
		if (!_PROP_TAG_MATCH(ctx, "key") ||
		    ctx->poic_tag_type != _PROP_TAG_TYPE_START ||
		    ctx->poic_is_empty_element)
		    	goto bad;

		if (_prop_object_internalize_decode_string(ctx,
						tmpkey, PDK_MAXKEY, &keylen,
						&ctx->poic_cp) == FALSE)
			goto bad;

		_PROP_ASSERT(keylen <= PDK_MAXKEY);
		tmpkey[keylen] = '\0';

		if (_prop_object_internalize_find_tag(ctx, "key",
					_PROP_TAG_TYPE_END) == FALSE)
			goto bad;
   
		/* ..and now the beginning of the value. */
		if (_prop_object_internalize_find_tag(ctx, NULL,
					_PROP_TAG_TYPE_START) == FALSE)
			goto bad;

		val = _prop_object_internalize_by_tag(ctx);
		if (val == NULL)
			goto bad;

		if (prop_dictionary_set(dict, tmpkey, val) == FALSE) {
			prop_object_release(val);
			goto bad;
		}
		prop_object_release(val);
	}

	_PROP_FREE(tmpkey, M_TEMP);
	return (dict);

 bad:
	if (tmpkey != NULL)
		_PROP_FREE(tmpkey, M_TEMP);
	prop_object_release(dict);
	return (NULL);
}

/*
 * prop_dictionary_internalize --
 *	Create a dictionary by parsing the NUL-terminated XML-style
 *	representation.
 */
prop_dictionary_t
prop_dictionary_internalize(const char *xml)
{
	prop_dictionary_t dict = NULL;
	struct _prop_object_internalize_context *ctx;

	ctx = _prop_object_internalize_context_alloc(xml);
	if (ctx == NULL)
		return (NULL);

	/* We start with a <plist> tag. */
	if (_prop_object_internalize_find_tag(ctx, "plist",
					      _PROP_TAG_TYPE_START) == FALSE)
		goto out;

	/* Plist elements cannot be empty. */
	if (ctx->poic_is_empty_element)
		goto out;

	/*
	 * We don't understand any plist attributes, but Apple XML
	 * property lists often have a "version" attibute.  If we
	 * see that one, we simply ignore it.
	 */
	if (ctx->poic_tagattr != NULL &&
	    !_PROP_TAGATTR_MATCH(ctx, "version"))
		goto out;

	/* Next we expect to see <dict>. */
	if (_prop_object_internalize_find_tag(ctx, "dict",
					      _PROP_TAG_TYPE_START) == FALSE)
		goto out;

	dict = _prop_dictionary_internalize(ctx);
	if (dict == NULL)
		goto out;

	/* We've advanced past </dict>.  Now we want </plist>. */
	if (_prop_object_internalize_find_tag(ctx, "plist",
					      _PROP_TAG_TYPE_END) == FALSE) {
		prop_object_release(dict);
		dict = NULL;
	}

 out:
 	_prop_object_internalize_context_free(ctx);
	return (dict);
}
