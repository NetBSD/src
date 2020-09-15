/* Private partial symbol table definitions.

   Copyright (C) 2009-2020 Free Software Foundation, Inc.

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

#ifndef PSYMPRIV_H
#define PSYMPRIV_H

#include "psymtab.h"
#include "objfiles.h"
#include "gdbsupport/gdb_string_view.h"

/* A partial_symbol records the name, domain, and address class of
   symbols whose types we have not parsed yet.  For functions, it also
   contains their memory address, so we can find them from a PC value.
   Each partial_symbol sits in a partial_symtab, all of which are chained
   on a  partial symtab list and which points to the corresponding
   normal symtab once the partial_symtab has been referenced.  */

/* This structure is space critical.  See space comments at the top of
   symtab.h.  */

struct partial_symbol
{
  /* Return the section for this partial symbol, or nullptr if no
     section has been set.  */
  struct obj_section *obj_section (struct objfile *objfile) const
  {
    if (ginfo.section >= 0)
      return &objfile->sections[ginfo.section];
    return nullptr;
  }

  /* Return the unrelocated address of this partial symbol.  */
  CORE_ADDR unrelocated_address () const
  {
    return ginfo.value.address;
  }

  /* Return the address of this partial symbol, relocated according to
     the offsets provided in OBJFILE.  */
  CORE_ADDR address (const struct objfile *objfile) const
  {
    return ginfo.value.address + objfile->section_offsets[ginfo.section];
  }

  /* Set the address of this partial symbol.  The address must be
     unrelocated.  */
  void set_unrelocated_address (CORE_ADDR addr)
  {
    ginfo.value.address = addr;
  }

  /* Note that partial_symbol does not derive from general_symbol_info
     due to the bcache.  See add_psymbol_to_bcache.  */

  struct general_symbol_info ginfo;

  /* Name space code.  */

  ENUM_BITFIELD(domain_enum_tag) domain : SYMBOL_DOMAIN_BITS;

  /* Address class (for info_symbols).  Note that we don't allow
     synthetic "aclass" values here at present, simply because there's
     no need.  */

  ENUM_BITFIELD(address_class) aclass : SYMBOL_ACLASS_BITS;
};

/* A convenience enum to give names to some constants used when
   searching psymtabs.  This is internal to psymtab and should not be
   used elsewhere.  */

enum psymtab_search_status
  {
    PST_NOT_SEARCHED,
    PST_SEARCHED_AND_FOUND,
    PST_SEARCHED_AND_NOT_FOUND
  };

/* Each source file that has not been fully read in is represented by
   a partial_symtab.  This contains the information on where in the
   executable the debugging symbols for a specific file are, and a
   list of names of global symbols which are located in this file.
   They are all chained on partial symtab lists.

   Even after the source file has been read into a symtab, the
   partial_symtab remains around.  */

struct partial_symtab
{
  /* Allocate a new partial symbol table associated with OBJFILE.
     FILENAME (which must be non-NULL) is the filename of this partial
     symbol table; it is copied into the appropriate storage.  The
     partial symtab will also be installed using
     psymtab_storage::install.  */

  partial_symtab (const char *filename, struct objfile *objfile)
    ATTRIBUTE_NONNULL (2) ATTRIBUTE_NONNULL (3);

  /* Like the above, but also sets the initial text low and text high
     from the ADDR argument, and sets the global- and
     static-offsets.  */

  partial_symtab (const char *filename, struct objfile *objfile,
		  CORE_ADDR addr)
    ATTRIBUTE_NONNULL (2) ATTRIBUTE_NONNULL (3);

  virtual ~partial_symtab ()
  {
  }

  /* Psymtab expansion is done in two steps:
     - a call to read_symtab
     - while that call is in progress, calls to expand_psymtab can be made,
       both for this psymtab, and its dependencies.
     This makes a distinction between a toplevel psymtab (for which both
     read_symtab and expand_psymtab will be called) and a non-toplevel
     psymtab (for which only expand_psymtab will be called). The
     distinction can be used f.i. to do things before and after all
     dependencies of a top-level psymtab have been expanded.

     Read the full symbol table corresponding to this partial symbol
     table.  Typically calls expand_psymtab.  */
  virtual void read_symtab (struct objfile *) = 0;

  /* Expand the full symbol table for this partial symbol table.  Typically
     calls expand_dependencies.  */
  virtual void expand_psymtab (struct objfile *) = 0;

  /* Ensure that all the dependencies are read in.  Calls
     expand_psymtab for each non-shared dependency.  */
  void expand_dependencies (struct objfile *);

  /* Return true if the symtab corresponding to this psymtab has been
     read in in the context of this objfile.  */
  virtual bool readin_p (struct objfile *) const = 0;

  /* Return a pointer to the compunit allocated for this source file
     in the context of this objfile.

     Return nullptr if the compunit was not read in or if there was no
     symtab.  */
  virtual struct compunit_symtab *get_compunit_symtab
    (struct objfile *) const = 0;

