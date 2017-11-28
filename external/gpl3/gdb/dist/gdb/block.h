/* Code dealing with blocks for GDB.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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
struct compunit_symtab;
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

  /* Contains information about namespace-related info relevant to this block:
     using directives and the current namespace scope.  */

  struct block_namespace_info *namespace_info;
};

/* The global block is singled out so that we can provide a back-link
   to the compunit symtab.  */

struct global_block
{
  /* The block.  */

  struct block block;

  /* This holds a pointer to the compunit symtab holding this block.  */

  struct compunit_symtab *compunit_symtab;
};

#define BLOCK_START(bl)		(bl)->startaddr
#define BLOCK_END(bl)		(bl)->endaddr
#define BLOCK_FUNCTION(bl)	(bl)->function
#define BLOCK_SUPERBLOCK(bl)	(bl)->superblock
#define BLOCK_DICT(bl)		(bl)->dict
#define BLOCK_NAMESPACE(bl)	(bl)->namespace_info

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

/* Return the objfile of BLOCK, which must be non-NULL.  */

extern struct objfile *block_objfile (const struct block *block);

/* Return the architecture of BLOCK, which must be non-NULL.  */

extern struct gdbarch *block_gdbarch (const struct block *block);

extern struct symbol *block_linkage_function (const struct block *);

extern struct symbol *block_containing_function (const struct block *);

extern int block_inlined_p (const struct block *block);

extern int contained_in (const struct block *, const struct block *);

extern const struct blockvector *blockvector_for_pc (CORE_ADDR,
					       const struct block **);

extern const struct blockvector *
  blockvector_for_pc_sect (CORE_ADDR, struct obj_section *,
			   const struct block **, struct compunit_symtab *);

extern int blockvector_contains_pc (const struct blockvector *bv, CORE_ADDR pc);

extern struct call_site *call_site_for_pc (struct gdbarch *gdbarch,
					   CORE_ADDR pc);

extern const struct block *block_for_pc (CORE_ADDR);

extern const struct block *block_for_pc_sect (CORE_ADDR, struct obj_section *);

extern const char *block_scope (const struct block *block);

extern void block_set_scope (struct block *block, const char *scope,
			     struct obstack *obstack);

extern struct using_direct *block_using (const struct block *block);

extern void block_set_using (struct block *block,
			     struct using_direct *using_decl,
			     struct obstack *obstack);

extern const struct block *block_static_block (const struct block *block);

extern const struct block *block_global_block (const struct block *block);

extern struct block *allocate_block (struct obstack *obstack);

extern struct block *allocate_global_block (struct obstack *obstack);

extern void set_block_compunit_symtab (struct block *,
				       struct compunit_symtab *);

/* Return a property to evaluate the static link associated to BLOCK.

   In the context of nested functions (available in Pascal, Ada and GNU C, for
   instance), a static link (as in DWARF's DW_AT_static_link attribute) for a
   function is a way to get the frame corresponding to the enclosing function.

   Note that only objfile-owned and function-level blocks can have a static
   link.  Return NULL if there is no such property.  */

extern struct dynamic_prop *block_static_link (const struct block *block);

/* A block iterator.  This structure should be treated as though it
   were opaque; it is only defined here because we want to support
   stack allocation of iterators.  */

struct block_iterator
{
  /* If we're iterating over a single block, this holds the block.
     Otherwise, it holds the canonical compunit.  */

  union
  {
    struct compunit_symtab *compunit_symtab;
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

/* Search BLOCK for symbol NAME in DOMAIN.  */

extern struct symbol *block_lookup_symbol (const struct block *block,
					   const char *name,
					   const domain_enum domain);

/* Search BLOCK for symbol NAME in DOMAIN but only in primary symbol table of
   BLOCK.  BLOCK must be STATIC_BLOCK or GLOBAL_BLOCK.  Function is useful if
   one iterates all global/static blocks of an objfile.  */

extern struct symbol *block_lookup_symbol_primary (const struct block *block,
						   const char *name,
						   const domain_enum domain);

/* The type of the MATCHER argument to block_find_symbol.  */

typedef int (block_symbol_matcher_ftype) (struct symbol *, void *);

/* Find symbol NAME in BLOCK and in DOMAIN that satisfies MATCHER.
   DATA is passed unchanged to MATCHER.
   BLOCK must be STATIC_BLOCK or GLOBAL_BLOCK.  */

extern struct symbol *block_find_symbol (const struct block *block,
					 const char *name,
					 const domain_enum domain,
					 block_symbol_matcher_ftype *matcher,
					 void *data);

/* A matcher function for block_find_symbol to find only symbols with
   non-opaque types.  */

extern int block_find_non_opaque_type (struct symbol *sym, void *data);

/* A matcher function for block_find_symbol to prefer symbols with
   non-opaque types.  The way to use this function is as follows:

   struct symbol *with_opaque = NULL;
   struct symbol *sym
     = block_find_symbol (block, name, domain,
                          block_find_non_opaque_type_preferred, &with_opaque);

   At this point if SYM is non-NULL then a non-opaque type has been found.
   Otherwise, if WITH_OPAQUE is non-NULL then an opaque type has been found.
   Otherwise, the symbol was not found.  */

extern int block_find_non_opaque_type_preferred (struct symbol *sym,
						 void *data);

/* Macro to loop through all symbols in BLOCK, in no particular
   order.  ITER helps keep track of the iteration, and must be a
   struct block_iterator.  SYM points to the current symbol.  */

#define ALL_BLOCK_SYMBOLS(block, iter, sym)		\
  for ((sym) = block_iterator_first ((block), &(iter));	\
       (sym);						\
       (sym) = block_iterator_next (&(iter)))

/* Macro to loop through all symbols with name NAME in BLOCK,
   in no particular order.  ITER helps keep track of the iteration, and
   must be a struct block_iterator.  SYM points to the current symbol.  */

#define ALL_BLOCK_SYMBOLS_WITH_NAME(block, name, iter, sym)		\
  for ((sym) = block_iter_name_first ((block), (name), &(iter));	\
       (sym) != NULL;							\
       (sym) = block_iter_name_next ((name), &(iter)))

#endif /* BLOCK_H */
