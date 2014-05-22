// gdb_index_test.cc -- a test case for the --gdb-index option.

// Copyright 2012 Free Software Foundation, Inc.
// Written by Cary Coutant <ccoutant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

// This source file defines a number of symbols of different forms
// to exercise the DWARF scanner in gold.

namespace
{
int c1_count;
int c2_count;
};

namespace one
{

enum G
{
  G_A,
  G_B,
  G_C
};

class c1
{
 public:
  static int count;

  c1()
  { ++c1_count; }

  ~c1()
  {
    --c1_count;
  }

  enum E
  {
    E_A,
    E_B,
    E_C,
  };

  int
  val()
  { return E_A; }
};

c1 c1v;
};

namespace two
{
const int ci = 3;

template <typename T>
class c2
{
 public:
  c2(T t)
    : t_(t)
  {
    ++c2_count;
  }

  ~c2()
  { --c2_count; }

  T
  val()
  { return this->t_; }

  T t_;
};

c2<int> c2v1(1);
c2<double> c2v2(2.0);
c2<int const*> c2v3(&ci);
};

enum F
{
  F_A,
  F_B,
  F_C
};

template <class C>
bool
check(C* c)
{ return c->val() == 0; }

bool
check_enum(int i)
{ return i > 0; }

struct anonymous_union_container {
  union {
    struct astruct {
      int a;
    };
    int b;
  } u;
};

anonymous_union_container anonymous_union_var;

int
main()
{
  F f = F_A;
  one::G g = one::G_A;
  check_enum(f);
  check_enum(g);
  check(&one::c1v);
  check(&two::c2v1);
  check(&two::c2v2);
  check(&two::c2v3);
  return anonymous_union_var.u.b;
}
