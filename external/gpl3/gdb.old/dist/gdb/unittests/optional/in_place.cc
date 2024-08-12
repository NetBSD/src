// Copyright (C) 2013-2023 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

namespace in_place {

static void
test ()
{
  // [20.5.5] In-place construction
  {
    gdb::optional<int> o { gdb::in_place };
    VERIFY( o );
    VERIFY( *o == int() );

#ifndef GDB_OPTIONAL
    static_assert( !std::is_convertible<gdb::in_place_t, gdb::optional<int>>(), "" );
#endif
  }

  {
    gdb::optional<int> o { gdb::in_place, 42 };
    VERIFY( o );
    VERIFY( *o == 42 );
  }

  {
    gdb::optional<std::vector<int>> o { gdb::in_place, 18, 4 };
    VERIFY( o );
    VERIFY( o->size() == 18 );
    VERIFY( (*o)[17] == 4 );
  }

#ifndef GDB_OPTIONAL
  {
    gdb::optional<std::vector<int>> o { gdb::in_place, { 18, 4 } };
    VERIFY( o );
    VERIFY( o->size() == 2 );
    VERIFY( (*o)[0] == 18 );
  }
#endif

#ifndef GDB_OPTIONAL
  {
    gdb::optional<std::vector<int>> o { gdb::in_place, { 18, 4 }, std::allocator<int> {} };
    VERIFY( o );
    VERIFY( o->size() == 2 );
    VERIFY( (*o)[0] == 18 );
  }
#endif
}

} // namespace in_place
