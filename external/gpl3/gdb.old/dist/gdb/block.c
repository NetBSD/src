/* Block-related functions for the GNU debugger, GDB.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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
#include "block.h"
#include "symtab.h"
#include "symfile.h"
#include "gdb_obstack.h"
#include "cp-support.h"
#include "addrmap.h"
#include "gdbtypes.h"
#include "objfiles.h"

/* This is used by struct block to store namespace-related info for
   C++ files, namely using declarations and the current namespace in
   scope.  */

struct block_namespace_info
{
  const char *scope;
  struct using_direct *using_decl;
};

static void block_initialize_namespace (struct block *block,
					struct obstack *obstack);

/* See block.h.  */

struct objfile *
block_objfile (const struct block *block)
{
  const struct global_block *global_block;

  if (BLOCK_FUNCTION (block) != NULL)
    return symbol_objfile (BLOCK_FUNCTION (block));

  global_block = (struct global_block *) block_global_block (block);
  return COMPUNIT_OBJFILE (global_block->compunit_symtab);
}

/* See block.  */

struct gdbarch *
block_gdbarch (const struct block *block)
{
  if (BLOCK_FUNCTION (block) != NULL)
    return symbol_arch (BLOCK_FUNCTION (block));

  return get_objfile_arch (block_objfile (block));
}

/* Return Nonzero if block a is lexically nested within block b,
   or if a and b have the same pc range.
   Return zero otherwise.  */

int
contained_in (const struct block *a, const struct block *b)
{
  if (!a || !b)
    return 0;

  do
    {
      if (a == b)
	return 1;
      /* If A is a function block, then A cannot be contained in B,
         except if A was inlined.  */
      if (BLOCK_FUNCTION (a) != NULL && !block_inlined_p (a))
        return 0;
      a = BLOCK_SUPERBLOCK (a);
    }
  while (a != NULL);

  return 0;
}


/* Return the symbol for the function which contains a specified
   lexical block, described by a struct block BL.  The return value
   will not be an inlined function; the containing function will be
   returned instead.  */

struct symbol *
block_linkage_function (const struct block *bl)
{
  while ((BLOCK_FUNCTION (bl) == NULL || block_inlined_p (bl))
	 && BLOCK_SUPERBLOCK (bl) != NULL)
    bl = BLOCK_SUPERBLOCK (bl);

  return BLOCK_FUNCTION (bl);
}

/* Return the symbol for the function which contains a specified
   block, described by a struct block BL.  The return value will be
   the closest enclosing function, which might be an inline
   function.  */

struct symbol *
block_containing_function (const struct block *bl)
{
  while (BLOCK_FUNCTION (bl) == NULL && BLOCK_SUPERBLOCK (bl) != NULL)
    bl = BLOCK_SUPERBLOCK (bl);

  return BLOCK_FUNCTION (bl);
}

/* Return one if BL represents an inlined function.  */

int
block_inlined_p (const struct block *bl)
{
  return BLOCK_FUNCTION (bl) != NULL && SYMBOL_INLINED (BLOCK_FUNCTION (bl));
}

/* A helper function that checks whether PC is in the blockvector BL.
   It returns the containing block if there is one, or else NULL.  */

static struct block *
find_block_in_blockvector (const struct blockvector *bl, CORE_ADDR pc)
{
  struct block *b;
  int bot, top, half;

  /* If we have an addrmap mapping code addresses to blocks, then use
     that.  */
  if (BLOCKVECTOR_MAP (bl))
    return (struct block *) addrmap_find (BLOCKVECTOR_MAP (bl), pc);

  /* Otherwise, use binary search to find the last block that starts
     before PC.
     Note: GLOBAL_BLOCK is block 0, STATIC_BLOCK is block 1.
     They both have the same START,END values.
     Historically this code would choose STATIC_BLOCK over GLOBAL_BLOCK but the
     fact that this choice was made was subtle, now we make it explicit.  */
  gdb_assert (BLOCKVECTOR_NBLOCKS (bl) >= 2);
  bot = STATIC_BLOCK;
  top = BLOCKVECTOR_NBLOCKS (bl);

  while (top - bot > 1)
    {
      half = (top - bot + 1) >> 1;
      b = BLOCKVECTOR_BLOCK (bl, bot + half);
      if (BLOCK_START (b) <= pc)
	bot += half;
      else
	top = bot + half;
    }

  /* Now search backward for a block that ends after PC.  */

  while (bot >= STATIC_BLOCK)
    {
      b = BLOCKVECTOR_BLOCK (bl, bot);
      if (BLOCK_END (b) > pc)
	return b;
      bot--;
    }

  return NULL;
}

