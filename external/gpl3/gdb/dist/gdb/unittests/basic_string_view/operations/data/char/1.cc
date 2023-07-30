// { dg-options "-std=gnu++17" }

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

// string_view operations

namespace operations_data_1 {

static int
test01 ()
{
  gdb::string_view empty;

  VERIFY( empty.size() == 0 );
  const gdb::string_view::value_type* p = empty.data();
  VERIFY( p == nullptr );

  return 0;
}

static int
main ()
{ 
  test01();

  return 0;
}

} // namespace operations_data_1
