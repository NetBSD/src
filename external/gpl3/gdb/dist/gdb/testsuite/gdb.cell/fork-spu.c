/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2013 Free Software Foundation, Inc.

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

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>  */

#include <spu_mfcio.h>

int var;

void
func (void)
{
}

int
main (unsigned long long speid, unsigned long long argp,
      unsigned long long envp)
{
  /* Signal to PPU side that it should fork now.  */
  spu_write_out_intr_mbox (0);

  /* Wait until fork completed.  */
  spu_read_in_mbox ();

  /* Trigger watchpoint.  */
  var = 1;

  /* Now call some function to trigger breakpoint.  */
  func ();

  return 0;
}

