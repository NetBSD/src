/* Read HP PA/Risc object files for GDB.
   Copyright (C) 1991-2014 Free Software Foundation, Inc.
   Written by Fred Fish at Cygnus Support.

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
#include "bfd.h"
#include "som/aout.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "buildsym.h"
#include "stabsread.h"
#include "gdb-stabs.h"
#include "complaints.h"
#include <string.h>
#include "demangle.h"
#include "som.h"
#include "libhppa.h"
#include "psymtab.h"

#include "solib-som.h"

/* Read the symbol table of a SOM file.

   Given an open bfd, a base address to relocate symbols to, and a
   flag that specifies whether or not this bfd is for an executable
   or not (may be shared library for example), add all the global
   function and data symbols to the minimal symbol table.  */

static void
som_symtab_read (bfd *abfd, struct objfile *objfile,
		 struct section_offsets *section_offsets)
{
  struct cleanup *cleanup;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  unsigned int number_of_symbols;
  int val, dynamic;
  char *stringtab;
  asection *shlib_info;
  struct som_external_symbol_dictionary_record *buf, *bufp, *endbufp;
  char *symname;
  const int symsize = sizeof (struct som_external_symbol_dictionary_record);


#define text_offset ANOFFSET (section_offsets, SECT_OFF_TEXT (objfile))
#define data_offset ANOFFSET (section_offsets, SECT_OFF_DATA (objfile))

  number_of_symbols = bfd_get_symcount (abfd);

  /* Allocate a buffer to read in the debug info.
     We avoid using alloca because the memory size could be so large
     that we could hit the stack size limit.  */
  buf = xmalloc (symsize * number_of_symbols);
  cleanup = make_cleanup (xfree, buf);
  bfd_seek (abfd, obj_som_sym_filepos (abfd), SEEK_SET);
  val = bfd_bread (buf, symsize * number_of_symbols, abfd);
  if (val != symsize * number_of_symbols)
    error (_("Couldn't read symbol dictionary!"));

  /* Allocate a buffer to read in the som stringtab section of
     the debugging info.  Again, we avoid using alloca because
     the data could be so large that we could potentially hit
     the stack size limitat.  */
  stringtab = xmalloc (obj_som_stringtab_size (abfd));
  make_cleanup (xfree, stringtab);
  bfd_seek (abfd, obj_som_str_filepos (abfd), SEEK_SET);
  val = bfd_bread (stringtab, obj_som_stringtab_size (abfd), abfd);
  if (val != obj_som_stringtab_size (abfd))
    error (_("Can't read in HP string table."));

  /* We need to determine if objfile is a dynamic executable (so we
     can do the right thing for ST_ENTRY vs ST_CODE symbols).

     There's nothing in the header which easily allows us to do
     this.

     This code used to rely upon the existence of a $SHLIB_INFO$
     section to make this determination.  HP claims that it is
     more accurate to check for a nonzero text offset, but they
     have not provided any information about why that test is
     more accurate.  */
  dynamic = (text_offset != 0);

