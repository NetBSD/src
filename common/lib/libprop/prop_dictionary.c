/*	$NetBSD: prop_dictionary.c,v 1.17 2007/07/16 19:20:17 joerg Exp $	*/

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
#include <prop/prop_dictionary.h>
#include <prop/prop_string.h>
#include "prop_object_impl.h"
#include "prop_rb_impl.h"

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <errno.h>
#endif

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
	size_t				pdk_size;
	struct rb_node			pdk_link;
	char 				pdk_key[1];
	/* actually variable length */
};

#define	RBNODE_TO_PDK(n)						\
	((struct _prop_dictionary_keysym *)				\
	 ((uintptr_t)n - offsetof(struct _prop_dictionary_keysym, pdk_link)))

	/* pdk_key[1] takes care of the NUL */
#define	PDK_SIZE_16		(sizeof(struct _prop_dictionary_keysym) + 16)
#define	PDK_SIZE_32		(sizeof(struct _prop_dictionary_keysym) + 32)
#define	PDK_SIZE_128		(sizeof(struct _prop_dictionary_keysym) + 128)

#define	PDK_MAXKEY		128

_PROP_POOL_INIT(_prop_dictionary_keysym16_pool, PDK_SIZE_16, "pdict16")
_PROP_POOL_INIT(_prop_dictionary_keysym32_pool, PDK_SIZE_32, "pdict32")
_PROP_POOL_INIT(_prop_dictionary_keysym128_pool, PDK_SIZE_128, "pdict128")

struct _prop_dict_entry {
	prop_dictionary_keysym_t	pde_key;
	prop_object_t			pde_objref;
};

