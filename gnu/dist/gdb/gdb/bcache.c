/* Implement a cached obstack.
   Written by Fred Fish <fnf@cygnus.com>
   Rewritten by Jim Blandy <jimb@cygnus.com>

   Copyright 1999, 2000, 2002 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdb_obstack.h"
#include "bcache.h"
#include "gdb_string.h"		/* For memcpy declaration */

#include <stddef.h>
#include <stdlib.h>

/* The type used to hold a single bcache string.  The user data is
   stored in d.data.  Since it can be any type, it needs to have the
   same alignment as the most strict alignment of any type on the host
   machine.  I don't know of any really correct way to do this in
   stock ANSI C, so just do it the same way obstack.h does.  */

struct bstring
{
  struct bstring *next;
  size_t length;

  union
  {
    char data[1];
    double dummy;
  }
  d;
};


/* The structure for a bcache itself.  The bcache is initialized, in
   bcache_xmalloc(), by filling it with zeros and then setting the
   corresponding obstack's malloc() and free() methods.  */

struct bcache
{
  /* All the bstrings are allocated here.  */
  struct obstack cache;

  /* How many hash buckets we're using.  */
  unsigned int num_buckets;
  
  /* Hash buckets.  This table is allocated using malloc, so when we
     grow the table we can return the old table to the system.  */
  struct bstring **bucket;

  /* Statistics.  */
  unsigned long unique_count;	/* number of unique strings */
  long total_count;	/* total number of strings cached, including dups */
  long unique_size;	/* size of unique strings, in bytes */
  long total_size;      /* total number of bytes cached, including dups */
  long structure_size;	/* total size of bcache, including infrastructure */
};

/* The old hash function was stolen from SDBM. This is what DB 3.0 uses now,
 * and is better than the old one. 
 */

unsigned long
hash(const void *addr, int length)
{
		const unsigned char *k, *e;
		unsigned long h;
		
		k = (const unsigned char *)addr;
		e = k+length;
		for (h=0; k< e;++k)
		{
				h *=16777619;
				h ^= *k;
		}
		return (h);
}

/* Growing the bcache's hash table.  */

/* If the average chain length grows beyond this, then we want to
   resize our hash table.  */
#define CHAIN_LENGTH_THRESHOLD (5)

static void
expand_hash_table (struct bcache *bcache)
{
  /* A table of good hash table sizes.  Whenever we grow, we pick the
     next larger size from this table.  sizes[i] is close to 1 << (i+10),
     so we roughly double the table size each time.  After we fall off 
     the end of this table, we just double.  Don't laugh --- there have
     been executables sighted with a gigabyte of debug info.  */
  static unsigned long sizes[] = { 
    1021, 2053, 4099, 8191, 16381, 32771,
    65537, 131071, 262144, 524287, 1048573, 2097143,
    4194301, 8388617, 16777213, 33554467, 67108859, 134217757,
    268435459, 536870923, 1073741827, 2147483659UL
  };
  unsigned int new_num_buckets;
  struct bstring **new_buckets;
  unsigned int i;

  /* Find the next size.  */
  new_num_buckets = bcache->num_buckets * 2;
  for (i = 0; i < (sizeof (sizes) / sizeof (sizes[0])); i++)
    if (sizes[i] > bcache->num_buckets)
      {
	new_num_buckets = sizes[i];
	break;
      }

  /* Allocate the new table.  */
  {
    size_t new_size = new_num_buckets * sizeof (new_buckets[0]);
    new_buckets = (struct bstring **) xmalloc (new_size);
    memset (new_buckets, 0, new_size);

    bcache->structure_size -= (bcache->num_buckets
			       * sizeof (bcache->bucket[0]));
    bcache->structure_size += new_size;
  }

  /* Rehash all existing strings.  */
  for (i = 0; i < bcache->num_buckets; i++)
    {
      struct bstring *s, *next;

      for (s = bcache->bucket[i]; s; s = next)
	{
	  struct bstring **new_bucket;
	  next = s->next;

	  new_bucket = &new_buckets[(hash (&s->d.data, s->length)
				     % new_num_buckets)];
	  s->next = *new_bucket;
	  *new_bucket = s;
	}
    }

  /* Plug in the new table.  */
  if (bcache->bucket)
    xfree (bcache->bucket);
  bcache->bucket = new_buckets;
  bcache->num_buckets = new_num_buckets;
}


/* Looking up things in the bcache.  */

/* The number of bytes needed to allocate a struct bstring whose data
   is N bytes long.  */
#define BSTRING_SIZE(n) (offsetof (struct bstring, d.data) + (n))

/* Find a copy of the LENGTH bytes at ADDR in BCACHE.  If BCACHE has
   never seen those bytes before, add a copy of them to BCACHE.  In
   either case, return a pointer to BCACHE's copy of that string.  */
