/* Interface to hashtable implementations.
   Copyright (C) 2006-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <ctf-impl.h>
#include <string.h>
#include "libiberty.h"
#include "hashtab.h"

/* We have two hashtable implementations: one, ctf_dynhash_*(), is an interface to
   a dynamically-expanding hash with unknown size that should support addition
   of large numbers of items, and removal as well, and is used only at
   type-insertion time; the other, ctf_dynhash_*(), is an interface to a
   fixed-size hash from const char * -> ctf_id_t with number of elements
   specified at creation time, that should support addition of items but need
   not support removal.  These can be implemented by the same underlying hashmap
   if you wish.  */

typedef struct ctf_helem
{
  void *key;			 /* Either a pointer, or a coerced ctf_id_t.  */
  void *value;			 /* The value (possibly a coerced int).  */
  ctf_hash_free_fun key_free;
  ctf_hash_free_fun value_free;
} ctf_helem_t;

struct ctf_dynhash
{
  struct htab *htab;
  ctf_hash_free_fun key_free;
  ctf_hash_free_fun value_free;
};

/* Hash functions. */

unsigned int
ctf_hash_integer (const void *ptr)
{
  ctf_helem_t *hep = (ctf_helem_t *) ptr;

  return htab_hash_pointer (hep->key);
}

int
ctf_hash_eq_integer (const void *a, const void *b)
{
  ctf_helem_t *hep_a = (ctf_helem_t *) a;
  ctf_helem_t *hep_b = (ctf_helem_t *) b;

  return htab_eq_pointer (hep_a->key, hep_b->key);
}

unsigned int
ctf_hash_string (const void *ptr)
{
  ctf_helem_t *hep = (ctf_helem_t *) ptr;

  return htab_hash_string (hep->key);
}

int
ctf_hash_eq_string (const void *a, const void *b)
{
  ctf_helem_t *hep_a = (ctf_helem_t *) a;
  ctf_helem_t *hep_b = (ctf_helem_t *) b;

  return !strcmp((const char *) hep_a->key, (const char *) hep_b->key);
}

/* Hash a type_mapping_key.  */
unsigned int
ctf_hash_type_mapping_key (const void *ptr)
{
  ctf_helem_t *hep = (ctf_helem_t *) ptr;
  ctf_link_type_mapping_key_t *k = (ctf_link_type_mapping_key_t *) hep->key;

  return htab_hash_pointer (k->cltm_fp) + 59 * htab_hash_pointer ((void *) k->cltm_idx);
}

int
ctf_hash_eq_type_mapping_key (const void *a, const void *b)
{
  ctf_helem_t *hep_a = (ctf_helem_t *) a;
  ctf_helem_t *hep_b = (ctf_helem_t *) b;
  ctf_link_type_mapping_key_t *key_a = (ctf_link_type_mapping_key_t *) hep_a->key;
  ctf_link_type_mapping_key_t *key_b = (ctf_link_type_mapping_key_t *) hep_b->key;

  return (key_a->cltm_fp == key_b->cltm_fp)
    && (key_a->cltm_idx == key_b->cltm_idx);
}

/* The dynhash, used for hashes whose size is not known at creation time. */

/* Free a single ctf_helem.  */

static void
ctf_dynhash_item_free (void *item)
{
  ctf_helem_t *helem = item;

  if (helem->key_free && helem->key)
    helem->key_free (helem->key);
  if (helem->value_free && helem->value)
    helem->value_free (helem->value);
  free (helem);
}

ctf_dynhash_t *
ctf_dynhash_create (ctf_hash_fun hash_fun, ctf_hash_eq_fun eq_fun,
                    ctf_hash_free_fun key_free, ctf_hash_free_fun value_free)
{
  ctf_dynhash_t *dynhash;

  dynhash = malloc (sizeof (ctf_dynhash_t));
  if (!dynhash)
    return NULL;

  /* 7 is arbitrary and untested for now..  */
  if ((dynhash->htab = htab_create_alloc (7, (htab_hash) hash_fun, eq_fun,
                                          ctf_dynhash_item_free, xcalloc, free)) == NULL)
    {
      free (dynhash);
      return NULL;
    }

  dynhash->key_free = key_free;
  dynhash->value_free = value_free;

  return dynhash;
}

static ctf_helem_t **
ctf_hashtab_lookup (struct htab *htab, const void *key, enum insert_option insert)
{
  ctf_helem_t tmp = { .key = (void *) key };
  return (ctf_helem_t **) htab_find_slot (htab, &tmp, insert);
}

static ctf_helem_t *
ctf_hashtab_insert (struct htab *htab, void *key, void *value,
		    ctf_hash_free_fun key_free,
		    ctf_hash_free_fun value_free)
{
  ctf_helem_t **slot;

  slot = ctf_hashtab_lookup (htab, key, INSERT);

  if (!slot)
    {
      errno = -ENOMEM;
      return NULL;
    }

  if (!*slot)
    {
      *slot = malloc (sizeof (ctf_helem_t));
      if (!*slot)
	return NULL;
    }
  else
    {
      if (key_free)
	  key_free ((*slot)->key);
      if (value_free)
	  value_free ((*slot)->value);
    }
  (*slot)->key = key;
  (*slot)->value = value;
  return *slot;
}