struct _prop_dictionary {
	struct _prop_object	pd_obj;
	_PROP_RWLOCK_DECL(pd_rwlock)
	struct _prop_dict_entry	*pd_array;
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

static void		_prop_dict_keysym_free(void *);
static boolean_t	_prop_dict_keysym_externalize(
				struct _prop_object_externalize_context *,
				void *);
static boolean_t	_prop_dict_keysym_equals(void *, void *);

static const struct _prop_object_type _prop_object_type_dict_keysym = {
	.pot_type	=	PROP_TYPE_DICT_KEYSYM,
	.pot_free	=	_prop_dict_keysym_free,
	.pot_extern	=	_prop_dict_keysym_externalize,
	.pot_equals	=	_prop_dict_keysym_equals,
};

#define	prop_object_is_dictionary(x)		\
	((x) != NULL && (x)->pd_obj.po_type == &_prop_object_type_dictionary)
#define	prop_object_is_dictionary_keysym(x)	\
	((x) != NULL && (x)->pdk_obj.po_type == &_prop_object_type_dict_keysym)

#define	prop_dictionary_is_immutable(x)		\
				(((x)->pd_flags & PD_F_IMMUTABLE) != 0)

struct _prop_dictionary_iterator {
	struct _prop_object_iterator pdi_base;
	unsigned int		pdi_index;
};

/*
 * Dictionary key symbols are immutable, and we are likely to have many
 * duplicated key symbols.  So, to save memory, we unique'ify key symbols
 * so we only have to have one copy of each string.
 */

static int
_prop_dict_keysym_rb_compare_nodes(const struct rb_node *n1,
				   const struct rb_node *n2)
{
	const prop_dictionary_keysym_t pdk1 = RBNODE_TO_PDK(n1);
	const prop_dictionary_keysym_t pdk2 = RBNODE_TO_PDK(n2);

	return (strcmp(pdk1->pdk_key, pdk2->pdk_key));
}

static int
_prop_dict_keysym_rb_compare_key(const struct rb_node *n,
				 const void *v)
{
	const prop_dictionary_keysym_t pdk = RBNODE_TO_PDK(n);
	const char *cp = v;

	return (strcmp(pdk->pdk_key, cp));
}

static const struct rb_tree_ops _prop_dict_keysym_rb_tree_ops = {
	.rbto_compare_nodes = _prop_dict_keysym_rb_compare_nodes,
	.rbto_compare_key   = _prop_dict_keysym_rb_compare_key,
};

static struct rb_tree _prop_dict_keysym_tree;
static boolean_t _prop_dict_keysym_tree_initialized;

_PROP_MUTEX_DECL_STATIC(_prop_dict_keysym_tree_mutex)

static void
_prop_dict_keysym_put(prop_dictionary_keysym_t pdk)
{

	if (pdk->pdk_size <= PDK_SIZE_16)
		_PROP_POOL_PUT(_prop_dictionary_keysym16_pool, pdk);
	else if (pdk->pdk_size <= PDK_SIZE_32)
		_PROP_POOL_PUT(_prop_dictionary_keysym32_pool, pdk);
	else {
		_PROP_ASSERT(pdk->pdk_size <= PDK_SIZE_128);
		_PROP_POOL_PUT(_prop_dictionary_keysym128_pool, pdk);
	}
}

static void
_prop_dict_keysym_free(void *v)
{
	prop_dictionary_keysym_t pdk = v;

	_PROP_MUTEX_LOCK(_prop_dict_keysym_tree_mutex);
	_prop_rb_tree_remove_node(&_prop_dict_keysym_tree, &pdk->pdk_link);
	_PROP_MUTEX_UNLOCK(_prop_dict_keysym_tree_mutex);

	_prop_dict_keysym_put(pdk);
}

static boolean_t
_prop_dict_keysym_externalize(struct _prop_object_externalize_context *ctx,
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
_prop_dict_keysym_equals(void *v1, void *v2)
{
	prop_dictionary_keysym_t pdk1 = v1;
	prop_dictionary_keysym_t pdk2 = v2;

	if (! (prop_object_is_dictionary_keysym(pdk1) &&
	       prop_object_is_dictionary_keysym(pdk2)))
		return (FALSE);

	/*
	 * There is only ever one copy of a keysym at any given time,
	 * so we can reduce this to a simple pointer equality check.
	 */
	return (pdk1 == pdk2);
}

static prop_dictionary_keysym_t
_prop_dict_keysym_alloc(const char *key)
{
	prop_dictionary_keysym_t opdk, pdk;
	const struct rb_node *n;
	size_t size;

	/*
	 * Check to see if this already exists in the tree.  If it does,
	 * we just retain it and return it.
	 */
	_PROP_MUTEX_LOCK(_prop_dict_keysym_tree_mutex);
	if (! _prop_dict_keysym_tree_initialized) {
		_prop_rb_tree_init(&_prop_dict_keysym_tree,
				   &_prop_dict_keysym_rb_tree_ops);
		_prop_dict_keysym_tree_initialized = TRUE;
	} else {
		n = _prop_rb_tree_find(&_prop_dict_keysym_tree, key);
		if (n != NULL) {
			opdk = RBNODE_TO_PDK(n);
			prop_object_retain(opdk);
			_PROP_MUTEX_UNLOCK(_prop_dict_keysym_tree_mutex);
			return (opdk);
		}
	}
	_PROP_MUTEX_UNLOCK(_prop_dict_keysym_tree_mutex);

	/*
	 * Not in the tree.  Create it now.
	 */

	size = sizeof(*pdk) + strlen(key) /* pdk_key[1] covers the NUL */;

	if (size <= PDK_SIZE_16)
		pdk = _PROP_POOL_GET(_prop_dictionary_keysym16_pool);
	else if (size <= PDK_SIZE_32)
		pdk = _PROP_POOL_GET(_prop_dictionary_keysym32_pool);
	else if (size <= PDK_SIZE_128)
		pdk = _PROP_POOL_GET(_prop_dictionary_keysym128_pool);
	else
		pdk = NULL;	/* key too long */

	if (pdk == NULL)
		return (NULL);

	_prop_object_init(&pdk->pdk_obj, &_prop_object_type_dict_keysym);

	strcpy(pdk->pdk_key, key);
	pdk->pdk_size = size;

	/*
	 * We dropped the mutex when we allocated the new object, so
	 * we have to check again if it is in the tree.
	 */
	_PROP_MUTEX_LOCK(_prop_dict_keysym_tree_mutex);
	n = _prop_rb_tree_find(&_prop_dict_keysym_tree, key);
	if (n != NULL) {
		opdk = RBNODE_TO_PDK(n);
		prop_object_retain(opdk);
		_PROP_MUTEX_UNLOCK(_prop_dict_keysym_tree_mutex);
		_prop_dict_keysym_put(pdk);
		return (opdk);
	}
	_prop_rb_tree_insert_node(&_prop_dict_keysym_tree, &pdk->pdk_link);
	_PROP_MUTEX_UNLOCK(_prop_dict_keysym_tree_mutex);
	return (pdk);
}

static void
_prop_dictionary_free(void *v)
{
	prop_dictionary_t pd = v;
	prop_dictionary_keysym_t pdk;
	prop_object_t po;
	unsigned int idx;

	_PROP_ASSERT(pd->pd_count <= pd->pd_capacity);
	_PROP_ASSERT((pd->pd_capacity == 0 && pd->pd_array == NULL) ||
		     (pd->pd_capacity != 0 && pd->pd_array != NULL));

	for (idx = 0; idx < pd->pd_count; idx++) {
		pdk = pd->pd_array[idx].pde_key;
		_PROP_ASSERT(pdk != NULL);
		prop_object_release(pdk);
		po = pd->pd_array[idx].pde_objref;
		_PROP_ASSERT(po != NULL);
		prop_object_release(po);
	}

	if (pd->pd_array != NULL)
		_PROP_FREE(pd->pd_array, M_PROP_DICT);

	_PROP_RWLOCK_DESTROY(pd->pd_rwlock);

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
	boolean_t rv = FALSE;

	_PROP_RWLOCK_RDLOCK(pd->pd_rwlock);

	if (pd->pd_count == 0) {
		_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
		return (_prop_object_externalize_empty_tag(ctx, "dict"));
	}

	if (_prop_object_externalize_start_tag(ctx, "dict") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE)
		goto out;

	pi = prop_dictionary_iterator(pd);
	if (pi == NULL)
		goto out;
	
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
			goto out;
		}
	}