  endbufp = buf + number_of_symbols;
  for (bufp = buf; bufp < endbufp; ++bufp)
    {
      enum minimal_symbol_type ms_type;
      unsigned int flags = bfd_getb32 (bufp->flags);
      unsigned int symbol_type
	= (flags >> SOM_SYMBOL_TYPE_SH) & SOM_SYMBOL_TYPE_MASK;
      unsigned int symbol_scope
	= (flags >> SOM_SYMBOL_SCOPE_SH) & SOM_SYMBOL_SCOPE_MASK;
      CORE_ADDR symbol_value = bfd_getb32 (bufp->symbol_value);
      asection *section = NULL;

      QUIT;

      /* Compute the section.  */
      switch (symbol_scope)
	{
	case SS_EXTERNAL:
	  if (symbol_type != ST_STORAGE)
	    section = bfd_und_section_ptr;
	  else
	    section = bfd_com_section_ptr;
	  break;

	case SS_UNSAT:
	  if (symbol_type != ST_STORAGE)
	    section = bfd_und_section_ptr;
	  else
	    section = bfd_com_section_ptr;
	  break;

	case SS_UNIVERSAL:
	  section = bfd_section_from_som_symbol (abfd, bufp);
	  break;

	case SS_LOCAL:
	  section = bfd_section_from_som_symbol (abfd, bufp);
	  break;
	}

      switch (symbol_scope)
	{
	case SS_UNIVERSAL:
	case SS_EXTERNAL:
	  switch (symbol_type)
	    {
	    case ST_SYM_EXT:
	    case ST_ARG_EXT:
	      continue;

	    case ST_CODE:
	    case ST_PRI_PROG:
	    case ST_SEC_PROG:
	    case ST_MILLICODE:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      ms_type = mst_text;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);
	      break;

	    case ST_ENTRY:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      /* For a dynamic executable, ST_ENTRY symbols are
	         the stubs, while the ST_CODE symbol is the real
	         function.  */
	      if (dynamic)
		ms_type = mst_solib_trampoline;
	      else
		ms_type = mst_text;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);
	      break;

	    case ST_STUB:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      ms_type = mst_solib_trampoline;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);
	      break;

	    case ST_DATA:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      symbol_value += data_offset;
	      ms_type = mst_data;
	      break;
	    default:
	      continue;
	    }
	  break;

#if 0
	  /* SS_GLOBAL and SS_LOCAL are two names for the same thing (!).  */
	case SS_GLOBAL:
#endif
	case SS_LOCAL:
	  switch (symbol_type)
	    {
	    case ST_SYM_EXT:
	    case ST_ARG_EXT:
	      continue;

	    case ST_CODE:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      ms_type = mst_file_text;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);

	    check_strange_names:
	      /* Utah GCC 2.5, FSF GCC 2.6 and later generate correct local
	         label prefixes for stabs, constant data, etc.  So we need
	         only filter out L$ symbols which are left in due to
	         limitations in how GAS generates SOM relocations.

	         When linking in the HPUX C-library the HP linker has
	         the nasty habit of placing section symbols from the literal
	         subspaces in the middle of the program's text.  Filter
	         those out as best we can.  Check for first and last character
	         being '$'.

	         And finally, the newer HP compilers emit crud like $PIC_foo$N
	         in some circumstance (PIC code I guess).  It's also claimed
	         that they emit D$ symbols too.  What stupidity.  */
	      if ((symname[0] == 'L' && symname[1] == '$')
	      || (symname[0] == '$' && symname[strlen (symname) - 1] == '$')
		  || (symname[0] == 'D' && symname[1] == '$')
		  || (strncmp (symname, "L0\001", 3) == 0)
		  || (strncmp (symname, "$PIC", 4) == 0))
		continue;
	      break;

	    case ST_PRI_PROG:
	    case ST_SEC_PROG:
	    case ST_MILLICODE:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      ms_type = mst_file_text;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);
	      break;

	    case ST_ENTRY:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      /* SS_LOCAL symbols in a shared library do not have
		 export stubs, so we do not have to worry about
		 using mst_file_text vs mst_solib_trampoline here like
		 we do for SS_UNIVERSAL and SS_EXTERNAL symbols above.  */
	      ms_type = mst_file_text;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);
	      break;

	    case ST_STUB:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      ms_type = mst_solib_trampoline;
	      symbol_value += text_offset;
	      symbol_value = gdbarch_addr_bits_remove (gdbarch, symbol_value);
	      break;


	    case ST_DATA:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      symbol_value += data_offset;
	      ms_type = mst_file_data;
	      goto check_strange_names;

	    default:
	      continue;
	    }
	  break;

	  /* This can happen for common symbols when -E is passed to the
	     final link.  No idea _why_ that would make the linker force
	     common symbols to have an SS_UNSAT scope, but it does.

	     This also happens for weak symbols, but their type is
	     ST_DATA.  */
	case SS_UNSAT:
	  switch (symbol_type)
	    {
	    case ST_STORAGE:
	    case ST_DATA:
	      symname = bfd_getb32 (bufp->name) + stringtab;
	      symbol_value += data_offset;
	      ms_type = mst_data;
	      break;

	    default:
	      continue;
	    }
	  break;

	default:
	  continue;
	}

      if (bfd_getb32 (bufp->name) > obj_som_stringtab_size (abfd))
	error (_("Invalid symbol data; bad HP string table offset: %s"),
	       plongest (bfd_getb32 (bufp->name)));

      if (bfd_is_const_section (section))
	{
	  struct obj_section *iter;

	  ALL_OBJFILE_OSECTIONS (objfile, iter)
	    {
	      if (bfd_is_const_section (iter->the_bfd_section))
		continue;

	      if (obj_section_addr (iter) <= symbol_value
		  && symbol_value < obj_section_endaddr (iter))
		{
		  section = iter->the_bfd_section;
		  break;
		}
	    }
	}

      prim_record_minimal_symbol_and_info (symname, symbol_value, ms_type,
					   gdb_bfd_section_index (objfile->obfd,
								  section),
					   objfile);
    }

  do_cleanups (cleanup);
}

