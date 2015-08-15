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

/* Put the path to the pretty-printer script in .debug_gdb_scripts so
   gdb will automagically loaded it.  */

#define DEFINE_GDB_SCRIPT(script_name) \
  asm("\
.pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n\
.byte 1\n\
.asciz \"" script_name "\"\n\
.popsection \n\
");

DEFINE_GDB_SCRIPT (SCRIPT_FILE)

struct ss
{
  int a;
  int b;
};

void
init_ss (struct ss *s, int a, int b)
{
  s->a = a;
  s->b = b;
}

int
main ()
{
  struct ss ss;

  init_ss (&ss, 1, 2);

  return 0;      /* break to inspect struct and union */
}
