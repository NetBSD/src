/* Implement a cached obstack.
   Written by Fred Fish <fnf@cygnus.com>
   Rewritten by Jim Blandy <jimb@cygnus.com>

   Copyright (C) 1999-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "gdb_obstack.h"
#include "bcache.h"

/* The type used to hold a single bcache string.  The user data is
   stored in d.data.  Since it can be any type, it needs to have the
   same alignment as the most strict alignment of any type on the host
   machine.  I don't know of any really correct way to do this in
   stock ANSI C, so just do it the same way obstack.h does.  */

struct bstring
{
  /* Hash chain.  */
  struct bstring *next;
  /* Assume the data length is no more than 64k.  */
  unsigned short length;
  /* The half hash hack.  This contains the upper 16 bits of the hash
     value and is used as a pre-check when comparing two strings and
     avoids the need to do length or memcmp calls.  It proves to be
     roughly 100% effective.  */
  unsigned short half_hash;

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
  /* Number of times that the hash table is expanded and hence
     re-built, and the corresponding number of times that a string is
     [re]hashed as part of entering it into the expanded table.  The
     total number of hashes can be computed by adding TOTAL_COUNT to
     expand_hash_count.  */
  unsigned long expand_count;
  unsigned long expand_hash_count;
  /* Number of times that the half-hash compare hit (compare the upper
     16 bits of hash values) hit, but the corresponding combined
     length/data compare missed.  */
  unsigned long half_hash_miss_count;

  /* Hash function to be used for this bcache object.  */
  unsigned long (*hash_function)(const void *addr, int length);

  /* Compare function to be used for this bcache object.  */
  int (*compare_function)(const void *, const void *, int length);
};

/* The old hash function was stolen from SDBM. This is what DB 3.0
   uses now, and is better than the old one.  */

unsigned long
hash(const void *addr, int length)
{
  return hash_continue (addr, length, 0);
}

/* Continue the calculation of the hash H at the given address.  */

