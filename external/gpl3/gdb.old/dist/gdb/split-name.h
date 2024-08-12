/* Split a symbol name.

   Copyright (C) 2022-2023 Free Software Foundation, Inc.

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

#ifndef GDB_SPLIT_NAME_H
#define GDB_SPLIT_NAME_H

#include "gdbsupport/gdb_string_view.h"

/* The available styles of name splitting.  */

enum class split_style
{
  /* No splitting - C style.  */
  NONE,
  /* C++ style, with "::" and template parameter intelligence.  */
  CXX,
  /* Split at ".".  Used by Ada, Go, D.  */
  DOT,
  /* Split at "__".  Used by Ada encoded names.  */
  UNDERSCORE,
};

/* Split NAME into components at module boundaries.  STYLE indicates
   which style of splitting to use.  */

extern std::vector<gdb::string_view> split_name (const char *name,
						 split_style style);

#endif /* GDB_SPLIT_NAME_H */