  /* Return the raw low text address of this partial_symtab.  */
  CORE_ADDR raw_text_low () const
  {
    return m_text_low;
  }

  /* Return the raw high text address of this partial_symtab.  */
  CORE_ADDR raw_text_high () const
  {
    return m_text_high;
  }

  /* Return the relocated low text address of this partial_symtab.  */
  CORE_ADDR text_low (struct objfile *objfile) const
  {
    return m_text_low + objfile->text_section_offset ();
  }

  /* Return the relocated high text address of this partial_symtab.  */
  CORE_ADDR text_high (struct objfile *objfile) const
  {
    return m_text_high + objfile->text_section_offset ();
  }

  /* Set the low text address of this partial_symtab.  */
  void set_text_low (CORE_ADDR addr)
  {
    m_text_low = addr;
    text_low_valid = 1;
  }

  /* Set the hight text address of this partial_symtab.  */
  void set_text_high (CORE_ADDR addr)
  {
    m_text_high = addr;
    text_high_valid = 1;
  }


  /* Chain of all existing partial symtabs.  */

  struct partial_symtab *next = nullptr;

  /* Name of the source file which this partial_symtab defines,
     or if the psymtab is anonymous then a descriptive name for
     debugging purposes, or "".  It must not be NULL.  */

  const char *filename = nullptr;

  /* Full path of the source file.  NULL if not known.  */

  char *fullname = nullptr;

  /* Directory in which it was compiled, or NULL if we don't know.  */

  const char *dirname = nullptr;

  /* Range of text addresses covered by this file; texthigh is the
     beginning of the next section.  Do not use if PSYMTABS_ADDRMAP_SUPPORTED
     is set.  Do not refer directly to these fields.  Instead, use the
     accessors.  The validity of these fields is determined by the
     text_low_valid and text_high_valid fields; these are located later
     in this structure for better packing.  */

  CORE_ADDR m_text_low = 0;
  CORE_ADDR m_text_high = 0;

  /* If NULL, this is an ordinary partial symbol table.

     If non-NULL, this holds a single includer of this partial symbol
     table, and this partial symbol table is a shared one.

     A shared psymtab is one that is referenced by multiple other
     psymtabs, and which conceptually has its contents directly
     included in those.

     Shared psymtabs have special semantics.  When a search finds a
     symbol in a shared table, we instead return one of the non-shared
     tables that include this one.

     A shared psymtabs can be referred to by other shared ones.

     The psymtabs that refer to a shared psymtab will list the shared
     psymtab in their 'dependencies' array.

     In DWARF terms, a shared psymtab is a DW_TAG_partial_unit; but
     of course using a name based on that would be too confusing, so
     "shared" was chosen instead.

     Only a single user is needed because, when expanding a shared
     psymtab, we only need to expand its "canonical" non-shared user.
     The choice of which one should be canonical is left to the
     debuginfo reader; it can be arbitrary.  */

  struct partial_symtab *user = nullptr;

  /* Array of pointers to all of the partial_symtab's which this one
     depends on.  Since this array can only be set to previous or
     the current (?) psymtab, this dependency tree is guaranteed not
     to have any loops.  "depends on" means that symbols must be read
     for the dependencies before being read for this psymtab; this is
     for type references in stabs, where if foo.c includes foo.h, declarations
     in foo.h may use type numbers defined in foo.c.  For other debugging
     formats there may be no need to use dependencies.  */

  struct partial_symtab **dependencies = nullptr;

  int number_of_dependencies = 0;

  /* Global symbol list.  This list will be sorted after readin to
     improve access.  Binary search will be the usual method of
     finding a symbol within it.  globals_offset is an integer offset
     within global_psymbols[].  */

  int globals_offset = 0;
  int n_global_syms = 0;

  /* Static symbol list.  This list will *not* be sorted after readin;
     to find a symbol in it, exhaustive search must be used.  This is
     reasonable because searches through this list will eventually
     lead to either the read in of a files symbols for real (assumed
     to take a *lot* of time; check) or an error (and we don't care
     how long errors take).  This is an offset and size within
     static_psymbols[].  */

  int statics_offset = 0;
  int n_static_syms = 0;

  /* True iff objfile->psymtabs_addrmap is properly populated for this
     partial_symtab.  For discontiguous overlapping psymtabs is the only usable
     info in PSYMTABS_ADDRMAP.  */

  bool psymtabs_addrmap_supported = false;

  /* True if the name of this partial symtab is not a source file name.  */

  bool anonymous = false;

  /* A flag that is temporarily used when searching psymtabs.  */

  ENUM_BITFIELD (psymtab_search_status) searched_flag : 2;

  /* Validity of the m_text_low and m_text_high fields.  */

  unsigned int text_low_valid : 1;
  unsigned int text_high_valid : 1;
};

