/* Base class for mapped indices

   Copyright (C) 2021-2025 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_MAPPED_INDEX_H
#define GDB_DWARF2_MAPPED_INDEX_H

#include "quick-symbol.h"

class cooked_index;

/* Base class of all DWARF scanner types.  */

struct dwarf_scanner_base
{
  dwarf_scanner_base () = default;
  virtual ~dwarf_scanner_base () = default;
  DISABLE_COPY_AND_ASSIGN (dwarf_scanner_base);

  /* Start the reading.  This is only really relevant to the cooked
     index; see cooked-index.h.  */
  virtual void start_reading ()
  { }

  /* Return a quick_symbol_functions instance that refers back to this
     dwarf_scanner_base.  */
  virtual quick_symbol_functions_up make_quick_functions () const = 0;

  /* An ad hoc version check.  This is needed for .gdb_index to check
     whether a version 8 or above index is in use.  Returns true if
     the index is usable, false otherwise.  */
  virtual bool version_check () const
  {
    return true;
  }

  /* This is called when writing an index.  For a cooked index, it
     will return 'this' as a cooked index.  For other forms, it will
     throw an exception with an appropriate error message.  */
  virtual cooked_index *index_for_writing () = 0;

  /* Wait for reading of the debuginfo to be completely finished.
     This normally has a trivial implementation, but if a subclass
     does any background reading, it's needed to ensure that the
     reading is completed before destroying the containing per-BFD
     object.  */
  virtual void wait_completely ()
  {
  }

  /* Look up ADDR, and return either the corresponding CU, or nullptr
     if the address could not be found.  */
  virtual dwarf2_per_cu *lookup (unrelocated_addr addr)
  { return nullptr; }
};

using dwarf_scanner_base_up = std::unique_ptr<dwarf_scanner_base>;

#endif /* GDB_DWARF2_MAPPED_INDEX_H */
