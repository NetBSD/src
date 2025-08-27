/* Definitions for symbol-reading containing "stabs", for GDB.
   Copyright (C) 1992-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support.  Written by John Gilmore.

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

#ifndef GDB_GDB_STABS_H
#define GDB_GDB_STABS_H

/* During initial symbol readin, we need to have a structure to keep
   track of which psymtabs have which bincls in them.  This structure
   is used during readin to setup the list of dependencies within each
   partial symbol table.  */
struct legacy_psymtab;

struct header_file_location
{
  header_file_location (const char *name_, int instance_,
			legacy_psymtab *pst_)
    : name (name_),
      instance (instance_),
      pst (pst_)
  {
  }

  const char *name;		/* Name of header file */
  int instance;			/* See above */
  legacy_psymtab *pst;	/* Partial symtab that has the
				   BINCL/EINCL defs for this file.  */
};

/* This file exists to hold the common definitions required of most of
   the symbol-readers that end up using stabs.  The common use of
   these `symbol-type-specific' customizations of the generic data
   structures makes the stabs-oriented symbol readers able to call
   each others' functions as required.  */

struct stabsread_context {
  /* Remember what we deduced to be the source language of this psymtab.  */
  enum language psymtab_language = language_unknown;

  /* The size of each symbol in the symbol file (in external form).
     This is set by dbx_symfile_read when building psymtabs, and by
     dbx_psymtab_to_symtab when building symtabs.  */
  unsigned symbol_size = 0;

  /* This is the offset of the symbol table in the executable file.  */
  unsigned symbol_table_offset = 0;

  /* This is the offset of the string table in the executable file.  */
  unsigned string_table_offset = 0;

  /* For elf+stab executables, the n_strx field is not a simple index
     into the string table.  Instead, each .o file has a base offset in
     the string table, and the associated symbols contain offsets from
     this base.  The following two variables contain the base offset for
     the current and next .o files.  */
  unsigned int file_string_table_offset = 0;

  /* .o and NLM files contain unrelocated addresses which are based at
     0.  When non-zero, this flag disables some of the special cases for
     Solaris elf+stab text addresses at location 0.  */
  int symfile_relocatable = 0;

  /* When set, we are processing a .o file compiled by sun acc.  This is
     misnamed; it refers to all stabs-in-elf implementations which use
     N_UNDF the way Sun does, including Solaris gcc.  Hopefully all
     stabs-in-elf implementations ever invented will choose to be
     compatible.  */
  unsigned char processing_acc_compilation = 0;

  /* The lowest text address we have yet encountered.  This is needed
     because in an a.out file, there is no header field which tells us
     what address the program is actually going to be loaded at, so we
     need to make guesses based on the symbols (which *are* relocated to
     reflect the address it will be loaded at).  */
  unrelocated_addr lowest_text_address;

  /* Non-zero if there is any line number info in the objfile.  Prevents
     dbx_end_psymtab from discarding an otherwise empty psymtab.  */
  int has_line_numbers = 0;

  /* The list of bincls.  */
  std::vector<struct header_file_location> bincl_list;

  /* Name of last function encountered.  Used in Solaris to approximate
     object file boundaries.  */
  const char *last_function_name = nullptr;

  /* The address in memory of the string table of the object file we are
     reading (which might not be the "main" object file, but might be a
     shared library or some other dynamically loaded thing).  This is
     set by read_dbx_symtab when building psymtabs, and by
     read_ofile_symtab when building symtabs, and is used only by
     next_symbol_text.  FIXME: If that is true, we don't need it when
     building psymtabs, right?  */
  char *stringtab_global = nullptr;

  /* These variables are used to control fill_symbuf when the stabs
     symbols are not contiguous (as may be the case when a COFF file is
     linked using --split-by-reloc).  */
  const std::vector<asection *> *symbuf_sections;
  size_t sect_idx = 0;
  unsigned int symbuf_left = 0;
  unsigned int symbuf_read = 0;

  /* This variable stores a global stabs buffer, if we read stabs into
     memory in one chunk in order to process relocations.  */
  bfd_byte *stabs_data = nullptr;
};


/* Information is passed among various dbxread routines for accessing
   symbol files.  A pointer to this structure is kept in the objfile,
   using the dbx_objfile_data_key.  */

struct dbx_symfile_info
  {
    ~dbx_symfile_info ();

    CORE_ADDR text_addr = 0;	/* Start of text section */
    int text_size = 0;		/* Size of text section */
    int symcount = 0;		/* How many symbols are there in the file */
    char *stringtab = nullptr;		/* The actual string table */
    int stringtab_size = 0;		/* Its size */
    file_ptr symtab_offset = 0;	/* Offset in file to symbol table */
    int symbol_size = 0;		/* Bytes in a single symbol */

    stabsread_context ctx; /* Context for the symfile being read.  */

    /* See stabsread.h for the use of the following.  */
    struct header_file *header_files = nullptr;
    int n_header_files = 0;
    int n_allocated_header_files = 0;

    /* Pointers to BFD sections.  These are used to speed up the building of
       minimal symbols.  */
    asection *text_section = nullptr;
    asection *data_section = nullptr;
    asection *bss_section = nullptr;

    /* Pointer to the separate ".stab" section, if there is one.  */
    asection *stab_section = nullptr;
  };

/* The tag used to find the DBX info attached to an objfile.  This is
   global because it is referenced by several modules.  */
extern const registry<objfile>::key<dbx_symfile_info> dbx_objfile_data_key;

#define DBX_SYMFILE_INFO(o)	(dbx_objfile_data_key.get (o))
#define DBX_TEXT_ADDR(o)	(DBX_SYMFILE_INFO(o)->text_addr)
#define DBX_TEXT_SIZE(o)	(DBX_SYMFILE_INFO(o)->text_size)
#define DBX_SYMCOUNT(o)		(DBX_SYMFILE_INFO(o)->symcount)
#define DBX_STRINGTAB(o)	(DBX_SYMFILE_INFO(o)->stringtab)
#define DBX_STRINGTAB_SIZE(o)	(DBX_SYMFILE_INFO(o)->stringtab_size)
#define DBX_SYMTAB_OFFSET(o)	(DBX_SYMFILE_INFO(o)->symtab_offset)
#define DBX_SYMBOL_SIZE(o)	(DBX_SYMFILE_INFO(o)->symbol_size)
#define DBX_TEXT_SECTION(o)	(DBX_SYMFILE_INFO(o)->text_section)
#define DBX_DATA_SECTION(o)	(DBX_SYMFILE_INFO(o)->data_section)
#define DBX_BSS_SECTION(o)	(DBX_SYMFILE_INFO(o)->bss_section)
#define DBX_STAB_SECTION(o)	(DBX_SYMFILE_INFO(o)->stab_section)

#endif /* GDB_GDB_STABS_H */
