/* hash - implement simple hashing table with string based keys.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, October 1994.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS 
# include <stdlib.h> 
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif 

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <stdio.h>
#include <sys/types.h>

#if HAVE_OBSTACK
# include <obstack.h>
#else
# include "obstack.h"
#endif

#if HAVE_VALUES_H
# include <values.h>
#endif

#include "hash.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

#ifndef BITSPERBYTE
# define BITSPERBYTE 8
#endif

#ifndef	LONGBITS
# define LONGBITS (sizeof (long) * BITSPERBYTE)
#endif

#ifndef bcopy
# define bcopy(S, D, N)	memcpy ((D), (S), (N))
#endif

void *xmalloc PARAMS ((size_t __n));

typedef struct hash_entry
{
  unsigned long used;
  const char *key;
  void *data;
  struct hash_entry *next;
}
hash_entry;

/* Prototypes for local functions.  */
static void insert_entry_2 PARAMS ((hash_table *htab, const char *key,
				    unsigned long hval, size_t idx,
				    void *data));
static size_t lookup PARAMS ((hash_table *htab, const char *key, size_t keylen,
			      unsigned long hval));
static size_t lookup_2 PARAMS ((hash_table *htab, const char *key,
				unsigned long hval));
static unsigned long compute_hashval PARAMS ((const char *key, size_t keylen));
static int is_prime PARAMS ((unsigned long candidate));


int
init_hash (htab, init_size)
     hash_table *htab;
     unsigned long init_size;
{
  /* We need the size to be a prime.  */
  init_size = next_prime (init_size);

  /* Initialize the data structure.  */
  htab->size = init_size;
  htab->filled = 0;
  htab->first = NULL;
  htab->table = (void *) xmalloc ((init_size + 1) * sizeof (hash_entry));
  if (htab->table == NULL)
    return -1;

  memset (htab->table, '\0', (init_size + 1) * sizeof (hash_entry));
  obstack_init (&htab->mem_pool);

  return 0;
}


int
delete_hash (htab)
     hash_table *htab;
{
  free (htab->table);
  obstack_free (&htab->mem_pool, NULL);
  return 0;
}


int
insert_entry (htab, key, keylen, data)
     hash_table *htab;
     const char *key;
     size_t keylen;
     void *data;
{
  unsigned long hval = compute_hashval (key, keylen);
  hash_entry *table = (hash_entry *) htab->table;
  size_t idx = lookup (htab, key, keylen, hval);

  if (table[idx].used)
    /* We don't want to overwrite the old value.  */
    return -1;
  else
    {
      /* An empty bucket has been found.  */
      insert_entry_2 (htab, obstack_copy0 (&htab->mem_pool, key, keylen),
		      hval, idx, data);
      return 0;
    }
}

static void
insert_entry_2 (htab, key, hval, idx, data)
     hash_table *htab;
     const char *key;
     unsigned long hval;
     size_t idx;
     void *data;
{
  hash_entry *table = (hash_entry *) htab->table;

  table[idx].used = hval;
  table[idx].key = key;
  table[idx].data = data;

      /* List the new value in the list.  */
  if ((hash_entry *) htab->first == NULL)
    {
      table[idx].next = &table[idx];
      *(hash_entry **) &htab->first = &table[idx];
    }
  else
    {
      table[idx].next = ((hash_entry *) htab->first)->next;
      ((hash_entry *) htab->first)->next = &table[idx];
      *(hash_entry **) &htab->first = &table[idx];
    }

  ++htab->filled;
  if (100 * htab->filled > 90 * htab->size)
    {
      /* Table is filled more than 90%.  Resize the table.  */
      unsigned long old_size = htab->size;

      htab->size = next_prime (htab->size * 2);
      htab->filled = 0;
      htab->first = NULL;
      htab->table = (void *) xmalloc ((1 + htab->size)
				      * sizeof (hash_entry));
      memset (htab->table, '\0', (1 + htab->size) * sizeof (hash_entry));

      for (idx = 1; idx <= old_size; ++idx)
	if (table[idx].used)
	  insert_entry_2 (htab, table[idx].key, table[idx].used,
			  lookup_2 (htab, table[idx].key, table[idx].used),
			  table[idx].data);

      free (table);
    }
}


