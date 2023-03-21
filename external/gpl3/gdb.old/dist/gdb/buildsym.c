/* Support routines for building symbol tables in GDB's internal format.
   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include "buildsym-legacy.h"
#include "bfd.h"
#include "gdb_obstack.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "complaints.h"
#include "expression.h"		/* For "enum exp_opcode" used by...  */
#include "filenames.h"		/* For DOSish file names.  */
#include "macrotab.h"
#include "demangle.h"		/* Needed by SYMBOL_INIT_DEMANGLED_NAME.  */
#include "block.h"
#include "cp-support.h"
#include "dictionary.h"
#include "addrmap.h"
#include <algorithm>

/* For cleanup_undefined_stabs_types and finish_global_stabs (somewhat
   questionable--see comment where we call them).  */

#include "stabsread.h"

/* List of blocks already made (lexical contexts already closed).
   This is used at the end to make the blockvector.  */

struct pending_block
  {
    struct pending_block *next;
    struct block *block;
  };

/* Initial sizes of data structures.  These are realloc'd larger if
   needed, and realloc'd down to the size actually used, when
   completed.  */

#define	INITIAL_LINE_VECTOR_LENGTH	1000


buildsym_compunit::buildsym_compunit (struct objfile *objfile_,
				      const char *name,
				      const char *comp_dir_,
				      enum language language_,
				      CORE_ADDR last_addr)
  : m_objfile (objfile_),
    m_last_source_file (name == nullptr ? nullptr : xstrdup (name)),
    m_comp_dir (comp_dir_ == nullptr ? nullptr : xstrdup (comp_dir_)),
    m_language (language_),
    m_last_source_start_addr (last_addr)
{
  /* Allocate the compunit symtab now.  The caller needs it to allocate
     non-primary symtabs.  It is also needed by get_macro_table.  */
  m_compunit_symtab = allocate_compunit_symtab (m_objfile, name);

  /* Build the subfile for NAME (the main source file) so that we can record
     a pointer to it for later.
     IMPORTANT: Do not allocate a struct symtab for NAME here.
     It can happen that the debug info provides a different path to NAME than
     DIRNAME,NAME.  We cope with this in watch_main_source_file_lossage but
     that only works if the main_subfile doesn't have a symtab yet.  */
  start_subfile (name);
  /* Save this so that we don't have to go looking for it at the end
     of the subfiles list.  */
  m_main_subfile = m_current_subfile;
}

buildsym_compunit::~buildsym_compunit ()
{
  struct subfile *subfile, *nextsub;

  if (m_pending_macros != nullptr)
    free_macro_table (m_pending_macros);

  for (subfile = m_subfiles;
       subfile != NULL;
       subfile = nextsub)
    {
      nextsub = subfile->next;
      xfree (subfile->name);
      xfree (subfile->line_vector);
      xfree (subfile);
    }

  struct pending *next, *next1;

  for (next = m_file_symbols; next != NULL; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }

  for (next = m_global_symbols; next != NULL; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }
}

struct macro_table *
buildsym_compunit::get_macro_table ()
{
  if (m_pending_macros == nullptr)
    m_pending_macros = new_macro_table (&m_objfile->per_bfd->storage_obstack,
					&m_objfile->per_bfd->string_cache,
					m_compunit_symtab);
  return m_pending_macros;
}

/* Maintain the lists of symbols and blocks.  */

/* Add a symbol to one of the lists of symbols.  */

void
add_symbol_to_list (struct symbol *symbol, struct pending **listhead)
{
  struct pending *link;

  /* If this is an alias for another symbol, don't add it.  */
  if (symbol->linkage_name () && symbol->linkage_name ()[0] == '#')
    return;

  /* We keep PENDINGSIZE symbols in each link of the list.  If we
     don't have a link with room in it, add a new link.  */
  if (*listhead == NULL || (*listhead)->nsyms == PENDINGSIZE)
    {
      link = XNEW (struct pending);
      link->next = *listhead;
      *listhead = link;
      link->nsyms = 0;
    }

  (*listhead)->symbol[(*listhead)->nsyms++] = symbol;
}

/* Find a symbol named NAME on a LIST.  NAME need not be
   '\0'-terminated; LENGTH is the length of the name.  */

struct symbol *
find_symbol_in_list (struct pending *list, char *name, int length)
{
  int j;
  const char *pp;

  while (list != NULL)
    {
      for (j = list->nsyms; --j >= 0;)
	{
	  pp = list->symbol[j]->linkage_name ();
	  if (*pp == *name && strncmp (pp, name, length) == 0
	      && pp[length] == '\0')
	    {
	      return (list->symbol[j]);
	    }
	}
      list = list->next;
    }
  return (NULL);
}

/* Record BLOCK on the list of all blocks in the file.  Put it after
   OPBLOCK, or at the beginning if opblock is NULL.  This puts the
   block in the list after all its subblocks.  */

void
buildsym_compunit::record_pending_block (struct block *block,
					 struct pending_block *opblock)
{
  struct pending_block *pblock;

