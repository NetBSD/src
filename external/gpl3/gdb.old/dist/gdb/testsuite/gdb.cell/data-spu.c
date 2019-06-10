/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Markus Deuling <deuling@de.ibm.com>  */

#include <stdio.h>

int
main (unsigned long long speid, unsigned long long argp,
      unsigned long long envp)
{
  char var_char = 'c';
  short var_short = 7;
  int var_int = 1337;
  long var_long = 123456;
  long long var_longlong = 123456789;
  float var_float = 1.23;
  double var_double = 2.3456;
  long double var_longdouble = 3.45678;
  return 0; /* Marker SPU End */
}

