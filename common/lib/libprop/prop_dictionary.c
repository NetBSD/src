/*	$NetBSD: prop_dictionary.c,v 1.2 2006/05/07 06:25:49 simonb Exp $	*/

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
	struct _prop_object		pde_obj;
	prop_object_t			pde_objref;
	size_t				pde_size;
	char 				pde_key[1];
	/* actually variable length */
};

	/* pde_key[1] takes care of the NUL */
#define	PDE_SIZE_16		(sizeof(struct _prop_dictionary_keysym) + 16)
#define	PDE_SIZE_32		(sizeof(struct _prop_dictionary_keysym) + 32)
#define	PDE_SIZE_128		(sizeof(struct _prop_dictionary_keysym) + 128)

#define	PDE_MAXKEY		128

_PROP_POOL_INIT(_prop_dictionary_keysym16_pool, PDE_SIZE_16, "pdict16")
_PROP_POOL_INIT(_prop_dictionary_keysym32_pool, PDE_SIZE_32, "pdict32")
_PROP_POOL_INIT(_prop_dictionary_keysym128_pool, PDE_SIZE_128, "pdict128")

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

#define	prop_object_is_dictionary(x)		\
				((x)->pd_obj.po_type == PROP_TYPE_DICTIONARY)
#define	prop_object_is_dictionary_keysym(x)	\
				((x)->pde_obj.po_type == PROP_TYPE_DICT_KEYSYM)

#define	prop_dictionary_is_immutable(x)		\
				(((x)->pd_flags & PD_F_IMMUTABLE) != 0)

struct _prop_dictionary_iterator {
	struct _prop_object_iterator pdi_base;
	unsigned int		pdi_index;
};

static void
_prop_dict_entry_free(void *v)
{
	prop_dictionary_keysym_t pde = v;
	prop_object_t po;

	_PROP_ASSERT(pde->pde_objref != NULL);
	po = pde->pde_objref;

	if (pde->pde_size <= PDE_SIZE_16)
		_PROP_POOL_PUT(_prop_dictionary_keysym16_pool, pde);
	else if (pde->pde_size <= PDE_SIZE_32)
		_PROP_POOL_PUT(_prop_dictionary_keysym32_pool, pde);
	else {
		_PROP_ASSERT(pde->pde_size <= PDE_SIZE_128);
		_PROP_POOL_PUT(_prop_dictionary_keysym128_pool, pde);
	}
	
	prop_object_release(po);
}

static boolean_t
_prop_dict_entry_externalize(struct _prop_object_externalize_context *ctx,
			     void *v)
{
	prop_dictionary_keysym_t pde = v;

	/* We externalize these as strings, and they're never empty. */

	_PROP_ASSERT(pde->pde_key[0] != '\0');

	if (_prop_object_externalize_start_tag(ctx, "string") == FALSE ||
	    _prop_object_externalize_append_encoded_cstring(ctx,
						pde->pde_key) == FALSE ||
	    _prop_object_externalize_end_tag(ctx, "string") == FALSE)
		return (FALSE);
	
	return (TRUE);
}

static prop_dictionary_keysym_t
_prop_dict_entry_alloc(const char *key, prop_object_t obj)
{
	prop_dictionary_keysym_t pde;
	size_t size;

	size = sizeof(*pde) + strlen(key) /* pde_key[1] covers the NUL */;

	if (size <= PDE_SIZE_16)
		pde = _PROP_POOL_GET(_prop_dictionary_keysym16_pool);
	else if (size <= PDE_SIZE_32)
		pde = _PROP_POOL_GET(_prop_dictionary_keysym32_pool);
	else if (size <= PDE_SIZE_128)
		pde = _PROP_POOL_GET(_prop_dictionary_keysym128_pool);
	else
		return (NULL);	/* key too long */

	if (pde != NULL) {
		_prop_object_init(&pde->pde_obj);
		pde->pde_obj.po_type = PROP_TYPE_DICT_KEYSYM;
		pde->pde_obj.po_free = _prop_dict_entry_free;
		pde->pde_obj.po_extern = _prop_dict_entry_externalize;

		strcpy(pde->pde_key, key);

		prop_object_retain(obj);
		pde->pde_objref = obj;
		pde->pde_size = size;
	}
	return (pde);
}