  pblock = XOBNEW (&m_pending_block_obstack, struct pending_block);
  pblock->block = block;
  if (opblock)
    {
      pblock->next = opblock->next;
      opblock->next = pblock;
    }
  else
    {
      pblock->next = m_pending_blocks;
      m_pending_blocks = pblock;
    }
}

/* Take one of the lists of symbols and make a block from it.  Keep
   the order the symbols have in the list (reversed from the input
   file).  Put the block on the list of pending blocks.  */

struct block *
buildsym_compunit::finish_block_internal
    (struct symbol *symbol,
     struct pending **listhead,
     struct pending_block *old_blocks,
     const struct dynamic_prop *static_link,
     CORE_ADDR start, CORE_ADDR end,
     int is_global, int expandable)
{
  struct gdbarch *gdbarch = m_objfile->arch ();
  struct pending *next, *next1;
  struct block *block;
  struct pending_block *pblock;
  struct pending_block *opblock;

  block = (is_global
	   ? allocate_global_block (&m_objfile->objfile_obstack)
	   : allocate_block (&m_objfile->objfile_obstack));

  if (symbol)
    {
      BLOCK_MULTIDICT (block)
	= mdict_create_linear (&m_objfile->objfile_obstack, *listhead);
    }
  else
    {
      if (expandable)
	{
	  BLOCK_MULTIDICT (block) = mdict_create_hashed_expandable (m_language);
	  mdict_add_pending (BLOCK_MULTIDICT (block), *listhead);
	}
      else
	{
	  BLOCK_MULTIDICT (block) =
	    mdict_create_hashed (&m_objfile->objfile_obstack, *listhead);
	}
    }

  BLOCK_START (block) = start;
  BLOCK_END (block) = end;

  /* Put the block in as the value of the symbol that names it.  */

  if (symbol)
    {
      struct type *ftype = SYMBOL_TYPE (symbol);
      struct mdict_iterator miter;
      SYMBOL_BLOCK_VALUE (symbol) = block;
      BLOCK_FUNCTION (block) = symbol;

      if (ftype->num_fields () <= 0)
	{
	  /* No parameter type information is recorded with the
	     function's type.  Set that from the type of the
	     parameter symbols.  */
	  int nparams = 0, iparams;
	  struct symbol *sym;

	  /* Here we want to directly access the dictionary, because
	     we haven't fully initialized the block yet.  */
	  ALL_DICT_SYMBOLS (BLOCK_MULTIDICT (block), miter, sym)
	    {
	      if (SYMBOL_IS_ARGUMENT (sym))
		nparams++;
	    }
	  if (nparams > 0)
	    {
	      ftype->set_num_fields (nparams);
	      ftype->set_fields
		((struct field *)
		 TYPE_ALLOC (ftype, nparams * sizeof (struct field)));

	      iparams = 0;
	      /* Here we want to directly access the dictionary, because
		 we haven't fully initialized the block yet.  */
	      ALL_DICT_SYMBOLS (BLOCK_MULTIDICT (block), miter, sym)
		{
		  if (iparams == nparams)
		    break;

		  if (SYMBOL_IS_ARGUMENT (sym))
		    {
		      ftype->field (iparams).set_type (SYMBOL_TYPE (sym));
		      TYPE_FIELD_ARTIFICIAL (ftype, iparams) = 0;
		      iparams++;
		    }
		}
	    }
	}
    }
  else
    {
      BLOCK_FUNCTION (block) = NULL;
    }

  if (static_link != NULL)
    objfile_register_static_link (m_objfile, block, static_link);

  /* Now free the links of the list, and empty the list.  */

  for (next = *listhead; next; next = next1)
    {
      next1 = next->next;
      xfree (next);
    }
  *listhead = NULL;

  /* Check to be sure that the blocks have an end address that is
     greater than starting address.  */

  if (BLOCK_END (block) < BLOCK_START (block))
    {
      if (symbol)
	{
	  complaint (_("block end address less than block "
		       "start address in %s (patched it)"),
		     symbol->print_name ());
	}
      else
	{
	  complaint (_("block end address %s less than block "
		       "start address %s (patched it)"),
		     paddress (gdbarch, BLOCK_END (block)),
		     paddress (gdbarch, BLOCK_START (block)));
	}
      /* Better than nothing.  */
      BLOCK_END (block) = BLOCK_START (block);
    }

  /* Install this block as the superblock of all blocks made since the
     start of this scope that don't have superblocks yet.  */

  opblock = NULL;
  for (pblock = m_pending_blocks;
       pblock && pblock != old_blocks; 
       pblock = pblock->next)
    {
      if (BLOCK_SUPERBLOCK (pblock->block) == NULL)
	{
	  /* Check to be sure the blocks are nested as we receive
	     them.  If the compiler/assembler/linker work, this just
	     burns a small amount of time.

	     Skip blocks which correspond to a function; they're not
	     physically nested inside this other blocks, only
	     lexically nested.  */
	  if (BLOCK_FUNCTION (pblock->block) == NULL
	      && (BLOCK_START (pblock->block) < BLOCK_START (block)
		  || BLOCK_END (pblock->block) > BLOCK_END (block)))
	    {
	      if (symbol)
		{
		  complaint (_("inner block not inside outer block in %s"),
			     symbol->print_name ());
		}
	      else
		{
		  complaint (_("inner block (%s-%s) not "
			       "inside outer block (%s-%s)"),
			     paddress (gdbarch, BLOCK_START (pblock->block)),
			     paddress (gdbarch, BLOCK_END (pblock->block)),
			     paddress (gdbarch, BLOCK_START (block)),
			     paddress (gdbarch, BLOCK_END (block)));
		}
	      if (BLOCK_START (pblock->block) < BLOCK_START (block))
		BLOCK_START (pblock->block) = BLOCK_START (block);
	      if (BLOCK_END (pblock->block) > BLOCK_END (block))
		BLOCK_END (pblock->block) = BLOCK_END (block);
	    }
	  BLOCK_SUPERBLOCK (pblock->block) = block;
	}
      opblock = pblock;
    }

  block_set_using (block,
		   (is_global
		    ? m_global_using_directives
		    : m_local_using_directives),
		   &m_objfile->objfile_obstack);
  if (is_global)
    m_global_using_directives = NULL;
  else
    m_local_using_directives = NULL;

  record_pending_block (block, opblock);

  return block;
}

