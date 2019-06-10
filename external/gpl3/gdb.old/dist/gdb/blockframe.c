/* Get info from stack frames; convert between frames, blocks,
   functions and pc values.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "bfd.h"
#include "objfiles.h"
#include "frame.h"
#include "gdbcore.h"
#include "value.h"
#include "target.h"
#include "inferior.h"
#include "annotate.h"
#include "regcache.h"
#include "dummy-frame.h"
#include "command.h"
#include "gdbcmd.h"
#include "block.h"
#include "inline-frame.h"

/* Return the innermost lexical block in execution in a specified
   stack frame.  The frame address is assumed valid.

   If ADDR_IN_BLOCK is non-zero, set *ADDR_IN_BLOCK to the exact code
   address we used to choose the block.  We use this to find a source
   line, to decide which macro definitions are in scope.

   The value returned in *ADDR_IN_BLOCK isn't necessarily the frame's
   PC, and may not really be a valid PC at all.  For example, in the
   caller of a function declared to never return, the code at the
   return address will never be reached, so the call instruction may
   be the very last instruction in the block.  So the address we use
   to choose the block is actually one byte before the return address
   --- hopefully pointing us at the call instruction, or its delay
   slot instruction.  */

const struct block *
get_frame_block (struct frame_info *frame, CORE_ADDR *addr_in_block)
{
  CORE_ADDR pc;
  const struct block *bl;
  int inline_count;

  if (!get_frame_address_in_block_if_available (frame, &pc))
    return NULL;

  if (addr_in_block)
    *addr_in_block = pc;

  bl = block_for_pc (pc);
  if (bl == NULL)
    return NULL;

  inline_count = frame_inlined_callees (frame);

  while (inline_count > 0)
    {
      if (block_inlined_p (bl))
	inline_count--;

      bl = BLOCK_SUPERBLOCK (bl);
      gdb_assert (bl != NULL);
    }

  return bl;
}

CORE_ADDR
get_pc_function_start (CORE_ADDR pc)
{
  const struct block *bl;
  struct bound_minimal_symbol msymbol;

  bl = block_for_pc (pc);
  if (bl)
    {
      struct symbol *symbol = block_linkage_function (bl);

      if (symbol)
	{
	  bl = SYMBOL_BLOCK_VALUE (symbol);
	  return BLOCK_START (bl);
	}
    }

  msymbol = lookup_minimal_symbol_by_pc (pc);
  if (msymbol.minsym)
    {
      CORE_ADDR fstart = BMSYMBOL_VALUE_ADDRESS (msymbol);

      if (find_pc_section (fstart))
	return fstart;
    }

  return 0;
}

/* Return the symbol for the function executing in frame FRAME.  */

struct symbol *
get_frame_function (struct frame_info *frame)
{
  const struct block *bl = get_frame_block (frame, 0);

  if (bl == NULL)
    return NULL;

  while (BLOCK_FUNCTION (bl) == NULL && BLOCK_SUPERBLOCK (bl) != NULL)
    bl = BLOCK_SUPERBLOCK (bl);

  return BLOCK_FUNCTION (bl);
}


/* Return the function containing pc value PC in section SECTION.
   Returns 0 if function is not known.  */

struct symbol *
find_pc_sect_function (CORE_ADDR pc, struct obj_section *section)
{
  const struct block *b = block_for_pc_sect (pc, section);

  if (b == 0)
    return 0;
  return block_linkage_function (b);
}

/* Return the function containing pc value PC.
   Returns 0 if function is not known.  
   Backward compatibility, no section */

struct symbol *
find_pc_function (CORE_ADDR pc)
{
  return find_pc_sect_function (pc, find_pc_mapped_section (pc));
}

/* These variables are used to cache the most recent result
   of find_pc_partial_function.  */

static CORE_ADDR cache_pc_function_low = 0;
static CORE_ADDR cache_pc_function_high = 0;
static const char *cache_pc_function_name = 0;
static struct obj_section *cache_pc_function_section = NULL;
static int cache_pc_function_is_gnu_ifunc = 0;

/* Clear cache, e.g. when symbol table is discarded.  */

void
clear_pc_function_cache (void)
{
  cache_pc_function_low = 0;
  cache_pc_function_high = 0;
  cache_pc_function_name = (char *) 0;
  cache_pc_function_section = NULL;
  cache_pc_function_is_gnu_ifunc = 0;
}

/* Finds the "function" (text symbol) that is smaller than PC but
   greatest of all of the potential text symbols in SECTION.  Sets
   *NAME and/or *ADDRESS conditionally if that pointer is non-null.
   If ENDADDR is non-null, then set *ENDADDR to be the end of the
   function (exclusive), but passing ENDADDR as non-null means that
   the function might cause symbols to be read.  If IS_GNU_IFUNC_P is provided
   *IS_GNU_IFUNC_P is set to 1 on return if the function is STT_GNU_IFUNC.
   This function either succeeds or fails (not halfway succeeds).  If it
   succeeds, it sets *NAME, *ADDRESS, and *ENDADDR to real information and
   returns 1.  If it fails, it sets *NAME, *ADDRESS, *ENDADDR and
   *IS_GNU_IFUNC_P to zero and returns 0.  */

/* Backward compatibility, no section argument.  */

