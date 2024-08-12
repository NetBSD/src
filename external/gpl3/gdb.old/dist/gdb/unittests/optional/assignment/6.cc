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

namespace assign_6 {

static int counter = 0;

struct mixin_counter
{
  mixin_counter() { ++counter; }
  mixin_counter(mixin_counter const&) { ++counter; }
  ~mixin_counter() { --counter; }
};

struct value_type : private mixin_counter
{
  value_type() = default;
  value_type(int) : state(1) { }
  value_type(std::initializer_list<char>, const char*) : state(2) { }
  int state = 0;
};

static void
test ()
{
  using O = gdb::optional<value_type>;

  // Check emplace

  {
    O o;
    o.emplace();
    VERIFY( o && o->state == 0 );
  }
  {
    O o { gdb::in_place, 0 };
    o.emplace();
    VERIFY( o && o->state == 0 );
  }

  {
    O o;
    o.emplace(0);
    VERIFY( o && o->state == 1 );
  }
  {
    O o { gdb::in_place };
    o.emplace(0);
    VERIFY( o && o->state == 1 );
  }

#ifndef GDB_OPTIONAL
  {
    O o;
    o.emplace({ 'a' }, "");
    VERIFY( o && o->state == 2 );
  }
  {
    O o { gdb::in_place };
    o.emplace({ 'a' }, "");
    VERIFY( o && o->state == 2 );
  }
#endif
  {
    O o;
    VERIFY(&o.emplace(0) == &*o);
#ifndef GDB_OPTIONAL
    VERIFY(&o.emplace({ 'a' }, "") == &*o);
#endif
  }

  static_assert( !std::is_constructible<O, std::initializer_list<int>, int>(), "" );

  VERIFY( counter == 0 );
}

} // namespace assign_6