struct block *
buildsym_compunit::finish_block (struct symbol *symbol,
				 struct pending_block *old_blocks,
				 const struct dynamic_prop *static_link,
				 CORE_ADDR start, CORE_ADDR end)
{
  return finish_block_internal (symbol, &m_local_symbols,
				old_blocks, static_link, start, end, 0, 0);
}

/* Record that the range of addresses from START to END_INCLUSIVE
   (inclusive, like it says) belongs to BLOCK.  BLOCK's start and end
   addresses must be set already.  You must apply this function to all
   BLOCK's children before applying it to BLOCK.

   If a call to this function complicates the picture beyond that
   already provided by BLOCK_START and BLOCK_END, then we create an
   address map for the block.  */
void
buildsym_compunit::record_block_range (struct block *block,
				       CORE_ADDR start,
				       CORE_ADDR end_inclusive)
{
  /* If this is any different from the range recorded in the block's
     own BLOCK_START and BLOCK_END, then note that the address map has
     become interesting.  Note that even if this block doesn't have
     any "interesting" ranges, some later block might, so we still
     need to record this block in the addrmap.  */
  if (start != BLOCK_START (block)
      || end_inclusive + 1 != BLOCK_END (block))
    m_pending_addrmap_interesting = true;

  if (m_pending_addrmap == nullptr)
    m_pending_addrmap = addrmap_create_mutable (&m_pending_addrmap_obstack);

  addrmap_set_empty (m_pending_addrmap, start, end_inclusive, block);
}

struct blockvector *
buildsym_compunit::make_blockvector ()
{
  struct pending_block *next;
  struct blockvector *blockvector;
  int i;

  /* Count the length of the list of blocks.  */

  for (next = m_pending_blocks, i = 0; next; next = next->next, i++)
    {
    }

  blockvector = (struct blockvector *)
    obstack_alloc (&m_objfile->objfile_obstack,
		   (sizeof (struct blockvector)
		    + (i - 1) * sizeof (struct block *)));

  /* Copy the blocks into the blockvector.  This is done in reverse
     order, which happens to put the blocks into the proper order
     (ascending starting address).  finish_block has hair to insert
     each block into the list after its subblocks in order to make
     sure this is true.  */

  BLOCKVECTOR_NBLOCKS (blockvector) = i;
  for (next = m_pending_blocks; next; next = next->next)
    {
      BLOCKVECTOR_BLOCK (blockvector, --i) = next->block;
    }

  free_pending_blocks ();

  /* If we needed an address map for this symtab, record it in the
     blockvector.  */
  if (m_pending_addrmap != nullptr && m_pending_addrmap_interesting)
    BLOCKVECTOR_MAP (blockvector)
      = addrmap_create_fixed (m_pending_addrmap, &m_objfile->objfile_obstack);
  else
    BLOCKVECTOR_MAP (blockvector) = 0;

  /* Some compilers output blocks in the wrong order, but we depend on
     their being in the right order so we can binary search.  Check the
     order and moan about it.
     Note: Remember that the first two blocks are the global and static
     blocks.  We could special case that fact and begin checking at block 2.
     To avoid making that assumption we do not.  */
  if (BLOCKVECTOR_NBLOCKS (blockvector) > 1)
    {
      for (i = 1; i < BLOCKVECTOR_NBLOCKS (blockvector); i++)
	{
	  if (BLOCK_START (BLOCKVECTOR_BLOCK (blockvector, i - 1))
	      > BLOCK_START (BLOCKVECTOR_BLOCK (blockvector, i)))
	    {
	      CORE_ADDR start
		= BLOCK_START (BLOCKVECTOR_BLOCK (blockvector, i));

	      complaint (_("block at %s out of order"),
			 hex_string ((LONGEST) start));
	    }
	}
    }

  return (blockvector);
}

