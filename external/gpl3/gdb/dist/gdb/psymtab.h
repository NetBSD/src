/* Public partial symbol table definitions.

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

#ifndef PSYMTAB_H
#define PSYMTAB_H

#include "gdb_obstack.h"
#include "symfile.h"
#include "gdbsupport/next-iterator.h"
#include "bcache.h"

struct partial_symbol;

/* An instance of this class manages the partial symbol tables and
   partial symbols for a given objfile.

   The core psymtab functions -- those in psymtab.c -- arrange for
   nearly all psymtab- and psymbol-related allocations to happen
   either in the psymtab_storage object (either on its obstack or in
   other memory managed by this class), or on the per-BFD object.  The
   only link from the psymtab storage object back to the objfile (or
   objfile_obstack) that is made by the core psymtab code is the
   compunit_symtab member in the standard_psymtab -- and a given
   symbol reader can avoid this by implementing its own subclasses of
   partial_symtab.

   However, it is up to each symbol reader to maintain this invariant
   in other ways, if it wants to reuse psymtabs across multiple
   objfiles.  The main issue here is ensuring that read_symtab_private
   does not point into objfile_obstack.  */

class psymtab_storage
{
public:

  psymtab_storage ();

  ~psymtab_storage ();

  DISABLE_COPY_AND_ASSIGN (psymtab_storage);

  /* Discard all partial symbol tables starting with "psymtabs" and
     proceeding until "to" has been discarded.  */

  void discard_psymtabs_to (struct partial_symtab *to)
  {
    while (psymtabs != to)
      discard_psymtab (psymtabs);
  }

  /* Discard the partial symbol table.  */

  void discard_psymtab (struct partial_symtab *pst);

  /* Return the obstack that is used for storage by this object.  */

  struct obstack *obstack ()
  {
    if (!m_obstack.has_value ())
      m_obstack.emplace ();
    return &*m_obstack;
  }

  /* Allocate storage for the "dependencies" field of a psymtab.
     NUMBER says how many dependencies there are.  */

  struct partial_symtab **allocate_dependencies (int number)
  {
    return OBSTACK_CALLOC (obstack (), number, struct partial_symtab *);
  }

  /* Install a psymtab on the psymtab list.  This transfers ownership
     of PST to this object.  */

  void install_psymtab (partial_symtab *pst);

  typedef next_adapter<struct partial_symtab> partial_symtab_range;

  /* A range adapter that makes it possible to iterate over all
     psymtabs in one objfile.  */

  partial_symtab_range range ()
  {
    return partial_symtab_range (psymtabs);
  }


  /* Each objfile points to a linked list of partial symtabs derived from
     this file, one partial symtab structure for each compilation unit
     (source file).  */

  struct partial_symtab *psymtabs = nullptr;

  /* Map addresses to the entries of PSYMTABS.  It would be more efficient to
     have a map per the whole process but ADDRMAP cannot selectively remove
     its items during FREE_OBJFILE.  This mapping is already present even for
     PARTIAL_SYMTABs which still have no corresponding full SYMTABs read.

     The DWARF parser reuses this addrmap to store things other than
     psymtabs in the cases where debug information is being read from, for
     example, the .debug-names section.  */

  struct addrmap *psymtabs_addrmap = nullptr;

  /* A byte cache where we can stash arbitrary "chunks" of bytes that
     will not change.  */

  gdb::bcache psymbol_cache;

  /* Vectors of all partial symbols read in from file.  The actual data
     is stored in the objfile_obstack.  */

  std::vector<partial_symbol *> global_psymbols;
  std::vector<partial_symbol *> static_psymbols;

  /* Stack of vectors of partial symbols, using during psymtab
     initialization.  */

  std::vector<std::vector<partial_symbol *>*> current_global_psymbols;
  std::vector<std::vector<partial_symbol *>*> current_static_psymbols;

private:

  /* The obstack where allocations are made.  This is lazily allocated
     so that we don't waste memory when there are no psymtabs.  */

  gdb::optional<auto_obstack> m_obstack;
};


extern const struct quick_symbol_functions psym_functions;

extern const struct quick_symbol_functions dwarf2_gdb_index_functions;
extern const struct quick_symbol_functions dwarf2_debug_names_functions;

/* Ensure that the partial symbols for OBJFILE have been loaded.  If
   VERBOSE is true, then this will print a message when symbols
   are loaded.  This function returns a range adapter suitable for
   iterating over the psymtabs of OBJFILE.  */

extern psymtab_storage::partial_symtab_range require_partial_symbols
    (struct objfile *objfile, bool verbose);

#endif /* PSYMTAB_H */
