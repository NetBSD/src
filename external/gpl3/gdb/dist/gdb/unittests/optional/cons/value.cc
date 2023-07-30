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

namespace cons_value {

struct tracker
{
  tracker(int value) : value(value) { ++count; }
  ~tracker() { --count; }

  tracker(tracker const& other) : value(other.value) { ++count; }
  tracker(tracker&& other) : value(other.value)
  {
    other.value = -1;
    ++count;
  }

  tracker& operator=(tracker const&) = default;
  tracker& operator=(tracker&&) = default;

  int value;

  static int count;
};

int tracker::count = 0;

struct exception { };

struct throwing_construction
{
  explicit throwing_construction(bool propagate) : propagate(propagate) { }

  throwing_construction(throwing_construction const& other)
  : propagate(other.propagate)
  {
    if(propagate)
      throw exception {};
  }

  bool propagate;
};

static void
test ()
{
  // [20.5.4.1] Constructors

  {
    auto i = 0x1234ABCD;
    gdb::optional<long> o { i };
    VERIFY( o );
    VERIFY( *o == 0x1234ABCD );
    VERIFY( i == 0x1234ABCD );
  }

  {
    auto i = 0x1234ABCD;
    gdb::optional<long> o = i;
    VERIFY( o );
    VERIFY( *o == 0x1234ABCD );
    VERIFY( i == 0x1234ABCD );
  }

  {
    auto i = 0x1234ABCD;
    gdb::optional<long> o = { i };
    VERIFY( o );
    VERIFY( *o == 0x1234ABCD );
    VERIFY( i == 0x1234ABCD );
  }

  {
    auto i = 0x1234ABCD;
    gdb::optional<long> o { std::move(i) };
    VERIFY( o );
    VERIFY( *o == 0x1234ABCD );
    VERIFY( i == 0x1234ABCD );
  }

  {
    auto i = 0x1234ABCD;
    gdb::optional<long> o = std::move(i);
    VERIFY( o );
    VERIFY( *o == 0x1234ABCD );
    VERIFY( i == 0x1234ABCD );
  }

  {
    auto i = 0x1234ABCD;
    gdb::optional<long> o = { std::move(i) };
    VERIFY( o );
    VERIFY( *o == 0x1234ABCD );
    VERIFY( i == 0x1234ABCD );
  }

#ifndef GDB_OPTIONAL
  {
    std::vector<int> v = { 0, 1, 2, 3, 4, 5 };
    gdb::optional<std::vector<int>> o { v };
    VERIFY( !v.empty() );
    VERIFY( o->size() == 6 );
  }
#endif

  {
    std::vector<int> v = { 0, 1, 2, 3, 4, 5 };
    gdb::optional<std::vector<int>> o = v;
    VERIFY( !v.empty() );
    VERIFY( o->size() == 6 );
  }

  {
    std::vector<int> v = { 0, 1, 2, 3, 4, 5 };
    gdb::optional<std::vector<int>> o { v };
    VERIFY( !v.empty() );
    VERIFY( o->size() == 6 );
  }

  {
    std::vector<int> v = { 0, 1, 2, 3, 4, 5 };
    gdb::optional<std::vector<int>> o { std::move(v) };
    VERIFY( v.empty() );
    VERIFY( o->size() == 6 );
  }

  {
    std::vector<int> v = { 0, 1, 2, 3, 4, 5 };
    gdb::optional<std::vector<int>> o = std::move(v);
    VERIFY( v.empty() );
    VERIFY( o->size() == 6 );
  }

  {
    std::vector<int> v = { 0, 1, 2, 3, 4, 5 };
    gdb::optional<std::vector<int>> o { std::move(v) };
    VERIFY( v.empty() );
    VERIFY( o->size() == 6 );
  }

  {
    tracker t { 333 };
    gdb::optional<tracker> o = t;
    VERIFY( o->value == 333 );
    VERIFY( tracker::count == 2 );
    VERIFY( t.value == 333 );
  }

  {
    tracker t { 333 };
    gdb::optional<tracker> o = std::move(t);
    VERIFY( o->value == 333 );
    VERIFY( tracker::count == 2 );
    VERIFY( t.value == -1 );
  }

  enum outcome { nothrow, caught, bad_catch };

  {
    outcome result = nothrow;
    throwing_construction t { false };

    try
    {
      gdb::optional<throwing_construction> o { t };
    }
    catch(exception const&)
    { result = caught; }
    catch(...)
    { result = bad_catch; }

    VERIFY( result == nothrow );
  }

  {
    outcome result = nothrow;
    throwing_construction t { true };

    try
    {
      gdb::optional<throwing_construction> o { t };
    }
    catch(exception const&)
    { result = caught; }
    catch(...)
    { result = bad_catch; }

    VERIFY( result == caught );
  }

  {
    outcome result = nothrow;
    throwing_construction t { false };

    try
    {
      gdb::optional<throwing_construction> o { std::move(t) };
    }
    catch(exception const&)
    { result = caught; }
    catch(...)
    { result = bad_catch; }

    VERIFY( result == nothrow );
  }

  {
    outcome result = nothrow;
    throwing_construction t { true };

    try
    {
      gdb::optional<throwing_construction> o { std::move(t) };
    }
    catch(exception const&)
    { result = caught; }
    catch(...)
    { result = bad_catch; }

    VERIFY( result == caught );
  }

  {
#ifndef GDB_OPTIONAL
    gdb::optional<std::string> os = "foo";
#endif
    struct X
    {
      explicit X(int) {}
      X& operator=(int) {return *this;}
    };
#ifndef GDB_OPTIONAL
    gdb::optional<X> ox{42};
#endif
    gdb::optional<int> oi{42};
#ifndef GDB_OPTIONAL
    gdb::optional<X> ox2{oi};
#endif
    gdb::optional<std::string> os2;
    os2 = "foo";
#ifndef GDB_OPTIONAL
    gdb::optional<X> ox3;
    ox3 = 42;
    gdb::optional<X> ox4;
    ox4 = oi;
#endif
  }
  {
    // no converting construction.
#ifndef GDB_OPTIONAL
    gdb::optional<int> oi = gdb::optional<short>();
    VERIFY(!bool(oi));
    gdb::optional<std::string> os = gdb::optional<const char*>();
    VERIFY(!bool(os));
#endif
    gdb::optional<gdb::optional<int>> ooi = gdb::optional<int>();
    VERIFY(bool(ooi));
    ooi = gdb::optional<int>();
    VERIFY(bool(ooi));
    ooi = gdb::optional<int>(42);
    VERIFY(bool(ooi));
    VERIFY(bool(*ooi));
#ifndef GDB_OPTIONAL
    gdb::optional<gdb::optional<int>> ooi2 = gdb::optional<short>();
    VERIFY(bool(ooi2));
    ooi2 = gdb::optional<short>();
    VERIFY(bool(ooi2));
    ooi2 = gdb::optional<short>(6);
    VERIFY(bool(ooi2));
    VERIFY(bool(*ooi2));
    gdb::optional<gdb::optional<int>> ooi3 = gdb::optional<int>(42);
    VERIFY(bool(ooi3));
    VERIFY(bool(*ooi3));
    gdb::optional<gdb::optional<int>> ooi4 = gdb::optional<short>(6);
    VERIFY(bool(ooi4));
    VERIFY(bool(*ooi4));
#endif
  }
}

} // namespace cons_value
