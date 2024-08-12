/* Include file cached obstack implementation.
   Written by Fred Fish <fnf@cygnus.com>
   Rewritten by Jim Blandy <jimb@cygnus.com>

   Copyright (C) 1999-2023 Free Software Foundation, Inc.

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

#ifndef BCACHE_H
#define BCACHE_H 1

/* A bcache is a data structure for factoring out duplication in
   read-only structures.  You give the bcache some string of bytes S.
   If the bcache already contains a copy of S, it hands you back a
   pointer to its copy.  Otherwise, it makes a fresh copy of S, and
   hands you back a pointer to that.  In either case, you can throw
   away your copy of S, and use the bcache's.

   The "strings" in question are arbitrary strings of bytes --- they
   can contain zero bytes.  You pass in the length explicitly when you
   call the bcache function.

   This means that you can put ordinary C objects in a bcache.
   However, if you do this, remember that structs can contain `holes'
   between members, added for alignment.  These bytes usually contain
   garbage.  If you try to bcache two objects which are identical from
   your code's point of view, but have different garbage values in the
   structure's holes, then the bcache will treat them as separate
   strings, and you won't get the nice elimination of duplicates you
   were hoping for.  So, remember to memset your structures full of
   zeros before bcaching them!

   You shouldn't modify the strings you get from a bcache, because:

   - You don't necessarily know who you're sharing space with.  If I
   stick eight bytes of text in a bcache, and then stick an eight-byte
   structure in the same bcache, there's no guarantee those two
   objects don't actually comprise the same sequence of bytes.  If
   they happen to, the bcache will use a single byte string for both
   of them.  Then, modifying the structure will change the string.  In
   bizarre ways.

   - Even if you know for some other reason that all that's okay,
   there's another problem.  A bcache stores all its strings in a hash
   table.  If you modify a string's contents, you will probably change
   its hash value.  This means that the modified string is now in the
   wrong place in the hash table, and future bcache probes will never
   find it.  So by mutating a string, you give up any chance of
   sharing its space with future duplicates.


   Size of bcache VS hashtab:

   For bcache, the most critical cost is size (or more exactly the
   overhead added by the bcache).  It turns out that the bcache is
   remarkably efficient.

   Assuming a 32-bit system (the hash table slots are 4 bytes),
   ignoring alignment, and limit strings to 255 bytes (1 byte length)
   we get ...

   bcache: This uses a separate linked list to track the hash chain.
   The numbers show roughly 100% occupancy of the hash table and an
   average chain length of 4.  Spreading the slot cost over the 4
   chain elements:

   4 (slot) / 4 (chain length) + 1 (length) + 4 (chain) = 6 bytes

   hashtab: This uses a more traditional re-hash algorithm where the
   chain is maintained within the hash table.  The table occupancy is
   kept below 75% but we'll assume its perfect:

   4 (slot) x 4/3 (occupancy) +  1 (length) = 6 1/3 bytes

   So a perfect hashtab has just slightly larger than an average
   bcache.

   It turns out that an average hashtab is far worse.  Two things
   hurt:

   - Hashtab's occupancy is more like 50% (it ranges between 38% and
   75%) giving a per slot cost of 4x2 vs 4x4/3.

   - the string structure needs to be aligned to 8 bytes which for
   hashtab wastes 7 bytes, while for bcache wastes only 3.

   This gives:

   hashtab: 4 x 2 + 1 + 7 = 16 bytes

   bcache 4 / 4 + 1 + 4 + 3 = 9 bytes

   The numbers of GDB debugging GDB support this.  ~40% vs ~70% overhead.


   Speed of bcache VS hashtab (the half hash hack):

   While hashtab has a typical chain length of 1, bcache has a chain
   length of round 4.  This means that the bcache will require
   something like double the number of compares after that initial
   hash.  In both cases the comparison takes the form:

   a.length == b.length && memcmp (a.data, b.data, a.length) == 0

   That is lengths are checked before doing the memcmp.

   For GDB debugging GDB, it turned out that all lengths were 24 bytes
   (no C++ so only psymbols were cached) and hence, all compares
   required a call to memcmp.  As a hack, two bytes of padding
   (mentioned above) are used to store the upper 16 bits of the
   string's hash value and then that is used in the comparison vis:

   a.half_hash == b.half_hash && a.length == b.length && memcmp
   (a.data, b.data, a.length)

   The numbers from GDB debugging GDB show this to be a remarkable
   100% effective (only necessary length and memcmp tests being
   performed).

   Mind you, looking at the wall clock, the same GDB debugging GDB
   showed only marginal speed up (0.780 vs 0.773s).  Seems GDB is too
   busy doing something else :-(
  
*/

namespace gdb {

struct bstring;

struct bcache
{
  virtual ~bcache ();

  /* Find a copy of the LENGTH bytes at ADDR in BCACHE.  If BCACHE has
     never seen those bytes before, add a copy of them to BCACHE.  In
     either case, return a pointer to BCACHE's copy of that string.
     Since the cached value is meant to be read-only, return a const
     buffer.  If ADDED is not NULL, set *ADDED to true if the bytes
     were newly added to the cache, or to false if the bytes were
     found in the cache.  */

  const void *insert (const void *addr, int length, bool *added = nullptr);

  /* Print statistics on this bcache's memory usage and efficacity at
     eliminating duplication.  TYPE should be a string describing the
     kind of data this bcache holds.  Statistics are printed using
     `gdb_printf' and its ilk.  */
  void print_statistics (const char *type);
  int memory_used ();

protected:

  /* Hash function to be used for this bcache object.  Defaults to
     fast_hash.  */
  virtual unsigned long hash (const void *addr, int length);

  /* Compare function to be used for this bcache object.  Defaults to
     memcmp.  */
  virtual int compare (const void *left, const void *right, int length);

private:

  /* All the bstrings are allocated here.  */
  struct obstack m_cache {};

  /* How many hash buckets we're using.  */
  unsigned int m_num_buckets = 0;

  /* Hash buckets.  This table is allocated using malloc, so when we
     grow the table we can return the old table to the system.  */
  struct bstring **m_bucket = nullptr;

  /* Statistics.  */
  unsigned long m_unique_count = 0;	/* number of unique strings */
  long m_total_count = 0;	/* total number of strings cached, including dups */
  long m_unique_size = 0;	/* size of unique strings, in bytes */
  long m_total_size = 0;      /* total number of bytes cached, including dups */
  long m_structure_size = 0;	/* total size of bcache, including infrastructure */
  /* Number of times that the hash table is expanded and hence
     re-built, and the corresponding number of times that a string is
     [re]hashed as part of entering it into the expanded table.  The
     total number of hashes can be computed by adding TOTAL_COUNT to
     expand_hash_count.  */
  unsigned long m_expand_count = 0;
  unsigned long m_expand_hash_count = 0;
  /* Number of times that the half-hash compare hit (compare the upper
     16 bits of hash values) hit, but the corresponding combined
     length/data compare missed.  */
  unsigned long m_half_hash_miss_count = 0;

  /* Expand the hash table.  */
  void expand_hash_table ();
};

} /* namespace gdb */

#endif /* BCACHE_H */
