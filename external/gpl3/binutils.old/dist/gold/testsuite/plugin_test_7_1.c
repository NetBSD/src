/* plugin_test_7_1.c -- a test case for the plugin API with GC.

   Copyright (C) 2010-2015 Free Software Foundation, Inc.
   Written by Rafael Avila de Espindola <espindola@google.com>.

   This file is part of gold.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

int fun1(void);
int fun2(void);
void set_x(int y);

#ifndef LTO
static int x = 0;

void set_x(int y)
{
  x = y;
}
#endif

int fun1(void)
{
#ifndef LTO
  if (x)
    return fun2();
#endif
  return 0;
}
