/* Copyright 2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

__attribute__((__noinline__)) static int
callee (int x)
{
  return x * 2;
}

static inline int __attribute__((__always_inline__))
compute (int x)
{
  return callee (x);
}

static int
return_value (int x)
{
  int accum = 0;

  for (int i = 0; i < x; ++i)
    {
      int value = compute (i);
      if (value < 0)
	goto out_label;
    }

 out_label:

  return accum;
}

int
main ()
{
  int value = return_value (23);
  return value > 0 ? 0 : 1;
}
