/* This test program is part of GDB, the GNU debugger.

   Copyright 2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <string.h>

int
main (void)
{
  /* Some targets need 4-byte alignment for hardware watchpoints.  */
  char s[40] __attribute__ ((aligned (4)))
    = "This is a relatively long string...";
  char a[40] __attribute__ ((aligned (4)))
    = "String to be overwritten with zeroes";
  char b[40] __attribute__ ((aligned (4)))
    = "Another string to be memcopied...";
  char c[40] __attribute__ ((aligned (4)))
    = "Another string to be memmoved...";

  /* Break here.  */
  memset (a, 0, sizeof (a));

  memcpy (b, s, sizeof (b));

  memmove (c, s, sizeof (c));

  printf ("b = '%s'\n", b);
  printf ("c = '%s'\n", c);

  return 0;
}
