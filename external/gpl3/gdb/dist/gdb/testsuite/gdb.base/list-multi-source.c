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

extern int function_a (void);
extern int function_b (void);

#ifdef MAIN
int
main (void)
{
  int res;

  res = function_a ();

  res += function_b ();

  return res;
}
#endif

#if defined FILE_A || defined FILE_B
static int
get_value_common (void)
{
  /* NOTE: When reading this file in the source tree, the variable used in
     the return statement below will be replaced by a constant value when
     the file is copied into the source tree.  */
  return value_to_return;	/* List this line.  */
}
#endif

#ifdef FILE_A
int
function_a (void)
{
  return get_value_common ();
}
#endif

#ifdef FILE_B
int
function_b (void)
{
  return get_value_common ();
}
#endif