	prop_object_iterator_release(pi);

	ctx->poec_depth--;
	for (i = 0; i < ctx->poec_depth; i++) {
		if (_prop_object_externalize_append_char(ctx, '\t') == FALSE)
			goto out;
	}
	if (_prop_object_externalize_end_tag(ctx, "dict") == FALSE)
		goto out;
	
	rv = TRUE;

 out:
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
	return (rv);
}

static boolean_t
_prop_dictionary_equals(void *v1, void *v2)
{
	prop_dictionary_t dict1 = v1;
	prop_dictionary_t dict2 = v2;
	const struct _prop_dict_entry *pde1, *pde2;
	unsigned int idx;
	boolean_t rv = FALSE;

	if (! (prop_object_is_dictionary(dict1) &&
	       prop_object_is_dictionary(dict2)))
		return (FALSE);

	if (dict1 == dict2)
		return (TRUE);

	if ((uintptr_t)dict1 < (uintptr_t)dict2) {
		_PROP_RWLOCK_RDLOCK(dict1->pd_rwlock);
		_PROP_RWLOCK_RDLOCK(dict2->pd_rwlock);
	} else {
		_PROP_RWLOCK_RDLOCK(dict2->pd_rwlock);
		_PROP_RWLOCK_RDLOCK(dict1->pd_rwlock);
	}

	if (dict1->pd_count != dict2->pd_count)
		goto out;

	for (idx = 0; idx < dict1->pd_count; idx++) {
		pde1 = &dict1->pd_array[idx];
		pde2 = &dict2->pd_array[idx];

		if (prop_dictionary_keysym_equals(pde1->pde_key,
						  pde2->pde_key) == FALSE)
			goto out;
		if (prop_object_equals(pde1->pde_objref,
				       pde2->pde_objref) == FALSE)
			goto out;
	}

	rv = TRUE;

 out:
 	_PROP_RWLOCK_UNLOCK(dict1->pd_rwlock);
	_PROP_RWLOCK_UNLOCK(dict2->pd_rwlock);
	return (rv);
}

