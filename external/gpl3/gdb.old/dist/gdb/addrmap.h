/* addrmap.h --- interface to address map data structure.

   Copyright (C) 2007-2017 Free Software Foundation, Inc.

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

#ifndef ADDRMAP_H
#define ADDRMAP_H

/* An address map is essentially a table mapping CORE_ADDRs onto GDB
   data structures, like blocks, symtabs, partial symtabs, and so on.
   An address map uses memory proportional to the number of
   transitions in the map, where a CORE_ADDR N is mapped to one
   object, and N+1 is mapped to a different object.

   Address maps come in two flavors: fixed, and mutable.  Mutable
   address maps consume more memory, but can be changed and extended.
   A fixed address map, once constructed (from a mutable address map),
   can't be edited.  Both kinds of map are allocated in obstacks.  */

/* The opaque type representing address maps.  */
struct addrmap;

/* Create a mutable address map which maps every address to NULL.
   Allocate entries in OBSTACK.  */
struct addrmap *addrmap_create_mutable (struct obstack *obstack);

/* In the mutable address map MAP, associate the addresses from START
   to END_INCLUSIVE that are currently associated with NULL with OBJ
   instead.  Addresses mapped to an object other than NULL are left
   unchanged.

   As the name suggests, END_INCLUSIVE is also mapped to OBJ.  This
   convention is unusual, but it allows callers to accurately specify
   ranges that abut the top of the address space, and ranges that
   cover the entire address space.

   This operation seems a bit complicated for a primitive: if it's
   needed, why not just have a simpler primitive operation that sets a
   range to a value, wiping out whatever was there before, and then
   let the caller construct more complicated operations from that,
   along with some others for traversal?

   It turns out this is the mutation operation we want to use all the
   time, at least for now.  Our immediate use for address maps is to
   represent lexical blocks whose address ranges are not contiguous.
   We walk the tree of lexical blocks present in the debug info, and
   only create 'struct block' objects after we've traversed all a
   block's children.  If a lexical block declares no local variables
   (and isn't the lexical block for a function's body), we omit it
   from GDB's data structures entirely.

   However, this means that we don't decide to create a block (and
   thus record it in the address map) until after we've traversed its
   children.  If we do decide to create the block, we do so at a time
   when all its children have already been recorded in the map.  So
   this operation --- change only those addresses left unset --- is
   actually the operation we want to use every time.

   It seems simpler to let the code which operates on the
   representation directly deal with the hair of implementing these
   semantics than to provide an interface which allows it to be
   implemented efficiently, but doesn't reveal too much of the
   representation.  */
void addrmap_set_empty (struct addrmap *map,
                        CORE_ADDR start, CORE_ADDR end_inclusive,
                        void *obj);

/* Return the object associated with ADDR in MAP.  */
void *addrmap_find (struct addrmap *map, CORE_ADDR addr);

/* Create a fixed address map which is a copy of the mutable address
   map ORIGINAL.  Allocate entries in OBSTACK.  */
struct addrmap *addrmap_create_fixed (struct addrmap *original,
                                      struct obstack *obstack);

/* Relocate all the addresses in MAP by OFFSET.  (This can be applied
   to either mutable or immutable maps.)  */
void addrmap_relocate (struct addrmap *map, CORE_ADDR offset);

/* The type of a function used to iterate over the map.
   OBJ is NULL for unmapped regions.  */
typedef int (*addrmap_foreach_fn) (void *data, CORE_ADDR start_addr,
				   void *obj);

/* Call FN, passing it DATA, for every address in MAP, following an
   in-order traversal.  If FN ever returns a non-zero value, the
   iteration ceases immediately, and the value is returned.
   Otherwise, this function returns 0.  */
int addrmap_foreach (struct addrmap *map, addrmap_foreach_fn fn, void *data);

#endif /* ADDRMAP_H */
