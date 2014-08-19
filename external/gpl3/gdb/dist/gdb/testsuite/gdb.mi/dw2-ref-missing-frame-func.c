/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2014 Free Software Foundation, Inc.

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

asm (".globl cu_text_start");
asm ("cu_text_start:");

asm (".globl func_nofb_start");
asm ("func_nofb_start:");

void
func_nofb (void)
{
  /* int func_nofb_var; */
  /* int func_nofb_var2; */

  extern void func_nofb_marker (void);
  func_nofb_marker ();
}

asm (".globl func_nofb_end");
asm ("func_nofb_end:");

asm (".globl func_loopfb_start");
asm ("func_loopfb_start:");

void
func_loopfb (void)
{
  /* int func_loopfb_var; */
  /* int func_loopfb_var2; */

  extern void func_loopfb_marker (void);
  func_loopfb_marker ();
}

asm (".globl func_loopfb_end");
asm ("func_loopfb_end:");

asm (".globl cu_text_end");
asm ("cu_text_end:");
