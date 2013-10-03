/* Support routines for building symbol tables in GDB's internal format.
   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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

/* This module provides subroutines used for creating and adding to
   the symbol table.  These routines are called from various symbol-
   file-reading routines.

   Routines to support specific debugging information formats (stabs,
   DWARF, etc) belong somewhere else.  */

#include "defs.h"
#include "bfd.h"
#include "gdb_obstack.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "gdb_assert.h"
#include "complaints.h"
#include "gdb_string.h"
#include "expression.h"		/* For "enum exp_opcode" used by...  */
#include "bcache.h"
#include "filenames.h"		/* For DOSish file names.  */
#include "macrotab.h"
#include "demangle.h"		/* Needed by SYMBOL_INIT_DEMANGLED_NAME.  */
#include "block.h"
#include "cp-support.h"
#include "dictionary.h"
#include "addrmap.h"

/* Ask buildsym.h to define the vars it normally declares `extern'.  */
#define	EXTERN
/**/
#include "buildsym.h"		/* Our own declarations.  */
#undef	EXTERN

/* For cleanup_undefined_stabs_types and finish_global_stabs (somewhat
   questionable--see comment where we call them).  */

#include "stabsread.h"

/* List of subfiles.  */

static struct subfile *subfiles;

/* List of free `struct pending' structures for reuse.  */

static struct pending *free_pendings;

/* Non-zero if symtab has line number info.  This prevents an
   otherwise empty symtab from being tossed.  */

static int have_line_numbers;

/* The mutable address map for the compilation unit whose symbols
   we're currently reading.  The symtabs' shared blockvector will
   point to a fixed copy of this.  */
static struct addrmap *pending_addrmap;

/* The obstack on which we allocate pending_addrmap.
   If pending_addrmap is NULL, this is uninitialized; otherwise, it is
   initialized (and holds pending_addrmap).  */
static struct obstack pending_addrmap_obstack;

/* Non-zero if we recorded any ranges in the addrmap that are
   different from those in the blockvector already.  We set this to
   zero when we start processing a symfile, and if it's still zero at
   the end, then we just toss the addrmap.  */
static int pending_addrmap_interesting;

/* An obstack used for allocating pending blocks.  */

static struct obstack pending_block_obstack;

/* List of blocks already made (lexical contexts already closed).
   This is used at the end to make the blockvector.  */

struct pending_block
  {
    struct pending_block *next;
    struct block *block;
  };

/* Pointer to the head of a linked list of symbol blocks which have
   already been finalized (lexical contexts already closed) and which
   are just waiting to be built into a blockvector when finalizing the
   associated symtab.  */

static struct pending_block *pending_blocks;

static int compare_line_numbers (const void *ln1p, const void *ln2p);

static void record_pending_block (struct objfile *objfile,
				  struct block *block,
				  struct pending_block *opblock);


/* Initial sizes of data structures.  These are realloc'd larger if
   needed, and realloc'd down to the size actually used, when
   completed.  */

#define	INITIAL_CONTEXT_STACK_SIZE	10
#define	INITIAL_LINE_VECTOR_LENGTH	1000


/* Maintain the lists of symbols and blocks.  */

/* Add a symbol to one of the lists of symbols.  */