/* Return the blockvector immediately containing the innermost lexical
   block containing the specified pc value and section, or 0 if there
   is none.  PBLOCK is a pointer to the block.  If PBLOCK is NULL, we
   don't pass this information back to the caller.  */

const struct blockvector *
blockvector_for_pc_sect (CORE_ADDR pc, struct obj_section *section,
			 const struct block **pblock,
			 struct compunit_symtab *cust)
{
  const struct blockvector *bl;
  struct block *b;

  if (cust == NULL)
    {
      /* First search all symtabs for one whose file contains our pc */
      cust = find_pc_sect_compunit_symtab (pc, section);
      if (cust == NULL)
	return 0;
    }

  bl = COMPUNIT_BLOCKVECTOR (cust);

  /* Then search that symtab for the smallest block that wins.  */
  b = find_block_in_blockvector (bl, pc);
  if (b == NULL)
    return NULL;

  if (pblock)
    *pblock = b;
  return bl;
}

/* Return true if the blockvector BV contains PC, false otherwise.  */

int
blockvector_contains_pc (const struct blockvector *bv, CORE_ADDR pc)
{
  return find_block_in_blockvector (bv, pc) != NULL;
}

/* Return call_site for specified PC in GDBARCH.  PC must match exactly, it
   must be the next instruction after call (or after tail call jump).  Throw
   NO_ENTRY_VALUE_ERROR otherwise.  This function never returns NULL.  */

struct call_site *
call_site_for_pc (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct compunit_symtab *cust;
  void **slot = NULL;

  /* -1 as tail call PC can be already after the compilation unit range.  */
  cust = find_pc_compunit_symtab (pc - 1);

  if (cust != NULL && COMPUNIT_CALL_SITE_HTAB (cust) != NULL)
    slot = htab_find_slot (COMPUNIT_CALL_SITE_HTAB (cust), &pc, NO_INSERT);

  if (slot == NULL)
    {
      struct bound_minimal_symbol msym = lookup_minimal_symbol_by_pc (pc);

      /* DW_TAG_gnu_call_site will be missing just if GCC could not determine
	 the call target.  */
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("DW_OP_GNU_entry_value resolving cannot find "
		     "DW_TAG_GNU_call_site %s in %s"),
		   paddress (gdbarch, pc),
		   (msym.minsym == NULL ? "???"
		    : MSYMBOL_PRINT_NAME (msym.minsym)));
    }

  return (struct call_site *) *slot;
}

/* Return the blockvector immediately containing the innermost lexical block
   containing the specified pc value, or 0 if there is none.
   Backward compatibility, no section.  */

const struct blockvector *
blockvector_for_pc (CORE_ADDR pc, const struct block **pblock)
{
  return blockvector_for_pc_sect (pc, find_pc_mapped_section (pc),
				  pblock, NULL);
}

/* Return the innermost lexical block containing the specified pc value
   in the specified section, or 0 if there is none.  */