static prop_dictionary_t
_prop_dictionary_alloc(unsigned int capacity)
{
	prop_dictionary_t pd;
	struct _prop_dict_entry *array;

	if (capacity != 0) {
		array = _PROP_CALLOC(capacity * sizeof(*array), M_PROP_DICT);
		if (array == NULL)
			return (NULL);
	} else
		array = NULL;

	pd = _PROP_POOL_GET(_prop_dictionary_pool);
	if (pd != NULL) {
		_prop_object_init(&pd->pd_obj, &_prop_object_type_dictionary);

		_PROP_RWLOCK_INIT(pd->pd_rwlock);
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
	struct _prop_dict_entry *array, *oarray;

	/*
	 * Dictionary must be WRITE-LOCKED.
	 */

	oarray = pd->pd_array;

	array = _PROP_CALLOC(capacity * sizeof(*array), M_PROP_DICT);
	if (array == NULL)
		return (FALSE);
	if (oarray != NULL)
		memcpy(array, oarray, pd->pd_capacity * sizeof(*array));
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
	prop_dictionary_keysym_t pdk = NULL;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	_PROP_RWLOCK_RDLOCK(pd->pd_rwlock);

	if (pd->pd_version != pdi->pdi_base.pi_version)
		goto out;	/* dictionary changed during iteration */

	_PROP_ASSERT(pdi->pdi_index <= pd->pd_count);

	if (pdi->pdi_index == pd->pd_count)
		goto out;	/* we've iterated all objects */

	pdk = pd->pd_array[pdi->pdi_index].pde_key;
	pdi->pdi_index++;

 out:
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
	return (pdk);
}

static void
_prop_dictionary_iterator_reset(void *v)
{
	struct _prop_dictionary_iterator *pdi = v;
	prop_dictionary_t pd = pdi->pdi_base.pi_obj;

	_PROP_ASSERT(prop_object_is_dictionary(pd));

	_PROP_RWLOCK_RDLOCK(pd->pd_rwlock);

	pdi->pdi_index = 0;
	pdi->pdi_base.pi_version = pd->pd_version;

	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
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
 *	Copy a dictionary.  The new dictionary has an initial capacity equal
 *	to the number of objects stored int the original dictionary.  The new
 *	dictionary contains refrences to the original dictionary's objects,
 *	not copies of those objects (i.e. a shallow copy).
 */
prop_dictionary_t
prop_dictionary_copy(prop_dictionary_t opd)
{
	prop_dictionary_t pd;
	prop_dictionary_keysym_t pdk;
	prop_object_t po;
	unsigned int idx;

	if (! prop_object_is_dictionary(opd))
		return (NULL);

	_PROP_RWLOCK_RDLOCK(opd->pd_rwlock);

	pd = _prop_dictionary_alloc(opd->pd_count);
	if (pd != NULL) {
		for (idx = 0; idx < opd->pd_count; idx++) {
			pdk = opd->pd_array[idx].pde_key;
			po = opd->pd_array[idx].pde_objref;

			prop_object_retain(pdk);
			prop_object_retain(po);

			pd->pd_array[idx].pde_key = pdk;
			pd->pd_array[idx].pde_objref = po;
		}
		pd->pd_count = opd->pd_count;
		pd->pd_flags = opd->pd_flags;
	}
	_PROP_RWLOCK_UNLOCK(opd->pd_rwlock);
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

	if (! prop_object_is_dictionary(opd))
		return (NULL);

	pd = prop_dictionary_copy(opd);
	if (pd != NULL)
		pd->pd_flags &= ~PD_F_IMMUTABLE;

	return (pd);
}

/*
 * prop_dictionary_count --
 *	Return the number of objects stored in the dictionary.
 */
unsigned int
prop_dictionary_count(prop_dictionary_t pd)
{
	unsigned int rv;

	if (! prop_object_is_dictionary(pd))
		return (0);

	_PROP_RWLOCK_RDLOCK(pd->pd_rwlock);
	rv = pd->pd_count;
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);

	return (rv);
}

/*
 * prop_dictionary_ensure_capacity --
 *	Ensure that the dictionary has the capacity to store the specified
 *	total number of objects (including the objects already stored in
 *	the dictionary).
 */
boolean_t
prop_dictionary_ensure_capacity(prop_dictionary_t pd, unsigned int capacity)
{
	boolean_t rv;

	if (! prop_object_is_dictionary(pd))
		return (FALSE);

	_PROP_RWLOCK_WRLOCK(pd->pd_rwlock);
	if (capacity > pd->pd_capacity)
		rv = _prop_dictionary_expand(pd, capacity);
	else
		rv = TRUE;
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
	return (rv);
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

	if (! prop_object_is_dictionary(pd))
		return (NULL);

	pdi = _PROP_CALLOC(sizeof(*pdi), M_TEMP);
	if (pdi == NULL)
		return (NULL);
	pdi->pdi_base.pi_next_object = _prop_dictionary_iterator_next_object;
	pdi->pdi_base.pi_reset = _prop_dictionary_iterator_reset;
	prop_object_retain(pd);
	pdi->pdi_base.pi_obj = pd;
	_prop_dictionary_iterator_reset(pdi);

	return (&pdi->pdi_base);
}

/*
 * prop_dictionary_all_keys --
 *	Return an array containing a snapshot of all of the keys
 *	in the dictionary.
 */
prop_array_t
prop_dictionary_all_keys(prop_dictionary_t pd)
{
	prop_array_t array;
	unsigned int idx;
	boolean_t rv = TRUE;

	if (! prop_object_is_dictionary(pd))
		return (NULL);

	/* There is no pressing need to lock the dictionary for this. */
	array = prop_array_create_with_capacity(pd->pd_count);

	_PROP_RWLOCK_RDLOCK(pd->pd_rwlock);

	for (idx = 0; idx < pd->pd_count; idx++) {
		rv = prop_array_add(array, pd->pd_array[idx].pde_key);
		if (rv == FALSE)
			break;
	}

	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);

	if (rv == FALSE) {
		prop_object_release(array);
		array = NULL;
	}
	return (array);
}