unsigned long
hash_continue (const void *addr, int length, unsigned long h)
{
  const unsigned char *k, *e;

  k = (const unsigned char *)addr;
  e = k+length;
  for (; k< e;++k)
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

  /* Count the stats.  Every unique item needs to be re-hashed and
     re-entered.  */
  bcache->expand_count++;
  bcache->expand_hash_count += bcache->unique_count;

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

	  new_bucket = &new_buckets[(bcache->hash_function (&s->d.data,
							    s->length)
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
const void *
bcache (const void *addr, int length, struct bcache *cache)
{
  return bcache_full (addr, length, cache, NULL);
}

/* Find a copy of the LENGTH bytes at ADDR in BCACHE.  If BCACHE has
   never seen those bytes before, add a copy of them to BCACHE.  In
   either case, return a pointer to BCACHE's copy of that string.  If
   optional ADDED is not NULL, return 1 in case of new entry or 0 if
   returning an old entry.  */

const void *
bcache_full (const void *addr, int length, struct bcache *bcache, int *added)
{
  unsigned long full_hash;
  unsigned short half_hash;
  int hash_index;
  struct bstring *s;

  if (added)
    *added = 0;

  /* Lazily initialize the obstack.  This can save quite a bit of
     memory in some cases.  */
  if (bcache->total_count == 0)
    {
      /* We could use obstack_specify_allocation here instead, but
	 gdb_obstack.h specifies the allocation/deallocation
	 functions.  */
      obstack_init (&bcache->cache);
    }

  /* If our average chain length is too high, expand the hash table.  */
  if (bcache->unique_count >= bcache->num_buckets * CHAIN_LENGTH_THRESHOLD)
    expand_hash_table (bcache);

  bcache->total_count++;
  bcache->total_size += length;

  full_hash = bcache->hash_function (addr, length);

  half_hash = (full_hash >> 16);
  hash_index = full_hash % bcache->num_buckets;

  /* Search the hash bucket for a string identical to the caller's.
     As a short-circuit first compare the upper part of each hash
     values.  */
  for (s = bcache->bucket[hash_index]; s; s = s->next)
    {
      if (s->half_hash == half_hash)
	{
	  if (s->length == length
	      && bcache->compare_function (&s->d.data, addr, length))
	    return &s->d.data;
	  else
	    bcache->half_hash_miss_count++;
	}
    }

  /* The user's string isn't in the list.  Insert it after *ps.  */
  {
    struct bstring *newobj
      = (struct bstring *) obstack_alloc (&bcache->cache,
					  BSTRING_SIZE (length));

    memcpy (&newobj->d.data, addr, length);
    newobj->length = length;
    newobj->next = bcache->bucket[hash_index];
    newobj->half_hash = half_hash;
    bcache->bucket[hash_index] = newobj;

    bcache->unique_count++;
    bcache->unique_size += length;
    bcache->structure_size += BSTRING_SIZE (length);

    if (added)
      *added = 1;

    return &newobj->d.data;
  }
}


/* Compare the byte string at ADDR1 of lenght LENGHT to the
   string at ADDR2.  Return 1 if they are equal.  */

static int
bcache_compare (const void *addr1, const void *addr2, int length)
{
  return memcmp (addr1, addr2, length) == 0;
}

/* Allocating and freeing bcaches.  */

/* Allocated a bcache.  HASH_FUNCTION and COMPARE_FUNCTION can be used
   to pass in custom hash, and compare functions to be used by this
   bcache.  If HASH_FUNCTION is NULL hash() is used and if
   COMPARE_FUNCTION is NULL memcmp() is used.  */

struct bcache *
bcache_xmalloc (unsigned long (*hash_function)(const void *, int length),
                int (*compare_function)(const void *, 
					const void *, 
					int length))
{
  /* Allocate the bcache pre-zeroed.  */
  struct bcache *b = XCNEW (struct bcache);

  if (hash_function)
    b->hash_function = hash_function;
  else
    b->hash_function = hash;

  if (compare_function)
    b->compare_function = compare_function;
  else
    b->compare_function = bcache_compare;
  return b;
}

/* Free all the storage associated with BCACHE.  */
void
bcache_xfree (struct bcache *bcache)
{
  if (bcache == NULL)
    return;
  /* Only free the obstack if we actually initialized it.  */
  if (bcache->total_count > 0)
    obstack_free (&bcache->cache, 0);
  xfree (bcache->bucket);
  xfree (bcache);
}



/* Printing statistics.  */

static void
print_percentage (int portion, int total)
{
  if (total == 0)
    /* i18n: Like "Percentage of duplicates, by count: (not applicable)".  */
    printf_filtered (_("(not applicable)\n"));
  else
    printf_filtered ("%3d%%\n", (int) (portion * 100.0 / total));
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
  int max_entry_size;
  int median_entry_size;

  /* Count the number of occupied buckets, tally the various string
     lengths, and measure chain lengths.  */
  {
    unsigned int b;
    int *chain_length = XCNEWVEC (int, c->num_buckets + 1);
    int *entry_size = XCNEWVEC (int, c->unique_count + 1);
    int stringi = 0;

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
		gdb_assert (b < c->num_buckets);
		chain_length[b]++;
		gdb_assert (stringi < c->unique_count);
		entry_size[stringi++] = s->length;
		s = s->next;
	      }
	  }
      }

    /* To compute the median, we need the set of chain lengths
       sorted.  */
    qsort (chain_length, c->num_buckets, sizeof (chain_length[0]),
	   compare_positive_ints);
    qsort (entry_size, c->unique_count, sizeof (entry_size[0]),
	   compare_positive_ints);

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
    if (c->unique_count > 0)
      {
	max_entry_size = entry_size[c->unique_count - 1];
	median_entry_size = entry_size[c->unique_count / 2];
      }
    else
      {
	max_entry_size = 0;
	median_entry_size = 0;
      }

    xfree (chain_length);
    xfree (entry_size);
  }

  printf_filtered (_("  Cached '%s' statistics:\n"), type);
  printf_filtered (_("    Total object count:  %ld\n"), c->total_count);
  printf_filtered (_("    Unique object count: %lu\n"), c->unique_count);
  printf_filtered (_("    Percentage of duplicates, by count: "));
  print_percentage (c->total_count - c->unique_count, c->total_count);
  printf_filtered ("\n");

  printf_filtered (_("    Total object size:   %ld\n"), c->total_size);
  printf_filtered (_("    Unique object size:  %ld\n"), c->unique_size);
  printf_filtered (_("    Percentage of duplicates, by size:  "));
  print_percentage (c->total_size - c->unique_size, c->total_size);
  printf_filtered ("\n");

  printf_filtered (_("    Max entry size:     %d\n"), max_entry_size);
  printf_filtered (_("    Average entry size: "));
  if (c->unique_count > 0)
    printf_filtered ("%ld\n", c->unique_size / c->unique_count);
  else
    /* i18n: "Average entry size: (not applicable)".  */
    printf_filtered (_("(not applicable)\n"));    
  printf_filtered (_("    Median entry size:  %d\n"), median_entry_size);
  printf_filtered ("\n");

  printf_filtered (_("    \
Total memory used by bcache, including overhead: %ld\n"),
		   c->structure_size);
  printf_filtered (_("    Percentage memory overhead: "));
  print_percentage (c->structure_size - c->unique_size, c->unique_size);
  printf_filtered (_("    Net memory savings:         "));
  print_percentage (c->total_size - c->structure_size, c->total_size);
  printf_filtered ("\n");

  printf_filtered (_("    Hash table size:           %3d\n"), 
		   c->num_buckets);
  printf_filtered (_("    Hash table expands:        %lu\n"),
		   c->expand_count);
  printf_filtered (_("    Hash table hashes:         %lu\n"),
		   c->total_count + c->expand_hash_count);
  printf_filtered (_("    Half hash misses:          %lu\n"),
		   c->half_hash_miss_count);
  printf_filtered (_("    Hash table population:     "));
  print_percentage (occupied_buckets, c->num_buckets);
  printf_filtered (_("    Median hash chain length:  %3d\n"),
		   median_chain_length);
  printf_filtered (_("    Average hash chain length: "));
  if (c->num_buckets > 0)
    printf_filtered ("%3lu\n", c->unique_count / c->num_buckets);
  else
    /* i18n: "Average hash chain length: (not applicable)".  */
    printf_filtered (_("(not applicable)\n"));
  printf_filtered (_("    Maximum hash chain length: %3d\n"), 
		   max_chain_length);
  printf_filtered ("\n");
}

int
bcache_memory_used (struct bcache *bcache)
{
  if (bcache->total_count == 0)
    return 0;
  return obstack_memory_used (&bcache->cache);
}