const struct block *
block_for_pc_sect (CORE_ADDR pc, struct obj_section *section)
{
  const struct blockvector *bl;
  const struct block *b;

  bl = blockvector_for_pc_sect (pc, section, &b, NULL);
  if (bl)
    return b;
  return 0;
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  Backward compatibility, no section.  */

const struct block *
block_for_pc (CORE_ADDR pc)
{
  return block_for_pc_sect (pc, find_pc_mapped_section (pc));
}

/* Now come some functions designed to deal with C++ namespace issues.
   The accessors are safe to use even in the non-C++ case.  */

/* This returns the namespace that BLOCK is enclosed in, or "" if it
   isn't enclosed in a namespace at all.  This travels the chain of
   superblocks looking for a scope, if necessary.  */

const char *
block_scope (const struct block *block)
{
  for (; block != NULL; block = BLOCK_SUPERBLOCK (block))
    {
      if (BLOCK_NAMESPACE (block) != NULL
	  && BLOCK_NAMESPACE (block)->scope != NULL)
	return BLOCK_NAMESPACE (block)->scope;
    }

  return "";
}

/* Set BLOCK's scope member to SCOPE; if needed, allocate memory via
   OBSTACK.  (It won't make a copy of SCOPE, however, so that already
   has to be allocated correctly.)  */

void
block_set_scope (struct block *block, const char *scope,
		 struct obstack *obstack)
{
  block_initialize_namespace (block, obstack);

  BLOCK_NAMESPACE (block)->scope = scope;
}

/* This returns the using directives list associated with BLOCK, if
   any.  */

struct using_direct *
block_using (const struct block *block)
{
  if (block == NULL || BLOCK_NAMESPACE (block) == NULL)
    return NULL;
  else
    return BLOCK_NAMESPACE (block)->using_decl;
}

/* Set BLOCK's using member to USING; if needed, allocate memory via
   OBSTACK.  (It won't make a copy of USING, however, so that already
   has to be allocated correctly.)  */

void
block_set_using (struct block *block,
		 struct using_direct *using_decl,
		 struct obstack *obstack)
{
  block_initialize_namespace (block, obstack);

  BLOCK_NAMESPACE (block)->using_decl = using_decl;
}

/* If BLOCK_NAMESPACE (block) is NULL, allocate it via OBSTACK and
   ititialize its members to zero.  */

static void
block_initialize_namespace (struct block *block, struct obstack *obstack)
{
  if (BLOCK_NAMESPACE (block) == NULL)
    {
      BLOCK_NAMESPACE (block) = XOBNEW (obstack, struct block_namespace_info);
      BLOCK_NAMESPACE (block)->scope = NULL;
      BLOCK_NAMESPACE (block)->using_decl = NULL;
    }
}

/* Return the static block associated to BLOCK.  Return NULL if block
   is NULL or if block is a global block.  */

const struct block *
block_static_block (const struct block *block)
{
  if (block == NULL || BLOCK_SUPERBLOCK (block) == NULL)
    return NULL;

  while (BLOCK_SUPERBLOCK (BLOCK_SUPERBLOCK (block)) != NULL)
    block = BLOCK_SUPERBLOCK (block);

  return block;
}

/* Return the static block associated to BLOCK.  Return NULL if block
   is NULL.  */

const struct block *
block_global_block (const struct block *block)
{
  if (block == NULL)
    return NULL;

  while (BLOCK_SUPERBLOCK (block) != NULL)
    block = BLOCK_SUPERBLOCK (block);

  return block;
}

/* Allocate a block on OBSTACK, and initialize its elements to
   zero/NULL.  This is useful for creating "dummy" blocks that don't
   correspond to actual source files.

   Warning: it sets the block's BLOCK_DICT to NULL, which isn't a
   valid value.  If you really don't want the block to have a
   dictionary, then you should subsequently set its BLOCK_DICT to
   dict_create_linear (obstack, NULL).  */

struct block *
allocate_block (struct obstack *obstack)
{
  struct block *bl = OBSTACK_ZALLOC (obstack, struct block);

  return bl;
}

/* Allocate a global block.  */

struct block *
allocate_global_block (struct obstack *obstack)
{
  struct global_block *bl = OBSTACK_ZALLOC (obstack, struct global_block);

  return &bl->block;
}

/* Set the compunit of the global block.  */

void
set_block_compunit_symtab (struct block *block, struct compunit_symtab *cu)
{
  struct global_block *gb;

  gdb_assert (BLOCK_SUPERBLOCK (block) == NULL);
  gb = (struct global_block *) block;
  gdb_assert (gb->compunit_symtab == NULL);
  gb->compunit_symtab = cu;
}

/* See block.h.  */

struct dynamic_prop *
block_static_link (const struct block *block)
{
  struct objfile *objfile = block_objfile (block);

  /* Only objfile-owned blocks that materialize top function scopes can have
     static links.  */
  if (objfile == NULL || BLOCK_FUNCTION (block) == NULL)
    return NULL;

  return (struct dynamic_prop *) objfile_lookup_static_link (objfile, block);
}

/* Return the compunit of the global block.  */

static struct compunit_symtab *
get_block_compunit_symtab (const struct block *block)
{
  struct global_block *gb;

  gdb_assert (BLOCK_SUPERBLOCK (block) == NULL);
  gb = (struct global_block *) block;
  gdb_assert (gb->compunit_symtab != NULL);
  return gb->compunit_symtab;
}



/* Initialize a block iterator, either to iterate over a single block,
   or, for static and global blocks, all the included symtabs as
   well.  */

static void
initialize_block_iterator (const struct block *block,
			   struct block_iterator *iter)
{
  enum block_enum which;
  struct compunit_symtab *cu;

  iter->idx = -1;

  if (BLOCK_SUPERBLOCK (block) == NULL)
    {
      which = GLOBAL_BLOCK;
      cu = get_block_compunit_symtab (block);
    }
  else if (BLOCK_SUPERBLOCK (BLOCK_SUPERBLOCK (block)) == NULL)
    {
      which = STATIC_BLOCK;
      cu = get_block_compunit_symtab (BLOCK_SUPERBLOCK (block));
    }
  else
    {
      iter->d.block = block;
      /* A signal value meaning that we're iterating over a single
	 block.  */
      iter->which = FIRST_LOCAL_BLOCK;
      return;
    }

  /* If this is an included symtab, find the canonical includer and
     use it instead.  */
  while (cu->user != NULL)
    cu = cu->user;

  /* Putting this check here simplifies the logic of the iterator
     functions.  If there are no included symtabs, we only need to
     search a single block, so we might as well just do that
     directly.  */
  if (cu->includes == NULL)
    {
      iter->d.block = block;
      /* A signal value meaning that we're iterating over a single
	 block.  */
      iter->which = FIRST_LOCAL_BLOCK;
    }
  else
    {
      iter->d.compunit_symtab = cu;
      iter->which = which;
    }
}

/* A helper function that finds the current compunit over whose static
   or global block we should iterate.  */

static struct compunit_symtab *
find_iterator_compunit_symtab (struct block_iterator *iterator)
{
  if (iterator->idx == -1)
    return iterator->d.compunit_symtab;
  return iterator->d.compunit_symtab->includes[iterator->idx];
}

/* Perform a single step for a plain block iterator, iterating across
   symbol tables as needed.  Returns the next symbol, or NULL when
   iteration is complete.  */

static struct symbol *
block_iterator_step (struct block_iterator *iterator, int first)
{
  struct symbol *sym;

  gdb_assert (iterator->which != FIRST_LOCAL_BLOCK);

  while (1)
    {
      if (first)
	{
	  struct compunit_symtab *cust
	    = find_iterator_compunit_symtab (iterator);
	  const struct block *block;

	  /* Iteration is complete.  */
	  if (cust == NULL)
	    return  NULL;

	  block = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cust),
				     iterator->which);
	  sym = dict_iterator_first (BLOCK_DICT (block), &iterator->dict_iter);
	}
      else
	sym = dict_iterator_next (&iterator->dict_iter);

      if (sym != NULL)
	return sym;

      /* We have finished iterating the appropriate block of one
	 symtab.  Now advance to the next symtab and begin iteration
	 there.  */
      ++iterator->idx;
      first = 1;
    }
}

