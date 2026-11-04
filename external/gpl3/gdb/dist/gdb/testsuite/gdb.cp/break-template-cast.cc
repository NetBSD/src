/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024-2025 Free Software Foundation, Inc.

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

/* Construct "foo<(enum_test)*>()" symbols.  These symbols can be tricky
   to handle, because of having a type cast inside the template parameter
   list, the "<(enum_test)*>" part.  The "<(" sequence in there can throw
   a wrench in "cp_search_name_hash()" function that tries to process
   things like "operator<(...)" while ignoring template parameter lists
   at the same time.

   If a breakpoint can be set on "foo", then all is in good order.  */

enum enum_test
{
  zero = 0,
  one
};

/* A template with a non-type parameter.  */
template <enum_test test>
void
foo ()
{
}

int
main ()
{
  /* Instantiate a "foo<(enum_test)1>()" symbol explicitly.  */
  foo<(enum_test)1> ();

  /* Some compilers, like g++, transform "enum_test::zero" to
     "(enum_test)0".  For such compilers, this "foo" instance
     would become "foo<(enum_test)0>()".  */
  foo<enum_test::zero> ();

  return 0;
}