/* Start recording information about source code that came from an
   included (or otherwise merged-in) source file with a different
   name.  NAME is the name of the file (cannot be NULL).  */

void
buildsym_compunit::start_subfile (const char *name)
{
  const char *subfile_dirname;
  struct subfile *subfile;

  subfile_dirname = m_comp_dir.get ();

  /* See if this subfile is already registered.  */

  for (subfile = m_subfiles; subfile; subfile = subfile->next)
    {
      char *subfile_name;

      /* If NAME is an absolute path, and this subfile is not, then
	 attempt to create an absolute path to compare.  */
      if (IS_ABSOLUTE_PATH (name)
	  && !IS_ABSOLUTE_PATH (subfile->name)
	  && subfile_dirname != NULL)
	subfile_name = concat (subfile_dirname, SLASH_STRING,
			       subfile->name, (char *) NULL);
      else
	subfile_name = subfile->name;

      if (FILENAME_CMP (subfile_name, name) == 0)
	{
	  m_current_subfile = subfile;
	  if (subfile_name != subfile->name)
	    xfree (subfile_name);
	  return;
	}
      if (subfile_name != subfile->name)
	xfree (subfile_name);
    }

  /* This subfile is not known.  Add an entry for it.  */

  subfile = XNEW (struct subfile);
  memset (subfile, 0, sizeof (struct subfile));
  subfile->buildsym_compunit = this;

  subfile->next = m_subfiles;
  m_subfiles = subfile;

  m_current_subfile = subfile;

  subfile->name = xstrdup (name);

  /* Initialize line-number recording for this subfile.  */
  subfile->line_vector = NULL;

  /* Default the source language to whatever can be deduced from the
     filename.  If nothing can be deduced (such as for a C/C++ include
     file with a ".h" extension), then inherit whatever language the
     previous subfile had.  This kludgery is necessary because there
     is no standard way in some object formats to record the source
     language.  Also, when symtabs are allocated we try to deduce a
     language then as well, but it is too late for us to use that
     information while reading symbols, since symtabs aren't allocated
     until after all the symbols have been processed for a given
     source file.  */

  subfile->language = deduce_language_from_filename (subfile->name);
  if (subfile->language == language_unknown
      && subfile->next != NULL)
    {
      subfile->language = subfile->next->language;
    }

  /* If the filename of this subfile ends in .C, then change the
     language of any pending subfiles from C to C++.  We also accept
     any other C++ suffixes accepted by deduce_language_from_filename.  */
  /* Likewise for f2c.  */

  if (subfile->name)
    {
      struct subfile *s;
      enum language sublang = deduce_language_from_filename (subfile->name);

      if (sublang == language_cplus || sublang == language_fortran)
	for (s = m_subfiles; s != NULL; s = s->next)
	  if (s->language == language_c)
	    s->language = sublang;
    }

  /* And patch up this file if necessary.  */
  if (subfile->language == language_c
      && subfile->next != NULL
      && (subfile->next->language == language_cplus
	  || subfile->next->language == language_fortran))
    {
      subfile->language = subfile->next->language;
    }
}

/* For stabs readers, the first N_SO symbol is assumed to be the
   source file name, and the subfile struct is initialized using that
   assumption.  If another N_SO symbol is later seen, immediately
   following the first one, then the first one is assumed to be the
   directory name and the second one is really the source file name.

   So we have to patch up the subfile struct by moving the old name
   value to dirname and remembering the new name.  Some sanity
   checking is performed to ensure that the state of the subfile
   struct is reasonable and that the old name we are assuming to be a
   directory name actually is (by checking for a trailing '/').  */

void
buildsym_compunit::patch_subfile_names (struct subfile *subfile,
					const char *name)
{
  if (subfile != NULL
      && m_comp_dir == NULL
      && subfile->name != NULL
      && IS_DIR_SEPARATOR (subfile->name[strlen (subfile->name) - 1]))
    {
      m_comp_dir.reset (subfile->name);
      subfile->name = xstrdup (name);
      set_last_source_file (name);

      /* Default the source language to whatever can be deduced from
         the filename.  If nothing can be deduced (such as for a C/C++
         include file with a ".h" extension), then inherit whatever
         language the previous subfile had.  This kludgery is
         necessary because there is no standard way in some object
         formats to record the source language.  Also, when symtabs
         are allocated we try to deduce a language then as well, but
         it is too late for us to use that information while reading
         symbols, since symtabs aren't allocated until after all the
         symbols have been processed for a given source file.  */

      subfile->language = deduce_language_from_filename (subfile->name);
      if (subfile->language == language_unknown
	  && subfile->next != NULL)
	{
	  subfile->language = subfile->next->language;
	}
    }
}

/* Handle the N_BINCL and N_EINCL symbol types that act like N_SOL for
   switching source files (different subfiles, as we call them) within
   one object file, but using a stack rather than in an arbitrary
   order.  */

