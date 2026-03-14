/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

/* Test that we can access class method/data member via reference.  */

struct foo
{
  foo () : m_a (42) {}
  int get_a () const { return m_a; }
  int m_a;
};

struct bar
{
  bar () : m_foo () {}
  const foo &get_foo () const { return m_foo; }
  foo m_foo;
};

int
main (int argc, char *argv[])
{
  bar b;
  const foo &ref = b.get_foo ();
  int ret = ref.m_a;  // breakpoint here
  ret += ref.get_a ();
  return ret;
}