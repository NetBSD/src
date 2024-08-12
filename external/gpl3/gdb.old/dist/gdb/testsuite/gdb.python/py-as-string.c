/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2023 Free Software Foundation, Inc.

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

enum EnumType {
  ENUM_VALUE_A,
  ENUM_VALUE_B,
  ENUM_VALUE_C,
  ENUM_VALUE_D,
};

static enum EnumType __attribute__ ((used)) enum_valid = ENUM_VALUE_B;
static enum EnumType __attribute__ ((used)) enum_invalid = (enum EnumType) 20;

int
main ()
{
  return 0;
}