void
buildsym_compunit::push_subfile ()
{
  gdb_assert (m_current_subfile != NULL);
  gdb_assert (m_current_subfile->name != NULL);
  m_subfile_stack.push_back (m_current_subfile->name);
}

const char *
buildsym_compunit::pop_subfile ()
{
  gdb_assert (!m_subfile_stack.empty ());
  const char *name = m_subfile_stack.back ();
  m_subfile_stack.pop_back ();
  return name;
}

/* Add a linetable entry for line number LINE and address PC to the
   line vector for SUBFILE.  */

void
buildsym_compunit::record_line (struct subfile *subfile, int line,
				CORE_ADDR pc, bool is_stmt)
{
  struct linetable_entry *e;

  /* Make sure line vector exists and is big enough.  */
  if (!subfile->line_vector)
    {
      subfile->line_vector_length = INITIAL_LINE_VECTOR_LENGTH;
      subfile->line_vector = (struct linetable *)
	xmalloc (sizeof (struct linetable)
	   + subfile->line_vector_length * sizeof (struct linetable_entry));
      subfile->line_vector->nitems = 0;
      m_have_line_numbers = true;
    }

  if (subfile->line_vector->nitems >= subfile->line_vector_length)
    {
      subfile->line_vector_length *= 2;
      subfile->line_vector = (struct linetable *)
	xrealloc ((char *) subfile->line_vector,
		  (sizeof (struct linetable)
		   + (subfile->line_vector_length
		      * sizeof (struct linetable_entry))));
    }

  /* Normally, we treat lines as unsorted.  But the end of sequence
     marker is special.  We sort line markers at the same PC by line
     number, so end of sequence markers (which have line == 0) appear
     first.  This is right if the marker ends the previous function,
     and there is no padding before the next function.  But it is
     wrong if the previous line was empty and we are now marking a
     switch to a different subfile.  We must leave the end of sequence
     marker at the end of this group of lines, not sort the empty line
     to after the marker.  The easiest way to accomplish this is to
     delete any empty lines from our table, if they are followed by
     end of sequence markers.  All we lose is the ability to set
     breakpoints at some lines which contain no instructions
     anyway.  */
  if (line == 0)
    {
      while (subfile->line_vector->nitems > 0)
	{
	  e = subfile->line_vector->item + subfile->line_vector->nitems - 1;
	  if (e->pc != pc)
	    break;
	  subfile->line_vector->nitems--;
	}
    }

  e = subfile->line_vector->item + subfile->line_vector->nitems++;
  e->line = line;
  e->is_stmt = is_stmt ? 1 : 0;
  e->pc = pc;
}


/* Subroutine of end_symtab to simplify it.  Look for a subfile that
   matches the main source file's basename.  If there is only one, and
   if the main source file doesn't have any symbol or line number
   information, then copy this file's symtab and line_vector to the
   main source file's subfile and discard the other subfile.  This can
   happen because of a compiler bug or from the user playing games
   with #line or from things like a distributed build system that
   manipulates the debug info.  This can also happen from an innocent
   symlink in the paths, we don't canonicalize paths here.  */

void
buildsym_compunit::watch_main_source_file_lossage ()
{
  struct subfile *mainsub, *subfile;

  /* Get the main source file.  */
  mainsub = m_main_subfile;

  /* If the main source file doesn't have any line number or symbol
     info, look for an alias in another subfile.  */

  if (mainsub->line_vector == NULL
      && mainsub->symtab == NULL)
    {
      const char *mainbase = lbasename (mainsub->name);
      int nr_matches = 0;
      struct subfile *prevsub;
      struct subfile *mainsub_alias = NULL;
      struct subfile *prev_mainsub_alias = NULL;

      prevsub = NULL;
      for (subfile = m_subfiles;
	   subfile != NULL;
	   subfile = subfile->next)
	{
	  if (subfile == mainsub)
	    continue;
	  if (filename_cmp (lbasename (subfile->name), mainbase) == 0)
	    {
	      ++nr_matches;
	      mainsub_alias = subfile;
	      prev_mainsub_alias = prevsub;
	    }
	  prevsub = subfile;
	}

      if (nr_matches == 1)
	{
	  gdb_assert (mainsub_alias != NULL && mainsub_alias != mainsub);

	  /* Found a match for the main source file.
	     Copy its line_vector and symtab to the main subfile
	     and then discard it.  */

	  mainsub->line_vector = mainsub_alias->line_vector;
	  mainsub->line_vector_length = mainsub_alias->line_vector_length;
	  mainsub->symtab = mainsub_alias->symtab;

	  if (prev_mainsub_alias == NULL)
	    m_subfiles = mainsub_alias->next;
	  else
	    prev_mainsub_alias->next = mainsub_alias->next;
	  xfree (mainsub_alias->name);
	  xfree (mainsub_alias);
	}
    }
}

/* Implementation of the first part of end_symtab.  It allows modifying
   STATIC_BLOCK before it gets finalized by end_symtab_from_static_block.
   If the returned value is NULL there is no blockvector created for
   this symtab (you still must call end_symtab_from_static_block).

   END_ADDR is the same as for end_symtab: the address of the end of the
   file's text.

   If EXPANDABLE is non-zero the STATIC_BLOCK dictionary is made
   expandable.

   If REQUIRED is non-zero, then a symtab is created even if it does
   not contain any symbols.  */

