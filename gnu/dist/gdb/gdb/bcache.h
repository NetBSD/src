/* Include file cached obstack implementation.
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
     stick eight bytes of text in a bcache, and then stick an
     eight-byte structure in the same bcache, there's no guarantee
     those two objects don't actually comprise the same sequence of
     bytes.  If they happen to, the bcache will use a single byte
     string for both of them.  Then, modifying the structure will
     change the string.  In bizarre ways.

   - Even if you know for some other reason that all that's okay,
     there's another problem.  A bcache stores all its strings in a
     hash table.  If you modify a string's contents, you will probably
     change its hash value.  This means that the modified string is
     now in the wrong place in the hash table, and future bcache
     probes will never find it.  So by mutating a string, you give up
     any chance of sharing its space with future duplicates.  */


struct bcache;

/* Find a copy of the LENGTH bytes at ADDR in BCACHE.  If BCACHE has
   never seen those bytes before, add a copy of them to BCACHE.  In
   either case, return a pointer to BCACHE's copy of that string.  */
extern void *bcache (const void *addr, int length, struct bcache *bcache);

/* Free all the storage used by BCACHE.  */
extern void bcache_xfree (struct bcache *bcache);

/* Create a new bcache object.  */
extern struct bcache *bcache_xmalloc (void);

/* Print statistics on BCACHE's memory usage and efficacity at
   eliminating duplication.  TYPE should be a string describing the
   kind of data BCACHE holds.  Statistics are printed using
   `printf_filtered' and its ilk.  */
extern void print_bcache_statistics (struct bcache *bcache, char *type);
extern int bcache_memory_used (struct bcache *bcache);

/* The hash function */
extern unsigned long hash(const void *addr, int length);

#endif /* BCACHE_H */