static void
_prop_dictionary_free(void *v)
{
	prop_dictionary_t pd = v;
	prop_dictionary_keysym_t pde;
	unsigned int idx;

	_PROP_ASSERT(pd->pd_count <= pd->pd_capacity);
	_PROP_ASSERT((pd->pd_capacity == 0 && pd->pd_array == NULL) ||
		     (pd->pd_capacity != 0 && pd->pd_array != NULL));

	for (idx = 0; idx < pd->pd_count; idx++) {
		pde = pd->pd_array[idx];
		_PROP_ASSERT(pde != NULL);
		prop_object_release(pde);
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
	prop_dictionary_keysym_t pde;
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

	while ((pde = prop_object_iterator_next(pi)) != NULL) {
		po = prop_dictionary_get_keysym(pd, pde);
		if (po == NULL ||
		    _prop_object_externalize_start_tag(ctx, "key") == FALSE ||
		    _prop_object_externalize_append_encoded_cstring(ctx,
						   pde->pde_key) == FALSE ||
		    _prop_object_externalize_end_tag(ctx, "key") == FALSE ||
		    (*po->po_extern)(ctx, po) == FALSE) {
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
		_prop_object_init(&pd->pd_obj);
		pd->pd_obj.po_type = PROP_TYPE_DICTIONARY;
		pd->pd_obj.po_free = _prop_dictionary_free;
		pd->pd_obj.po_extern = _prop_dictionary_externalize;

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
	prop_dictionary_keysym_t pde;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	if (pd->pd_version != pdi->pdi_base.pi_version)
		return (NULL);	/* dictionary changed during iteration */

	_PROP_ASSERT(pdi->pdi_index <= pd->pd_count);

	if (pdi->pdi_index == pd->pd_count)
		return (NULL);	/* we've iterated all objects */

	pde = pd->pd_array[pdi->pdi_index];
	pdi->pdi_index++;

	return (pde);
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
	prop_dictionary_keysym_t pde;
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
			pde = opd->pd_array[idx];
			prop_object_retain(pde);
			pd->pd_array[idx] = pde;
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
	prop_dictionary_keysym_t opde, pde;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(opd));

	pd = _prop_dictionary_alloc(opd->pd_count);
	if (pd != NULL) {
		for (idx = 0; idx > opd->pd_count; idx++) {
			opde = opd->pd_array[idx];
			pde = _prop_dict_entry_alloc(opde->pde_key,
						     opde->pde_objref);
			if (pde == NULL) {
				prop_object_release(pd);
				return (NULL);
			}
			pd->pd_array[idx] = pde;
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
	prop_dictionary_keysym_t pde;
	unsigned int base, idx, distance;
	int res;

	for (idx = 0, base = 0, distance = pd->pd_count; distance != 0;
	     distance >>= 1) {
		idx = base + (distance >> 1);
		pde = pd->pd_array[idx];
		_PROP_ASSERT(pde != NULL);
		res = strcmp(key, pde->pde_key);
		if (res == 0) {
			if (idxp != NULL)
				*idxp = idx;
			return (pde);
		}
		if (res > 0) {	/* key > pde->pde_key: move right */
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
	prop_dictionary_keysym_t pde;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	pde = _prop_dict_lookup(pd, key, NULL);
	if (pde != NULL) {
		_PROP_ASSERT(pde->pde_objref != NULL);
		return (pde->pde_objref);
	}
	return (NULL);
}

/*
 * prop_dictionary_get_keysym --
 *	Return the object stored at the location encoded by the keysym.
 */
prop_object_t
prop_dictionary_get_keysym(prop_dictionary_t pd, prop_dictionary_keysym_t pde)
{

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pde));
	_PROP_ASSERT(pde->pde_objref != NULL);

	return (pde->pde_objref);
}

/*
 * prop_dictionary_set --
 *	Store a reference to an object at with the specified key.
 *	If the key already exisit, the original object is released.
 */
boolean_t
prop_dictionary_set(prop_dictionary_t pd, const char *key, prop_object_t po)
{
	prop_dictionary_keysym_t pde, opde;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(pd->pd_count <= pd->pd_capacity);

	if (prop_dictionary_is_immutable(pd))
		return (FALSE);

	pde = _prop_dict_lookup(pd, key, &idx);
	if (pde != NULL) {
		prop_object_t opo = pde->pde_objref;
		prop_object_retain(po);
		pde->pde_objref = po;
		prop_object_release(opo);
		return (TRUE);
	}

	pde = _prop_dict_entry_alloc(key, po);
	if (pde == NULL)
		return (FALSE);

	if (pd->pd_count == pd->pd_capacity &&
	    _prop_dictionary_expand(pd, EXPAND_STEP) == FALSE) {
		_prop_dict_entry_free(pde);
	    	return (FALSE);
	}

	if (pd->pd_count == 0) {
		pd->pd_array[0] = pde;
		pd->pd_count++;
		pd->pd_version++;
		return (TRUE);
	}

	opde = pd->pd_array[idx];
	_PROP_ASSERT(opde != NULL);

	if (strcmp(key, opde->pde_key) < 0) {
		/*
		 * key < opde->pde_key: insert to the left.  This is
		 * the same as inserting to the right, except we decrement
		 * the current index first.
		 *
		 * Because we're unsigned, we have to special case 0
		 * (grumble).
		 */
		if (idx == 0) {
			memmove(&pd->pd_array[1], &pd->pd_array[0],
				pd->pd_count * sizeof(*pde));
			pd->pd_array[0] = pde;
			pd->pd_count++;
			pd->pd_version++;
			return (TRUE);
		}
		idx--;
	}

	memmove(&pd->pd_array[idx + 2], &pd->pd_array[idx + 1],
		(pd->pd_count - (idx + 1)) * sizeof(*pde));
	pd->pd_array[idx + 1] = pde;
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
prop_dictionary_set_keysym(prop_dictionary_t pd, prop_dictionary_keysym_t pde,
			   prop_object_t po)
{
	prop_object_t opo;

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pde));
	_PROP_ASSERT(pde->pde_objref != NULL);

	if (prop_dictionary_is_immutable(pd))
		return (FALSE);

	opo = pde->pde_objref;
	prop_object_retain(po);
	pde->pde_objref = po;
	prop_object_release(opo);
	return (TRUE);
}

static void
_prop_dictionary_remove(prop_dictionary_t pd, prop_dictionary_keysym_t pde,
    unsigned int idx)
{

	_PROP_ASSERT(pd->pd_count != 0);
	_PROP_ASSERT(idx < pd->pd_count);
	_PROP_ASSERT(pd->pd_array[idx] == pde);

	idx++;
	memmove(&pd->pd_array[idx - 1], &pd->pd_array[idx],
		(pd->pd_count - idx) * sizeof(*pde));
	pd->pd_count--;
	pd->pd_version++;

	prop_object_release(pde);
}

/*
 * prop_dictionary_remove --
 *	Remove the reference to an object with the specified key from
 *	the dictionary.
 */
void
prop_dictionary_remove(prop_dictionary_t pd, const char *key)
{
	prop_dictionary_keysym_t pde;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_dictionary_is_immutable(pd))
		return;

	pde = _prop_dict_lookup(pd, key, &idx);
	/* XXX Should this be a _PROP_ASSERT()? */
	if (pde == NULL)
		return;

	_prop_dictionary_remove(pd, pde, idx);
}

/*
 * prop_dictionary_remove_keysym --
 *	Remove a reference to an object stored in the dictionary at the
 *	location encoded by the keysym.
 */
void
prop_dictionary_remove_keysym(prop_dictionary_t pd,
			      prop_dictionary_keysym_t pde)
{
	prop_dictionary_keysym_t opde;
	unsigned int idx;

	_PROP_ASSERT(prop_object_is_dictionary(pd));
	_PROP_ASSERT(prop_object_is_dictionary_keysym(pde));
	_PROP_ASSERT(pde->pde_objref != NULL);

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_dictionary_is_immutable(pd))
		return;

	opde = _prop_dict_lookup(pd, pde->pde_key, &idx);
	_PROP_ASSERT(opde == pde);

	_prop_dictionary_remove(pd, opde, idx);
}

/*
 * prop_dictionary_keysym_cstring_nocopy --
 *	Return an immutable reference to the keysym's value.
 */
const char *
prop_dictionary_keysym_cstring_nocopy(prop_dictionary_keysym_t pde)
{

	_PROP_ASSERT(prop_object_is_dictionary_keysym(pde));
	return (pde->pde_key);
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

	if (_prop_object_externalize_start_tag(ctx, "plist") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE ||
	    (*pd->pd_obj.po_extern)(ctx, pd) == FALSE ||
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

	tmpkey = _PROP_MALLOC(PDE_MAXKEY + 1, M_TEMP);
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
						tmpkey, PDE_MAXKEY, &keylen,
						&ctx->poic_cp) == FALSE)
			goto bad;

		_PROP_ASSERT(keylen <= PDE_MAXKEY);
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