/* See block.h.  */

struct symbol *
block_iterator_first (const struct block *block,
		      struct block_iterator *iterator)
{
  initialize_block_iterator (block, iterator);

  if (iterator->which == FIRST_LOCAL_BLOCK)
    return dict_iterator_first (block->dict, &iterator->dict_iter);

  return block_iterator_step (iterator, 1);
}

/* See block.h.  */

struct symbol *
block_iterator_next (struct block_iterator *iterator)
{
  if (iterator->which == FIRST_LOCAL_BLOCK)
    return dict_iterator_next (&iterator->dict_iter);

  return block_iterator_step (iterator, 0);
}

/* Perform a single step for a "name" block iterator, iterating across
   symbol tables as needed.  Returns the next symbol, or NULL when
   iteration is complete.  */

static struct symbol *
block_iter_name_step (struct block_iterator *iterator, const char *name,
		      int first)
{
  struct symbol *sym;

  gdb_assert (iterator->which != FIRST_LOCAL_BLOCK);

  while (1)
    {
      if (first)
	{
	  struct compunit_symtab *cust
	    = find_iterator_compunit_symtab (iterator);
	  const struct block *block;

	  /* Iteration is complete.  */
	  if (cust == NULL)
	    return  NULL;

	  block = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cust),
				     iterator->which);
	  sym = dict_iter_name_first (BLOCK_DICT (block), name,
				      &iterator->dict_iter);
	}
      else
	sym = dict_iter_name_next (name, &iterator->dict_iter);

      if (sym != NULL)
	return sym;

      /* We have finished iterating the appropriate block of one
	 symtab.  Now advance to the next symtab and begin iteration
	 there.  */
      ++iterator->idx;
      first = 1;
    }
}

