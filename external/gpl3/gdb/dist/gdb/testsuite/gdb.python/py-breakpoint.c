/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

int result = 0;

int multiply (int i)
{
  return i * i;
}

int add (int i)
{
  return i + i; 
}


int main (int argc, char *argv[])
{
  int foo = 5;
  int bar = 42;
  int i;

  for (i = 0; i < 10; i++)
    {
      result += multiply (foo);  /* Break at multiply. */
      result += add (bar); /* Break at add. */
    }

  return 0; /* Break at end. */
}
