/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2014 Free Software Foundation, Inc.

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

#ifdef SYMBOL_PREFIX
#define SYMBOL(str)     SYMBOL_PREFIX #str
#else
#define SYMBOL(str)     #str
#endif

static void
foo ()
{}

void
pendfunc2 (int x)
{
  /* `set_point2' is the label where we'll set multiple tracepoints and
     breakpoints at.  The insn at the label must the large enough to
     fit a fast tracepoint jump.  */
  asm ("    .global " SYMBOL(set_point2) "\n"
       SYMBOL(set_point2) ":\n"
#if (defined __x86_64__ || defined __i386__)
       "    call " SYMBOL(foo) "\n"
#endif
       );
}