/* See block.h.  */

struct symbol *
block_iter_name_first (const struct block *block,
		       const char *name,
		       struct block_iterator *iterator)
{
  initialize_block_iterator (block, iterator);

  if (iterator->which == FIRST_LOCAL_BLOCK)
    return dict_iter_name_first (block->dict, name, &iterator->dict_iter);

  return block_iter_name_step (iterator, name, 1);
}

/* See block.h.  */

struct symbol *
block_iter_name_next (const char *name, struct block_iterator *iterator)
{
  if (iterator->which == FIRST_LOCAL_BLOCK)
    return dict_iter_name_next (name, &iterator->dict_iter);

  return block_iter_name_step (iterator, name, 0);
}

/* Perform a single step for a "match" block iterator, iterating
   across symbol tables as needed.  Returns the next symbol, or NULL
   when iteration is complete.  */

static struct symbol *
block_iter_match_step (struct block_iterator *iterator,
		       const char *name,
		       symbol_compare_ftype *compare,
		       int first)
{
  struct symbol *sym;

  gdb_assert (iterator->which != FIRST_LOCAL_BLOCK);

  while (1)
    {
      if (first)
	{
	  struct compunit_symtab *cust
	    = find_iterator_compunit_symtab (iterator);
	  const struct block *block;

	  /* Iteration is complete.  */
	  if (cust == NULL)
	    return  NULL;

	  block = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cust),
				     iterator->which);
	  sym = dict_iter_match_first (BLOCK_DICT (block), name,
				       compare, &iterator->dict_iter);
	}
      else
	sym = dict_iter_match_next (name, compare, &iterator->dict_iter);

      if (sym != NULL)
	return sym;

      /* We have finished iterating the appropriate block of one
	 symtab.  Now advance to the next symtab and begin iteration
	 there.  */
      ++iterator->idx;
      first = 1;
    }
}

/* See block.h.  */

struct symbol *
block_iter_match_first (const struct block *block,
			const char *name,
			symbol_compare_ftype *compare,
			struct block_iterator *iterator)
{
  initialize_block_iterator (block, iterator);

  if (iterator->which == FIRST_LOCAL_BLOCK)
    return dict_iter_match_first (block->dict, name, compare,
				  &iterator->dict_iter);

  return block_iter_match_step (iterator, name, compare, 1);
}

/* See block.h.  */

struct symbol *
block_iter_match_next (const char *name,
		       symbol_compare_ftype *compare,
		       struct block_iterator *iterator)
{
  if (iterator->which == FIRST_LOCAL_BLOCK)
    return dict_iter_match_next (name, compare, &iterator->dict_iter);

  return block_iter_match_step (iterator, name, compare, 0);
}

/* See block.h.

   Note that if NAME is the demangled form of a C++ symbol, we will fail
   to find a match during the binary search of the non-encoded names, but
   for now we don't worry about the slight inefficiency of looking for
   a match we'll never find, since it will go pretty quick.  Once the
   binary search terminates, we drop through and do a straight linear
   search on the symbols.  Each symbol which is marked as being a ObjC/C++
   symbol (language_cplus or language_objc set) has both the encoded and
   non-encoded names tested for a match.  */

