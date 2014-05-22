/* Code dealing with blocks for GDB.

   Copyright (C) 2003-2013 Free Software Foundation, Inc.

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

#ifndef BLOCK_H
#define BLOCK_H

#include "dictionary.h"

/* Opaque declarations.  */

struct symbol;
struct symtab;
struct block_namespace_info;
struct using_direct;
struct obstack;
struct addrmap;

/* All of the name-scope contours of the program
   are represented by `struct block' objects.
   All of these objects are pointed to by the blockvector.

   Each block represents one name scope.
   Each lexical context has its own block.

   The blockvector begins with some special blocks.
   The GLOBAL_BLOCK contains all the symbols defined in this compilation
   whose scope is the entire program linked together.
   The STATIC_BLOCK contains all the symbols whose scope is the
   entire compilation excluding other separate compilations.
   Blocks starting with the FIRST_LOCAL_BLOCK are not special.

   Each block records a range of core addresses for the code that
   is in the scope of the block.  The STATIC_BLOCK and GLOBAL_BLOCK
   give, for the range of code, the entire range of code produced
   by the compilation that the symbol segment belongs to.

   The blocks appear in the blockvector
   in order of increasing starting-address,
   and, within that, in order of decreasing ending-address.

   This implies that within the body of one function
   the blocks appear in the order of a depth-first tree walk.  */

struct block
{

  /* Addresses in the executable code that are in this block.  */

  CORE_ADDR startaddr;
  CORE_ADDR endaddr;

  /* The symbol that names this block, if the block is the body of a
     function (real or inlined); otherwise, zero.  */

  struct symbol *function;

  /* The `struct block' for the containing block, or 0 if none.

     The superblock of a top-level local block (i.e. a function in the
     case of C) is the STATIC_BLOCK.  The superblock of the
     STATIC_BLOCK is the GLOBAL_BLOCK.  */

  struct block *superblock;

  /* This is used to store the symbols in the block.  */

  struct dictionary *dict;

  /* Used for language-specific info.  */

  union
  {
    struct
    {
      /* Contains information about namespace-related info relevant to
	 this block: using directives and the current namespace
	 scope.  */
      
      struct block_namespace_info *namespace;
    }
    cplus_specific;
  }
  language_specific;
};

/* The global block is singled out so that we can provide a back-link
   to the primary symtab.  */

struct global_block
{
  /* The block.  */

  struct block block;

  /* This holds a pointer to the primary symtab holding this
     block.  */

  struct symtab *symtab;
};

#define BLOCK_START(bl)		(bl)->startaddr
#define BLOCK_END(bl)		(bl)->endaddr
#define BLOCK_FUNCTION(bl)	(bl)->function
#define BLOCK_SUPERBLOCK(bl)	(bl)->superblock
#define BLOCK_DICT(bl)		(bl)->dict
#define BLOCK_NAMESPACE(bl)   (bl)->language_specific.cplus_specific.namespace

struct blockvector
{
  /* Number of blocks in the list.  */
  int nblocks;
  /* An address map mapping addresses to blocks in this blockvector.
     This pointer is zero if the blocks' start and end addresses are
     enough.  */
  struct addrmap *map;
  /* The blocks themselves.  */
  struct block *block[1];
};

#define BLOCKVECTOR_NBLOCKS(blocklist) (blocklist)->nblocks
#define BLOCKVECTOR_BLOCK(blocklist,n) (blocklist)->block[n]
#define BLOCKVECTOR_MAP(blocklist) ((blocklist)->map)

extern struct symbol *block_linkage_function (const struct block *);

extern struct symbol *block_containing_function (const struct block *);

extern int block_inlined_p (const struct block *block);

extern int contained_in (const struct block *, const struct block *);

extern struct blockvector *blockvector_for_pc (CORE_ADDR, struct block **);

extern struct blockvector *blockvector_for_pc_sect (CORE_ADDR, 
						    struct obj_section *,
						    struct block **,
                                                    struct symtab *);

extern int blockvector_contains_pc (struct blockvector *bv, CORE_ADDR pc);

extern struct call_site *call_site_for_pc (struct gdbarch *gdbarch,
					   CORE_ADDR pc);

extern struct block *block_for_pc (CORE_ADDR);

extern struct block *block_for_pc_sect (CORE_ADDR, struct obj_section *);