static struct _prop_dict_entry *
_prop_dict_lookup(prop_dictionary_t pd, const char *key,
		  unsigned int *idxp)
{
	struct _prop_dict_entry *pde;
	unsigned int base, idx, distance;
	int res;

	/*
	 * Dictionary must be READ-LOCKED or WRITE-LOCKED.
	 */

	for (idx = 0, base = 0, distance = pd->pd_count; distance != 0;
	     distance >>= 1) {
		idx = base + (distance >> 1);
		pde = &pd->pd_array[idx];
		_PROP_ASSERT(pde->pde_key != NULL);
		res = strcmp(key, pde->pde_key->pdk_key);
		if (res == 0) {
			if (idxp != NULL)
				*idxp = idx;
			return (pde);
		}
		if (res > 0) {	/* key > pdk_key: move right */
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
	const struct _prop_dict_entry *pde;
	prop_object_t po = NULL;

	if (! prop_object_is_dictionary(pd))
		return (NULL);

	_PROP_RWLOCK_RDLOCK(pd->pd_rwlock);
	pde = _prop_dict_lookup(pd, key, NULL);
	if (pde != NULL) {
		_PROP_ASSERT(pde->pde_objref != NULL);
		po = pde->pde_objref;
	}
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
	return (po);
}

/*
 * prop_dictionary_get_keysym --
 *	Return the object stored at the location encoded by the keysym.
 */
prop_object_t
prop_dictionary_get_keysym(prop_dictionary_t pd, prop_dictionary_keysym_t pdk)
{

	if (! (prop_object_is_dictionary(pd) &&
	       prop_object_is_dictionary_keysym(pdk)))
		return (NULL);

	return (prop_dictionary_get(pd, pdk->pdk_key));
}

/*
 * prop_dictionary_set --
 *	Store a reference to an object at with the specified key.
 *	If the key already exisit, the original object is released.
 */
boolean_t
prop_dictionary_set(prop_dictionary_t pd, const char *key, prop_object_t po)
{
	struct _prop_dict_entry *pde;
	prop_dictionary_keysym_t pdk;
	unsigned int idx;
	boolean_t rv = FALSE;

	if (! prop_object_is_dictionary(pd))
		return (FALSE);

	_PROP_ASSERT(pd->pd_count <= pd->pd_capacity);

	if (prop_dictionary_is_immutable(pd))
		return (FALSE);

	_PROP_RWLOCK_WRLOCK(pd->pd_rwlock);

	pde = _prop_dict_lookup(pd, key, &idx);
	if (pde != NULL) {
		prop_object_t opo = pde->pde_objref;
		prop_object_retain(po);
		pde->pde_objref = po;
		prop_object_release(opo);
		rv = TRUE;
		goto out;
	}

	pdk = _prop_dict_keysym_alloc(key);
	if (pdk == NULL)
		goto out;

	if (pd->pd_count == pd->pd_capacity &&
	    _prop_dictionary_expand(pd,
	    			    pd->pd_capacity + EXPAND_STEP) == FALSE) {
		prop_object_release(pdk);
	    	goto out;
	}

	/* At this point, the store will succeed. */
	prop_object_retain(po);

	if (pd->pd_count == 0) {
		pd->pd_array[0].pde_key = pdk;
		pd->pd_array[0].pde_objref = po;
		pd->pd_count++;
		pd->pd_version++;
		rv = TRUE;
		goto out;
	}

	pde = &pd->pd_array[idx];
	_PROP_ASSERT(pde->pde_key != NULL);

	if (strcmp(key, pde->pde_key->pdk_key) < 0) {
		/*
		 * key < pdk_key: insert to the left.  This is the same as
		 * inserting to the right, except we decrement the current
		 * index first.
		 *
		 * Because we're unsigned, we have to special case 0
		 * (grumble).
		 */
		if (idx == 0) {
			memmove(&pd->pd_array[1], &pd->pd_array[0],
				pd->pd_count * sizeof(*pde));
			pd->pd_array[0].pde_key = pdk;
			pd->pd_array[0].pde_objref = po;
			pd->pd_count++;
			pd->pd_version++;
			rv = TRUE;
			goto out;
		}
		idx--;
	}

	memmove(&pd->pd_array[idx + 2], &pd->pd_array[idx + 1],
		(pd->pd_count - (idx + 1)) * sizeof(*pde));
	pd->pd_array[idx + 1].pde_key = pdk;
	pd->pd_array[idx + 1].pde_objref = po;
	pd->pd_count++;

	pd->pd_version++;

	rv = TRUE;

 out:
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
	return (rv);
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

	if (! (prop_object_is_dictionary(pd) &&
	       prop_object_is_dictionary_keysym(pdk)))
		return (FALSE);

	return (prop_dictionary_set(pd, pdk->pdk_key, po));
}

static void
_prop_dictionary_remove(prop_dictionary_t pd, struct _prop_dict_entry *pde,
    unsigned int idx)
{
	prop_dictionary_keysym_t pdk = pde->pde_key;
	prop_object_t po = pde->pde_objref;

	/*
	 * Dictionary must be WRITE-LOCKED.
	 */

	_PROP_ASSERT(pd->pd_count != 0);
	_PROP_ASSERT(idx < pd->pd_count);
	_PROP_ASSERT(pde == &pd->pd_array[idx]);

	idx++;
	memmove(&pd->pd_array[idx - 1], &pd->pd_array[idx],
		(pd->pd_count - idx) * sizeof(*pde));
	pd->pd_count--;
	pd->pd_version++;

	prop_object_release(pdk);
	prop_object_release(po);
}

/*
 * prop_dictionary_remove --
 *	Remove the reference to an object with the specified key from
 *	the dictionary.
 */
void
prop_dictionary_remove(prop_dictionary_t pd, const char *key)
{
	struct _prop_dict_entry *pde;
	unsigned int idx;

	if (! prop_object_is_dictionary(pd))
		return;

	_PROP_RWLOCK_WRLOCK(pd->pd_rwlock);

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_dictionary_is_immutable(pd))
		goto out;

	pde = _prop_dict_lookup(pd, key, &idx);
	/* XXX Should this be a _PROP_ASSERT()? */
	if (pde == NULL)
		goto out;

	_prop_dictionary_remove(pd, pde, idx);
 out:
	_PROP_RWLOCK_UNLOCK(pd->pd_rwlock);
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

	if (! (prop_object_is_dictionary(pd) &&
	       prop_object_is_dictionary_keysym(pdk)))
		return;

	prop_dictionary_remove(pd, pdk->pdk_key);
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

	if (! prop_object_is_dictionary_keysym(pdk))
		return (NULL);

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

	return (_prop_dict_keysym_equals(pdk1, pdk2));
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

	if (_prop_object_externalize_header(ctx) == FALSE ||
	    (*pd->pd_obj.po_type->pot_extern)(ctx, pd) == FALSE ||
	    _prop_object_externalize_footer(ctx) == FALSE) {
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
	return _prop_generic_internalize(xml, "dict");
}

#if !defined(_KERNEL) && !defined(_STANDALONE)
/*
 * prop_dictionary_externalize_to_file --
 *	Externalize a dictionary to the specified file.
 */
boolean_t
prop_dictionary_externalize_to_file(prop_dictionary_t dict, const char *fname)
{
	char *xml;
	boolean_t rv;
	int save_errno = 0;	/* XXXGCC -Wuninitialized [mips, ...] */

	xml = prop_dictionary_externalize(dict);
	if (xml == NULL)
		return (FALSE);
	rv = _prop_object_externalize_write_file(fname, xml, strlen(xml));
	if (rv == FALSE)
		save_errno = errno;
	_PROP_FREE(xml, M_TEMP);
	if (rv == FALSE)
		errno = save_errno;

	return (rv);
}

/*
 * prop_dictionary_internalize_from_file --
 *	Internalize a dictionary from a file.
 */
prop_dictionary_t
prop_dictionary_internalize_from_file(const char *fname)
{
	struct _prop_object_internalize_mapped_file *mf;
	prop_dictionary_t dict;

	mf = _prop_object_internalize_map_file(fname);
	if (mf == NULL)
		return (NULL);
	dict = prop_dictionary_internalize(mf->poimf_xml);
	_prop_object_internalize_unmap_file(mf);

	return (dict);
}
#endif /* !_KERNEL && !_STANDALONE */