struct block *
buildsym_compunit::end_symtab_get_static_block (CORE_ADDR end_addr,
						int expandable, int required)
{
  /* Finish the lexical context of the last function in the file; pop
     the context stack.  */

  if (!m_context_stack.empty ())
    {
      struct context_stack cstk = pop_context ();

      /* Make a block for the local symbols within.  */
      finish_block (cstk.name, cstk.old_blocks, NULL,
		    cstk.start_addr, end_addr);

      if (!m_context_stack.empty ())
	{
	  /* This is said to happen with SCO.  The old coffread.c
	     code simply emptied the context stack, so we do the
	     same.  FIXME: Find out why it is happening.  This is not
	     believed to happen in most cases (even for coffread.c);
	     it used to be an abort().  */
	  complaint (_("Context stack not empty in end_symtab"));
	  m_context_stack.clear ();
	}
    }

  /* Reordered executables may have out of order pending blocks; if
     OBJF_REORDERED is true, then sort the pending blocks.  */

  if ((m_objfile->flags & OBJF_REORDERED) && m_pending_blocks)
    {
      struct pending_block *pb;

      std::vector<block *> barray;

      for (pb = m_pending_blocks; pb != NULL; pb = pb->next)
	barray.push_back (pb->block);

      /* Sort blocks by start address in descending order.  Blocks with the
	 same start address must remain in the original order to preserve
	 inline function caller/callee relationships.  */
      std::stable_sort (barray.begin (), barray.end (),
			[] (const block *a, const block *b)
			{
			  return BLOCK_START (a) > BLOCK_START (b);
			});

      int i = 0;
      for (pb = m_pending_blocks; pb != NULL; pb = pb->next)
	pb->block = barray[i++];
    }

  /* Cleanup any undefined types that have been left hanging around
     (this needs to be done before the finish_blocks so that
     file_symbols is still good).

     Both cleanup_undefined_stabs_types and finish_global_stabs are stabs
     specific, but harmless for other symbol readers, since on gdb
     startup or when finished reading stabs, the state is set so these
     are no-ops.  FIXME: Is this handled right in case of QUIT?  Can
     we make this cleaner?  */

  cleanup_undefined_stabs_types (m_objfile);
  finish_global_stabs (m_objfile);

  if (!required
      && m_pending_blocks == NULL
      && m_file_symbols == NULL
      && m_global_symbols == NULL
      && !m_have_line_numbers
      && m_pending_macros == NULL
      && m_global_using_directives == NULL)
    {
      /* Ignore symtabs that have no functions with real debugging info.  */
      return NULL;
    }
  else
    {
      /* Define the STATIC_BLOCK.  */
      return finish_block_internal (NULL, get_file_symbols (), NULL, NULL,
				    m_last_source_start_addr,
				    end_addr, 0, expandable);
    }
}

/* Subroutine of end_symtab_from_static_block to simplify it.
   Handle the "have blockvector" case.
   See end_symtab_from_static_block for a description of the arguments.  */