int
ctf_dynhash_insert (ctf_dynhash_t *hp, void *key, void *value)
{
  ctf_helem_t *slot;

  slot = ctf_hashtab_insert (hp->htab, key, value,
			     hp->key_free, hp->value_free);

  if (!slot)
    return errno;

  /* We need to keep the key_free and value_free around in each item because the
     del function has no visibility into the hash as a whole, only into the
     individual items.  */

  slot->key_free = hp->key_free;
  slot->value_free = hp->value_free;

  return 0;
}

void
ctf_dynhash_remove (ctf_dynhash_t *hp, const void *key)
{
  ctf_helem_t hep = { (void *) key, NULL, NULL, NULL };
  htab_remove_elt (hp->htab, &hep);
}

void
ctf_dynhash_empty (ctf_dynhash_t *hp)
{
  htab_empty (hp->htab);
}

void *
ctf_dynhash_lookup (ctf_dynhash_t *hp, const void *key)
{
  ctf_helem_t **slot;

  slot = ctf_hashtab_lookup (hp->htab, key, NO_INSERT);

  if (slot)
    return (*slot)->value;

  return NULL;
}

typedef struct ctf_traverse_cb_arg
{
  ctf_hash_iter_f fun;
  void *arg;
} ctf_traverse_cb_arg_t;

static int
ctf_hashtab_traverse (void **slot, void *arg_)
{
  ctf_helem_t *helem = *((ctf_helem_t **) slot);
  ctf_traverse_cb_arg_t *arg = (ctf_traverse_cb_arg_t *) arg_;

  arg->fun (helem->key, helem->value, arg->arg);
  return 1;
}

void
ctf_dynhash_iter (ctf_dynhash_t *hp, ctf_hash_iter_f fun, void *arg_)
{
  ctf_traverse_cb_arg_t arg = { fun, arg_ };
  htab_traverse (hp->htab, ctf_hashtab_traverse, &arg);
}

typedef struct ctf_traverse_remove_cb_arg
{
  struct htab *htab;
  ctf_hash_iter_remove_f fun;
  void *arg;
} ctf_traverse_remove_cb_arg_t;

static int
ctf_hashtab_traverse_remove (void **slot, void *arg_)
{
  ctf_helem_t *helem = *((ctf_helem_t **) slot);
  ctf_traverse_remove_cb_arg_t *arg = (ctf_traverse_remove_cb_arg_t *) arg_;

  if (arg->fun (helem->key, helem->value, arg->arg))
    htab_clear_slot (arg->htab, slot);
  return 1;
}

void
ctf_dynhash_iter_remove (ctf_dynhash_t *hp, ctf_hash_iter_remove_f fun,
                         void *arg_)
{
  ctf_traverse_remove_cb_arg_t arg = { hp->htab, fun, arg_ };
  htab_traverse (hp->htab, ctf_hashtab_traverse_remove, &arg);
}

void
ctf_dynhash_destroy (ctf_dynhash_t *hp)
{
  if (hp != NULL)
    htab_delete (hp->htab);
  free (hp);
}

/* ctf_hash, used for fixed-size maps from const char * -> ctf_id_t without
   removal.  This is a straight cast of a hashtab.  */

ctf_hash_t *
ctf_hash_create (unsigned long nelems, ctf_hash_fun hash_fun,
		 ctf_hash_eq_fun eq_fun)
{
  return (ctf_hash_t *) htab_create_alloc (nelems, (htab_hash) hash_fun,
					   eq_fun, free, xcalloc, free);
}

uint32_t
ctf_hash_size (const ctf_hash_t *hp)
{
  return htab_elements ((struct htab *) hp);
}

int
ctf_hash_insert_type (ctf_hash_t *hp, ctf_file_t *fp, uint32_t type,
		      uint32_t name)
{
  const char *str = ctf_strraw (fp, name);

  if (type == 0)
    return EINVAL;

  if (str == NULL
      && CTF_NAME_STID (name) == CTF_STRTAB_1
      && fp->ctf_syn_ext_strtab == NULL
      && fp->ctf_str[CTF_NAME_STID (name)].cts_strs == NULL)
    return ECTF_STRTAB;

  if (str == NULL)
    return ECTF_BADNAME;

  if (str[0] == '\0')
    return 0;		   /* Just ignore empty strings on behalf of caller.  */

  if (ctf_hashtab_insert ((struct htab *) hp, (char *) str,
			  (void *) (ptrdiff_t) type, NULL, NULL) != NULL)
    return 0;
  return errno;
}

/* if the key is already in the hash, override the previous definition with
   this new official definition. If the key is not present, then call
   ctf_hash_insert_type() and hash it in.  */
int
ctf_hash_define_type (ctf_hash_t *hp, ctf_file_t *fp, uint32_t type,
                      uint32_t name)
{
  /* This matches the semantics of ctf_hash_insert_type() in this
     implementation anyway.  */

  return ctf_hash_insert_type (hp, fp, type, name);
}

ctf_id_t
ctf_hash_lookup_type (ctf_hash_t *hp, ctf_file_t *fp __attribute__ ((__unused__)),
		      const char *key)
{
  ctf_helem_t **slot;

  slot = ctf_hashtab_lookup ((struct htab *) hp, key, NO_INSERT);

  if (slot)
    return (ctf_id_t) ((*slot)->value);

  return 0;
}

void
ctf_hash_destroy (ctf_hash_t *hp)
{
  if (hp != NULL)
    htab_delete ((struct htab *) hp);
}