struct symbol *
block_lookup_symbol (const struct block *block, const char *name,
		     const domain_enum domain)
{
  struct block_iterator iter;
  struct symbol *sym;

  if (!BLOCK_FUNCTION (block))
    {
      struct symbol *other = NULL;

      ALL_BLOCK_SYMBOLS_WITH_NAME (block, name, iter, sym)
	{
	  if (SYMBOL_DOMAIN (sym) == domain)
	    return sym;
	  /* This is a bit of a hack, but symbol_matches_domain might ignore
	     STRUCT vs VAR domain symbols.  So if a matching symbol is found,
	     make sure there is no "better" matching symbol, i.e., one with
	     exactly the same domain.  PR 16253.  */
	  if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				     SYMBOL_DOMAIN (sym), domain))
	    other = sym;
	}
      return other;
    }
  else
    {
      /* Note that parameter symbols do not always show up last in the
	 list; this loop makes sure to take anything else other than
	 parameter symbols first; it only uses parameter symbols as a
	 last resort.  Note that this only takes up extra computation
	 time on a match.
	 It's hard to define types in the parameter list (at least in
	 C/C++) so we don't do the same PR 16253 hack here that is done
	 for the !BLOCK_FUNCTION case.  */

      struct symbol *sym_found = NULL;

      ALL_BLOCK_SYMBOLS_WITH_NAME (block, name, iter, sym)
	{
	  if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				     SYMBOL_DOMAIN (sym), domain))
	    {
	      sym_found = sym;
	      if (!SYMBOL_IS_ARGUMENT (sym))
		{
		  break;
		}
	    }
	}
      return (sym_found);	/* Will be NULL if not found.  */
    }
}

/* See block.h.  */

struct symbol *
block_lookup_symbol_primary (const struct block *block, const char *name,
			     const domain_enum domain)
{
  struct symbol *sym, *other;
  struct dict_iterator dict_iter;

  /* Verify BLOCK is STATIC_BLOCK or GLOBAL_BLOCK.  */
  gdb_assert (BLOCK_SUPERBLOCK (block) == NULL
	      || BLOCK_SUPERBLOCK (BLOCK_SUPERBLOCK (block)) == NULL);

  other = NULL;
  for (sym = dict_iter_name_first (block->dict, name, &dict_iter);
       sym != NULL;
       sym = dict_iter_name_next (name, &dict_iter))
    {
      if (SYMBOL_DOMAIN (sym) == domain)
	return sym;

      /* This is a bit of a hack, but symbol_matches_domain might ignore
	 STRUCT vs VAR domain symbols.  So if a matching symbol is found,
	 make sure there is no "better" matching symbol, i.e., one with
	 exactly the same domain.  PR 16253.  */
      if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				 SYMBOL_DOMAIN (sym), domain))
	other = sym;
    }

  return other;
}

/* See block.h.  */

struct symbol *
block_find_symbol (const struct block *block, const char *name,
		   const domain_enum domain,
		   block_symbol_matcher_ftype *matcher, void *data)
{
  struct block_iterator iter;
  struct symbol *sym;

  /* Verify BLOCK is STATIC_BLOCK or GLOBAL_BLOCK.  */
  gdb_assert (BLOCK_SUPERBLOCK (block) == NULL
	      || BLOCK_SUPERBLOCK (BLOCK_SUPERBLOCK (block)) == NULL);

  ALL_BLOCK_SYMBOLS_WITH_NAME (block, name, iter, sym)
    {
      /* MATCHER is deliberately called second here so that it never sees
	 a non-domain-matching symbol.  */
      if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				 SYMBOL_DOMAIN (sym), domain)
	  && matcher (sym, data))
	return sym;
    }
  return NULL;
}

/* See block.h.  */

int
block_find_non_opaque_type (struct symbol *sym, void *data)
{
  return !TYPE_IS_OPAQUE (SYMBOL_TYPE (sym));
}

/* See block.h.  */

int
block_find_non_opaque_type_preferred (struct symbol *sym, void *data)
{
  struct symbol **best = (struct symbol **) data;

  if (!TYPE_IS_OPAQUE (SYMBOL_TYPE (sym)))
    return 1;
  *best = sym;
  return 0;
}