/* A partial symtab that tracks the "readin" and "compunit_symtab"
   information in the ordinary way -- by storing it directly in this
   object.  */
struct standard_psymtab : public partial_symtab
{
  standard_psymtab (const char *filename, struct objfile *objfile)
    : partial_symtab (filename, objfile)
  {
  }

  standard_psymtab (const char *filename, struct objfile *objfile,
		    CORE_ADDR addr)
    : partial_symtab (filename, objfile, addr)
  {
  }

  bool readin_p (struct objfile *) const override
  {
    return readin;
  }

  struct compunit_symtab *get_compunit_symtab (struct objfile *) const override
  {
    return compunit_symtab;
  }

  /* True if the symtab corresponding to this psymtab has been
     readin.  */

  bool readin = false;

  /* Pointer to compunit eventually allocated for this source file, 0 if
     !readin or if we haven't looked for the symtab after it was readin.  */

  struct compunit_symtab *compunit_symtab = nullptr;
};

/* A partial_symtab that works in the historical db way.  This should
   not be used in new code, but exists to transition the somewhat
   unmaintained "legacy" debug formats.  */

struct legacy_psymtab : public standard_psymtab
{
  legacy_psymtab (const char *filename, struct objfile *objfile)
    : standard_psymtab (filename, objfile)
  {
  }

  legacy_psymtab (const char *filename, struct objfile *objfile,
		  CORE_ADDR addr)
    : standard_psymtab (filename, objfile, addr)
  {
  }

  void read_symtab (struct objfile *objf) override
  {
    if (legacy_read_symtab)
      (*legacy_read_symtab) (this, objf);
  }

  void expand_psymtab (struct objfile *objf) override
  {
    (*legacy_expand_psymtab) (this, objf);
  }

  /* Pointer to function which will read in the symtab corresponding to
     this psymtab.  */

  void (*legacy_read_symtab) (legacy_psymtab *, struct objfile *) = nullptr;

  /* Pointer to function which will actually expand this psymtab into
     a full symtab.  */

  void (*legacy_expand_psymtab) (legacy_psymtab *, struct objfile *) = nullptr;

  /* Information that lets read_symtab() locate the part of the symbol table
     that this psymtab corresponds to.  This information is private to the
     format-dependent symbol reading routines.  For further detail examine
     the various symbol reading modules.  */

  void *read_symtab_private = nullptr;
};

/* Specify whether a partial psymbol should be allocated on the global
   list or the static list.  */

enum class psymbol_placement
{
  STATIC,
  GLOBAL
};

/* Add a symbol to the partial symbol table of OBJFILE.

   If COPY_NAME is true, make a copy of NAME, otherwise use the passed
   reference.

   THECLASS is the type of symbol.

   SECTION is the index of the section of OBJFILE in which the symbol is found.

   WHERE determines whether the symbol goes in the list of static or global
   partial symbols of OBJFILE.

   COREADDR is the address of the symbol.  For partial symbols that don't have
   an address, zero is passed.

   LANGUAGE is the language from which the symbol originates.  This will
   influence, amongst other things, how the symbol name is demangled. */

extern void add_psymbol_to_list (gdb::string_view name,
				 bool copy_name, domain_enum domain,
				 enum address_class theclass,
				 short section,
				 psymbol_placement where,
				 CORE_ADDR coreaddr,
				 enum language language,
				 struct objfile *objfile);

/* Add a symbol to the partial symbol table of OBJFILE.  The psymbol
   must be fully constructed, and the names must be set and intern'd
   as appropriate.  */

extern void add_psymbol_to_list (const partial_symbol &psym,
				 psymbol_placement where,
				 struct objfile *objfile);

/* Initialize storage for partial symbols.  If partial symbol storage
   has already been initialized, this does nothing.  TOTAL_SYMBOLS is
   an estimate of how many symbols there will be.  */

extern void init_psymbol_list (struct objfile *objfile, int total_symbols);

extern void end_psymtab_common (struct objfile *, struct partial_symtab *);

/* Used when recording partial symbol tables.  On destruction,
   discards any partial symbol tables that have been built.  However,
   the tables can be kept by calling the "keep" method.  */
class psymtab_discarder
{
 public:

  psymtab_discarder (struct objfile *objfile)
    : m_objfile (objfile),
      m_psymtab (objfile->partial_symtabs->psymtabs)
  {
  }

  ~psymtab_discarder ()
  {
    if (m_objfile != NULL)
      m_objfile->partial_symtabs->discard_psymtabs_to (m_psymtab);
  }

  /* Keep any partial symbol tables that were built.  */
  void keep ()
  {
    m_objfile = NULL;
  }

 private:

  /* The objfile.  If NULL this serves as a sentinel to indicate that
     the psymtabs should be kept.  */
  struct objfile *m_objfile;
  /* How far back to free.  */
  struct partial_symtab *m_psymtab;
};

#endif /* PSYMPRIV_H */
