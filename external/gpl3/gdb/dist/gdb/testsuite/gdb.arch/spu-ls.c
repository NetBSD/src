/* Copyright 2010-2013 Free Software Foundation, Inc.

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

   This file is part of the gdb testsuite.

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>.
   Tests for SPU local-store access.  */

char *ptr = (char *)0x12345678;

char array[256];

int
main (unsigned long long speid, unsigned long long argp, 
      unsigned long long envp)
{
  return 0;
}