int
find_entry (htab, key, keylen, result)
     hash_table *htab;
     const char *key;
     size_t keylen;
     void **result;
{
  hash_entry *table = (hash_entry *) htab->table;
  size_t idx = lookup (htab, key, keylen, compute_hashval (key, keylen));

  if (table[idx].used == 0)
    return -1;

  *result = table[idx].data;
  return 0;
}


int
iterate_table (htab, ptr, key, data)
     hash_table *htab;
     void **ptr;
     const void **key;
     void **data;
{
  if (*ptr == NULL)
    {
      if (htab->first == NULL)
	return -1;
      *ptr = (void *) ((hash_entry *) htab->first)->next;
    }
  else
    {
      if (*ptr == htab->first)
	return -1;
      *ptr = (void *) (((hash_entry *) *ptr)->next);
    }

  *key = ((hash_entry *) *ptr)->key;
  *data = ((hash_entry *) *ptr)->data;
  return 0;
}


static size_t
lookup (htab, key, keylen, hval)
     hash_table *htab;
     const char *key;
     size_t keylen;
     unsigned long hval;
{
  unsigned long hash;
  size_t idx;
  hash_entry *table = (hash_entry *) htab->table;

  /* First hash function: simply take the modul but prevent zero.  */
  hash = 1 + hval % htab->size;

  idx = hash;

  if (table[idx].used)
    {
      if (table[idx].used == hval && table[idx].key[keylen] == '\0'
	  && strncmp (key, table[idx].key, keylen) == 0)
	return idx;

      /* Second hash function as suggested in [Knuth].  */
      hash = 1 + hval % (htab->size - 2);

      do
	{
	  if (idx <= hash)
	    idx = htab->size + idx - hash;
	  else
	    idx -= hash;

	  /* If entry is found use it.  */
	  if (table[idx].used == hval && table[idx].key[keylen] == '\0'
	      && strncmp (key, table[idx].key, keylen) == 0)
	    return idx;
	}
      while (table[idx].used);
    }
  return idx;
}


/* References:
   [Aho,Sethi,Ullman] Compilers: Principles, Techniques and Tools, 1986
   [Knuth]	      The Art of Computer Programming, part3 (6.4) */

static size_t
lookup_2 (htab, key, hval)
     hash_table *htab;
     const char *key;
     unsigned long hval;
{
  unsigned long hash;
  size_t idx;
  hash_entry *table = (hash_entry *) htab->table;

  /* First hash function: simply take the modul but prevent zero.  */
  hash = 1 + hval % htab->size;

  idx = hash;

  if (table[idx].used)
    {
      if (table[idx].used == hval && strcmp (key, table[idx].key) == 0)
	return idx;

      /* Second hash function as suggested in [Knuth].  */
      hash = 1 + hval % (htab->size - 2);

      do
	{
	  if (idx <= hash)
	    idx = htab->size + idx - hash;
	  else
	    idx -= hash;

	  /* If entry is found use it.  */
	  if (table[idx].used == hval && strcmp (key, table[idx].key) == 0)
	    return idx;
	}
      while (table[idx].used);
    }
  return idx;
}


static unsigned long
compute_hashval (key, keylen)
     const char *key;
     size_t keylen;
{
  size_t cnt;
  unsigned long hval, g;

  /* Compute the hash value for the given string.  The algorithm
     is taken from [Aho,Sethi,Ullman].  */
  cnt = 0;
  hval = keylen;
  while (cnt < keylen)
    {
      hval <<= 4;
      hval += key[cnt++];
      g = hval & ((unsigned long) 0xf << (LONGBITS - 4));
      if (g != 0)
	{
	  hval ^= g >> (LONGBITS - 8);
	  hval ^= g;
	}
    }
  return hval != 0 ? hval : ~((unsigned long) 0);
}


unsigned long
next_prime (seed)
     unsigned long seed;
{
  /* Make it definitely odd.  */
  seed |= 1;

  while (!is_prime (seed))
    seed += 2;

  return seed;
}


static int
is_prime (candidate)
     unsigned long candidate;
{
  /* No even number and none less than 10 will be passed here.  */
  unsigned long divn = 3;
  unsigned long sq = divn * divn;

  while (sq < candidate && candidate % divn != 0)
    {
      ++divn;
      sq += 4 * divn;
      ++divn;
    }

  return candidate % divn != 0;
}