struct compunit_symtab *
buildsym_compunit::end_symtab_with_blockvector (struct block *static_block,
						int section, int expandable)
{
  struct compunit_symtab *cu = m_compunit_symtab;
  struct blockvector *blockvector;
  struct subfile *subfile;
  CORE_ADDR end_addr;

  gdb_assert (static_block != NULL);
  gdb_assert (m_subfiles != NULL);

  end_addr = BLOCK_END (static_block);

  /* Create the GLOBAL_BLOCK and build the blockvector.  */
  finish_block_internal (NULL, get_global_symbols (), NULL, NULL,
			 m_last_source_start_addr, end_addr,
			 1, expandable);
  blockvector = make_blockvector ();

  /* Read the line table if it has to be read separately.
     This is only used by xcoffread.c.  */
  if (m_objfile->sf->sym_read_linetable != NULL)
    m_objfile->sf->sym_read_linetable (m_objfile);

  /* Handle the case where the debug info specifies a different path
     for the main source file.  It can cause us to lose track of its
     line number information.  */
  watch_main_source_file_lossage ();

  /* Now create the symtab objects proper, if not already done,
     one for each subfile.  */

  for (subfile = m_subfiles;
       subfile != NULL;
       subfile = subfile->next)
    {
      int linetablesize = 0;

      if (subfile->line_vector)
	{
	  linetablesize = sizeof (struct linetable) +
	    subfile->line_vector->nitems * sizeof (struct linetable_entry);

	  const auto lte_is_less_than
	    = [] (const linetable_entry &ln1,
		  const linetable_entry &ln2) -> bool
	      {
		if (ln1.pc == ln2.pc
		    && ((ln1.line == 0) != (ln2.line == 0)))
		  return ln1.line == 0;

		return (ln1.pc < ln2.pc);
	      };

	  /* Like the pending blocks, the line table may be scrambled in
	     reordered executables.  Sort it if OBJF_REORDERED is true.  It
	     is important to preserve the order of lines at the same
	     address, as this maintains the inline function caller/callee
	     relationships, this is why std::stable_sort is used.  */
	  if (m_objfile->flags & OBJF_REORDERED)
	    std::stable_sort (subfile->line_vector->item,
			      subfile->line_vector->item
			      + subfile->line_vector->nitems,
			      lte_is_less_than);
	}

      /* Allocate a symbol table if necessary.  */
      if (subfile->symtab == NULL)
	subfile->symtab = allocate_symtab (cu, subfile->name);
      struct symtab *symtab = subfile->symtab;

      /* Fill in its components.  */

      if (subfile->line_vector)
	{
	  /* Reallocate the line table on the symbol obstack.  */
	  SYMTAB_LINETABLE (symtab) = (struct linetable *)
	    obstack_alloc (&m_objfile->objfile_obstack, linetablesize);
	  memcpy (SYMTAB_LINETABLE (symtab), subfile->line_vector,
		  linetablesize);
	}
      else
	{
	  SYMTAB_LINETABLE (symtab) = NULL;
	}

      /* Use whatever language we have been using for this
	 subfile, not the one that was deduced in allocate_symtab
	 from the filename.  We already did our own deducing when
	 we created the subfile, and we may have altered our
	 opinion of what language it is from things we found in
	 the symbols.  */
      symtab->language = subfile->language;
    }

  /* Make sure the symtab of main_subfile is the first in its list.  */
  {
    struct symtab *main_symtab, *prev_symtab;

    main_symtab = m_main_subfile->symtab;
    prev_symtab = NULL;
    for (symtab *symtab : compunit_filetabs (cu))
      {
	if (symtab == main_symtab)
	  {
	    if (prev_symtab != NULL)
	      {
		prev_symtab->next = main_symtab->next;
		main_symtab->next = COMPUNIT_FILETABS (cu);
		COMPUNIT_FILETABS (cu) = main_symtab;
	      }
	    break;
	  }
	prev_symtab = symtab;
      }
    gdb_assert (main_symtab == COMPUNIT_FILETABS (cu));
  }

  /* Fill out the compunit symtab.  */

  if (m_comp_dir != NULL)
    {
      /* Reallocate the dirname on the symbol obstack.  */
      const char *comp_dir = m_comp_dir.get ();
      COMPUNIT_DIRNAME (cu) = obstack_strdup (&m_objfile->objfile_obstack,
					      comp_dir);
    }

  /* Save the debug format string (if any) in the symtab.  */
  COMPUNIT_DEBUGFORMAT (cu) = m_debugformat;

  /* Similarly for the producer.  */
  COMPUNIT_PRODUCER (cu) = m_producer;

  COMPUNIT_BLOCKVECTOR (cu) = blockvector;
  {
    struct block *b = BLOCKVECTOR_BLOCK (blockvector, GLOBAL_BLOCK);

    set_block_compunit_symtab (b, cu);
  }

  COMPUNIT_BLOCK_LINE_SECTION (cu) = section;

  COMPUNIT_MACRO_TABLE (cu) = release_macros ();

  /* Default any symbols without a specified symtab to the primary symtab.  */
  {
    int block_i;

    /* The main source file's symtab.  */
    struct symtab *symtab = COMPUNIT_FILETABS (cu);

    for (block_i = 0; block_i < BLOCKVECTOR_NBLOCKS (blockvector); block_i++)
      {
	struct block *block = BLOCKVECTOR_BLOCK (blockvector, block_i);
	struct symbol *sym;
	struct mdict_iterator miter;

	/* Inlined functions may have symbols not in the global or
	   static symbol lists.  */
	if (BLOCK_FUNCTION (block) != NULL)
	  if (symbol_symtab (BLOCK_FUNCTION (block)) == NULL)
	    symbol_set_symtab (BLOCK_FUNCTION (block), symtab);

	/* Note that we only want to fix up symbols from the local
	   blocks, not blocks coming from included symtabs.  That is why
	   we use ALL_DICT_SYMBOLS here and not ALL_BLOCK_SYMBOLS.  */
	ALL_DICT_SYMBOLS (BLOCK_MULTIDICT (block), miter, sym)
	  if (symbol_symtab (sym) == NULL)
	    symbol_set_symtab (sym, symtab);
      }
  }

  add_compunit_symtab_to_objfile (cu);

  return cu;
}

/* Implementation of the second part of end_symtab.  Pass STATIC_BLOCK
   as value returned by end_symtab_get_static_block.

   SECTION is the same as for end_symtab: the section number
   (in objfile->section_offsets) of the blockvector and linetable.

   If EXPANDABLE is non-zero the GLOBAL_BLOCK dictionary is made
   expandable.  */

