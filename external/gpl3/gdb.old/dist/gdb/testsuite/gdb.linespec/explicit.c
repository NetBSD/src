/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2017 Free Software Foundation, Inc.

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

extern int myfunction2 (int arg);

static int
myfunction (int arg)
{
  int i, j, r;

  j = 0; /* myfunction location */
  r = arg;

 top:
  ++j;  /* top location */

  if (j == 10)
    goto done;

  for (i = 0; i < 10; ++i)
    {
      r += i;
      if (j % 2)
	goto top;
    }

 done:
  return r;
}

static int
my_unique_function_name (int arg)
{
  int j = 0;

  /* Just do something random.  We only care about the unique function
     name.  */
  if (arg == 50)
    j = 10;

  return j;
}

int
main (void)
{
  int i, j;

  /* Call the test function repeatedly, enough times for all our tests
     without running forever if something goes wrong.  */
  for (i = 0, j = 0; i < 1000; ++i)
    j += myfunction (0);

  my_unique_function_name (j);

  return myfunction2 (j);
}
