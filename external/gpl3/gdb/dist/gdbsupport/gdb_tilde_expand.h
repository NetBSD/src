/* Perform tilde expansion on paths for GDB and gdbserver.

   Copyright (C) 2017-2025 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_GDB_TILDE_EXPAND_H
#define GDBSUPPORT_GDB_TILDE_EXPAND_H

/* Perform tilde expansion on PATH, and return the full path.  */
extern std::string gdb_tilde_expand (const char *path);

/* Overload of gdb_tilde_expand that takes std::string.  */
static inline std::string
gdb_tilde_expand (const std::string &path)
{
  return gdb_tilde_expand (path.c_str ());
}

/* Overload of gdb_tilde_expand that takes gdb::unique_xmalloc_ptr<char>.  */
static inline std::string
gdb_tilde_expand (const gdb::unique_xmalloc_ptr<char> &path)
{
  return gdb_tilde_expand (path.get ());
}

#endif /* GDBSUPPORT_GDB_TILDE_EXPAND_H */
