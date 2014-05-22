/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2013 Free Software Foundation, Inc.

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

/* class C
     {
     public:
       static C s;
     };
   C C::s;
   C f()
   {
     return C::s;
   }  */

asm (".globl cu_text_start");
asm ("cu_text_start:");

asm (".globl f_start");
asm ("f_start:");

void
f (void)
{
}

asm (".globl f_end");
asm ("f_end:");

int
main (void)
{
  f ();
  return 0;
}

asm (".globl cu_text_end");
asm ("cu_text_end:");
