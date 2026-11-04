/* Read dbx symbol tables and convert to internal format, for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

/* This module provides three functions: dbx_symfile_init,
   which initializes to read a symbol file; dbx_new_init, which 
   discards existing cached information when all symbols are being
   discarded; and dbx_symfile_read, which reads a symbol table
   from a file.

   dbx_symfile_read only does the minimum work necessary for letting the
   user "name" things symbolically; it does not read the entire symtab.
   Instead, it reads the external and static symbols and puts them in partial
   symbol tables.  When more extensive information is requested of a
   file, the corresponding partial symbol table is mutated into a full
   fledged symbol table by going back and reading the symbols
   for real.  dbx_psymtab_to_symtab() is the function that does this */


#include "event-top.h"
#include "gdbsupport/gdb_obstack.h"
#include <sys/stat.h>
#include "symtab.h"
#include "breakpoint.h"
#include "target.h"
#include "gdbcore.h"
#include "libaout.h"
#include "filenames.h"
#include "objfiles.h"
#include "buildsym-legacy.h"
#include "stabsread.h"
#include "gdb-stabs.h"
#include "demangle.h"
#include "complaints.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "c-lang.h"
#include "psymtab.h"
#include "block.h"
#include "aout/aout64.h"
#include "aout/stab_gnu.h"

/* Required for the following registry.  */
#include "gdb-stabs.h"





/* Local function prototypes.  */

static void dbx_symfile_init (struct objfile *);

static void dbx_new_init (struct objfile *);

static void dbx_symfile_read (struct objfile *, symfile_add_flags);

static void dbx_symfile_finish (struct objfile *);


#if 0
static struct type **
explicit_lookup_type (int real_filenum, int index)
{
  struct header_file *f = &HEADER_FILES (dbxread_objfile)[real_filenum];

  if (index >= f->length)
    {
      f->length *= 2;
      f->vector = (struct type **)
	xrealloc (f->vector, f->length * sizeof (struct type *));
      memset (&f->vector[f->length / 2],
	      '\0', f->length * sizeof (struct type *) / 2);
    }
  return &f->vector[index];
}
#endif

/* Scan and build partial symbols for a symbol file.
   We have been initialized by a call to dbx_symfile_init, which 
   put all the relevant info into a "struct dbx_symfile_info",
   hung off the objfile structure.  */

static void
dbx_symfile_read (struct objfile *objfile, symfile_add_flags symfile_flags)
{
  read_stabs_symtab (objfile, symfile_flags);
}

/* Initialize anything that needs initializing when a completely new
   symbol file is specified (not just adding some symbols from another
   file, e.g. a shared library).  */

static void
dbx_new_init (struct objfile *ignore)
{
  stabsread_new_init ();
  init_header_files ();
}


/* dbx_symfile_init ()
   is the dbx-specific initialization routine for reading symbols.
   It is passed a struct objfile which contains, among other things,
   the BFD for the file whose symbols are being read, and a slot for a pointer
   to "private data" which we fill with goodies.

   We read the string table into malloc'd space and stash a pointer to it.

   Since BFD doesn't know how to read debug symbols in a format-independent
   way (and may never do so...), we have to do it ourselves.  We will never
   be called unless this is an a.out (or very similar) file.
   FIXME, there should be a cleaner peephole into the BFD environment here.  */

#define DBX_STRINGTAB_SIZE_SIZE sizeof(long)	/* FIXME */

