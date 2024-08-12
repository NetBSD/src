/* This test is part of GDB, the GNU debugger.

   Copyright 2007-2023 Free Software Foundation, Inc.

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

int leaf (void)
{
  return 1;
}

#ifdef __cplusplus
/* So that the alias attribute below work without having to figure out
   this function's mangled name.  */
int marker (int val) asm ("marker");
#endif

int marker (int val)
{
  leaf ();
  return leaf () * val;
}

int other (int val) __attribute__((alias("marker")));

int main(void)
{
  marker (0);
  marker (0);

  return 0;
}