extern const char *block_scope (const struct block *block);

extern void block_set_scope (struct block *block, const char *scope,
			     struct obstack *obstack);

extern struct using_direct *block_using (const struct block *block);

extern void block_set_using (struct block *block,
			     struct using_direct *using,
			     struct obstack *obstack);

extern const struct block *block_static_block (const struct block *block);

extern const struct block *block_global_block (const struct block *block);

extern struct block *allocate_block (struct obstack *obstack);

extern struct block *allocate_global_block (struct obstack *obstack);

extern void set_block_symtab (struct block *, struct symtab *);

/* A block iterator.  This structure should be treated as though it
   were opaque; it is only defined here because we want to support
   stack allocation of iterators.  */

struct block_iterator
{
  /* If we're iterating over a single block, this holds the block.
     Otherwise, it holds the canonical symtab.  */

  union
  {
    struct symtab *symtab;
    const struct block *block;
  } d;

  /* If we're iterating over a single block, this is always -1.
     Otherwise, it holds the index of the current "included" symtab in
     the canonical symtab (that is, d.symtab->includes[idx]), with -1
     meaning the canonical symtab itself.  */

  int idx;

  /* Which block, either static or global, to iterate over.  If this
     is FIRST_LOCAL_BLOCK, then we are iterating over a single block.
     This is used to select which field of 'd' is in use.  */

  enum block_enum which;

  /* The underlying dictionary iterator.  */

  struct dict_iterator dict_iter;
};

/* Initialize ITERATOR to point at the first symbol in BLOCK, and
   return that first symbol, or NULL if BLOCK is empty.  */

extern struct symbol *block_iterator_first (const struct block *block,
					    struct block_iterator *iterator);

/* Advance ITERATOR, and return the next symbol, or NULL if there are
   no more symbols.  Don't call this if you've previously received
   NULL from block_iterator_first or block_iterator_next on this
   iteration.  */

extern struct symbol *block_iterator_next (struct block_iterator *iterator);

/* Initialize ITERATOR to point at the first symbol in BLOCK whose
   SYMBOL_SEARCH_NAME is NAME (as tested using strcmp_iw), and return
   that first symbol, or NULL if there are no such symbols.  */

extern struct symbol *block_iter_name_first (const struct block *block,
					     const char *name,
					     struct block_iterator *iterator);

/* Advance ITERATOR to point at the next symbol in BLOCK whose
   SYMBOL_SEARCH_NAME is NAME (as tested using strcmp_iw), or NULL if
   there are no more such symbols.  Don't call this if you've
   previously received NULL from block_iterator_first or
   block_iterator_next on this iteration.  And don't call it unless
   ITERATOR was created by a previous call to block_iter_name_first
   with the same NAME.  */

extern struct symbol *block_iter_name_next (const char *name,
					    struct block_iterator *iterator);

/* Initialize ITERATOR to point at the first symbol in BLOCK whose
   SYMBOL_SEARCH_NAME is NAME, as tested using COMPARE (which must use
   the same conventions as strcmp_iw and be compatible with any
   block hashing function), and return that first symbol, or NULL
   if there are no such symbols.  */

extern struct symbol *block_iter_match_first (const struct block *block,
					      const char *name,
					      symbol_compare_ftype *compare,
					      struct block_iterator *iterator);

/* Advance ITERATOR to point at the next symbol in BLOCK whose
   SYMBOL_SEARCH_NAME is NAME, as tested using COMPARE (see
   block_iter_match_first), or NULL if there are no more such symbols.
   Don't call this if you've previously received NULL from 
   block_iterator_match_first or block_iterator_match_next on this
   iteration.  And don't call it unless ITERATOR was created by a
   previous call to block_iter_match_first with the same NAME and COMPARE.  */

extern struct symbol *block_iter_match_next (const char *name,
					     symbol_compare_ftype *compare,
					     struct block_iterator *iterator);

/* Macro to loop through all symbols in a block BL, in no particular
   order.  ITER helps keep track of the iteration, and should be a
   struct block_iterator.  SYM points to the current symbol.  */

#define ALL_BLOCK_SYMBOLS(block, iter, sym)		\
  for ((sym) = block_iterator_first ((block), &(iter));	\
       (sym);						\
       (sym) = block_iterator_next (&(iter)))

#endif /* BLOCK_H */