/* Scan and build partial symbols for a symbol file.
   We have been initialized by a call to som_symfile_init, which 
   currently does nothing.

   SECTION_OFFSETS is a set of offsets to apply to relocate the symbols
   in each section.  This is ignored, as it isn't needed for SOM.

   This function only does the minimum work necessary for letting the
   user "name" things symbolically; it does not read the entire symtab.
   Instead, it reads the external and static symbols and puts them in partial
   symbol tables.  When more extensive information is requested of a
   file, the corresponding partial symbol table is mutated into a full
   fledged symbol table by going back and reading the symbols
   for real.

   We look for sections with specific names, to tell us what debug
   format to look for.

   somstab_build_psymtabs() handles STABS symbols.

   Note that SOM files have a "minimal" symbol table, which is vaguely
   reminiscent of a COFF symbol table, but has only the minimal information
   necessary for linking.  We process this also, and use the information to
   build gdb's minimal symbol table.  This gives us some minimal debugging
   capability even for files compiled without -g.  */

static void
som_symfile_read (struct objfile *objfile, int symfile_flags)
{
  bfd *abfd = objfile->obfd;
  struct cleanup *back_to;

  init_minimal_symbol_collection ();
  back_to = make_cleanup_discard_minimal_symbols ();

  /* Process the normal SOM symbol table first.
     This reads in the DNTT and string table, but doesn't
     actually scan the DNTT.  It does scan the linker symbol
     table and thus build up a "minimal symbol table".  */

  som_symtab_read (abfd, objfile, objfile->section_offsets);

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for this objfile.
     Further symbol-reading is done incrementally, file-by-file,
     in a step known as "psymtab-to-symtab" expansion.  hp-symtab-read.c
     contains the code to do the actual DNTT scanning and symtab building.  */
  install_minimal_symbols (objfile);
  do_cleanups (back_to);

  /* Now read information from the stabs debug sections.
     This is emitted by gcc.  */
  stabsect_build_psymtabs (objfile,
			   "$GDB_SYMBOLS$", "$GDB_STRINGS$", "$TEXT$");
}

/* Initialize anything that needs initializing when a completely new symbol
   file is specified (not just adding some symbols from another file, e.g. a
   shared library).

   We reinitialize buildsym, since we may be reading stabs from a SOM file.  */

static void
som_new_init (struct objfile *ignore)
{
  stabsread_new_init ();
  buildsym_new_init ();
}

/* Perform any local cleanups required when we are done with a particular
   objfile.  I.e, we are in the process of discarding all symbol information
   for an objfile, freeing up all memory held for it, and unlinking the
   objfile struct from the global list of known objfiles.  */

static void
som_symfile_finish (struct objfile *objfile)
{
}

/* SOM specific initialization routine for reading symbols.  */

static void
som_symfile_init (struct objfile *objfile)
{
  /* SOM objects may be reordered, so set OBJF_REORDERED.  If we
     find this causes a significant slowdown in gdb then we could
     set it in the debug symbol readers only when necessary.  */
  objfile->flags |= OBJF_REORDERED;
}

/* An object of this type is passed to find_section_offset.  */

