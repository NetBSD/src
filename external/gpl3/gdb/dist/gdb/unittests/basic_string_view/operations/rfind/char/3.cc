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

namespace operations_rfind_3 {

// basic_string_view::find_last_not_of

static void
test03 ()
{
  typedef gdb::string_view::size_type csize_type;
  gdb::string_view::size_type pos;
  csize_type npos = gdb::string_view::npos;

  gdb::string_view x;
  pos = x.find_last_not_of('X');
  VERIFY( pos == npos );
  pos = x.find_last_not_of("XYZ");
  VERIFY( pos == npos );

  gdb::string_view y("a");
  pos = y.find_last_not_of('X');
  VERIFY( pos == 0 );
  pos = y.find_last_not_of('a');
  VERIFY( pos == npos );
  pos = y.find_last_not_of("XYZ");
  VERIFY( pos == 0 );
  pos = y.find_last_not_of("a");
  VERIFY( pos == npos );

  gdb::string_view z("ab");
  pos = z.find_last_not_of('X');
  VERIFY( pos == 1 );
  pos = z.find_last_not_of("XYZ");
  VERIFY( pos == 1 );
  pos = z.find_last_not_of('b');
  VERIFY( pos == 0 );
  pos = z.find_last_not_of("Xb");
  VERIFY( pos == 0 );
  pos = z.find_last_not_of("Xa");
  VERIFY( pos == 1 );
}

static int
main ()
{
  test03();

  return 0;
}

} // namespace operations_rfind_3
