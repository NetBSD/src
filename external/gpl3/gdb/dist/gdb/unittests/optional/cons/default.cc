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

namespace cons_default {

struct tracker
{
  tracker() { ++count; }
  ~tracker() { --count; }

  tracker(tracker const&) { ++count; }
  tracker(tracker&&) { ++count; }

  tracker& operator=(tracker const&) = default;
  tracker& operator=(tracker&&) = default;

  static int count;
};

int tracker::count = 0;

static void
test ()
{
  // [20.5.4.1] Constructors

  {
    gdb::optional<tracker> o;
    VERIFY( !o );
  }

  {
    gdb::optional<tracker> o {};
    VERIFY( !o );
  }

  {
    gdb::optional<tracker> o = {};
    VERIFY( !o );
  }

  VERIFY( tracker::count == 0 );
}

} // namespace cons_default