struct compunit_symtab *
buildsym_compunit::end_symtab_from_static_block (struct block *static_block,
						 int section, int expandable)
{
  struct compunit_symtab *cu;

  if (static_block == NULL)
    {
      /* Handle the "no blockvector" case.
	 When this happens there is nothing to record, so there's nothing
	 to do: memory will be freed up later.

	 Note: We won't be adding a compunit to the objfile's list of
	 compunits, so there's nothing to unchain.  However, since each symtab
	 is added to the objfile's obstack we can't free that space.
	 We could do better, but this is believed to be a sufficiently rare
	 event.  */
      cu = NULL;
    }
  else
    cu = end_symtab_with_blockvector (static_block, section, expandable);

  return cu;
}

/* Finish the symbol definitions for one main source file, close off
   all the lexical contexts for that file (creating struct block's for
   them), then make the struct symtab for that file and put it in the
   list of all such.

   END_ADDR is the address of the end of the file's text.  SECTION is
   the section number (in objfile->section_offsets) of the blockvector
   and linetable.

   Note that it is possible for end_symtab() to return NULL.  In
   particular, for the DWARF case at least, it will return NULL when
   it finds a compilation unit that has exactly one DIE, a
   TAG_compile_unit DIE.  This can happen when we link in an object
   file that was compiled from an empty source file.  Returning NULL
   is probably not the correct thing to do, because then gdb will
   never know about this empty file (FIXME).

   If you need to modify STATIC_BLOCK before it is finalized you should
   call end_symtab_get_static_block and end_symtab_from_static_block
   yourself.  */

struct compunit_symtab *
buildsym_compunit::end_symtab (CORE_ADDR end_addr, int section)
{
  struct block *static_block;

  static_block = end_symtab_get_static_block (end_addr, 0, 0);
  return end_symtab_from_static_block (static_block, section, 0);
}

/* Same as end_symtab except create a symtab that can be later added to.  */

struct compunit_symtab *
buildsym_compunit::end_expandable_symtab (CORE_ADDR end_addr, int section)
{
  struct block *static_block;

  static_block = end_symtab_get_static_block (end_addr, 1, 0);
  return end_symtab_from_static_block (static_block, section, 1);
}

/* Subroutine of augment_type_symtab to simplify it.
   Attach the main source file's symtab to all symbols in PENDING_LIST that
   don't have one.  */

static void
set_missing_symtab (struct pending *pending_list,
		    struct compunit_symtab *cu)
{
  struct pending *pending;
  int i;

  for (pending = pending_list; pending != NULL; pending = pending->next)
    {
      for (i = 0; i < pending->nsyms; ++i)
	{
	  if (symbol_symtab (pending->symbol[i]) == NULL)
	    symbol_set_symtab (pending->symbol[i], COMPUNIT_FILETABS (cu));
	}
    }
}

/* Same as end_symtab, but for the case where we're adding more symbols
   to an existing symtab that is known to contain only type information.
   This is the case for DWARF4 Type Units.  */

void
buildsym_compunit::augment_type_symtab ()
{
  struct compunit_symtab *cust = m_compunit_symtab;
  const struct blockvector *blockvector = COMPUNIT_BLOCKVECTOR (cust);

  if (!m_context_stack.empty ())
    complaint (_("Context stack not empty in augment_type_symtab"));
  if (m_pending_blocks != NULL)
    complaint (_("Blocks in a type symtab"));
  if (m_pending_macros != NULL)
    complaint (_("Macro in a type symtab"));
  if (m_have_line_numbers)
    complaint (_("Line numbers recorded in a type symtab"));

  if (m_file_symbols != NULL)
    {
      struct block *block = BLOCKVECTOR_BLOCK (blockvector, STATIC_BLOCK);

      /* First mark any symbols without a specified symtab as belonging
	 to the primary symtab.  */
      set_missing_symtab (m_file_symbols, cust);

      mdict_add_pending (BLOCK_MULTIDICT (block), m_file_symbols);
    }

  if (m_global_symbols != NULL)
    {
      struct block *block = BLOCKVECTOR_BLOCK (blockvector, GLOBAL_BLOCK);

      /* First mark any symbols without a specified symtab as belonging
	 to the primary symtab.  */
      set_missing_symtab (m_global_symbols, cust);

      mdict_add_pending (BLOCK_MULTIDICT (block),
			m_global_symbols);
    }
}

/* Push a context block.  Args are an identifying nesting level
   (checkable when you pop it), and the starting PC address of this
   context.  */

struct context_stack *
buildsym_compunit::push_context (int desc, CORE_ADDR valu)
{
  m_context_stack.emplace_back ();
  struct context_stack *newobj = &m_context_stack.back ();

  newobj->depth = desc;
  newobj->locals = m_local_symbols;
  newobj->old_blocks = m_pending_blocks;
  newobj->start_addr = valu;
  newobj->local_using_directives = m_local_using_directives;
  newobj->name = NULL;

  m_local_symbols = NULL;
  m_local_using_directives = NULL;

  return newobj;
}

/* Pop a context block.  Returns the address of the context block just
   popped.  */

struct context_stack
buildsym_compunit::pop_context ()
{
  gdb_assert (!m_context_stack.empty ());
  struct context_stack result = m_context_stack.back ();
  m_context_stack.pop_back ();
  return result;
}
