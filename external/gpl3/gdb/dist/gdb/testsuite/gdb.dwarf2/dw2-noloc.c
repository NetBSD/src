/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2023 Free Software Foundation, Inc.

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

/* This is the value that all variables should have, here for convenience. */
#define VALUE 1234567890

/* These variables are here so that gcc adds them to the .symtab section
   on its own, instead of needing the DWARF assembler.  */
int file_locno_resolvable = VALUE;
int file_locempty_resolvable = VALUE;
int file_locaddr_resolvable = VALUE;
int main_local_locno_resolvable = VALUE;
int main_local_locempty_resolvable = VALUE;
int main_local_locaddr_resolvable = VALUE;
/* Despite these variables being marked as external in the debuginfo, if
   we do have them as external, the compiler won't add them to the .symtab
   section.  */
/* extern */ int file_extern_locno_resolvable = VALUE;
/* extern */ int file_extern_locempty_resolvable = VALUE;
/* extern */ int file_extern_locaddr_resolvable = VALUE;
/* extern */ int main_extern_locno_resolvable = VALUE;
/* extern */ int main_extern_locempty_resolvable = VALUE;
/* extern */ int main_extern_locaddr_resolvable = VALUE;

int
main (void)
{
  asm ("main_label: .global main_label");
  return 0;					/* main start */
}
