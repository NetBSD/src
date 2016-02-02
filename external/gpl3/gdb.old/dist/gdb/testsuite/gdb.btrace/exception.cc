/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2015 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

static void
bad (void)
{
  throw 42;
}

static void
bar (void)
{
  bad ();
}

static void
foo (void)
{
  bar ();
}

static void
test (void)
{
  try
    {
      foo ();
    }
  catch (...)
    {
    }
}

int
main (void)
{
  test ();
  test (); /* bp.1  */
  return 0; /* bp.2  */
}