struct find_section_offset_arg
{
  /* The objfile.  */

  struct objfile *objfile;

  /* Flags to invert.  */

  flagword invert;

  /* Flags to look for.  */

  flagword flags;

  /* A text section with non-zero size, if any.  */

  asection *best_section;

  /* An empty text section, if any.  */

  asection *empty_section;
};

/* A callback for bfd_map_over_sections that tries to find a section
   with particular flags in an objfile.  */

static void
find_section_offset (bfd *abfd, asection *sect, void *arg)
{
  struct find_section_offset_arg *info = arg;
  flagword aflag;

  aflag = bfd_get_section_flags (abfd, sect);

  aflag ^= info->invert;

  if ((aflag & info->flags) == info->flags)
    {
      if (bfd_section_size (abfd, sect) > 0)
	{
	  if (info->best_section == NULL)
	    info->best_section = sect;
	}
      else
	{
	  if (info->empty_section == NULL)
	    info->empty_section = sect;
	}
    }
}

/* Set a section index from a BFD.  */

static void
set_section_index (struct objfile *objfile, flagword invert, flagword flags,
		   int *index_ptr)
{
  struct find_section_offset_arg info;

  info.objfile = objfile;
  info.best_section = NULL;
  info.empty_section = NULL;
  info.invert = invert;
  info.flags = flags;
  bfd_map_over_sections (objfile->obfd, find_section_offset, &info);

  if (info.best_section)
    *index_ptr = info.best_section->index;
  else if (info.empty_section)
    *index_ptr = info.empty_section->index;
}

/* SOM specific parsing routine for section offsets.

   Plain and simple for now.  */

static void
som_symfile_offsets (struct objfile *objfile,
		     const struct section_addr_info *addrs)
{
  int i;
  CORE_ADDR text_addr;
  asection *sect;

  objfile->num_sections = bfd_count_sections (objfile->obfd);
  objfile->section_offsets = (struct section_offsets *)
    obstack_alloc (&objfile->objfile_obstack, 
		   SIZEOF_N_SECTION_OFFSETS (objfile->num_sections));

  set_section_index (objfile, 0, SEC_ALLOC | SEC_CODE,
		     &objfile->sect_index_text);
  set_section_index (objfile, 0, SEC_ALLOC | SEC_DATA,
		     &objfile->sect_index_data);
  set_section_index (objfile, SEC_LOAD, SEC_ALLOC | SEC_LOAD,
		     &objfile->sect_index_bss);
  set_section_index (objfile, 0, SEC_ALLOC | SEC_READONLY,
		     &objfile->sect_index_rodata);

  /* First see if we're a shared library.  If so, get the section
     offsets from the library, else get them from addrs.  */
  if (!som_solib_section_offsets (objfile, objfile->section_offsets))
    {
      /* Note: Here is OK to compare with ".text" because this is the
         name that gdb itself gives to that section, not the SOM
         name.  */
      for (i = 0; i < addrs->num_sections; i++)
	if (strcmp (addrs->other[i].name, ".text") == 0)
	  break;
      text_addr = addrs->other[i].addr;

      for (i = 0; i < objfile->num_sections; i++)
	(objfile->section_offsets)->offsets[i] = text_addr;
    }
}



/* Register that we are able to handle SOM object file formats.  */

static const struct sym_fns som_sym_fns =
{
  som_new_init,			/* init anything gbl to entire symtab */
  som_symfile_init,		/* read initial info, setup for sym_read() */
  som_symfile_read,		/* read a symbol file into symtab */
  NULL,				/* sym_read_psymbols */
  som_symfile_finish,		/* finished with file, cleanup */
  som_symfile_offsets,		/* Translate ext. to int. relocation */
  default_symfile_segments,	/* Get segment information from a file.  */
  NULL,
  default_symfile_relocate,	/* Relocate a debug section.  */
  NULL,				/* sym_get_probes */
  &psym_functions
};

initialize_file_ftype _initialize_somread;

void
_initialize_somread (void)
{
  add_symtab_fns (bfd_target_som_flavour, &som_sym_fns);
}