int
find_pc_partial_function_gnu_ifunc (CORE_ADDR pc, const char **name,
				    CORE_ADDR *address, CORE_ADDR *endaddr,
				    int *is_gnu_ifunc_p)
{
  struct obj_section *section;
  struct symbol *f;
  struct bound_minimal_symbol msymbol;
  struct compunit_symtab *compunit_symtab = NULL;
  struct objfile *objfile;
  CORE_ADDR mapped_pc;

  /* To ensure that the symbol returned belongs to the correct setion
     (and that the last [random] symbol from the previous section
     isn't returned) try to find the section containing PC.  First try
     the overlay code (which by default returns NULL); and second try
     the normal section code (which almost always succeeds).  */
  section = find_pc_overlay (pc);
  if (section == NULL)
    section = find_pc_section (pc);

  mapped_pc = overlay_mapped_address (pc, section);

  if (mapped_pc >= cache_pc_function_low
      && mapped_pc < cache_pc_function_high
      && section == cache_pc_function_section)
    goto return_cached_value;

  msymbol = lookup_minimal_symbol_by_pc_section (mapped_pc, section);
  ALL_OBJFILES (objfile)
  {
    if (objfile->sf)
      {
	compunit_symtab
	  = objfile->sf->qf->find_pc_sect_compunit_symtab (objfile, msymbol,
							   mapped_pc, section,
							   0);
      }
    if (compunit_symtab != NULL)
      break;
  }

  if (compunit_symtab != NULL)
    {
      /* Checking whether the msymbol has a larger value is for the
	 "pathological" case mentioned in print_frame_info.  */
      f = find_pc_sect_function (mapped_pc, section);
      if (f != NULL
	  && (msymbol.minsym == NULL
	      || (BLOCK_START (SYMBOL_BLOCK_VALUE (f))
		  >= BMSYMBOL_VALUE_ADDRESS (msymbol))))
	{
	  cache_pc_function_low = BLOCK_START (SYMBOL_BLOCK_VALUE (f));
	  cache_pc_function_high = BLOCK_END (SYMBOL_BLOCK_VALUE (f));
	  cache_pc_function_name = SYMBOL_LINKAGE_NAME (f);
	  cache_pc_function_section = section;
	  cache_pc_function_is_gnu_ifunc = TYPE_GNU_IFUNC (SYMBOL_TYPE (f));
	  goto return_cached_value;
	}
    }

  /* Not in the normal symbol tables, see if the pc is in a known
     section.  If it's not, then give up.  This ensures that anything
     beyond the end of the text seg doesn't appear to be part of the
     last function in the text segment.  */

  if (!section)
    msymbol.minsym = NULL;

  /* Must be in the minimal symbol table.  */
  if (msymbol.minsym == NULL)
    {
      /* No available symbol.  */
      if (name != NULL)
	*name = 0;
      if (address != NULL)
	*address = 0;
      if (endaddr != NULL)
	*endaddr = 0;
      if (is_gnu_ifunc_p != NULL)
	*is_gnu_ifunc_p = 0;
      return 0;
    }

  cache_pc_function_low = BMSYMBOL_VALUE_ADDRESS (msymbol);
  cache_pc_function_name = MSYMBOL_LINKAGE_NAME (msymbol.minsym);
  cache_pc_function_section = section;
  cache_pc_function_is_gnu_ifunc = (MSYMBOL_TYPE (msymbol.minsym)
				    == mst_text_gnu_ifunc);
  cache_pc_function_high = minimal_symbol_upper_bound (msymbol);

 return_cached_value:

  if (address)
    {
      if (pc_in_unmapped_range (pc, section))
	*address = overlay_unmapped_address (cache_pc_function_low, section);
      else
	*address = cache_pc_function_low;
    }

  if (name)
    *name = cache_pc_function_name;

  if (endaddr)
    {
      if (pc_in_unmapped_range (pc, section))
	{
	  /* Because the high address is actually beyond the end of
	     the function (and therefore possibly beyond the end of
	     the overlay), we must actually convert (high - 1) and
	     then add one to that.  */

	  *endaddr = 1 + overlay_unmapped_address (cache_pc_function_high - 1,
						   section);
	}
      else
	*endaddr = cache_pc_function_high;
    }

  if (is_gnu_ifunc_p)
    *is_gnu_ifunc_p = cache_pc_function_is_gnu_ifunc;

  return 1;
}

/* See find_pc_partial_function_gnu_ifunc, only the IS_GNU_IFUNC_P parameter
   is omitted here for backward API compatibility.  */

int
find_pc_partial_function (CORE_ADDR pc, const char **name, CORE_ADDR *address,
			  CORE_ADDR *endaddr)
{
  return find_pc_partial_function_gnu_ifunc (pc, name, address, endaddr, NULL);
}

/* Return the innermost stack frame that is executing inside of BLOCK and is
   at least as old as the selected frame. Return NULL if there is no
   such frame.  If BLOCK is NULL, just return NULL.  */

struct frame_info *
block_innermost_frame (const struct block *block)
{
  struct frame_info *frame;

  if (block == NULL)
    return NULL;

  frame = get_selected_frame_if_set ();
  if (frame == NULL)
    frame = get_current_frame ();
  while (frame != NULL)
    {
      const struct block *frame_block = get_frame_block (frame, NULL);
      if (frame_block != NULL && contained_in (frame_block, block))
	return frame;

      frame = get_prev_frame (frame);
    }

  return NULL;
}
