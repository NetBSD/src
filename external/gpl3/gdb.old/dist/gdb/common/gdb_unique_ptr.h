/* std::unique_ptr specializations for GDB.

   Copyright (C) 2016-2017 Free Software Foundation, Inc.

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

#ifndef GDB_UNIQUE_PTR_H
#define GDB_UNIQUE_PTR_H 1

#include <memory>

namespace gdb
{
/* Define gdb::unique_xmalloc_ptr, a std::unique_ptr that manages
   xmalloc'ed memory.  */

/* The deleter for std::unique_xmalloc_ptr.  Uses xfree.  */
template <typename T>
struct xfree_deleter
{
  void operator() (T *ptr) const { xfree (ptr); }
};

/* Import the standard unique_ptr to our namespace with a custom
   deleter.  */

template<typename T> using unique_xmalloc_ptr
  = std::unique_ptr<T, xfree_deleter<T>>;

} /* namespace gdb */

#endif /* GDB_UNIQUE_PTR_H */
