/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2014 Free Software Foundation, Inc.

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

#include <spu_mfcio.h>

void
terminal_func ()
{
  spu_write_out_intr_mbox (0);
  spu_read_in_mbox ();
}

int
factorial_func (int value)
{
  if (value > 1)
    value *= factorial_func (value - 1);

  terminal_func ();
  return value;
}

int
main (unsigned long long speid, unsigned long long argp,
      unsigned long long envp)
{
  factorial_func (6);
  return 0;
}