static void
dbx_symfile_init (struct objfile *objfile)
{
  int val;
  bfd *sym_bfd = objfile->obfd.get ();
  const char *name = bfd_get_filename (sym_bfd);
  asection *text_sect;
  unsigned char size_temp[DBX_STRINGTAB_SIZE_SIZE];

  /* Allocate struct to keep track of the symfile.  */
  dbx_objfile_data_key.emplace (objfile);

  DBX_TEXT_SECTION (objfile) = bfd_get_section_by_name (sym_bfd, ".text");
  DBX_DATA_SECTION (objfile) = bfd_get_section_by_name (sym_bfd, ".data");
  DBX_BSS_SECTION (objfile) = bfd_get_section_by_name (sym_bfd, ".bss");

  /* FIXME POKING INSIDE BFD DATA STRUCTURES.  */
#define	STRING_TABLE_OFFSET	(sym_bfd->origin + obj_str_filepos (sym_bfd))
#define	SYMBOL_TABLE_OFFSET	(sym_bfd->origin + obj_sym_filepos (sym_bfd))

  /* FIXME POKING INSIDE BFD DATA STRUCTURES.  */

  text_sect = bfd_get_section_by_name (sym_bfd, ".text");
  if (!text_sect)
    error (_("Can't find .text section in symbol file"));
  DBX_TEXT_ADDR (objfile) = bfd_section_vma (text_sect);
  DBX_TEXT_SIZE (objfile) = bfd_section_size (text_sect);

  DBX_SYMBOL_SIZE (objfile) = obj_symbol_entry_size (sym_bfd);
  DBX_SYMCOUNT (objfile) = bfd_get_symcount (sym_bfd);
  DBX_SYMTAB_OFFSET (objfile) = SYMBOL_TABLE_OFFSET;

  /* Read the string table and stash it away in the objfile_obstack.
     When we blow away the objfile the string table goes away as well.
     Note that gdb used to use the results of attempting to malloc the
     string table, based on the size it read, as a form of sanity check
     for botched byte swapping, on the theory that a byte swapped string
     table size would be so totally bogus that the malloc would fail.  Now
     that we put in on the objfile_obstack, we can't do this since gdb gets
     a fatal error (out of virtual memory) if the size is bogus.  We can
     however at least check to see if the size is less than the size of
     the size field itself, or larger than the size of the entire file.
     Note that all valid string tables have a size greater than zero, since
     the bytes used to hold the size are included in the count.  */

  if (STRING_TABLE_OFFSET == 0)
    {
      /* It appears that with the existing bfd code, STRING_TABLE_OFFSET
	 will never be zero, even when there is no string table.  This
	 would appear to be a bug in bfd.  */
      DBX_STRINGTAB_SIZE (objfile) = 0;
      DBX_STRINGTAB (objfile) = NULL;
    }
  else
    {
      val = bfd_seek (sym_bfd, STRING_TABLE_OFFSET, SEEK_SET);
      if (val < 0)
	perror_with_name (name);

      memset (size_temp, 0, sizeof (size_temp));
      val = bfd_read (size_temp, sizeof (size_temp), sym_bfd);
      if (val < 0)
	{
	  perror_with_name (name);
	}
      else if (val == 0)
	{
	  /* With the existing bfd code, STRING_TABLE_OFFSET will be set to
	     EOF if there is no string table, and attempting to read the size
	     from EOF will read zero bytes.  */
	  DBX_STRINGTAB_SIZE (objfile) = 0;
	  DBX_STRINGTAB (objfile) = NULL;
	}
      else
	{
	  /* Read some data that would appear to be the string table size.
	     If there really is a string table, then it is probably the right
	     size.  Byteswap if necessary and validate the size.  Note that
	     the minimum is DBX_STRINGTAB_SIZE_SIZE.  If we just read some
	     random data that happened to be at STRING_TABLE_OFFSET, because
	     bfd can't tell us there is no string table, the sanity checks may
	     or may not catch this.  */
	  DBX_STRINGTAB_SIZE (objfile) = bfd_h_get_32 (sym_bfd, size_temp);

	  if (DBX_STRINGTAB_SIZE (objfile) < sizeof (size_temp)
	      || DBX_STRINGTAB_SIZE (objfile) > bfd_get_size (sym_bfd))
	    error (_("ridiculous string table size (%d bytes)."),
		   DBX_STRINGTAB_SIZE (objfile));

	  DBX_STRINGTAB (objfile) =
	    (char *) obstack_alloc (&objfile->objfile_obstack,
				    DBX_STRINGTAB_SIZE (objfile));
	  OBJSTAT (objfile, sz_strtab += DBX_STRINGTAB_SIZE (objfile));

	  /* Now read in the string table in one big gulp.  */

	  val = bfd_seek (sym_bfd, STRING_TABLE_OFFSET, SEEK_SET);
	  if (val < 0)
	    perror_with_name (name);
	  val = bfd_read (DBX_STRINGTAB (objfile),
			  DBX_STRINGTAB_SIZE (objfile),
			  sym_bfd);
	  if (val != DBX_STRINGTAB_SIZE (objfile))
	    perror_with_name (name);
	}
    }
}

/* Perform any local cleanups required when we are done with a particular
   objfile.  I.E, we are in the process of discarding all symbol information
   for an objfile, freeing up all memory held for it, and unlinking the
   objfile struct from the global list of known objfiles.  */

static void
dbx_symfile_finish (struct objfile *objfile)
{
  free_header_files ();
}






static const struct sym_fns aout_sym_fns =
{
  dbx_new_init,			/* init anything gbl to entire symtab */
  dbx_symfile_init,		/* read initial info, setup for sym_read() */
  dbx_symfile_read,		/* read a symbol file into symtab */
  dbx_symfile_finish,		/* finished with file, cleanup */
  default_symfile_offsets, 	/* parse user's offsets to internal form */
  default_symfile_segments,	/* Get segment information from a file.  */
  NULL,
  default_symfile_relocate,	/* Relocate a debug section.  */
  NULL,				/* sym_probe_fns */
};

void _initialize_dbxread ();
void
_initialize_dbxread ()
{
  add_symtab_fns (bfd_target_aout_flavour, &aout_sym_fns);
}