void
add_symbol_to_list (struct symbol *symbol, struct pending **listhead)
{
  struct pending *link;

  /* If this is an alias for another symbol, don't add it.  */
  if (symbol->ginfo.name && symbol->ginfo.name[0] == '#')
    return;

  /* We keep PENDINGSIZE symbols in each link of the list.  If we
     don't have a link with room in it, add a new link.  */
  if (*listhead == NULL || (*listhead)->nsyms == PENDINGSIZE)
    {
      if (free_pendings)
	{
	  link = free_pendings;
	  free_pendings = link->next;
	}
      else
	{
	  link = (struct pending *) xmalloc (sizeof (struct pending));
	}

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
	  pp = SYMBOL_LINKAGE_NAME (list->symbol[j]);
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

/* At end of reading syms, or in case of quit, really free as many
   `struct pending's as we can easily find.  */

void
really_free_pendings (void *dummy)
{
  struct pending *next, *next1;

  for (next = free_pendings; next; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }
  free_pendings = NULL;

  free_pending_blocks ();

  for (next = file_symbols; next != NULL; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }
  file_symbols = NULL;

  for (next = global_symbols; next != NULL; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }
  global_symbols = NULL;

  if (pending_macros)
    free_macro_table (pending_macros);

  if (pending_addrmap)
    {
      obstack_free (&pending_addrmap_obstack, NULL);
      pending_addrmap = NULL;
    }
}

/* This function is called to discard any pending blocks.  */

void
free_pending_blocks (void)
{
  if (pending_blocks != NULL)
    {
      obstack_free (&pending_block_obstack, NULL);
      pending_blocks = NULL;
    }
}

/* Take one of the lists of symbols and make a block from it.  Keep
   the order the symbols have in the list (reversed from the input
   file).  Put the block on the list of pending blocks.  */

static struct block *
finish_block_internal (struct symbol *symbol, struct pending **listhead,
		       struct pending_block *old_blocks,
		       CORE_ADDR start, CORE_ADDR end,
		       struct objfile *objfile,
		       int is_global, int expandable)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  struct pending *next, *next1;
  struct block *block;
  struct pending_block *pblock;
  struct pending_block *opblock;

  block = (is_global
	   ? allocate_global_block (&objfile->objfile_obstack)
	   : allocate_block (&objfile->objfile_obstack));

  if (symbol)
    {
      BLOCK_DICT (block) = dict_create_linear (&objfile->objfile_obstack,
					       *listhead);
    }
  else
    {
      if (expandable)
	{
	  BLOCK_DICT (block) = dict_create_hashed_expandable ();
	  dict_add_pending (BLOCK_DICT (block), *listhead);
	}
      else
	{
	  BLOCK_DICT (block) =
	    dict_create_hashed (&objfile->objfile_obstack, *listhead);
	}
    }

  BLOCK_START (block) = start;
  BLOCK_END (block) = end;

  /* Put the block in as the value of the symbol that names it.  */

  if (symbol)
    {
      struct type *ftype = SYMBOL_TYPE (symbol);
      struct dict_iterator iter;
      SYMBOL_BLOCK_VALUE (symbol) = block;
      BLOCK_FUNCTION (block) = symbol;

      if (TYPE_NFIELDS (ftype) <= 0)
	{
	  /* No parameter type information is recorded with the
	     function's type.  Set that from the type of the
	     parameter symbols.  */
	  int nparams = 0, iparams;
	  struct symbol *sym;

	  /* Here we want to directly access the dictionary, because
	     we haven't fully initialized the block yet.  */
	  ALL_DICT_SYMBOLS (BLOCK_DICT (block), iter, sym)
	    {
	      if (SYMBOL_IS_ARGUMENT (sym))
		nparams++;
	    }
	  if (nparams > 0)
	    {
	      TYPE_NFIELDS (ftype) = nparams;
	      TYPE_FIELDS (ftype) = (struct field *)
		TYPE_ALLOC (ftype, nparams * sizeof (struct field));

	      iparams = 0;
	      /* Here we want to directly access the dictionary, because
		 we haven't fully initialized the block yet.  */
	      ALL_DICT_SYMBOLS (BLOCK_DICT (block), iter, sym)
		{
		  if (iparams == nparams)
		    break;

		  if (SYMBOL_IS_ARGUMENT (sym))
		    {
		      TYPE_FIELD_TYPE (ftype, iparams) = SYMBOL_TYPE (sym);
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

  /* Now "free" the links of the list, and empty the list.  */

  for (next = *listhead; next; next = next1)
    {
      next1 = next->next;
      next->next = free_pendings;
      free_pendings = next;
    }
  *listhead = NULL;

  /* Check to be sure that the blocks have an end address that is
     greater than starting address.  */

  if (BLOCK_END (block) < BLOCK_START (block))
    {
      if (symbol)
	{
	  complaint (&symfile_complaints,
		     _("block end address less than block "
		       "start address in %s (patched it)"),
		     SYMBOL_PRINT_NAME (symbol));
	}
      else
	{
	  complaint (&symfile_complaints,
		     _("block end address %s less than block "
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
  for (pblock = pending_blocks; 
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
		  complaint (&symfile_complaints,
			     _("inner block not inside outer block in %s"),
			     SYMBOL_PRINT_NAME (symbol));
		}
	      else
		{
		  complaint (&symfile_complaints,
			     _("inner block (%s-%s) not "
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

  block_set_using (block, using_directives, &objfile->objfile_obstack);
  using_directives = NULL;

  record_pending_block (objfile, block, opblock);

  return block;
}

struct block *
finish_block (struct symbol *symbol, struct pending **listhead,
	      struct pending_block *old_blocks,
	      CORE_ADDR start, CORE_ADDR end,
	      struct objfile *objfile)
{
  return finish_block_internal (symbol, listhead, old_blocks,
				start, end, objfile, 0, 0);
}

/* Record BLOCK on the list of all blocks in the file.  Put it after
   OPBLOCK, or at the beginning if opblock is NULL.  This puts the
   block in the list after all its subblocks.

   Allocate the pending block struct in the objfile_obstack to save
   time.  This wastes a little space.  FIXME: Is it worth it?  */

static void
record_pending_block (struct objfile *objfile, struct block *block,
		      struct pending_block *opblock)
{
  struct pending_block *pblock;

  if (pending_blocks == NULL)
    obstack_init (&pending_block_obstack);

  pblock = (struct pending_block *)
    obstack_alloc (&pending_block_obstack, sizeof (struct pending_block));
  pblock->block = block;
  if (opblock)
    {
      pblock->next = opblock->next;
      opblock->next = pblock;
    }
  else
    {
      pblock->next = pending_blocks;
      pending_blocks = pblock;
    }
}


/* Record that the range of addresses from START to END_INCLUSIVE
   (inclusive, like it says) belongs to BLOCK.  BLOCK's start and end
   addresses must be set already.  You must apply this function to all
   BLOCK's children before applying it to BLOCK.

   If a call to this function complicates the picture beyond that
   already provided by BLOCK_START and BLOCK_END, then we create an
   address map for the block.  */
void
record_block_range (struct block *block,
                    CORE_ADDR start, CORE_ADDR end_inclusive)
{
  /* If this is any different from the range recorded in the block's
     own BLOCK_START and BLOCK_END, then note that the address map has
     become interesting.  Note that even if this block doesn't have
     any "interesting" ranges, some later block might, so we still
     need to record this block in the addrmap.  */
  if (start != BLOCK_START (block)
      || end_inclusive + 1 != BLOCK_END (block))
    pending_addrmap_interesting = 1;

  if (! pending_addrmap)
    {
      obstack_init (&pending_addrmap_obstack);
      pending_addrmap = addrmap_create_mutable (&pending_addrmap_obstack);
    }

  addrmap_set_empty (pending_addrmap, start, end_inclusive, block);
}


static struct blockvector *
make_blockvector (struct objfile *objfile)
{
  struct pending_block *next;
  struct blockvector *blockvector;
  int i;

  /* Count the length of the list of blocks.  */

  for (next = pending_blocks, i = 0; next; next = next->next, i++)
    {;
    }

  blockvector = (struct blockvector *)
    obstack_alloc (&objfile->objfile_obstack,
		   (sizeof (struct blockvector)
		    + (i - 1) * sizeof (struct block *)));

  /* Copy the blocks into the blockvector.  This is done in reverse
     order, which happens to put the blocks into the proper order
     (ascending starting address).  finish_block has hair to insert
     each block into the list after its subblocks in order to make
     sure this is true.  */

  BLOCKVECTOR_NBLOCKS (blockvector) = i;
  for (next = pending_blocks; next; next = next->next)
    {
      BLOCKVECTOR_BLOCK (blockvector, --i) = next->block;
    }

  free_pending_blocks ();

  /* If we needed an address map for this symtab, record it in the
     blockvector.  */
  if (pending_addrmap && pending_addrmap_interesting)
    BLOCKVECTOR_MAP (blockvector)
      = addrmap_create_fixed (pending_addrmap, &objfile->objfile_obstack);
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

	      complaint (&symfile_complaints, _("block at %s out of order"),
			 hex_string ((LONGEST) start));
	    }
	}
    }

  return (blockvector);
}

/* Start recording information about source code that came from an
   included (or otherwise merged-in) source file with a different
   name.  NAME is the name of the file (cannot be NULL), DIRNAME is
   the directory in which the file was compiled (or NULL if not
   known).  */

void
start_subfile (const char *name, const char *dirname)
{
  struct subfile *subfile;

  /* See if this subfile is already known as a subfile of the current
     main source file.  */

  for (subfile = subfiles; subfile; subfile = subfile->next)
    {
      char *subfile_name;

      /* If NAME is an absolute path, and this subfile is not, then
	 attempt to create an absolute path to compare.  */
      if (IS_ABSOLUTE_PATH (name)
	  && !IS_ABSOLUTE_PATH (subfile->name)
	  && subfile->dirname != NULL)
	subfile_name = concat (subfile->dirname, SLASH_STRING,
			       subfile->name, (char *) NULL);
      else
	subfile_name = subfile->name;

      if (FILENAME_CMP (subfile_name, name) == 0)
	{
	  current_subfile = subfile;
	  if (subfile_name != subfile->name)
	    xfree (subfile_name);
	  return;
	}
      if (subfile_name != subfile->name)
	xfree (subfile_name);
    }

  /* This subfile is not known.  Add an entry for it.  Make an entry
     for this subfile in the list of all subfiles of the current main
     source file.  */

  subfile = (struct subfile *) xmalloc (sizeof (struct subfile));
  memset ((char *) subfile, 0, sizeof (struct subfile));
  subfile->next = subfiles;
  subfiles = subfile;
  current_subfile = subfile;

  /* Save its name and compilation directory name.  */
  subfile->name = xstrdup (name);
  subfile->dirname = (dirname == NULL) ? NULL : xstrdup (dirname);

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

  /* Initialize the debug format string to NULL.  We may supply it
     later via a call to record_debugformat.  */
  subfile->debugformat = NULL;

  /* Similarly for the producer.  */
  subfile->producer = NULL;

  /* If the filename of this subfile ends in .C, then change the
     language of any pending subfiles from C to C++.  We also accept
     any other C++ suffixes accepted by deduce_language_from_filename.  */
  /* Likewise for f2c.  */

  if (subfile->name)
    {
      struct subfile *s;
      enum language sublang = deduce_language_from_filename (subfile->name);

      if (sublang == language_cplus || sublang == language_fortran)
	for (s = subfiles; s != NULL; s = s->next)
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
patch_subfile_names (struct subfile *subfile, char *name)
{
  if (subfile != NULL && subfile->dirname == NULL && subfile->name != NULL
      && IS_DIR_SEPARATOR (subfile->name[strlen (subfile->name) - 1]))
    {
      subfile->dirname = subfile->name;
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
push_subfile (void)
{
  struct subfile_stack *tem
    = (struct subfile_stack *) xmalloc (sizeof (struct subfile_stack));

  tem->next = subfile_stack;
  subfile_stack = tem;
  if (current_subfile == NULL || current_subfile->name == NULL)
    {
      internal_error (__FILE__, __LINE__, 
		      _("failed internal consistency check"));
    }
  tem->name = current_subfile->name;
}

char *
pop_subfile (void)
{
  char *name;
  struct subfile_stack *link = subfile_stack;

  if (link == NULL)
    {
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));
    }
  name = link->name;
  subfile_stack = link->next;
  xfree ((void *) link);
  return (name);
}

/* Add a linetable entry for line number LINE and address PC to the
   line vector for SUBFILE.  */

void
record_line (struct subfile *subfile, int line, CORE_ADDR pc)
{
  struct linetable_entry *e;

  /* Ignore the dummy line number in libg.o */
  if (line == 0xffff)
    {
      return;
    }

  /* Make sure line vector exists and is big enough.  */
  if (!subfile->line_vector)
    {
      subfile->line_vector_length = INITIAL_LINE_VECTOR_LENGTH;
      subfile->line_vector = (struct linetable *)
	xmalloc (sizeof (struct linetable)
	   + subfile->line_vector_length * sizeof (struct linetable_entry));
      subfile->line_vector->nitems = 0;
      have_line_numbers = 1;
    }

  if (subfile->line_vector->nitems + 1 >= subfile->line_vector_length)
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
  if (line == 0 && subfile->line_vector->nitems > 0)
    {
      e = subfile->line_vector->item + subfile->line_vector->nitems - 1;
      while (subfile->line_vector->nitems > 0 && e->pc == pc)
	{
	  e--;
	  subfile->line_vector->nitems--;
	}
    }

  e = subfile->line_vector->item + subfile->line_vector->nitems++;
  e->line = line;
  e->pc = pc;
}

/* Needed in order to sort line tables from IBM xcoff files.  Sigh!  */

static int
compare_line_numbers (const void *ln1p, const void *ln2p)
{
  struct linetable_entry *ln1 = (struct linetable_entry *) ln1p;
  struct linetable_entry *ln2 = (struct linetable_entry *) ln2p;

  /* Note: this code does not assume that CORE_ADDRs can fit in ints.
     Please keep it that way.  */
  if (ln1->pc < ln2->pc)
    return -1;

  if (ln1->pc > ln2->pc)
    return 1;

  /* If pc equal, sort by line.  I'm not sure whether this is optimum
     behavior (see comment at struct linetable in symtab.h).  */
  return ln1->line - ln2->line;
}

/* Start a new symtab for a new source file.  Called, for example,
   when a stabs symbol of type N_SO is seen, or when a DWARF
   TAG_compile_unit DIE is seen.  It indicates the start of data for
   one original source file.

   NAME is the name of the file (cannot be NULL).  DIRNAME is the directory in
   which the file was compiled (or NULL if not known).  START_ADDR is the
   lowest address of objects in the file (or 0 if not known).  */

void
start_symtab (const char *name, const char *dirname, CORE_ADDR start_addr)
{
  restart_symtab (start_addr);
  set_last_source_file (name);
  start_subfile (name, dirname);
}

/* Restart compilation for a symtab.
   This is used when a symtab is built from multiple sources.
   The symtab is first built with start_symtab and then for each additional
   piece call restart_symtab.  */

void
restart_symtab (CORE_ADDR start_addr)
{
  set_last_source_file (NULL);
  last_source_start_addr = start_addr;
  file_symbols = NULL;
  global_symbols = NULL;
  within_function = 0;
  have_line_numbers = 0;

  /* Context stack is initially empty.  Allocate first one with room
     for 10 levels; reuse it forever afterward.  */
  if (context_stack == NULL)
    {
      context_stack_size = INITIAL_CONTEXT_STACK_SIZE;
      context_stack = (struct context_stack *)
	xmalloc (context_stack_size * sizeof (struct context_stack));
    }
  context_stack_depth = 0;

  /* We shouldn't have any address map at this point.  */
  gdb_assert (! pending_addrmap);

  /* Initialize the list of sub source files with one entry for this
     file (the top-level source file).  */
  subfiles = NULL;
  current_subfile = NULL;
}

/* Subroutine of end_symtab to simplify it.  Look for a subfile that
   matches the main source file's basename.  If there is only one, and
   if the main source file doesn't have any symbol or line number
   information, then copy this file's symtab and line_vector to the
   main source file's subfile and discard the other subfile.  This can
   happen because of a compiler bug or from the user playing games
   with #line or from things like a distributed build system that
   manipulates the debug info.  */

static void
watch_main_source_file_lossage (void)
{
  struct subfile *mainsub, *subfile;

  /* Find the main source file.
     This loop could be eliminated if start_symtab saved it for us.  */
  mainsub = NULL;
  for (subfile = subfiles; subfile; subfile = subfile->next)
    {
      /* The main subfile is guaranteed to be the last one.  */
      if (subfile->next == NULL)
	mainsub = subfile;
    }

  /* If the main source file doesn't have any line number or symbol
     info, look for an alias in another subfile.

     We have to watch for mainsub == NULL here.  It's a quirk of
     end_symtab, it can return NULL so there may not be a main
     subfile.  */

  if (mainsub
      && mainsub->line_vector == NULL
      && mainsub->symtab == NULL)
    {
      const char *mainbase = lbasename (mainsub->name);
      int nr_matches = 0;
      struct subfile *prevsub;
      struct subfile *mainsub_alias = NULL;
      struct subfile *prev_mainsub_alias = NULL;

      prevsub = NULL;
      for (subfile = subfiles;
	   /* Stop before we get to the last one.  */
	   subfile->next;
	   subfile = subfile->next)
	{
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
	    subfiles = mainsub_alias->next;
	  else
	    prev_mainsub_alias->next = mainsub_alias->next;
	  xfree (mainsub_alias);
	}
    }
}

/* Helper function for qsort.  Parameters are `struct block *' pointers,
   function sorts them in descending order by their BLOCK_START.  */

static int
block_compar (const void *ap, const void *bp)
{
  const struct block *a = *(const struct block **) ap;
  const struct block *b = *(const struct block **) bp;

  return ((BLOCK_START (b) > BLOCK_START (a))
	  - (BLOCK_START (b) < BLOCK_START (a)));
}

/* Reset globals used to build symtabs.  */

static void
reset_symtab_globals (void)
{
  set_last_source_file (NULL);
  current_subfile = NULL;
  pending_macros = NULL;
  if (pending_addrmap)
    {
      obstack_free (&pending_addrmap_obstack, NULL);
      pending_addrmap = NULL;
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
end_symtab_get_static_block (CORE_ADDR end_addr, struct objfile *objfile,
			     int expandable, int required)
{
  /* Finish the lexical context of the last function in the file; pop
     the context stack.  */

  if (context_stack_depth > 0)
    {
      struct context_stack *cstk = pop_context ();

      /* Make a block for the local symbols within.  */
      finish_block (cstk->name, &local_symbols, cstk->old_blocks,
		    cstk->start_addr, end_addr, objfile);

      if (context_stack_depth > 0)
	{
	  /* This is said to happen with SCO.  The old coffread.c
	     code simply emptied the context stack, so we do the
	     same.  FIXME: Find out why it is happening.  This is not
	     believed to happen in most cases (even for coffread.c);
	     it used to be an abort().  */
	  complaint (&symfile_complaints,
	             _("Context stack not empty in end_symtab"));
	  context_stack_depth = 0;
	}
    }

  /* Reordered executables may have out of order pending blocks; if
     OBJF_REORDERED is true, then sort the pending blocks.  */

  if ((objfile->flags & OBJF_REORDERED) && pending_blocks)
    {
      unsigned count = 0;
      struct pending_block *pb;
      struct block **barray, **bp;
      struct cleanup *back_to;

      for (pb = pending_blocks; pb != NULL; pb = pb->next)
	count++;

      barray = xmalloc (sizeof (*barray) * count);
      back_to = make_cleanup (xfree, barray);

      bp = barray;
      for (pb = pending_blocks; pb != NULL; pb = pb->next)
	*bp++ = pb->block;

      qsort (barray, count, sizeof (*barray), block_compar);

      bp = barray;
      for (pb = pending_blocks; pb != NULL; pb = pb->next)
	pb->block = *bp++;

      do_cleanups (back_to);
    }

  /* Cleanup any undefined types that have been left hanging around
     (this needs to be done before the finish_blocks so that
     file_symbols is still good).

     Both cleanup_undefined_stabs_types and finish_global_stabs are stabs
     specific, but harmless for other symbol readers, since on gdb
     startup or when finished reading stabs, the state is set so these
     are no-ops.  FIXME: Is this handled right in case of QUIT?  Can
     we make this cleaner?  */

  cleanup_undefined_stabs_types (objfile);
  finish_global_stabs (objfile);

  if (!required
      && pending_blocks == NULL
      && file_symbols == NULL
      && global_symbols == NULL
      && have_line_numbers == 0
      && pending_macros == NULL)
    {
      /* Ignore symtabs that have no functions with real debugging info.  */
      return NULL;
    }
  else
    {
      /* Define the STATIC_BLOCK.  */
      return finish_block_internal (NULL, &file_symbols, NULL,
				    last_source_start_addr, end_addr, objfile,
				    0, expandable);
    }
}

/* Implementation of the second part of end_symtab.  Pass STATIC_BLOCK
   as value returned by end_symtab_get_static_block.

   SECTION is the same as for end_symtab: the section number
   (in objfile->section_offsets) of the blockvector and linetable.

   If EXPANDABLE is non-zero the GLOBAL_BLOCK dictionary is made
   expandable.  */

struct symtab *
end_symtab_from_static_block (struct block *static_block,
			      struct objfile *objfile, int section,
			      int expandable)
{
  struct symtab *symtab = NULL;
  struct blockvector *blockvector;
  struct subfile *subfile;
  struct subfile *nextsub;

  if (static_block == NULL)
    {
      /* Ignore symtabs that have no functions with real debugging info.  */
      blockvector = NULL;
    }
  else
    {
      CORE_ADDR end_addr = BLOCK_END (static_block);

      /* Define after STATIC_BLOCK also GLOBAL_BLOCK, and build the
         blockvector.  */
      finish_block_internal (NULL, &global_symbols, NULL,
			     last_source_start_addr, end_addr, objfile,
			     1, expandable);
      blockvector = make_blockvector (objfile);
    }

  /* Read the line table if it has to be read separately.  */
  if (objfile->sf->sym_read_linetable != NULL)
    objfile->sf->sym_read_linetable ();

  /* Handle the case where the debug info specifies a different path
     for the main source file.  It can cause us to lose track of its
     line number information.  */
  watch_main_source_file_lossage ();

  /* Now create the symtab objects proper, one for each subfile.  */
  /* (The main file is the last one on the chain.)  */

  for (subfile = subfiles; subfile; subfile = nextsub)
    {
      int linetablesize = 0;
      symtab = NULL;

      /* If we have blocks of symbols, make a symtab.  Otherwise, just
         ignore this file and any line number info in it.  */
      if (blockvector)
	{
	  if (subfile->line_vector)
	    {
	      linetablesize = sizeof (struct linetable) +
	        subfile->line_vector->nitems * sizeof (struct linetable_entry);

	      /* Like the pending blocks, the line table may be
	         scrambled in reordered executables.  Sort it if
	         OBJF_REORDERED is true.  */
	      if (objfile->flags & OBJF_REORDERED)
		qsort (subfile->line_vector->item,
		       subfile->line_vector->nitems,
		     sizeof (struct linetable_entry), compare_line_numbers);
	    }

	  /* Now, allocate a symbol table.  */
	  if (subfile->symtab == NULL)
	    symtab = allocate_symtab (subfile->name, objfile);
	  else
	    symtab = subfile->symtab;

	  /* Fill in its components.  */
	  symtab->blockvector = blockvector;
          symtab->macro_table = pending_macros;
	  if (subfile->line_vector)
	    {
	      /* Reallocate the line table on the symbol obstack.  */
	      symtab->linetable = (struct linetable *)
		obstack_alloc (&objfile->objfile_obstack, linetablesize);
	      memcpy (symtab->linetable, subfile->line_vector, linetablesize);
	    }
	  else
	    {
	      symtab->linetable = NULL;
	    }
	  symtab->block_line_section = section;
	  if (subfile->dirname)
	    {
	      /* Reallocate the dirname on the symbol obstack.  */
	      symtab->dirname = (char *)
		obstack_alloc (&objfile->objfile_obstack,
			       strlen (subfile->dirname) + 1);
	      strcpy (symtab->dirname, subfile->dirname);
	    }
	  else
	    {
	      symtab->dirname = NULL;
	    }

	  /* Use whatever language we have been using for this
	     subfile, not the one that was deduced in allocate_symtab
	     from the filename.  We already did our own deducing when
	     we created the subfile, and we may have altered our
	     opinion of what language it is from things we found in
	     the symbols.  */
	  symtab->language = subfile->language;

	  /* Save the debug format string (if any) in the symtab.  */
	  symtab->debugformat = subfile->debugformat;

	  /* Similarly for the producer.  */
	  symtab->producer = subfile->producer;

	  /* All symtabs for the main file and the subfiles share a
	     blockvector, so we need to clear primary for everything
	     but the main file.  */

	  symtab->primary = 0;
	}
      else
        {
          if (subfile->symtab)
            {
              /* Since we are ignoring that subfile, we also need
                 to unlink the associated empty symtab that we created.
                 Otherwise, we can run into trouble because various parts
                 such as the block-vector are uninitialized whereas
                 the rest of the code assumes that they are.
                 
                 We can only unlink the symtab because it was allocated
                 on the objfile obstack.  */
              struct symtab *s;

              if (objfile->symtabs == subfile->symtab)
                objfile->symtabs = objfile->symtabs->next;
              else
                ALL_OBJFILE_SYMTABS (objfile, s)
                  if (s->next == subfile->symtab)
                    {
                      s->next = s->next->next;
                      break;
                    }
              subfile->symtab = NULL;
            }
        }
      if (subfile->name != NULL)
	{
	  xfree ((void *) subfile->name);
	}
      if (subfile->dirname != NULL)
	{
	  xfree ((void *) subfile->dirname);
	}
      if (subfile->line_vector != NULL)
	{
	  xfree ((void *) subfile->line_vector);
	}

      nextsub = subfile->next;
      xfree ((void *) subfile);
    }

  /* Set this for the main source file.  */
  if (symtab)
    {
      symtab->primary = 1;

      if (symtab->blockvector)
	{
	  struct block *b = BLOCKVECTOR_BLOCK (symtab->blockvector,
					       GLOBAL_BLOCK);

	  set_block_symtab (b, symtab);
	}
    }

  /* Default any symbols without a specified symtab to the primary
     symtab.  */
  if (blockvector)
    {
      int block_i;

      for (block_i = 0; block_i < BLOCKVECTOR_NBLOCKS (blockvector); block_i++)
	{
	  struct block *block = BLOCKVECTOR_BLOCK (blockvector, block_i);
	  struct symbol *sym;
	  struct dict_iterator iter;

	  /* Inlined functions may have symbols not in the global or
	     static symbol lists.  */
	  if (BLOCK_FUNCTION (block) != NULL)
	    if (SYMBOL_SYMTAB (BLOCK_FUNCTION (block)) == NULL)
	      SYMBOL_SYMTAB (BLOCK_FUNCTION (block)) = symtab;

	  /* Note that we only want to fix up symbols from the local
	     blocks, not blocks coming from included symtabs.  That is why
	     we use ALL_DICT_SYMBOLS here and not ALL_BLOCK_SYMBOLS.  */
	  ALL_DICT_SYMBOLS (BLOCK_DICT (block), iter, sym)
	    if (SYMBOL_SYMTAB (sym) == NULL)
	      SYMBOL_SYMTAB (sym) = symtab;
	}
    }

  reset_symtab_globals ();

  return symtab;
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

struct symtab *
end_symtab (CORE_ADDR end_addr, struct objfile *objfile, int section)
{
  struct block *static_block;

  static_block = end_symtab_get_static_block (end_addr, objfile, 0, 0);
  return end_symtab_from_static_block (static_block, objfile, section, 0);
}

/* Same as end_symtab except create a symtab that can be later added to.  */

struct symtab *
end_expandable_symtab (CORE_ADDR end_addr, struct objfile *objfile,
		       int section)
{
  struct block *static_block;

  static_block = end_symtab_get_static_block (end_addr, objfile, 1, 0);
  return end_symtab_from_static_block (static_block, objfile, section, 1);
}

/* Subroutine of augment_type_symtab to simplify it.
   Attach SYMTAB to all symbols in PENDING_LIST that don't have one.  */

static void
set_missing_symtab (struct pending *pending_list, struct symtab *symtab)
{
  struct pending *pending;
  int i;

  for (pending = pending_list; pending != NULL; pending = pending->next)
    {
      for (i = 0; i < pending->nsyms; ++i)
	{
	  if (SYMBOL_SYMTAB (pending->symbol[i]) == NULL)
	    SYMBOL_SYMTAB (pending->symbol[i]) = symtab;
	}
    }
}

/* Same as end_symtab, but for the case where we're adding more symbols
   to an existing symtab that is known to contain only type information.
   This is the case for DWARF4 Type Units.  */

void
augment_type_symtab (struct objfile *objfile, struct symtab *primary_symtab)
{
  struct blockvector *blockvector = primary_symtab->blockvector;
  int i;

  if (context_stack_depth > 0)
    {
      complaint (&symfile_complaints,
		 _("Context stack not empty in augment_type_symtab"));
      context_stack_depth = 0;
    }
  if (pending_blocks != NULL)
    complaint (&symfile_complaints, _("Blocks in a type symtab"));
  if (pending_macros != NULL)
    complaint (&symfile_complaints, _("Macro in a type symtab"));
  if (have_line_numbers)
    complaint (&symfile_complaints,
	       _("Line numbers recorded in a type symtab"));

  if (file_symbols != NULL)
    {
      struct block *block = BLOCKVECTOR_BLOCK (blockvector, STATIC_BLOCK);

      /* First mark any symbols without a specified symtab as belonging
	 to the primary symtab.  */
      set_missing_symtab (file_symbols, primary_symtab);

      dict_add_pending (BLOCK_DICT (block), file_symbols);
    }

  if (global_symbols != NULL)
    {
      struct block *block = BLOCKVECTOR_BLOCK (blockvector, GLOBAL_BLOCK);

      /* First mark any symbols without a specified symtab as belonging
	 to the primary symtab.  */
      set_missing_symtab (global_symbols, primary_symtab);

      dict_add_pending (BLOCK_DICT (block), global_symbols);
    }

  reset_symtab_globals ();
}

/* Push a context block.  Args are an identifying nesting level
   (checkable when you pop it), and the starting PC address of this
   context.  */

struct context_stack *
push_context (int desc, CORE_ADDR valu)
{
  struct context_stack *new;

  if (context_stack_depth == context_stack_size)
    {
      context_stack_size *= 2;
      context_stack = (struct context_stack *)
	xrealloc ((char *) context_stack,
		  (context_stack_size * sizeof (struct context_stack)));
    }

  new = &context_stack[context_stack_depth++];
  new->depth = desc;
  new->locals = local_symbols;
  new->old_blocks = pending_blocks;
  new->start_addr = valu;
  new->using_directives = using_directives;
  new->name = NULL;

  local_symbols = NULL;
  using_directives = NULL;

  return new;
}

/* Pop a context block.  Returns the address of the context block just
   popped.  */

struct context_stack *
pop_context (void)
{
  gdb_assert (context_stack_depth > 0);
  return (&context_stack[--context_stack_depth]);
}



/* Compute a small integer hash code for the given name.  */

int
hashname (const char *name)
{
    return (hash(name,strlen(name)) % HASHSIZE);
}


void
record_debugformat (const char *format)
{
  current_subfile->debugformat = format;
}

void
record_producer (const char *producer)
{
  current_subfile->producer = producer;
}

/* Merge the first symbol list SRCLIST into the second symbol list
   TARGETLIST by repeated calls to add_symbol_to_list().  This
   procedure "frees" each link of SRCLIST by adding it to the
   free_pendings list.  Caller must set SRCLIST to a null list after
   calling this function.

   Void return.  */

void
merge_symbol_lists (struct pending **srclist, struct pending **targetlist)
{
  int i;

  if (!srclist || !*srclist)
    return;

  /* Merge in elements from current link.  */
  for (i = 0; i < (*srclist)->nsyms; i++)
    add_symbol_to_list ((*srclist)->symbol[i], targetlist);

  /* Recurse on next.  */
  merge_symbol_lists (&(*srclist)->next, targetlist);

  /* "Free" the current link.  */
  (*srclist)->next = free_pendings;
  free_pendings = (*srclist);
}


/* Name of source file whose symbol data we are now processing.  This
   comes from a symbol of type N_SO for stabs.  For Dwarf it comes
   from the DW_AT_name attribute of a DW_TAG_compile_unit DIE.  */

static char *last_source_file;

/* See buildsym.h.  */

void
set_last_source_file (const char *name)
{
  xfree (last_source_file);
  last_source_file = name == NULL ? NULL : xstrdup (name);
}

/* See buildsym.h.  */

const char *
get_last_source_file (void)
{
  return last_source_file;
}



/* Initialize anything that needs initializing when starting to read a
   fresh piece of a symbol file, e.g. reading in the stuff
   corresponding to a psymtab.  */

void
buildsym_init (void)
{
  free_pendings = NULL;
  file_symbols = NULL;
  global_symbols = NULL;
  pending_blocks = NULL;
  pending_macros = NULL;
  using_directives = NULL;

  /* We shouldn't have any address map at this point.  */
  gdb_assert (! pending_addrmap);
  pending_addrmap_interesting = 0;
}

/* Initialize anything that needs initializing when a completely new
   symbol file is specified (not just adding some symbols from another
   file, e.g. a shared library).  */

void
buildsym_new_init (void)
{
  buildsym_init ();
}
