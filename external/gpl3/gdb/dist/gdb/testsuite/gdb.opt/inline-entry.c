/* Copyright 2024-2025 Free Software Foundation, Inc.

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

#include "attributes.h"

volatile int global = 0;

__attribute__((noinline)) ATTRIBUTE_NOCLONE void
foo (int arg)
{
  global += arg;
}

inline __attribute__((always_inline)) int
bar (int val)
{
  if (global == val)
    return 1;
  foo (1);
  return 1;
}

int
main (void)
{
  if ((global && bar (1)) || bar (2))
    return 0;
  return 1;
}
