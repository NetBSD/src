/* plugin_test_wrap_symbols -- a test case for gold

   Copyright (C) 2017-2020 Free Software Foundation, Inc.
   Written by Sriraman Tallam <tmsriram@google.com>.

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

extern "C" {
int __real_hello();
int __wrap_hello()
{
  return __real_hello();
}
int hello();
int __real_jello();
int __wrap_jello()
{
  return __real_jello();
}
int jello();
int main()
{
  return 0;
}
}