void *
bcache (const void *addr, int length, struct bcache *bcache)
{
  int hash_index;
  struct bstring *s;

  /* If our average chain length is too high, expand the hash table.  */
  if (bcache->unique_count >= bcache->num_buckets * CHAIN_LENGTH_THRESHOLD)
    expand_hash_table (bcache);

  bcache->total_count++;
  bcache->total_size += length;

  hash_index = hash (addr, length) % bcache->num_buckets;

  /* Search the hash bucket for a string identical to the caller's.  */
  for (s = bcache->bucket[hash_index]; s; s = s->next)
    if (s->length == length
	&& ! memcmp (&s->d.data, addr, length))
      return &s->d.data;

  /* The user's string isn't in the list.  Insert it after *ps.  */
  {
    struct bstring *new
      = obstack_alloc (&bcache->cache, BSTRING_SIZE (length));
    memcpy (&new->d.data, addr, length);
    new->length = length;
    new->next = bcache->bucket[hash_index];
    bcache->bucket[hash_index] = new;

    bcache->unique_count++;
    bcache->unique_size += length;
    bcache->structure_size += BSTRING_SIZE (length);

    return &new->d.data;
  }
}


/* Allocating and freeing bcaches.  */

struct bcache *
bcache_xmalloc (void)
{
  /* Allocate the bcache pre-zeroed.  */
  struct bcache *b = XCALLOC (1, struct bcache);
  obstack_specify_allocation (&b->cache, 0, 0, xmalloc, xfree);
  return b;
}

/* Free all the storage associated with BCACHE.  */
void
bcache_xfree (struct bcache *bcache)
{
  if (bcache == NULL)
    return;
  obstack_free (&bcache->cache, 0);
  xfree (bcache->bucket);
  xfree (bcache);
}



/* Printing statistics.  */

static int
compare_ints (const void *ap, const void *bp)
{
  /* Because we know we're comparing two ints which are positive,
     there's no danger of overflow here.  */
  return * (int *) ap - * (int *) bp;
}


static void
print_percentage (int portion, int total)
{
  if (total == 0)
    printf_filtered ("(not applicable)\n");
  else
    printf_filtered ("%3d%%\n", portion * 100 / total);
}


/* Print statistics on BCACHE's memory usage and efficacity at
   eliminating duplication.  NAME should describe the kind of data
   BCACHE holds.  Statistics are printed using `printf_filtered' and
   its ilk.  */
void
print_bcache_statistics (struct bcache *c, char *type)
{
  int occupied_buckets;
  int max_chain_length;
  int median_chain_length;

  /* Count the number of occupied buckets, and measure chain lengths.  */
  {
    unsigned int b;
    int *chain_length
      = (int *) alloca (c->num_buckets * sizeof (*chain_length));

    occupied_buckets = 0;

    for (b = 0; b < c->num_buckets; b++)
      {
	struct bstring *s = c->bucket[b];

	chain_length[b] = 0;

	if (s)
	  {
	    occupied_buckets++;
	    
	    while (s)
	      {
		chain_length[b]++;
		s = s->next;
	      }
	  }
      }

    /* To compute the median, we need the set of chain lengths sorted.  */
    qsort (chain_length, c->num_buckets, sizeof (chain_length[0]),
	   compare_ints);

    if (c->num_buckets > 0)
      {
	max_chain_length = chain_length[c->num_buckets - 1];
	median_chain_length = chain_length[c->num_buckets / 2];
      }
    else
      {
	max_chain_length = 0;
	median_chain_length = 0;
      }
  }

  printf_filtered ("  Cached '%s' statistics:\n", type);
  printf_filtered ("    Total object count:  %ld\n", c->total_count);
  printf_filtered ("    Unique object count: %lu\n", c->unique_count);
  printf_filtered ("    Percentage of duplicates, by count: ");
  print_percentage (c->total_count - c->unique_count, c->total_count);
  printf_filtered ("\n");

  printf_filtered ("    Total object size:   %ld\n", c->total_size);
  printf_filtered ("    Unique object size:  %ld\n", c->unique_size);
  printf_filtered ("    Percentage of duplicates, by size:  ");
  print_percentage (c->total_size - c->unique_size, c->total_size);
  printf_filtered ("\n");

  printf_filtered ("    Total memory used by bcache, including overhead: %ld\n",
		   c->structure_size);
  printf_filtered ("    Percentage memory overhead: ");
  print_percentage (c->structure_size - c->unique_size, c->unique_size);
  printf_filtered ("    Net memory savings:         ");
  print_percentage (c->total_size - c->structure_size, c->total_size);
  printf_filtered ("\n");

  printf_filtered ("    Hash table size:           %3d\n", c->num_buckets);
  printf_filtered ("    Hash table population:     ");
  print_percentage (occupied_buckets, c->num_buckets);
  printf_filtered ("    Median hash chain length:  %3d\n",
		   median_chain_length);
  printf_filtered ("    Average hash chain length: ");
  if (c->num_buckets > 0)
    printf_filtered ("%3lu\n", c->unique_count / c->num_buckets);
  else
    printf_filtered ("(not applicable)\n");
  printf_filtered ("    Maximum hash chain length: %3d\n", max_chain_length);
  printf_filtered ("\n");
}

int
bcache_memory_used (struct bcache *bcache)
{
  return obstack_memory_used (&bcache->cache);
}
